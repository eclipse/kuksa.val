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
#include <turtle/mock.hpp>

#include <boost/core/ignore_unused.hpp>

#include <jwt-cpp/jwt.h>

#include <memory>
#include <string>
#include <chrono>
#include <algorithm>

#include "WsChannel.hpp"
#include "ILoggerMock.hpp"
#include "JsonResponses.hpp"
#include "exception.hpp"

#include "RestV1ApiHandler.hpp"

namespace {
  // common resources for tests
  std::shared_ptr<ILoggerMock> logMock;

  std::unique_ptr<RestV1ApiHandler> restHndl;

  // Pre-test initialization and post-test desctruction of common resources
  struct TestSuiteFixture {
    TestSuiteFixture() {
      std::string docRoot{"/vss/api/test/"};
      logMock = std::make_shared<ILoggerMock>();

      MOCK_EXPECT(logMock->Log).at_least( 0 ); // ignore log events
      restHndl = std::make_unique<RestV1ApiHandler>(logMock, docRoot);
    }
    ~TestSuiteFixture() {
      logMock.reset();
    }
  };
}

// Define name of test suite and define test suite fixture for pre and post test handling
BOOST_FIXTURE_TEST_SUITE(RestV1ApiHandlerTests, TestSuiteFixture)

////////////////////
// Positive tests

BOOST_AUTO_TEST_CASE(Given_GetMethod_When_ValidSignalTarget_Shall_ReturnValidJson)
{
  std::string resultStr;
  jsoncons::json resultJson;

  // setup

  std::string httpMethod{"GET"};

  std::string httpDocRoot{"/vss/api/test/signals/"};
  std::vector<std::string> validTargetFormats{"Vehicle/OBD/SomeSignal",
                                              "Vehicle.OBD.SomeSignal",
                                              "DummyRootPath"};

  // verify

  for (auto & target : validTargetFormats) {
    auto httpTarget{httpDocRoot + target};
    BOOST_TEST(restHndl->GetJson(std::move(httpMethod), std::move(httpTarget), resultStr) == true);

    resultJson = jsoncons::json::parse(resultStr);
    std::replace( std::begin(target), std::end(target), '/', '.'); // replace '/'' if exist with default ','

    BOOST_TEST(resultJson["action"].as<std::string>() == "get");
    BOOST_TEST(resultJson["path"].as<std::string>() == target);
    BOOST_TEST(resultJson.has_key("requestId"));
  }
}

BOOST_AUTO_TEST_CASE(Given_GetMethod_When_ValidMetadataTarget_Shall_ReturnValidJson)
{
  std::string resultStr;
  jsoncons::json resultJson;

  // setup

  std::string httpMethod{"GET"};

  std::string httpDocRoot{"/vss/api/test/metadata/"};
  std::vector<std::string> validTargetFormats{"Vehicle/OBD/SomeSignal",
                                              "Vehicle.OBD.SomeSignal",
                                              "DummyRootPath"};

  // verify

  for (auto & target : validTargetFormats) {
    auto httpTarget{httpDocRoot + target};
    BOOST_TEST(restHndl->GetJson(std::move(httpMethod), std::move(httpTarget), resultStr) == true);

    resultJson = jsoncons::json::parse(resultStr);
    std::replace( std::begin(target), std::end(target), '/', '.'); // replace '/'' if exist with default ','

    BOOST_TEST(resultJson["action"].as<std::string>() == "getMetadata");
    BOOST_TEST(resultJson["path"].as<std::string>() == target);
    BOOST_TEST(resultJson.has_key("requestId"));
  }
}

BOOST_AUTO_TEST_CASE(Given_PutMethod_When_ValidSignalTargetValues_Shall_ReturnValidJson)
{
  std::string resultStr;
  jsoncons::json resultJson;

  // setup

  std::string httpMethod{"PUT"};

  std::string httpDocRoot{"/vss/api/test/signals/"};
  std::vector<std::pair<std::string, std::string>> validTargetFormats{
    {"Vehicle/OBD/SomeSignal", "12313"},
    {"Vehicle.OBD.SomeSignal", "test_string"},
    {"DummyRootPath", "3.45638e+22"}};

  // verify

  for (auto & target : validTargetFormats) {
    auto httpTarget{httpDocRoot + target.first + "?value=" + target.second};
    BOOST_TEST(restHndl->GetJson(std::move(httpMethod), std::move(httpTarget), resultStr) == true);

    resultJson = jsoncons::json::parse(resultStr);
    std::replace( std::begin(target.first), std::end(target.first), '/', '.'); // replace '/'' if exist with default ','

    BOOST_TEST(resultJson["action"].as<std::string>() == "set");
    BOOST_TEST(resultJson["path"].as<std::string>() == target.first);
    BOOST_TEST(resultJson["value"].as<std::string>() == target.second);
    BOOST_TEST(resultJson.has_key("requestId"));
  }

  // verify empty value
  BOOST_TEST(restHndl->GetJson(std::string("PUT"),
                               std::string("/vss/api/test/signals/Signal?value="),
                               resultStr) == true);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "set");
  BOOST_TEST(resultJson["path"].as<std::string>() == "Signal");
  BOOST_TEST(resultJson["value"].as<std::string>() == "");
  BOOST_TEST(resultJson.has_key("requestId"));
}

