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

#ifndef __IRESTHANDLER_H__
#define __IRESTHANDLER_H__

#include <string>

/**
 * @class IRestHandler
 * @brief Interface class for handling REST API and convert them to default JSON
 *
 * @note  Any class implementing this interface shall output JSON requests as defined by
 *        https://www.w3.org/TR/vehicle-information-service/#message-structure
 */
class IRestHandler {
  public:
    virtual ~IRestHandler() = default;

    /**
     * @brief Get JSON request string based on input REST API request
     * @param restMethod REST request method string (get/set/...)
     * @param restTarget REST request URI
     * @param resultJson Output JSON string for both valid and invalid REST requests. If invalid REST
     *        request, parameter shall contain JSON describing error
     * @return true if REST request correct and JSON request generated
     *         false otherwise indicating error in REST request. \a jsonRequest shall be updated
     *         with JSON response providing error details
     *
     * @note Function can be updated/extended/overloaded to provide additional relevant information,
     *       (e.g. HTTP header information, body of request with JSON, etc...)
     */
    virtual bool GetJson(std::string&& restMethod,
                         std::string&& restTarget,
                         std::string& resultJson) = 0;
};


#endif
