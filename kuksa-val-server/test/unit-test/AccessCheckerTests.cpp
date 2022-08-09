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


#include <memory>
#include <string>

#include "KuksaChannel.hpp"
#include "IAuthenticatorMock.hpp"
#include "exception.hpp"
#include "kuksa.pb.h"

#include "AccessChecker.hpp"

namespace {
  // common resources for tests
  std::shared_ptr<IAuthenticatorMock> authMock;

  std::unique_ptr<AccessChecker> accChecker;

  // Pre-test initialization and post-test desctruction of common resources
  struct TestSuiteFixture {
    TestSuiteFixture() {
      authMock = std::make_shared<IAuthenticatorMock>();

      accChecker = std::make_unique<AccessChecker>(authMock);
    }
    ~TestSuiteFixture() {
      accChecker.reset();
    }
  };
}

// Define name of test suite and define test suite fixture for pre and post test handling
BOOST_FIXTURE_TEST_SUITE(AccessCheckerTests, TestSuiteFixture)

BOOST_AUTO_TEST_CASE(Given_AuthorizedChannel_When_ReadPathAuthorized_Shall_ReturnTrue) {
  KuksaChannel channel;
  jsoncons::json permissions;
  
  // setup
  permissions.insert_or_assign("Vehicle.Acceleration.Vertical", "rw");
  permissions.insert_or_assign("Vehicle.Acceleration.Longitudinal", "rw");
  permissions.insert_or_assign("Vehicle.Acceleration.Lateral", "r");

  VSSPath path  = VSSPath::fromJSON("$['Vehicle']['children']['Acceleration']['children']['Vertical']", false);

  std::string channelPermissions;
  permissions.dump_pretty(channelPermissions);
 
  channel.setConnID(11);
  channel.setAuthorized(true);
  channel.setPermissions(channelPermissions);

  // verify
  BOOST_TEST(accChecker->checkReadAccess(channel, path) == true);
}

BOOST_AUTO_TEST_CASE(Given_AuthorizedChannel_When_ReadPathNotAuthorized_Shall_ReturnFalse) {
  KuksaChannel channel;
  jsoncons::json permissions;
  
  // setup
  permissions.insert_or_assign("Vehicle.Acceleration.Vertical", "w");
  permissions.insert_or_assign("Vehicle.Acceleration.Longitudinal", "rw");
  permissions.insert_or_assign("Vehicle.Acceleration.Lateral", "r");

  VSSPath path = VSSPath::fromJSON("$['Vehicle']['children']['Acceleration']['children']['Vertical']", false);

  std::string channelPermissions;
  permissions.dump_pretty(channelPermissions);
 
  channel.setConnID(11);
  channel.setAuthorized(true);
  channel.setPermissions(channelPermissions);

  // verify
  BOOST_TEST(accChecker->checkReadAccess(channel, path) == false);
}

BOOST_AUTO_TEST_CASE(Given_AuthorizedChannel_When_ReadPathNotExistent_Shall_ReturnFalse) {
  KuksaChannel channel;
  jsoncons::json permissions;
  
  // setup
  permissions.insert_or_assign("Vehicle.Acceleration.Vertical", "w");
  permissions.insert_or_assign("Vehicle.Acceleration.Longitudinal", "rw");
  permissions.insert_or_assign("Vehicle.Acceleration.Lateral", "r");

  VSSPath path = VSSPath::fromJSON("$['Vehicle']['children']['Dummy']['children']['Leaf']", false);

  std::string channelPermissions;
  permissions.dump_pretty(channelPermissions);
 
  channel.setConnID(11);
  channel.setAuthorized(true);
  channel.setPermissions(channelPermissions);

  // verify
  BOOST_TEST(accChecker->checkReadAccess(channel, path) == false);
}

BOOST_AUTO_TEST_CASE(Given_AuthorizedChannel_When_WritePathAuthorized_Shall_ReturnTrue) {
  KuksaChannel channel;
  jsoncons::json permissions;
  
  // setup
  permissions.insert_or_assign("Vehicle.Acceleration.Vertical", "rw");
  permissions.insert_or_assign("Vehicle.Acceleration.Longitudinal", "rw");
  permissions.insert_or_assign("Vehicle.Acceleration.Lateral", "r");

  VSSPath path=VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");

  std::string channelPermissions;
  permissions.dump_pretty(channelPermissions);
 
  channel.setConnID(11);
  channel.setAuthorized(true);
  channel.setPermissions(channelPermissions);

  // verify
  BOOST_TEST(accChecker->checkWriteAccess(channel, path) == true);
}

