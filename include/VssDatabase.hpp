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
#include "VSSPath.hpp"

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

 public:
  VssDatabase(std::shared_ptr<ILogger> loggerUtil,
              std::shared_ptr<ISubscriptionHandler> subHandle);
  ~VssDatabase();

  //helpers
  bool pathExists(const VSSPath &path) override;
  bool pathIsWritable(const VSSPath &path) override;
  std::list<VSSPath> getLeafPaths(const VSSPath& path) override;

  void checkAndSanitizeType(jsoncons::json &meta, jsoncons::json &val) override;


  void initJsonTree(const boost::filesystem::path &fileName) override;
  
  bool checkPathValid(const VSSPath& path);
  static bool isActor(const jsoncons::json &element);
  static bool isSensor(const jsoncons::json &element);
  static bool isAttribute(const jsoncons::json &element);


  void updateJsonTree(jsoncons::json& sourceTree, const jsoncons::json& jsonTree);
  void updateJsonTree(kuksa::kuksaChannel& channel, jsoncons::json& value) override;
  void updateMetaData(kuksa::kuksaChannel& channel, const VSSPath& path, const jsoncons::json& newTree) override;
  jsoncons::json getMetaData(const VSSPath& path) override;
  
  jsoncons::json setSignal(const VSSPath &path, jsoncons::json &value) override; //gen2 version
  jsoncons::json getSignal(const VSSPath &path) override; //Gen2 version

  void applyDefaultValues(jsoncons::json &tree, VSSPath currentPath);

  private:

    void checkArrayType(std::string datatype, jsoncons::json &val );

};
#endif
