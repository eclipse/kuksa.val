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

#include "IAccessChecker.hpp"

MOCK_BASE_CLASS( IAccessCheckerMock, IAccessChecker )
{
  MOCK_METHOD(checkReadAccess, 2, bool(WsChannel&, const std::string& ), checkReadDeprecated)
  MOCK_METHOD(checkReadAccess, 2, bool(WsChannel&, const VSSPath& ), checkReadNew)
  MOCK_METHOD(checkWriteAccess, 2)
  MOCK_METHOD(checkPathWriteAccess, 2)
};
