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

#include <unistd.h> // usleep
#include <string>

#include <jsoncons_ext/jsonpath/json_query.hpp>

#include "AccessChecker.hpp"
#include "Authenticator.hpp"
#include "exception.hpp"
#include "visconf.hpp"
#include "VssDatabase.hpp"
#include "WsServer.hpp"
#include "ILogger.hpp"

using namespace std;
using namespace jsoncons::jsonpath;
using jsoncons::json;

SubscriptionHandler::SubscriptionHandler(std::shared_ptr<ILogger> loggerUtil,
                                         std::shared_ptr<IServer> wserver,
                                         std::shared_ptr<IAuthenticator> authenticate,
                                         std::shared_ptr<IAccessChecker> checkAcc) {
  logger = loggerUtil;
  server = wserver;
  validator = authenticate;
  checkAccess = checkAcc;
  startThread();
}

SubscriptionHandler::~SubscriptionHandler() {
  stopThread();
}

uint64_t SubscriptionHandler::subscribe(WsChannel& channel,
                                        std::shared_ptr<IVssDatabase> db,
                                        const string &path) {
  // generate subscribe ID "randomly".
  uint64_t subId = rand() % 9999999;
  // embed connection ID into subID.
  subId = channel.getConnID() + subId;

  bool isBranch = false;
  string jPath = db->getVSSSpecificPath(path, isBranch, db->data_tree);

  if (jPath == "") {
    throw noPathFoundonTree(path);
  } else if (!checkAccess->checkReadAccess(channel, jPath)) {
    stringstream msg;
    msg << "no permission to subscribe to path";
    throw noPermissionException(msg.str());
  }

  jsoncons::json resArray = jsonpath::json_query(db->data_tree, jPath);

  if (resArray.is_array() && resArray.size() == 1) {
    jsoncons::json result = resArray[0];
    string sigUUID = result["uuid"].as<string>();
    auto handle = subscribeHandle.find(sigUUID);

    if (handle != subscribeHandle.end()) {
      logger->Log(LogLevel::VERBOSE, string("SubscriptionHandler::subscribe: Updating the previous subscribe ")
                  + string("ID with a new one"));
    }

    subscribeHandle[sigUUID][subId] = channel.getConnID();

    return subId;
  } else if (resArray.is_array()) {
    logger->Log(LogLevel::INFO, "SubscriptionHandler::subscribe :signals found in path" + path
                + "Array size: " + to_string(resArray.size())
                + ". Subscribe works for 1 signal at a time");
    stringstream msg;
    msg << "signals found in path" << path
        << ". Subscribe works for 1 signal at a time";
    throw noPathFoundonTree(msg.str());
  } else {
    logger->Log(LogLevel::ERROR, string("SubscriptionHandler::subscribe: some error occurred while adding ")
                + string("subscription"));
    stringstream msg;
    msg << "some error occured while adding subscription for path = " << path;
    throw genException(msg.str());
  }
}

int SubscriptionHandler::unsubscribe(uint32_t subscribeID) {
  for (auto& uuid : subscribeHandle) {
    auto subscriptions = &(uuid.second);
    auto subscription = subscriptions->find(subscribeID);
    if (subscription != subscriptions->end()) {
      subscriptions->erase(subscription);
    }
  }
  return 0;
}

int SubscriptionHandler::unsubscribeAll(uint32_t connectionID) {
  for (auto& uuid : subscribeHandle) {
    auto subscriptions = &(uuid.second);
    for (auto& subscription : *subscriptions) {
      if (subscription.second == (connectionID)) {
        subscriptions->erase(subscription.first);
      }
    }
  }
  return 0;
}

int SubscriptionHandler::updateByUUID(const string &signalUUID,
                                      const jsoncons::json &value) {
  auto handle = subscribeHandle.find(signalUUID);
  if (handle == subscribeHandle.end()) {
    // UUID not found
    return 0;
  }

  for (auto subID : handle->second) {
    std::lock_guard<std::mutex> lock(subMutex);
    pair<uint32_t, json> newSub;
    newSub = std::make_pair(subID.second, value);
    buffer.push(newSub);
  }

  return 0;
}

std::shared_ptr<IServer> SubscriptionHandler::getServer() {
  return server;
}

int SubscriptionHandler::updateByPath(const string &path, const json &value) {
  /* TODO: Implement */
  (void) path;
  (void) value;

  return 0;
}

void* SubscriptionHandler::subThreadRunner() {
  logger->Log(LogLevel::VERBOSE, "SubscribeThread: Started Subscription Thread!");

  while (isThreadRunning()) {
    if (buffer.size() > 0) {
      std::lock_guard<std::mutex> lock(subMutex);
      auto newSub = buffer.front();
      buffer.pop();

      auto connId = newSub.first;
      jsoncons::json value = newSub.second;

      jsoncons::json answer;
      answer["action"] = "subscribe";
      answer["subscriptionId"] = connId;
      answer.insert_or_assign("value", value);
      answer["timestamp"] = time(NULL);

      stringstream ss;
      ss << pretty_print(answer);
      string message = ss.str();

      getServer()->SendToConnection(connId, message);
    }
    // sleep 10 ms
    usleep(10000);
  }
  logger->Log(LogLevel::INFO, "SubscribeThread: Subscription handler thread stopped running");

  return NULL;
}

int SubscriptionHandler::startThread() {
  subThread = thread(&SubscriptionHandler::subThreadRunner, this);
  threadRun = true;
  return 0;
}

int SubscriptionHandler::stopThread() {
  std::lock_guard<std::mutex> lock(subMutex);
  threadRun = false;
  subThread.join();
  return 0;
}

bool SubscriptionHandler::isThreadRunning() const { return threadRun; }
