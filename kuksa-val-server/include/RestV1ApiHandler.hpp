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
 
#ifndef __REST2JSONCONVERTER___
#define __REST2JSONCONVERTER___

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include <jsoncons/json.hpp>

#include "IRestHandler.hpp"


class ILogger;

/**
 *  @class RestV1ApiHandler
 *  @brief Handle initial implementation of REST API for VIS server
 */
class RestV1ApiHandler : public IRestHandler {
  private:
    std::shared_ptr<ILogger> logger_;
    std::string docRoot_;
    std::vector<std::string> resourceHandleNames_;
    //const std::unordered_map<std::string, std::function> resourceHandlers_;
    std::string regexResources_;
    std::string regexHttpMethods_;
    std::string regexToken_;

  public:
    /**
     * Enumeration of all available REST API resources
     */
    enum class Resources {
      Signals,     //!< Signals
      Metadata,    //!< Metadata
      Authorize,   //!< Authorize
      Count
    };

  private:
    bool GetSignalPath(std::string requestId,
                       jsoncons::json& json,
                       std::string& restTarget);
    /**
     * @brief Verify that HTTP target begins with correct root path and remove it if found
     * @param restTarget HTTP target path
     * @param path Path to verify if \p restTarget begins with
     * @return true if root path found with updated \p restTarget string, false if not found
     */
    bool verifyPathAndStrip(std::string& restTarget, std::string& path);

    bool verifySignalResource(std::string& restTarget, jsoncons::json& res);

  public:
    RestV1ApiHandler(std::shared_ptr<ILogger> loggerUtil, std::string& docRoot);
    ~RestV1ApiHandler();

    // IRest2JsonConverter

    bool GetJson(std::string&& restMethod,
                 std::string&& restTarget,
                 std::string& resultJson);
};

#endif
