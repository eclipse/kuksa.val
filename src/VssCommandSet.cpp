/*
 * ******************************************************************************
 * Copyright (c) 2020 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH
 * *****************************************************************************
 */

#include "JsonResponses.hpp"
#include "VSSPath.hpp"
#include "VSSRequestValidator.hpp"
#include "VssCommandProcessor.hpp"
#include "exception.hpp"

#include "ILogger.hpp"
#include "IVssDatabase.hpp"

/** Implements the Websocket get request according to GEN2, with GEN1 backwards
 * compatibility **/
std::string VssCommandProcessor::processSet2(WsChannel &channel,
                                             jsoncons::json &request) {
  requestValidator->validateSet(request);
  VSSPath path=VSSPath::fromVSS(request["path"].as_string());
  //bool  gen1_compat_mode=path.isGen1Origin();

string requestId = request["requestId"].as_string();
  logger->Log(LogLevel::VERBOSE,
              "Set request with id " + requestId + " for path: " + path.getVSSPath());

  return JsonResponses::noAccess(requestId, "get", "Not implemented");
} 
