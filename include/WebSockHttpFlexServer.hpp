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
#ifndef __WEBSOCKHTTPFLEXSERVER_H__
#define __WEBSOCKHTTPFLEXSERVER_H__

#include "IServer.hpp"

#include <vector>
#include <string>
#include <mutex>
#include <memory>

class WsChannel;
class IRestHandler;
class ILogger;

/**
 * \class WebSockHttpFlexServer
 * \brief Combined Web-socket and HTTP server for both plain and SSL connections
 *        Implementation of \ref IServer interface, based on Boost.Beast library
 *        and its flex example code
 */
class WebSockHttpFlexServer : public IServer {
  private:
    std::vector<std::pair<ObserverType,std::shared_ptr<IVssCommandProcessor>>> listeners_;
    std::mutex mutex_;
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IRestHandler> rest2json_;

    const uint8_t NumOfThreads = 1;
    bool isInitialized = false;
    std::string docRoot_;

    /**
     * @brief Load server SSL certificates
     * @param ctx ssl context to which certificates will be added
     * @note 'Server.pem' and 'Server.key' needs to be located with executable
     */
    void LoadCertData(boost::asio::ssl::context& ctx);
    /**
     * @brief Handle incoming data requests
     * @param req_json Request message from connection
     * @param channel Connection identifier
     * @return Response JSON message for client
     */
    std::string HandleRequest(const std::string &req_json, WsChannel &channel);
  public:
    WebSockHttpFlexServer(std::shared_ptr<ILogger> loggerUtil,
                          std::shared_ptr<IRestHandler> rest2jsonUtil);
    ~WebSockHttpFlexServer();

    /**
     * @brief Initialize Boost.Beast server
     * @param host Hostname for server connection
     * @param port Port where to wait for server connections
     * @param docRoot URL path that is handled
     * @param allowInsecure If true, plain connections are allowed, otherwise SSL is mandatory
     */
    void Initialize(std::string host, int port, std::string && docRoot, bool allowInsecure = false);
    /**
     * @brief Start server
     *        Server needs to be initialized before is started
     */
    void Start();

    // IServer

    void AddListener(ObserverType type,   std::shared_ptr<IVssCommandProcessor> listener);
    void RemoveListener(ObserverType type, std::shared_ptr<IVssCommandProcessor> listener);
    void SendToConnection(uint64_t connID, const std::string &message);
};



#endif /* __WEBSOCKHTTPFLEXSERVER_H__ */
