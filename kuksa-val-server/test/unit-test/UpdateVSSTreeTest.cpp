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

/** This are tests for VSSUpdateTree. No access checks */

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
BOOST_FIXTURE_TEST_SUITE(UpdateVSSTreeTests, TestSuiteFixture);

/* Adding to VSS tree */
BOOST_AUTO_TEST_CASE(update_vss_tree_add) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_modifytree(true);
  channel.set_connectionid(1);

  json metatdata = json::parse(R"(
    {
    "Vehicle": {
        "type": "branch",
        "uuid": "ccc825f94139544dbb5f4bfd033bece6",
        "children": {
        "Private": {
            "type": "branch",
            "uuid": "4161866b048a5b76aa3124dec82e0260",
            "children": {
            "ThrustersActive": {
                "datatype": "boolean",
                "type": "sensor",
                "uuid": "304b0817a2df524fa442c6a2d2742ed0"
            }
            }
        }
        }
    }
    }
  )");

  jsoncons::json updateRequest;
  updateRequest["action"] = "updateVSSTree";
  updateRequest["metadata"] = metatdata;
  updateRequest["requestId"] = "1";

  std::string expectedupdateVSSTreeAnswerString{R"(
    {
    "action": "updateVSSTree",  
    "requestId": "1" 
    }
    )"};
  jsoncons::json expectedupdateVSSTreeAnswer =
      jsoncons::json::parse(expectedupdateVSSTreeAnswerString);

  // We need to verify metadata has been added correctly
  std::string getMetadataRequestString{R"(
    {
    "action": "getMetaData",  
    "path": "Vehicle.Private",
    "requestId": "1"
    }
    )"};

  std::string getMetadataResponseString{R"(
    {
    "action": "getMetaData",
    "metadata": {
        "Vehicle": {
        "children": {
            "Private": {
            "children": {
                "ThrustersActive": {
                "datatype": "boolean",
                "type": "sensor",
                "uuid": "304b0817a2df524fa442c6a2d2742ed0"
                }
            },
            "description": "Uncontrolled branch where non-public signals can be defined.",
            "type": "branch",
            "uuid": "4161866b048a5b76aa3124dec82e0260"
            }
        },
        "description": "High-level vehicle data.",
        "type": "branch",
        "uuid": "ccc825f94139544dbb5f4bfd033bece6"
        }
    },
    "requestId": "1"
    }
    )"};
  jsoncons::json getMetadataResponse =
      jsoncons::json::parse(getMetadataResponseString);

  // run updateVSSTree
  auto resStr = processor->processQuery(updateRequest.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("ts"));
  // Assign timestamp for comparision purposes
  expectedupdateVSSTreeAnswer.insert_or_assign("ts", res["ts"]);

  BOOST_TEST(res == expectedupdateVSSTreeAnswer);

  // verify metadata
  resStr = processor->processQuery(getMetadataRequestString, channel);
  res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("ts"));
  // Assign timestamp for comparision purposes
  getMetadataResponse.insert_or_assign("ts", res["ts"]);

  BOOST_TEST(res == getMetadataResponse);
}

/* Check whether a default is applied */
BOOST_AUTO_TEST_CASE(update_vss_tree_apply_default) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_modifytree(true);
  channel.set_connectionid(1);

  json metatdata = json::parse(R"(
    {
    "Vehicle": {
        "type": "branch",
        "uuid": "ccc825f94139544dbb5f4bfd033bece6",
        "children": {
        "Private": {
            "type": "branch",
            "uuid": "4161866b048a5b76aa3124dec82e0260",
            "children": {
            "TestAttribute": {
                "datatype": "uint16",
                "default": "100",
                "type": "attribute",
                "uuid": "0185"
            }
            }
        }
        }
    }
    }
  )");

  jsoncons::json updateRequest;
  updateRequest["action"] = "updateVSSTree";
  updateRequest["metadata"] = metatdata;
  updateRequest["requestId"] = "1";

  std::string expectedupdateVSSTreeAnswerString{R"(
    {
    "action": "updateVSSTree",  
    "requestId": "1" 
    }
    )"};
  jsoncons::json expectedupdateVSSTreeAnswer =
      jsoncons::json::parse(expectedupdateVSSTreeAnswerString);

  // Verify default is applied as value
  std::string getValueString{R"(
    {
    "action": "get",  
    "path": "Vehicle.Private.TestAttribute",
    "requestId": "1"
    }
    )"};

  // Read access has been checked
  MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, VSSPath::fromVSS("Vehicle/Private/TestAttribute"))
      .returns(true);

  std::string getValueResponseString{R"(
    {
    "action": "get",
    "requestId": "1",
    "data":{
        "path": "Vehicle.Private.TestAttribute",
        "dp":{
            "value": "100"
        }
    }
    }
    )"};
  jsoncons::json getValueResponse =
      jsoncons::json::parse(getValueResponseString);

  // run updateVSSTree
  auto resStr = processor->processQuery(updateRequest.as_string(), channel);
  auto res = json::parse(resStr);


  verify_and_erase_timestamp(res);
  BOOST_TEST(res == expectedupdateVSSTreeAnswer);

  // verify metadata
  resStr = processor->processQuery(getValueString, channel);
  res = json::parse(resStr);

  // Does result have a timestamp?
  verify_and_erase_timestamp(res);
  verify_and_erase_timestampZero(res["data"]["dp"]);

  BOOST_TEST(res == getValueResponse);
}

