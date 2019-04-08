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
#ifndef __W3CUNITTEST_HPP__
#define __W3CUNITTEST_HPP__

#include <jsoncons/json.hpp>
#include "vssdatabase.hpp"
#include "wsserver.hpp"
#include "subscriptionhandler.hpp"
#include "authenticator.hpp"
#include "accesschecker.hpp"
#include "signing.hpp"

using namespace std;
using namespace jsoncons;
using namespace jsoncons::jsonpath;
using jsoncons::json;

class w3cunittest {

 public:

    w3cunittest(bool secure);
    ~w3cunittest();
    list<string> test_wrap_getPathForGet(string path , bool &isBranch);
    json test_wrap_getPathForSet(string path,  json value);
};



#endif
