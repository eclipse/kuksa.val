/*
 * ******************************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
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
    virtual void SendToConnection(ConnectionId connID, const std::string &message) = 0;
};
#endif
