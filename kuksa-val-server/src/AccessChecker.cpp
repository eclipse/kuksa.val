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


#include "AccessChecker.hpp"

#include <jsoncons/json.hpp>
#include <string>
#include <regex>

#include "IAuthenticator.hpp"
#include "KuksaChannel.hpp"

using namespace std;
using namespace jsoncons;

AccessChecker::AccessChecker(std::shared_ptr<IAuthenticator> vdator) {
  tokenValidator = vdator;
}

bool AccessChecker::checkSignalAccess(const KuksaChannel& channel, const string& path, const string& requiredPermission){
  json permissions;
  if(!channel.getPermissions().empty()){
    permissions = json::parse(channel.getPermissions().as_string());
  }
  else{
    permissions = json::parse("{}");
  }
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


// check the permissions json in KuksaChannel if path has read access
bool AccessChecker::checkReadAccess(KuksaChannel &channel, const VSSPath &path) {
  return checkSignalAccess(channel, path.getVSSGen1Path(), "r");
}

// check the permissions json in KuksaChannel if path has read access
bool AccessChecker::checkWriteAccess(KuksaChannel &channel, const VSSPath &path) {
  return checkSignalAccess(channel, path.getVSSGen1Path(), "w");
}


// Checks if all the paths have write access.If even 1 path in the list does not
// have write access, this method returns false.
bool AccessChecker::checkPathWriteAccess(KuksaChannel &channel, const json &paths) {
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

