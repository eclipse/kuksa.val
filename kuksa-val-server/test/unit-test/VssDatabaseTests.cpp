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
  std::string validFilename{"test_vss_release_latest.json"};

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

  std::string expectedJsonString{R"({"Vehicle":{"children":{"Acceleration":{"children":{"Vertical":{"datatype":"float","description":"Vehicle acceleration in Z (vertical acceleration).","type":"sensor","unit":"m/s^2","uuid":"a4a8a7c4ac5b52deb0b3ee4ed8787c59"}},"description":"Spatial acceleration. Axis definitions according to ISO 8855.","type":"branch","uuid":"6c490e6a798c5abc8f0178ed6deae0a8"}},"description":"High-level vehicle data.","type":"branch","uuid":"ccc825f94139544dbb5f4bfd033bece6"}})"};
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

  std::string expectedJsonString{R"({"Vehicle":{"children":{"Acceleration":{"children":{"Lateral":{"datatype":"float","description":"Vehicle acceleration in Y (lateral acceleration).","type":"sensor","unit":"m/s^2","uuid":"7522c5d6b7665b16a099643b2700e93c"},"Longitudinal":{"datatype":"float","description":"Vehicle acceleration in X (longitudinal acceleration).","type":"sensor","unit":"m/s^2","uuid":"3d511fe7232b5841be311b37f322de5a"},"Vertical":{"datatype":"float","description":"Vehicle acceleration in Z (vertical acceleration).","type":"sensor","unit":"m/s^2","uuid":"a4a8a7c4ac5b52deb0b3ee4ed8787c59"}},"description":"Spatial acceleration. Axis definitions according to ISO 8855.","type":"branch","uuid":"6c490e6a798c5abc8f0178ed6deae0a8"}},"description":"High-level vehicle data.","type":"branch","uuid":"ccc825f94139544dbb5f4bfd033bece6"}})"};
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

  std::string expectedJsonString{R"({"Vehicle":{"children":{"Acceleration":{"children":{"Vertical":{"bla":"blu","datatype":"int64","description":"Vehicle acceleration in Z (vertical acceleration).","type":"sensor","unit":"m/s^2","uuid":"a4a8a7c4ac5b52deb0b3ee4ed8787c59"}},"description":"Spatial acceleration. Axis definitions according to ISO 8855.","type":"branch","uuid":"6c490e6a798c5abc8f0178ed6deae0a8"}},"description":"High-level vehicle data.","type":"branch","uuid":"ccc825f94139544dbb5f4bfd033bece6"}})"};
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

  std::string expectedJsonString{R"({})"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // verify

  BOOST_CHECK_THROW(returnJson = db->getSignal(signalPath, "value", false), notSetException);
  BOOST_TEST(returnJson == expectedJson);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_GetBranch_Shall_ReturnNoValues) {
  jsoncons::json returnJson;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration");

  // expectations

  std::string expectedJsonString{R"({})"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // verify

  BOOST_CHECK_THROW(returnJson = db->getSignal(signalPath, "value", true), notSetException);
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
    .with(mock::any, "float", "value", mock::any)
    .returns(0);

  // verify

  BOOST_CHECK_NO_THROW(db->setSignal(signalPath, "value", setValue));

  BOOST_CHECK_NO_THROW(returnJson = db->getSignal(signalPath, "value"));
  BOOST_TEST(returnJson["dp"]["value"].as<float>() == 10);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_SetSingleSignalNoChannel_Shall_SetValue) {
    // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");

  // expectations

  jsoncons::json setValue, returnJson;
  setValue = 10;

  // verify
  MOCK_EXPECT(subHandlerMock->publishForVSSPath).with(mock::any, "float", "value", mock::any).returns(0);

  BOOST_CHECK_NO_THROW(db->setSignal(signalPath, "value", setValue));

  BOOST_CHECK_NO_THROW(returnJson = db->getSignal(signalPath, "value"));
  BOOST_TEST(returnJson["dp"]["value"].as<float>() == 10);
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
  BOOST_TEST(dt == "float");
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
