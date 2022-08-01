/**********************************************************************
 * Copyright (c) 2018-2022 Robert Bosch GmbH.
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
 *      Robert Bosch GmbH
 **********************************************************************/


#include "VssCommandProcessor.hpp"

#include <stdint.h>
#include <iostream>
#include <sstream>
#include <string>

#include "exception.hpp"
#include "KuksaChannel.hpp"

#include "JsonResponses.hpp"
#include "visconf.hpp"
#include "VssDatabase.hpp"
#include "AccessChecker.hpp"
#include "SubscriptionHandler.hpp"
#include "ILogger.hpp"
#include "IVssDatabase.hpp"
#include "IAuthenticator.hpp"
#include "ISubscriptionHandler.hpp"
#include <boost/algorithm/string.hpp>


using namespace std;

VssCommandProcessor::VssCommandProcessor(
    std::shared_ptr<ILogger> loggerUtil,
    std::shared_ptr<IVssDatabase> dbase,
    std::shared_ptr<IAuthenticator> vdator,
    std::shared_ptr<IAccessChecker> accC,
    std::shared_ptr<ISubscriptionHandler> subhandler) {
  logger = loggerUtil;
  database = dbase;
  tokenValidator = vdator;
  subHandler = subhandler;
  accessValidator_ = accC;
  requestValidator = new VSSRequestValidator(logger);
}

VssCommandProcessor::~VssCommandProcessor() {
  accessValidator_.reset();
  delete requestValidator;
}


jsoncons::json VssCommandProcessor::processUpdateVSSTree(KuksaChannel& channel, jsoncons::json &request){
  logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processUpdateVSSTree");
  
  try {
    requestValidator->validateUpdateVSSTree(request);
  } catch (jsoncons::jsonschema::schema_error &e) {
    std::string msg=std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, msg);
    return JsonResponses::malFormedRequest( requestValidator->tryExtractRequestId(request), "updateVSSTree", string("Schema error: ") + msg);
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request) , "updateVSSTree", string("Unhandled error: ") + e.what());
  }

  jsoncons::json answer;
  answer["action"] = "updateVSSTree";
  answer.insert_or_assign("requestId", request["requestId"]);
  answer["ts"] = JsonResponses::getTimeStamp();

  std::stringstream ss;
  try {
    database->updateJsonTree(channel, request["metadata"]);
  } catch (genException &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    jsoncons::json error;

    error["number"] = "401";
    error["reason"] = "Unknown error";
    error["message"] = e.what();

    answer["error"] = error;

    return answer;
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::ERROR, string(nopermission.what()));
    return JsonResponses::noAccess(request["requestId"].as<std::string>(), "updateVSSTree", nopermission.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(request["requestId"].as<std::string>(), "get", string("Unhandled error: ") + e.what());
  }

  return answer;
}

jsoncons::json VssCommandProcessor::processGetMetaData(jsoncons::json &request) {
  VSSPath path=VSSPath::fromVSS(request["path"].as_string());

  try {
    requestValidator->validateGet(request);
  } catch (jsoncons::jsonschema::schema_error & e) {
    std::string msg=std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, msg);
    return JsonResponses::malFormedRequest(requestValidator->tryExtractRequestId(request), "getMetaData", string("Schema error: ") + msg);
  } catch (std::exception &e) {
    std::string msg=std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, "Unhandled error: " + msg );
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request), "getMetaData", string("Unhandled error: ") + e.what());
  }

  jsoncons::json result;
  result["action"] = "getMetaData";
  result.insert_or_assign("requestId", request["requestId"]);
  jsoncons::json st = database->getMetaData(path);
  result["ts"] = JsonResponses::getTimeStamp();
  if (0 == st.size()){
    jsoncons::json error;
    error["number"] = "404";
    error["reason"] = "Path not found";
    error["message"] = "In database no metadata found for path " + path.getVSSPath();
    result["error"] = error;
  
  } else {
    result["metadata"] = st;
  }
  return result;
}

