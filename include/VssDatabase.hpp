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
  std::shared_ptr<ILogger> logger_;
  std::mutex rwMutex_;

  std::shared_ptr<ISubscriptionHandler> subHandler_;
  std::shared_ptr<IAccessChecker> accessValidator_;
  std::string getPathForMetadata(std::string path, bool& isBranch);
  std::string getReadablePath(std::string jsonpath);
  void checkSetPermission(WsChannel& channel, jsoncons::json valueJson);
  void HandleSet(jsoncons::json & setValues);

 public:
  VssDatabase(std::shared_ptr<ILogger> loggerUtil,
              std::shared_ptr<ISubscriptionHandler> subHandle,
              std::shared_ptr<IAccessChecker> accValidator);
  ~VssDatabase();

  void initJsonTree(const boost::filesystem::path &fileName) override;
  bool checkPathValid(const std::string& path);
  jsoncons::json getMetaData(const std::string &path) override;
  void setSignal(WsChannel& channel, const std::string &path, jsoncons::json value) override;
  void setSignal(const std::string &path, jsoncons::json value);
  jsoncons::json getSignal(WsChannel& channel, const std::string &path) override;

  std::list<std::string> getPathForGet(const std::string &path, bool& isBranch) override;
  std::string getVSSSpecificPath(const std::string &path, bool& isBranch,
                                 jsoncons::json& tree) override;
  jsoncons::json getPathForSet(const std::string &path, jsoncons::json value) override;
};
#endif
