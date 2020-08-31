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

class IVssDatabase;
class ISubscriptionHandler;
class IAuthenticator;
class IAccessChecker;
class ILogger;
class WsChannel;


class VssCommandProcessor : public IVssCommandProcessor {
 private:
  std::shared_ptr<ILogger> logger;
  std::shared_ptr<IVssDatabase> database;
  std::shared_ptr<ISubscriptionHandler> subHandler;
  std::shared_ptr<IAuthenticator> tokenValidator;
  std::shared_ptr<IAccessChecker> accessValidator;
#ifdef JSON_SIGNING_ON
  std::shared_ptr<SigningHandler> signer;
#endif

  std::string processGet(WsChannel& channel, const std::string& request_id, const std::string& path);
  std::string processSet(WsChannel& channel, const std::string& request_id, const std::string& path,
                         jsoncons::json value);
  std::string processSubscribe(WsChannel& channel, const std::string& request_id, const std::string& path);
  std::string processUnsubscribe(const std::string & request_id, uint32_t subscribeID);
  std::string processGetMetaData(const std::string & request_id, const std::string & path);
  std::string processUpdateMetaData(WsChannel& channel, const std::string& request_id, const std::string& path, const jsoncons::json& metadata);
  std::string processAuthorize(WsChannel& channel, const std::string & request_id,
                          const std::string & token);
  std::string processAuthorizeWithPermManager(WsChannel &channel, const std::string & request_id,
                                 const std::string & client, const std::string& clientSecret);
  

 public:
  VssCommandProcessor(std::shared_ptr<ILogger> loggerUtil,
                      std::shared_ptr<IVssDatabase> database,
                      std::shared_ptr<IAuthenticator> vdator,
                      std::shared_ptr<ISubscriptionHandler> subhandler);
  ~VssCommandProcessor();

  std::string processQuery(const std::string &req_json, WsChannel& channel);
};

#endif
