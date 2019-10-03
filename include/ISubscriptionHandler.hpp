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
#ifndef __ISUBSCRIPTIONHANDLER_H__
#define __ISUBSCRIPTIONHANDLER_H__

#include <string>
#include <memory>
#include <jsoncons/json.hpp>

class VssDatabase;
class WsChannel;
class WsServer;
class IVssDatabase;
class IServer;

class ISubscriptionHandler {
  public:
    virtual ~ISubscriptionHandler() {}

    virtual uint32_t subscribe(WsChannel& channel,
                               std::shared_ptr<IVssDatabase> db,
                               uint32_t channelID,
                               const std::string &path) = 0;
    virtual int unsubscribe(uint32_t subscribeID) = 0;
    virtual int unsubscribeAll(uint32_t connectionID) = 0;
    virtual int updateByUUID(const std::string &signalUUID, const jsoncons::json &value) = 0;
    virtual int updateByPath(const std::string &path, const jsoncons::json &value) = 0;
    virtual std::shared_ptr<IServer> getServer() = 0;
    virtual int startThread() = 0;
    virtual int stopThread() = 0;
    virtual bool isThreadRunning() const = 0;
    virtual void* subThreadRunner() = 0;
};
#endif
