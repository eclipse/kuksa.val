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


# include "authenticator.hpp"
#include <iostream>
#include <jsoncons/json.hpp>
#include <iostream>
#include <fstream>

using namespace jsoncons;
using jsoncons::json;

authenticator::authenticator( string secretkey, string algo) {
   algorithm = algo; 
   key = secretkey;

}

string getPublicKey (string fileName) {

  std::ifstream fileStream (fileName);
  std::string key( (std::istreambuf_iterator<char>(fileStream)),
                       (std::istreambuf_iterator<char>()));

  return key;
}

int authenticator::validate (wschannel &channel , string authToken) {
  
  auto decoded = jwt::decode(authToken);
  json claims;
  for(auto& e : decoded.get_payload_claims()) {
      std::cout << e.first << " = " << e.second.to_json() << std::endl;
      claims[e.first] = e.second.to_json().to_str();
  }

   string rsa_pub_key = getPublicKey("jwt.key.pub");
   auto verifier =jwt::verify()
   .allow_algorithm(jwt::algorithm::rs256(rsa_pub_key, "", "", ""));
  
  try {
      verifier.verify(decoded);
  } catch (const std::runtime_error& e) {
      cout << "authenticator::validate: "<< e.what() << " Exception occured while authentification. Token is not valid!"<<endl;
      return -1; 
  }

  int ttl = claims["exp"].as<int>();
  channel.setAuthorized(true);
  channel.setAuthToken(authToken);

  return ttl;
}

bool authenticator::isStillValid (wschannel &channel) {

  string token = channel.getAuthToken();
  int ret = validate (channel , token);
  
   if( ret == -1) {
      return false;
   } else {
      return true;
   }
}
