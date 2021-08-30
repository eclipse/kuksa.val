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

#include <time.h>


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
    jsonResponse["ts"] = getTimeStamp();
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
    jsonResponse["ts"] = getTimeStamp();
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
    jsonResponse["ts"] = getTimeStamp();
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
    jsonResponse["ts"] = getTimeStamp();
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
    jsonResponse["ts"] = getTimeStamp();
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

  /** Return an extended ISO8601 UTC timestamp according to W3C guidelines https://www.w3.org/TR/NOTE-datetime
   * Complete date plus hours, minutes, seconds and a decimal fraction of a second
      YYYY-MM-DDThh:mm:ss.sTZD (eg 1997-07-16T19:20:30.45+01:00)
    
    where:

     YYYY = four-digit year
     MM   = two-digit month (01=January, etc.)
     DD   = two-digit day of month (01 through 31)
     hh   = two digits of hour (00 through 23) (am/pm NOT allowed)
     mm   = two digits of minute (00 through 59)
     ss   = two digits of second (00 through 59)
     s    = one or more digits representing a decimal fraction of a second
     TZD  = time zone designator (Z or +hh:mm or -hh:mm)
  */
  std::string getTimeStamp(){
    auto itt=std::time(nullptr);
    std::ostringstream ss;
    ss << std::put_time(gmtime(&itt), "%FT%T.%sZ");
    return ss.str();
  }

  /** This extended ISO8601 UTC timestamp according to W3C guidelines https://www.w3.org/TR/NOTE-datetime
  *  for unix timestamp zero. This will be used  when values that have never been set are queried
  *  This makes sure to habe a syntactically compliant timestamp
  */
  std::string getTimeStampZero() {
    return "1981-01-01T00:00:00.0000000000Z";
  }
}

