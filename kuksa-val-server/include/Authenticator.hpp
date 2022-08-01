/**********************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
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
 *      Robert Bosch GmbH - initial API and functionality
 **********************************************************************/

#ifndef __AUTHENTICATOR_H__
#define __AUTHENTICATOR_H__

#include <memory>
#include <string>

#include "IAuthenticator.hpp"

using namespace std;

class ILogger;

class Authenticator : public IAuthenticator {
 private:
  string pubkey = "secret";
  string algorithm = "RS256";
  std::shared_ptr<ILogger> logger;

  int validateToken(KuksaChannel& channel, string authToken);

 public:
  Authenticator(std::shared_ptr<ILogger> loggerUtil, string secretkey, string algorithm);

  int validate(KuksaChannel &channel,
               string authToken);

  void updatePubKey(string key);
  bool isStillValid(KuksaChannel &channel);
  void resolvePermissions(KuksaChannel &channel);

  static string getPublicKeyFromFile(string fileName, std::shared_ptr<ILogger> logger);
};
#endif
