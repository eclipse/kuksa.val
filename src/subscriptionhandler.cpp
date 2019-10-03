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
#include "subscriptionhandler.hpp"

#include <unistd.h> // usleep
#include <string>

#include <jsoncons_ext/jsonpath/json_query.hpp>

#include "accesschecker.hpp"
#include "Authenticator.hpp"
#include "exception.hpp"
#include "visconf.hpp"
#include "vssdatabase.hpp"
#include "WsServer.hpp"
#include "ILogger.hpp"

using namespace std;
// using namespace jsoncons;
using namespace jsoncons::jsonpath;
// using jsoncons::jsoncons::jsoncons::json;

subscriptionhandler::subscriptionhandler(std::shared_ptr<ILogger> loggerUtil,
                                         WsServer* wserver,
                                         Authenticator* authenticate,
                                         accesschecker* checkAcc) {
  logger = loggerUtil;
  server = wserver;
  validator = authenticate;
  checkAccess = checkAcc;
  startThread();
}

subscriptionhandler::~subscriptionhandler() {
  stopThread();
}

uint32_t subscriptionhandler::subscribe(wschannel& channel,
                                        vssdatabase* db,
                                        uint32_t channelID, string path) {
  // generate subscribe ID "randomly".
  uint32_t subId = rand() % 9999999;
  // embed connection ID into subID.
  subId = channelID + subId;

  bool isBranch = false;
  string jPath = db->getVSSSpecificPath(path, isBranch, db->data_tree);

  if (jPath == "") {
    throw noPathFoundonTree(path);
  } else if (!checkAccess->checkReadAccess(channel, jPath)) {
    stringstream msg;
    msg << "no permission to subscribe to path";
    throw noPermissionException(msg.str());
  }

  int clientID = channelID / CLIENT_MASK;
  jsoncons::json resArray = jsonpath::json_query(db->data_tree, jPath);

  if (resArray.is_array() && resArray.size() == 1) {
    jsoncons::json result = resArray[0];
    string sigUUID = result["uuid"].as<string>();
    auto handle = subscribeHandle.find(sigUUID);

    if (handle != subscribeHandle.end()) {
      logger->Log(LogLevel::VERBOSE, string("subscriptionhandler::subscribe: Updating the previous subscribe ")
                  + string("ID with a new one"));
    }

    subscribeHandle[sigUUID][subId] = clientID;

    return subId;
  } else if (resArray.is_array()) {
    logger->Log(LogLevel::INFO, "subscriptionhandler::subscribe :signals found in path" + path
                + "Array size: " + to_string(resArray.size())
                + ". Subscribe works for 1 signal at a time");
    stringstream msg;
    msg << "signals found in path" << path
        << ". Subscribe works for 1 signal at a time";
    throw noPathFoundonTree(msg.str());
  } else {
    logger->Log(LogLevel::ERROR, string("subscriptionhandler::subscribe: some error occurred while adding ")
                + string("subscription"));
    stringstream msg;
    msg << "some error occured while adding subscription for path = " << path;
    throw genException(msg.str());
  }
}

int subscriptionhandler::unsubscribe(uint32_t subscribeID) {
  for (auto& uuid : subscribeHandle) {
    auto subscriptions = &(uuid.second);
    auto subscription = subscriptions->find(subscribeID);
    if (subscription != subscriptions->end()) {
      subscriptions->erase(subscription);
    }
  }
  return 0;
}

int subscriptionhandler::unsubscribeAll(uint32_t connectionID) {
  for (auto& uuid : subscribeHandle) {
    auto subscriptions = &(uuid.second);
    for (auto& subscription : *subscriptions) {
      if (subscription.second == (connectionID / CLIENT_MASK)) {
        subscriptions->erase(subscription.first);
      }
    }
  }
  return 0;
}

int subscriptionhandler::updateByUUID(string UUID, jsoncons::json value) {
  auto handle = subscribeHandle.find(UUID);
  if (handle == subscribeHandle.end()) {
    // UUID not found
    return 0;
  }

  for (auto subID : handle->second) {
    subMutex.lock();
    pair<uint32_t, json> newSub;
    newSub = std::make_pair(subID.first, value);
    buffer.push(newSub);
    subMutex.unlock();
  }

  return 0;
}

WsServer* subscriptionhandler::getServer() {
  return server;
}

int subscriptionhandler::updateByPath(string path, json value) {
  /* TODO: Implement */
  (void) path;
  (void) value;

  return 0;
}

void* subscriptionhandler::subThreadRunner() {
  // subscriptionhandler* handler = (subscriptionhandler*)instance;

  logger->Log(LogLevel::VERBOSE, "SubscribeThread: Started Subscription Thread!");

  while (isThreadRunning()) {
    subMutex.lock();
    if (buffer.size() > 0) {
      pair<uint32_t, jsoncons::json> newSub = buffer.front();
      buffer.pop();

      uint32_t subID = newSub.first;
      jsoncons::json value = newSub.second;

      jsoncons::json answer;
      answer["action"] = "subscribe";
      answer["subscriptionId"] = subID;
      answer.insert_or_assign("value", value);
      answer["timestamp"] = time(NULL);

      stringstream ss;
      ss << pretty_print(answer);
      string message = ss.str();

      uint32_t connectionID = (subID / CLIENT_MASK) * CLIENT_MASK;
      getServer()->sendToConnection(connectionID, message);
    }
    subMutex.unlock();
    // sleep 10 ms
    usleep(10000);
  }
  logger->Log(LogLevel::INFO, "SubscribeThread: Subscription handler thread stopped running");

  return NULL;
}

int subscriptionhandler::startThread() {
  subThread = thread(&subscriptionhandler::subThreadRunner, this);
  /*
  if (pthread_create(&subscription_thread, NULL, &subThread, this)) {
    logger->Log(LogLevel::ERROR, "subscriptionhandler::startThread: Error creating subscription "
                + "handler thread");
    return 1;
  }
  */
  threadRun = true;
  return 0;
}

int subscriptionhandler::stopThread() {
  subMutex.lock();
  threadRun = false;
  subThread.join();
  subMutex.unlock();
  return 0;
}

bool subscriptionhandler::isThreadRunning() { return threadRun; }
