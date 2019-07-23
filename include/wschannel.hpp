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
#ifndef __WSCHANNEL_H__
#define __WSCHANNEL_H__

#include <stdint.h>
#include <jsoncons/json.hpp>
#include <string>

using namespace std;
using namespace jsoncons;
using jsoncons::json;

class wschannel {
 private:
  uint32_t connectionID;
  bool authorized = false;
  string authToken;
  json permissions;

 public:
  void setConnID(uint32_t conID) { connectionID = conID; }
  void setAuthorized(bool isauth) { authorized = isauth; }
  void setAuthToken(string tok) { authToken = tok; }
  void setPermissions(json perm) { permissions = perm; }

  uint32_t getConnID() { return connectionID; }

  bool isAuthorized() { return authorized; }

  string getAuthToken() { return authToken; }

  json getPermissions() { return permissions; }
};
#endif
