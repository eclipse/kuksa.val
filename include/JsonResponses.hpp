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
  std::string malFormedRequest(uint32_t request_id,
                               const std::string action,
                               std::string message);
  void malFormedRequest(uint32_t request_id,
                        const std::string action,
                        std::string message,
                        jsoncons::json& jsonResponse);

  std::string malFormedRequest(std::string message);
  void malFormedRequest(std::string message,
                        jsoncons::json& jsonResponse);

  /** A API call requested a non-existant path */
  std::string pathNotFound(uint32_t request_id,
                           const std::string action,
                           const std::string path);
  void pathNotFound(uint32_t request_id,
                    const std::string action,
                    const std::string path,
                    jsoncons::json& jsonResponse);

  std::string noAccess(uint32_t request_id,
                       const std::string action,
                       std::string message);
  void noAccess(uint32_t request_id,
                const std::string action,
                std::string message,
                jsoncons::json& jsonResponse);

  std::string valueOutOfBounds(uint32_t request_id,
                               const std::string action,
                               const std::string message);
  void valueOutOfBounds(uint32_t request_id,
                        const std::string action,
                        const std::string message,
                        jsoncons::json& jsonResponse);
}

#endif
