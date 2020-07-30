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
    struct mosquitto *mosq_;

    bool isInitialized = false;

  public:
    /**
     * @brief Initialize Boost.Beast server
     * @param loggerUtil Hostname for server connection
     * @param id client id
     * @param host broker host
     * @param port Port where to wait for connections
     * @param keepalive the number of seconds after which the broker should send a PING
     *              message to the client if no other messages have been exchanged
     *              in that time. Default is 60s
     */
    MQTTClient(std::shared_ptr<ILogger> loggerUtil, const char *id, const std::string& host, int port, int keepalive=60);
    ~MQTTClient();

    /**
     * @brief Start client
     */
    void Start();

    // IClient
    bool SendMsg(const char *topic, int payloadlen=0, const void *payload=NULL);
    bool SendPathValue(const std::string &path, const jsoncons::json &value) override;

};



#endif /* __MQTTCLIENT_H__ */

