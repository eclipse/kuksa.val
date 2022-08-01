/**********************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Contributors:
 *      Robert Bosch GmbH
 **********************************************************************/

#include <boost/test/unit_test.hpp>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <turtle/mock.hpp>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS


#include <memory>
#include <string>

#include <boost/uuid/uuid.hpp>            
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "KuksaChannel.hpp"
#include "ILoggerMock.hpp"
#include "IVssDatabaseMock.hpp"
#include "IAuthenticatorMock.hpp"
#include "ISubscriptionHandlerMock.hpp"
#include "kuksa.pb.h"

#include "IAccessCheckerMock.hpp"

#include "VSSPath.hpp"

#include "exception.hpp"
#include "JsonResponses.hpp"
#include "VssCommandProcessor.hpp"
#include "UnitTestHelpers.hpp" 

namespace {
  // common resources for tests
  std::shared_ptr<ILoggerMock> logMock;
  std::shared_ptr<IVssDatabaseMock> dbMock;
  std::shared_ptr<IAuthenticatorMock> authMock;
  std::shared_ptr<IAccessCheckerMock> accCheckMock;

  std::shared_ptr<ISubscriptionHandlerMock> subsHndlMock;

  std::unique_ptr<VssCommandProcessor> processor;

  // Pre-test initialization and post-test desctruction of common resources
  struct TestSuiteFixture {
    TestSuiteFixture() {
      logMock = std::make_shared<ILoggerMock>();
      dbMock = std::make_shared<IVssDatabaseMock>();
      authMock = std::make_shared<IAuthenticatorMock>();
      subsHndlMock = std::make_shared<ISubscriptionHandlerMock>();
      //real auth checker, becasue this test module has been written before this could be mocked
      accCheckMock = std::make_shared<IAccessCheckerMock>();

      processor = std::make_unique<VssCommandProcessor>(logMock, dbMock, authMock, accCheckMock, subsHndlMock);
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
  KuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  const VSSPath path2 = VSSPath::fromVSSGen1(path);
  
  // setup

  channel.setAuthorized(false);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "get", path2.getVSSGen1Path(), jsonPathNotFound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(dbMock->getLeafPaths)
    .once()
    .with(mock::equal(path2))
    .returns(std::list<VSSPath>());

  // run UUT
  jsoncons::json res = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  // verify
  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  res["ts"]=jsonPathNotFound["ts"].as_string(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonPathNotFound);
}


BOOST_AUTO_TEST_CASE(Given_ValidGetQuery_When_DBThrowsNotExpectedException_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonMalformedReq;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  VSSPath path2 = VSSPath::fromVSSGen1(path);


  // setup

  channel.setAuthorized(false);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);
  channel.setType(KuksaChannel::Type::WEBSOCKET_PLAIN);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::malFormedRequest(requestId, "get", "Unhandled error: std::exception", jsonMalformedReq);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(dbMock->getLeafPaths)
    .once()
    .with(mock::equal(path2))
    .returns(std::list<VSSPath>{path2});
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .once()
    .with(mock::any, mock::equal(path2))
    .returns(true);
  MOCK_EXPECT(dbMock->pathIsAttributable)
    .once()
    .with(path2, "value")
    .returns(true);  
  MOCK_EXPECT(dbMock->getSignal)
    .once()
    .with(path2, "value", true)
    .throws(std::exception());

  // run UUT
  auto res = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  // verify
  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonMalformedReq["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response


  BOOST_TEST(res == jsonMalformedReq);
}

BOOST_AUTO_TEST_CASE(Given_ValidGetQuery_When_UserNotAuthorized_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonNoAccess;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  VSSPath path2 = VSSPath::fromVSSGen1(path);

  // setup

  channel.setAuthorized(false);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::noAccess(requestId, "get", "Insufficient read access to " + path, jsonNoAccess);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );
  MOCK_EXPECT(dbMock->getLeafPaths)
    .once()
    .with(mock::equal(path2))
    .returns(std::list<VSSPath>{path2});
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .once()
    .with(mock::any, mock::equal(path2))
    .returns(false);

  MOCK_EXPECT(dbMock->getSignal)
    .with(path2, "value", true)
    .throws(noPermissionException("Insufficient read access to " + path));

