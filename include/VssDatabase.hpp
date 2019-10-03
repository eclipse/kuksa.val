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

class SubscriptionHandler;
class AccessChecker;
class WsChannel;
class ILogger;

class VssDatabase {
  friend class SubscriptionHandler;
  friend class Authenticator;
#ifdef UNIT_TEST
  friend class w3cunittest;
#endif

 private:
  std::shared_ptr<ILogger> logger;
  std::mutex rwMutex;
  jsoncons::json data_tree;
  jsoncons::json meta_tree;
  SubscriptionHandler* subHandler;
  AccessChecker* accessValidator;
  std::string getVSSSpecificPath(std::string path, bool& isBranch, jsoncons::json& tree);
  std::string getPathForMetadata(std::string path, bool& isBranch);
  std::list<std::string> getPathForGet(std::string path, bool& isBranch);
  jsoncons::json getPathForSet(std::string path, jsoncons::json value);
  std::string getReadablePath(std::string jsonpath);
  void checkSetPermission(WsChannel& channel, jsoncons::json valueJson);

 public:
  VssDatabase(std::shared_ptr<ILogger> loggerUtil,
              SubscriptionHandler* subHandle,
              AccessChecker* accValidator);
  ~VssDatabase();
  void initJsonTree(std::string fileName);
  jsoncons::json getMetaData(std::string path);
  void setSignal(WsChannel& channel, std::string path, jsoncons::json value);
  void setSignal(std::string path, jsoncons::json value);
  jsoncons::json getSignal(WsChannel& channel, std::string path);

};
#endif
