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

#ifndef __WSSERVER_H__
#define __WSSERVER_H__

#include "server_wss.hpp"
#include "IWsServer.hpp"


class VssCommandProcessor;
class VssCommandProcessor;
class SubscriptionHandler;
class Authenticator;
class VssDatabase;
class AccessChecker;
class ILogger;

class WsServer : public IWsServer {
 private:
  SimpleWeb::SocketServer<SimpleWeb::WSS> *secureServer_;
  SimpleWeb::SocketServer<SimpleWeb::WS> *insecureServer_;
  std::shared_ptr<ILogger> logger;
  bool isSecure_;
  std::string configFileName_;

 public:
  VssCommandProcessor* cmdProcessor;
  SubscriptionHandler* subHandler;
  Authenticator* tokenValidator;
  VssDatabase* database;
  AccessChecker* accessCheck;

  WsServer(std::shared_ptr<ILogger> loggerUtil, int port, std::string configFileName, bool secure);
  ~WsServer();
  void startServer(const std::string &endpointName);
  void sendToConnection(uint32_t connID, const std::string &message);
  void start();
};
#endif
