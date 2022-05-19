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

/** This are tests for VIS Gen2-style set request. No access checks */

#include <boost/test/unit_test.hpp>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <turtle/mock.hpp>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS

#include "UnitTestHelpers.hpp"

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
std::string validFilename{"test_vss_rel_2.2.json"};

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

    std::string vss_file{validFilename};

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
BOOST_FIXTURE_TEST_SUITE(Gen2SetTests, TestSuiteFixture);

/** Set an existing sensor */
BOOST_AUTO_TEST_CASE(Gen2_Set_Sensor_Simple) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsoncons::json jsonSetRequestForSignal;

  string requestId = "1";
  std::string path{"Vehicle/Speed"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["requestId"] = requestId;
  jsonSetRequestForSignal["value"] = 100;

  std::string expectedJsonString{R"(
      {
    "action": "set", 
    "requestId": "1"
    }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // Write access has been checked
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
      .with(mock::any, vss_path)
      .returns(true);

  // Notify subscribers
  MOCK_EXPECT(subHandlerMock->publishForVSSPath)
      .once()
      .with(mock::any, "value", mock::any)
      .returns(true);

  // run UUT
  auto resStr =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}

/** Set an array */
BOOST_AUTO_TEST_CASE(Gen2_Set_Array) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsoncons::json jsonSetRequestForSignal;

  string requestId = "1";
  std::string path{"Vehicle/OBD/DTCList"};
  std::string value{R"(
    ["dtc1", "dtc2"]
  )"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["requestId"] = requestId;
  jsonSetRequestForSignal["value"] = jsoncons::json::parse(value);

  std::string expectedJsonString{R"(
      {
    "action": "set", 
    "requestId": "1"
    }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // Write access has been checked
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
      .with(mock::any, vss_path)
      .returns(true);

  // Notify subscribers
  MOCK_EXPECT(subHandlerMock->publishForVSSPath)
      .once()
      .with(mock::any, "value", mock::any)
      .returns(true);

  // run UUT
  auto resStr =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}
/** Send an invalid JSON */
BOOST_AUTO_TEST_CASE(Gen2_Set_Invalid_JSON) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsoncons::json jsonSetRequestForSignal;

  std::string path{"Vehicle/Speed"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["requestId"] = 100;  // int is invalid here;
  jsonSetRequestForSignal["value"] = 100;

  std::string expectedJsonString{R"(
  {
    "action": "set",
    "error": {
      "message": "Schema error: #/requestId: Expected string, found uint64",
      "number": "400",
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

  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}

/** Send an invalid JSON, without any determinable Request Id */
BOOST_AUTO_TEST_CASE(Gen2_Set_Invalid_JSON_NoRequestID) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsoncons::json jsonSetRequestForSignal;

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = 999;  // int as path is wrong
  jsonSetRequestForSignal["value"] = 100;

  std::string expectedJsonString{R"(
{
  "action": "set",
  "error": {
    "message": "Schema error: #: Required property \"requestId\" not found\n#/path: Expected string, found uint64",
    "number": "400",
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

  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}

/** Set an non-existing path */
BOOST_AUTO_TEST_CASE(Gen2_Set_Non_Existing_Path) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsoncons::json jsonSetRequestForSignal;

  string requestId = "1";
  std::string path{"Vehicle/Fluxcapacitor/Charge"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["requestId"] = requestId;
  jsonSetRequestForSignal["value"] = 100;

  std::string test = jsonSetRequestForSignal.as_string();

  std::string expectedJsonString{R"(
      {
  "action": "set",
  "error": {
    "message": "I can not find Vehicle/Fluxcapacitor/Charge in my db",
    "number": "404",
    "reason": "Path not found"
  },
  "requestId": "1"
  }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // Write access has been checked
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
      .with(mock::any, vss_path)
      .returns(true);

  // run UUT
  auto resStr =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  auto res = json::parse(resStr);

  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}


/** Set branch */
BOOST_AUTO_TEST_CASE(Gen2_Set_Branch) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsoncons::json jsonSetRequestForSignal;

  string requestId = "1";
  std::string path{"Vehicle/VehicleIdentification"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["requestId"] = requestId;
  jsonSetRequestForSignal["value"] = 100;

  std::string test = jsonSetRequestForSignal.as_string();

  std::string expectedJsonString{R"(
      {
  "action": "set",
  "error": {
    "message": "Can not set Vehicle/VehicleIdentification. Only sensor or actor leaves can be set.",
    "number": "403",
    "reason": "Forbidden"
  },
  "requestId": "1"
}
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // Write access has been checked
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
      .with(mock::any, vss_path)
      .returns(true);

  // run UUT
  auto resStr =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  auto res = json::parse(resStr);

  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}


/** Set attribute */
BOOST_AUTO_TEST_CASE(Gen2_Set_Attribute) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsoncons::json jsonSetRequestForSignal;

  string requestId = "1";
  std::string path{"Vehicle/VehicleIdentification/Brand"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = path;
  jsonSetRequestForSignal["requestId"] = requestId;
  jsonSetRequestForSignal["value"] = 100;

  std::string test = jsonSetRequestForSignal.as_string();

  std::string expectedJsonString{R"(
      {
  "action": "set",
  "error": {
    "message": "Can not set Vehicle/VehicleIdentification/Brand. Only sensor or actor leaves can be set.",
    "number": "403",
    "reason": "Forbidden"
  },
  "requestId": "1"
}
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // Write access has been checked
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
      .with(mock::any, vss_path)
      .returns(true);

  // run UUT
  auto resStr =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  auto res = json::parse(resStr);

  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Set_noPermissionException) {
  kuksa::kuksaChannel channel;

  jsoncons::json jsonSetRequestForSignal;
  jsoncons::json jsonNoAccess;

  string requestId = "1";
  const VSSPath vss_path = VSSPath::fromVSSGen2("Vehicle/VehicleIdentification/Brand");

  // setup
  channel.set_authorized(false);
  channel.set_connectionid(1);

  jsonSetRequestForSignal["action"] = "set";
  jsonSetRequestForSignal["path"] = vss_path.to_string();
  jsonSetRequestForSignal["requestId"] = requestId;
  jsonSetRequestForSignal["value"] = 100;

  JsonResponses::noAccess(requestId, "set", "No write access to " + vss_path.to_string(), jsonNoAccess);


  // expectations
  MOCK_EXPECT(accCheckMock->checkWriteAccess)
    .returns(false);

  // run UUT
  auto resStr = processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  auto res = json::parse(resStr);

  // verify
  verify_and_erase_timestamp(res);
  verify_and_erase_timestamp(jsonNoAccess);

  BOOST_TEST(res == jsonNoAccess);
}

BOOST_AUTO_TEST_SUITE_END()

