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
#include <ctime>
#include <boost/algorithm/string.hpp>
#include "jsonpath/json_query.hpp"
#include "jsoncons_ext/jsonpatch/jsonpatch.hpp"

#include "exception.hpp"
#include "visconf.hpp"
#include "ILogger.hpp"
#include "IAccessChecker.hpp"
#include "ISubscriptionHandler.hpp"
#include "VssDatabase.hpp"
#include "KuksaChannel.hpp"
#include "JsonResponses.hpp"

using namespace std;
using namespace jsoncons;
using jsoncons::json;

// Constructor
VssDatabase::VssDatabase(std::shared_ptr<ILogger> loggerUtil,
                         std::shared_ptr<ISubscriptionHandler> subHandle) {
  logger_ = loggerUtil;
  subHandler_ = subHandle;
}

VssDatabase::~VssDatabase() {}

/** Returns true if the json structure is of VSS type sensor */
bool VssDatabase::isActor(const json &element) {
  return element.contains("type") && element["type"]=="actuator";
}

/** Returns true if the json structure is of VSS type sensor */
bool VssDatabase::isSensor(const json &element) {
  return element.contains("type") && element["type"]=="sensor";
}

/** Returns true if the json structure is of VSS type attribute */
bool VssDatabase::isAttribute(const json &element) {
  return element.contains("type") && element["type"]=="attribute";
}


/** Iterates over a given VSS tree and checks attributes specifying the "default"
 *  metadata. If a default is present, it will be used as "value" (and thus returned upon get)
 */ 
void VssDatabase::applyDefaultValues(json &tree, VSSPath path) {
  //logger_->Log(LogLevel::VERBOSE, "Applying default values in "+path.to_string());

  if ( isAttribute(tree)) {
    if (tree.contains("default")) {
      logger_->Log(LogLevel::INFO, "Setting default for "+path.to_string()+" to "+tree["default"].as<string>());
      /** Not using setSignal, as that always operates on the complete tree. However applyDefaultValues shall also
       *  be applied to layers or metadata loaded later at runtime, before joining, so that defaults are not applied
       *  several times.
       *  Always add as string is correct, as VISS standard demands representation of all values, irrespective of 
       *  VSS datatype as string 
       */ 
      tree.insert_or_assign("value",tree["default"].as<string>());
      return;
    }
  }

  //need to iterate?
  if (tree.contains("children")) {
    //logger_->Log(LogLevel::VERBOSE, "Recurse into "+path.to_string());
    for (auto subtree : tree["children"].object_range() ) {
      /** Note: can not recurse on subtree.value(), as that seems to create a copy... */
      applyDefaultValues(tree["children"][subtree.key()], VSSPath::fromVSS(path.getVSSPath()+"/"+subtree.key()));
    }
  }
}

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

  applyDefaultValues(data_tree__["Vehicle"], VSSPath::fromVSS("Vehicle"));
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

// Check if a path is "attributable"  _in principle_, i.e. whether a particular attribute can be set on a particular path.
// This does _not_ check whether a user is authorized, and it will return false in case
// the VSSPath references multiple destinations
bool VssDatabase::pathIsAttributable(const VSSPath &path, const std::string& attr) {
  jsoncons::json res = jsonpath::json_query(data_tree__, path.getJSONPath(),jsonpath::result_type::value);
  if (res.size() < 1) { // either no match,
    return false;
  } else if (res.size() > 1) { // multiple matches - Allow them to enable get using wildcards
    return true;
  }

  if (attr == "targetValue") {
    if (res[0].contains("type") && (res[0]["type"].as<string>() == "actuator" )) {
      return true; //only actors can have target values/setpoints
    }
  } if (attr == "value") {
      return true;
  }
  //else it is either another type (branch/sensor), or a broken part (no type at all) of the tree, and thus not writable
  return false;
}

// Check if a path is readable _in principle_, i.e. whether it is an actor, sensor or attribute.
// This does _not_ check whether a user is authorized, and it will return false in case
// the VSSPath references multiple destinations
bool VssDatabase::pathIsReadable(const VSSPath &path) {
  jsoncons::json res = jsonpath::json_query(data_tree__, path.getJSONPath(),jsonpath::result_type::value);
  if (res.size() != 1) { //either no match, or multiple matches
    return false;
  }
  if ( isSensor(res[0]) || isActor(res[0])|| isAttribute(res[0]) ) {
    return true; //sensors, actors and attributes can be read
  }
  //else it is either another type (branch), or a broken part (no type at all) of the tree, and thus not writable
  return false;
}


//returns true if the given path contains usable leafs
bool VssDatabase::checkPathValid(const VSSPath & path){
    return !getLeafPaths(path).empty();
}