BOOST_AUTO_TEST_CASE(Given_PostMethod_When_ValidAuthorizeTarget_Shall_ReturnValidJson)
{
  std::string resultStr;
  jsoncons::json resultJson;

  // setup

  std::string httpMethod{"POST"};

  std::string httpDocRoot{"/vss/api/test/authorize?token=header.payload.signature"};

  // verify

  auto httpTarget{httpDocRoot};
  BOOST_TEST(restHndl->GetJson(std::move(httpMethod), std::move(httpTarget), resultStr) == true);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "authorize");
  BOOST_TEST(resultJson["tokens"].as<std::string>() == "header.payload.signature");
  BOOST_TEST(resultJson.has_key("requestId"));
}

////////////////////
// Negative tests

BOOST_AUTO_TEST_CASE(Given_GetMethod_When_InvalidRootResourceTarget_Shall_ReturnValidErrorJson)
{
  std::string resultStr;
  jsoncons::json resultJson;

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("GET"),
                               std::string("/vss/api/non/existent/root"),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "get");
  BOOST_TEST(resultJson.has_key("error"));
  auto errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);
}

BOOST_AUTO_TEST_CASE(Given_GetMethod_When_InvalidTargetFormats_Shall_ReturnValidErrorJson)
{
  std::string resultStr;
  jsoncons::json resultJson;

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("GET"),
                               std::string("/vss/api/test/unsuported_resource/Vehicle.ODB.Test"),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "get");
  BOOST_TEST(resultJson.has_key("error"));
  auto errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 404);

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("GET"),
                               std::string("/vss/api/test/signals"),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "get");
  BOOST_TEST(resultJson.has_key("error"));
  errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("GET"),
                               std::string("/vss/api/test/signals/Signal*"),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "get");
  BOOST_TEST(resultJson.has_key("error"));
  errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("GET"),
                               std::string("/vss/api/test/signals/Signal("),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "get");
  BOOST_TEST(resultJson.has_key("error"));
  errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);
}

BOOST_AUTO_TEST_CASE(Given_PutMethod_When_InvalidTargetString_Shall_ReturnValidErrorJson)
{
  std::string resultStr;
  jsoncons::json resultJson;

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("PUT"),
                               std::string("/vss/api/test/signals/Signal"),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "set");
  BOOST_TEST(resultJson.has_key("error"));
  auto errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("PUT"),
                               std::string("/vss/api/test/signals/Signal?"),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "set");
  BOOST_TEST(resultJson.has_key("error"));
  errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("PUT"),
                               std::string("/vss/api/test/signals/Signal?value"),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "set");
  BOOST_TEST(resultJson.has_key("error"));
  errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);
}


BOOST_AUTO_TEST_CASE(Given_PostMethod_When_InvalidAuthorizeTargetString_Shall_ReturnValidErrorJson)
{
  std::string resultStr;
  jsoncons::json resultJson;

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("POST"),
                               std::string("/vss/api/test/authorize"),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "authorize");
  BOOST_TEST(resultJson.has_key("error"));
  auto errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("POST"),
                               std::string("/vss/api/test/authorize?"),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "authorize");
  BOOST_TEST(resultJson.has_key("error"));
  errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("POST"),
                               std::string("/vss/api/test/authorize?token"),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "authorize");
  BOOST_TEST(resultJson.has_key("error"));
  errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);

  // verify

  BOOST_TEST(restHndl->GetJson(std::string("POST"),
                               std::string("/vss/api/test/authorize?token="),
                               resultStr) == false);

  resultJson = jsoncons::json::parse(resultStr);

  BOOST_TEST(resultJson["action"].as<std::string>() == "authorize");
  BOOST_TEST(resultJson.has_key("error"));
  errJson = resultJson["error"].as<jsoncons::json>();
  BOOST_TEST(errJson["number"].as<int>() == 400);
}

BOOST_AUTO_TEST_SUITE_END()
