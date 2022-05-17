/*
 * ******************************************************************************
 * Copyright (c) 2018-2020 Robert Bosch GmbH.
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

#include "VssCommandProcessor.hpp"

#include <stdint.h>
#include <iostream>
#include <sstream>
#include <string>

#include "exception.hpp"
#include "WsChannel.hpp"

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


string VssCommandProcessor::processUpdateVSSTree(kuksa::kuksaChannel& channel, jsoncons::json &request){
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

    ss << pretty_print(answer);
    return ss.str();
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::ERROR, string(nopermission.what()));
    return JsonResponses::noAccess(request["requestId"].as<std::string>(), "updateVSSTree", nopermission.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(request["requestId"].as<std::string>(), "get", string("Unhandled error: ") + e.what());
  }

  ss << pretty_print(answer);
  return ss.str();
}

string VssCommandProcessor::processGetMetaData(jsoncons::json &request) {
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

  std::stringstream ss;
  ss << pretty_print(result);

  return ss.str();
}

string VssCommandProcessor::processUpdateMetaData(kuksa::kuksaChannel& channel, jsoncons::json& request){
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

    ss << pretty_print(answer);
    return ss.str();
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::WARNING, string(nopermission.what()));
    return JsonResponses::noAccess(request["requestId"].as<string>(), "updateMetaData", nopermission.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(request["requestId"].as<string>(), "get", string("Unhandled error: ") + e.what());
  } 

  ss << pretty_print(answer);
  return ss.str();
  
}

string VssCommandProcessor::processAuthorize(kuksa::kuksaChannel &channel,
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

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();

  } else {
    jsoncons::json result;
    result["action"] = "authorize";
    result["requestId"] = request_id;
    result["TTL"] = ttl;
    result["ts"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();
  }
}

string VssCommandProcessor::processQuery(const string &req_json,
                                         kuksa::kuksaChannel &channel) {
  jsoncons::json root;
  string response;
  try {
    root = jsoncons::json::parse(req_json);
    string action = root["action"].as<string>();
    logger->Log(LogLevel::VERBOSE, "Receive action: " + action);

    if (action == "get") {
        response = processGet2(channel, root);
    } 
    else if (action == "set") {
        response = processSet2(channel, root);
    }
    else if (action == "getMetaData") {
        response = processGetMetaData(root);
    }
    else if (action == "updateMetaData") {
        response = processUpdateMetaData(channel, root);
    }
    else if (action == "authorize") {
      string token = root["tokens"].as<string>();
      //string request_id = root["requestId"].as<int>();
      string request_id = root["requestId"].as<string>();
      logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: authorize query with token = "
           + token + " with request id " + request_id);

      response = processAuthorize(channel, request_id, token);
    } 
    else if (action == "unsubscribe") {
      response = processUnsubscribe(channel, root);
    } 
    else if (action == "subscribe") {
      response = processSubscribe(channel, root);
    } else if (action == "updateVSSTree") {
      response = processUpdateVSSTree(channel,root);
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
  return response;
}