//return the VSS datatype of a path. If the path is not found, throw
//an exception
string VssDatabase::getDatatypeForPath(const VSSPath &path) {
  if ( !pathExists(path)) {
     throw noPathFoundonTree(path.to_string());
  }
  if ( !pathIsReadable(path)) { //"readable" implies sensor, actuator or attribute, those have datatypes
    stringstream ss;
    ss << path.to_string() + " does not contain a datatype because it is not a sensor/actuator/attribute.";
    throw genException(ss.str());
  }
  jsoncons::json resArray;
  {
    std::lock_guard<std::mutex> lock_guard(rwMutex_);
    resArray = jsonpath::json_query(data_tree__, path.getJSONPath());
  }
  jsoncons::json datapoint;
  jsoncons::json result = resArray[0];
  if (result.contains("datatype")) {
    return result["datatype"].as<string>();
  }
  else {
    stringstream ss;
    ss << path.to_string() + " does not contain a datatype.";
    throw genException(ss.str());
  }
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

// Return a list of path of all leaf nodes, which are the children of the given path
// If the given path is already a leaf node, the to returned list contains only the given nodes 
list<VSSPath> VssDatabase::getLeafPaths(const VSSPath &path) {
  list<VSSPath> paths;
  bool path_is_gen1 = path.isGen1Origin();

  //If this is a branch, recures
  jsoncons::json pathRes;
  try {
    std::lock_guard<std::mutex> lock_guard(rwMutex_);
    pathRes = jsonpath::json_query(data_tree__, path.getJSONPath(), jsonpath::result_type::path);
  }
  catch (jsonpath::jsonpath_error &e) { //no valid path, return empty list
    logger_->Log(LogLevel::VERBOSE, path.getJSONPath() + " is not a a valid path "+e.what());
    return paths;
  }

  for (auto jpath : pathRes.array_range()) {
    jsoncons::json resArray;
    {
      std::lock_guard<std::mutex> lock_guard(rwMutex_);
      resArray = jsonpath::json_query(data_tree__, jpath.as<string>());
    }
    if (resArray.size() == 0) {
      continue;
    }

    //recurse if branch
    if (resArray[0].contains("type") && resArray[0]["type"].as<string>() == "branch") {
      VSSPath recursepath = VSSPath::fromJSON(jpath.as<string>()+"['children'][*]", path_is_gen1);
      paths.merge(getLeafPaths(recursepath));
      continue;
    }
    else {
      paths.push_back(VSSPath::fromJSON(jpath.as<string>(), path_is_gen1));
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


// Apply changes for the given sourceTree
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

void VssDatabase::updateJsonTree(KuksaChannel& channel,  jsoncons::json& jsonTree){
  if (! channel.authorizedToModifyTree()) {
     stringstream msg;
     msg << "do not have write access for updating json tree or is invalid";
     throw noPermissionException(msg.str());
  }
  if (jsonTree.contains("Vehicle")) {
    applyDefaultValues(jsonTree["Vehicle"], VSSPath::fromVSS(""));
  }
  updateJsonTree(meta_tree__, jsonTree);
  updateJsonTree(data_tree__, jsonTree);
}

// update a metadata in tree, which will only do one-level-deep shallow merge/update.
// If deep merge/update are expected, use `updateJsonTree` instead.
void VssDatabase::updateMetaData(KuksaChannel& channel, const VSSPath& path, const jsoncons::json& metadata){
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

// Set signal value of given path
jsoncons::json VssDatabase::setSignal(const VSSPath &path, const std::string& attr, jsoncons::json &value) {
  jsoncons::json data;
  jsoncons::json datapoint;
  
  data["path"] = path.to_string();

  jsoncons::json res; 
  {
    std::lock_guard<std::mutex> lock_guard(rwMutex_);
    res = jsonpath::json_query(data_tree__, path.getJSONPath());
    if (res.is_array() && res.size() == 1) {
      jsoncons::json resJson = res[0];
      if (resJson.contains("datatype")) {
        checkAndSanitizeType(resJson, value);
        resJson.insert_or_assign(attr, value);
        JsonResponses::addTimeStampToJSON(resJson, "-"+attr);
        
        datapoint.insert_or_assign(attr, value);
        datapoint.insert_or_assign("ts_s-"+attr,  resJson["ts_s-"+attr]);
        datapoint.insert_or_assign("ts_ns-"+attr, resJson["ts_ns-"+attr]);

        data.insert_or_assign("dp", datapoint);
        {
          jsonpath::json_replace(data_tree__, path.getJSONPath(), resJson);
          subHandler_->publishForVSSPath(path, attr, data);
        }
      }
      else {
        throw genException(path.getVSSPath()+ "is invalid for set"); //Todo better error message. (Does not propagate);
      }
    }
  }
  return data;
}

// Returns signal in JSON format
jsoncons::json VssDatabase::getSignal(const VSSPath& path, const std::string& attr, bool as_string) {
    jsoncons::json resArray;
    {
      std::lock_guard<std::mutex> lock_guard(rwMutex_);
      resArray = jsonpath::json_query(data_tree__, path.getJSONPath());
    }
    jsoncons::json answer;
    jsoncons::json datapoint;
    answer.insert_or_assign("path", path.to_string());
    jsoncons::json result = resArray[0];
    if (result.contains(attr)) {
      if (as_string) {
        datapoint.insert_or_assign(attr, result[attr].as<string>());
      }
      else {
        datapoint.insert_or_assign(attr, result[attr]);
      }
    } else {
      datapoint[attr] = "---"; //Todo return error/nothing and handle this correctly
    }
    if (result.contains("ts_s-"+attr) && result.contains("ts_ns-"+attr)) {
      datapoint["ts_s"] = result["ts_s-"+attr].as<uint64_t>();
      datapoint["ts_ns"] = result["ts_ns-"+attr].as<uint32_t>();
    } else {
      datapoint["ts_s"] = 0;
      datapoint["ts_ns"] = 0;
    }

    if (as_string) {
      JsonResponses::convertJSONTimeStampToISO8601(datapoint["dp"]);
    }

    answer.insert_or_assign("dp", datapoint);
    return answer;

}