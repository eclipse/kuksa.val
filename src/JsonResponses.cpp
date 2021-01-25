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
#include "JsonResponses.hpp"
#include <chrono>

namespace JsonResponses {
  void malFormedRequest(std::string request_id,
                        const std::string action,
                        std::string message,
                        jsoncons::json& jsonResponse) {
    jsonResponse["action"] = action;
    jsonResponse["requestId"] = request_id;
    jsoncons::json error;
    error["number"] = 400;
    error["reason"] = "Bad Request";
    error["message"] = message;
    jsonResponse["error"] = error;
    jsonResponse["timestamp"] = getTimeStamp();
  }
  std::string malFormedRequest(std::string request_id,
                               const std::string action,
                               std::string message) {
    jsoncons::json answer;
    malFormedRequest(request_id, action, message, answer);

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();
  }

  void malFormedRequest(std::string message, jsoncons::json& jsonResponse) {
    jsoncons::json error;

    error["number"] = 400;
    error["reason"] = "Bad Request";
    error["message"] = message;
    jsonResponse["error"] = error;
    jsonResponse["timestamp"] = getTimeStamp();
  }
  std::string malFormedRequest(std::string message) {
    jsoncons::json answer;
    malFormedRequest(message, answer);

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();
  }

  /** A API call requested a non-existant path */
  void pathNotFound(std::string request_id,
                    const std::string action,
                    const std::string path,
                    jsoncons::json& jsonResponse) {
    jsonResponse["action"] = action;
    jsonResponse["requestId"] = request_id;
    jsoncons::json error;
    error["number"] = 404;
    error["reason"] = "Path not found";
    error["message"] = "I can not find " + path + " in my db";
    jsonResponse["error"] = error;
    jsonResponse["timestamp"] = getTimeStamp();
  }
  std::string pathNotFound(std::string request_id,
                           const std::string action,
                           const std::string path) {
    jsoncons::json answer;
    pathNotFound(request_id, action, path, answer);

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();
  }

  void noAccess(std::string request_id,
                const std::string action,
                std::string message,
                jsoncons::json& jsonResponse) {
    jsoncons::json error;
    jsonResponse["action"] = action;
    jsonResponse["requestId"] = request_id;
    error["number"] = 403;
    error["reason"] = "Forbidden";
    error["message"] = message;
    jsonResponse["error"] = error;
    jsonResponse["timestamp"] = getTimeStamp();
  }
  std::string noAccess(std::string request_id,
                       const std::string action,
                       std::string message) {
    jsoncons::json answer;
    noAccess(request_id, action, message, answer);

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();
  }

  void valueOutOfBounds(std::string request_id,
                        const std::string action,
                        const std::string message,
                        jsoncons::json& jsonResponse) {
    jsonResponse["action"] = action;
    jsonResponse["requestId"] = request_id;
    jsoncons::json error;
    error["number"] = 400;
    error["reason"] = "Value passed is out of bounds";
    error["message"] = message;
    jsonResponse["error"] = error;
    jsonResponse["timestamp"] = getTimeStamp();
  }
  std::string valueOutOfBounds(std::string request_id,
                               const std::string action,
                               const std::string message) {
    jsoncons::json answer;
    valueOutOfBounds(request_id, action, message, answer);

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();
  }

  int64_t getTimeStamp(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()
          ).count();
  }
}
