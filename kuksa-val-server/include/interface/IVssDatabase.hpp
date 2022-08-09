/**********************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
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
 **********************************************************************/

#ifndef __IVSSDATABASE_HPP__
#define __IVSSDATABASE_HPP__

#include <string>

#include <jsoncons/json.hpp>
#include <boost/filesystem.hpp>

#include "KuksaChannel.hpp"
#include "VSSPath.hpp"

class IVssDatabase {
  public:
    virtual ~IVssDatabase() {}

    virtual void initJsonTree(const boost::filesystem::path &fileName) = 0;
    virtual void updateJsonTree(KuksaChannel& channel, jsoncons::json& value) = 0;
    virtual void updateMetaData(KuksaChannel& channel, const VSSPath& path, const jsoncons::json& value) = 0;
    virtual jsoncons::json getMetaData(const VSSPath &path) = 0;
  
    virtual jsoncons::json setSignal(const VSSPath &path, const std::string& attr, jsoncons::json &value) = 0; //gen2 version
    virtual jsoncons::json getSignal(const VSSPath& path, const std::string& attr, bool as_string=false) = 0;

    virtual bool pathExists(const VSSPath &path) = 0;
    virtual bool pathIsWritable(const VSSPath &path) = 0;
    virtual bool pathIsReadable(const VSSPath &path) = 0;
    virtual bool pathIsAttributable(const VSSPath &path, const std::string &attr) = 0;

    virtual string getDatatypeForPath(const VSSPath &path) = 0;

    virtual std::list<VSSPath> getLeafPaths(const VSSPath& path) = 0;

    virtual void checkAndSanitizeType(jsoncons::json &meta, jsoncons::json &val) = 0;
                           
    // TODO: temporary added while components are refactored
    jsoncons::json data_tree__;
    jsoncons::json meta_tree__;
};

#endif
