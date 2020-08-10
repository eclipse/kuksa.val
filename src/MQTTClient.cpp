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

#include <sstream>

#include "MQTTClient.hpp"

#include "ILogger.hpp"

MQTTClient::MQTTClient(std::shared_ptr<ILogger> loggerUtil,
                       const std::string& id, const std::string& host, int port,
                       bool insecure, int keepalive, int qos,
                       int connection_retry)
    : logger_(loggerUtil),
      paths_(),
      keepalive_(keepalive),
      qos_(qos),
      connection_retry_(connection_retry),
      host_(host),
      port_(port) {
  mosquitto_lib_init();
  mosq_ = mosquitto_new(id.c_str(), true, this);
  if (insecure) {
    int rc = mosquitto_tls_insecure_set(mosq_, true);
    if (MOSQ_ERR_SUCCESS != rc) {
      logger_->Log(LogLevel::ERROR,
                   std::string("MQTT Problem setting TLS insecure option: ") +
                       std::string(mosquitto_strerror(rc)));
      mosquitto_lib_cleanup();
    } else {
      logger_->Log(LogLevel::WARNING,
                   std::string("MQTT tls set to be insecure. Do not use it in "
                               "production code!"));
      mosquitto_lib_cleanup();
    }
  }
}

bool MQTTClient::start() {
  if (isConnected_ || paths_.empty()) {
    return isConnected_;
  }
  for (int retry = 0; retry < connection_retry_; retry++) {
    int rc = mosquitto_connect_async(mosq_, host_.c_str(), port_, keepalive_);
    if (rc != MOSQ_ERR_SUCCESS) {
      logger_->Log(LogLevel::ERROR, std::string("MQTT Connection Error: ") +
                                        std::string(mosquitto_strerror(rc)));
      isConnected_ = false;
    } else {
      logger_->Log(LogLevel::INFO, std::string("Connect to MQTT server ") +
                                       host_ + ":" + std::to_string(port_));
      isConnected_ = true;
      mosquitto_loop_start(mosq_);
      break;
    }
  }
  return isConnected_;
}

MQTTClient::~MQTTClient() {
  if (isConnected_) {
    mosquitto_disconnect(mosq_);
    mosquitto_loop_stop(mosq_, false);
  }
  mosquitto_destroy(mosq_);
  mosquitto_lib_cleanup();
}

void MQTTClient::addPrefix(const std::string& prefix) { prefix_ = prefix; }
void MQTTClient::addPublishPath(const std::string& path) {
  paths_.push_back(path);
  logger_->Log(LogLevel::VERBOSE,
               std::string("MQTTClient::addPublishPath: ") + path);
}

bool MQTTClient::sendPathValue(const std::string& topic_path,
                               const jsoncons::json& value) {
  logger_->Log(LogLevel::VERBOSE, "MQTTClient::sendPathValue: send path " +
                                      topic_path + " value " +
                                      value.as_string() + " length " +
                                      std::to_string(value.as_string().size()));
  for (auto& path : paths_) {
    auto topic_regex = std::regex{
        std::regex_replace(path, std::regex("\\*"), std::string(".*"))};
    std::smatch base_match;
    if (std::regex_match(topic_path, base_match, topic_regex)) {
      if (!start()) {
        std::string err("Cannot send to connection, server not initialized!");
        logger_->Log(LogLevel::ERROR, err);
        return false;
      }
      std::string topic_name =
          std::regex_replace(topic_path, std::regex("\\."), std::string("/"));
      if (!prefix_.empty()) {
        topic_name = prefix_ + "/" + topic_name;
      }
      logger_->Log(LogLevel::VERBOSE,
                   "MQTTClient::Publish topic " + topic_name);
      auto payload = value.as_string();
      int rc = mosquitto_publish(mosq_, NULL, topic_name.c_str(),
                                 payload.size(), payload.c_str(), qos_, false);
      if (rc != MOSQ_ERR_SUCCESS) {
        logger_->Log(LogLevel::ERROR, std::string("MQTT publish Error: ") +
                                          std::string(mosquitto_strerror(rc)));
      }
      return true;
    }
  }
  return false;
}

bool MQTTClient::setUsernamePassword(const std::string& username,
                                     const std::string& password) {
  int rc = mosquitto_username_pw_set(mosq_, username.c_str(), password.c_str());
  if (rc != MOSQ_ERR_SUCCESS) {
    logger_->Log(LogLevel::ERROR,
                 std::string("MQTT username password error: ") +
                     std::string(mosquitto_strerror(rc)));
  }
  return rc == MOSQ_ERR_SUCCESS;
}
