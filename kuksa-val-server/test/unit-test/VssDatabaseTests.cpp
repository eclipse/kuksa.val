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

#include "UnitTestHelpers.hpp"

#include <memory>
#include <string>

#include "ILoggerMock.hpp"
#include "IAccessCheckerMock.hpp"
#include "ISubscriptionHandlerMock.hpp"
#include "exception.hpp"
#include "kuksa.pb.h"

#include "VssDatabase.hpp"

namespace {
  // common resources for tests
  std::string validFilename{"test_vss_rel_2.0.json"};

  std::shared_ptr<ILoggerMock> logMock;
  std::shared_ptr<ISubscriptionHandlerMock> subHandlerMock;

  std::unique_ptr<VssDatabase> db;

  // Pre-test initialization and post-test desctruction of common resources
  struct TestSuiteFixture {
    TestSuiteFixture() {
      logMock = std::make_shared<ILoggerMock>();
      subHandlerMock = std::make_shared<ISubscriptionHandlerMock>();

      MOCK_EXPECT(logMock->Log).at_least(0); // ignore log events
      db = std::make_unique<VssDatabase>(logMock, subHandlerMock);
    }
    ~TestSuiteFixture() {
      db.reset();
      logMock.reset();
    }
  };
}

// Define name of test suite and define test suite fixture for pre and post test handling
BOOST_FIXTURE_TEST_SUITE(VssDatabaseTests, TestSuiteFixture)

