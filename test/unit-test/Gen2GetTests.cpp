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
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <turtle/mock.hpp>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <chrono>
#include <thread>

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
std::shared_ptr<IAuthenticatorMock> authMock;
std::shared_ptr<IAccessCheckerMock> accCheckMock;

std::shared_ptr<ISubscriptionHandlerMock> subHandlerMock;

std::shared_ptr<VssDatabase> db;

std::unique_ptr<VssCommandProcessor> processor;

// Pre-test initialization and post-test desctruction of common resources
struct TestSuiteFixture {
  TestSuiteFixture() {
    logMock = std::make_shared<ILoggerMock>();
    authMock = std::make_shared<IAuthenticatorMock>();

    accCheckMock = std::make_shared<IAccessCheckerMock>();
    subHandlerMock = std::make_shared<ISubscriptionHandlerMock>();

    std::string vss_file{"test_vss_rel_2.0.json"};

    MOCK_EXPECT(logMock->Log).at_least(0);  // ignore log events
    db = std::make_shared<VssDatabase>(logMock, subHandlerMock);
    db->initJsonTree(vss_file);

    processor = std::make_unique<VssCommandProcessor>(
        logMock, db, authMock, accCheckMock, subHandlerMock);
  }
  ~TestSuiteFixture() {
    logMock.reset();
    accCheckMock.reset();
    authMock.reset();
    subHandlerMock.reset();
    processor.reset();
  }
};
}  // namespace

// Define name of test suite and define test suite fixture for pre and post test
// handling
BOOST_FIXTURE_TEST_SUITE(Gen2GetTests, TestSuiteFixture);

