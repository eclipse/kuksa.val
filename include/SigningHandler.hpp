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
#ifndef __SIGNING_H__
#define __SIGNING_H__

#include <stdexcept>
#include <jwt-cpp/base.h>
#include <jwt-cpp/jwt.h>
#include <fstream>
#include <iostream>
#include <jsoncons/json.hpp>

#include <memory>

#include "ISigningHandler.hpp"

using namespace std;
using namespace jsoncons;
using namespace jwt;

class ILogger;

class SigningHandler : public ISigningHandler {
 private:
  std::shared_ptr<ILogger> logger;
  string key = "";
  string pubkey = "";
  string algorithm = "RS256";

 public:
  SigningHandler(std::shared_ptr<ILogger> loggerUtil);
  string getKey(const string &fileName);
  string getPublicKey(const string &fileName);
  string sign(const json &data);
  string sign(const string &data);
#ifdef UNIT_TEST
  string decode(string signedData);
#endif
};

#endif
