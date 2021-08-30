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

#include <list>
#include <string>

#include <jsoncons/json.hpp>
#include "VssCommandProcessor.hpp"

// #include "VssDatabase.hpp"
// #include "WsServer.hpp"
// #include "SubscriptionHandler.hpp"
// #include "Authenticator.hpp"
// #include "AccessChecker.hpp"
// #include "SigningHandler.hpp"

// using namespace std;
// using namespace jsoncons;
// using namespace jsoncons::jsonpath;
// using jsoncons::json;

class kuksavalunittest {

 public:
    std::shared_ptr<VssCommandProcessor> commandProc_auth_;
    kuksavalunittest();
    ~kuksavalunittest();
};



#endif
