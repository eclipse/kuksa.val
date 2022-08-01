/**********************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 **********************************************************************/

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
  bool pathIsReadable(const VSSPath &path) override;
  bool pathIsAttributable(const VSSPath &path, const std::string &attr) override;

  string getDatatypeForPath(const VSSPath &path) override;

  std::list<VSSPath> getLeafPaths(const VSSPath& path) override;

  void checkAndSanitizeType(jsoncons::json &meta, jsoncons::json &val) override;


  void initJsonTree(const boost::filesystem::path &fileName) override;
  
  bool checkPathValid(const VSSPath& path);
  static bool isActor(const jsoncons::json &element);
  static bool isSensor(const jsoncons::json &element);
  static bool isAttribute(const jsoncons::json &element);


  void updateJsonTree(jsoncons::json& sourceTree, const jsoncons::json& jsonTree);
  void updateJsonTree(KuksaChannel& channel, jsoncons::json& value) override;
  void updateMetaData(KuksaChannel& channel, const VSSPath& path, const jsoncons::json& newTree) override;
  jsoncons::json getMetaData(const VSSPath& path) override;
  
  jsoncons::json setSignal(const VSSPath &path, const std::string& attr, jsoncons::json &value) override; //gen2 version
  jsoncons::json getSignal(const VSSPath &path, const std::string& attr, bool as_string=false) override; //Gen2 version

  void applyDefaultValues(jsoncons::json &tree, VSSPath currentPath);

  private:

    void checkArrayType(std::string& subdatatype, jsoncons::json &val);

};
#endif
