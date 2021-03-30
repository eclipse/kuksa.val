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
 *      Robert Bosch GmbH 
 * *****************************************************************************
 */


/** This are tests for the type and limit checks during set */

#include <boost/test/unit_test.hpp>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <turtle/mock.hpp>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <memory>
#include <string>

#include "IAccessCheckerMock.hpp"
#include "IAuthenticatorMock.hpp"
#include "ILoggerMock.hpp"
#include "ISubscriptionHandlerMock.hpp"
#include "WsChannel.hpp"

#include "exception.hpp"

#include "VSSPath.hpp"

#include "JsonResponses.hpp"
#include "VssCommandProcessor.hpp"
#include "VssDatabase.hpp"

#include "exception.hpp"



namespace {
// common resources for tests
std::string validFilename{"test_vss_rel_2.0.json"};

std::shared_ptr<ILoggerMock> logMock;
std::shared_ptr<IAccessCheckerMock> accCheckMock;

std::shared_ptr<ISubscriptionHandlerMock> subHandlerMock;

std::shared_ptr<VssDatabase> db;

// Pre-test initialization and post-test desctruction of common resources
struct TestSuiteFixture {
  TestSuiteFixture() {
    logMock = std::make_shared<ILoggerMock>();
    
    accCheckMock = std::make_shared<IAccessCheckerMock>();
    subHandlerMock = std::make_shared<ISubscriptionHandlerMock>();

    std::string vss_file{"test_vss_rel_2.0.json"};

    MOCK_EXPECT(logMock->Log).at_least(0);  // ignore log events
    db = std::make_shared<VssDatabase>(logMock, subHandlerMock, accCheckMock);
    db->initJsonTree(vss_file);
  }
  ~TestSuiteFixture() {
    logMock.reset();
    accCheckMock.reset();
    subHandlerMock.reset();
  }
};
}  // namespace


BOOST_FIXTURE_TEST_SUITE(VSSTypeSanitizerTests, TestSuiteFixture)

BOOST_AUTO_TEST_CASE(valid_uint8_nolimits) {
    jsoncons::json meta;
    meta["datatype"] = "uint8";

    jsoncons::json value = "127";

    BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta,value));
}


BOOST_AUTO_TEST_SUITE_END()

