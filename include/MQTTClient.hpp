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

#include "IClient.hpp"
#include <mosquittopp.h>
#include <boost/asio/ssl/context.hpp>
#include <vector>
#include <mutex>

class ILogger;

/**
 * \class MQTTClient 
 * \brief A MQTT client as publisher
 *        Implementation of \ref IClient interface, based on mosquittopp library
 */
class MQTTClient : public IClient, public mosqpp::mosquittopp
 {
  private:
    std::mutex mutex_;
    std::shared_ptr<ILogger> logger_;

    bool isInitialized = false;
    std::string docRoot_;

    const uint8_t NumOfThreads = 1;

    void on_connect(int rc);
	void on_message(const struct mosquitto_message *message);
	void on_subscribe(int mid, int qos_count, const int *granted_qos);

  public:
    MQTTClient(std::shared_ptr<ILogger> loggerUtil, const char *id, const char *host, int port);
    ~MQTTClient();

    /**
     * @brief Initialize Boost.Beast server
     * @param host Hostname for server connection
     * @param port Port where to wait for server connections
     * @param docRoot URL path that is handled
     * @param certPath Directory path where 'Server.pem' and 'Server.key' are located
     * @param allowInsecure If true, plain connections are allowed, otherwise SSL is mandatory
     */
    void Initialize(std::string host,
                    int port,
                    std::string && docRoot,
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
    void SendToConnection(ConnectionId connID, const std::string &message);

};



#endif /* __WEBSOCKHTTPFLEXSERVER_H__ */

