/*
 * ******************************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 * *****************************************************************************
 */
#include "SubscriptionHandler.hpp"

#include <unistd.h>  // usleep
#include <string>

#include <boost/uuid/uuid_generators.hpp> 
#include <boost/uuid/uuid_io.hpp>         

#include <jsonpath/json_query.hpp>

#include "AccessChecker.hpp"
#include "Authenticator.hpp"
#include "ILogger.hpp"
#include "JsonResponses.hpp"
#include "VssDatabase.hpp"
#include "WsChannel.hpp"
#include "exception.hpp"
#include "visconf.hpp"

using namespace std;
using namespace jsoncons::jsonpath;
using jsoncons::json;

SubscriptionHandler::SubscriptionHandler(
    std::shared_ptr<ILogger> loggerUtil, std::shared_ptr<IServer> wserver,
    std::shared_ptr<IAuthenticator> authenticate,
    std::shared_ptr<IAccessChecker> checkAcc)
    : publishers_() {
  logger = loggerUtil;
  server = wserver;
  validator = authenticate;
  checkAccess = checkAcc;
  startThread();
}

SubscriptionHandler::~SubscriptionHandler() { stopThread(); }

SubscriptionId SubscriptionHandler::subscribe(kuksa::kuksaChannel& channel,
                                              std::shared_ptr<IVssDatabase> db,
                                              const string& path) {
  // generate subscribe ID "randomly".
  SubscriptionId subId = boost::uuids::random_generator()();

  VSSPath vssPath = VSSPath::fromVSS(path);

  if (!db->pathExists(vssPath)) {
    throw noPathFoundonTree(path);
  } else if (!db->pathIsReadable(vssPath)) {
    stringstream msg;
    msg << path
        << " is not a sensor, actor or attribute leaf. Subscribe not supported";
    logger->Log(LogLevel::INFO,
                "SubscriptionHandler::subscribe : " + msg.str());
    throw noPathFoundonTree(msg.str());
  } else if (!checkAccess->checkReadAccess(channel, vssPath)) {
    stringstream msg;
    msg << "no permission to subscribe to path " << path;
    throw noPermissionException(msg.str());
  }

  auto existingsubscription = subscriptions.find(vssPath);
  if (existingsubscription != subscriptions.end()) {
    logger->Log(LogLevel::VERBOSE, string("SubscriptionHandler::subscribe: "
                                          "Updating the previous subscribe ") +
                                       string("ID with a new one"));
  }
  logger->Log(LogLevel::VERBOSE,
              string("SubscriptionHandler::subscribe: Subscribing to ") +
                  vssPath.getVSSPath());

  std::unique_lock<std::mutex> lock(accessMutex);                
  subscriptions[vssPath][subId] = channel.connectionid();
  return subId;
}

int SubscriptionHandler::unsubscribe(SubscriptionId subscribeID) {
  bool found_subscription=false;
  logger->Log(LogLevel::VERBOSE,
              string("SubscriptionHandler::unsubscribe: Unsubscribe on ") +
                  boost::uuids::to_string(subscribeID));
  std::unique_lock<std::mutex> lock(accessMutex);
  for (auto& sub : subscriptions) {
    auto subsforpath = &(sub.second);
    auto subid = subsforpath->find(subscribeID);
    if (subid != subsforpath->end()) {
      logger->Log(
          LogLevel::VERBOSE,
          string("SubscriptionHandler::unsubscribe: Unsubscribing path ") +
              sub.first.getVSSPath());
      subsforpath->erase(subid);
      found_subscription=true;
    }
  }
  if (found_subscription)
    return 0;
  return -1;
}

int SubscriptionHandler::unsubscribeAll(ConnectionId connectionID) {
  logger->Log(LogLevel::VERBOSE,
              string("SubscriptionHandler::unsubscribeAll: Unsubscribing all "
                     "for connectionId ") +
                  std::to_string(connectionID));

  std::unique_lock<std::mutex> lock(accessMutex);
  for (auto& subs : subscriptions) {
    auto condition =
        [connectionID](const std::pair<SubscriptionId, ConnectionId>& pair) {
          return (pair.second == (connectionID));
        };

    auto found =
        std::find_if(std::begin(subs.second), std::end(subs.second), condition);

    if (found != std::end(subs.second)) {
      subs.second.erase(found);
      logger->Log(LogLevel::VERBOSE, "SubscriptionHandler::unsubscribeAll: Unsubscribing " +
                  subs.first.getVSSPath() + " for "+std::to_string(connectionID) );
    }
  }
  return 0;
}

std::shared_ptr<IServer> SubscriptionHandler::getServer() { return server; }

int SubscriptionHandler::publishForVSSPath(const VSSPath path,
                                           const jsoncons::json& data) {
  // Publish MQTT
  for (auto& publisher : publishers_) {
    publisher->sendPathValue(path.getVSSPath(), data["dp"]["value"]);
  }
  std::stringstream ss;
  ss << "SubscriptionHandler::publishForVSSPath: set value "
     << data["dp"]["value"] << " for path " << path.to_string();
  logger->Log(LogLevel::VERBOSE, ss.str());

  std::unique_lock<std::mutex> lock(accessMutex);
  auto handle = subscriptions.find(path);
  if (handle == subscriptions.end()) {
    // no subscriptions for path
    return 0;
  }

  for (auto subID : handle->second) {
    std::lock_guard<std::mutex> lock(subMutex);
    tuple<SubscriptionId, ConnectionId, json> newSub;
    logger->Log(
        LogLevel::VERBOSE,
        "SubscriptionHandler::publishForVSSPath: new value set at path " +
            boost::uuids::to_string(subID.first) + ": " + ss.str());
    newSub = std::make_tuple(subID.first, subID.second, data);
    buffer.push(newSub);
    c.notify_one();
  }
  return 0;
}

void* SubscriptionHandler::subThreadRunner() {
  logger->Log(LogLevel::VERBOSE,
              "SubscribeThread: Started Subscription Thread!");

  while (isThreadRunning()) {
    if (buffer.size() > 0) {
      std::unique_lock<std::mutex> lock(subMutex);
      auto newSub = buffer.front();
      buffer.pop();

      auto connId = std::get<1>(newSub);
      jsoncons::json data = std::get<2>(newSub);

      jsoncons::json answer;
      answer["action"] = "subscription";
      answer["subscriptionId"] = boost::uuids::to_string(std::get<0>(newSub));
      answer.insert_or_assign("data", data);

      stringstream ss;
      ss << pretty_print(answer);
      string message = ss.str();

      bool connectionexist=getServer()->SendToConnection(connId, message);
      if(!connectionexist) {
        this->unsubscribeAll(connId);
      }
    } else {
      std::unique_lock<std::mutex> lock(subMutex);
      c.wait(lock);
    }
  }

  logger->Log(LogLevel::VERBOSE,
              "SubscribeThread: Subscription handler thread stopped running");

  return NULL;
}

int SubscriptionHandler::startThread() {
  subThread = thread(&SubscriptionHandler::subThreadRunner, this);
  threadRun = true;
  return 0;
}

int SubscriptionHandler::stopThread() {
  if (isThreadRunning()) {
    {
      std::lock_guard<std::mutex> lock(subMutex);
      threadRun = false;
      c.notify_one();
    }
    subThread.join();
  }
  return 0;
}

bool SubscriptionHandler::isThreadRunning() const { return threadRun; }