BOOST_AUTO_TEST_CASE(Given_AuthorizedChannel_When_WritePathNotAuthorized_Shall_ReturnFalse) {
  KuksaChannel channel;
  jsoncons::json permissions;

  // setup
  permissions.insert_or_assign("Vehicle.Acceleration.Vertical", "r");
  permissions.insert_or_assign("Vehicle.Acceleration.Longitudinal", "rw");
  permissions.insert_or_assign("Vehicle.Acceleration.Lateral", "r");

  VSSPath path = VSSPath::fromVSSGen2("Vehicle.Acceleration.Vertical");

  std::string channelPermissions;
  permissions.dump_pretty(channelPermissions);
 
  channel.setConnID(11);
  channel.setAuthorized(true);
  channel.setPermissions(channelPermissions);

  // verify
  BOOST_TEST(accChecker->checkWriteAccess(channel, path) == false);
}

BOOST_AUTO_TEST_CASE(Given_AuthorizedChannel_When_WritePathNotExistent_Shall_ReturnFalse) {
  KuksaChannel channel;
  jsoncons::json permissions;

  // setup
  permissions.insert_or_assign("$['Vehicle']['children']['Acceleration']['children']['Vertical']", "w");
  permissions.insert_or_assign("$['Vehicle']['children']['Acceleration']['children']['Longitudinal']", "rw");
  permissions.insert_or_assign("$['Vehicle']['children']['Acceleration']['children']['Lateral']", "r");

  VSSPath path = VSSPath::fromVSSGen2("Vehicle.Dummy.Leaf");

  std::string channelPermissions;
  permissions.dump_pretty(channelPermissions);
 
  channel.setConnID(11);
  channel.setAuthorized(true);
  channel.setPermissions(channelPermissions);

  // verify

  BOOST_TEST(accChecker->checkWriteAccess(channel, path) == false);
}

BOOST_AUTO_TEST_CASE(Given_AuthorizedChannel_When_PathWriteForAllPathsValid_Shall_ReturnTrue) {
  KuksaChannel channel;
  jsoncons::json jsonPaths, permissions;
  std::string paths;

  // setup


  permissions.insert_or_assign("Vehicle.Acceleration.Vertical", "w");
  permissions.insert_or_assign("Vehicle.Acceleration.Longitudinal", "rw");
  permissions.insert_or_assign("Vehicle.Acceleration.Lateral", "r");

  std::string channelPermissions;
  permissions.dump_pretty(channelPermissions);
 
  channel.setConnID(11);
  channel.setAuthorized(true);
  channel.setPermissions(channelPermissions);

  paths = "[{\"path\":\"$['Vehicle']['children']['Acceleration']['children']['Vertical']\",\"value\":1},"
           "{\"path\":\"$['Vehicle']['children']['Acceleration']['children']['Longitudinal']\",\"value\":2}"
           "]";

  jsonPaths = jsoncons::json::parse(paths);

  // verify

  BOOST_TEST(accChecker->checkPathWriteAccess(channel, jsonPaths) == true);
}

BOOST_AUTO_TEST_CASE(Given_AuthorizedChannel_When_PathWriteForAllPathsNotValid_Shall_ReturnFalse) {
  KuksaChannel channel;
  jsoncons::json jsonPaths, permissions;
  std::string paths;

  // setup

  permissions.insert_or_assign("$['Vehicle']['children']['Acceleration']['children']['Vertical']", "w");
  permissions.insert_or_assign("$['Vehicle']['children']['Acceleration']['children']['Lateral']", "r");

  std::string channelPermissions;
  permissions.dump_pretty(channelPermissions);
 
  channel.setConnID(11);
  channel.setAuthorized(true);
  channel.setPermissions(channelPermissions);

  paths = "[{\"path\":\"$['Vehicle']['children']['Acceleration']['children']['Vertical']\",\"value\":1},"
           "{\"path\":\"$['Vehicle']['children']['Acceleration']['children']['Longitudinal']\",\"value\":2}"
           "]";

  jsonPaths = jsoncons::json::parse(paths);

  // verify

  BOOST_TEST(accChecker->checkPathWriteAccess(channel, jsonPaths) == false);
}


BOOST_AUTO_TEST_CASE(Given_AuthorizedChannel_When_PathWriteForAllPathsNotHaveWritePerm_Shall_ReturnFalse) {
  KuksaChannel channel;
  jsoncons::json jsonPaths, permissions;
  std::string paths;

  // setup

  permissions.insert_or_assign("$['Vehicle']['children']['Acceleration']['children']['Vertical']", "wr");
  permissions.insert_or_assign("$['Vehicle']['children']['Acceleration']['children']['Longitudinal']", "r");

  std::string channelPermissions;
  permissions.dump_pretty(channelPermissions);
 
  channel.setConnID(11);
  channel.setAuthorized(true);
  channel.setPermissions(channelPermissions);

  paths = "[{\"path\":\"$['Vehicle']['children']['Acceleration']['children']['Vertical']\",\"value\":1},"
           "{\"path\":\"$['Vehicle']['children']['Acceleration']['children']['Longitudinal']\",\"value\":2}"
           "]";

  jsonPaths = jsoncons::json::parse(paths);

  // verify

  BOOST_TEST(accChecker->checkPathWriteAccess(channel, jsonPaths) == false);
}

BOOST_AUTO_TEST_SUITE_END()
