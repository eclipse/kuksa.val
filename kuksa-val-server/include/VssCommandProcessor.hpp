/*
 * ******************************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
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
#ifndef __VSSCOMMANDPROCESSOR_H__
#define __VSSCOMMANDPROCESSOR_H__

#include <string>
#include <memory>

#include <jsoncons/json.hpp>

#include "IVssCommandProcessor.hpp"
#include "VSSRequestValidator.hpp"
#include "KuksaChannel.hpp"
#include "IAccessChecker.hpp"

class IVssDatabase;
class ISubscriptionHandler;
class IAuthenticator;
class ILogger;


class VssCommandProcessor : public IVssCommandProcessor {
 private:
  std::shared_ptr<ILogger> logger;
  std::shared_ptr<IVssDatabase> database;
  std::shared_ptr<ISubscriptionHandler> subHandler;
  std::shared_ptr<IAuthenticator> tokenValidator;
  std::shared_ptr<IAccessChecker> accessValidator_;
  VSSRequestValidator *requestValidator;

  std::string processUpdateMetaData(KuksaChannel& channel, jsoncons::json& request);
  std::string processAuthorizeWithPermManager(KuksaChannel &channel, const std::string & request_id,
                                 const std::string & client, const std::string& clientSecret);

  std::string getPathFromRequest(const jsoncons::json &req, bool *gen1_compat);
  std::string processUpdateVSSTree(KuksaChannel& channel, jsoncons::json &request);

 public:
  std::string processGetMetaData(jsoncons::json &request);
  std::string processAuthorize(KuksaChannel& channel, const std::string & request_id,
                          const std::string & token);
  std::string processGet2(KuksaChannel &channel, jsoncons::json &request);
  std::string processSet2(KuksaChannel &channel, jsoncons::json &request);
  std::string processSubscribe(KuksaChannel& channel, jsoncons::json &request);
  std::string processUnsubscribe(KuksaChannel &channel, jsoncons::json &request);
  
  VssCommandProcessor(std::shared_ptr<ILogger> loggerUtil,
                      std::shared_ptr<IVssDatabase> database,
                      std::shared_ptr<IAuthenticator> vdator,
                      std::shared_ptr<IAccessChecker> accC,
                      std::shared_ptr<ISubscriptionHandler> subhandler);
  ~VssCommandProcessor();

  std::string processQuery(const std::string &req_json, KuksaChannel& channel);
};

#endif
