/*
 * ******************************************************************************
 * Copyright (c) 2022 Robert Bosch GmbH.
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

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <vector>
#include "ILogger.hpp"
#include "IVssDatabase.hpp"

/** Implements the Websocket getTargetValue **/
std::string VssCommandProcessor::processGetTarget(kuksa::kuksaChannel &channel,
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
        requestValidator->tryExtractRequestId(request), "getTargetValue",
        string("Schema error: ") + msg);
  } catch (std::exception &e) {
    std::string msg = std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, "Unhandled error: " + msg);
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request), "getTargetValue",
        string("Unhandled error: ") + e.what());
  }

  string requestId = request["requestId"].as_string();

  logger->Log(LogLevel::VERBOSE, "getTargetValue request with id " + requestId +
                                     " for path: " + path.to_string());

  jsoncons::json answer;
  jsoncons::json datapoints = jsoncons::json::array();

  try {
    list<VSSPath> vssPaths = database->getLeafPaths(path);
    for (const auto &vssPath : vssPaths) {
      // check Read access here.
      if (!accessValidator_->checkReadAccess(channel, vssPath)) {
        stringstream msg;
        msg << "Insufficient read access to " << pathStr;
        logger->Log(LogLevel::WARNING, msg.str());
        return JsonResponses::noAccess(requestId, "getTargetValue", msg.str());
      } else if (! database->pathIsTargetable(vssPath)) {
        stringstream msg;
        msg << "Can not get target value for " << pathStr << ". Only actor leaves have target values.";
        logger->Log(LogLevel::WARNING,msg.str());
        return JsonResponses::noAccess(request["requestId"].as<string>(), "getTargetValue", msg.str());
      } else {
        datapoints.push_back(database->getSignalTarget(vssPath));
      }
    }
    if (vssPaths.size() < 1) {
      return JsonResponses::pathNotFound(requestId, "getTargetValue", pathStr);
    }
    if (vssPaths.size() == 1) {
      answer["data"] = datapoints[0];
    } else {
      answer["data"] = datapoints;
    }
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
    requestId, "getTargetValue", string("Unhandled error: ") + e.what());
  }

  answer["action"] = "getTargetValue";
  answer["requestId"] = requestId;
  answer["ts"] = JsonResponses::getTimeStamp();
  stringstream ss;
  ss << pretty_print(answer);
  return ss.str();
}
