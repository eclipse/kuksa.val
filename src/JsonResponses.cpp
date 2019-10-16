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

namespace JsonResponses {
  std::string malFormedRequest(uint32_t request_id,
                               const std::string action,
                               std::string message) {
    jsoncons::json answer;
    answer["action"] = action;
    answer["requestId"] = request_id;
    jsoncons::json error;
    error["number"] = 400;
    error["reason"] = "Bad Request";
    error["message"] = message;
    answer["error"] = error;
    answer["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();
  }

  std::string malFormedRequest(std::string message) {
    jsoncons::json answer;
    jsoncons::json error;

    error["number"] = 400;
    error["reason"] = "Bad Request";
    error["message"] = message;
    answer["error"] = error;
    answer["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();
  }

  /** A API call requested a non-existant path */
  std::string pathNotFound(uint32_t request_id,
                           const std::string action,
                           const std::string path) {
    jsoncons::json answer;
    answer["action"] = action;
    answer["requestId"] = request_id;
    jsoncons::json error;
    error["number"] = 404;
    error["reason"] = "Path not found";
    error["message"] = "I can not find " + path + " in my db";
    answer["error"] = error;
    answer["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();
  }

  std::string noAccess(uint32_t request_id,
                       const std::string action,
                       std::string message) {
    jsoncons::json result;
    jsoncons::json error;
    result["action"] = action;
    result["requestId"] = request_id;
    error["number"] = 403;
    error["reason"] = "Forbidden";
    error["message"] = message;
    result["error"] = error;
    result["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();
  }

  std::string valueOutOfBounds(uint32_t request_id,
                               const std::string action,
                               const std::string message) {
    jsoncons::json answer;
    answer["action"] = action;
    answer["requestId"] = request_id;
    jsoncons::json error;
    error["number"] = 400;
    error["reason"] = "Value passed is out of bounds";
    error["message"] = message;
    answer["error"] = error;
    answer["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();
  }
}
