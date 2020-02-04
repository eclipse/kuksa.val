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

#include "IServer.hpp"

MOCK_BASE_CLASS( IServerMock, IServer )
{
  MOCK_METHOD(AddListener, 2)
  MOCK_METHOD(RemoveListener, 2)
  MOCK_METHOD(SendToConnection, 2, void( ConnectionId, const std::string & ) )
};
