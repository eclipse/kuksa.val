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
#include "VSSRequestValidator.hpp"
#include "VssCommandProcessor.hpp"
#include "VSSPath.hpp"
#include "exception.hpp"

#include "ILogger.hpp"
#include "IVssDatabase.hpp"

#include <boost/algorithm/string.hpp>


/** Implements the Websocket get request accroding to GEN2, with GEN1 backwards
 * compatibility **/
std::string VssCommandProcessor::processGet2(WsChannel &channel,
                                             jsoncons::json &request) {
  std::string pathStr= request["path"].as_string();
  VSSPath path = VSSPath::fromVSS(pathStr);

  try {
    requestValidator->validateGet(request);
  } catch (jsoncons::jsonschema::schema_error &e) {
    std::string msg = std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, msg);
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request), "get",
        string("Schema error: ") + msg);
  } catch (std::exception &e) {
    std::string msg = std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, "Unhandled error: " + msg);
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request), "get",
        string("Unhandled error: ") + e.what());
  }

  string requestId = request["requestId"].as_string();

  logger->Log(LogLevel::VERBOSE, "Get request with id " + requestId +
                                     " for path: " + path.to_string());

  //-------------------------------------
  jsoncons::json res;
  try {
    res = database->getSignal(channel, path);
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::ERROR, string(nopermission.what()));
    return JsonResponses::noAccess(requestId, "get", nopermission.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
        requestId, "get", string("Unhandled error: ") + e.what());
  }

  if (!res.contains("value")) {
    return JsonResponses::pathNotFound(requestId, "get", path.to_string());
  } else {
    res["action"] = "get";
    res["requestId"] = requestId;
    res["timestamp"] = JsonResponses::getTimeStamp();
    stringstream ss;
    ss << pretty_print(res);
    return ss.str();
  }
}
