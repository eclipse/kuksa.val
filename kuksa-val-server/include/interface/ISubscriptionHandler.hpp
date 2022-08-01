/**********************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
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
 **********************************************************************/

#ifndef __ISUBSCRIPTIONHANDLER_H__
#define __ISUBSCRIPTIONHANDLER_H__

#include <string>
#include <memory>
#include <jsoncons/json.hpp>
#include <boost/uuid/uuid.hpp>
#include "IPublisher.hpp"
#include "IServer.hpp"
#include "VSSPath.hpp"

#include "KuksaChannel.hpp"

class VssDatabase;
class WsServer;
class IVssDatabase;
class IPublisher;

using SubscriptionId = boost::uuids::uuid;

class ISubscriptionHandler {
  public:
    virtual ~ISubscriptionHandler() {}

    virtual SubscriptionId subscribe(KuksaChannel& channel,
                                     std::shared_ptr<IVssDatabase> db,
                                     const std::string &path, const std::string& attr) = 0;
    virtual int unsubscribe(SubscriptionId subscribeID) = 0;
    virtual int unsubscribeAll(KuksaChannel channel) = 0;
    virtual int publishForVSSPath(const VSSPath path, const std::string& vssdatatype, const std::string& attr, const jsoncons::json &value) = 0;

    virtual std::shared_ptr<IServer> getServer() = 0;
    virtual int startThread() = 0;
    virtual int stopThread() = 0;
    virtual bool isThreadRunning() const = 0;
    virtual void* subThreadRunner() = 0;
    virtual void addPublisher(std::shared_ptr<IPublisher> publisher) = 0;
};
#endif
