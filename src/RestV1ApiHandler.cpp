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

#include "RestV1ApiHandler.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <limits>

#include "JsonResponses.hpp"
#include "ILogger.hpp"


RestV1ApiHandler::RestV1ApiHandler(std::shared_ptr<ILogger> loggerUtil, std::string& docRoot)
  : logger_(loggerUtil),
    docRoot_(docRoot) {

  // Supported HTTP methods
  regexHttpMethods_ = "^(GET|POST|PUT|OPTIONS)";

  // Resource strings for REST API hooks. Order must match order in RestV1ApiHandler::Resources enum
  resourceHandleNames_ = std::vector<std::string>{std::string{"signals"},
                                                  std::string{"metadata"},
                                                  std::string{"authorize"}};
  // verify that sizes match
  assert(resourceHandleNames_.size() == static_cast<size_t>(Resources::Count));

  // fill regex automatically with all available resources
  regexResources_= std::string("^(");
  for (unsigned i = 0; i < resourceHandleNames_.size() - 1u; ++i) {
    regexResources_ += resourceHandleNames_[i] + std::string("|");
  }
  regexResources_ += resourceHandleNames_[resourceHandleNames_.size() - 1u] + std::string(")");

  // base64 url allowed characters
  regexToken_ = std::string("[A-Za-z0-9_-]+\\.[A-Za-z0-9_-]+\\.[A-Za-z0-9_-]+");
}

RestV1ApiHandler::~RestV1ApiHandler() {
}

bool RestV1ApiHandler::verifyPathAndStrip(std::string& restTarget, std::string& path) {
  size_t pos = restTarget.find(path);

  // found doc root at correct position
  if (pos == 0) {
    // remove root path to leave only next level
    restTarget.erase(pos, path.length());
    return true;
  }

  return false;
}

bool RestV1ApiHandler::GetSignalPath(uint32_t requestId,
                                     jsoncons::json& json,
                                     std::string& restTarget) {
  std::string signalPath;
  std::string foundStr;
  std::string restDelimiter("/");
  std::string defaultDelimiter(".");
  std::string queryDelimiter("?");
  std::smatch sm;
  bool ret = true;

  if (restTarget.size() && verifyPathAndStrip(restTarget, restDelimiter)) {
    while (restTarget.size()) {
      // we only accept clean printable characters
      const std::regex regexValidWord("^([A-Za-z]+)");

      if (std::regex_search(restTarget, sm, regexValidWord)) {
        foundStr = sm.str(1);
        signalPath += foundStr;
        if (verifyPathAndStrip(restTarget, foundStr)) {
          if ((restTarget.size() == 0)) {
            break;
          }
          // we support both '/' and '.' as branch/signal delimiters
          else if (verifyPathAndStrip(restTarget, restDelimiter)) {
            signalPath += defaultDelimiter;
          }
          // we support both '/' and '.' as branch/signal delimiters
          else if (verifyPathAndStrip(restTarget, defaultDelimiter)) {
            signalPath += defaultDelimiter;
          }
          // if we got to query start '?' char, end extraction if signal path
          else if (restTarget[0] == '?') {
            break;
          }
          else {
            JsonResponses::malFormedRequest(
                requestId,
                json["action"].as_string(),
                "Signal path delimiter not valid",
                json);
            ret = false;
          }
        }
        else {
           JsonResponses::malFormedRequest(
              requestId,
              json["action"].as_string(),
              "Signal path not valid",
              json);
          ret = false;
        }
      }
      else {
        JsonResponses::malFormedRequest(
            requestId,
            json["action"].as_string(),
            "Signal path URI not valid",
            json);
        ret = false;
      }
    }
  }
  else {
    // not supporting retrieving of all signals by default
    JsonResponses::malFormedRequest(
        requestId,
        json["action"].as_string(),
        "Signals cannot be retrieved in bulk request for now, only through single signal requests",
        json);
    ret = false;
  }

  // update signal path if all is OK
  if (ret) {
    json["path"] = signalPath;
  }

  return ret;
}

