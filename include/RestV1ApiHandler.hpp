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
    bool GetSignalPath(uint32_t requestId,
                       jsoncons::json& json,
                       std::string&& restTarget);
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
