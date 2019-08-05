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

#include "vsscommandprocessor.hpp"

#include <stdint.h>
#include <iostream>
#include <sstream>

#include "exception.hpp"
#include "server_ws.hpp"
#include "visconf.hpp"
#include "vssdatabase.hpp"
#include "accesschecker.hpp"
#include "subscriptionhandler.hpp"

#ifdef JSON_SIGNING_ON
#include "signing.hpp"
#endif

using namespace std;

string malFormedRequestResponse(uint32_t request_id, const string action) {
  jsoncons::json answer;
  answer["action"] = action;
  answer["requestId"] = request_id;
  jsoncons::json error;
  error["number"] = 400;
  error["reason"] = "Request malformed";
  error["message"] = "Request malformed";
  answer["error"] = error;
  answer["timestamp"] = time(NULL);
  stringstream ss;
  ss << pretty_print(answer);
  return ss.str();
}

/** A API call requested a non-existant path */
string pathNotFoundResponse(uint32_t request_id, const string action,
                            const string path) {
  jsoncons::json answer;
  answer["action"] = action;
  answer["requestId"] = request_id;
  jsoncons::json error;
  error["number"] = 404;
  error["reason"] = "Path not found";
  error["message"] = "I can not find " + path + " in my db";
  answer["error"] = error;
  answer["timestamp"] = time(NULL);
  stringstream ss;
  ss << pretty_print(answer);
  return ss.str();
}