  // run UUT
  auto res = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  // verify
  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonNoAccess["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response


  BOOST_TEST(res == jsonNoAccess);
}

BOOST_AUTO_TEST_CASE(Given_ValidGetQuery_When_UserAuthorized_Shall_ReturnValue)
{
  KuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonSignalData;
  jsoncons::json jsonSignalDataPoint;
  jsoncons::json jsonGetResponseForSignal;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  VSSPath path2 = VSSPath::fromVSSGen1(path);

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  jsonSignalData["path"] = path;
  jsonSignalDataPoint["value"] = 123;
  jsonSignalDataPoint["ts"] = "1970-01-01T00:00:00.0Z";
  jsonSignalData["dp"] = jsonSignalDataPoint;

  jsonGetResponseForSignal["action"] = "get";
  jsonGetResponseForSignal["requestId"] = requestId;
  jsonGetResponseForSignal["data"] = jsonSignalData;


  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );
  MOCK_EXPECT(dbMock->getLeafPaths)
    .once()
    .with(mock::equal(path2))
    .returns(std::list<VSSPath>{path2});
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .once()
    .with(mock::any, mock::equal(path2))
    .returns(true);
  MOCK_EXPECT(dbMock->pathIsAttributable)
    .once()
    .with(path2, "value")
    .returns(true); 
  MOCK_EXPECT(dbMock->getSignal)
    .with(path2, "value", true)
    .returns(jsonSignalData);

  // run UUT
  auto res = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  // verify
  verify_and_erase_timestamp(res);

