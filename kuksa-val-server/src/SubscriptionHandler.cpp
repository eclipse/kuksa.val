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

#include "SubscriptionHandler.hpp"

#include <unistd.h>  // usleep
#include <string>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <jsonpath/json_query.hpp>

#include "AccessChecker.hpp"
#include "Authenticator.hpp"
#include "ILogger.hpp"
#include "JsonResponses.hpp"
#include "KuksaChannel.hpp"
#include "VssDatabase.hpp"
#include "exception.hpp"
#include "visconf.hpp"

#include "grpcHandler.hpp"

using namespace std;
using namespace jsoncons::jsonpath;
using jsoncons::json;

SubscriptionHandler::SubscriptionHandler(
    std::shared_ptr<ILogger> loggerUtil, std::shared_ptr<IServer> wserver,
    std::shared_ptr<IAuthenticator> authenticate,
    std::shared_ptr<IAccessChecker> checkAcc)
    : publishers_() {
  logger = loggerUtil;
  server = wserver;
  validator = authenticate;
  checkAccess = checkAcc;
  startThread();
}

SubscriptionHandler::~SubscriptionHandler() { stopThread(); }

SubscriptionId SubscriptionHandler::subscribe(KuksaChannel& channel,
                                              std::shared_ptr<IVssDatabase> db,
                                              const string& path,
                                              const std::string& attr) {
  // generate subscribe ID "randomly".
  SubscriptionId subId = boost::uuids::random_generator()();

  VSSPath vssPath = VSSPath::fromVSS(path);

  if (!db->pathExists(vssPath)) {
    throw noPathFoundonTree(path);
  } else if (!db->pathIsReadable(vssPath)) {
    stringstream msg;
    msg << path
        << " is not a sensor, actor or attribute leaf. Subscribe not supported";
    logger->Log(LogLevel::INFO,
                "SubscriptionHandler::subscribe : " + msg.str());
    throw noPathFoundonTree(msg.str());
  } else if (!checkAccess->checkReadAccess(channel, vssPath)) {
    stringstream msg;
    msg << "no permission to subscribe to path " << path;
    throw noPermissionException(msg.str());
  }

  subscription_keys_t subsKey = subscription_keys_t(vssPath.getVSSPath(), attr);
  logger->Log(LogLevel::VERBOSE,
              string("SubscriptionHandler::subscribe: Subscribing to ") +
                  vssPath.getVSSPath());

  std::unique_lock<std::mutex> lock(accessMutex);
  subscriptions[subsKey][subId] = channel;
  return subId;
}

int SubscriptionHandler::unsubscribe(SubscriptionId subscribeID) {
  bool found_subscription = false;
  logger->Log(LogLevel::VERBOSE,
              string("SubscriptionHandler::unsubscribe: Unsubscribe on ") +
                  boost::uuids::to_string(subscribeID));
  std::unique_lock<std::mutex> lock(accessMutex);
  for (auto& sub : subscriptions) {
    auto subsforpath = &(sub.second);
    auto subid = subsforpath->find(subscribeID);
    if (subid != subsforpath->end()) {
      logger->Log(
          LogLevel::VERBOSE,
          string("SubscriptionHandler::unsubscribe: Unsubscribing path ") +
              sub.first.path);
      subsforpath->erase(subid);
      found_subscription = true;
    }
  }
  if (found_subscription) return 0;
  return -1;
}

int SubscriptionHandler::unsubscribeAll(KuksaChannel channel) {
  logger->Log(LogLevel::VERBOSE,
              string("SubscriptionHandler::unsubscribeAll: Unsubscribing all "
                     "for channel ") +
                  std::to_string(channel.getConnID()));

  std::unique_lock<std::mutex> lock(accessMutex);
  for (auto& subs : subscriptions) {
    auto condition =
        [channel](const std::pair<SubscriptionId, KuksaChannel>& pair) {
          return (pair.second == channel);
        };

    auto found =
        std::find_if(std::begin(subs.second), std::end(subs.second), condition);

    if (found != std::end(subs.second)) {
      subs.second.erase(found);
      logger->Log(LogLevel::VERBOSE,
                  "SubscriptionHandler::unsubscribeAll: Unsubscribing " +
                      subs.first.path + " for " +
                      std::to_string(channel.getConnID()));
    }
  }
  return 0;
}

