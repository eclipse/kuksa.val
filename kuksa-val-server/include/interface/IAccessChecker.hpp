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

#ifndef __IACCESSCHECKER_H__
#define __IACCESSCHECKER_H__

#include <jsoncons/json.hpp>
#include <string>

#include "KuksaChannel.hpp"
#include "VSSPath.hpp"

class IAccessChecker {
 public:
  virtual ~IAccessChecker() {}

  virtual bool checkPathWriteAccess(KuksaChannel &channel,
                                    const jsoncons::json &paths) = 0;
  virtual bool checkReadAccess(KuksaChannel &channel, const VSSPath &path) = 0;
  virtual bool checkWriteAccess(KuksaChannel &channel, const VSSPath &path) = 0;
};

#endif
