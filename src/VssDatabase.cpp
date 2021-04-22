/*
 * ******************************************************************************
 * Copyright (c) 2018-2021 Robert Bosch GmbH.
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

#include <limits>
#include <regex>
#include <stdexcept>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include "jsonpath/json_query.hpp"
#include "jsoncons_ext/jsonpatch/jsonpatch.hpp"

#include "exception.hpp"
#include "visconf.hpp"
#include "ILogger.hpp"
#include "IAccessChecker.hpp"
#include "ISubscriptionHandler.hpp"
#include "VssDatabase.hpp"
#include "WsChannel.hpp"

using namespace std;
using namespace jsoncons;
using jsoncons::json;

namespace {
  
  // Utility method for setting values to JSON.
  void setJsonValue(std::shared_ptr<ILogger> logger,
                    jsoncons::json& dest,
                    jsoncons::json& source,
                    string key) {
    if (!source.contains("type")) {
      string msg = "Unknown type for signal found at " + key;
      logger->Log(LogLevel::ERROR, "VssDatabase::setJsonValue : " + msg);

      throw genException(msg);
    }

    if (source["datatype"] == "uint8")
      dest[key] = source["value"].as<uint8_t>();
    else if (source["datatype"] == "int8")
      dest[key] = source["value"].as<int8_t>();
    else if (source["datatype"] == "uint16")
      dest[key] = source["value"].as<uint16_t>();
    else if (source["datatype"] == "int16")
      dest[key] = source["value"].as<int16_t>();
    else if (source["datatype"] == "uint32")
      dest[key] = source["value"].as<uint32_t>();
    else if (source["datatype"] == "int32")
      dest[key] = source["value"].as<int32_t>();
    else if (source["datatype"] == "uint64")
      dest[key] = source["value"].as<uint64_t>();
    else if (source["datatype"] == "int64")
      dest[key] = source["value"].as<int64_t>();
    else if (source["datatype"] == "boolean")
      dest[key] = source["value"].as<bool>();
    else if (source["datatype"] == "float")
      dest[key] = source["value"].as<float>();
    else if (source["datatype"] == "double")
      dest[key] = source["value"].as<double>();
    else if (source["datatype"] == "string")
      dest[key] = source["value"].as<std::string>();
    else {
      logger->Log(LogLevel::WARNING, "VSSDatabase unknown datatype \"" + source["datatype"].as<std::string>() + "\". Falling back to string" );
      dest[key] = source["value"].as<std::string>();
    }
  }
}

// Constructor
VssDatabase::VssDatabase(std::shared_ptr<ILogger> loggerUtil,
                         std::shared_ptr<ISubscriptionHandler> subHandle,
                         std::shared_ptr<IAccessChecker> accValidator) {
  logger_ = loggerUtil;
  subHandler_ = subHandle;
  accessValidator_ = accValidator;
}

VssDatabase::~VssDatabase() {}

// Initializer
void VssDatabase::initJsonTree(const boost::filesystem::path &fileName) {
  try {
    std::ifstream is(fileName.string());
    is >> data_tree__;
    meta_tree__ = data_tree__;

    logger_->Log(LogLevel::VERBOSE, "VssDatabase::VssDatabase : VSS tree initialized using JSON file = "
                + fileName.string());
    is.close();
  } catch (exception const& e) {
    logger_->Log(LogLevel::ERROR,
                "Exception occured while initializing database/ tree structure. Probably the init json file not found!"
                + string(e.what()));
    throw e;
  }
}

//Check if a path exists, doesn't care about the type
bool VssDatabase::pathExists(const VSSPath &path) {
  jsoncons::json res = jsonpath::json_query(data_tree__, path.getJSONPath());
  if (res.size() == 0) {
    return false;
  }
  return true;
  
}

// Check if a path is writable _in principle_, i.e. whether it is an actor or sensor.
// This does _not_ check whether a user is authorized, and it will return false in case
// the VSSPath references multiple destinations
bool VssDatabase::pathIsWritable(const VSSPath &path) {
  jsoncons::json res = jsonpath::json_query(data_tree__, path.getJSONPath(),jsonpath::result_type::value);
  if (res.size() != 1) { //either no match, or multiple matches
    return false;
  }
  if (res[0].contains("type") && ( res[0]["type"].as<string>() == "sensor" || res[0]["type"].as<string>() == "actuator" )) {
    return true; //sensors and actors can be written to
  }
  //else it is either another type (branch), or a broken part (no type at all) of the tree, and thus not writable
  return false;
}


//returns true if the given path contains usable leafs
bool VssDatabase::checkPathValid(const VSSPath & path){
    return !getJSONPaths(path).empty();
}

// Tokenizes path with '.' as separator.
vector<string> getPathTokens(string path) {
  vector<string> tokens;
  char* path_char = (char*)path.c_str();
  char* tok = strtok(path_char, ".");

  while (tok != NULL) {
    tokens.push_back(string(tok));
    tok = strtok(NULL, ".");
  }
  return tokens;
}

vector<string> tokenizePath(string path) {
  vector<string> tokens;
  boost::split(tokens,path, boost::is_any_of("/"));
  return tokens;
}

list<string> VssDatabase::getJSONPaths(const VSSPath &path) {
  list<string> paths;

  //If this is a branch, recures
  jsoncons::json pathRes;
  try {
    pathRes = jsonpath::json_query(data_tree__, path.getJSONPath(), jsonpath::result_type::path);
  }
  catch (jsonpath::jsonpath_error &e) { //no valid path, return empty list
    logger_->Log(LogLevel::VERBOSE, path.getJSONPath() + " is not a a valid path "+e.what());
    return paths;
  }

  string json = pathRes.as_string();
  for (auto jpath : pathRes.array_range()) {
    jsoncons::json resArray = jsonpath::json_query(data_tree__, jpath.as<string>());
    if (resArray.size() == 0) {
      continue;
    }

    //recurse if branch
    if (resArray[0].contains("type") && resArray[0]["type"].as<string>() == "branch") {
      VSSPath recursepath = VSSPath::fromJSON(jpath.as<string>()+"['children'][*]");
      paths.merge(getJSONPaths(recursepath));
      continue;
    }
    else {
      paths.push_back(jpath.as<string>());
    }
  }

  return paths;
}



// Tokenizes the signal path with '[' - ']' as separator for internal
// processing.
// TODO- Regex could be used here??
vector<string> getVSSTokens(string path) {
  vector<string> tokens;
  char* path_char = (char*)path.c_str();
  char* tok = strtok(path_char, "['");
  int count = 1;
  while (tok != NULL) {
    string tokString = string(tok);
    if (count % 2 == 0) {
      tok = strtok(NULL, "']");
      tokens.push_back(tokString);
    } else {
      tok = strtok(NULL, "['");
    }
    count++;
  }

  return tokens;
}


void VssDatabase::updateJsonTree(jsoncons::json& sourceTree, const jsoncons::json& jsonTree){
  std::error_code ec;

  jsoncons::json patches;
  {
    std::lock_guard<std::mutex> lock_guard(rwMutex_);
    patches = jsoncons::jsonpatch::from_diff(sourceTree, jsonTree);
  }
  jsoncons::json patchArray = jsoncons::json::array();
  //std::cout << pretty_print(patches) << std::endl;
  for(auto& patch: patches.array_range()){
    if (patch["op"] != "remove"){
      patchArray.push_back(patch);
       
    }
  }
  {
    std::lock_guard<std::mutex> lock_guard(rwMutex_);
    jsonpatch::apply_patch(sourceTree, patchArray, ec);
  }

  if(ec){
    std::cout << "error " << ec.message() << std::endl;
    throw genException(ec.message());
  }

}

void VssDatabase::updateJsonTree(WsChannel& channel, const jsoncons::json& jsonTree){
  if (! channel.authorizedToModifyTree()) {
     stringstream msg;
     msg << "do not have write access for updating json tree or is invalid";
     throw noPermissionException(msg.str());
  }

  updateJsonTree(meta_tree__, jsonTree);
  updateJsonTree(data_tree__, jsonTree);

}

// update a metadata in tree, which will only do one-level-deep shallow merge/update.
// If deep merge/update are expected, use `updateJsonTree` instead.
void VssDatabase::updateMetaData(WsChannel& channel, const VSSPath& path, const jsoncons::json& metadata){
  if (! channel.authorizedToModifyTree()) {
     stringstream msg;
     msg << "do not have write access for updating MetaData or is invalid";
     throw noPermissionException(msg.str());
  }
  string jPath = path.getJSONPath();
  
  logger_->Log(LogLevel::VERBOSE, "VssDatabase::updateMetaData: VSS specific path =" + jPath);
    
  jsoncons::json resMetaTree, resDataTree, resMetaTreeArray, resDataTreeArray;
    
  {
    std::lock_guard<std::mutex> lock_guard(rwMutex_);
    resMetaTreeArray= jsonpath::json_query(meta_tree__, jPath);
    resDataTreeArray = jsonpath::json_query(data_tree__, jPath);
  }

  if (resMetaTreeArray.is_array() && resMetaTreeArray.size() == 1) {
    resMetaTree = resMetaTreeArray[0];
  }else if(resMetaTreeArray.is_object()){
    resMetaTree = resMetaTreeArray;
  }else{
    std::stringstream msg;
    msg << jPath + " is not a valid path";
    logger_->Log(LogLevel::ERROR, "VssDatabase::updateMetaData " + msg.str());

    throw notValidException(msg.str());
  }
  if (resDataTreeArray.is_array() && resDataTreeArray.size() == 1) {
    resDataTree = resDataTreeArray[0];
  }else if(resDataTreeArray.is_object()){
    resDataTree = resDataTreeArray;
  }else{
    std::stringstream msg;
    msg << jPath + " is not a valid path";
    logger_->Log(LogLevel::ERROR, "VssDatabase::updateMetaData " + msg.str());

    throw notValidException(msg.str());
  }
  // Note: merge metadata may cause overwritting existing data values
  resMetaTree.merge_or_update(metadata);
  resDataTree.merge_or_update(metadata);
  {
    std::lock_guard<std::mutex> lock_guard(rwMutex_);
    jsonpath::json_replace(meta_tree__, jPath, resMetaTree);
    jsonpath::json_replace(data_tree__, jPath, resDataTree);
  }
}

// Returns the response JSON for metadata request.
jsoncons::json VssDatabase::getMetaData(const VSSPath& path) {
  string jPath = path.getJSONPath();
  jsoncons::json pathRes = jsonpath::json_query(meta_tree__, jPath, jsonpath::result_type::path);
  if (pathRes.size() > 0) {
    jPath = pathRes[0].as<string>();
  } else {
    return NULL;
  }
  logger_->Log(LogLevel::VERBOSE, "VssDatabase::getMetaData: VSS specific path =" + jPath);

  vector<string> tokens = getVSSTokens(jPath);
  int tokLength = tokens.size();

  jsoncons::json parentArray[16];
  string parentName[16];

  int parentCount = 0;
  jsoncons::json resJson;
  string format_path = "$";
  for (int i = 0; i < tokLength; i++) {
    format_path = format_path + "." + tokens[i];
    if ((i < tokLength - 1) && (tokens[i] == "children")) {
      continue;
    }
    jsoncons::json resArray;
    {
      std::lock_guard<std::mutex> lock_guard(rwMutex_);
      resArray = jsonpath::json_query(meta_tree__, format_path);
    }

    if (resArray.is_array() && resArray.size() == 1) {
      resJson = resArray[0];
      if (resJson.contains("description")) {
        resJson.insert_or_assign("description",
                                 resJson["description"].as<string>());
      }

      if (resJson.contains("type")) {
        string type = resJson["type"].as<string>();
        resJson.insert_or_assign("type", type);
      }
    } else {
      // handle exception.
      logger_->Log(LogLevel::ERROR, string("VssDatabase::getMetaData : More than 1 Branch/ value found! ")
                  + string("Path requested needs to be more refined"));
      return NULL;
    }

    // last element is the requested signal.
    if ((i == tokLength - 1) && (tokens[i] != "children")) {
      jsoncons::json value;
      value.insert_or_assign(tokens[i], resJson);
      resJson = value;
    }

    parentArray[parentCount] = resJson;
    parentName[parentCount] = tokens[i];
    parentCount++;
  }

  jsoncons::json result = resJson;

  for (int i = parentCount - 2; i >= 0; i--) {
    jsoncons::json element = parentArray[i];
    element.insert_or_assign("children", result);
    jsoncons::json parent;
    parent.insert_or_assign(parentName[i], element);
    result = parent;
  }

  return result;
}


jsoncons::json  VssDatabase::setSignal(WsChannel& channel, const VSSPath &path, jsoncons::json &value, bool gen1_compat) {
  jsoncons::json answer;
  
  answer["path"] = gen1_compat? path.getVSSGen1Path() : path.getVSSPath();

  if (!accessValidator_->checkWriteAccess(channel, path )) {
      stringstream msg;
      msg << "No write  access to " << path.getVSSPath();
      throw noPermissionException(msg.str());
  }

  jsoncons::json res; 
  {
    std::lock_guard<std::mutex> lock_guard(rwMutex_);
    res = jsonpath::json_query(data_tree__, path.getJSONPath());
    if (res.is_array() && res.size() == 1) {
    jsoncons::json resJson = res[0];
      if (resJson.contains("datatype")) {
        string value_type = resJson["datatype"].as<string>();
        checkAndSanitizeType(resJson, value);
        resJson.insert_or_assign("value", value);
        {
          jsonpath::json_replace(data_tree__, path.getJSONPath(), resJson);
          subHandler_->updateByPath(path.getVSSPath(), value);
          subHandler_->updateByUUID(resJson["uuid"].as<string>(),value);
        }
      }
      else {
        throw genException(path.getVSSPath()+ "is invalid for set"); //Todo better error message. (Does not propagate);
      }
    }
  }
  return answer;
}


// Returns response JSON for get request, checking authorization.
jsoncons::json VssDatabase::getSignal(class WsChannel& channel, const VSSPath& path, bool gen1_compat_mode) {

  list<string> jPaths;
  {
    std::lock_guard<std::mutex> lock_guard(rwMutex_);
    jPaths = getJSONPaths(path);
  }
  int pathsFound = jPaths.size();
  if (pathsFound == 0) {
    jsoncons::json answer;
    return answer;
  }

  logger_->Log(LogLevel::VERBOSE, "VssDatabase::getSignal: " + to_string(pathsFound)
              + " signals found under path = \"" + path.getVSSPath() + "\"");
 
   if (pathsFound == 1) {
    string jPath = jPaths.back();
    VSSPath path=VSSPath::fromJSON(jPath);
    // check Read access here.
    if (!accessValidator_->checkReadAccess(channel, path )) {
      stringstream msg;
      msg << "No read access to " << VSSPath::fromJSON(jPath).getVSSPath();
      throw noPermissionException(msg.str());
    }
    jsoncons::json resArray;
    {
      std::lock_guard<std::mutex> lock_guard(rwMutex_);
      resArray = jsonpath::json_query(data_tree__, jPath);
    }
    jsoncons::json answer;
    answer["path"] = gen1_compat_mode? path.getVSSGen1Path() : path.getVSSPath();
    jsoncons::json result = resArray[0];
    if (result.contains("value")) {
      setJsonValue(logger_, answer, result, "value");
      return answer;
    } else {
      answer["value"] = "---";
      return answer;
    }

  } else if (pathsFound > 1) {
    jsoncons::json answer;
    jsoncons::json valueArray = jsoncons::json::array();

    for (int i = 0; i < pathsFound; i++) {
      jsoncons::json value;
      string jPath = jPaths.back();
      VSSPath path = VSSPath::fromJSON(jPath);
      // Check access here.
      if (!accessValidator_->checkReadAccess(channel, path)) {
        // Allow the permitted signals to return. If exception is enable here,
        // then say only "Signal.OBD.RPM" is permitted and get request is made
        // using wildcard like "Signal.OBD.*" then
        // an error would be returned. By disabling the exception the value for
        // Signal.OBD.RPM (only) will be returned.
        /*stringstream msg;
        msg << "No read access to  " << getReadablePath(jPath);
        throw genException (msg.str());*/
        jPaths.pop_back();
        continue;
      }
      jsoncons::json resArray;
      {
        std::lock_guard<std::mutex> lock_guard(rwMutex_);
        resArray = jsonpath::json_query(data_tree__, jPath);
      }
      jPaths.pop_back();
      jsoncons::json result = resArray[0];
      
      string spath = gen1_compat_mode? path.getVSSGen1Path() : path.getVSSPath();
      if (result.contains("value")) {
        setJsonValue(logger_, value, result, spath);
      } else {
        value[spath] = "---";
      }
      valueArray.insert(valueArray.array_range().end(), value);
    }
    answer["value"] = valueArray;
    return answer;
  }
  return NULL;
}
