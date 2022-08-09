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
 **********************************************************************/

#ifndef __IAUTHENTICATOR_H__
#define __IAUTHENTICATOR_H__

#include <memory>
#include <string>

#include "KuksaChannel.hpp"

using namespace std;

class ILogger;

class IAuthenticator {
public:
  virtual ~IAuthenticator() {}

  virtual int validate(KuksaChannel &channel,
                       string authToken) = 0;
  virtual void updatePubKey(string key) = 0;
  virtual bool isStillValid(KuksaChannel &channel) = 0;
  virtual void resolvePermissions(KuksaChannel &channel) = 0;
};

#endif
