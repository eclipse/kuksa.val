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
#include "KuksaChannel.hpp"

/**
 * @class IVssCommandProcessor
 * @brief Interface class for handling input JSON requests and providing response
 *
 * @note  Any class implementing this interface shall handle JSON requests as defined by
 *        https://www.w3.org/TR/vehicle-information-service/#message-structure
 */
class IVssCommandProcessor {
  public:
    virtual ~IVssCommandProcessor() {}

    /**
     * @brief Process JSON request and provide response JSON string
     * @param req_json JSON formated request
     * @param channel Active channel information on which \a req_json was received
     * @return JSON formated response string
     */
    virtual std::string processQuery(const std::string &req_json,
                                     KuksaChannel& channel) = 0;
};

#endif
