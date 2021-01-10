/*
 * ******************************************************************************
 * Copyright (c) 2020 Robert Bosch GmbH.
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

/** This are tests for VIS Gen2-style get request. No access checks */



#include <boost/test/unit_test.hpp>
#include <turtle/mock.hpp>

#include <memory>
#include <string>

#include "WsChannel.hpp"
#include "ILoggerMock.hpp"
#include "IVssDatabaseMock.hpp"
#include "IAuthenticatorMock.hpp"
#include "ISubscriptionHandlerMock.hpp"

#include "VSSPath.hpp"

#include "exception.hpp"
#include "JsonResponses.hpp"
#include "VssCommandProcessor.hpp"

namespace {
  // common resources for tests
  std::shared_ptr<ILoggerMock> logMock;
  std::shared_ptr<IVssDatabaseMock> dbMock;
  std::shared_ptr<IAuthenticatorMock> authMock;
  std::shared_ptr<ISubscriptionHandlerMock> subsHndlMock;

  std::unique_ptr<VssCommandProcessor> processor;

  // Pre-test initialization and post-test desctruction of common resources
  struct TestSuiteFixture {
    TestSuiteFixture() {
      logMock = std::make_shared<ILoggerMock>();
      dbMock = std::make_shared<IVssDatabaseMock>();
      authMock = std::make_shared<IAuthenticatorMock>();
      subsHndlMock = std::make_shared<ISubscriptionHandlerMock>();

      processor = std::make_unique<VssCommandProcessor>(logMock, dbMock, authMock, subsHndlMock);
    }
    ~TestSuiteFixture() {
      logMock.reset();
      dbMock.reset();
      authMock.reset();
      subsHndlMock.reset();
      processor.reset();
    }
  };
}

// Define name of test suite and define test suite fixture for pre and post test handling
BOOST_FIXTURE_TEST_SUITE(Gen2GetTests, TestSuiteFixture);

//GetVallidpath
//GetNonExistingPatjh
//getBRanch
//getWildcard End
//getWildcardMiddle
//getNonExistingWildcardMiddle


BOOST_AUTO_TEST_CASE(Gen2_Get_Sensor)
{
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/Speed"};
  const VSSPath vss_path = VSSPath::fromVSS(path);

  
  // setup
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
      {
    "action": "get", 
    "path": "Vehicle.Speed", 
    "requestId": "1", 
    "value": "---"
    }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  
  // expectations
  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  
    MOCK_EXPECT(dbMock->getSignal2)
    .once()
    .with(mock::any, mock::equal(vss_path) ,false)
    .returns(expectedJson);
  

  // run UUT
  auto resStr = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  //Does result have a timestamp?
  BOOST_TEST(res["timestamp"].as<int>() > 0);
  
  //Remove timestamp for comparision purposes
  expectedJson["timestamp"]=res["timestamp"].as<int>();
  
  BOOST_TEST(res == expectedJson);
}


BOOST_AUTO_TEST_CASE(Gen2_Get_NonExistingPath)
{
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/OBD/FluxCapacitorCharge"};
  const VSSPath vss_path = VSSPath::fromVSS(path);

  
  // setup
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "get", path, jsonPathNotFound);

  // expectations
  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  //expect a call to getSignal2 with negative result
  MOCK_EXPECT(dbMock->getSignal2)
    .once()
    .with(mock::any, mock::equal(vss_path) ,false)
    .returns(jsonPathNotFound);

  // run UUT
  auto resStr = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  BOOST_TEST(res == jsonPathNotFound);
}

}