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

#include "IAuthenticator.hpp"

MOCK_BASE_CLASS( IAuthenticatorMock, IAuthenticator )
{
  MOCK_METHOD(validate, 3)
  MOCK_METHOD(updatePubKey, 1)
  MOCK_METHOD(isStillValid, 1)
  MOCK_METHOD(resolvePermissions, 2)
};
