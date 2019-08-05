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
#ifndef __SUBSCRIPTIONHANDLER_H__
#define __SUBSCRIPTIONHANDLER_H__

#include <mutex>
#include <queue>
#include "visconf.hpp"
#include <string>
#include <thread>

#include <jsoncons/json.hpp>

using namespace std;

class accesschecker;
class authenticator;
class vssdatabase;
class wschannel;
class wsserver;

class subscriptionhandler {
 private:
  uint32_t subscribeHandle[MAX_SIGNALS][MAX_CLIENTS];
  class wsserver* server;
  authenticator* validator;
  accesschecker* checkAccess;
  std::mutex subMutex;
  std::thread subThread;
  bool threadRun;
  queue<pair<uint32_t, jsoncons::json>> buffer;

 public:
  subscriptionhandler(wsserver* wserver,
                      authenticator* authenticate,
                      accesschecker* checkAccess);
  ~subscriptionhandler();

  uint32_t subscribe(wschannel& channel, vssdatabase* db,
                     uint32_t channelID, string path);
  int unsubscribe(uint32_t subscribeID);
  int unsubscribeAll(uint32_t connectionID);
  int update(int signalID, jsoncons::json value);
  int update(string path, jsoncons::json value);
  wsserver* getServer();
  int startThread();
  int stopThread();
  bool isThreadRunning();
  void* subThreadRunner();
};
#endif