// Basic implementation of REST API handling
// For now governed by KISS principle, but in future, it should be re-factored
// to handle independently different methods and resources for easier maintenance..
// possibly by providing hooks for each resource and|or API version, etc...
bool RestV1ApiHandler::GetJson(std::string&& restMethod,
                               std::string&& restTarget,
                               std::string& jsonRequest) {
  bool ret = true;
  std::smatch sm;
  jsoncons::json json;

  // TODO: should client provide request ID when using REST API?
  uint32_t requestId = std::rand() % std::numeric_limits<uint32_t>::max();
  json["requestId"] = requestId;

  // search for supported HTTP requests
  const std::regex regSupportedHttpActions(regexHttpMethods_, std::regex_constants::icase);
  // check REST action method
  std::regex_match (restMethod, sm, regSupportedHttpActions);
  // if supported methods found, parse further
  if (sm.size()) {
    std::string httpMethod = sm.str(1);
    boost::algorithm::to_lower(httpMethod);

    if (verifyPathAndStrip(restTarget, docRoot_)) {
       const std::regex regResources(regexResources_, std::regex_constants::icase);

       // get requested resource type
       std::regex_search(restTarget, sm, regResources);
       if (sm.size()) {
         std::string foundStr = sm.str(1);

         if (verifyPathAndStrip(restTarget, foundStr)) {
           // //////
           // signals handler
           if (foundStr.compare("signals") == 0) {
             // handler CORS pre-flight requests from browsers
             if (httpMethod.compare("options") == 0) {
               json["action"] = "options";
               json["methods"] = "PUT, GET, OPTIONS";
               json["headers"] = "X-PINGOTHER, Content-Type";
               json["max-age"] = "86400";
               json["origin"] = "*";
             }
             else if (httpMethod.compare("get") == 0) {
               if (GetSignalPath(requestId, json, restTarget)) {
                 json["action"] = "get";
               }
               else {
                 JsonResponses::malFormedRequest(
                     requestId,
                     json["action"].as_string(),
                     "Invalid path",
                     json);
                 ret = false;
               }
             }
             else if (httpMethod.compare("put") == 0) {
               if (GetSignalPath(requestId, json, restTarget)) {
                 json["action"] = "set";
                 std::string queryStr("?value=");

                 if (verifyPathAndStrip(restTarget, queryStr)) {
                   json["value"] = restTarget;
                 }
                 else {
                   JsonResponses::malFormedRequest(
                       requestId,
                       json["action"].as_string(),
                       "Invalid query parameter",
                       json);
                 }
               }
               else {
                 JsonResponses::malFormedRequest(
                    requestId,
                    json["action"].as_string(),
                    "Invalid path",
                    json);
                 ret = false;
               }
             }
             else {
               // TODO: handle signal POST
               JsonResponses::malFormedRequest(
                   requestId,
                   json["action"].as_string(),
                   "HTTP method not yet supported for 'signals' resource",
                   json);
               ret = false;
             }
           }
           // //////
           // metadata handler
           else if (foundStr.compare("metadata") == 0) {
             // handler CORS pre-flight requests from browsers
             if (httpMethod.compare("options") == 0) {
               json["action"] = "options";
               json["methods"] = "GET, OPTIONS";
               json["headers"] = "X-PINGOTHER, Content-Type";
               json["max-age"] = "86400";
               json["origin"] = "*";
             }
             else if (httpMethod.compare("get") == 0) {
               ret = GetSignalPath(requestId, json, restTarget);

               json["action"] = "getMetadata";
             }
             else {
               // TODO: handle signal POST
               JsonResponses::malFormedRequest(
                   requestId,
                   json["action"].as_string(),
                   "POST method not supported for 'metadata' resource",
                   json);
               ret = false;
             }
           }
           // //////
           // authorize handler
           else if (foundStr == "authorize") {
             // handler CORS pre-flight requests from browsers
             if (httpMethod.compare("options") == 0) {
               json["action"] = "options";
               json["methods"] = "POST, OPTIONS";
               json["headers"] = "X-PINGOTHER, Content-Type";
               json["max-age"] = "86400";
               json["origin"] = "*";
             }
             else if (httpMethod == "post") {
               std::string tokenParam("?token=");
               if (verifyPathAndStrip(restTarget, tokenParam)) {
                 const std::regex regToken(regexToken_);
                 std::regex_search(restTarget, sm, regToken);

                 sleep(1);
                 if (sm.size()) {
                   json["action"] = "authorize";
                   json["tokens"] = sm.str(0);
                 }
                 else {
                   JsonResponses::malFormedRequest(
                       requestId,
                       json["action"].as_string(),
                       "Token for 'authorize' not valid",
                       json);
                   ret = false;
                 }
               }
               else {
                 JsonResponses::malFormedRequest(
                     requestId,
                     json["action"].as_string(),
                     "Parameters for 'authorize' not valid",
                     json);
                 ret = false;
               }
             }
             else {
               // GET not supported for authorize resource
               JsonResponses::malFormedRequest(
                   requestId,
                   json["action"].as_string(),
                   "GET method not supported for 'authorize' resource",
                   json);
               ret = false;
             }
           }
           else {
             JsonResponses::malFormedRequest(
                 requestId,
                 json["action"].as_string(),
                 "Requested resource do not exist",
                 json);
             ret = false;
           }
         }
         else {
            JsonResponses::malFormedRequest(
               requestId,
               json["action"].as_string(),
               "Signal path URI not valid",
               json);
           ret = false;
         }
       }
       else
       {
         JsonResponses::malFormedRequest(
             requestId,
             json["action"].as_string(),
             "Requested resource do not exist",
             json);
         ret = false;
       }
    }
  }
  else
  {
    // TODO: evaluate what and how we should support HTTP methods (put, patch, ...)
    JsonResponses::malFormedRequest(
        requestId,
        json["action"].as_string(),
        "Requested HTTP method is not supported",
        json);
    ret = false;
  }

  std::stringstream ss;
  ss << pretty_print(json);
  jsonRequest = ss.str();

  return ret;
}
