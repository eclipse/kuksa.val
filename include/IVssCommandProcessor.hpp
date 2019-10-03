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
#ifndef __IVSSCOMMANDPROCESSOR_H__
#define __IVSSCOMMANDPROCESSOR_H__

#include <string>

#include <jsoncons/json.hpp>

class WsChannel;

class IVssCommandProcessor {
  public:
    virtual ~IVssCommandProcessor() {}

    virtual std::string processQuery(const std::string &req_json,
                                     WsChannel& channel) = 0;
};

#endif
