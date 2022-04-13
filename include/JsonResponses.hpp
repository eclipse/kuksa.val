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

#ifndef __DEFAULTJSONRESPONSES___
#define __DEFAULTJSONRESPONSES___

#include <string>
#include <jsoncons/json.hpp>

namespace JsonResponses {
  std::string malFormedRequest(std::string request_id,
                               const std::string action,
                               std::string message);

  void malFormedRequest(std::string request_id,
                        const std::string action,
                        std::string message,
                        jsoncons::json& jsonResponse);

  std::string malFormedRequest(std::string message, std::string requestId="UNKNOWN");
  
  void malFormedRequest(std::string message,
                        jsoncons::json& jsonResponse, std::string requesId="UNKNOWN");

  /** A API call requested a non-existant path */
  std::string pathNotFound(std::string request_id,
                           const std::string action,
                           const std::string path);

  void pathNotFound(std::string request_id,
                    const std::string action,
                    const std::string path,
                    jsoncons::json& jsonResponse);

  std::string noAccess(std::string request_id,
                       const std::string action,
                       std::string message);

  void noAccess(std::string request_id,
                const std::string action,
                std::string message,
                jsoncons::json& jsonResponse);

  std::string valueOutOfBounds(std::string request_id,
                               const std::string action,
                               const std::string message);

  void valueOutOfBounds(std::string request_id,
                        const std::string action,
                        const std::string message,
                        jsoncons::json& jsonResponse);

  std::string getTimeStamp();

  std::string getTimeStampZero();
}

#endif
