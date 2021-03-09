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

#pragma once

#include <turtle/mock.hpp>

#include "IVssDatabase.hpp"

MOCK_BASE_CLASS( IVssDatabaseMock, IVssDatabase )
{
  MOCK_METHOD(initJsonTree, 1)
  MOCK_METHOD(updateJsonTree, 2)
  MOCK_METHOD(updateMetaData, 3)
  MOCK_METHOD(getMetaData, 1)
  MOCK_METHOD(setSignal, 4)
  MOCK_METHOD(getSignal, 2, jsoncons::json(WsChannel&, const std::string &), getSignalDeprecated )
  MOCK_METHOD(getSignal, 3, jsoncons::json(WsChannel&, const VSSPath&, bool), getSignalNew )
  MOCK_METHOD(getPathForGet, 2)
  MOCK_METHOD(getVSSSpecificPath, 3)
  MOCK_METHOD(getPathForSet, 2)
  MOCK_METHOD(pathExists, 1)
  MOCK_METHOD(pathIsWritable, 1)
};
