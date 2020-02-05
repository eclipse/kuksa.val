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
#ifndef __ISIGNING_H__
#define __ISIGNING_H__

#include <string>
#include <jsoncons/json.hpp>

class ISigningHandler {
  public:
    virtual ~ISigningHandler() {}

    virtual std::string getKey(const std::string &fileName) = 0;
    virtual std::string getPublicKey(const std::string &fileName) = 0;
    virtual std::string sign(const jsoncons::json &data) = 0;
    virtual std::string sign(const std::string &data) = 0;

#ifdef UNIT_TEST
    virtual std::string decode(std::string signedData) = 0;
#endif
};

#endif