  BOOST_TEST(res == jsonGetResponseForSignal);
}

BOOST_AUTO_TEST_CASE(Given_ValidGetQuery_When_NoValueFromDB_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  VSSPath path2 = VSSPath::fromVSS(path);


  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "get";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["ts"] = 11111111;

  JsonResponses::pathNotFound(requestId, "get", path2.getVSSGen1Path(), jsonPathNotFound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(dbMock->getLeafPaths)
    .once()
    .with(mock::equal(path2))
    .returns(std::list<VSSPath>{});

  // run UUT
  auto res = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonPathNotFound["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonPathNotFound);
}

///////////////////////////
// Test SET handling

BOOST_AUTO_TEST_CASE(Given_ValidSetQuery_When_InvalidPath_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Vehicle.OBD.DTC1"};
  VSSPath vsspath = VSSPath::fromVSSGen1(path);
  
  // setup
  //We need permission first, (otherwise get 403 before checking for invalid path)
  std::string perm = "{\"Vehicle.OBD.*\" : \"wr\"}";

  channel.setPermissions(perm);

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "set", vsspath.getVSSGen1Path(), jsonPathNotFound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );
  
  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  MOCK_EXPECT(dbMock->pathExists)
    .with(vsspath).returns(false);

  // run UUT
  auto res = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonPathNotFound["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Given_ValidSetQuery_When_ValueOutOfBound_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonValueOutOfBound;

   // setup
  //We need permission first, (otherwise get 403 before checking for invalid path)
  std::string perm = "{\"Vehicle.OBD.*\" : \"wr\"}";
  channel.setPermissions(perm);

  string requestId = "1";
  int requestValue = 300; //OoB for uint8
  std::string path{"Vehicle.OBD.ShortTermO2Trim2"};
  VSSPath vsspath = VSSPath::fromVSSGen1(path);

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "set";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["ts"] = 11111111;

  JsonResponses::valueOutOfBounds(requestId, "set", "", jsonValueOutOfBound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  MOCK_EXPECT(dbMock->pathExists).with(vsspath).returns(true);
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
    .once()
    .with(mock::any, mock::equal(vsspath))
    .returns(true);
  MOCK_EXPECT(dbMock->pathIsWritable).with(vsspath).returns(true);
  MOCK_EXPECT(dbMock->pathIsAttributable)
    .once()
    .with(vsspath, "value")
    .returns(true); 
  MOCK_EXPECT(dbMock->setSignal)
    .with(vsspath, "value", jsonValue)
    .throws(outOfBoundException(""));

  // run UUT
  auto res = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonValueOutOfBound["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonValueOutOfBound);
}

BOOST_AUTO_TEST_CASE(Given_ValidSetQuery_When_NoPermission_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonNoAccess;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Signal.OBD.DTC1"};
  VSSPath vsspath = VSSPath::fromVSSGen1(path);

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "set";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["ts"] = 11111111;

  JsonResponses::noAccess(requestId, "set", "No write access to Signal.OBD.DTC1", jsonNoAccess);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );
  MOCK_EXPECT(dbMock->pathExists).with(vsspath).returns(true);
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
    .once()
    .with(mock::any, mock::equal(vsspath))
    .returns(false);

  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  MOCK_EXPECT(dbMock->setSignal)
    .with(vsspath, "value", jsonValue)
    .throws(noPermissionException(""));

  // run UUT
  auto res = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonNoAccess["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonNoAccess);
}

BOOST_AUTO_TEST_CASE(Given_ValidSetQuery_When_DBThrowsNotExpectedException_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonMalformedReq;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Vehicle.OBD.Speed"};
  VSSPath vsspath = VSSPath::fromVSSGen1(path);

  // setup
  //We need permission first, (otherwise get 403 before checking for invalid path)
  std::string perm = "{\"Vehicle.OBD.*\" : \"wr\"}";
  channel.setPermissions(perm);
  channel.setAuthorized(true);
  channel.setConnID(1);
    channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "set";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["ts"] = 11111111;

  JsonResponses::malFormedRequest(requestId, "set", "Unhandled error: std::exception", jsonMalformedReq);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );
  MOCK_EXPECT(dbMock->pathExists).with(vsspath).returns(true);
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
    .once()
    .with(mock::any, mock::equal(vsspath))
    .returns(true);
  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  MOCK_EXPECT(dbMock->pathIsWritable).with(vsspath).returns(true);
  MOCK_EXPECT(dbMock->pathIsAttributable)
    .once()
    .with(vsspath, "value")
    .returns(true); 
  MOCK_EXPECT(dbMock->setSignal)
    .with(vsspath, "value", jsonValue)
    .throws(std::exception());

  // run UUT
  auto res = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonMalformedReq["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonMalformedReq);
}

BOOST_AUTO_TEST_CASE(Given_ValidSetQuery_When_UserAuthorized_Shall_UpdateValue)
{
  KuksaChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonMalformedReq;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Vehicle.OBD.DTC1"};
  VSSPath vsspath = VSSPath::fromVSSGen1(path);

  // setup
  //We need permission first, (otherwise get 403 before checking for invalid path)
  std::string perm = "{\"Vehicle.OBD.*\" : \"wr\"}";
  channel.setPermissions(perm);
  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "set";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["ts"] = 11111111;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  //as db is mocked, this test basically only checks if the command proccesor routes the query accordingly
  MOCK_EXPECT(dbMock->pathExists).with(vsspath).returns(true);
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
    .once()
    .with(mock::any, mock::equal(vsspath))
    .returns(true);
  MOCK_EXPECT(dbMock->pathIsWritable).with(vsspath).returns(true);
  MOCK_EXPECT(dbMock->pathIsAttributable)
    .once()
    .with(vsspath, "value")
    .returns(true); 
  MOCK_EXPECT(dbMock->setSignal)
    .with(vsspath, "value", mock::any).returns(jsonSignalValue);

  // run UUT
  auto res = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonSignalValue["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonSignalValue);
}

///////////////////////////
// Test SET Target Value handling

BOOST_AUTO_TEST_CASE(Given_ValidSetTargetValueQuery_When_InvalidPath_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Vehicle.OBD.DTC1"};
  VSSPath vsspath = VSSPath::fromVSSGen1(path);
  
  // setup
  //We need permission first, (otherwise get 403 before checking for invalid path)
  std::string perm = "{\"Vehicle.OBD.*\" : \"wr\"}";

  channel.setPermissions(perm);

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["value"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "set", vsspath.getVSSGen1Path(), jsonPathNotFound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );
  
  jsoncons::json jsonValue = jsonSetRequestForSignal["value"];
  MOCK_EXPECT(dbMock->pathExists)
    .with(vsspath).returns(false);

  // run UUT
  auto res = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonPathNotFound["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Given_ValidSetTargetValueQuery_When_Sensor_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonNoAccess;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Vehicle.Acceleration.Lateral"};
  VSSPath vsspath = VSSPath::fromVSSGen1(path);
  
  // setup
  //We need permission first, (otherwise get 403 before checking for invalid path)
  std::string perm = "{\"Vehicle.*\" : \"wr\"}";

  channel.setPermissions(perm);

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["attribute"] = "targetValue";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["targetValue"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  // expectations
  JsonResponses::noAccess(requestId, "set", "Can not set Vehicle.Acceleration.Lateral. Only sensor or actor leaves can be set.", jsonNoAccess);

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );
  MOCK_EXPECT(dbMock->pathExists).with(vsspath).returns(true);
  MOCK_EXPECT(dbMock->pathIsWritable)
    .with(vsspath).returns(false);
  MOCK_EXPECT(dbMock->pathIsAttributable)
    .with(vsspath, "targetValue").returns(false);
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
    .once()
    .with(mock::any, mock::equal(vsspath))
    .returns(true);
  
  jsoncons::json jsonValue = jsonSetRequestForSignal["targetValue"];

  // run UUT
  auto res = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonNoAccess["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonNoAccess);
}

BOOST_AUTO_TEST_CASE(Given_ValidSetTargetValueQuery_When_Actor_Shall_Work)
{
  KuksaChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonGetResponseForSignal;

  string requestId = "1";
  int requestValue = 123;
  std::string path{"Vehicle.Acceleration.Lateral"};
  VSSPath vsspath = VSSPath::fromVSSGen1(path);
  
  // setup
  //We need permission first, (otherwise get 403 before checking for invalid path)
  std::string perm = "{\"Vehicle.*\" : \"wr\"}";

  channel.setPermissions(perm);

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["attribute"] = "targetValue";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["targetValue"] = requestValue;
  jsonSetRequestForSignal["requestId"] = requestId;

  // expectations
  jsonGetResponseForSignal["action"] = "set";
  jsonGetResponseForSignal["requestId"] = requestId;

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );
  MOCK_EXPECT(dbMock->setSignal).with(vsspath, "targetValue", 123).returns(jsonGetResponseForSignal);
  MOCK_EXPECT(dbMock->pathExists).with(vsspath).returns(true);
  MOCK_EXPECT(dbMock->pathIsWritable)
    .with(vsspath).returns(true);
  MOCK_EXPECT(dbMock->pathIsAttributable)
    .with(vsspath, "targetValue").returns(true);
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
    .once()
    .with(mock::any, mock::equal(vsspath))
    .returns(true);
  
  jsoncons::json jsonValue = jsonSetRequestForSignal["targetValue"];

  // run UUT
  auto res = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonGetResponseForSignal["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonGetResponseForSignal);
}

///////////////////////////
// Test SUBSCRIBE handling

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_UserAuthorized_Shall_ReturnSubscrId)
{
  KuksaChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonSignalValue;

  string requestId = "1";
  boost::uuids::uuid subscriptionId;
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "subscribe";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["ts"] = 11111111;
  jsonSignalValue["subscriptionId"] = boost::uuids::to_string(subscriptionId);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path, "value")
    .returns(subscriptionId);

  // run UUT
  auto res = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);


  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonSignalValue["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonSignalValue);
}

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_UserAuthorizedButSubIdZero_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonSignalValue;
  jsoncons::json jsonSignalValueErr;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  jsonSignalValueErr["number"] = "404";
  jsonSignalValueErr["reason"] = "Path not found";
  jsonSignalValueErr["message"] = "I can not find Signal.OBD.DTC1 in my db";
  jsonSignalValue["action"] = "subscribe";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["ts"] = 11111111;
  jsonSignalValue["error"] = jsonSignalValueErr;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path, "value")
    .throws(noPathFoundonTree(path));

  // run UUT
  auto res = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);


  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonSignalValue["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response

  BOOST_TEST(res == jsonSignalValue);
}

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_UserNotAuthorized_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonNoAccess;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  JsonResponses::noAccess(requestId, "subscribe", "", jsonNoAccess);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path, "value")
    .throws(noPermissionException(""));

  // run UUT
  auto res = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonNoAccess["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonNoAccess);
}

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_PathNotValid_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "subscribe", path, jsonPathNotFound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path, "value")
    .throws(noPathFoundonTree(path));