jsoncons::json VssCommandProcessor::processUpdateMetaData(KuksaChannel& channel, jsoncons::json& request){
  logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processUpdateMetaData");
  VSSPath path=VSSPath::fromVSS(request["path"].as_string());

  try {
    requestValidator->validateUpdateMetadata(request);
  } catch (jsoncons::jsonschema::schema_error & e) {
    std::string msg=std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, msg);
    return JsonResponses::malFormedRequest(requestValidator->tryExtractRequestId(request), "updateMetaData", string("Schema error: ") + msg);
  } catch (std::exception &e) {
    std::string msg=std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, "Unhandled error: " + msg );
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request), "updateMetaData", string("Unhandled error: ") + e.what());
  }

  jsoncons::json answer;
  answer["action"] = "updateMetaData";
  answer.insert_or_assign("requestId", request["requestId"]);
  answer["ts"] = JsonResponses::getTimeStamp();

  std::stringstream ss;
  try {
    database->updateMetaData(channel, path, request["metadata"]);
  } catch (genException &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    jsoncons::json error;

    error["number"] = "401";
    error["reason"] = "Unknown error";
    error["message"] = e.what();

    answer["error"] = error;

    return answer;
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::WARNING, string(nopermission.what()));
    return JsonResponses::noAccess(request["requestId"].as<string>(), "updateMetaData", nopermission.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(request["requestId"].as<string>(), "get", string("Unhandled error: ") + e.what());
  } 

  return answer;
  
}

jsoncons::json VssCommandProcessor::processAuthorize(KuksaChannel &channel,
                                             const string & request_id,
                                             const string & token) {
  int ttl = tokenValidator->validate(channel, token);

  if (ttl == -1) {
    jsoncons::json result;
    jsoncons::json error;
    result["action"] = "authorize";
    result["requestId"] = request_id;
    error["number"] = "401";
    error["reason"] = "Invalid Token";
    error["message"] = "Check the JWT token passed";

    result["error"] = error;
    result["ts"] = JsonResponses::getTimeStamp();

    return result;
  } else {
    jsoncons::json result;
    result["action"] = "authorize";
    result["requestId"] = request_id;
    result["TTL"] = ttl;
    result["ts"] = JsonResponses::getTimeStamp();

    return result;
  }
}

jsoncons::json VssCommandProcessor::processQuery(const string &req_json,
                                         KuksaChannel &channel) {
  jsoncons::json root;
  jsoncons::json jresponse;
  try {
    root = jsoncons::json::parse(req_json);
    string action = root["action"].as<string>();
    logger->Log(LogLevel::VERBOSE, "Receive action: " + action);

    if (action == "get") {
        jresponse = processGet(channel, root);
    } 
    else if (action == "set") {
        jresponse = processSet(channel, root);
    }
    else if (action == "getMetaData") {
        jresponse = processGetMetaData(root);
    }
    else if (action == "updateMetaData") {
        jresponse = processUpdateMetaData(channel, root);
    }
    else if (action == "authorize") {
      string token = root["tokens"].as<string>();
      string request_id = root["requestId"].as<string>();
      logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: authorize query with token = "
           + token + " with request id " + request_id);

      jresponse = processAuthorize(channel, request_id, token);
    } 
    else if (action == "unsubscribe") {
      jresponse = processUnsubscribe(channel, root);
    } 
    else if (action == "subscribe") {
      jresponse = processSubscribe(channel, root);
    } else if (action == "updateVSSTree") {
      jresponse = processUpdateVSSTree(channel,root);
    } else {
      logger->Log(LogLevel::WARNING, "VssCommandProcessor::processQuery: Unknown action " + action);
      return JsonResponses::malFormedRequest("Unknown action requested", root.get_value_or<std::string>("requestId", "UNKNOWN"));
    }
  } catch (jsoncons::ser_error &e) {
    logger->Log(LogLevel::WARNING, "JSON parse error");
    return JsonResponses::malFormedRequest(e.what());
  } catch (jsoncons::key_not_found &e) {
    logger->Log(LogLevel::WARNING, "JSON key not found error");
    return JsonResponses::malFormedRequest(e.what());
  } catch (jsoncons::not_an_object &e) {
    logger->Log(LogLevel::WARNING, "JSON not an object error");
    return JsonResponses::malFormedRequest(e.what());
  }
  return jresponse;
}



