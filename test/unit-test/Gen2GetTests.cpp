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
    db = std::make_shared<VssDatabase>(logMock, subHandlerMock, accCheckMock);
    db->initJsonTree(vss_file);

    processor = std::make_unique<VssCommandProcessor>(logMock, db, authMock,
                                                      subHandlerMock);

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

  //Read access has been checked
  MOCK_EXPECT(accCheckMock->checkReadNew).once().with(mock::any,vss_path).returns(true);
  
  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res["timestamp"].as<int>() > 0);

  // Remove timestamp for comparision purposes
  expectedJson["timestamp"] = res["timestamp"].as<int>();

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
  res["timestamp"] = jsonPathNotFound["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
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
    "value": [
        {
            "Vehicle/VehicleIdentification/vehicleinteriorType": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/vehicleinteriorColor": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/vehicleSpecialUsage": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/vehicleSeatingCapacity": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/vehicleModelDate": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/vehicleConfiguration": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/purchaseDate": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/productionDate": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/meetsEmissionStandard": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/knownVehicleDamages": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/dateVehicleFirstRegistered": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/bodyType": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/Year": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/WMI": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/VIN": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/Model": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/Brand": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/ACRISSCode": "---"
        }
    ]
}
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  //it needs to check all elements in subtree. Expect one example explicitely, and allow for others
  auto vss_access_path = VSSPath::fromVSSGen2("Vehicle/VehicleIdentification/Brand");
  MOCK_EXPECT(accCheckMock->checkReadNew).once().with(mock::any,vss_access_path).returns(true);
  MOCK_EXPECT(accCheckMock->checkReadNew).with(mock::any,mock::any).returns(true);


  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res["timestamp"].as<int>() > 0);

  // Remove timestamp for comparision purposes
  expectedJson["timestamp"] = res["timestamp"].as<int>();

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
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
     {
    "action": "get", 
    "requestId": "1", 
    "value": [
        {
            "Vehicle/VehicleIdentification/vehicleinteriorType": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/vehicleinteriorColor": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/vehicleSpecialUsage": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/vehicleSeatingCapacity": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/vehicleModelDate": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/vehicleConfiguration": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/purchaseDate": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/productionDate": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/meetsEmissionStandard": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/knownVehicleDamages": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/dateVehicleFirstRegistered": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/bodyType": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/Year": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/WMI": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/VIN": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/Model": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/Brand": "---"
        }, 
        {
            "Vehicle/VehicleIdentification/ACRISSCode": "---"
        }
    ]
}
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  //it needs to check all elements in subtree. Expect one example explicitely, and allow for others
  auto vss_access_path = VSSPath::fromVSSGen2("Vehicle/VehicleIdentification/Brand");
  MOCK_EXPECT(accCheckMock->checkReadNew).once().with(mock::any,vss_access_path).returns(true);
  MOCK_EXPECT(accCheckMock->checkReadNew).with(mock::any,mock::any).returns(true);

  // run UUT
  auto resStr =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res["timestamp"].as<int>() > 0);

  // Remove timestamp for comparision purposes
  expectedJson["timestamp"] = res["timestamp"].as<int>();

  BOOST_TEST(res == expectedJson);
}


BOOST_AUTO_TEST_CASE(Gen2_Get_Wildcard_NonExisting) {
  WsChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/*/FluxCapacitorCharge"};
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
  res["timestamp"] = jsonPathNotFound["timestamp"].as<int64_t>(); // ignoring timestamp difference for response
  BOOST_TEST(res == jsonPathNotFound);
}




}
