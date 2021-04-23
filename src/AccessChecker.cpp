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

#include "AccessChecker.hpp"

#include <jsoncons/json.hpp>
#include <string>
#include <regex>

#include "IAuthenticator.hpp"
#include "WsChannel.hpp"

using namespace std;
using namespace jsoncons;
//using jsoncons::json;

AccessChecker::AccessChecker(std::shared_ptr<IAuthenticator> vdator) {
  tokenValidator = vdator;
}

bool AccessChecker::checkSignalAccess(const WsChannel& channel, const string& path, const string& requiredPermission){
  json permissions = channel.getPermissions();
  string permissionValue = permissions.get_with_default(path, "");
  if (permissionValue.empty()){
    for (auto permission : permissions.object_range()) {
      string pathString(permission.key());
      auto path_regex = std::regex{
        std::regex_replace(pathString, std::regex("\\*"), std::string(".*"))};
      std::smatch base_match;
      if (std::regex_match(path, base_match, path_regex)) {
        permissionValue = permission.value().as<string>();
      }
    }
  
  } 
  return permissionValue.find(requiredPermission) != std::string::npos;
}


// check the permissions json in WsChannel if path has read access
bool AccessChecker::checkReadAccess(WsChannel &channel, const VSSPath &path) {
  return checkSignalAccess(channel, path.getVSSGen1Path(), "r");
}

// check the permissions json in WsChannel if path has read access
bool AccessChecker::checkWriteAccess(WsChannel &channel, const VSSPath &path) {
  return checkSignalAccess(channel, path.getVSSGen1Path(), "w");
}


// Checks if all the paths have write access.If even 1 path in the list does not
// have write access, this method returns false.
bool AccessChecker::checkPathWriteAccess(WsChannel &channel, const json &paths) {
  for (size_t i = 0; i < paths.size(); i++) {
    json item = paths[i];
    string jPath = item["path"].as<string>();
    // TODO check if this function is needed and if false here is correct
    VSSPath path = VSSPath::fromJSON(jPath, false);
    if (!checkWriteAccess(channel, path)) {
      return false;
    }
  }
  return true;
}

