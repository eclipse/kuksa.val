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

#include <jsoncons_ext/jsonpath/json_query.hpp>

#include "accesschecker.hpp"
#include "authenticator.hpp"
#include "exception.hpp"
#include "visconf.hpp"
#include "vssdatabase.hpp"
#include "wsserver.hpp"

// using namespace jsoncons;
using namespace jsoncons::jsonpath;
// using jsoncons::jsoncons::jsoncons::json;

subscriptionhandler::subscriptionhandler(wsserver* wserver,
                                         authenticator* authenticate,
                                         accesschecker* checkAcc) {
  for (int i = 0; i < MAX_SIGNALS; i++) {
    for (int j = 0; j < MAX_CLIENTS; j++) {
      subscribeHandle[i][j] = 0;
    }
  }
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
    int sigID = result["id"].as<int>();
#ifdef DEBUG
    if (subscribeHandle[sigID][clientID] != 0) {
      cout << "subscriptionhandler::subscribe: Updating the previous subscribe "
              "ID with a new one"
           << endl;
    }
#endif
    subscribeHandle[sigID][clientID] = subId;
    return subId;
  } else if (resArray.is_array()) {
    cout << resArray.size()
         << "subscriptionhandler::subscribe :signals found in path" << path
         << ". Subscribe works for 1 signal at a time" << endl;
    stringstream msg;
    msg << "signals found in path" << path
        << ". Subscribe works for 1 signal at a time";
    throw noPathFoundonTree(msg.str());
  } else {
    cout << "subscriptionhandler::subscribe: some error occured while adding "
            "subscription"
         << endl;
    stringstream msg;
    msg << "some error occured while adding subscription for path = " << path;
    throw genException(msg.str());
  }
}

int subscriptionhandler::unsubscribe(uint32_t subscribeID) {
  int clientID = subscribeID / CLIENT_MASK;
  for (int i = 0; i < MAX_SIGNALS; i++) {
    if (subscribeHandle[i][clientID] == subscribeID) {
      subscribeHandle[i][clientID] = 0;
      return 0;
    }
  }
  return -1;
}

int subscriptionhandler::unsubscribeAll(uint32_t connectionID) {
  for (int i = 0; i < MAX_SIGNALS; i++) {
    subscribeHandle[i][connectionID] = 0;
  }
#ifdef DEBUG
  cout << "subscriptionhandler::unsubscribeAll: Removed all subscriptions for "
          "client with ID ="
       << connectionID * CLIENT_MASK << endl;
#endif
  return 0;
}

int subscriptionhandler::update(int signalID, jsoncons::json value) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    uint32_t subID = subscribeHandle[signalID][i];
    if (subID != 0) {
      subMutex.lock();
      pair<uint32_t, jsoncons::json> newSub;
      newSub = std::make_pair(subID, value);
      buffer.push(newSub);
      subMutex.unlock();
    }
  }
  return 0;
}

wsserver* subscriptionhandler::getServer() {
  return server;
}

int subscriptionhandler::update(string path, json value) {
  /* TODO: Implement */
  (void) path;
  (void) value;

  return 0;
}

void* subscriptionhandler::subThreadRunner() {
  // subscriptionhandler* handler = (subscriptionhandler*)instance;
#ifdef DEBUG
  cout << "SubscribeThread: Started Subscription Thread!" << endl;
#endif
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
  cout << "SubscribeThread: Subscription handler thread stopped running"
       << endl;

  return NULL;
}

int subscriptionhandler::startThread() {
  subThread = thread(&subscriptionhandler::subThreadRunner, this);
  // if (pthread_create(&subscription_thread, NULL, &subThread, this)) {
  //   cout << "subscriptionhandler::startThread: Error creating subscription "
  //           "handler thread"
  //        << endl;
  //   return 1;
  // }
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
