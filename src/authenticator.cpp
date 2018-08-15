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

authenticator::authenticator( string secretkey, string algo) {
   algorithm = algo; 
   key = secretkey;

}

int authenticator::validate (wschannel &channel , string authToken) {
  
  auto decoded = jwt::decode(authToken);

  for(auto& e : decoded.get_payload_claims())
      std::cout << e.first << " = " << e.second.to_json() << std::endl;
  
  auto verifier = jwt::verify()
	.allow_algorithm(jwt::algorithm::hs256{ key })
	.with_issuer("kuksa");
  try {
      verifier.verify(decoded);
  } catch (const std::runtime_error& e) {
      cout << " "<< e.what() << " Exception occured while authentification. Token is not valid!"<<endl;
      return -1; 
  }

  int ttl = 0;
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
