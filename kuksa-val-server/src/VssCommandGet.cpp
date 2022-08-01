/**********************************************************************
 * Copyright (c) 2020 Robert Bosch GmbH.
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
 *      Robert Bosch GmbH
 **********************************************************************/


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
jsoncons::json VssCommandProcessor::processGet(KuksaChannel &channel,
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
  } catch (notSetException &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    return JsonResponses::notSetResponse(requestId, e.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
    requestId, "get", string("Unhandled error: ") + e.what());
  }

  answer["action"] = "get";
  answer["requestId"] = requestId;
  answer["ts"] = JsonResponses::getTimeStamp();
  return answer;

}
