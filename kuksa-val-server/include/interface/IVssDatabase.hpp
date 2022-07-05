/*
 * ******************************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
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
