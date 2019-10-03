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

class vssdatabase;
class SubscriptionHandler;
class Authenticator;
class AccessChecker;
class wschannel;
class ILogger;

class vsscommandprocessor {
 private:
  std::shared_ptr<ILogger> logger;
  vssdatabase* database = NULL;
  SubscriptionHandler* subHandler = NULL;
  Authenticator* tokenValidator = NULL;
  AccessChecker* accessValidator = NULL;
#ifdef JSON_SIGNING_ON
  signing* signer = NULL;
#endif

  std::string processGet(wschannel& channel, uint32_t request_id, std::string path);
  std::string processSet(wschannel& channel, uint32_t request_id, std::string path,
                         jsoncons::json value);
  std::string processSubscribe(wschannel& channel, uint32_t request_id,
                          std::string path, uint32_t connectionID);
  std::string processUnsubscribe(uint32_t request_id, uint32_t subscribeID);
  std::string processGetMetaData(uint32_t request_id, std::string path);
  std::string processAuthorize(wschannel& channel, uint32_t request_id,
                          std::string token);
  std::string processAuthorizeWithPermManager(wschannel &channel, uint32_t request_id,
                                 std::string client, std::string clientSecret);
  

 public:
  vsscommandprocessor(std::shared_ptr<ILogger> loggerUtil,
                      vssdatabase* database,
                      Authenticator* vdator,
                      SubscriptionHandler* subhandler);
  ~vsscommandprocessor();
  std::string processQuery(std::string req_json, wschannel& channel);
};

#endif
