/**********************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Contributors:
 *      Robert Bosch GmbH
 **********************************************************************/

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
