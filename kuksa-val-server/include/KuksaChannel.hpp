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

  KuksaChannel ( const KuksaChannel & ) = default;
  KuksaChannel (  ) = default;
  KuksaChannel& operator= (const KuksaChannel& kc) = default;

  friend bool operator==(const KuksaChannel  &lh, const KuksaChannel  &rh) {
    return lh.getConnID() == rh.getConnID();
  }
};
#endif