BOOST_AUTO_TEST_CASE(Gen2_Get_Sensor) {
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/Speed"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
      {
    "action": "get", 
    "path": "Vehicle/Speed", 
    "requestId": "1", 
    "value": "---"
    }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // Read access has been checked
  MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vss_path)
      .returns(true);

  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("timestamp"));

  // Assign timestamp for comparision purposes
  expectedJson.insert_or_assign("timestamp",res["timestamp"]);

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Invalid_JSON) {
  WsChannel channel;
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsoncons::json jsonSetRequestForSignal;

  std::string path{"Vehicle/Speed"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  jsonSetRequestForSignal["action"] = "get";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["requestId"] = 100;  // int is invalid here;

  std::string expectedJsonString{R"(
      {
  "action": "get",
  "error": {
    "message": "Schema error: VSS get malformed: #/requestId: Expected string, found uint64",
    "number": 400,
    "reason": "Bad Request"
  },
  "requestId": "100",
  "timestamp": 0
}
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // run UUT
  auto resStr =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);

  // Remove timestamp for comparision purposes
  expectedJson["timestamp"] = res["timestamp"].as<int64_t>();

  BOOST_TEST(res == expectedJson);
}

/** Send an invalid JSON, without any determinable Request Id */
BOOST_AUTO_TEST_CASE(Gen2_Get_Invalid_JSON_NoRequestID) {
  WsChannel channel;
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsoncons::json jsonSetRequestForSignal;

  jsonSetRequestForSignal["action"] = "get";
  jsonSetRequestForSignal["path"] = 999;  // int as path is wrong

  std::string expectedJsonString{R"(
  {
  "action": "get",
  "error": {
    "message": "Schema error: VSS get malformed: #: Required property \"requestId\" not found\n#/path: Expected string, found uint64",
    "number": 400,
    "reason": "Bad Request"
  },
  "requestId": "UNKNOWN",
  "timestamp": 0
}
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // run UUT
  auto resStr =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);

  // Remove timestamp for comparision purposes
  expectedJson["timestamp"] = res["timestamp"].as<int64_t>();

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_NonExistingPath) {
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/OBD/FluxCapacitorCharge"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "get", path, jsonPathNotFound);

  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] =
      jsonPathNotFound["timestamp"]
          .as<int64_t>();  // ignoring timestamp difference for response
  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Branch) {
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/VehicleIdentification"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
    {
    "action": "get", 
    "requestId": "1", 
    "value": [{"Vehicle/VehicleIdentification/ACRISSCode":"---"},{"Vehicle/VehicleIdentification/Brand":"---"},{"Vehicle/VehicleIdentification/Model":"---"},{"Vehicle/VehicleIdentification/VIN":"---"},{"Vehicle/VehicleIdentification/WMI":"---"},{"Vehicle/VehicleIdentification/Year":"---"},{"Vehicle/VehicleIdentification/bodyType":"---"},{"Vehicle/VehicleIdentification/dateVehicleFirstRegistered":"---"},{"Vehicle/VehicleIdentification/knownVehicleDamages":"---"},{"Vehicle/VehicleIdentification/meetsEmissionStandard":"---"},{"Vehicle/VehicleIdentification/productionDate":"---"},{"Vehicle/VehicleIdentification/purchaseDate":"---"},{"Vehicle/VehicleIdentification/vehicleConfiguration":"---"},{"Vehicle/VehicleIdentification/vehicleModelDate":"---"},{"Vehicle/VehicleIdentification/vehicleSeatingCapacity":"---"},{"Vehicle/VehicleIdentification/vehicleSpecialUsage":"---"},{"Vehicle/VehicleIdentification/vehicleinteriorColor":"---"},{"Vehicle/VehicleIdentification/vehicleinteriorType":"---"}]
    }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // it needs to check all elements in subtree. Expect one example explicitely,
  // and allow for others
  MOCK_EXPECT(accCheckMock->checkReadAccess)
      .exactly(18)
      .with(mock::any, mock::any)
      .returns(true);

  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("timestamp"));

  // Insert timestamp for comparision purposes
  expectedJson.insert_or_assign("timestamp",res["timestamp"]);

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Wildcard_End) {
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/VehicleIdentification/*"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
    {
    "action": "get", 
    "requestId": "1", 
    "value": [{"Vehicle/VehicleIdentification/ACRISSCode":"---"},{"Vehicle/VehicleIdentification/Brand":"---"},{"Vehicle/VehicleIdentification/Model":"---"},{"Vehicle/VehicleIdentification/VIN":"---"},{"Vehicle/VehicleIdentification/WMI":"---"},{"Vehicle/VehicleIdentification/Year":"---"},{"Vehicle/VehicleIdentification/bodyType":"---"},{"Vehicle/VehicleIdentification/dateVehicleFirstRegistered":"---"},{"Vehicle/VehicleIdentification/knownVehicleDamages":"---"},{"Vehicle/VehicleIdentification/meetsEmissionStandard":"---"},{"Vehicle/VehicleIdentification/productionDate":"---"},{"Vehicle/VehicleIdentification/purchaseDate":"---"},{"Vehicle/VehicleIdentification/vehicleConfiguration":"---"},{"Vehicle/VehicleIdentification/vehicleModelDate":"---"},{"Vehicle/VehicleIdentification/vehicleSeatingCapacity":"---"},{"Vehicle/VehicleIdentification/vehicleSpecialUsage":"---"},{"Vehicle/VehicleIdentification/vehicleinteriorColor":"---"},{"Vehicle/VehicleIdentification/vehicleinteriorType":"---"}]
    }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // it needs to check all elements in subtree. Expect one example explicitely,
  // and allow for others
  MOCK_EXPECT(accCheckMock->checkReadAccess)
      .exactly(18)
      .with(mock::any, mock::any)
      .returns(true);

  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("timestamp"));

  // Assign timestamp for comparision purposes
  expectedJson.insert_or_assign("timestamp",res["timestamp"]);

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Wildcard_NonExisting) {
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/*/FluxCapacitorCharge"};
  const VSSPath vss_path = VSSPath::fromVSSGen2("Vehicle/VehicleIdentification/Brand");

  // setup
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "get", path, jsonPathNotFound);

  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  res["timestamp"] =
      jsonPathNotFound["timestamp"]
          .as<int64_t>();  // ignoring timestamp difference for response
  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_noPermissionException) {
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonNoAccess;

  string requestId = "1";
  const VSSPath vss_path = VSSPath::fromVSSGen2("Vehicle/VehicleIdentification/Brand");

  // setup
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = vss_path.to_string();
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::noAccess(requestId, "get", "No enough read access to " + vss_path.to_string(), jsonNoAccess);


  // expectations
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .returns(false);

  // run UUT
  auto resStr = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  // timestamp must not be zero
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);
  jsonNoAccess["timestamp"] = res["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonNoAccess);

}

/** Differnt calls to get a specific path should yield the same timestamp
 *  if the value associated with the path has not been updated in
 *  the meantime 
 */
BOOST_AUTO_TEST_CASE(Gen2_Get_StableTimestamp) {
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/Speed"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.setAuthorized(false);
  channel.setConnID(1);

  //Setting data (to put a valid timestamp into tree)
  jsoncons::json value=100;
  MOCK_EXPECT(subHandlerMock->updateByPath)
      .once()
      .with(path, value)
      .returns(true);
  MOCK_EXPECT(subHandlerMock->updateByUUID)
      .once()
      .with(mock::any, value)
      .returns(true);
  db->setSignal(vss_path,value);
  

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
      {
    "action": "get", 
    "path": "Vehicle/Speed", 
    "requestId": "1", 
    "value": 100
    }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // Read access has been checked
  MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vss_path)
      .returns(true);

  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res["timestamp"].as<int64_t>() > 0);

  // Remove timestamp for comparision purposes
  expectedJson.insert_or_assign("timestamp",res["timestamp"]);
  BOOST_TEST(res == expectedJson);

  //wait 20ms (timestamps should be 1 ms resolution, but 20 ms should
  // be enough to trigger error also on systems with 10ms tick)
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vss_path)
      .returns(true);
  resStr = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  res = json::parse(resStr);

  //Answer should be identical
  BOOST_TEST(res == expectedJson);
}


BOOST_AUTO_TEST_SUITE_END()