std::shared_ptr<IServer> SubscriptionHandler::getServer() { return server; }

int SubscriptionHandler::publishForVSSPath(const VSSPath path,
                                           const std::string& vssdatatype,
                                           const std::string& attr,
                                           const jsoncons::json& data) {
  // Publish MQTT
  for (auto& publisher : publishers_) {
    publisher->sendPathValue(path.getVSSPath(), data["dp"][attr]);
  }
  std::stringstream ss;
  ss << "SubscriptionHandler::publishForVSSPath: set " << attr << " "
     << data["dp"][attr] << " for path " << path.to_string();
  logger->Log(LogLevel::VERBOSE, ss.str());

  std::unique_lock<std::mutex> lock(accessMutex);
  subscription_keys_t subsKey = subscription_keys_t(path.getVSSPath(), attr);
  auto handle = subscriptions.find(subsKey);
  if (handle == subscriptions.end()) {
    // no subscriptions for path
    return 0;
  }

  for (auto subID : handle->second) {
    std::lock_guard<std::mutex> lock(subMutex);
    tuple<SubscriptionId, KuksaChannel, std::string, json> newSub;
    logger->Log(LogLevel::VERBOSE,
                "SubscriptionHandler::publishForVSSPath: new " + attr +
                    " set at path " + boost::uuids::to_string(subID.first) +
                    ": " + ss.str());
    newSub = std::make_tuple(subID.first, subID.second, vssdatatype, data);
    buffer.push(newSub);
    c.notify_one();
  }
  return 0;
}

void* SubscriptionHandler::subThreadRunner() {
  logger->Log(LogLevel::VERBOSE,
              "SubscribeThread: Started Subscription Thread!");

  while (isThreadRunning()) {
    if (buffer.size() > 0) {
      std::unique_lock<std::mutex> lock(subMutex);
      auto newSub = buffer.front();
      buffer.pop();

      KuksaChannel channel = std::get<1>(newSub);
      std::string vssdatatype = std::get<2>(newSub);
      jsoncons::json data = std::get<3>(newSub);

      jsoncons::json answer;
      answer["action"] = "subscription";
      answer["subscriptionId"] = boost::uuids::to_string(std::get<0>(newSub));

      JsonResponses::convertJSONTimeStampToISO8601(data["dp"]);
      answer.insert_or_assign("data", data);

      stringstream ss;
      ss << pretty_print(answer);
      string message = ss.str();
      if (channel.getType() == KuksaChannel::Type::GRPC) {
        // check for subscriptionID in channel
        auto handle = channel.grpcSubsMap->find(std::get<0>(newSub));
        if (handle == channel.grpcSubsMap->end()) {
          logger->Log(LogLevel::WARNING, "Subscription thread: No subscription for requested path in GRPC");
          continue;
        }
        grpcHandler::grpc_send_object_to_stream(logger, vssdatatype, answer,
                                                handle->second);
      } else {  // WEBSOCKET
        bool connectionexist =
            getServer()->SendToConnection(channel.getConnID(), message);
        if (!connectionexist) {
          this->unsubscribeAll(channel);
        }
      }

    } else {
      std::unique_lock<std::mutex> lock(subMutex);
      c.wait(lock);
    }
  }

  logger->Log(LogLevel::VERBOSE,
              "SubscribeThread: Subscription handler thread stopped running");

  return NULL;
}

int SubscriptionHandler::startThread() {
  subThread = thread(&SubscriptionHandler::subThreadRunner, this);
  threadRun = true;
  return 0;
}

int SubscriptionHandler::stopThread() {
  if (isThreadRunning()) {
    {
      std::lock_guard<std::mutex> lock(subMutex);
      threadRun = false;
      c.notify_one();
    }
    subThread.join();
  }
  return 0;
}

bool SubscriptionHandler::isThreadRunning() const { return threadRun; }
