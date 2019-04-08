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

#include <jwt-cpp/base.h>
#include <jwt-cpp/jwt.h>
#include <jsoncons/json.hpp>
#include <fstream>
#include <iostream>

using namespace std;
using namespace jsoncons;
using namespace jwt;

class signing {

private:
    string key = "";
    string pubkey = "";
    string algorithm = "RS256";

public:
   signing();
   string getKey (string fileName);
   string getPublicKey (string fileName);
   string sign(json data);
   string sign(string data);
#ifdef UNIT_TEST
   string decode(string signedData);
#endif
};

#endif