  // run UUT
  auto res = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);


  // verify
  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonPathNotFound["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 
  
  BOOST_TEST(res == jsonPathNotFound);
  
  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_OutOfBounds_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonOutOfBound;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  JsonResponses::valueOutOfBounds(requestId, "subscribe", path, jsonOutOfBound);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path, "value")
    .throws(genException(path));

  // run UUT
  auto res = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);


  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonOutOfBound["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 
  
  BOOST_TEST(res == jsonOutOfBound);
}

BOOST_AUTO_TEST_CASE(Given_ValidSubscribeQuery_When_SubHandlerThrowsNotExpectedException_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonSubscribeRequestForSignal;
  jsoncons::json jsonMalformedReq;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonSubscribeRequestForSignal["action"] = "subscribe";
  jsonSubscribeRequestForSignal["path"] = path;
  jsonSubscribeRequestForSignal["requestId"] = requestId;

  JsonResponses::malFormedRequest(requestId, "get", "Unhandled error: std::exception", jsonMalformedReq);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->subscribe)
    .with(mock::any, dbMock, path, "value")
    .throws(std::exception());

  // run UUT
  auto res = processor->processQuery(jsonSubscribeRequestForSignal.as_string(), channel);


  // verify

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonMalformedReq["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonMalformedReq);
}

