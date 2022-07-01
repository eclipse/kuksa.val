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

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <vector>
#include "ILogger.hpp"
#include "IVssDatabase.hpp"

/** Implements the Websocket get request accroding to GEN2, with GEN1 backwards
 * compatibility **/
std::string VssCommandProcessor::processGet2(KuksaChannel &channel,
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

  std::string attribute;
  if (request.contains("attribute")) {
    attribute = request["attribute"].as_string();
  } else {
    attribute = "value";
  }

  logger->Log(LogLevel::VERBOSE, "Get request with id " + requestId +
                                     " for path: " + path.to_string() + " with attribute: " + attribute);

  jsoncons::json answer;
  jsoncons::json datapoints = jsoncons::json::array();
  jsoncons::json current_dp;

  try {
    list<VSSPath> vssPaths = database->getLeafPaths(path);
    for (const auto &vssPath : vssPaths) {
      // check Read access here.
      if (!accessValidator_->checkReadAccess(channel, vssPath)) {
        stringstream msg;
        msg << "Insufficient read access to " << pathStr;
        logger->Log(LogLevel::WARNING, msg.str());
        return JsonResponses::noAccess(requestId, "get", msg.str());
      } else if (! database->pathIsAttributable(path, attribute)) {
        stringstream msg;
        msg << "Can not get " << path.to_string() << " with attribute " << attribute << ".";
        logger->Log(LogLevel::WARNING,msg.str());
        return JsonResponses::noAccess(request["requestId"].as<string>(), "set", msg.str());
      } else {
        bool as_string = channel.getType() != KuksaChannel::Type::GRPC;
        current_dp = database->getSignal(vssPath, attribute, as_string);
        JsonResponses::convertJSONTimeStampToISO8601(current_dp["dp"]);
        datapoints.push_back(current_dp);
      }
    }
    if (vssPaths.size() < 1) {
      return JsonResponses::pathNotFound(requestId, "get", pathStr);
    }
    if (vssPaths.size() == 1) {
      answer["data"] = datapoints[0];
    } else {
      answer["data"] = datapoints;
    }
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
    requestId, "get", string("Unhandled error: ") + e.what());
  }

  answer["action"] = "get";
  answer["requestId"] = requestId;
  answer["ts"] = JsonResponses::getTimeStamp();
  stringstream ss;
  ss << pretty_print(answer);
  return ss.str();
}
