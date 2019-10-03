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


class IWsServer {
  public:
    virtual ~IWsServer() {}

    virtual void startServer(const std::string &endpointName) = 0;
    virtual void sendToConnection(uint32_t connID, const std::string &message) = 0;
    virtual void start() = 0;
};
#endif
