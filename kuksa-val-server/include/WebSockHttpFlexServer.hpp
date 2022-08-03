/**********************************************************************
 * Copyright (c) 2019-2022 Robert Bosch GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 **********************************************************************/

#ifndef __WEBSOCKHTTPFLEXSERVER_H__
#define __WEBSOCKHTTPFLEXSERVER_H__

#include "IServer.hpp"
#include "KuksaChannel.hpp"

#include <boost/asio/ssl/context.hpp>
#include <vector>
#include <string>
#include <mutex>
#include <memory>

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

    bool isInitialized = false;

    const uint8_t NumOfThreads = 1;
    boost::asio::io_context ioc_;

    /// Default name for server certificate file
    static const std::string serverCertFilename_;
    /// Default name for server key file
    static const std::string serverKeyFilename_;

    /**
     * @brief Load server SSL certificates
     * @param certPath Directory path where 'Server.pem' and 'Server.key' are located
     * @param ctx ssl context to which certificates will be added
     * @note 'Server.pem' and 'Server.key' needs to be located with executable
     */
    void LoadCertData(std::string & certPath, boost::asio::ssl::context& ctx);
    /**
     * @brief Handle incoming data requests
     * @param req_json Request message from connection
     * @param channel Connection identifier
     * @return Response JSON message for client
     */
    std::string HandleRequest(const std::string &req_json, KuksaChannel &channel);
  public:
    WebSockHttpFlexServer(std::shared_ptr<ILogger> loggerUtil);
    ~WebSockHttpFlexServer();

    /**
     * @brief Initialize Boost.Beast server
     * @param host Hostname for server connection
     * @param port Port where to wait for server connections
     * @param certPath Directory path where 'Server.pem' and 'Server.key' are located
     * @param allowInsecure If true, plain connections are allowed, otherwise SSL is mandatory
     */
    void Initialize(std::string host,
                    int port,
                    std::string certPath,
                    bool allowInsecure = false);
    /**
     * @brief Start server
     *        Server needs to be initialized before is started
     */
    void Start();

    // IServer

    void AddListener(ObserverType type,   std::shared_ptr<IVssCommandProcessor> listener);
    void RemoveListener(ObserverType type, std::shared_ptr<IVssCommandProcessor> listener);
    bool SendToConnection(ConnectionId connID, const std::string &message);
};



#endif /* __WEBSOCKHTTPFLEXSERVER_H__ */