/* Override existing metadata in tree */
BOOST_AUTO_TEST_CASE(update_vss_tree_override) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_modifytree(true);
  channel.set_connectionid(1);

  json metatdata = json::parse(R"(
    {
    "Vehicle": {
        "type": "branch",
        "uuid": "ccc825f94139544dbb5f4bfd033bece6",
        "children": {
        "Speed": {
            "max": 9100
        }
        }
    }
    }
  )");

  jsoncons::json updateRequest;
  updateRequest["action"] = "updateVSSTree";
  updateRequest["metadata"] = metatdata;
  updateRequest["requestId"] = "1";

  std::string expectedupdateVSSTreeAnswerString{R"(
    {
    "action": "updateVSSTree",  
    "requestId": "1" 
    }
    )"};
  jsoncons::json expectedupdateVSSTreeAnswer =
      jsoncons::json::parse(expectedupdateVSSTreeAnswerString);

  // We need to verify metadata has been added correctly
  std::string getMetadataRequestString{R"(
    {
    "action": "getMetaData",  
    "path": "Vehicle.Speed",
    "requestId": "1"
    }
    )"};

  std::string getMetadataResponseString{R"(
    {
    "action": "getMetaData",
    "metadata": {
        "Vehicle": {
        "children": {
            "Speed": {
            "datatype": "int32",
            "description": "Vehicle speed, as sensed by the gearbox.",
            "max": 9100,
            "min": -250,
            "type": "sensor",
            "unit": "km/h",
            "uuid": "1efc9a11943b5a15ac159786051c5836"
            }
        },
        "description": "High-level vehicle data.",
        "type": "branch",
        "uuid": "ccc825f94139544dbb5f4bfd033bece6"
        }
    },
    "requestId": "1"
    }
    )"};
  jsoncons::json getMetadataResponse =
      jsoncons::json::parse(getMetadataResponseString);

  // run updateVSSTree
  auto resStr = processor->processQuery(updateRequest.as_string(), channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("ts"));
  // Assign timestamp for comparision purposes
  expectedupdateVSSTreeAnswer.insert_or_assign("ts", res["ts"]);

  BOOST_TEST(res == expectedupdateVSSTreeAnswer);

  // verify metadata
  resStr = processor->processQuery(getMetadataRequestString, channel);
  res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("ts"));
  // Assign timestamp for comparision purposes
  getMetadataResponse.insert_or_assign("ts", res["ts"]);

  BOOST_TEST(res == getMetadataResponse);
}

/* Broken Request */
BOOST_AUTO_TEST_CASE(update_vss_tree_brokenrequest) {
  kuksa::kuksaChannel channel;
  channel.set_authorized(false);
  channel.set_modifytree(true);
  channel.set_connectionid(1);

  std::string brokenUpdateVSSTreeRequest{R"(
    {
    "action": "updateVSSTree",  
    "element": "this request has no metadata",
    "requestId": "1" 
    }
    )"};

  std::string expectedupdateVSSTreeAnswerString{R"(
  {
  "action": "updateVSSTree",
  "error": {
    "message": "Schema error: #: Required property \"metadata\" not found",
    "number": "400",
    "reason": "Bad Request"
  },
  "requestId": "1"
  }
    )"};
  jsoncons::json expectedupdateVSSTreeAnswer =
      jsoncons::json::parse(expectedupdateVSSTreeAnswerString);

  // run updateVSSTree
  auto resStr = processor->processQuery(brokenUpdateVSSTreeRequest, channel);
  auto res = json::parse(resStr);

  // Does result have a timestamp?
  BOOST_TEST(res.contains("ts"));
  // Assign timestamp for comparision purposes
  expectedupdateVSSTreeAnswer.insert_or_assign("ts", res["ts"]);

  BOOST_TEST(res == expectedupdateVSSTreeAnswer);
}

BOOST_AUTO_TEST_SUITE_END()
