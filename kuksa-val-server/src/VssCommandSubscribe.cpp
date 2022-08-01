/**********************************************************************
 * Copyright (c) 2022 Robert Bosch GmbH.
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


#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp> 
#include <boost/uuid/uuid_io.hpp>  
#include "ISubscriptionHandler.hpp"
#include "JsonResponses.hpp"
#include "VSSRequestValidator.hpp"
#include "VssCommandProcessor.hpp"
#include "exception.hpp"

jsoncons::json VssCommandProcessor::processSubscribe(KuksaChannel &channel,
                                             jsoncons::json &request) {
  try {
    requestValidator->validateSubscribe(request);
  } catch (jsoncons::jsonschema::schema_error &e) {
    std::string msg = std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, msg);
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request), "subscribe",
        string("Schema error: ") + msg);
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request), "get",
        string("Unhandled error: ") + e.what());
  }

  string path = request["path"].as<string>();
  string request_id = request["requestId"].as<string>();
  std::string attribute;
  if (request.contains("attribute")) {
    attribute = request["attribute"].as_string();
  } else {
    attribute = "value";
  }


  logger->Log(
      LogLevel::VERBOSE,
      string(
          "VssCommandProcessor::processSubscribe: Client wants to subscribe ") +
          path);

  boost::uuids::uuid subId;;
  try {
    subId = subHandler->subscribe(channel, database, path, attribute);
  } catch (noPathFoundonTree &noPathFound) {
    logger->Log(LogLevel::ERROR, string(noPathFound.what()));
    return JsonResponses::pathNotFound(request_id, "subscribe", path);
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::ERROR, string(nopermission.what()));
    return JsonResponses::noAccess(request_id, "subscribe",
                                   nopermission.what());
  } catch (genException &outofboundExp) {
    logger->Log(LogLevel::ERROR, string(outofboundExp.what()));
    return JsonResponses::valueOutOfBounds(request_id, "subscribe",
                                           outofboundExp.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
        request_id, "get", string("Unhandled error: ") + e.what());
  }

  //If no exceptions, we have a vaöid subscription ID
  
  jsoncons::json answer;
  answer["action"] = "subscribe";
  answer["requestId"] = request_id;
  answer["subscriptionId"] = boost::uuids::to_string(subId);
  answer["ts"] = JsonResponses::getTimeStamp();
  return answer;
}
