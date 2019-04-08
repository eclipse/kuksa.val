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

#include "signing.hpp"

#define PRIVATEFILENAME "signing.private.key"
#define PUBLICFILENAME "signing.public.key"

/**
 Constructor
*/
signing::signing() {
   getKey(PRIVATEFILENAME);
   getPublicKey(PUBLICFILENAME); 
   
}

/**
 Get the private key for signing.
*/
string signing::getKey (string fileName) {
  
  std::ifstream fileStream(fileName);
  std::string privatekey((std::istreambuf_iterator<char>(fileStream)),
                          (std::istreambuf_iterator<char>()));
  key = privatekey;
  return key;
}

/**
 Get the public key for signing.
*/
string signing::getPublicKey (string fileName) {

  std::ifstream fileStream (fileName);
  std::string privatekey( (std::istreambuf_iterator<char>(fileStream)),
                       (std::istreambuf_iterator<char>()));
  pubkey = privatekey;
  return pubkey;
}

/**
 Signs the JSON and returns a string token
*/
string signing::sign(json data) {
   auto algo = jwt::algorithm::rs256(pubkey, key, "", "");
   auto encode = [](const std::string& data) {
	auto base = base::encode<alphabet::base64url>(data);
	auto pos = base.find(alphabet::base64url::fill());
	base = base.substr(0, pos);
	return base;
   };

   string header_json (R"({
		"alg": "RS256",
		"typ": "GENIVI-VSS"
   })");

   std::string header = encode(header_json);
   std::string payload = encode(data.as<string>());
   std::string token = header + "." + payload;

   return token + "." + encode(algo.sign(token));
}

/**
 Signs the JSON and returns a string token
*/
string signing::sign(string data) {
   auto algo = jwt::algorithm::rs256(pubkey, key, "", "");
   auto encode = [](const std::string& data) {
	auto base = base::encode<alphabet::base64url>(data);
	auto pos = base.find(alphabet::base64url::fill());
	base = base.substr(0, pos);
	return base;
   };

   string header_json (R"({
		"alg": "RS256",
		"typ": "GENIVI-VSS"
   })");

   std::string header = encode(header_json);
   std::string payload = encode(data);
   std::string token = header + "." + payload;

   return token + "." + encode(algo.sign(token));
}

#ifdef UNIT_TEST
string signing::decode(string signedData) {
   
   auto verify = jwt::verify()
		.allow_algorithm(jwt::algorithm::rs256(pubkey, "", "", ""));

   auto decoded_token = jwt::decode(signedData);
   try {
      verify.verify(decoded_token);
   } catch (exception &e) {
      cout <<"Error while verfying JSON signing " << e.what() << endl;
      return "";
   }
   
   return decoded_token.get_payload();
}  
#endif
