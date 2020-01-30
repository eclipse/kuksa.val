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

#include <memory>

#include "server_wss.hpp"

#include "IServer.hpp"


class IVssCommandProcessor;
class ILogger;

class WsServer : public IServer {
 private:
  SimpleWeb::SocketServer<SimpleWeb::WSS> *secureServer_;
  SimpleWeb::SocketServer<SimpleWeb::WS> *insecureServer_;
  std::shared_ptr<ILogger> logger;
  bool isSecure_;
  bool isInitialized_;

 public:
  std::shared_ptr<IVssCommandProcessor> cmdProcessor;

  WsServer();
  bool Initialize(std::shared_ptr<ILogger> loggerUtil,
                  std::shared_ptr<IVssCommandProcessor> processor,
                  bool secure,
                  int port);
  ~WsServer();
  void startServer(const std::string &endpointName);
  bool start();

  // IServer

  void AddListener(ObserverType, std::shared_ptr<IVssCommandProcessor>);
  void RemoveListener(ObserverType, std::shared_ptr<IVssCommandProcessor>);
  void SendToConnection(uint64_t connID, const std::string &message);
};
#endif
