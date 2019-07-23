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

#include <queue>
#include "accesschecker.hpp"
#include "authenticator.hpp"
#include "exception.hpp"
#include "visconf.hpp"
#include "vssdatabase.hpp"
#include "wsserver.hpp"

using namespace std;
using namespace jsoncons;
using namespace jsoncons::jsonpath;
using jsoncons::json;

class subscriptionhandler {
 private:
  uint32_t subscribeHandle[MAX_SIGNALS][MAX_CLIENTS];
  class wsserver* server;
  authenticator* validator;
  accesschecker* checkAccess;
  bool threadRun;

 public:
  queue<pair<uint32_t, json>> buffer;
  subscriptionhandler(class wsserver* wserver,
                      class authenticator* authenticate,
                      class accesschecker* checkAccess);
  uint32_t subscribe(class wschannel& channel, class vssdatabase* db,
                     uint32_t channelID, string path);
  int unsubscribe(uint32_t subscribeID);
  int unsubscribeAll(uint32_t connectionID);
  int update(int signalID, json value);
  int update(string path, json value);
  class wsserver* getServer();
  int startThread();
  int stopThread();
  bool isThreadRunning();
};
#endif
