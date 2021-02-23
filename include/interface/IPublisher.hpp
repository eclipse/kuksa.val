/*
 * ******************************************************************************
 * Copyright (c) 2021 Robert Bosch GmbH.
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

#ifndef __IWSPUBLISHER_H__
#define __IWSPUBLISHER_H__

#include <string>
#include <memory>
#include <jsoncons/json.hpp>

class IPublisher {
  public:
    virtual ~IPublisher() {}

    virtual bool sendPathValue(const std::string &path, const jsoncons::json &value) = 0;
};
#endif
