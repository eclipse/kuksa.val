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
#ifndef __VSSDATABASE_HPP__
#define __VSSDATABASE_HPP__

#include <string>
#include <list>
#include <mutex>
#include <memory>

#include <jsoncons/json.hpp>

#include "IVssDatabase.hpp"

class WsChannel;
class IAccessChecker;
class ISubscriptionHandler;
class ILogger;

class VssDatabase : public IVssDatabase {
#ifdef UNIT_TEST
  friend class w3cunittest;
#endif

 private:
  std::shared_ptr<ILogger> logger;
  std::mutex rwMutex;

  std::shared_ptr<ISubscriptionHandler> subHandler;
  std::shared_ptr<IAccessChecker> accessValidator;
  std::string getPathForMetadata(std::string path, bool& isBranch);
  std::string getReadablePath(std::string jsonpath);
  void checkSetPermission(WsChannel& channel, jsoncons::json valueJson);
  void HandleSet(jsoncons::json & setValues);

 public:
  VssDatabase(std::shared_ptr<ILogger> loggerUtil,
              std::shared_ptr<ISubscriptionHandler> subHandle,
              std::shared_ptr<IAccessChecker> accValidator);
  ~VssDatabase();

  void initJsonTree(const std::string &fileName);
  jsoncons::json getMetaData(const std::string &path);
  void setSignal(WsChannel& channel, const std::string &path, jsoncons::json value);
  void setSignal(const std::string &path, jsoncons::json value);
  jsoncons::json getSignal(WsChannel& channel, const std::string &path);

  std::list<std::string> getPathForGet(const std::string &path, bool& isBranch);
  std::string getVSSSpecificPath(const std::string &path, bool& isBranch,
                                 jsoncons::json& tree);
  jsoncons::json getPathForSet(const std::string &path, jsoncons::json value);
};
#endif
