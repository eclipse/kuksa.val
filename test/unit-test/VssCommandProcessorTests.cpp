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
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <turtle/mock.hpp>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS


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
BOOST_FIXTURE_TEST_SUITE(VssCommandProcessorTests, TestSuiteFixture)

///////////////////////////
// Test GET handling

BOOST_AUTO_TEST_CASE(Given_ValidGetQuery_When_PathNotValid_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  const VSSPath path2 = VSSPath::fromVSSGen1(path);
  
  // setup

  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "get", path2.getVSSPath(), jsonPathNotFound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(dbMock->getSignalNew)
    .once()
    .with(mock::any, mock::equal(path2) ,true)
    .returns(jsonPathNotFound);

  // run UUT
  auto resStr = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonPathNotFound["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonPathNotFound);
}


BOOST_AUTO_TEST_CASE(Given_ValidGetQuery_When_DBThrowsNotExpectedException_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonMalformedReq;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  VSSPath path2 = VSSPath::fromVSSGen1(path);


  // setup

  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::malFormedRequest(requestId, "get", "Unhandled error: std::exception", jsonMalformedReq);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(dbMock->getSignalNew)
    .once()
    .with(mock::any, path2, true)
    .throws(std::exception());

  // run UUT
  auto resStr = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify
  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonMalformedReq["timestamp"].as<int64_t>(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonMalformedReq);
}

BOOST_AUTO_TEST_CASE(Given_ValidGetQuery_When_UserNotAuthorized_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonNoAccess;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  VSSPath path2 = VSSPath::fromVSSGen1(path);

  // setup

  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::noAccess(requestId, "get", "No read access to " + path, jsonNoAccess);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(dbMock->getSignalNew)
    .with(mock::any, path2, true)
    .throws(noPermissionException("No read access to " + path));

  // run UUT
  auto resStr = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify
  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonNoAccess["timestamp"].as<int64_t>(); // ignoring timestamp difference for response


  BOOST_TEST(res == jsonNoAccess);
}

BOOST_AUTO_TEST_CASE(Given_ValidGetQuery_When_UserAuthorized_Shall_ReturnValue)
{
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonSignalValue;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  VSSPath path2 = VSSPath::fromVSSGen1(path);

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "get";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["value"] = 123;
  jsonSignalValue["timestamp"] = 11111111;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(dbMock->getSignalNew)
    .with(mock::any, path2, true)
    .returns(jsonSignalValue);

  // run UUT
  auto resStr = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonSignalValue["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonSignalValue);
}

BOOST_AUTO_TEST_CASE(Given_ValidGetQuery_When_NoValueFromDB_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  VSSPath path2 = VSSPath::fromVSSGen1(path);


  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "get";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["timestamp"] = 11111111;

  JsonResponses::pathNotFound(requestId, "get", path2.getVSSPath(), jsonPathNotFound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(dbMock->getSignalNew)
    .with(mock::any, path2, true)
    .returns(jsonSignalValue);

  // run UUT
  auto resStr = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonPathNotFound["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonPathNotFound);
}

///////////////////////////
// Test SET handling

BOOST_AUTO_TEST_CASE(Given_ValidSetQuery_When_InvalidPath_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "set", path, jsonPathNotFound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  MOCK_EXPECT(dbMock->setSignal)
    .with(mock::any, path, jsonValue)
    .throws(noPathFoundonTree(""));

  // run UUT
  auto resStr = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonPathNotFound["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Given_ValidSetQuery_When_ValueOutOfBound_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonValueOutOfBound;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "set";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["timestamp"] = 11111111;

  JsonResponses::valueOutOfBounds(requestId, "set", "", jsonValueOutOfBound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  MOCK_EXPECT(dbMock->setSignal)
    .with(mock::any, path, jsonValue)
    .throws(outOfBoundException(""));

  // run UUT
  auto resStr = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  jsonValueOutOfBound["timestamp"] = res["timestamp"].as<int64_t>(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonValueOutOfBound);
}

BOOST_AUTO_TEST_CASE(Given_ValidSetQuery_When_NoPermission_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonNoAccess;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "set";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["timestamp"] = 11111111;

  JsonResponses::noAccess(requestId, "set", "", jsonNoAccess);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  MOCK_EXPECT(dbMock->setSignal)
    .with(mock::any, path, jsonValue)
    .throws(noPermissionException(""));

  // run UUT
  auto resStr = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  jsonNoAccess["timestamp"] = res["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonNoAccess);
}

BOOST_AUTO_TEST_CASE(Given_ValidSetQuery_When_DBThrowsNotExpectedException_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonMalformedReq;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "set";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["timestamp"] = 11111111;

  JsonResponses::malFormedRequest(requestId, "get", "Unhandled error: std::exception", jsonMalformedReq);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  MOCK_EXPECT(dbMock->setSignal)
    .with(mock::any, path, jsonValue)
    .throws(std::exception());

  // run UUT
  auto resStr = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  // Set timestamp for comparision purposes
  jsonMalformedReq["timestamp"] = res["timestamp"].as<int64_t>();

  BOOST_TEST(res == jsonMalformedReq);
}

BOOST_AUTO_TEST_CASE(Given_ValidSetQuery_When_UserAuthorized_Shall_UpdateValue)
{
  WsChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonMalformedReq;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "set";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["timestamp"] = 11111111;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  MOCK_EXPECT(dbMock->setSignal)
    .with(mock::any, path, jsonValue);

  // run UUT
  auto resStr = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonSignalValue["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonSignalValue);
}

///////////////////////////
// Test SUBSCRIBE handling

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_UserAuthorized_Shall_ReturnSubscrId)
{
  WsChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonSignalValue;

  string requestId = "1";
  int subscriptionId = 123;
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "subscribe";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["timestamp"] = 11111111;
  jsonSignalValue["subscriptionId"] = subscriptionId;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path)
    .returns(subscriptionId);

  // run UUT
  auto resStr = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonSignalValue["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonSignalValue);
}

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_UserAuthorizedButSubIdZero_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonSignalValueErr;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  jsonSignalValueErr["number"] = 400;
  jsonSignalValueErr["reason"] = "Bad Request";
  jsonSignalValueErr["message"] = "Unknown";
  jsonSignalValue["action"] = "subscribe";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["timestamp"] = 11111111;
  jsonSignalValue["error"] = jsonSignalValueErr;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path)
    .returns(0);

  // run UUT
  auto resStr = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonSignalValue["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonSignalValue);
}

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_UserNotAuthorized_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonNoAccess;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  JsonResponses::noAccess(requestId, "subscribe", "", jsonNoAccess);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path)
    .throws(noPermissionException(""));

  // run UUT
  auto resStr = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonNoAccess["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonNoAccess);
}

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_PathNotValid_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "subscribe", path, jsonPathNotFound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path)
    .throws(noPathFoundonTree(path));

  // run UUT
  auto resStr = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonPathNotFound["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_OutOfBounds_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonOutOfBound;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  JsonResponses::valueOutOfBounds(requestId, "subscribe", path, jsonOutOfBound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path)
    .throws(genException(path));

  // run UUT
  auto resStr = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonOutOfBound["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonOutOfBound);
}

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_SubHandlerThrowsNotExpectedException_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonMalformedReq;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  JsonResponses::malFormedRequest(requestId, "get", "Unhandled error: std::exception", jsonMalformedReq);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path)
    .throws(std::exception());

  // run UUT
  auto resStr = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonMalformedReq["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonMalformedReq);
}

///////////////////////////
// Test UN-SUBSCRIBE handling

BOOST_AUTO_TEST_CASE(Given_ValidUnsubscribeQuery_When_UserAuthorized_Shall_Unsubscribe)
{
  WsChannel channel;

  jsoncons::json jsonUnsubscribeRequestForSignal;
  jsoncons::json jsonSignalValue;

  string requestId = "1";
  int subscriptionId = 123;

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonUnsubscribeRequestForSignal["action"] = "unsubscribe";
  jsonUnsubscribeRequestForSignal["subscriptionId"] = subscriptionId;
  jsonUnsubscribeRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "unsubscribe";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["timestamp"] = 11111111;
  jsonSignalValue["subscriptionId"] = subscriptionId;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->unsubscribe)
    .with(subscriptionId)
    .returns(0);

  // run UUT
  auto resStr = processor->processQuery(jsonUnsubscribeRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonSignalValue["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonSignalValue);
}

BOOST_AUTO_TEST_CASE(Given_ValidUnsubscribeQuery_When_Error_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonUnsubscribeRequestForSignal;
  jsoncons::json jsonSignalValue, jsonSignalValueErr;

  string requestId = "1";
  int subscriptionId = 123;

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonUnsubscribeRequestForSignal["action"] = "unsubscribe";
  jsonUnsubscribeRequestForSignal["subscriptionId"] = subscriptionId;
  jsonUnsubscribeRequestForSignal["requestId"] = requestId;

  jsonSignalValueErr["number"] = 400;
  jsonSignalValueErr["reason"] = "Unknown error";
  jsonSignalValueErr["message"] = "Error while unsubscribing";
  jsonSignalValue["action"] = "unsubscribe";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["timestamp"] = 11111111;
  jsonSignalValue["error"] = jsonSignalValueErr;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->unsubscribe)
    .with(subscriptionId)
    .returns(1);

  // run UUT
  auto resStr = processor->processQuery(jsonUnsubscribeRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonSignalValue["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonSignalValue);
}

///////////////////////////
// Test GET METADATA handling

BOOST_AUTO_TEST_CASE(Given_ValidGetMetadataQuery_When_UserAuthorized_Shall_GetMetadata)
{
  WsChannel channel;

  jsoncons::json jsonGetMetaRequest;
  jsoncons::json jsonValue, jsonMetadata;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonGetMetaRequest["action"] = "getMetaData";
  jsonGetMetaRequest["requestId"] = requestId;
  jsonGetMetaRequest["path"] = path;

  jsonMetadata = json::parse("{\"Vehicle\": {\"children\": { \"Drivetrain\": {\"children\": {\"Transmission\": { \"children\": {\"TravelledDistance\": {\"datatype\": \"float\",\"description\": \"Odometer reading\",\"type\": \"sensor\",\"unit\": \"km\",\"uuid\": \"d0e3b50d81a1521da0fbf5cbd1cab95c\"} }, \"description\": \"Transmission-specific data, stopping at the drive shafts.\", \"type\": \"branch\", \"uuid\": \"e198a8805b345c8c818558bc79b0ce25\"}},\"description\": \"Drivetrain data for internal combustion engines, transmissions, electric motors, etc.\",\"type\": \"branch\",\"uuid\": \"8876a6c501b658688843d3d5566e4963\" }},\"description\": \"High-level vehicle data.\",\"type\": \"branch\",\"uuid\": \"1c72453e738511e9b29ad46a6a4b77e9\"} }");

  jsonValue["action"] = "getMetaData";
  jsonValue["requestId"] = requestId;
  jsonValue["timestamp"] = 11111111;
  jsonValue["metadata"] = jsonMetadata;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(dbMock->getMetaData)
    .with(path)
    .returns(jsonMetadata);

  // run UUT
  auto resStr = processor->processQuery(jsonGetMetaRequest.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonValue["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonValue);
}

///////////////////////////
// Invalid AUTHORIZE handling

BOOST_AUTO_TEST_CASE(Given_ValidAuthJson_When_TokenValid_Shall_Authorize)
{
  WsChannel channel;

  jsoncons::json jsonAuthRequest;
  jsoncons::json jsonValue, jsonMetadata;

  string requestId = "1";
  int ttl = 5555;
  std::string dummyToken{"header.payload.signature"};

  // setup

  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonAuthRequest["action"] = "authorize";
  jsonAuthRequest["requestId"] = requestId;
  jsonAuthRequest["tokens"] = dummyToken;

  jsonValue["action"] = "authorize";
  jsonValue["requestId"] = requestId;
  jsonValue["timestamp"] = 11111111;
  jsonValue["TTL"] = ttl;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  
  MOCK_EXPECT(authMock->validate)
    .once()
    .with(mock::any, dummyToken)
    .returns(ttl);

  // run UUT
  auto resStr = processor->processQuery(jsonAuthRequest.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonValue["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonValue);
}

BOOST_AUTO_TEST_CASE(Given_ValidAuthJson_When_TokenInvalid_Shall_ReturnError)
{
  WsChannel channel;

  jsoncons::json jsonAuthRequest;
  jsoncons::json jsonValue, jsonValueErr;

  string requestId = "1";
  int ttl = -1;
  std::string dummyToken{"header.payload.signature"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonAuthRequest["action"] = "authorize";
  jsonAuthRequest["requestId"] = requestId;
  jsonAuthRequest["tokens"] = dummyToken;

  jsonValueErr["number"] = 401;
  jsonValueErr["reason"] = "Invalid Token";
  jsonValueErr["message"] = "Check the JWT token passed";
  jsonValue["action"] = "authorize";
  jsonValue["requestId"] = requestId;
  jsonValue["timestamp"] = 11111111;
  jsonValue["error"] = jsonValueErr;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  
  MOCK_EXPECT(authMock->validate)
    .once()
    .with(mock::any, dummyToken)
    .returns(ttl);

  // run UUT
  auto resStr = processor->processQuery(jsonAuthRequest.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonValue["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonValue);
}

///////////////////////////
// Invalid JSON handling

BOOST_AUTO_TEST_CASE(Given_JsonStrings_When_processQuery_Shall_HandleCorrectlyErrors)
{
  WsChannel channel;

  jsoncons::json jsonRequest;
  jsoncons::json jsonExpected, jsonValueErr;

  std::string path{"Signal.OBD.DTC1"};

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  jsonValueErr["number"] = 400;
  jsonValueErr["reason"] = "Bad Request";
  jsonValueErr["message"] = "Key not found: 'action'";
  jsonExpected["error"] = jsonValueErr;
  jsonExpected["timestamp"] = 123;

  //////////////////////
  // check empty request
  auto resStr = processor->processQuery(jsonRequest.as_string(), channel);
  auto res = json::parse(resStr);

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonExpected["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check request with only action
  jsonRequest["action"] = "notSupportedAction";
  resStr = processor->processQuery(jsonRequest.as_string(), channel);
  res = json::parse(resStr);

  jsonValueErr["message"] = "Key not found: 'path'";
  jsonExpected["error"] = jsonValueErr;

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonExpected["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check request with added path
  jsonRequest["path"] = path;
  resStr = processor->processQuery(jsonRequest.as_string(), channel);
  res = json::parse(resStr);

  jsonValueErr["message"] = "Key not found: 'requestId'";
  jsonExpected["error"] = jsonValueErr;

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonExpected["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check request with added requestId
  jsonRequest["requestId"] = 1;
  resStr = processor->processQuery(jsonRequest.as_string(), channel);
  res = json::parse(resStr);

  jsonValueErr["message"] = "Unknown action requested";
  jsonExpected["error"] = jsonValueErr;

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonExpected["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check random string as request
  resStr = processor->processQuery("random string P{ }", channel);
  res = json::parse(resStr);

  jsonValueErr["message"] = "";
  jsonExpected["error"] = jsonValueErr;

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonExpected["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  res["error"] = jsonExpected["error"].as<json>();     // ignoring error content
  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check empty JSON
  resStr = processor->processQuery("{ }", channel);
  res = json::parse(resStr);

  jsonValueErr["message"] = "";
  jsonExpected["error"] = jsonValueErr;

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonExpected["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  res["error"] = jsonExpected["error"].as<json>();     // ignoring error content
  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check malformed JSON
  resStr = processor->processQuery("{ \"action\": asdasd, }", channel);
  res = json::parse(resStr);

  jsonValueErr["message"] = "";
  jsonExpected["error"] = jsonValueErr;

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] = jsonExpected["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  res["error"] = jsonExpected["error"].as<json>();     // ignoring error content
  BOOST_TEST(res == jsonExpected);
}

BOOST_AUTO_TEST_SUITE_END()
