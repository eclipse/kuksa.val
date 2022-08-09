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
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "ISubscriptionHandler.hpp"
#include "JsonResponses.hpp"
#include "VSSRequestValidator.hpp"
#include "VssCommandProcessor.hpp"
#include "exception.hpp"

jsoncons::json VssCommandProcessor::processUnsubscribe(KuksaChannel &channel,
                                               jsoncons::json &request) {
  try {
    requestValidator->validateUnsubscribe(request);
  } catch (jsoncons::jsonschema::schema_error &e) {
    std::string msg = std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, msg);
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request), "unsubscribe",
        string("Schema error: ") + msg);
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request), "unsubscribe",
        string("Unhandled error: ") + e.what());
  }

  string request_id = request["requestId"].as<string>();
  boost::uuids::uuid subscribeID;
  try {
    subscribeID = boost::uuids::string_generator()(request["subscriptionId"].as<std::string>());
  } catch (std::runtime_error &e) {
    logger->Log(LogLevel::ERROR, "Subscription ID is not a valid UUID");
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request), "unsubscribe",
        string("Subscription ID is not a UUID: ") + e.what());
  }
  logger->Log(
      LogLevel::VERBOSE,
      "VssCommandProcessor::processQuery: unsubscribe query  for sub ID = " +
          boost::uuids::to_string(subscribeID) + " with request id " + request_id);

  int res = subHandler->unsubscribe(subscribeID);
  if (res == 0) {
    jsoncons::json answer;
    answer["action"] = "unsubscribe";
    answer["requestId"] = request_id;
    answer["subscriptionId"] = boost::uuids::to_string(subscribeID);
    answer["ts"] = JsonResponses::getTimeStamp();

    return answer;

  } else {
    jsoncons::json root;
    jsoncons::json error;

    root["action"] = "unsubscribe";
    root["requestId"] = request_id;
    error["number"] = "400";
    error["reason"] = "Unknown error";
    error["message"] = "Error while unsubscribing";

    root["error"] = error;
    root["ts"] = JsonResponses::getTimeStamp();
    return root;
  }
}
