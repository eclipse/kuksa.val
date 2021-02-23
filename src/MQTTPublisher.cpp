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

#include "MQTTPublisher.hpp"

#include "ILogger.hpp"

MQTTPublisher::MQTTPublisher(std::shared_ptr<ILogger> loggerUtil,
                             const std::string& id,
                             boost::program_options::variables_map& config)
    : logger_(loggerUtil),
      paths_(),
      keepalive_(config["mqtt.keepalive"].as<int>()),
      qos_(config["mqtt.qos"].as<int>()),
      connection_retry_(config["mqtt.retry"].as<int>()),
      prefix_(config["mqtt.topic-prefix"].as<std::string>()),
      host_(config["mqtt.address"].as<std::string>()),
      port_(config["mqtt.port"].as<int>()) {
  init(id, config["mqtt.insecure"].as<bool>());
    if (config.count("mqtt.username")) {
      std::string password;
      if (!config.count("mqtt.password")) {
        std::cout << "Please input your mqtt password:" << std::endl;
        std::cin >> password;

      } else {
        password = config["mqtt.password"].as<std::string>();
      }
      setUsernamePassword(config["mqtt.username"].as<std::string>(),
                                      password);
    }
}

void MQTTPublisher::init(const std::string& id, bool insecure) {
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

boost::program_options::options_description& MQTTPublisher::getOptions() {
  // mqtt options
  static boost::program_options::options_description mqtt_desc("MQTT Options");
  mqtt_desc.add_options()(
      "mqtt.insecure", boost::program_options::bool_switch()->default_value(false),
      "Do not check that the server certificate hostname matches the remote "
      "hostname. Do not use this option in a production environment")(
      "mqtt.username", boost::program_options::value<std::string>(),
      "Provide a mqtt username")("mqtt.password",
                                 boost::program_options::value<std::string>(),
                                 "Provide a mqtt password")(
      "mqtt.address",
      boost::program_options::value<std::string>()->default_value("localhost"),
      "Address of MQTT broker")(
      "mqtt.port", boost::program_options::value<int>()->default_value(1883),
      "Port of MQTT broker")(
      "mqtt.qos", boost::program_options::value<int>()->default_value(0),
      "Quality of service level to use for all messages. Defaults to 0")(
      "mqtt.keepalive", boost::program_options::value<int>()->default_value(60),
      "Keep alive in seconds for this mqtt client. Defaults to 60")(
      "mqtt.retry", boost::program_options::value<int>()->default_value(3),
      "Times of retry via connections. Defaults to 3")(
      "mqtt.topic-prefix", boost::program_options::value<std::string>(),
      "Prefix to add for each mqtt topics")(
      "mqtt.publish", boost::program_options::value<std::string>()->default_value(""),
      "List of vss data path (using readable format with `.`) to be published "
      "to mqtt broker, using \";\" to seperate multiple path and \"*\" as "
      "wildcard");
  return mqtt_desc;
}
bool MQTTPublisher::start() {
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

MQTTPublisher::~MQTTPublisher() {
  if (isConnected_) {
    mosquitto_disconnect(mosq_);
    mosquitto_loop_stop(mosq_, false);
  }
  mosquitto_destroy(mosq_);
  mosquitto_lib_cleanup();
}

void MQTTPublisher::addPublishPath(const std::string& path) {
  paths_.push_back(path);
  logger_->Log(LogLevel::VERBOSE,
               std::string("MQTTPublisher::addPublishPath: ") + path);
}

bool MQTTPublisher::sendPathValue(const std::string& topic_path,
                                  const jsoncons::json& value) {
  logger_->Log(LogLevel::VERBOSE, "MQTTPublisher::sendPathValue: send path " +
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
                   "MQTTPublisher::Publish topic " + topic_name);
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

bool MQTTPublisher::setUsernamePassword(const std::string& username,
                                        const std::string& password) {
  int rc = mosquitto_username_pw_set(mosq_, username.c_str(), password.c_str());
  if (rc != MOSQ_ERR_SUCCESS) {
    logger_->Log(LogLevel::ERROR,
                 std::string("MQTT username password error: ") +
                     std::string(mosquitto_strerror(rc)));
  }
  return rc == MOSQ_ERR_SUCCESS;
}