///////////////////////////
// Test UN-SUBSCRIBE handling

BOOST_AUTO_TEST_CASE(Given_ValidUnsubscribeQuery_When_UserAuthorized_Shall_Unsubscribe)
{
  KuksaChannel channel;

  jsoncons::json jsonUnsubscribeRequestForSignal;
  jsoncons::json jsonSignalValue;

  string requestId = "1";
  boost::uuids::uuid subscriptionId;

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonUnsubscribeRequestForSignal["action"] = "unsubscribe";
  jsonUnsubscribeRequestForSignal["subscriptionId"] = boost::uuids::to_string(subscriptionId);
  jsonUnsubscribeRequestForSignal["requestId"] = requestId;

  jsonSignalValue["action"] = "unsubscribe";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["ts"] = 11111111;
  jsonSignalValue["subscriptionId"] = boost::uuids::to_string(subscriptionId);

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->unsubscribe)
    .with(subscriptionId)
    .returns(0);

  // run UUT
  auto res = processor->processQuery(jsonUnsubscribeRequestForSignal.as_string(), channel);


  // verify
  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonSignalValue["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonSignalValue);
}

BOOST_AUTO_TEST_CASE(Given_ValidUnsubscribeQuery_When_Error_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonUnsubscribeRequestForSignal;
  jsoncons::json jsonSignalValue, jsonSignalValueErr;

  string requestId = "1";
  boost::uuids::uuid subscriptionId;

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonUnsubscribeRequestForSignal["action"] = "unsubscribe";
  jsonUnsubscribeRequestForSignal["subscriptionId"] = boost::uuids::to_string(subscriptionId);
  jsonUnsubscribeRequestForSignal["requestId"] = requestId;

  jsonSignalValueErr["number"] = "400";
  jsonSignalValueErr["reason"] = "Unknown error";
  jsonSignalValueErr["message"] = "Error while unsubscribing";
  jsonSignalValue["action"] = "unsubscribe";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["ts"] = 11111111;
  jsonSignalValue["error"] = jsonSignalValueErr;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->unsubscribe)
    .with(subscriptionId)
    .returns(1);

  // run UUT
  auto res = processor->processQuery(jsonUnsubscribeRequestForSignal.as_string(), channel);


  // verify
  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonSignalValue["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonSignalValue);
}

