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
  try {
    requestValidator->validateSet(request);
  } catch (jsoncons::jsonschema::schema_error &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    return JsonResponses::malFormedRequest("UNKNOWN", "set",
                                           string("Schema error: ") + e.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
        "UNKNOWN", "get", string("Unhandled error: ") + e.what());
  }

  VSSPath path = VSSPath::fromVSS(request["path"].as_string());
  bool gen1_compat_mode = path.isGen1Origin();

  string requestId = request["requestId"].as_string();

  logger->Log(LogLevel::VERBOSE, "Set request with id " + requestId +
                                     " for path: " + path.getVSSPath());

  try {
    database->setSignal(channel, path, request["value"], gen1_compat_mode);
  } catch (genException &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    jsoncons::json root;
    jsoncons::json error;

    root["action"] = "set";
    root.insert_or_assign("requestId", request["requestId"]);

    error["number"] = 401;
    error["reason"] = "Unknown error";
    error["message"] = e.what();

    root["error"] = error;
    root["timestamp"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(root);
    return ss.str();
  } catch (noPathFoundonTree &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    return JsonResponses::pathNotFound(request["requestId"].as<string>(), "set",
                                       path.getVSSPath());
  } catch (outOfBoundException &outofboundExp) {
    logger->Log(LogLevel::ERROR, string(outofboundExp.what()));
    return JsonResponses::valueOutOfBounds(request["requestId"].as<string>(),
                                           "set", outofboundExp.what());
  } catch (noPermissionException &nopermission) {
    logger->Log(LogLevel::ERROR, string(nopermission.what()));
    return JsonResponses::noAccess(request["requestId"].as<string>(), "set",
                                   nopermission.what());
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
        request["requestId"].as<string>(), "get",
        string("Unhandled error: ") + e.what());
  }

  jsoncons::json answer;
  answer["action"] = "set";
  answer.insert_or_assign("requestId", request["requestId"]);
  answer["timestamp"] = JsonResponses::getTimeStamp();

  std::stringstream ss;
  ss << pretty_print(answer);
  return ss.str();
}
