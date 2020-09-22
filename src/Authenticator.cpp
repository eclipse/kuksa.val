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
#include "Authenticator.hpp"
#include "ILogger.hpp"
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <unistd.h>

#include <jwt-cpp/jwt.h>
#include <jsoncons/json.hpp>
#include "VssDatabase.hpp"
#include "WsChannel.hpp"

using namespace std;

// using jsoncons;
using jsoncons::json;

string Authenticator::getPublicKeyFromFile(string fileName, std::shared_ptr<ILogger> logger) {
  logger->Log(LogLevel::VERBOSE, "Try reading JWT pub key from "+fileName);

  if ( access(fileName.c_str(),F_OK) != 0 )
  {
    logger->Log(LogLevel::ERROR, "Unable to find JWT pub key "+fileName);
    return "";
  }
  std::ifstream fileStream(fileName);
  if (fileStream.fail()) {
    logger->Log(LogLevel::ERROR, "Unable to open JWT pub key "+fileName);
  }
  std::string key((std::istreambuf_iterator<char>(fileStream)),
                   (std::istreambuf_iterator<char>()));
  return key;
}


void Authenticator::updatePubKey(string key) {
  pubkey=key;
  if (key == "") {
    logger->Log(LogLevel::WARNING, "Empty key in Authenticator::updatePubKey. Subsequent JWT token validations will fail.");
    return;
  }
  logger->Log(LogLevel::VERBOSE, "Updated JWT token validation public key.");
}

// utility method to validate token.
int Authenticator::validateToken(WsChannel& channel, string authToken) {
  json claims;
  int ttl = -1;

  try {
    auto decoded = jwt::decode(authToken);

    for (auto& e : decoded.get_payload_claims()) {
      logger->Log(LogLevel::INFO, e.first + " = " + e.second.to_json().to_str());
      claims[e.first] = e.second.to_json().to_str();
    }

    auto verifier = jwt::verify().allow_algorithm(
        jwt::algorithm::rs256(pubkey, "", "", ""));
        
    try {
      verifier.verify(decoded);
    } catch (const std::runtime_error& e) {
      logger->Log(LogLevel::ERROR, "Authenticator::validate: " + string(e.what())
                  + " Exception occurred while authentication. Token is not valid!");
      return -1;
    }

    channel.setAuthorized(true);
    channel.setAuthToken(authToken);
    ttl = claims["exp"].as<int>();
  }
  catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Authenticator::validate: " + string(e.what())
                + " Exception occurred while decoding token!");
  }
  return ttl;
}

Authenticator::Authenticator(std::shared_ptr<ILogger> loggerUtil, string secretkey, string algo) {
  logger = loggerUtil;
  algorithm = algo;
  pubkey = secretkey;
}

// validates the token against expiry date/time. should be extended to check
// some other claims.
int Authenticator::validate(WsChannel& channel,
                            string authToken) {
  int ttl = validateToken(channel, authToken);
  if (ttl > 0) {
    resolvePermissions(channel);
  }

  return ttl;
}

// Checks if the token is still valid for the requests from the channel(client).
// Internally check this before publishing messages for previously subscribed
// signals.
bool Authenticator::isStillValid(WsChannel& channel) {
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
// signals in permissions JSON in WsChannel.
void Authenticator::resolvePermissions(WsChannel& channel) {
  string authToken = channel.getAuthToken();
  auto decoded = jwt::decode(authToken);
  json claims;
  for (auto& e : decoded.get_payload_claims()) {
    logger->Log(LogLevel::INFO, string(e.first) + " = " + e.second.to_json().to_str());
    stringstream value;
    value << e.second.to_json();
    claims[e.first] = json::parse(value.str());
  }

  json permissions;
  if (claims.contains("modifyTree") && claims["modifyTree"].as<bool>()) {
    channel.enableModifyTree();
  }

  if (claims.contains("kuksa-vss")) {
    json tokenPermJson = claims["kuksa-vss"];
    for (auto permission : tokenPermJson.object_range()) {
      // TODO use regex to check
      if(permission.value() == "rw"
        || permission.value() == "wr"
        || permission.value() == "w"
        || permission.value() == "r"){
        permissions.insert_or_assign(permission.key(), permission.value());
      } else {
        logger->Log(LogLevel::ERROR, "Permission for " + string(permission.key()) + " = " + permission.value().as<std::string>() + " is not valid, only r|w are supported");
      }
    }
  }
  channel.setPermissions(permissions);
}
