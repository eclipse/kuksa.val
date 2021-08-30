/*
 * ******************************************************************************
 * Copyright (c) 2020-2021 Robert Bosch GmbH.
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

#include "UnitTestHelpers.hpp"

#include <thread>

#include <memory>
#include <string>

#include "IAccessCheckerMock.hpp"
#include "IAuthenticatorMock.hpp"
#include "ILoggerMock.hpp"
#include "ISubscriptionHandlerMock.hpp"
#include "WsChannel.hpp"
#include "kuksa.pb.h"

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
  kuksa::kuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/Speed"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
    {
        "action": "get", 
        "requestId": "1", 
        "data": {
            "path": "Vehicle/Speed", 
            "dp": {
                "value": "---"
            }
        }
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
  verify_and_erase_timestamp(res);
  verify_and_erase_timestampZero(res["data"]["dp"]);

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Invalid_JSON) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_connectionid(1);

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
        "message": "Schema error: #/requestId: Expected string, found uint64",
        "number": 400,
        "reason": "Bad Request"
      },
      "requestId": "100"
    }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // run UUT
  auto resStr =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}

/** Send an invalid JSON, without any determinable Request Id */
BOOST_AUTO_TEST_CASE(Gen2_Get_Invalid_JSON_NoRequestID) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsoncons::json jsonSetRequestForSignal;

  jsonSetRequestForSignal["action"] = "get";
  jsonSetRequestForSignal["path"] = 999;  // int as path is wrong

  std::string expectedJsonString{R"(
  {
      "action": "get",
      "error": {
        "message": "Schema error: #: Required property \"requestId\" not found\n#/path: Expected string, found uint64",
        "number": 400,
        "reason": "Bad Request"
      },
      "requestId": "UNKNOWN"
    }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // run UUT
  auto resStr =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);
  // Does result have a timestamp?
  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_NonExistingPath) {
  kuksa::kuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/OBD/FluxCapacitorCharge"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "get", path, jsonPathNotFound);

  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Branch) {
  kuksa::kuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/VehicleIdentification"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
    {
        "action": "get", 
        "requestId": "1", 
        "data": [
            {"path": "Vehicle/VehicleIdentification/ACRISSCode","dp": {"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/Brand", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/Model", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/VIN", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/WMI", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/Year", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/bodyType", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/dateVehicleFirstRegistered", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/knownVehicleDamages", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/meetsEmissionStandard", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/productionDate", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/purchaseDate", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleConfiguration", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleModelDate", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleSeatingCapacity", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleSpecialUsage", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleinteriorColor", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleinteriorType", "dp":{"value": "---"}}
        ]
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
  verify_and_erase_timestamp(res);
  for (auto &  dataRes : res["data"].array_range()) {
    verify_and_erase_timestampZero(dataRes["dp"]);
  }

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Wildcard_End) {
  kuksa::kuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/VehicleIdentification/*"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.set_authorized(true);
  channel.set_connectionid(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
    {
        "action": "get", 
        "requestId": "1", 
        "data": [
            {"path": "Vehicle/VehicleIdentification/ACRISSCode","dp": {"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/Brand", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/Model", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/VIN", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/WMI", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/Year", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/bodyType", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/dateVehicleFirstRegistered", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/knownVehicleDamages", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/meetsEmissionStandard", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/productionDate", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/purchaseDate", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleConfiguration", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleModelDate", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleSeatingCapacity", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleSpecialUsage", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleinteriorColor", "dp":{"value": "---"}},
            {"path": "Vehicle/VehicleIdentification/vehicleinteriorType", "dp":{"value": "---"}}
        ]
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
  verify_and_erase_timestamp(res);
  for (auto &  dataRes : res["data"].array_range()) {
    verify_and_erase_timestampZero(dataRes["dp"]);
  }

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Wildcard_NonExisting) {
  kuksa::kuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/*/FluxCapacitorCharge"};
  const VSSPath vss_path = VSSPath::fromVSSGen2("Vehicle/VehicleIdentification/Brand");

  // setup
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::pathNotFound(requestId, "get", path, jsonPathNotFound);

  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_noPermissionException) {
  kuksa::kuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonNoAccess;

  string requestId = "1";
  const VSSPath vss_path = VSSPath::fromVSSGen2("Vehicle/VehicleIdentification/Brand");

  // setup
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = vss_path.to_string();
  jsonGetRequestForSignal["requestId"] = requestId;

  JsonResponses::noAccess(requestId, "get", "Insufficient read access to " + vss_path.to_string(), jsonNoAccess);


  // expectations
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .returns(false);

  // run UUT
  auto resStr = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify

  BOOST_TEST(res == jsonNoAccess);

}

/** Differnt calls to get a specific path should yield the same timestamp
 *  if the value associated with the path has not been updated in
 *  the meantime 
 */
BOOST_AUTO_TEST_CASE(Gen2_Get_StableTimestamp) {
  kuksa::kuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/Speed"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.set_authorized(false);
  channel.set_connectionid(1);

  //Setting data (to put a valid timestamp into tree)
  jsoncons::json value="100";
  MOCK_EXPECT(subHandlerMock->updateByUUID)
      .once()
      .with(mock::any, mock::any)
      .returns(true);
  db->setSignal(vss_path,value);
  

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
      {
        "action": "get", 
        "requestId": "1", 
        "data": {
            "path": "Vehicle/Speed", 
            "dp": {
                "value": "100"
            }
        }
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

  verify_and_erase_timestamp(res);
  verify_and_erase_timestamp(res["data"]["dp"]);

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

  verify_and_erase_timestamp(res);
  verify_and_erase_timestamp(res["data"]["dp"]);

  //Answer should be identical
  BOOST_TEST(res == expectedJson);
}


BOOST_AUTO_TEST_SUITE_END()