BOOST_AUTO_TEST_CASE(Given_ValidVssFilename_When_FileCorrect_Shall_InitCorrectly) {
  BOOST_CHECK_NO_THROW(db->initJsonTree(validFilename));
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilename_When_FileMissing_Shall_ThrowError) {
  std::string missingFilename{"dummy_filename.json"};
  BOOST_CHECK_THROW(db->initJsonTree(missingFilename), std::exception);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilename_When_GetMetadataForSingleSignal_Shall_ReturnMetadata) {
  jsoncons::json returnJson;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");

  // expectations

  std::string expectedJsonString{R"({"Vehicle":{"children":{"Acceleration":{"children":{"Vertical":{"datatype":"int32","description":"Vehicle acceleration in Z (vertical acceleration).","type":"sensor","unit":"m/s2","uuid":"9521e8d36a9b546d9414a779f5dd9bef"}},"description":"Spatial acceleration","type":"branch","uuid":"ce0fb48b566354c7841e279125f6f66d"}},"description":"High-level vehicle data.","type":"branch","uuid":"1c72453e738511e9b29ad46a6a4b77e9"}})"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // verify

  BOOST_CHECK_NO_THROW(returnJson = db->getMetaData(signalPath));
  BOOST_TEST(returnJson == expectedJson);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilename_When_GetMetadataForBranch_Shall_ReturnMetadata) {
  jsoncons::json returnJson;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration");

  // expectations

  std::string expectedJsonString{R"({"Vehicle":{"children":{"Acceleration":{"children":{"Lateral":{"datatype":"int32","description":"Vehicle acceleration in Y (lateral acceleration).","type":"sensor","unit":"m/s2","uuid":"5c28427f79ca5fe394b47fe057a2af9b"},"Longitudinal":{"datatype":"int32","description":"Vehicle acceleration in X (longitudinal acceleration).","type":"sensor","unit":"m/s2","uuid":"c83f0c12653b5e7baf000799052f5533"},"Vertical":{"datatype":"int32","description":"Vehicle acceleration in Z (vertical acceleration).","type":"sensor","unit":"m/s2","uuid":"9521e8d36a9b546d9414a779f5dd9bef"}},"description":"Spatial acceleration","type":"branch","uuid":"ce0fb48b566354c7841e279125f6f66d"}},"description":"High-level vehicle data.","type":"branch","uuid":"1c72453e738511e9b29ad46a6a4b77e9"}})"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // verify

  BOOST_CHECK_NO_THROW(returnJson = db->getMetaData(signalPath));
  BOOST_TEST(returnJson == expectedJson);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilename_When_GetMetadataForInvalidPath_Shall_ReturnNull) {
  jsoncons::json returnJson;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Invalid.Path");

  // expectations

  // verify

  BOOST_CHECK_NO_THROW(returnJson = db->getMetaData(signalPath));
  BOOST_TEST(returnJson == NULL);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_updateMetadata) {
  jsoncons::json newMetaData, returnJson;
  KuksaChannel channel;

  channel.setConnID(11);
  channel.setAuthorized(true);

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Invalid.Path");

  // expectations

  // verify

  BOOST_CHECK_THROW(db->updateMetaData(channel, signalPath, newMetaData), noPermissionException);
}
BOOST_AUTO_TEST_CASE(Given_ValidVssFilename_When_updateMetadataForInvalidPath_Shall_throwException) {
  jsoncons::json newMetaData, returnJson;
  KuksaChannel channel;

  channel.setConnID(11);
  channel.enableModifyTree();

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Invalid.Path");

  // expectations

  // verify

  BOOST_CHECK_THROW(db->updateMetaData(channel, signalPath, newMetaData), notValidException);
}
BOOST_AUTO_TEST_CASE(Given_ValidVssFilename_When_updateMetadataValidPath) {
  jsoncons::json newMetaData, returnJson;
  KuksaChannel channel;

  channel.setConnID(11);
  channel.enableModifyTree();

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");

  // expectations

  std::string expectedJsonString{R"({"Vehicle":{"children":{"Acceleration":{"children":{"Vertical":{"bla":"blu","datatype":"int64","description":"Vehicle acceleration in Z (vertical acceleration).","type":"sensor","unit":"m/s2","uuid":"9521e8d36a9b546d9414a779f5dd9bef"}},"description":"Spatial acceleration","type":"branch","uuid":"ce0fb48b566354c7841e279125f6f66d"}},"description":"High-level vehicle data.","type":"branch","uuid":"1c72453e738511e9b29ad46a6a4b77e9"}})"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  std::string metaDataString{R"({"bla":"blu","datatype":"int64"})"};
  newMetaData = jsoncons::json::parse(metaDataString);
  BOOST_CHECK_NO_THROW(db->updateMetaData(channel, signalPath, newMetaData));

  // verify

  BOOST_CHECK_NO_THROW(returnJson = db->getMetaData(signalPath));
  BOOST_TEST(returnJson == expectedJson);

}
BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_GetSingleSignal_Shall_ReturnSignal) {
  jsoncons::json returnJson;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");

  // expectations

  std::string expectedJsonString{R"({"path":"Vehicle.Acceleration.Vertical","dp":{"value":"---"}})"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // verify

  BOOST_CHECK_NO_THROW(returnJson = db->getSignal(signalPath, "value", false));

  std::string t = returnJson.to_string();

  //false mans timestamp as _s and _ns
  BOOST_TEST(returnJson["dp"].contains("ts_s"));
  BOOST_TEST(returnJson["dp"].contains("ts_ns"));
  BOOST_TEST(returnJson["dp"]["ts_s"].as<uint64_t>()==0);
  BOOST_TEST(returnJson["dp"]["ts_ns"].as<uint32_t>()==0);

  returnJson["dp"].erase("ts_s");
  returnJson["dp"].erase("ts_ns");
  
  BOOST_TEST(returnJson == expectedJson);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_GetBranch_Shall_ReturnNoValues) {
  jsoncons::json returnJson;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration");

  // expectations

  std::string expectedJsonString{R"({"path":"Vehicle.Acceleration","dp":{"value":"---"}})"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // verify

  BOOST_CHECK_NO_THROW(returnJson = db->getSignal(signalPath, "value", true));

  verify_and_erase_timestampZero(returnJson["dp"]);

  BOOST_TEST(returnJson == expectedJson);
}


BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_SetSingleSignal_Shall_SetValue) {
  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");
  
  // expectations

  jsoncons::json setValue, returnJson;
  setValue = 10;

  MOCK_EXPECT(subHandlerMock->publishForVSSPath)
    .at_least(1)
    .with(mock::any, "int32", "value", mock::any)
    .returns(0);

  // verify

  BOOST_CHECK_NO_THROW(db->setSignal(signalPath, "value", setValue));

  BOOST_CHECK_NO_THROW(returnJson = db->getSignal(signalPath, "value"));
  BOOST_TEST(returnJson["dp"]["value"].as<int>() == 10);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_SetSingleSignalNoChannel_Shall_SetValue) {
    // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");

  // expectations

  jsoncons::json setValue, returnJson;
  setValue = 10;

  // verify
  MOCK_EXPECT(subHandlerMock->publishForVSSPath).with(mock::any, "int32", "value", mock::any).returns(0);

  BOOST_CHECK_NO_THROW(db->setSignal(signalPath, "value", setValue));

  BOOST_CHECK_NO_THROW(returnJson = db->getSignal(signalPath, "value"));
  BOOST_TEST(returnJson["dp"]["value"].as<int>() == 10);
}


/*********************** isActor() tests ************************/
BOOST_AUTO_TEST_CASE(Check_IsActor_ForActor) {
    std::string inputJsonString{R"({
  "datatype": "boolean",
  "description": "Is brake light on",
  "type": "actuator",
  "uuid": "7b8b136ec8aa59cb8773aa3c455611a4"
})"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isActor(inputJson) == true);
}

BOOST_AUTO_TEST_CASE(Check_IsActor_ForSensor) {
    std::string inputJsonString{R"({
  "datatype": "uint8",
  "description": "Rain intensity. 0 = Dry, No Rain. 100 = Covered.",
  "type": "sensor",
  "unit": "percent",
  "uuid": "02828e9e5f7b593fa2160e7b6dbad157"
})"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isActor(inputJson) == false);
}

BOOST_AUTO_TEST_CASE(Check_IsActor_ForAttribute) {
    std::string inputJsonString{R"(
    {
      "datatype": "uint16",
      "description": "Gross capacity of the battery",
      "type": "attribute",
      "unit": "kWh",
      "uuid": "7b5402cc647454b49ee019e8689d3737"
    }
    )"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isActor(inputJson) == false);
}

BOOST_AUTO_TEST_CASE(Check_IsActor_ForInvalid) {
    std::string inputJsonString{R"({
  "element": "invalidVSSJSon"
})"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isActor(inputJson) == false);
}

