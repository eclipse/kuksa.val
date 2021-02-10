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
#include "IAccessCheckerMock.hpp"
#include "ISubscriptionHandlerMock.hpp"
#include "exception.hpp"

#include "VssDatabase.hpp"

namespace {
  // common resources for tests
  std::string validFilename{"test_vss_rel_2.0.json"};

  std::shared_ptr<ILoggerMock> logMock;
  std::shared_ptr<IAccessCheckerMock> accCheckMock;
  std::shared_ptr<ISubscriptionHandlerMock> subHandlerMock;

  std::unique_ptr<VssDatabase> db;

  // Pre-test initialization and post-test desctruction of common resources
  struct TestSuiteFixture {
    TestSuiteFixture() {
      logMock = std::make_shared<ILoggerMock>();
      accCheckMock = std::make_shared<IAccessCheckerMock>();
      subHandlerMock = std::make_shared<ISubscriptionHandlerMock>();

      MOCK_EXPECT(logMock->Log).at_least(0); // ignore log events
      db = std::make_unique<VssDatabase>(logMock, subHandlerMock, accCheckMock);
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

  BOOST_CHECK_NO_THROW(returnJson = db->getMetaData(signalPath.getVSSGen1Path()));
  BOOST_TEST(returnJson == expectedJson);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilename_When_GetMetadataForBranch_Shall_ReturnMetadata) {
  jsoncons::json returnJson;

  // setup
  db->initJsonTree(validFilename);
  std::string signalPath{"Vehicle.Acceleration"};

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
  std::string signalPath{"Vehicle.Invalid.Path"};

  // expectations

  // verify

  BOOST_CHECK_NO_THROW(returnJson = db->getMetaData(signalPath));
  BOOST_TEST(returnJson == NULL);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_GetSingleSignal_Shall_ReturnSignal) {
  jsoncons::json returnJson;
  WsChannel channel;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");

  channel.setConnID(11);
  channel.setAuthorized(true);

  // expectations

  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .returns(true);

  std::string expectedJsonString{R"({"path":"Vehicle.Acceleration.Vertical","value":"---"})"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // verify

  BOOST_CHECK_NO_THROW(returnJson = db->getSignal(channel, signalPath, true));
  BOOST_TEST(returnJson == expectedJson);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_GetBranch_Shall_ReturnAllValues) {
  jsoncons::json returnJson;
  WsChannel channel;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration");

  channel.setConnID(11);
  channel.setAuthorized(true);

  // expectations

  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .returns(true);

  std::string expectedJsonString{R"({"value":[{"Vehicle.Acceleration.Vertical":"---"},{"Vehicle.Acceleration.Longitudinal":"---"},{"Vehicle.Acceleration.Lateral":"---"}]})"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // verify

  BOOST_CHECK_NO_THROW(returnJson = db->getSignal(channel, signalPath, true));
  BOOST_TEST(returnJson == expectedJson);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelNotAuthorized_When_GetSingleSignal_Shall_Throw) {
  WsChannel channel;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");

  channel.setConnID(11);
  channel.setAuthorized(false);

  // expectations

  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .returns(false);

  // verify

  BOOST_CHECK_THROW(db->getSignal(channel, signalPath, true), noPermissionException);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelNotAuthorized_When_GetBranch_Shall_Throw) {
  WsChannel channel;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");

  channel.setConnID(11);
  channel.setAuthorized(false);

  // expectations

  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .returns(false);

  // verify

  BOOST_CHECK_THROW(db->getSignal(channel, signalPath, true), noPermissionException);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_SetSingleSignal_Shall_SetValue) {
  WsChannel channel;

  // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");
  
  channel.setConnID(11);
  channel.setAuthorized(false);

  // expectations

  jsoncons::json setValue, returnJson;
  setValue = 10;

  MOCK_EXPECT(accCheckMock->checkWriteAccess)
    .returns(true);
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .returns(true);

  // verify

  BOOST_CHECK_NO_THROW(db->setSignal(channel, signalPath, setValue, true));

  BOOST_CHECK_NO_THROW(returnJson = db->getSignal(channel, signalPath, true));
  BOOST_TEST(returnJson["value"].as<int>() == 10);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_SetPath_Shall_ThrowError) {
  WsChannel channel;

  // setup
  db->initJsonTree(validFilename);
  std::string signalPath{"Vehicle.Acceleration"};
  VSSPath vsspath = VSSPath::fromVSSGen1(signalPath);


  channel.setConnID(11);
  channel.setAuthorized(true);

  // expectations

  jsoncons::json setValue, returnJson;
  setValue = 10;

  MOCK_EXPECT(accCheckMock->checkWriteAccess)
    .returns(true);
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .returns(true);

  // verify
  //Acceleration is a branch, so can not be set
  BOOST_CHECK_THROW(db->setSignal(channel, vsspath, setValue, true), genException);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelNotAuthorized_When_SetPath_Shall_ThrowError) {
  WsChannel channel;

  // setup
  db->initJsonTree(validFilename);
  std::string signalPath{"Vehicle.Acceleration.Vectical"};
  VSSPath vsspath = VSSPath::fromVSSGen1(signalPath);

  channel.setConnID(11);
  channel.setAuthorized(false);

  // expectations

  jsoncons::json setValue, returnJson;
  setValue = 10;

  MOCK_EXPECT(accCheckMock->checkWriteAccess)
    .returns(false);

  // verify

  BOOST_CHECK_THROW(db->setSignal(channel, vsspath, setValue, true), noPermissionException);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_SetSingleSignalNoChannel_Shall_SetValue) {
  WsChannel channel;

    // setup
  db->initJsonTree(validFilename);
  VSSPath signalPath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");

  // expectations

  jsoncons::json setValue, returnJson;
  setValue = 10;

  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .returns(true);


  // verify

  BOOST_CHECK_NO_THROW(db->setSignal(signalPath.getVSSGen1Path(), setValue));

  BOOST_CHECK_NO_THROW(returnJson = db->getSignal(channel, signalPath, true));
  BOOST_TEST(returnJson["value"].as<int>() == 10);
}

BOOST_AUTO_TEST_CASE(Given_ValidVssFilenameAndChannelAuthorized_When_SetPathNoChannel_Shall_ThrowError) {
  // setup
  db->initJsonTree(validFilename);
  std::string signalPath{"Vehicle.Acceleration"};

  // expectations

  jsoncons::json setValue, returnJson;
  setValue = 10;

  // verify

  BOOST_CHECK_THROW(db->setSignal(signalPath, setValue), noPathFoundonTree);
}

BOOST_AUTO_TEST_SUITE_END()
