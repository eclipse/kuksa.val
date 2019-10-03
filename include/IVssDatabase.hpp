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
#ifndef __IVSSDATABASE_HPP__
#define __IVSSDATABASE_HPP__

#include <string>

#include <jsoncons/json.hpp>

class WsChannel;

class IVssDatabase {
  public:
    virtual ~IVssDatabase() {}

    virtual void initJsonTree(const std::string &fileName) = 0;
    virtual jsoncons::json getMetaData(const std::string &path) = 0;
    virtual void setSignal(WsChannel& channel,
                           const std::string &path,
                           jsoncons::json value) = 0;
    virtual jsoncons::json getSignal(WsChannel& channel, const std::string &path) = 0;
};

#endif
