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
#include "authenticator.hpp"
#include <fstream>
#include <iostream>

authenticator::authenticator(string secretkey, string algo) {
  algorithm = algo;
  key = secretkey;
}

string getPublicKey(string fileName) {
  std::ifstream fileStream(fileName);
  std::string key((std::istreambuf_iterator<char>(fileStream)),
                  (std::istreambuf_iterator<char>()));

  return key;
}

// utility method to validate token.
int validateToken(wschannel& channel, string authToken) {
  auto decoded = jwt::decode(authToken);
  json claims;
  for (auto& e : decoded.get_payload_claims()) {
    std::cout << e.first << " = " << e.second.to_json() << std::endl;
    claims[e.first] = e.second.to_json().to_str();
  }

  string rsa_pub_key = getPublicKey("jwt.pub.key");
  auto verifier = jwt::verify().allow_algorithm(
      jwt::algorithm::rs256(rsa_pub_key, "", "", ""));

  try {
    verifier.verify(decoded);
  } catch (const std::runtime_error& e) {
    cout << "authenticator::validate: " << e.what()
         << " Exception occured while authentication. Token is not valid!"
         << endl;
    return -1;
  }

  int ttl = claims["exp"].as<int>();
  channel.setAuthorized(true);
  channel.setAuthToken(authToken);
  return ttl;
}

// validates the token against expiry date/time. should be extended to check
// some other claims.
int authenticator::validate(wschannel& channel, vssdatabase* db,
                            string authToken) {
  int ttl = validateToken(channel, authToken);
  if (ttl > 0) resolvePermissions(channel, db);

  return ttl;
}

// Checks if the token is still valid for the requests from the channel(client).
// Internally check this before publishing messages for previously subscribed
// signals.
bool authenticator::isStillValid(wschannel& channel) {
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
json authenticator::resolvePermissions(wschannel& channel,
                                       vssdatabase* database) {
  string authToken = channel.getAuthToken();
  auto decoded = jwt::decode(authToken);
  json claims;
  for (auto& e : decoded.get_payload_claims()) {
    std::cout << e.first << " = " << e.second.to_json() << std::endl;
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
  } else {
    return json::parse("{}");
  }
}
