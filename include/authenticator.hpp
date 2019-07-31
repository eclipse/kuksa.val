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
#ifndef __AUTHENTICATOR_H__
#define __AUTHENTICATOR_H__

#include <jwt-cpp/jwt.h>
#include <jsoncons/json.hpp>
#include "vssdatabase.hpp"
#include "wschannel.hpp"

using namespace std;
using namespace jsoncons;
using jsoncons::json;

class authenticator {
 private:
  string key = "secret";
  string algorithm = "HS256";

 public:
  authenticator(string secretkey, string algorithm);
  int validate(wschannel &channel, class vssdatabase *database,
               string authToken);
  bool isStillValid(wschannel &channel);
  void resolvePermissions(wschannel &channel, class vssdatabase *database);
};
#endif
