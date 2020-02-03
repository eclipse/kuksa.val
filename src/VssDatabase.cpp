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

#include <jsonpath/json_query.hpp>
#include <limits>
#include <regex>
#include <stdexcept>
#include <boost/algorithm/string.hpp>

#include "exception.hpp"
#include "visconf.hpp"
#include "ILogger.hpp"
#include "IAccessChecker.hpp"
#include "ISubscriptionHandler.hpp"
#include "VssDatabase.hpp"

using namespace std;
using namespace jsoncons::jsonpath;
using jsoncons::json;

namespace {
  // Check if the value can be converted to requested output type
  template <typename OutType>
  void ValidateValue(std::shared_ptr<ILogger> logger, const jsoncons::json &val, OutType &outVal) {
    try{
      outVal = val.as<OutType>();
    }
    catch (exception &e) {
      std::stringstream msg;
      msg << "The input value '" + val.as_string() + "' is not valid!";
      logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

      throw notValidException(msg.str());
    }
  }

    // Check the value type and if the value is within the range
  void checkTypeAndBound(std::shared_ptr<ILogger> logger, string value_type, jsoncons::json val) {
    bool typeValid = false;

    boost::algorithm::to_lower(value_type);

    if (value_type == "uint8") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<uint8_t>::max()) &&
            (longDoubleVal >= numeric_limits<uint8_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "uint16") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<uint16_t>::max()) &&
            (longDoubleVal >= numeric_limits<uint16_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "uint32") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<uint32_t>::max()) &&
            (longDoubleVal >= numeric_limits<uint32_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "int8") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<int8_t>::max()) &&
            (longDoubleVal >= numeric_limits<int8_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "int16") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<int16_t>::max()) &&
            (longDoubleVal >= numeric_limits<int16_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "int32") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<int32_t>::max()) &&
            (longDoubleVal >= numeric_limits<int32_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "float") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      float max = numeric_limits<float>::max();
      float min = numeric_limits<float>::lowest();
      if (!((longDoubleVal <= max) && (longDoubleVal >= min))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value '" << val.as<double>()
            << "' is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "double") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      double max = numeric_limits<double>::max();
      double min = numeric_limits<double>::lowest();
      if (!((longDoubleVal <= max) && (longDoubleVal >= min))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value "
            << val.as<long double>() << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "boolean") {
      typeValid = true;
    } else if (value_type == "string") {
      typeValid = true;
    }

    if (!typeValid) {
      string msg = "The type " + value_type + " is not supported ";
      logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg);

      throw genException(msg);
    }
  }


  // Utility method for setting values to JSON.
  void setJsonValue(std::shared_ptr<ILogger> logger,
                    jsoncons::json& dest,
                    jsoncons::json& source,
                    string key) {
    if (!source.has_key("type")) {
      string msg = "Unknown type for signal found at " + key;
      logger->Log(LogLevel::ERROR, "VssDatabase::setJsonValue : " + msg);

      throw genException(msg);
    }
    dest[key] = source["value"];
  }
}

// Constructor
VssDatabase::VssDatabase(std::shared_ptr<ILogger> loggerUtil,
                         std::shared_ptr<ISubscriptionHandler> subHandle,
                         std::shared_ptr<IAccessChecker> accValidator) {
  logger = loggerUtil;
  subHandler = subHandle;
  accessValidator = accValidator;
}

VssDatabase::~VssDatabase() {}

