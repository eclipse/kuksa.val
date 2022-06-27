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

#include<tuple> 

#include "JsonResponses.hpp"
#include "VSSPath.hpp"
#include "VSSRequestValidator.hpp"
#include "VssCommandProcessor.hpp"
#include "exception.hpp"

#include "ILogger.hpp"
#include "IVssDatabase.hpp"
#include "IAccessChecker.hpp"

#include <boost/algorithm/string.hpp>


/** Implements the Websocket set request according to GEN2, with GEN1 backwards
 * compatibility **/
std::string VssCommandProcessor::processSet2(KuksaChannel &channel,
                                             jsoncons::json &request) {
  try {
    requestValidator->validateSet(request);
  } catch (jsoncons::jsonschema::schema_error &e) {
    std::string msg=std::string(e.what());
    boost::algorithm::trim(msg);
    logger->Log(LogLevel::ERROR, msg);
    return JsonResponses::malFormedRequest( requestValidator->tryExtractRequestId(request), "set",
                                           string("Schema error: ") + msg);
  } catch (std::exception &e) {
    logger->Log(LogLevel::ERROR, "Unhandled error: " + string(e.what()));
    return JsonResponses::malFormedRequest(
        requestValidator->tryExtractRequestId(request) , "set", string("Unhandled error: ") + e.what());
  }

  VSSPath path = VSSPath::fromVSS(request["path"].as_string());
  
  string requestId = request["requestId"].as_string();

  std::string attribute;
  if (request.contains("attribute")) {
    attribute = request["attribute"].as_string();
  } else {
    attribute = "value";
  }
  logger->Log(LogLevel::VERBOSE, "Set request with id " + requestId +
                                     " for path: " + path.to_string() + " with attribute: " + attribute);

  //unpack any multiset or filters here later as in
  //list of setPairs=expand(VSSPath, filters)

  std::vector<std::tuple<VSSPath,jsoncons::json>> setPairs;
  setPairs.push_back(std::make_tuple(path, (jsoncons::json&)request[attribute]));

  //Check Access rights  & types first. Will only proceed to set, if all paths in set are valid
  //(set all or none)
  for ( std::tuple<VSSPath,jsoncons::json> setTuple : setPairs) {
    if (! database->pathExists(std::get<0>(setTuple) )) {
      stringstream msg;
      logger->Log(LogLevel::WARNING,msg.str());
      return JsonResponses::pathNotFound(request["requestId"].as<string>(), "set", std::get<0>(setTuple).to_string());
    }
    if (! accessValidator_->checkWriteAccess(channel, std::get<0>(setTuple) )) {
      stringstream msg;
      msg << "No write access to " << std::get<0>(setTuple).to_string();
      logger->Log(LogLevel::WARNING,msg.str());
      return JsonResponses::noAccess(request["requestId"].as<string>(), "set", msg.str());
    }
    if (! database->pathIsWritable(std::get<0>(setTuple))) {
      stringstream msg;
      msg << "Can not set " << std::get<0>(setTuple).to_string() << ". Only sensor or actor leaves can be set.";
      logger->Log(LogLevel::WARNING,msg.str());
      return JsonResponses::noAccess(request["requestId"].as<string>(), "set", msg.str());
    }
    if (! database->pathIsAttributable(std::get<0>(setTuple), attribute)) {
      stringstream msg;
      msg << "Can not set path:" << std::get<0>(setTuple).to_string() << " with attribute:" << attribute << ".";
      logger->Log(LogLevel::WARNING,msg.str());
      return JsonResponses::noAccess(request["requestId"].as<string>(), "set", msg.str());
    }
  }

  //If all preliminary checks successful, we are setting everything
  try {
    for ( std::tuple<VSSPath,jsoncons::json> setTuple : setPairs) {
      database->setSignal(std::get<0>(setTuple), attribute, std::get<1>(setTuple));
    }
  } catch (genException &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    jsoncons::json root;
    jsoncons::json error;

    root["action"] = "set";
    root.insert_or_assign("requestId", request["requestId"]);

    error["number"] = "401";
    error["reason"] = "Unknown error";
    error["message"] = e.what();

    root["error"] = error;
    root["ts"] = JsonResponses::getTimeStamp();

    std::stringstream ss;
    ss << pretty_print(root);
    return ss.str();
  } catch (noPathFoundonTree &e) {
    logger->Log(LogLevel::ERROR, string(e.what()));
    return JsonResponses::pathNotFound(request["requestId"].as<string>(), "set",
                                       path.to_string());
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
        request["requestId"].as<string>(), "set",
        string("Unhandled error: ") + e.what());
  }

  jsoncons::json answer;
  answer["action"] = "set";
  answer.insert_or_assign("requestId", request["requestId"]);
  answer["ts"] = JsonResponses::getTimeStamp();

  std::stringstream ss;
  ss << pretty_print(answer);
  return ss.str();
}
