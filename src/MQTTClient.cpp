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

#include <regex>
#include "MQTTClient.hpp"

#include "ILogger.hpp"

MQTTClient::MQTTClient(std::shared_ptr<ILogger> loggerUtil, const char *id, const std::string & host, int port, int keepalive)
 : logger_(loggerUtil){
    mosquitto_lib_init();
    mosq_ = mosquitto_new(id, true, this);

    int rc = mosquitto_connect_async(mosq_, host.c_str(), port, keepalive);
	if(rc){
        logger_->Log(LogLevel::ERROR, std::string("Connection Error: ")+std::string(mosquitto_strerror(rc)));
        throw std::runtime_error(mosquitto_strerror(rc));
	}
    isInitialized = (rc == 0);
}

MQTTClient::~MQTTClient() { 
    mosquitto_disconnect(mosq_);
    mosquitto_loop_stop(mosq_, false);
    mosquitto_destroy(mosq_);
    mosquitto_lib_cleanup();
}
bool MQTTClient::SendMsg(const char *topic, int payloadlen, const void *payload) {
  if (!isInitialized)
  {
    std::string err("Cannot send to connection, server not initialized!");
    logger_->Log(LogLevel::ERROR, err);
    throw std::runtime_error(err);
  }
  int rc = mosquitto_publish(mosq_, NULL, topic, payloadlen, payload, 0, false);
	if(rc){
        logger_->Log(LogLevel::ERROR, std::string("Connection Error: ")+std::string(mosquitto_strerror(rc)));
        throw std::runtime_error(mosquitto_strerror(rc));
	}
    return (rc == 0);
}

bool MQTTClient::SendPathValue(const std::string &path, const jsoncons::json &value) {
    logger_->Log(LogLevel::VERBOSE, "MQTTClient::SendPathValue: send path " + path + " value " + value.as_string() + " length " + std::to_string(value.as_string().size()));
    // TODO better parsing
    const std::regex regex("([a-zA-Z]+)");
    std::smatch matches; 
    std::string topic;
    std::string pathString(path);
    while (std::regex_search (pathString,matches,regex)) {
        if(matches[1] == std::string("children")){
            pathString = matches.suffix().str();
            continue;
        }
        if(topic.size() > 0){
            topic += "/";
        }
        topic += matches[1];
        pathString = matches.suffix().str();
    }
    logger_->Log(LogLevel::VERBOSE, "MQTTClient::SendPathValue: Topic is " + topic);
    SendMsg(topic.c_str(), value.as_string().size(), value.as_cstring());
    return true;
}

void MQTTClient::Start() {
  if (!isInitialized)
  {
    std::string err("Cannot start client, client not initialized!");
    logger_->Log(LogLevel::ERROR, err);
    throw std::runtime_error(err);
  }
  mosquitto_loop_start(mosq_);

}