// Initializer
void VssDatabase::initJsonTree(const string &fileName) {
  try {
    std::ifstream is(fileName);
    is >> data_tree;
    meta_tree = data_tree;

    logger->Log(LogLevel::VERBOSE, "VssDatabase::VssDatabase : VSS tree initialized using JSON file = "
                + fileName);
    is.close();
  } catch (exception const& e) {
    logger->Log(LogLevel::ERROR,
                "Exception occured while initializing database/ tree structure. Probably the init json file not found!"
                + string(e.what()));
    throw e;
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

// Removes the internally used "children" tag and "$" tag and returns a client
// readable path.
// Eg: jsonpath = $['Signal']['children']['OBD']['children']['RPM']
// The method returns Signal.OBD.RPM
string VssDatabase::getReadablePath(string jsonpath) {
  stringstream ss;
  // regex to remove special characters from JSONPath and make it VSS
  // compatible.
  std::regex bracket("[$\\[\\]']{2,4}");
  std::copy(
      std::sregex_token_iterator(jsonpath.begin(), jsonpath.end(), bracket, -1),
      std::sregex_token_iterator(),
      std::ostream_iterator<std::string>(ss, "."));

  ss.seekg(0, ios::end);
  int size = ss.tellg();
  string readablePath = ss.str().substr(1, size - 2);
  regex e("\\b(.children)([]*)");
  readablePath = regex_replace(readablePath, e, "");
  return readablePath;
}

// Appends the internally used "children" tag to the path. And also formats the
// path in JSONPath querry format.
// Eg: path = Signal.OBD.RPM
// The method returns $['Signal']['children']['OBD']['children']['RPM'] and
// updates isBranch to false.
string VssDatabase::getVSSSpecificPath(const string &path, bool& isBranch,
                                       jsoncons::json& tree) {
  vector<string> tokens = getPathTokens(path);
  int tokLength = tokens.size();
  string format_path = "$";
  for (int i = 0; i < tokLength; i++) {
    if (tokens[i] == "*") {
      format_path = format_path + "[" + tokens[i] + "]";
    } else {
      format_path = format_path + "[\'" + tokens[i] + "\']";
    }

    if (i < (tokLength - 1)) {
      format_path = format_path + "[\'children\']";
    }
  }
  try {
    jsoncons::json res = json_query(tree, format_path);
    string type = "";
    if ((res.is_array() && res.size() > 0) && res[0].has_key("type")) {
      type = res[0]["type"].as<string>();
    } else if (res.has_key("type")) {
      type = res["type"].as<string>();
    }
    if (type != "" && type == "branch") {
      isBranch = true;
    } else if (type != "") {
      isBranch = false;
    } else {
      isBranch = false;
      logger->Log(LogLevel::ERROR, "VssDatabase::getVSSSpecificPath : Path "
                  + format_path + " is invalid or is an empty tag!");
      return "";
    }
  } catch (exception& e) {
    logger->Log(LogLevel::ERROR, "VssDatabase::getVSSSpecificPath :Exception \""
         + string(e.what()) + "\" occured while querying JSON. Check Path!");
    isBranch = false;
    return "";
  }
  return format_path;
}

// Returns the absolute path but does not resolve wild card. Used only for
// metadata request.
string VssDatabase::getPathForMetadata(string path, bool& isBranch) {
  string format_path = getVSSSpecificPath(path, isBranch, meta_tree);
  jsoncons::json pathRes = json_query(meta_tree, format_path, result_type::path);
  if (pathRes.size() > 0) {
    string jPath = pathRes[0].as<string>();
    return jPath;
  } else {
    return "";
  }
}

// This method uses recursion.
// Returns the path for get method. Resolves wild card and gives the absolute
// path.
// For eg : path = Signal.*.RPM
// The method would return a list containing 1 signal path =
// $['Signal']['children']['OBD']['children']['RPM']
list<string> VssDatabase::getPathForGet(const string &path, bool& isBranch) {
  list<string> paths;
  string format_path = getVSSSpecificPath(path, isBranch, data_tree);
  jsoncons::json pathRes = json_query(data_tree, format_path, result_type::path);

  for (size_t i = 0; i < pathRes.size(); i++) {
    string jPath = pathRes[i].as<string>();
    jsoncons::json resArray = json_query(data_tree, jPath);
    if (resArray.size() == 0) {
      continue;
    }
    jsoncons::json result = resArray[0];
    if (!result.has_key("id") &&
        (result.has_key("type") && result["type"].as<string>() == "branch")) {
      bool dummy = true;
      paths.merge(getPathForGet(getReadablePath(jPath) + ".*", dummy));
      continue;
    } else {
      paths.push_back(pathRes[i].as<string>());
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
void VssDatabase::HandleSet(jsoncons::json & setValues) {
  for (size_t i = 0; i < setValues.size(); i++) {
    jsoncons::json item = setValues[i];
    string jPath = item["path"].as<string>();

    logger->Log(LogLevel::VERBOSE, "vssdatabase::setSignal: path found = " + jPath);
    logger->Log(LogLevel::VERBOSE, "value to set asstring = " + item["value"].as<string>());

    rwMutex.lock();
    jsoncons::json resArray = json_query(data_tree, jPath);
    rwMutex.unlock();
    if (resArray.is_array() && resArray.size() == 1) {
      jsoncons::json resJson = resArray[0];
      if (resJson.has_key("datatype")) {
        string value_type = resJson["datatype"].as<string>();
        json val = item["value"];
        checkTypeAndBound(logger, value_type, val);

        resJson.insert_or_assign("value", val);

        rwMutex.lock();
        json_replace(data_tree, jPath, resJson);
        rwMutex.unlock();

        logger->Log(LogLevel::VERBOSE, "vssdatabase::setSignal: new value set at path " + jPath);

        string uuid = resJson["uuid"].as<string>();

        jsoncons::json value = resJson["value"];
        subHandler->updateByUUID(uuid, value);
      } else {
        stringstream msg;
        msg << "Type key not found for " << jPath;
        throw genException(msg.str());
      }

    } else if (resArray.is_array()) {
      stringstream msg;
      msg << "Path " << jPath << " has " << resArray.size()
          << " signals, the path needs refinement";
      logger->Log(LogLevel::INFO, "vssdatabase::setSignal : " + msg.str());
      throw genException(msg.str());
    }
  }
}

// Returns the response JSON for metadata request.
jsoncons::json VssDatabase::getMetaData(const std::string &path) {
  string format_path = "$";
  bool isBranch = false;
  rwMutex.lock();
  string jPath = getPathForMetadata(path, isBranch);
  rwMutex.unlock();

  if (jPath == "") {
    return NULL;
  }
  logger->Log(LogLevel::VERBOSE, "VssDatabase::getMetaData: VSS specific path =" + jPath);

  vector<string> tokens = getVSSTokens(jPath);
  int tokLength = tokens.size();

  jsoncons::json parentArray[16];
  string parentName[16];

  int parentCount = 0;
  jsoncons::json resJson;
  for (int i = 0; i < tokLength; i++) {
    format_path = format_path + "." + tokens[i];
    if ((i < tokLength - 1) && (tokens[i] == "children")) {
      continue;
    }
    rwMutex.lock();
    jsoncons::json resArray = json_query(meta_tree, format_path);
    rwMutex.unlock();

    if (resArray.is_array() && resArray.size() == 1) {
      resJson = resArray[0];
      if (resJson.has_key("description")) {
        resJson.insert_or_assign("description",
                                 resJson["description"].as<string>());
      }

      if (resJson.has_key("type")) {
        string type = resJson["type"].as<string>();
        resJson.insert_or_assign("type", type);
      }
    } else {
      // handle exception.
      logger->Log(LogLevel::ERROR, string("VssDatabase::getMetaData : More than 1 Branch/ value found! ")
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

// returns the absolute path based on the base path and the path in the values
// JSON.
// eg : path = Signal.*
//      values = {"OBD.RPM" : 23, "OBD.Speed" : 10}
// The function would return = {{"path" : "$.Signal.children.OBD.children.RPM",
// "value" : 23}, {"path" : "$.Signal.children.OBD.children.Speed", "value" :
// 10}}
jsoncons::json VssDatabase::getPathForSet(const string &path, jsoncons::json values) {
  jsoncons::json setValues;
  string updatedPath = path;

  if (values.is_array()) {
    std::size_t found = updatedPath.find("*");
    if (found == std::string::npos) {
      updatedPath = updatedPath + ".";
      found = updatedPath.length();
    }
    setValues = jsoncons::json::make_array(values.size());
    for (size_t i = 0; i < values.size(); i++) {
      jsoncons::json value = values[i];
      string readablePath = updatedPath;
      string replaceString;
      auto iter = value.object_range();

      int size = std::distance(iter.begin(), iter.end());

      if (size == 1) {
        for (const auto& member : iter) {
          replaceString = string(member.key());
        }
      } else {
        stringstream msg;
        msg << "More than 1 signal found while setting for path = " << updatedPath
            << " with values " << pretty_print(values);
        throw genException(msg.str());
      }
      readablePath.replace(found, readablePath.length(), replaceString);
      jsoncons::json pathValue;
      bool isBranch = false;
      string absolutePath =
          getVSSSpecificPath(readablePath, isBranch, data_tree);
      if (isBranch) {
        stringstream msg;
        msg << "Path = " << updatedPath << " with values " << pretty_print(values)
            << " points to a branch. Needs to point to a signal";
        throw genException(msg.str());
      }

      pathValue["path"] = absolutePath;
      pathValue["value"] = value[replaceString];
      setValues[i] = pathValue;
    }
  } else {
    setValues = jsoncons::json::make_array(1);
    jsoncons::json pathValue;
    bool isBranch = false;
    string absolutePath = getVSSSpecificPath(updatedPath, isBranch, data_tree);
    if (isBranch) {
      stringstream msg;
      msg << "Path = " << updatedPath << " with values " << pretty_print(values)
          << " points to a branch. Needs to point to a signal";
      throw genException(msg.str());
    }
    pathValue["path"] = absolutePath;
    pathValue["value"] = values;
    setValues[0] = pathValue;
  }
  return setValues;
}

void VssDatabase::checkSetPermission(WsChannel& channel, jsoncons::json valueJson) {
    // check if all the paths have write access.
    bool haveAccess = accessValidator->checkPathWriteAccess(channel, valueJson);
    if (!haveAccess) {
       stringstream msg;
       msg << "Path(s) in set request do not have write access or is invalid";
       throw noPermissionException(msg.str());
    }
}

// Method for setting values to signals.
void VssDatabase::setSignal(WsChannel& channel,
                            const string &path,
                            jsoncons::json valueJson) {
  if (path == "") {
    string msg = "Path is empty while setting";
    throw genException(msg);
  }
  rwMutex.lock();
  jsoncons::json setValues = getPathForSet(path, valueJson);
  rwMutex.unlock();

  if (setValues.is_array()) {
    checkSetPermission(channel, setValues);
    HandleSet(setValues);
  } else {
    string msg = "Exception occured while setting data for " + path;
    throw genException(msg);
  }
}

// Method for setting values to signals.
void VssDatabase::setSignal(const string &path,
                            jsoncons::json valueJson) {
  if (path == "") {
    string msg = "Path is empty while setting";
    throw genException(msg);
  }
  rwMutex.lock();
  jsoncons::json setValues = getPathForSet(path, valueJson);
  rwMutex.unlock();

  if (setValues.is_array()) {
    HandleSet(setValues);
  } else {
    string msg = "Exception occurred while setting data for " + path;
    logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg);
    throw genException(msg);
  }
}

// Returns response JSON for get request.
jsoncons::json VssDatabase::getSignal(class WsChannel& channel, const string &path) {
  bool isBranch = false;

  rwMutex.lock();
  list<string> jPaths = getPathForGet(path, isBranch);
  rwMutex.unlock();
  int pathsFound = jPaths.size();
  if (pathsFound == 0) {
    jsoncons::json answer;
    return answer;
  }

  logger->Log(LogLevel::VERBOSE, "VssDatabase::getSignal: " + to_string(pathsFound)
              + " signals found under path = \"" + path + "\"");
  if (isBranch) {
    jsoncons::json answer;
    logger->Log(LogLevel::VERBOSE, " VssDatabase::getSignal : \"" + path + "\" is a Branch.");

    if (pathsFound == 0) {
      throw noPathFoundonTree(path);
    } else {
      jsoncons::json value;

      for (int i = 0; i < pathsFound; i++) {
        string jPath = jPaths.back();
        // check Read access here.
        if (!accessValidator->checkReadAccess(channel, jPath)) {
          // Allow the permitted signals to return. If exception is enable here,
          // then say only "Signal.OBD.RPM" is permitted and get request is made
          // for a branch like "Signal.OBD" then
          // an error would be returned. By disabling the exception the value
          // for Signal.OBD.RPM (only) will be returned.
          /*stringstream msg;
          msg << "No read access to  " << getReadablePath(jPath);
          throw genException (msg.str());*/
          jPaths.pop_back();
          continue;
        }
        rwMutex.lock();
        jsoncons::json resArray = json_query(data_tree, jPath);
        rwMutex.unlock();
        jPaths.pop_back();
        jsoncons::json result = resArray[0];
        if (result.has_key("value")) {
          setJsonValue(logger, value, result, getReadablePath(jPath));
        } else {
          value[getReadablePath(jPath)] = "---";
        }
      }
      answer["value"] = value;
    }
    return answer;
  } else if (pathsFound == 1) {
    string jPath = jPaths.back();
    // check Read access here.
    if (!accessValidator->checkReadAccess(channel, jPath)) {
      stringstream msg;
      msg << "No read access to " << getReadablePath(jPath);
      throw noPermissionException(msg.str());
    }
    rwMutex.lock();
    jsoncons::json resArray = json_query(data_tree, jPath);
    rwMutex.unlock();
    jsoncons::json answer;
    answer["path"] = getReadablePath(jPath);
    jsoncons::json result = resArray[0];
    if (result.has_key("value")) {
      setJsonValue(logger, answer, result, "value");
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
      // Check access here.
      if (!accessValidator->checkReadAccess(channel, jPath)) {
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
      rwMutex.lock();
      jsoncons::json resArray = json_query(data_tree, jPath);
      rwMutex.unlock();
      jPaths.pop_back();
      jsoncons::json result = resArray[0];
      if (result.has_key("value")) {
        setJsonValue(logger, value, result, getReadablePath(jPath));
      } else {
        value[getReadablePath(jPath)] = "---";
      }
      valueArray.insert(valueArray.array_range().end(), value);
    }
    answer["value"] = valueArray;
    return answer;
  }
  return NULL;
}
