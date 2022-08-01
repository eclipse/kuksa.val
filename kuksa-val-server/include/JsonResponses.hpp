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
 *      Robert Bosch GmbH - initial API and functionality
 **********************************************************************/

#ifndef __DEFAULTJSONRESPONSES___
#define __DEFAULTJSONRESPONSES___

#include <string>
#include <jsoncons/json.hpp>

namespace JsonResponses {
  jsoncons::json malFormedRequest(std::string request_id,
                               const std::string action,
                               std::string message);

  void malFormedRequest(std::string request_id,
                        const std::string action,
                        std::string message,
                        jsoncons::json& jsonResponse);

  jsoncons::json malFormedRequest(std::string message, std::string requestId="UNKNOWN");
  
  void malFormedRequest(std::string message,
                        jsoncons::json& jsonResponse, std::string requesId="UNKNOWN");

  /** A API call requested a non-existant path */
  jsoncons::json pathNotFound(std::string request_id,
                           const std::string action,
                           const std::string path);

  void pathNotFound(std::string request_id,
                    const std::string action,
                    const std::string path,
                    jsoncons::json& jsonResponse);

  jsoncons::json noAccess(std::string request_id,
                       const std::string action,
                       std::string message);

  void noAccess(std::string request_id,
                const std::string action,
                std::string message,
                jsoncons::json& jsonResponse);

  jsoncons::json valueOutOfBounds(std::string request_id,
                               const std::string action,
                               const std::string message);

  void valueOutOfBounds(std::string request_id,
                        const std::string action,
                        const std::string message,
                        jsoncons::json& jsonResponse);

  void notSetResponse(std::string message, jsoncons::json& jsonResponse,
                      std::string requestId);

  jsoncons::json notSetResponse(std::string request_id, std::string message);

  std::string getTimeStamp();

  void addTimeStampToJSON(jsoncons::json& jsontarget, const std::string suffix="");

  void convertJSONTimeStampToISO8601(jsoncons::json& jsontarget);

  std::string getTimeStampZero();
}

#endif
