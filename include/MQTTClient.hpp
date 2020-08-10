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
#ifndef __MQTTCLIENT_H__
#define __MQTTCLIENT_H__

#include "IClient.hpp"
#include <regex>
#include "mosquitto.h"

class ILogger;

/**
 * \class MQTTClient 
 * \brief A MQTT client as publisher
 *        Implementation of \ref IClient interface, based on mosquittopp library
 */
class MQTTClient : public IClient
 {
  private:
    std::shared_ptr<ILogger> logger_;
    std::vector<std::string> paths_;
    int keepalive_;
    int qos_;
    int connection_retry_;
    struct mosquitto *mosq_ = nullptr;
    bool isConnected_ = false;
    std::string prefix_{};
    const std::string host_;
    const int port_;

  public:
    /**
     * @brief Initialize Boost.Beast server
     * @param loggerUtil Hostname for server connection
     * @param id client id
     * @param host broker host
     * @param port Port where to wait for connections
     * @param keepalive the number of seconds after which the broker should send a PING
     * @param qos integer value 0, 1 or 2 indicating the Quality of Service to be used for the message
     *              message to the client if no other messages have been exchanged
     *              in that time. Default is 60s
     */
    MQTTClient(std::shared_ptr<ILogger> loggerUtil, const std::string &id, const std::string& host, int port, bool insecure = false,  int keepalive=60, int qos=0, int connection_retry = 3);
    ~MQTTClient();

    /**
     * @brief Start client
     */
    bool start();
    void addPrefix(const std::string& prefix);
    void addPublishPath(const std::string& path);

    /**
     * @brief Set username and password for this mqtt client
     */
    bool setUsernamePassword(const 	std::string& username, const std::string& password);

    // IClient
    bool sendPathValue(const std::string &path, const jsoncons::json &value) override;

};



#endif /* __MQTTCLIENT_H__ */