/*********************** isSensor() tests ************************/
BOOST_AUTO_TEST_CASE(Check_IsSensor_ForSensor) {
    std::string inputJsonString{R"({
  "datatype": "uint8",
  "description": "Rain intensity. 0 = Dry, No Rain. 100 = Covered.",
  "type": "sensor",
  "unit": "percent",
  "uuid": "02828e9e5f7b593fa2160e7b6dbad157"
})"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isSensor(inputJson) == true);
}

BOOST_AUTO_TEST_CASE(Check_IsSensor_ForActor) {
    std::string inputJsonString{R"({
  "datatype": "boolean",
  "description": "Is brake light on",
  "type": "actuator",
  "uuid": "7b8b136ec8aa59cb8773aa3c455611a4"
})"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isSensor(inputJson) == false);
}

BOOST_AUTO_TEST_CASE(Check_IsSensor_ForAttribute) {
  std::string inputJsonString{R"(
  {
    "datatype": "uint16",
    "description": "Gross capacity of the battery",
    "type": "attribute",
    "unit": "kWh",
    "uuid": "7b5402cc647454b49ee019e8689d3737"
  }    
  )"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isSensor(inputJson) == false);
}


BOOST_AUTO_TEST_CASE(Check_IsSensor_ForInvalid) {
    std::string inputJsonString{R"({
  "element": "invalidVSSJSon"
})"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isSensor(inputJson) == false);
}

/*********************** isAttribute() tests ************************/
BOOST_AUTO_TEST_CASE(Check_IsAttribute_ForSensor) {
    std::string inputJsonString{R"({
  "datatype": "uint8",
  "description": "Rain intensity. 0 = Dry, No Rain. 100 = Covered.",
  "type": "sensor",
  "unit": "percent",
  "uuid": "02828e9e5f7b593fa2160e7b6dbad157"
})"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isAttribute(inputJson) == false);
}

BOOST_AUTO_TEST_CASE(Check_IsAttribute_ForActor) {
    std::string inputJsonString{R"({
  "datatype": "boolean",
  "description": "Is brake light on",
  "type": "actuator",
  "uuid": "7b8b136ec8aa59cb8773aa3c455611a4"
})"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isAttribute(inputJson) == false);
}

