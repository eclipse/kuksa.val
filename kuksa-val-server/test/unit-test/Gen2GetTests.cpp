/**********************************************************************
 * Copyright (c) 2020-2021 Robert Bosch GmbH.
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
#include "KuksaChannel.hpp"
#include "kuksa.pb.h"

#include "exception.hpp"

#include "VSSPath.hpp"

#include "JsonResponses.hpp"
#include "VssCommandProcessor.hpp"
#include "VssDatabase.hpp"

#include "exception.hpp"

namespace {
// common resources for tests
std::string validFilename{"test_vss_release_latest.json"};

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

    std::string vss_file{"test_vss_release_latest.json"};

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
  KuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/Speed"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.setAuthorized(false);
  channel.setConnID(1);
  channel.setType(KuksaChannel::Type::WEBSOCKET_PLAIN);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
    {"action":"get",
     "error":{"message":"Attribute value on Vehicle/Speed has not been set yet.",
     "number":"404",
     "reason":"unavailable_data"},
     "requestId":"1"
     }
  )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // Read access has been checked
  MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vss_path)
      .returns(true);

  // run UUT
  auto res =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  // Does result have a timestamp?
  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Invalid_JSON) {
  KuksaChannel channel;
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
        "message": "Schema error: #/requestId: Expected string, found uint64",
        "number": "400",
        "reason": "Bad Request"
      },
      "requestId": "100"
    }
      )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // run UUT
  auto res =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);

  // Does result have a timestamp?
  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}

/** Send an invalid JSON, without any determinable Request Id */
BOOST_AUTO_TEST_CASE(Gen2_Get_Invalid_JSON_NoRequestID) {
  KuksaChannel channel;
  channel.setAuthorized(false);
  channel.setConnID(1);

  jsoncons::json jsonSetRequestForSignal;

  jsonSetRequestForSignal["action"] = "get";
  jsonSetRequestForSignal["path"] = 999;  // int as path is wrong

  std::string expectedJsonString{R"(
  {
      "action": "get",
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
  auto res =
      processor->processQuery(jsonSetRequestForSignal.as_string(), channel);
  // Does result have a timestamp?
  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_NonExistingPath) {
  KuksaChannel channel;

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
  auto res =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Branch) {
  KuksaChannel channel;

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
    {"action":"get",
     "error":{"message":"Attribute value on Vehicle/VehicleIdentification/AcrissCode has not been set yet.",
     "number":"404",
     "reason":"unavailable_data"},
     "requestId":"1"
     }
  )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // it needs to check all elements in subtree. Expect one example explicitely,
  // and allow for others
  MOCK_EXPECT(accCheckMock->checkReadAccess)
      .exactly(1)
      .with(mock::any, mock::any)
      .returns(true);

  // run UUT
  auto res =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  // Does result have a timestamp?
  verify_and_erase_timestamp(res);

  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Wildcard_End) {
  KuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/VehicleIdentification/*"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.setAuthorized(true);
  channel.setConnID(1);

  jsonGetRequestForSignal["action"] = "get";
  jsonGetRequestForSignal["attribute"] = "value";
  jsonGetRequestForSignal["path"] = path;
  jsonGetRequestForSignal["requestId"] = requestId;

  std::string expectedJsonString{R"(
    {"action":"get",
     "error":{"message":"Attribute value on Vehicle/VehicleIdentification/AcrissCode has not been set yet.",
     "number":"404",
     "reason":"unavailable_data"},
     "requestId":"1"
     }
  )"};
  jsoncons::json expectedJson = jsoncons::json::parse(expectedJsonString);

  // it needs to check all elements in subtree. Expect one example explicitely,
  // and allow for others
  MOCK_EXPECT(accCheckMock->checkReadAccess)
      .exactly(1)
      .with(mock::any, mock::any)
      .returns(true);

  // run UUT
  auto res =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  // Does result have a timestamp?
  verify_and_erase_timestamp(res);
  BOOST_TEST(res == expectedJson);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_Wildcard_NonExisting) {
  KuksaChannel channel;

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
  auto res =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  BOOST_TEST(res == jsonPathNotFound);
}

BOOST_AUTO_TEST_CASE(Gen2_Get_noPermissionException) {
  KuksaChannel channel;

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

  JsonResponses::noAccess(requestId, "get", "Insufficient read access to " + vss_path.to_string(), jsonNoAccess);


  // expectations
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .returns(false);

  // run UUT
  auto res = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  // verify

  BOOST_TEST(res == jsonNoAccess);

}

/** Differnt calls to get a specific path should yield the same timestamp
 *  if the value associated with the path has not been updated in
 *  the meantime 
 */
BOOST_AUTO_TEST_CASE(Gen2_Get_StableTimestamp) {
  KuksaChannel channel;

  jsoncons::json jsonGetRequestForSignal;
  jsoncons::json jsonPathNotFound;

  string requestId = "1";
  std::string path{"Vehicle/Speed"};
  const VSSPath vss_path = VSSPath::fromVSSGen2(path);

  // setup
  channel.setAuthorized(false);
  channel.setConnID(1);

  //Setting data (to put a valid timestamp into tree)
  jsoncons::json value="100";
  MOCK_EXPECT(subHandlerMock->publishForVSSPath)
      .once()
      .with(mock::any, "float", "value", mock::any)
      .returns(true);
  db->setSignal(vss_path, "value", value);
  

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
                "value": "100.0"
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
  auto res =
      processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

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
  res = processor->processQuery(jsonGetRequestForSignal.as_string(), channel);

  verify_and_erase_timestamp(res);
  verify_and_erase_timestamp(res["data"]["dp"]);

  //Answer should be identical
  BOOST_TEST(res == expectedJson);
}


BOOST_AUTO_TEST_SUITE_END()

