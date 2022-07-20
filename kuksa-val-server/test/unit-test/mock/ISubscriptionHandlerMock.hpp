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
#include "ISubscriptionHandler.hpp"

MOCK_BASE_CLASS( ISubscriptionHandlerMock, ISubscriptionHandler )
{
  MOCK_METHOD(subscribe, 4)
  MOCK_METHOD(unsubscribe, 1)
  MOCK_METHOD(unsubscribeAll, 1)
  MOCK_METHOD(publishForVSSPath, 4)
  MOCK_METHOD(getServer, 0)
  MOCK_METHOD(startThread, 0)
  MOCK_METHOD(stopThread, 0)
  MOCK_CONST_METHOD(isThreadRunning, 0, bool(void))
  MOCK_METHOD(subThreadRunner, 0, void*(void))
  MOCK_METHOD(addPublisher, 1, void(std::shared_ptr<IPublisher>))
};
