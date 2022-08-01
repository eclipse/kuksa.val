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


#ifndef __IWSSERVER_H__
#define __IWSSERVER_H__

#include <string>
#include <memory>

class IVssCommandProcessor;

/**
 * \class ObserverType
 * \brief Server traffic types which can be observed
 */
enum class ObserverType {
  WEBSOCKET = 0x01, //!< Receive Web-Socket traffic data
  HTTP      = 0x02, //!< Receive HTTP traffic data
  ALL       = 0x03  //!< Receive all traffic
};

using ConnectionId = uint64_t;

class IServer {
  public:
    virtual ~IServer() {}

    virtual void AddListener(ObserverType, std::shared_ptr<IVssCommandProcessor>) = 0;
    virtual void RemoveListener(ObserverType, std::shared_ptr<IVssCommandProcessor>) = 0;
    virtual bool SendToConnection(ConnectionId connID, const std::string &message) = 0;
};
#endif