BOOST_AUTO_TEST_CASE(Given_MalformedUUD_In_Unsubscribe)
{
  KuksaChannel channel;

  jsoncons::json jsonUnsubscribeRequestForSignal;
  jsoncons::json jsonSignalValue, jsonSignalValueErr;

  string requestId = "1";
  boost::uuids::uuid subscriptionId;

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonUnsubscribeRequestForSignal["action"] = "unsubscribe";
  jsonUnsubscribeRequestForSignal["subscriptionId"] = "definitely-not-an-uuid";
  jsonUnsubscribeRequestForSignal["requestId"] = requestId;

  jsonSignalValueErr["number"] = "400";
  jsonSignalValueErr["reason"] = "Bad Request";
  jsonSignalValueErr["message"] = "Subscription ID is not a UUID: invalid uuid string";
  jsonSignalValue["action"] = "unsubscribe";
  jsonSignalValue["requestId"] = requestId;
  jsonSignalValue["ts"] = 11111111;
  jsonSignalValue["error"] = jsonSignalValueErr;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(subsHndlMock->unsubscribe)
    .with(subscriptionId)
    .returns(1);

  // run UUT
  auto res = processor->processQuery(jsonUnsubscribeRequestForSignal.as_string(), channel);


  // verify
  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonSignalValue["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonSignalValue);
}

///////////////////////////
// Test GET METADATA handling

BOOST_AUTO_TEST_CASE(Given_ValidGetMetadataQuery_When_UserAuthorized_Shall_GetMetadata)
{
  KuksaChannel channel;

  jsoncons::json jsonGetMetaRequest;
  jsoncons::json jsonValue, jsonMetadata;

  string requestId = "1";
  std::string path{"Signal.OBD.DTC1"};
  const VSSPath vssPath = VSSPath::fromVSSGen1(path);

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonGetMetaRequest["action"] = "getMetaData";
  jsonGetMetaRequest["requestId"] = requestId;
  jsonGetMetaRequest["path"] = path;

  jsonMetadata = json::parse("{\"Vehicle\": {\"children\": { \"Drivetrain\": {\"children\": {\"Transmission\": { \"children\": {\"TravelledDistance\": {\"datatype\": \"float\",\"description\": \"Odometer reading\",\"type\": \"sensor\",\"unit\": \"km\",\"uuid\": \"d0e3b50d81a1521da0fbf5cbd1cab95c\"} }, \"description\": \"Transmission-specific data, stopping at the drive shafts.\", \"type\": \"branch\", \"uuid\": \"e198a8805b345c8c818558bc79b0ce25\"}},\"description\": \"Drivetrain data for internal combustion engines, transmissions, electric motors, etc.\",\"type\": \"branch\",\"uuid\": \"8876a6c501b658688843d3d5566e4963\" }},\"description\": \"High-level vehicle data.\",\"type\": \"branch\",\"uuid\": \"1c72453e738511e9b29ad46a6a4b77e9\"} }");

  jsonValue["action"] = "getMetaData";
  jsonValue["requestId"] = requestId;
  jsonValue["ts"] = 11111111;
  jsonValue["metadata"] = jsonMetadata;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  MOCK_EXPECT(dbMock->getMetaData)
    .with(vssPath)
    .returns(jsonMetadata);

  // run UUT
  auto res = processor->processQuery(jsonGetMetaRequest.as_string(), channel);


  // verify
  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonValue["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonValue);
}

///////////////////////////
// Invalid AUTHORIZE handling

BOOST_AUTO_TEST_CASE(Given_ValidAuthJson_When_TokenValid_Shall_Authorize)
{
  KuksaChannel channel;

  jsoncons::json jsonAuthRequest;
  jsoncons::json jsonValue, jsonMetadata;

  string requestId = "1";
  int ttl = 5555;
  std::string dummyToken{"header.payload.signature"};

  // setup

  channel.setAuthorized(false);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonAuthRequest["action"] = "authorize";
  jsonAuthRequest["requestId"] = requestId;
  jsonAuthRequest["tokens"] = dummyToken;

  jsonValue["action"] = "authorize";
  jsonValue["requestId"] = requestId;
  jsonValue["ts"] = 11111111;
  jsonValue["TTL"] = ttl;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  
  MOCK_EXPECT(authMock->validate)
    .once()
    .with(mock::any, dummyToken)
    .returns(ttl);

  // run UUT
  auto res = processor->processQuery(jsonAuthRequest.as_string(), channel);


  // verify
  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonValue["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonValue);
}

BOOST_AUTO_TEST_CASE(Given_ValidAuthJson_When_TokenInvalid_Shall_ReturnError)
{
  KuksaChannel channel;

  jsoncons::json jsonAuthRequest;
  jsoncons::json jsonValue, jsonValueErr;

  string requestId = "1";
  int ttl = -1;
  std::string dummyToken{"header.payload.signature"};

  // setup

  channel.setAuthorized(true);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_SSL);

  jsonAuthRequest["action"] = "authorize";
  jsonAuthRequest["requestId"] = requestId;
  jsonAuthRequest["tokens"] = dummyToken;

  jsonValueErr["number"] = "401";
  jsonValueErr["reason"] = "Invalid Token";
  jsonValueErr["message"] = "Check the JWT token passed";
  jsonValue["action"] = "authorize";
  jsonValue["requestId"] = requestId;
  jsonValue["ts"] = 11111111;
  jsonValue["error"] = jsonValueErr;

  // expectations

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  
  MOCK_EXPECT(authMock->validate)
    .once()
    .with(mock::any, dummyToken)
    .returns(ttl);

  // run UUT
  auto res = processor->processQuery(jsonAuthRequest.as_string(), channel);


  // verify
  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonValue["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonValue);
}

///////////////////////////
// Invalid JSON handling

BOOST_AUTO_TEST_CASE(Given_JsonStrings_When_processQuery_Shall_HandleCorrectlyErrors)
{
  KuksaChannel channel;

  jsoncons::json jsonRequest;
  jsoncons::json jsonExpected, jsonValueErr;

  std::string path{"Signal.OBD.DTC1"};

  // validate that at least one log event was processed
  MOCK_EXPECT(logMock->Log).at_least( 1 );

  jsonValueErr["number"] = "400";
  jsonValueErr["reason"] = "Bad Request";
  jsonValueErr["message"] = "Key not found: 'action'";

  jsonExpected["error"] = jsonValueErr;
  jsonExpected["ts"] = 123;
  jsonExpected["requestId"] = "UNKNOWN";


  //////////////////////
  // check empty request
  auto res = processor->processQuery(jsonRequest.as_string(), channel);


  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonExpected["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check request with only get action
  jsonRequest["action"] = "get";
  res = processor->processQuery(jsonRequest.as_string(), channel);
  
  jsonValueErr["message"] = "Key not found: 'path'";
  jsonExpected["error"] = jsonValueErr;

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonExpected["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check request with added path
  jsonRequest["path"] = path;
  res = processor->processQuery(jsonRequest.as_string(), channel);

  jsonValueErr["message"] = "Schema error: #: Required property \"requestId\" not found";

  jsonExpected["requestId"] = "UNKNOWN";
  jsonExpected["error"] = jsonValueErr;
  jsonExpected["action"] = "get";


  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonExpected["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 
  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check request with added requestId, but illegal action
  jsonRequest["requestId"] = 1;
  jsonRequest["action"] = "nothing";

  res = processor->processQuery(jsonRequest.as_string(), channel);

  jsonValueErr["message"] = "Unknown action requested";
  jsonExpected["error"] = jsonValueErr;
  jsonExpected.erase("action");
  jsonExpected["requestId"] = "1";


  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonExpected["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  BOOST_TEST(res == jsonExpected);


  //////////////////////
  /// check random string as request
  res = processor->processQuery("random string P{ }", channel);

  jsonValueErr["message"] = "";
  jsonExpected["error"] = jsonValueErr;
  jsonExpected["requestId"] = "UNKNOWN";


  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonExpected["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 
  
  res["error"] = jsonExpected["error"].as<json>();     // ignoring error content
  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check empty JSON
  res = processor->processQuery("{ }", channel);

  jsonValueErr["message"] = "";
  jsonExpected["error"] = jsonValueErr;

    BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonExpected["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 
  
  res["error"] = jsonExpected["error"].as<json>();     // ignoring error content
  BOOST_TEST(res == jsonExpected);

  //////////////////////
  /// check malformed JSON
  res = processor->processQuery("{ \"action\": asdasd, }", channel);

  jsonValueErr["message"] = "";
  jsonExpected["error"] = jsonValueErr;

  BOOST_TEST(res.contains("ts"));
  BOOST_TEST(res["ts"].is_string());
  jsonExpected["ts"]=res["ts"].as_string(); // ignoring timestamp difference for response 

  res["error"] = jsonExpected["error"].as<json>();     // ignoring error content
  BOOST_TEST(res == jsonExpected);
}

BOOST_AUTO_TEST_SUITE_END()