string noAccessResponse(uint32_t request_id, const string action,
                        string message) {
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

string valueOutOfBoundsResponse(uint32_t request_id, const string action,
                                const string message) {
  jsoncons::json answer;
  answer["action"] = action;
  answer["requestId"] = request_id;
  jsoncons::json error;
  error["number"] = 400;
  error["reason"] = "Value passed is out of bounds";
  error["message"] = message;
  answer["error"] = error;
  answer["timestamp"] = time(NULL);
  stringstream ss;
  ss << pretty_print(answer);
  return ss.str();
}

vsscommandprocessor::vsscommandprocessor(
    vssdatabase *dbase, authenticator *vdator,
    subscriptionhandler *subhandler) {
  database = dbase;
  tokenValidator = vdator;
  subHandler = subhandler;
  accessValidator = new accesschecker(tokenValidator);
#ifdef JSON_SIGNING_ON
  signer = new signing();
#endif
}

vsscommandprocessor::~vsscommandprocessor() {
  delete accessValidator;
}

string vsscommandprocessor::processGet(wschannel &channel,
                                       uint32_t request_id, string path) {
#ifdef DEBUG
  cout << "GET :: path received from client = " << path << endl;
#endif
  jsoncons::json res;
  try {
    res = database->getSignal(channel, path);
  } catch (noPermissionException &nopermission) {
    cout << nopermission.what() << endl;
    return noAccessResponse(request_id, "get", nopermission.what());
  }
  if (!res.has_key("value")) {
    return pathNotFoundResponse(request_id, "get", path);
  } else {
    res["action"] = "get";
    res["requestId"] = request_id;
    res["timestamp"] = time(NULL);
    stringstream ss;
    ss << pretty_print(res);
    return ss.str();
  }
}

string vsscommandprocessor::processSet(wschannel &channel,
                                       uint32_t request_id, string path,
                                       jsoncons::json value) {
#ifdef DEBUG
  cout << "vsscommandprocessor::processSet: path received from client" << path
       << endl;
#endif

  try {
    database->setSignal(channel, path, value);
  } catch (genException &e) {
    cout << e.what() << endl;
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
    cout << e.what() << endl;
    return pathNotFoundResponse(request_id, "set", path);
  } catch (outOfBoundException &outofboundExp) {
    cout << outofboundExp.what() << endl;
    return valueOutOfBoundsResponse(request_id, "set", outofboundExp.what());
  } catch (noPermissionException &nopermission) {
    cout << nopermission.what() << endl;
    return noAccessResponse(request_id, "set", nopermission.what());
  }

  jsoncons::json answer;
  answer["action"] = "set";
  answer["requestId"] = request_id;
  answer["timestamp"] = time(NULL);

  std::stringstream ss;
  ss << pretty_print(answer);
  return ss.str();
}

string vsscommandprocessor::processSubscribe(wschannel &channel,
                                             uint32_t request_id, string path,
                                             uint32_t connectionID) {
#ifdef DEBUG
  cout << "vsscommandprocessor::processSubscribe: path received from client "
          "for subscription"
       << path << endl;
#endif

  uint32_t subId = -1;
  try {
    subId = subHandler->subscribe(channel, database, connectionID, path);
  } catch (noPathFoundonTree &noPathFound) {
    cout << noPathFound.what() << endl;
    return pathNotFoundResponse(request_id, "subscribe", path);
  } catch (genException &outofboundExp) {
    cout << outofboundExp.what() << endl;
    return valueOutOfBoundsResponse(request_id, "subscribe",
                                    outofboundExp.what());
  } catch (noPermissionException &nopermission) {
    cout << nopermission.what() << endl;
    return noAccessResponse(request_id, "subscribe", nopermission.what());
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

string vsscommandprocessor::processUnsubscribe(uint32_t request_id,
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

string vsscommandprocessor::processGetMetaData(uint32_t request_id,
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

string vsscommandprocessor::processAuthorize(wschannel &channel,
                                             uint32_t request_id,
                                             string token) {
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

string vsscommandprocessor::processQuery(string req_json,
                                         wschannel &channel) {
  jsoncons::json root;
  string response;
  root = jsoncons::json::parse(req_json);
  string action = root["action"].as<string>();

  if (action == "authorize") {
    string token = root["tokens"].as<string>();
    uint32_t request_id = root["requestId"].as<int>();
#ifdef DEBUG
    cout << "vsscommandprocessor::processQuery: authorize query with token = "
         << token << " with request id " << request_id << endl;
#endif
    response = processAuthorize(channel, request_id, token);
  } else if (action == "unsubscribe") {
    uint32_t request_id = root["requestId"].as<int>();
    uint32_t subscribeID = root["subscriptionId"].as<int>();
#ifdef DEBUG
    cout
        << "vsscommandprocessor::processQuery: unsubscribe query  for sub ID = "
        << subscribeID << " with request id " << request_id << endl;
#endif
    response = processUnsubscribe(request_id, subscribeID);
  } else {
    string path = root["path"].as<string>();
    uint32_t request_id = root["requestId"].as<int>();

    if (action == "get") {
#ifdef DEBUG
      cout << "vsscommandprocessor::processQuery: get query  for " << path
           << " with request id " << request_id << endl;
#endif

      response = processGet(channel, request_id, path);
#ifdef JSON_SIGNING_ON
      response = signer->sign(response);
#endif
    } else if (action == "set") {
      jsoncons::json value = root["value"];
#ifdef DEBUG
      cout << "vsscommandprocessor::processQuery: set query  for " << path
           << " with request id " << request_id << " value "
           << pretty_print(value) << endl;
#endif
      response = processSet(channel, request_id, path, value);
    } else if (action == "subscribe") {
#ifdef DEBUG
      cout << "vsscommandprocessor::processQuery: subscribe query  for " << path
           << " with request id " << request_id << endl;
#endif
      response =
          processSubscribe(channel, request_id, path, channel.getConnID());
    } else if (action == "getMetadata") {
#ifdef DEBUG
      cout << "vsscommandprocessor::processQuery: metadata query  for " << path
           << " with request id " << request_id << endl;
#endif
      response = processGetMetaData(request_id, path);
    } else {
      cout << "vsscommandprocessor::processQuery: Unknown action " << action
           << endl;
    }
  }

  return response;
}
