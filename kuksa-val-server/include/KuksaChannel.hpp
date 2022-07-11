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
#ifndef __KUKSA_CHANNEL_H__
#define __KUKSA_CHANNEL_H__

#include <stdint.h>
#include <jsoncons/json.hpp>
#include <string>
#include <boost/uuid/uuid_io.hpp>  
#include <boost/functional/hash.hpp>
#include "kuksa.grpc.pb.h"

using namespace std;
using namespace jsoncons;
using jsoncons::json;

struct gRPCUUIDHasher
{
  std::size_t operator()(const boost::uuids::uuid& k) const
  {
    using std::size_t;

    boost::hash<boost::uuids::uuid> uuid_hasher;

    return (uuid_hasher(k));
  }
};

using gRPCSubscriptionMap_t = std::unordered_map<boost::uuids::uuid, grpc::ServerReaderWriter<::kuksa::SubscribeResponse, ::kuksa::SubscribeRequest>*, gRPCUUIDHasher>;


class KuksaChannel {
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
  std::shared_ptr<gRPCSubscriptionMap_t> grpcSubsMap;

  friend bool operator==(const KuksaChannel  &lh, const KuksaChannel  &rh) {
    return lh.getConnID() == rh.getConnID();
  }
};
#endif
