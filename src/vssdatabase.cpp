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
#include "vssdatabase.hpp"
#include <limits>
#include <regex>
#include <stdexcept>
#include "exception.hpp"
#include "visconf.hpp"

#include <jsoncons_ext/jsonpath/json_query.hpp>

#include "accesschecker.hpp"
#include "subscriptionhandler.hpp"

using namespace std;
using namespace jsoncons::jsonpath;

// Constructor
vssdatabase::vssdatabase(subscriptionhandler* subHandle,
                         accesschecker* accValidator) {
  subHandler = subHandle;
  accessValidator = accValidator;
}

vssdatabase::~vssdatabase() {}

// Initializer
void vssdatabase::initJsonTree() {
  try {
    string fileName = "vss_rel_1.0.json";
    std::ifstream is(fileName);
    is >> data_tree;
    meta_tree = data_tree;
#ifdef DEBUG
    cout << "vssdatabase::vssdatabase : VSS tree initialized using JSON file = "
         << fileName << endl;
#endif
    is.close();
  } catch (exception const& e) {
    cout << "Exception occured while initializing database/ tree structure. "
            "Probably the init json file not found!"
         << e.what() << endl;
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
string vssdatabase::getReadablePath(string jsonpath) {
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
string vssdatabase::getVSSSpecificPath(string path, bool& isBranch,
                                       jsoncons::json& tree) {
  vector<string> tokens = getPathTokens(path);
  int tokLength = tokens.size();
  string format_path = "$";
  for (int i = 0; i < tokLength; i++) {
    try {
      if (tokens[i] == "*")
        format_path = format_path + "[" + tokens[i] + "]";
      else
        format_path = format_path + "[\'" + tokens[i] + "\']";
      jsoncons::json res = json_query(tree, format_path);
      string type = "";
      if ((res.is_array() && res.size() > 0) && res[0].has_key("type")) {
        type = res[0]["type"].as<string>();
      } else if (res.has_key("type")) {
        type = res["type"].as<string>();
      }

      if (type != "" && type == "branch") {
        if (i < (tokLength - 1)) {
          format_path = format_path + "[\'children\']";
        }
        isBranch = true;
      } else if (type != "") {
        isBranch = false;
        break;
      } else {
        isBranch = false;
        cout << "vssdatabase::getVSSSpecificPath : Path " << format_path
             << " is invalid or is an empty tag!" << endl;
        return "";
      }

    } catch (exception& e) {
      cout << "vssdatabase::getVSSSpecificPath :Exception "
           << "\"" << e.what() << "\""
           << " occured while querying JSON. Check Path!" << endl;
      isBranch = false;
      return "";
    }
  }
  return format_path;
}

// Returns the absolute path but does not resolve wild card. Used only for
// metadata request.
string vssdatabase::getPathForMetadata(string path, bool& isBranch) {
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
list<string> vssdatabase::getPathForGet(string path, bool& isBranch) {
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

// Returns the response JSON for metadata request.
jsoncons::json vssdatabase::getMetaData(string path) {
  string format_path = "$";
  bool isBranch = false;
  rwMutex.lock();
  string jPath = getPathForMetadata(path, isBranch);
  rwMutex.unlock();

  if (jPath == "") {
    return NULL;
  }
#ifdef DEBUG
  cout << "vssdatabase::getMetaData: VSS specific path =" << jPath << endl;
#endif

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
      cout << "vssdatabase::getMetaData : More than 1 Branch/ value found! "
              "Path requested needs to be more refined"
           << endl;
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
jsoncons::json vssdatabase::getPathForSet(string path, jsoncons::json values) {
  jsoncons::json setValues;
  if (values.is_array()) {
    std::size_t found = path.find("*");
    if (found == std::string::npos) {
      path = path + ".";
      found = path.length();
    }
    setValues = jsoncons::json::make_array(values.size());
    for (size_t i = 0; i < values.size(); i++) {
      jsoncons::json value = values[i];
      string readablePath = path;
      string replaceString;
      auto iter = value.object_range();

      int size = std::distance(iter.begin(), iter.end());

      if (size == 1) {
        for (const auto& member : iter) {
          replaceString = string(member.key());
        }
      } else {
        stringstream msg;
        msg << "More than 1 signal found while setting for path = " << path
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
        msg << "Path = " << path << " with values " << pretty_print(values)
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
    string absolutePath = getVSSSpecificPath(path, isBranch, data_tree);
    if (isBranch) {
      stringstream msg;
      msg << "Path = " << path << " with values " << pretty_print(values)
          << " points to a branch. Needs to point to a signal";
      throw genException(msg.str());
    }
    pathValue["path"] = absolutePath;
    pathValue["value"] = values;
    setValues[0] = pathValue;
  }
  return setValues;
}

// Check the value type and if the value is within the range
void checkTypeAndBound(string value_type, jsoncons::json val) {
  bool typeValid = false;

  if (value_type == "UInt8") {
    typeValid = true;
    long double longDoubleVal = val.as<long double>();
    if (!((longDoubleVal <= numeric_limits<uint8_t>::max()) &&
          (longDoubleVal >= numeric_limits<uint8_t>::min()))) {
      cout << "vssdatabase::setSignal: The value passed " << val.as<float>()
           << "for type " << value_type << " is out of bound." << endl;
      std::stringstream msg;
      msg << "The type " << value_type << " with value " << val.as<float>()
          << " is out of bound";
      throw outOfBoundException(msg.str());
    }

  } else if (value_type == "UInt16") {
    typeValid = true;
    long double longDoubleVal = val.as<long double>();
    if (!((longDoubleVal <= numeric_limits<uint16_t>::max()) &&
          (longDoubleVal >= numeric_limits<uint16_t>::min()))) {
      cout << "vssdatabase::setSignal: The value passed " << val.as<float>()
           << "for type " << value_type << " is out of bound." << endl;
      std::stringstream msg;
      msg << "The type " << value_type << " with value " << val.as<float>()
          << " is out of bound";
      throw outOfBoundException(msg.str());
    }

  } else if (value_type == "UInt32") {
    typeValid = true;
    long double longDoubleVal = val.as<long double>();
    if (!((longDoubleVal <= numeric_limits<uint32_t>::max()) &&
          (longDoubleVal >= numeric_limits<uint32_t>::min()))) {
      cout << "vssdatabase::setSignal: The value passed " << val.as<float>()
           << " for type " << value_type << " is out of bound." << endl;
      std::stringstream msg;
      msg << "The type " << value_type << " with value " << val.as<float>()
          << " is out of bound";
      throw outOfBoundException(msg.str());
    }

  } else if (value_type == "Int8") {
    typeValid = true;
    long double longDoubleVal = val.as<long double>();
    if (!((longDoubleVal <= numeric_limits<int8_t>::max()) &&
          (longDoubleVal >= numeric_limits<int8_t>::min()))) {
      cout << "vssdatabase::setSignal: The value passed " << val.as<float>()
           << "for type " << value_type << " is out of bound." << endl;
      std::stringstream msg;
      msg << "The type " << value_type << " with value " << val.as<float>()
          << " is out of bound";
      throw outOfBoundException(msg.str());
    }
  } else if (value_type == "Int16") {
    typeValid = true;
    long double longDoubleVal = val.as<long double>();
    if (!((longDoubleVal <= numeric_limits<int16_t>::max()) &&
          (longDoubleVal >= numeric_limits<int16_t>::min()))) {
      cout << "vssdatabase::setSignal: The value passed " << val.as<float>()
           << "for type " << value_type << " is out of bound." << endl;
      std::stringstream msg;
      msg << "The type " << value_type << " with value " << val.as<float>()
          << " is out of bound";
      throw outOfBoundException(msg.str());
    }

  } else if (value_type == "Int32") {
    typeValid = true;
    long double longDoubleVal = val.as<long double>();
    if (!((longDoubleVal <= numeric_limits<int32_t>::max()) &&
          (longDoubleVal >= numeric_limits<int32_t>::min()))) {
      cout << "vssdatabase::setSignal: The value passed " << val.as<float>()
           << "for type " << value_type << " is out of bound." << endl;
      std::stringstream msg;
      msg << "The type " << value_type << " with value " << val.as<float>()
          << " is out of bound";
      throw outOfBoundException(msg.str());
    }
  } else if (value_type == "Float") {
    typeValid = true;
    long double longDoubleVal = val.as<long double>();
    float max = numeric_limits<float>::max();
    float min = numeric_limits<float>::min();
    if (!(((longDoubleVal <= max) && (longDoubleVal >= min)) ||
          ((longDoubleVal >= (max * -1)) && (longDoubleVal <= (min * -1))))) {
      cout << "vssdatabase::setSignal: The value passed " << val.as<double>()
           << "for type " << value_type << " is out of bound." << endl;
      std::stringstream msg;
      msg << "The type " << value_type << " with value " << val.as<double>()
          << " is out of bound";
      throw outOfBoundException(msg.str());
    }
  } else if (value_type == "Double") {
    typeValid = true;
    long double longDoubleVal = val.as<long double>();
    double max = numeric_limits<double>::max();
    double min = numeric_limits<double>::min();
    if (!(((longDoubleVal <= max) && (longDoubleVal >= min)) ||
          ((longDoubleVal >= (max * -1)) && (longDoubleVal <= (min * -1))))) {
      cout << "vssdatabase::setSignal: The value passed "
           << val.as<long double>() << "for type " << value_type
           << " is out of bound." << endl;
      std::stringstream msg;
      msg << "The type " << value_type << " with value "
          << val.as<long double>() << " is out of bound";
      throw outOfBoundException(msg.str());
    }

  } else if (value_type == "Boolean") {
    typeValid = true;
  } else if (value_type == "String") {
    typeValid = true;
  }

  if (!typeValid) {
    cout << "vssdatabase::setSignal: The  type " << value_type
         << " is either not supported " << endl;
    string msg = "The type " + value_type + " is not supported ";
    throw genException(msg);
  }
}

// Method for setting values to signals.
void vssdatabase::setSignal(wschannel& channel, string path,
                            jsoncons::json valueJson) {
  if (path == "") {
    string msg = "Path is empty while setting";
    throw genException(msg);
  }
  rwMutex.lock();
  jsoncons::json setValues = getPathForSet(path, valueJson);
  rwMutex.unlock();

  if (setValues.is_array()) {
    // check if all the paths have write access.
    bool haveAccess = accessValidator->checkPathWriteAccess(channel, setValues);
    if (!haveAccess) {
      stringstream msg;
      msg << "Path(s) in set request do not have write access or is invalid";
      throw noPermissionException(msg.str());
    }

    for (size_t i = 0; i < setValues.size(); i++) {
      jsoncons::json item = setValues[i];
      string jPath = item["path"].as<string>();
#ifdef DEBUG
      cout << "vssdatabase::setSignal: path found = " << jPath << endl;
      cout << "value to set asstring = " << item["value"].as<string>() << endl;
#endif
      rwMutex.lock();
      jsoncons::json resArray = json_query(data_tree, jPath);
      rwMutex.unlock();
      if (resArray.is_array() && resArray.size() == 1) {
        jsoncons::json resJson = resArray[0];
        if (resJson.has_key("type")) {
          string value_type = resJson["type"].as<string>();
          json val = item["value"];
          checkTypeAndBound(value_type, val);

          resJson.insert_or_assign("value", val);

          rwMutex.lock();
          json_replace(data_tree, jPath, resJson);
          rwMutex.unlock();
#ifdef DEBUG
          cout << "vssdatabase::setSignal: new value set at path " << jPath
               << endl;
#endif

          int signalID = resJson["id"].as<int>();

          jsoncons::json value = resJson["value"];
          subHandler->update(signalID, value);

        } else {
          stringstream msg;
          msg << "Type key not found for " << jPath;
          throw genException(msg.str());
        }

      } else if (resArray.is_array()) {
        cout << "vssdatabase::setSignal : Path " << jPath << " has "
             << resArray.size() << " signals, the path needs refinement"
             << endl;
        stringstream msg;
        msg << "Path " << jPath << " has " << resArray.size()
            << " signals, the path needs refinement";
        throw genException(msg.str());
      }
    }
  } else {
    string msg = "Exception occured while setting data for " + path;
    throw genException(msg);
  }
}

// Utility method for setting values to JSON.
void setJsonValue(jsoncons::json& dest, jsoncons::json& source, string key) {
  if (!source.has_key("type")) {
    cout << "vssdatabase::setJsonValue : could not set value! type is not "
            "present in source json!"
         << endl;
    string msg = "Unknown type for signal found at " + key;
    throw genException(msg);
  }
  dest[key] = source["value"];
}

// Returns response JSON for get request.
jsoncons::json vssdatabase::getSignal(class wschannel& channel, string path) {
  bool isBranch = false;

  rwMutex.lock();
  list<string> jPaths = getPathForGet(path, isBranch);
  rwMutex.unlock();
  int pathsFound = jPaths.size();
  if (pathsFound == 0) {
    jsoncons::json answer;
    return answer;
  }
#ifdef DEBUG
  cout << "vssdatabase::getSignal: " << pathsFound
       << " signals found under path = "
       << "\"" << path << "\"" << endl;
#endif
  if (isBranch) {
    jsoncons::json answer;
#ifdef DEBUG
    cout << " vssdatabase::getSignal :"
         << "\"" << path << "\""
         << " is a Branch." << endl;
#endif
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
          setJsonValue(value, result, getReadablePath(jPath));
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
      msg << "No read access to  " << getReadablePath(jPath);
      throw noPermissionException(msg.str());
    }
    rwMutex.lock();
    jsoncons::json resArray = json_query(data_tree, jPath);
    rwMutex.unlock();
    jsoncons::json answer;
    answer["path"] = getReadablePath(jPath);
    jsoncons::json result = resArray[0];
    if (result.has_key("value")) {
      setJsonValue(answer, result, "value");
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
        setJsonValue(value, result, getReadablePath(jPath));
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
