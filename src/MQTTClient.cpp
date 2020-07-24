/*
 * ******************************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
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

#include "MQTTClient.hpp"

#include "ILogger.hpp"

MQTTClient::MQTTClient(std::shared_ptr<ILogger> loggerUtil, const char *id, const char *host, int port)
 : logger_(loggerUtil),
{
}

MQTTClient::~MQTTClient() {
  ioc->stop(); // stop execution of io runner

  // wait to finish
  for(auto& thread : iocRunners) {
    thread.join();
  }
}

void MQTTClient::SendToConnection(ConnectionId connID, const std::string &message) {
  if (!isInitialized)
  {
    std::string err("Cannot send to connection, server not initialized!");
    logger_->Log(LogLevel::ERROR, err);
    throw std::runtime_error(err);
  }

  bool isFound = false;

  // try to find active connection to send data to

  if (!isFound) {
    auto session = reinterpret_cast<PlainWebsocketSession *>(connID);
    std::lock_guard<std::mutex> lock(connHandler.mPlainWebSock_);
    auto iter = connHandler.connPlainWebSock_.find(session);
    if (iter != std::end(connHandler.connPlainWebSock_))
    {
      isFound = true;
      session->write(message);
    }
  }
  if (!isFound) {
    auto session = reinterpret_cast<SslWebsocketSession *>(connID);
    std::lock_guard<std::mutex> lock(connHandler.mSslWebSock_);
    auto iter = connHandler.connSslWebSock_.find(session);
    if (iter != std::end(connHandler.connSslWebSock_))
    {
      isFound = true;
      session->write(message);
    }
  }
  if (!isFound) {
    auto session = reinterpret_cast<PlainHttpSession *>(connID);
    std::lock_guard<std::mutex> lock(connHandler.mPlainHttp_);
    auto iter = connHandler.connPlainHttp_.find(session);
    if (iter != std::end(connHandler.connPlainHttp_))
    {
      isFound = true;
      // TODO: check how we are going to handle ASYNC writes on HTTP connection?
    }
  }
  if (!isFound) {
    auto session = reinterpret_cast<SslHttpSession *>(connID);
    std::lock_guard<std::mutex> lock(connHandler.mSslHttp_);
    auto iter = connHandler.connSslHttp_.find(session);
    if (iter != std::end(connHandler.connSslHttp_))
    {
      isFound = true;
      // TODO: check how we are going to handle ASYNC writes on HTTP connection?
    }
  }
}

void MQTTClient::Start() {
  if (!isInitialized)
  {
    std::string err("Cannot start server, server not initialized!");
    logger_->Log(LogLevel::ERROR, err);
    throw std::runtime_error(err);
  }

  logger_->Log(LogLevel::INFO, "Starting Boost.Beast web-socket and http server");

  // start listening for connections
  connListener->run();

  // run the I/O service on the requested number of threads
  iocRunners.reserve(NumOfThreads);
  for(auto i = 0; i < NumOfThreads; ++i) {
    iocRunners.emplace_back(
      []
      {
        boost::system::error_code ec;
        ioc->run(ec);
      });
  }
}

