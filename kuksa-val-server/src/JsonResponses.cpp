/**********************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
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

#include "JsonResponses.hpp"

#include <time.h>

namespace JsonResponses {
void malFormedRequest(std::string request_id, const std::string action,
                      std::string message, jsoncons::json& jsonResponse) {
  jsonResponse["action"] = action;
  jsonResponse["requestId"] = request_id;
  jsoncons::json error;
  error["number"] = "400";
  error["reason"] = "Bad Request";
  error["message"] = message;
  jsonResponse["error"] = error;
  jsonResponse["ts"] = getTimeStamp();
}
jsoncons::json malFormedRequest(std::string request_id, const std::string action,
                             std::string message) {
  jsoncons::json answer;
  malFormedRequest(request_id, action, message, answer);
  return answer;
}

void malFormedRequest(std::string message, jsoncons::json& jsonResponse,
                      std::string requestId) {
  jsoncons::json error;

  error["number"] = "400";
  error["reason"] = "Bad Request";
  error["message"] = message;
  jsonResponse["error"] = error;
  jsonResponse["requestId"] = requestId;
  jsonResponse["ts"] = getTimeStamp();
}
jsoncons::json malFormedRequest(std::string message, std::string requestId) {
  jsoncons::json answer;
  malFormedRequest(message, answer, requestId);
  return answer;
}

// JSON Reponse to not set error
void notSetResponse(std::string message, jsoncons::json& jsonResponse,
                      std::string requestId) {
  jsoncons::json error;

  error["number"] = "404";
  error["reason"] = "unavailable_data";
  error["message"] = message;
  jsonResponse["error"] = error;
  jsonResponse["requestId"] = requestId;
  jsonResponse["ts"] = getTimeStamp();
  jsonResponse["action"] = "get";
}
jsoncons::json notSetResponse(std::string request_id, std::string message) {
  jsoncons::json answer;
  notSetResponse(message, answer, request_id);
  return answer;
}

/** A API call requested a non-existant path */
void pathNotFound(std::string request_id, const std::string action,
                  const std::string path, jsoncons::json& jsonResponse) {
  jsonResponse["action"] = action;
  jsonResponse["requestId"] = request_id;
  jsoncons::json error;
  error["number"] = "404";
  error["reason"] = "Path not found";
  error["message"] = "I can not find " + path + " in my db";
  jsonResponse["error"] = error;
  jsonResponse["ts"] = getTimeStamp();
}
jsoncons::json pathNotFound(std::string request_id, const std::string action,
                         const std::string path) {
  jsoncons::json answer;
  pathNotFound(request_id, action, path, answer);
  return answer;
}

void noAccess(std::string request_id, const std::string action,
              std::string message, jsoncons::json& jsonResponse) {
  jsoncons::json error;
  jsonResponse["action"] = action;
  jsonResponse["requestId"] = request_id;
  error["number"] = "403";
  error["reason"] = "Forbidden";
  error["message"] = message;
  jsonResponse["error"] = error;
  jsonResponse["ts"] = getTimeStamp();
}
jsoncons::json noAccess(std::string request_id, const std::string action,
                     std::string message) {
  jsoncons::json answer;
  noAccess(request_id, action, message, answer);
  return answer;
}

void valueOutOfBounds(std::string request_id, const std::string action,
                      const std::string message, jsoncons::json& jsonResponse) {
  jsonResponse["action"] = action;
  jsonResponse["requestId"] = request_id;
  jsoncons::json error;
  error["number"] = "400";
  error["reason"] = "Value passed is out of bounds";
  error["message"] = message;
  jsonResponse["error"] = error;
  jsonResponse["ts"] = getTimeStamp();
}
jsoncons::json valueOutOfBounds(std::string request_id, const std::string action,
                             const std::string message) {
  jsoncons::json answer;
  valueOutOfBounds(request_id, action, message, answer);
  return answer;
}

/** Return an extended ISO8601 UTC timestamp according to W3C guidelines
 https://www.w3.org/TR/NOTE-datetime
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
std::string getTimeStamp() {
  auto itt = std::time(nullptr);
  std::ostringstream ss;
  ss << std::put_time(gmtime(&itt), "%FT%T.%sZ");
  return ss.str();
}

/** This extended ISO8601 UTC timestamp according to W3C guidelines
 * https://www.w3.org/TR/NOTE-datetime for unix timestamp zero. This will be
 * used  when values that have never been set are queried This makes sure to
 * habe a syntactically compliant timestamp
 */
std::string getTimeStampZero() { 
  return "1970-01-01T00:00:00.0Z"; 
}

/** This adds tv_s and tv_ns timestamps to a jsoncons object in the attributes
 *  ts_s<suffix> and ts_ns<suffix>
 */
void addTimeStampToJSON(jsoncons::json& jsontarget, const std::string suffix) {
  timespec ts;
  timespec_get(&ts, TIME_UTC);
  jsontarget.insert_or_assign("ts_s" + suffix, ts.tv_sec);
  jsontarget.insert_or_assign("ts_ns" + suffix, ts.tv_nsec);
}

/** Checks for  ts_s and ts_ns attributes in jsoncons object and replaces them
 *  with and ISO8601 timestamp in ts attribute
 */
void convertJSONTimeStampToISO8601(jsoncons::json& jsontarget) {

  if (!( jsontarget.contains("ts_s") && jsontarget.contains("ts_ns") )) {
    jsontarget.insert_or_assign("ts", getTimeStampZero());
    if (jsontarget.contains("ts_s")) {
      jsontarget.erase("ts_s");
    }
    if (jsontarget.contains("ts_ns")) {
      jsontarget.erase("ts_ns");
    }
    return;
  } 
  else {
    timespec t;
    t.tv_sec = jsontarget["ts_s"].as<uint64_t>();
    t.tv_nsec = jsontarget["ts_ns"].as<uint32_t>();
    struct tm* tm = gmtime(&t.tv_sec);

    jsontarget.erase("ts_s");
    jsontarget.erase("ts_ns");
    char iso8601time[64];
    iso8601time[63] = '\0';
    std::strftime(iso8601time, 63, "%FT%T.", tm);
    std::ostringstream ss;
    ss << iso8601time << t.tv_nsec << "Z";
    jsontarget.insert_or_assign("ts", ss.str());
    return;
  }
}

}
