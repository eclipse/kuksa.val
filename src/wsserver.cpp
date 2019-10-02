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
#include "wsserver.hpp"

#include "ILogger.hpp"
#include "accesschecker.hpp"
#include "authenticator.hpp"
#include "subscriptionhandler.hpp"
#include "visconf.hpp"
#include "vsscommandprocessor.hpp"
#include "vssdatabase.hpp"

#include <string>

using namespace std;

using WssServer = SimpleWeb::SocketServer<SimpleWeb::WSS>;
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;

uint16_t connections[MAX_CLIENTS + 1] = {0};
wsserver *wserver;

uint32_t generateConnID() {
  uint32_t retValueValue = 0;
  for (int i = 1; i < (MAX_CLIENTS + 1); i++) {
    if (connections[i] == 0) {
      connections[i] = i;
      retValueValue = CLIENT_MASK * (i);
      return retValueValue;
    }
  }
  return retValueValue;
}

wsserver::wsserver(std::shared_ptr<ILogger> loggerUtil, int port, string configFileName, bool secure) {
  logger = loggerUtil;
  isSecure_ = secure;
  secureServer_ = nullptr;
  insecureServer_ = nullptr;
  configFileName_ = configFileName;

  if (isSecure_) {
    secureServer_ = new WssServer("Server.pem", "Server.key");
    secureServer_->config.port = port;
  } else {
    insecureServer_ = new WsServer();
    insecureServer_->config.port = port;
  }

  tokenValidator = new authenticator("appstacle", "RS256");
  accessCheck = new accesschecker(tokenValidator);
  subHandler = new subscriptionhandler(this, tokenValidator, accessCheck);
  database = new vssdatabase(subHandler, accessCheck);
  cmdProcessor = new vsscommandprocessor(database, tokenValidator, subHandler);
  wserver = this;
}

wsserver::~wsserver() {
  delete cmdProcessor;
  delete database;
  delete subHandler;
  delete accessCheck;
  delete tokenValidator;
  if (secureServer_) {
    delete secureServer_;
  }
  if (insecureServer_) {
    delete insecureServer_;
  }
}

static void onMessage(std::weak_ptr<ILogger> wLogger,
                      shared_ptr<WssServer::Connection> connection,
                      string message) {
  auto logger = wLogger.lock();
  if (logger) {
    logger->Log(LogLevel::VERBOSE, "main::onMessage: Message received: \""
        + message + "\" from " + connection->remote_endpoint_address());
  }

  string response =
      wserver->cmdProcessor->processQuery(message, connection->channel);

  auto send_stream = make_shared<WssServer::SendStream>();
  *send_stream << response;
  connection->send(send_stream);
}

static void onMessage(std::weak_ptr<ILogger> wLogger,
                      shared_ptr<WsServer::Connection> connection,
                      string message) {
  auto logger = wLogger.lock();
  if (logger) {
    logger->Log(LogLevel::VERBOSE, "main::onMessage: Message received: \""
        + message + "\" from " + connection->remote_endpoint_address());
  }

  string response =
      wserver->cmdProcessor->processQuery(message, connection->channel);

  auto send_stream = make_shared<WsServer::SendStream>();
  *send_stream << response;
  connection->send(send_stream);
}

