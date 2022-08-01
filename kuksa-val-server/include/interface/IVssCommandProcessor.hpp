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

#ifndef __IVSSCOMMANDPROCESSOR_H__
#define __IVSSCOMMANDPROCESSOR_H__

#include <string>

#include <jsoncons/json.hpp>
#include "KuksaChannel.hpp"

/**
 * @class IVssCommandProcessor
 * @brief Interface class for handling input JSON requests and providing response
 *
 * @note  Any class implementing this interface shall handle JSON requests as defined by
 *        https://www.w3.org/TR/vehicle-information-service/#message-structure
 */
class IVssCommandProcessor {
  public:
    virtual ~IVssCommandProcessor() {}

    /**
     * @brief Process JSON request and provide response JSON string
     * @param req_json JSON formated request
     * @param channel Active channel information on which \a req_json was received
     * @return JSON formated response string
     */
    virtual jsoncons::json processQuery(const std::string &req_json,
                                     KuksaChannel& channel) = 0;
};

#endif