BOOST_AUTO_TEST_CASE(Check_IsAttribute_ForAttribute) {
  std::string inputJsonString{R"(
  {
    "datatype": "uint16",
    "description": "Gross capacity of the battery",
    "type": "attribute",
    "unit": "kWh",
    "uuid": "7b5402cc647454b49ee019e8689d3737"
  }    
  )"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isAttribute(inputJson) == true);
}


BOOST_AUTO_TEST_CASE(Check_IsAttribute_ForInvalid) {
  std::string inputJsonString{R"({
  "element": "invalidVSSJSon"
  })"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);
  BOOST_TEST(VssDatabase::isAttribute(inputJson) == false);
}


/*** applyDefaultValue Checks **/
BOOST_AUTO_TEST_CASE(applyDefaultValues_withDefault) {
  std::string inputJsonString{R"(
    {
      "datatype": "uint8",
      "default": 0,
      "description": "Number of doors in vehicle",
      "type": "attribute",
      "uuid": "49a445e112f35283b4be6ec82812b29b"
    }
  )"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);

  std::string expectedJsonString{R"(
  {
    "datatype": "uint8",
    "default": 0,
    "value": "0",
    "description": "Number of doors in vehicle",
    "type": "attribute",
    "uuid": "49a445e112f35283b4be6ec82812b29b"
  }
  )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  db->applyDefaultValues(inputJson,VSSPath::fromVSS(""));
  BOOST_TEST(inputJson == expectedJson);
}


BOOST_AUTO_TEST_CASE(applyDefaultValues_Recurse) {
  std::string inputJsonString{R"(
  {
    "children": {
      "ChargePlugType": {
        "datatype": "string",
        "default": "ccs",
        "description": "Type of charge plug available on the vehicle (CSS includes Type2).",
        "enum": [
          "type 1",
          "type 2",
          "ccs",
          "chademo"
        ],
        "type": "attribute",
        "uuid": "0c4cf2b3979456928967e73b646bda05"
      }
    }
  }
  )"};
  jsoncons::json inputJson = jsoncons::json::parse(inputJsonString);

  std::string expectedJsonString{R"(
   {
    "children": {
      "ChargePlugType": {
        "datatype": "string",
        "default": "ccs",
        "value": "ccs",
        "description": "Type of charge plug available on the vehicle (CSS includes Type2).",
        "enum": [
          "type 1",
          "type 2",
          "ccs",
          "chademo"
        ],
        "type": "attribute",
        "uuid": "0c4cf2b3979456928967e73b646bda05"
      }
    }
  }
  )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  db->applyDefaultValues(inputJson,VSSPath::fromVSS(""));
  BOOST_TEST(inputJson == expectedJson);
}


/** getDataTypeTests **/
BOOST_AUTO_TEST_CASE(getDataTypeForSensor) {
  db->initJsonTree(validFilename);
  std::string path = "Vehicle.Speed";
  std::string dt = db->getDatatypeForPath(VSSPath::fromVSS(path));
  BOOST_TEST(dt == "int32");
}

BOOST_AUTO_TEST_CASE(getDataTypeForAttribute) {
  db->initJsonTree(validFilename);
  std::string path = "Vehicle/VehicleIdentification/VIN";
  std::string dt = db->getDatatypeForPath(VSSPath::fromVSS(path));
  BOOST_TEST(dt == "string");
}

BOOST_AUTO_TEST_CASE(getDataTypeForActuator) {
  db->initJsonTree(validFilename);
  std::string path = "Vehicle/Cabin/Door/Row1/Right/IsLocked";
  std::string dt = db->getDatatypeForPath(VSSPath::fromVSS(path));
  BOOST_TEST(dt == "boolean");
}

BOOST_AUTO_TEST_CASE(getDataTypeForBranch) {
  db->initJsonTree(validFilename);
  std::string path = "Vehicle/Body";
  BOOST_CHECK_THROW(db->getDatatypeForPath(VSSPath::fromVSS(path)), genException);
}

BOOST_AUTO_TEST_CASE(getDataTypeForNonExistingPath) {
  db->initJsonTree(validFilename);
  std::string path = "Vehicle/FluxCapacitor/Charge";
  BOOST_CHECK_THROW(db->getDatatypeForPath(VSSPath::fromVSS(path)), noPathFoundonTree);
}

BOOST_AUTO_TEST_SUITE_END()
