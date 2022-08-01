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

#ifndef __SUBSCRIPTIONHANDLER_H__
#define __SUBSCRIPTIONHANDLER_H__

#include <condition_variable>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <string>
#include <thread>
#include <memory>

#include <boost/uuid/uuid.hpp> 
#include <boost/functional/hash.hpp>

#include <jsoncons/json.hpp>

#include "visconf.hpp"
#include "ISubscriptionHandler.hpp"
#include "IAuthenticator.hpp"
#include "IAccessChecker.hpp"
#include "IServer.hpp"
#include "IPublisher.hpp"
#include "VSSPath.hpp"

class AccessChecker;
class Authenticator;
class VssDatabase;
class WsServer;
class ILogger;

// Subscription ID: Client ID
struct UUIDHasher
{
  std::size_t operator()(const boost::uuids::uuid& k) const
  {
    using std::size_t;

    boost::hash<boost::uuids::uuid> uuid_hasher;

    return (uuid_hasher(k));
  }
};
using subscriptions_t = std::unordered_map<SubscriptionId, KuksaChannel, UUIDHasher>;
using subscription_keys_t = struct subscription_keys {
  std::string path;
  std::string attribute;

  // constructor
  subscription_keys(std::string path, std::string attr) {
      this->path = path;
      this->attribute = attr;
  }

  // Equal operator
  bool operator==(const subscription_keys &p) const {
      return this->path == p.path && this->attribute == p.attribute;
  }
};

struct SubscriptionKeyHasher {
  std::size_t operator() (const subscription_keys_t& key) const {
    return (std::hash<VSSPath>()(VSSPath::fromVSS(key.path)) ^ std::hash<std::string>()(key.attribute));
  }
};

class SubscriptionHandler : public ISubscriptionHandler {
 private:
  std::unordered_map<subscription_keys_t, subscriptions_t, SubscriptionKeyHasher> subscriptions;
  std::shared_ptr<ILogger> logger;
  std::shared_ptr<IServer> server;
  std::vector<std::shared_ptr<IPublisher>> publishers_;
  std::shared_ptr<IAuthenticator> validator;
  std::shared_ptr<IAccessChecker> checkAccess;
  mutable std::mutex subMutex;
  mutable std::mutex accessMutex;
  std::condition_variable c;
  std::thread subThread;
  bool threadRun;
  //Tuple is UUID, channel object, vss datatye and jsoncons object for value
  std::queue<std::tuple<SubscriptionId, KuksaChannel, std::string, jsoncons::json>> buffer;

 public:
  SubscriptionHandler(std::shared_ptr<ILogger> loggerUtil,
                      std::shared_ptr<IServer> wserver,
                      std::shared_ptr<IAuthenticator> authenticate,
                      std::shared_ptr<IAccessChecker> checkAccess);
  ~SubscriptionHandler();

  void addPublisher(std::shared_ptr<IPublisher> publisher){
    publishers_.push_back(publisher);
  }
  SubscriptionId subscribe(KuksaChannel& channel,
                           std::shared_ptr<IVssDatabase> db,
                           const std::string &path, const std::string& attr);
  int unsubscribe(SubscriptionId subscribeID);
  int unsubscribeAll(KuksaChannel channel);
  int publishForVSSPath(const VSSPath path, const std::string& vssdatatype, const std::string& attr, const jsoncons::json &value);


  std::shared_ptr<IServer> getServer();
  int startThread();
  int stopThread();
  bool isThreadRunning() const;
  void* subThreadRunner();
};
#endif
