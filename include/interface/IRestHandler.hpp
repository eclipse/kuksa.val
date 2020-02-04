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
#ifndef __IRESTHANDLER_H__
#define __IRESTHANDLER_H__

#include <string>

/**
 * @class IRestHandler
 * @brief Interface class for handling REST API and convert them to default JSON
 *
 * @note  Any class implementing this interface shall output JSON requests as defined by
 *        https://www.w3.org/TR/vehicle-information-service/#message-structure
 */
class IRestHandler {
  public:
    virtual ~IRestHandler() = default;

    /**
     * @brief Get JSON request string based on input REST API request
     * @param restMethod REST request method string (get/set/...)
     * @param restTarget REST request URI
     * @param resultJson Output JSON string for both valid and invalid REST requests. If invalid REST
     *        request, parameter shall contain JSON describing error
     * @return true if REST request correct and JSON request generated
     *         false otherwise indicating error in REST request. \a jsonRequest shall be updated
     *         with JSON response providing error details
     *
     * @note Function can be updated/extended/overloaded to provide additional relevant information,
     *       (e.g. HTTP header information, body of request with JSON, etc...)
     */
    virtual bool GetJson(std::string&& restMethod,
                         std::string&& restTarget,
                         std::string& resultJson) = 0;
};


#endif
