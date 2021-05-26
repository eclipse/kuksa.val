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
#include "WsChannel.hpp"
#include "IAccessChecker.hpp"

class IVssDatabase;
class ISubscriptionHandler;
class IAuthenticator;
class ILogger;
class WsChannel;


class VssCommandProcessor : public IVssCommandProcessor {
 private:
  std::shared_ptr<ILogger> logger;
  std::shared_ptr<IVssDatabase> database;
  std::shared_ptr<ISubscriptionHandler> subHandler;
  std::shared_ptr<IAuthenticator> tokenValidator;
  std::shared_ptr<IAccessChecker> accessValidator_;
  VSSRequestValidator *requestValidator;
#ifdef JSON_SIGNING_ON
  std::shared_ptr<SigningHandler> signer;
#endif

  std::string processSubscribe(WsChannel& channel, const std::string& request_id, const std::string& path);
  std::string processUnsubscribe(const std::string & request_id, uint32_t subscribeID);
  std::string processUpdateVSSTree(WsChannel& channel, const std::string& request_id,  jsoncons::json& metadata);
  std::string processUpdateMetaData(WsChannel& channel, jsoncons::json& request);
  std::string processGetMetaData(jsoncons::json &request);
  std::string processAuthorize(WsChannel& channel, const std::string & request_id,
                          const std::string & token);
  std::string processAuthorizeWithPermManager(WsChannel &channel, const std::string & request_id,
                                 const std::string & client, const std::string& clientSecret);

  std::string getPathFromRequest(const jsoncons::json &req, bool *gen1_compat);
  std::string processGet2(WsChannel &channel, jsoncons::json &request);
  std::string processSet2(WsChannel &channel, jsoncons::json &request);

 public:
  VssCommandProcessor(std::shared_ptr<ILogger> loggerUtil,
                      std::shared_ptr<IVssDatabase> database,
                      std::shared_ptr<IAuthenticator> vdator,
                      std::shared_ptr<IAccessChecker> accC,
                      std::shared_ptr<ISubscriptionHandler> subhandler);
  ~VssCommandProcessor();

  std::string processQuery(const std::string &req_json, WsChannel& channel);
};

#endif
