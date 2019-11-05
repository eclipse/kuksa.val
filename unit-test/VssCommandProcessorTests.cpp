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
#include <boost/test/unit_test.hpp>
#include <turtle/mock.hpp>

#include <memory>
#include <string>

#include "WsChannel.hpp"
#include "ILoggerMock.hpp"
#include "IVssDatabaseMock.hpp"
#include "IAuthenticatorMock.hpp"
#include "ISubscriptionHandlerMock.hpp"

#include "VssCommandProcessor.hpp"

BOOST_AUTO_TEST_SUITE(VssCommandProcessorTests)


BOOST_AUTO_TEST_CASE(ProcessValidGetQueryForSingleSignal)
{
  WsChannel channel;
  std::shared_ptr<ILoggerMock> logMock = std::make_shared<ILoggerMock>();
  std::shared_ptr<IVssDatabaseMock> dbMock = std::make_shared<IVssDatabaseMock>();
  std::shared_ptr<IAuthenticatorMock> authMock = std::make_shared<IAuthenticatorMock>();
  std::shared_ptr<ISubscriptionHandlerMock> subsHndlMock = std::make_shared<ISubscriptionHandlerMock>();

  VssCommandProcessor proc(logMock, dbMock, authMock, subsHndlMock);
}

BOOST_AUTO_TEST_SUITE_END()
