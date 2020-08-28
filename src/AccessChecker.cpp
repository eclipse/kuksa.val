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

#include "IAuthenticator.hpp"
#include "WsChannel.hpp"

using namespace std;
using namespace jsoncons;
//using jsoncons::json;

AccessChecker::AccessChecker(std::shared_ptr<IAuthenticator> vdator) {
  tokenValidator = vdator;
}

// check the permissions json in WsChannel if path has read access
bool AccessChecker::checkReadAccess(WsChannel &channel, const string &path) {
  json permissions = channel.getPermissions();
  string perm = permissions.get_with_default(path, "");

  if (perm == "r" || perm == "rw" || perm == "wr") {
    return true;
  }
  return false;
}

// check the permissions json in WsChannel if path has write access
bool AccessChecker::checkWriteAccess(WsChannel &channel, const string &path) {
  json permissions = channel.getPermissions();
  string perm = permissions.get_with_default(path, "");

  if (perm == "w" || perm == "rw" || perm == "wr") {
    return true;
  }
  return false;
}

// Checks if all the paths have write access.If even 1 path in the list does not
// have write access, this method returns false.
bool AccessChecker::checkPathWriteAccess(WsChannel &channel, const json &paths) {
  for (size_t i = 0; i < paths.size(); i++) {
    json item = paths[i];
    string jPath = item["path"].as<string>();
    if (!checkWriteAccess(channel, jPath)) {
      return false;
    }
  }
  return true;
}

// check the permissions json in WsChannel if path has write access to metadata
bool AccessChecker::checkMetaDataWriteAccess(WsChannel &channel){
  string permission = channel.getPermissions().get_with_default("metadata", "r");
  // TODO
  return permission.find("r") != std::string::npos;
  
}
