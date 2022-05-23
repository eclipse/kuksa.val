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

class WsChannel {
 public:
  enum class Type {
    WEBSOCKET_PLAIN,
    WEBSOCKET_SSL,
    HTTP_PLAIN,
    HTTP_SSL,
    GRPC
  };
 private:
  uint64_t connectionID;
  bool authorized = false;
  bool modifyTree = false;
  string authToken;
  json permissions;
  Type typeOfConnection;

 public:

  void setConnID(uint64_t conID) { connectionID = conID; }
  void setAuthorized(bool isauth) { authorized = isauth; }
  void setAuthToken(string tok) { authToken = tok; }
  void setPermissions(json perm) { permissions = perm; }
  void setType(Type type) { typeOfConnection = type; }
  void enableModifyTree (){ modifyTree = true; }

  uint64_t getConnID() const { return connectionID; }
  bool isAuthorized() const { return authorized; }
  bool authorizedToModifyTree() const { return modifyTree; }
  string getAuthToken() const { return authToken; }
  json getPermissions() const { return permissions; }
  Type getType() const { return typeOfConnection; }
};
#endif
