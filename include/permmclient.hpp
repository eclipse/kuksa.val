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

#ifndef __PERMMCLIENT_H__
#define __PERMMCLIENT_H__

#include <jsoncons/json.hpp>
#include <memory>

using namespace std;
using namespace jsoncons;

class ILogger;

json getPermToken(std::shared_ptr<ILogger> logger, string clientName, string clientSecret);



#endif
