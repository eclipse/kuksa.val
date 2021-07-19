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

#include <condition_variable>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <string>
#include <thread>
#include <memory>

#include <jsoncons/json.hpp>

#include "visconf.hpp"
#include "ISubscriptionHandler.hpp"
#include "IAuthenticator.hpp"
#include "IAccessChecker.hpp"
#include "IServer.hpp"
#include "IPublisher.hpp"

class AccessChecker;
class Authenticator;
class VssDatabase;
class WsChannel;
class WsServer;
class ILogger;


using SubConnId = uint64_t;

// Subscription ID: Client ID
using subscriptions_t = std::unordered_map<SubscriptionId, SubConnId>;

// Subscription UUID
typedef std::string uuid_t;

class SubscriptionHandler : public ISubscriptionHandler {
 private:
  std::shared_ptr<ILogger> logger;
  std::unordered_map<uuid_t, subscriptions_t> subscribeHandle;
  std::shared_ptr<IServer> server;
  std::vector<std::shared_ptr<IPublisher>> publishers_;
  std::shared_ptr<IAuthenticator> validator;
  std::shared_ptr<IAccessChecker> checkAccess;
  mutable std::mutex subMutex;
  mutable std::mutex accessMutex;
  std::condition_variable c;
  std::thread subThread;
  bool threadRun;
  std::queue<std::tuple<SubscriptionId, ConnectionId, jsoncons::json>> buffer;

 public:
  SubscriptionHandler(std::shared_ptr<ILogger> loggerUtil,
                      std::shared_ptr<IServer> wserver,
                      std::shared_ptr<IAuthenticator> authenticate,
                      std::shared_ptr<IAccessChecker> checkAccess);
  ~SubscriptionHandler();

  void addPublisher(std::shared_ptr<IPublisher> publisher){
    publishers_.push_back(publisher);
  }
  SubscriptionId subscribe(WsChannel& channel,
                           std::shared_ptr<IVssDatabase> db,
                           const std::string &path);
  int unsubscribe(SubscriptionId subscribeID);
  int unsubscribeAll(ConnectionId connectionID);
  int updateByUUID(const std::string &signalUUID, const jsoncons::json &value);
  std::shared_ptr<IServer> getServer();
  int startThread();
  int stopThread();
  bool isThreadRunning() const;
  void* subThreadRunner();
};
#endif
