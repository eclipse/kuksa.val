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

#include "permmclient.hpp"
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

#ifdef JSON_SIGNING_ON
#include "SigningHandler.hpp"
#endif

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
#ifdef JSON_SIGNING_ON
  // TODO: add signer as dependency
  signer = std::make_shared<SigningHandler>();
#endif
}

VssCommandProcessor::~VssCommandProcessor() {
  accessValidator_.reset();
  delete requestValidator;
#ifdef JSON_SIGNING_ON
  signer.reset();
#endif
}



string VssCommandProcessor::processSubscribe(WsChannel &channel,
                                             const string & request_id, 
                                             const string & path) {
  logger->Log(LogLevel::VERBOSE, string("VssCommandProcessor::processSubscribe: Client wants to subscribe ")+path);

  uint32_t subId = -1;
  try {
    subId = subHandler->subscribe(channel, database, path);
  } catch (noPathFoundonTree &noPathFound) {
    logger->Log(LogLevel::ERROR, string(noPathFound.what()));
    return JsonResponses::pathNotFound(request_id, "subscribe", path);
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::ERROR, string(nopermission.what()));
    return JsonResponses::noAccess(request_id, "subscribe", nopermission.what());
  } catch (genException &outofboundExp) {
    logger->Log(LogLevel::ERROR, string(outofboundExp.what()));
    return JsonResponses::valueOutOfBounds(request_id, "subscribe",
                                    outofboundExp.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(request_id, "get", string("Unhandled error: ") + e.what());
  }

  if (subId > 0) {
    jsoncons::json answer;
    answer["action"] = "subscribe";
    answer["requestId"] = request_id;
    answer["subscriptionId"] = subId;
    answer["timestamp"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();

  } else {
    jsoncons::json root;
    jsoncons::json error;

    root["action"] = "subscribe";
    root["requestId"] = request_id;
    error["number"] = 400;
    error["reason"] = "Bad Request";
    error["message"] = "Unknown";

    root["error"] = error;
    root["timestamp"] = JsonResponses::getTimeStamp();

    std::stringstream ss;

    ss << pretty_print(root);
    return ss.str();
  }
}

string VssCommandProcessor::processUnsubscribe(const string & request_id,
                                               uint32_t subscribeID) {
  int res = subHandler->unsubscribe(subscribeID);
  if (res == 0) {
    jsoncons::json answer;
    answer["action"] = "unsubscribe";
    answer["requestId"] = request_id;
    answer["subscriptionId"] = subscribeID;
    answer["timestamp"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(answer);
    return ss.str();

  } else {
    jsoncons::json root;
    jsoncons::json error;

    root["action"] = "unsubscribe";
    root["requestId"] = request_id;
    error["number"] = 400;
    error["reason"] = "Unknown error";
    error["message"] = "Error while unsubscribing";

    root["error"] = error;
    root["timestamp"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(root);
    return ss.str();
  }
}

string VssCommandProcessor::processUpdateVSSTree(WsChannel& channel, const string& request_id,  jsoncons::json& metaData){
  logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processUpdateVSSTree");
  
  jsoncons::json answer;
  answer["action"] = "updateVSSTree";
  answer["requestId"] = request_id;
  answer["timestamp"] = JsonResponses::getTimeStamp();

  std::stringstream ss;
  try {
    database->updateJsonTree(channel, metaData);
  } catch (genException &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    jsoncons::json error;

    error["number"] = 401;
    error["reason"] = "Unknown error";
    error["message"] = e.what();

    answer["error"] = error;

    ss << pretty_print(answer);
    return ss.str();
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::ERROR, string(nopermission.what()));
    return JsonResponses::noAccess(request_id, "updateVSSTree", nopermission.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(request_id, "get", string("Unhandled error: ") + e.what());
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

  string requestId = request["requestId"].as_string();
  jsoncons::json result;
  result["action"] = "getMetaData";
  result["requestId"] = requestId;

  jsoncons::json st = database->getMetaData(path);
  result["timestamp"] = JsonResponses::getTimeStamp();
  if (0 == st.size()){
    jsoncons::json error;
    error["number"] = 404;
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

string VssCommandProcessor::processUpdateMetaData(WsChannel& channel, jsoncons::json& request){
  logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processUpdateMetaData");
  VSSPath path=VSSPath::fromVSS(request["path"].as_string());

  try {
    requestValidator->validateUpdateTree(request);
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
  string requestId = request["requestId"].as_string();
  answer["requestId"] = requestId;
  answer["timestamp"] = JsonResponses::getTimeStamp();

  std::stringstream ss;
  try {
    database->updateMetaData(channel, path, request["metadata"]);
  } catch (genException &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    jsoncons::json error;

    error["number"] = 401;
    error["reason"] = "Unknown error";
    error["message"] = e.what();

    answer["error"] = error;

    ss << pretty_print(answer);
    return ss.str();
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::ERROR, string(nopermission.what()));
    return JsonResponses::noAccess(requestId, "updateMetaData", nopermission.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(requestId, "get", string("Unhandled error: ") + e.what());
  } 

  ss << pretty_print(answer);
  return ss.str();
  
}

// Talks to the permission management daemon and processes the token received.
string VssCommandProcessor::processAuthorizeWithPermManager(WsChannel &channel,
                                                            const string & request_id,
                                                            const string & client, 
                                                            const string & clientSecret) {

  jsoncons::json response;
  // Get Token from permission management daemon.
  try {
     response = getPermToken(logger, client, clientSecret);
  } catch (genException &exp) {
    logger->Log(LogLevel::ERROR, exp.what());
    jsoncons::json result;
    jsoncons::json error;
    result["action"] = "kuksa-authorize";
    result["requestId"] = request_id;
    error["number"] = 501;
    error["reason"] = "No token received from permission management daemon";
    error["message"] = "Check if the permission managemnt daemon is running";

    result["error"] = error;
    result["timestamp"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();
  }
  int ttl = -1;
  if (response.contains("token") && response.contains("pubkey")) {
     try {
        tokenValidator->updatePubKey(response["pubkey"].as<string>());
        ttl = tokenValidator->validate(channel, response["token"].as<string>());
     } catch (exception &e) {
        logger->Log(LogLevel::ERROR, e.what());
        ttl = -1;
     }
  }

  if (ttl == -1) {
    jsoncons::json result;
    jsoncons::json error;
    result["action"] = "kuksa-authorize";
    result["requestId"] = request_id;
    error["number"] = 401;
    error["reason"] = "Invalid Token";
    error["message"] = "Check the JWT token passed";

    result["error"] = error;
    result["timestamp"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();

  } else {
    jsoncons::json result;
    result["action"] = "kuksa-authorize";
    result["requestId"] = request_id;
    result["TTL"] = ttl;
    result["timestamp"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();
  }
}

string VssCommandProcessor::processAuthorize(WsChannel &channel,
                                             const string & request_id,
                                             const string & token) {
  int ttl = tokenValidator->validate(channel, token);

  if (ttl == -1) {
    jsoncons::json result;
    jsoncons::json error;
    result["action"] = "authorize";
    result["requestId"] = request_id;
    error["number"] = 401;
    error["reason"] = "Invalid Token";
    error["message"] = "Check the JWT token passed";

    result["error"] = error;
    result["timestamp"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();

  } else {
    jsoncons::json result;
    result["action"] = "authorize";
    result["requestId"] = request_id;
    result["TTL"] = ttl;
    result["timestamp"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();
  }
}

string VssCommandProcessor::processQuery(const string &req_json,
                                         WsChannel &channel) {
  jsoncons::json root;
  string response;
  try {
    root = jsoncons::json::parse(req_json);
    string action = root["action"].as<string>();
    logger->Log(LogLevel::VERBOSE, "Receive action: " + action);

    if (action == "get") {
        response = processGet2(channel, root);
#ifdef JSON_SIGNING_ON
        response = signer->sign(response);
#endif  
    }
    else if (action == "set") {
        response = processSet2(channel, root);
#ifdef JSON_SIGNING_ON
        response = signer->sign(response);
#endif
    }
    else if (action == "getMetaData") {
        response = processGetMetaData(root);
#ifdef JSON_SIGNING_ON
        response = signer->sign(response);
#endif
    }
    else if (action == "updateMetaData") {
        response = processUpdateMetaData(channel, root);
#ifdef JSON_SIGNING_ON
        response = signer->sign(response);
#endif
    }
    else if (action == "authorize") {
      string token = root["tokens"].as<string>();
      //string request_id = root["requestId"].as<int>();
      string request_id = root["requestId"].as<string>();
      logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: authorize query with token = "
           + token + " with request id " + request_id);

      response = processAuthorize(channel, request_id, token);
    } else if (action == "unsubscribe") {
      //string request_id = root["requestId"].as<int>();
      string request_id = root["requestId"].as<string>();
      uint32_t subscribeID = root["subscriptionId"].as<int>();
      logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: unsubscribe query  for sub ID = "
              + to_string(subscribeID) + " with request id " + request_id);

      response = processUnsubscribe(request_id, subscribeID);
    } else if ( action == "kuksa-authorize") {
      string clientID = root["clientid"].as<string>();
      string clientSecret = root["secret"].as<string>();
      //string request_id = root["requestId"].as<int>();
      string request_id = root["requestId"].as<string>();
      logger->Log(LogLevel::VERBOSE, "vsscommandprocessor::processQuery: kuksa authorize query with clientID = "
           + clientID + " with secret " + clientSecret);
      response = processAuthorizeWithPermManager(channel, request_id, clientID, clientSecret);
    } else if (action == "updateVSSTree") {
      string request_id = root["requestId"].as<string>();
      logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: update MetaData query  for with request id " + request_id);
      response = processUpdateVSSTree(channel, request_id, root["metadata"]);
    } else {
      string path = root["path"].as<string>();
      string request_id = root["requestId"].as<string>();
     if (action == "subscribe") {
        logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: subscribe query  for "
             + path + " with request id " + request_id);
        response =
            processSubscribe(channel, request_id, path);
      } else {
        logger->Log(LogLevel::INFO, "VssCommandProcessor::processQuery: Unknown action " + action);
        return JsonResponses::malFormedRequest("Unknown action requested");
      }
    }
  } catch (jsoncons::ser_error &e) {
    logger->Log(LogLevel::ERROR, "JSON parse error");
    return JsonResponses::malFormedRequest(e.what());
  } catch (jsoncons::key_not_found &e) {
    logger->Log(LogLevel::ERROR, "JSON key not found error");
    return JsonResponses::malFormedRequest(e.what());
  } catch (jsoncons::not_an_object &e) {
    logger->Log(LogLevel::ERROR, "JSON not an object error");
    return JsonResponses::malFormedRequest(e.what());
  }


  return response;
}