void wsserver::startServer(string endpointName) {
  (void) endpointName;

  auto wLogger = std::weak_ptr<ILogger>(logger);

  if (isSecure_) {
    auto &vssEndpoint = secureServer_->endpoint["^/vss/?$"];

    vssEndpoint.on_message = [wLogger](shared_ptr<WssServer::Connection> connection,
                                shared_ptr<WssServer::Message> message) {
      auto message_str = message->string();
      onMessage(wLogger, connection, message_str);

    };

    vssEndpoint.on_open = [wLogger](shared_ptr<WssServer::Connection> connection) {
      connection->channel.setConnID(generateConnID());

      auto logger = wLogger.lock();
      if (logger) {
        logger->Log(LogLevel::INFO, std::string("wsserver: Opened connection "
          + connection->remote_endpoint_address()
          + "conn ID "
          + to_string(connection->channel.getConnID())));
      }
    };

    // See RFC 6455 7.4.1. for status codes
    vssEndpoint.on_close = [wLogger](shared_ptr<WssServer::Connection> connection,
                              int status, const string & /*reason*/) {
      uint32_t clientID = connection->channel.getConnID() / CLIENT_MASK;
      connections[clientID] = 0;
      // removeAllSubscriptions(clientID);

      auto logger = wLogger.lock();
      if (logger) {
        logger->Log(LogLevel::INFO, "wsserver: Closed connection "
          + connection->remote_endpoint_address()
          + " with status code "
          + to_string(status));
      }
    };

    // See
    // http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html,
    // Error Codes for error code meanings
    vssEndpoint.on_error = [wLogger](shared_ptr<WssServer::Connection> connection,
                                     const SimpleWeb::error_code &ec) {
      uint32_t clientID = connection->channel.getConnID() / CLIENT_MASK;
      connections[clientID] = 0;
      // removeAllSubscriptions(clientID);
      auto logger = wLogger.lock();
      if (logger) {
        logger->Log(LogLevel::ERROR, "wsserver: Connection "
          + connection->remote_endpoint_address() + " with con ID "
          + to_string(connection->channel.getConnID()) + ". "
          + "Error: " + to_string(ec.value()) + ", error message: " + ec.message());
      }
    };

    logger->Log(LogLevel::INFO, "started Secure WS server");
    secureServer_->start();
  } else {
    auto &vssEndpoint = insecureServer_->endpoint["^/vss/?$"];

    vssEndpoint.on_message = [wLogger](shared_ptr<WsServer::Connection> connection,
                                       shared_ptr<WsServer::Message> message) {
      auto message_str = message->string();
      onMessage(wLogger, connection, message_str);

    };

    vssEndpoint.on_open = [wLogger](shared_ptr<WsServer::Connection> connection) {
      connection->channel.setConnID(generateConnID());

      auto logger = wLogger.lock();
      if (logger) {
        logger->Log(LogLevel::VERBOSE, "wsserver: Opened connection "
           + connection->remote_endpoint_address() + "conn ID "
           + to_string(connection->channel.getConnID()));
      }
    };

    // See RFC 6455 7.4.1. for status codes
    vssEndpoint.on_close = [wLogger](shared_ptr<WsServer::Connection> connection,
                                     int status, const string & /*reason*/) {
      uint32_t clientID = connection->channel.getConnID() / CLIENT_MASK;
      connections[clientID] = 0;
      // removeAllSubscriptions(clientID);
      auto logger = wLogger.lock();
      if (logger) {
        logger->Log(LogLevel::INFO, "wsserver: Closed connection "
           + connection->remote_endpoint_address() + " with status code "
           + to_string(status));
      }
    };

    // See
    // http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html,
    // Error Codes for error code meanings
    vssEndpoint.on_error = [wLogger](shared_ptr<WsServer::Connection> connection,
                                     const SimpleWeb::error_code &ec) {
      uint32_t clientID = connection->channel.getConnID() / CLIENT_MASK;
      connections[clientID] = 0;
      // removeAllSubscriptions(clientID);
      auto logger = wLogger.lock();
      if (logger) {
        logger->Log(LogLevel::ERROR, "wsserver: Connection "
           + connection->remote_endpoint_address() + " with con ID "
           + to_string(connection->channel.getConnID()) + ". "
           + "Error: " + to_string(ec.value()) + ", error message: " + ec.message());
      }
    };

    logger->Log(LogLevel::INFO, "started Insecure WS server");
    insecureServer_->start();
  }
}

void wsserver::sendToConnection(uint32_t connectionID, string message) {
  if (isSecure_) {
    auto send_stream = make_shared<WssServer::SendStream>();
    *send_stream << message;
    for (auto &a_connection : secureServer_->get_connections()) {
      if (a_connection->channel.getConnID() == connectionID) {
        a_connection->send(send_stream);
        return;
      }
    }
  } else {
    auto send_stream = make_shared<WsServer::SendStream>();
    *send_stream << message;
    for (auto &a_connection : insecureServer_->get_connections()) {
      if (a_connection->channel.getConnID() == connectionID) {
        a_connection->send(send_stream);
        return;
      }
    }
  }
}

void *startWSServer(void *arg) {
  (void) arg;
  wserver->startServer("");
  while (1) {
      usleep(1000000);
  }
  return NULL;
}

vssdatabase* wsserver::start() {
  this->database->initJsonTree(configFileName_);
  pthread_t startWSServer_thread;

  /* create the web socket server thread. */
  if (pthread_create(&startWSServer_thread, NULL, &startWSServer, NULL)) {
    logger->Log(LogLevel::ERROR, "main: Error creating websocket server run thread");
  }
  return this->database;
}
