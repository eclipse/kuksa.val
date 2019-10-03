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
#include "Authenticator.hpp"
#include "ILogger.hpp"
#include <fstream>
#include <iostream>
#include <list>
#include <string>

#include <jwt-cpp/jwt.h>
#include <jsoncons/json.hpp>
#include "vssdatabase.hpp"
#include "wschannel.hpp"

using namespace std;

// using jsoncons;
using jsoncons::json;

namespace {
  string getPublicKeyFromFile(string fileName) {
    std::ifstream fileStream(fileName);
    std::string key((std::istreambuf_iterator<char>(fileStream)),
                    (std::istreambuf_iterator<char>()));

    return key;
  }
}


void Authenticator::updatePubKey(string key) {
   pubkey = key;
   if(pubkey == "")
      pubkey = getPublicKeyFromFile("jwt.pub.key");
}

// utility method to validate token.
int Authenticator::validateToken(wschannel& channel, string authToken) {
  auto decoded = jwt::decode(authToken);
  json claims;
  (void) channel;
  for (auto& e : decoded.get_payload_claims()) {
    logger->Log(LogLevel::INFO, e.first + " = " + e.second.as_string());
    claims[e.first] = e.second.to_json().to_str();
  }
  
  auto verifier = jwt::verify().allow_algorithm(
      jwt::algorithm::rs256(pubkey, "", "", ""));
  try {
    verifier.verify(decoded);
  } catch (const std::runtime_error& e) {
    logger->Log(LogLevel::ERROR, "Authenticator::validate: " + string(e.what())
         + " Exception occured while authentication. Token is not valid!");
    return -1;
  }

  int ttl = claims["exp"].as<int>();
  channel.setAuthorized(true);
  channel.setAuthToken(authToken);
  return ttl;
}

Authenticator::Authenticator(std::shared_ptr<ILogger> loggerUtil, string secretkey, string algo) {
  logger = loggerUtil;
  algorithm = algo;
  pubkey = secretkey;
}

// validates the token against expiry date/time. should be extended to check
// some other claims.
int Authenticator::validate(wschannel& channel, vssdatabase* db,
                            string authToken) {
  int ttl = validateToken(channel, authToken);
  if (ttl > 0) {
    resolvePermissions(channel, db);
  }

  return ttl;
}

// Checks if the token is still valid for the requests from the channel(client).
// Internally check this before publishing messages for previously subscribed
// signals.
bool Authenticator::isStillValid(wschannel& channel) {
  string token = channel.getAuthToken();
  int ret = validateToken(channel, token);

  if (ret == -1) {
    channel.setAuthorized(false);
    return false;
  } else {
    return true;
  }
}

// **Do this only once for authenticate request**
// resolves the permission in the JWT token and store the absolute path to the
// signals in permissions JSON in wschannel.
void Authenticator::resolvePermissions(wschannel& channel,
                                       vssdatabase* database) {
  string authToken = channel.getAuthToken();
  auto decoded = jwt::decode(authToken);
  json claims;
  for (auto& e : decoded.get_payload_claims()) {
    logger->Log(LogLevel::INFO, string(e.first) + " = " + e.second.to_json().to_str());
    stringstream value;
    value << e.second.to_json();
    claims[e.first] = json::parse(value.str());
  }

  if (claims.has_key("w3c-vss")) {
    json tokenPermJson = claims["w3c-vss"];
    json permissions;
    for (auto path : tokenPermJson.object_range()) {
      bool isBranch;
      string pathString(path.key());
      list<string> paths = database->getPathForGet(pathString, isBranch);

      for (string verifiedPath : paths) {
        permissions.insert_or_assign(verifiedPath, path.value());
      }
    }
    channel.setPermissions(permissions);
  }
}
