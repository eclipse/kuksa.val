/*
 * ******************************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
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

#ifdef JSON_SIGNING_ON
#include "SigningHandler.hpp"
#endif

using namespace std;

VssCommandProcessor::VssCommandProcessor(
    std::shared_ptr<ILogger> loggerUtil,
    std::shared_ptr<IVssDatabase> dbase,
    std::shared_ptr<IAuthenticator> vdator,
    std::shared_ptr<ISubscriptionHandler> subhandler) {
  logger = loggerUtil;
  database = dbase;
  tokenValidator = vdator;
  subHandler = subhandler;
  // TODO: add accessValidator as dependency
  accessValidator = std::make_shared<AccessChecker>(tokenValidator);
#ifdef JSON_SIGNING_ON
  // TODO: add signer as dependency
  signer = std::make_shared<SigningHandler>();
#endif
}

VssCommandProcessor::~VssCommandProcessor() {
  accessValidator.reset();
#ifdef JSON_SIGNING_ON
  signer.reset();
#endif
}

string VssCommandProcessor::processGet(WsChannel &channel,
                                       uint32_t request_id, string path) {
  logger->Log(LogLevel::VERBOSE, "GET :: path received from client = " + path);
  jsoncons::json res;
  try {
    res = database->getSignal(channel, path);
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::ERROR, string(nopermission.what()));
    return JsonResponses::noAccess(request_id, "get", nopermission.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(request_id, "get", string("Unhandled error: ") + e.what());
  }

  if (!res.has_key("value")) {
    return JsonResponses::pathNotFound(request_id, "get", path);
  } else {
    res["action"] = "get";
    res["requestId"] = request_id;
    res["timestamp"] = time(NULL);
    stringstream ss;
    ss << pretty_print(res);
    return ss.str();
  }
}

string VssCommandProcessor::processSet(WsChannel &channel,
                                       uint32_t request_id, string path,
                                       jsoncons::json value) {
  logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processSet: path received from client" + path);

  try {
    database->setSignal(channel, path, value);
  } catch (genException &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    jsoncons::json root;
    jsoncons::json error;

    root["action"] = "set";
    root["requestId"] = request_id;

    error["number"] = 401;
    error["reason"] = "Unknown error";
    error["message"] = e.what();

    root["error"] = error;
    root["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(root);
    return ss.str();
  } catch (noPathFoundonTree &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    return JsonResponses::pathNotFound(request_id, "set", path);
  } catch (outOfBoundException &outofboundExp) {
    logger->Log(LogLevel::ERROR, string(outofboundExp.what()));
    return JsonResponses::valueOutOfBounds(request_id, "set", outofboundExp.what());
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::ERROR, string(nopermission.what()));
    return JsonResponses::noAccess(request_id, "set", nopermission.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(request_id, "get", string("Unhandled error: ") + e.what());
  }

  jsoncons::json answer;
  answer["action"] = "set";
  answer["requestId"] = request_id;
  answer["timestamp"] = time(NULL);

  std::stringstream ss;
  ss << pretty_print(answer);
  return ss.str();
}

string VssCommandProcessor::processSubscribe(WsChannel &channel,
                                             uint32_t request_id, string path) {
  logger->Log(LogLevel::VERBOSE, string("VssCommandProcessor::processSubscribe: path received from client ")
              + string("for subscription"));

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
    answer["timestamp"] = time(NULL);

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
    root["timestamp"] = time(NULL);

    std::stringstream ss;

    ss << pretty_print(root);
    return ss.str();
  }
}

string VssCommandProcessor::processUnsubscribe(uint32_t request_id,
                                               uint32_t subscribeID) {
  int res = subHandler->unsubscribe(subscribeID);
  if (res == 0) {
    jsoncons::json answer;
    answer["action"] = "unsubscribe";
    answer["requestId"] = request_id;
    answer["subscriptionId"] = subscribeID;
    answer["timestamp"] = time(NULL);

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
    root["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(root);
    return ss.str();
  }
}

string VssCommandProcessor::processGetMetaData(uint32_t request_id,
                                               string path) {
  jsoncons::json st = database->getMetaData(path);

  jsoncons::json result;
  result["action"] = "getMetadata";
  result["requestId"] = request_id;
  result["metadata"] = st;
  result["timestamp"] = time(NULL);

  std::stringstream ss;
  ss << pretty_print(result);

  return ss.str();
}

// Talks to the permission management daemon and processes the token received.
string VssCommandProcessor::processAuthorizeWithPermManager(WsChannel &channel,
                                                            uint32_t request_id,
                                                            string client, string clientSecret) {

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
    result["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();
  }
  int ttl = -1;
  if (response.has_key("token") && response.has_key("pubkey")) {
     try {
        tokenValidator->updatePubKey(response["pubkey"].as<string>());
        ttl = tokenValidator->validate(channel, database, response["token"].as<string>());
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
    result["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();

  } else {
    jsoncons::json result;
    result["action"] = "kuksa-authorize";
    result["requestId"] = request_id;
    result["TTL"] = ttl;
    result["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();
  }
}

string VssCommandProcessor::processAuthorize(WsChannel &channel,
                                             uint32_t request_id,
                                             string token) {
  tokenValidator->updatePubKey("");
  int ttl = tokenValidator->validate(channel, database, token);

  if (ttl == -1) {
    jsoncons::json result;
    jsoncons::json error;
    result["action"] = "authorize";
    result["requestId"] = request_id;
    error["number"] = 401;
    error["reason"] = "Invalid Token";
    error["message"] = "Check the JWT token passed";

    result["error"] = error;
    result["timestamp"] = time(NULL);

    std::stringstream ss;
    ss << pretty_print(result);
    return ss.str();

  } else {
    jsoncons::json result;
    result["action"] = "authorize";
    result["requestId"] = request_id;
    result["TTL"] = ttl;
    result["timestamp"] = time(NULL);

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

    if (action == "authorize") {
      string token = root["tokens"].as<string>();
      uint32_t request_id = root["requestId"].as<int>();
      logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: authorize query with token = "
           + token + " with request id " + to_string(request_id));

      response = processAuthorize(channel, request_id, token);
    } else if (action == "unsubscribe") {
      uint32_t request_id = root["requestId"].as<int>();
      uint32_t subscribeID = root["subscriptionId"].as<int>();
      logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: unsubscribe query  for sub ID = "
              + to_string(subscribeID) + " with request id " + to_string(request_id));

      response = processUnsubscribe(request_id, subscribeID);
    } else if ( action == "kuksa-authorize") {
      string clientID = root["clientid"].as<string>();
      string clientSecret = root["secret"].as<string>();
      uint32_t request_id = root["requestId"].as<int>();
      logger->Log(LogLevel::VERBOSE, "vsscommandprocessor::processQuery: kuksa authorize query with clientID = "
           + clientID + " with secret " + clientSecret);
      response = processAuthorizeWithPermManager(channel, request_id, clientID, clientSecret);
    } else {
      string path = root["path"].as<string>();
      uint32_t request_id = root["requestId"].as<int>();

      if (action == "get") {
        logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: get query  for " + path
                    + " with request id " + to_string(request_id));

        response = processGet(channel, request_id, path);
#ifdef JSON_SIGNING_ON
        response = signer->sign(response);
#endif
      } else if (action == "set") {
        jsoncons::json value = root["value"];

        logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: set query  for " + path
             + " with request id " + to_string(request_id) + " value " + value.as_string());
        response = processSet(channel, request_id, path, value);
      } else if (action == "subscribe") {
        logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: subscribe query  for "
             + path + " with request id " + to_string(request_id));
        response =
            processSubscribe(channel, request_id, path);
      } else if (action == "getMetadata") {
        logger->Log(LogLevel::VERBOSE, "VssCommandProcessor::processQuery: metadata query  for "
             + path + " with request id " + to_string(request_id));
        response = processGetMetaData(request_id, path);
      } else {
        logger->Log(LogLevel::INFO, "VssCommandProcessor::processQuery: Unknown action " + action);
        return JsonResponses::malFormedRequest("Unknown action requested");
      }
    }
  } catch (jsoncons::json_parse_exception e) {
    logger->Log(LogLevel::ERROR, "JSON parse error");
    return JsonResponses::malFormedRequest(e.what());
  } catch (jsoncons::key_not_found e) {
    logger->Log(LogLevel::ERROR, "JSON key not found error");
    return JsonResponses::malFormedRequest(e.what());
  } catch (jsoncons::not_an_object e) {
    logger->Log(LogLevel::ERROR, "JSON not an object error");
    return JsonResponses::malFormedRequest(e.what());
  }


  return response;
}
