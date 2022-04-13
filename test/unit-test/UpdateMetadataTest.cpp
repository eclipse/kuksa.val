/*
 * ******************************************************************************
 * Copyright (c) 2021 Robert Bosch GmbH.
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

/** This are tests for VSSUpdateMetadata. No access checks */

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
BOOST_FIXTURE_TEST_SUITE(UpdateMetadataTests, TestSuiteFixture);

/* Adding to VSS tree */
BOOST_AUTO_TEST_CASE(update_metadata_change) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_modifytree(true);
  channel.set_connectionid(1);

  std::string updateMetadataRequestString = R"(
    {
        "action": "updateMetaData",
        "requestId": "1",
        "path": "Vehicle/Speed",
        "metadata": {
            "max": 9999
        }
    }
  )";


  json expectedupdateMetadataResponse = json::parse(R"(
    {
    "action": "updateMetaData",  
    "requestId": "1" 
    }
    )");
 

  // We need to verify metadata has been added correctly
  std::string getMetadataRequestString{R"(
    {
    "action": "getMetaData",  
    "path": "Vehicle/Speed",
    "requestId": "1"
    }
    )"};

  json getMetadataResponse = json::parse(R"(
    {
    "action": "getMetaData",
    "metadata": {
        "Vehicle": {
        "children": {
            "Speed": {
            "datatype": "int32",
            "description": "Vehicle speed, as sensed by the gearbox.",
            "max": 9999,
            "min": -250,
            "type": "sensor",
            "unit": "km/h",
            "uuid": "1efc9a11943b5a15ac159786051c5836"
            }
        },
        "description": "High-level vehicle data.",
        "type": "branch",
        "uuid": "1c72453e738511e9b29ad46a6a4b77e9"
        }
    },
    "requestId": "1"
    }
    )");


  // run updateVSSTree
  auto resStr = processor->processQuery(updateMetadataRequestString, channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("ts"));
  // Assign timestamp for comparision purposes
  expectedupdateMetadataResponse.insert_or_assign("ts", res["ts"]);

  BOOST_TEST(res == expectedupdateMetadataResponse);


  // verify metadata
  resStr = processor->processQuery(getMetadataRequestString, channel);
  res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("ts"));
  // Assign timestamp for comparision purposes
  getMetadataResponse.insert_or_assign("ts", res["ts"]);

  BOOST_TEST(res == getMetadataResponse);
  
}

BOOST_AUTO_TEST_CASE(update_metadata_brokenrequest) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_modifytree(true);
  channel.set_connectionid(1);

  std::string updateMetadataRequestString = R"(
    {
        "action": "updateMetaData",
        "requestId": "1",
        "path": "Vehicle/Speed",
        "metadata": 42
    })";


  json expectedupdateMetadataResponse = json::parse(R"(
  {
  "action": "updateMetaData",
  "error": {
    "message": "Schema error: #/metadata: Expected object, found uint64",
    "number": "400",
    "reason": "Bad Request"
  },
  "requestId": "1"
  }
  )");
 


  // run updateVSSTree
  auto resStr = processor->processQuery(updateMetadataRequestString, channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("ts"));
  // Assign timestamp for comparision purposes
  expectedupdateMetadataResponse.insert_or_assign("ts", res["ts"]);

  BOOST_TEST(res == expectedupdateMetadataResponse);  
}


BOOST_AUTO_TEST_SUITE_END()