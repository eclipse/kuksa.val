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


#include <jwt-cpp/jwt.h>

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <thread>
#include <fstream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "KuksaChannel.hpp"
#include "ILoggerMock.hpp"
#include "IVssDatabaseMock.hpp"
#include "IServerMock.hpp"
#include "IAuthenticatorMock.hpp"
#include "IAccessCheckerMock.hpp"
#include "JsonResponses.hpp"
#include "SubscriptionHandler.hpp"
#include "UnitTestHelpers.hpp"
#include "kuksa.pb.h"


namespace {
  // common resources for tests
  std::shared_ptr<ILoggerMock> logMock;
  std::shared_ptr<IVssDatabaseMock> dbMock;
  std::shared_ptr<IServerMock> serverMock;
  std::shared_ptr<IAuthenticatorMock> authMock;
  std::shared_ptr<IAccessCheckerMock> accCheckMock;

  std::unique_ptr<SubscriptionHandler> subHandler;

  // Pre-test initialization and post-test desctruction of common resources
  struct TestSuiteFixture {
    TestSuiteFixture() {
      logMock = std::make_shared<ILoggerMock>();
      dbMock = std::make_shared<IVssDatabaseMock>();
      serverMock = std::make_shared<IServerMock>();
      authMock = std::make_shared<IAuthenticatorMock>();
      accCheckMock = std::make_shared<IAccessCheckerMock>();

      MOCK_EXPECT(logMock->Log).at_least(0); // ignore log events
      subHandler = std::make_unique<SubscriptionHandler>(logMock, serverMock, authMock, accCheckMock);
    }
    ~TestSuiteFixture() {
      subHandler.reset();
    }
  };
}

// Define name of test suite and define test suite fixture for pre and post test handling
BOOST_FIXTURE_TEST_SUITE(SubscriptionHandlerTests, TestSuiteFixture)

BOOST_AUTO_TEST_CASE(Given_SingleClient_When_SubscribeRequest_Shall_SubscribeClient)
{
  KuksaChannel channel;

  // setup

  std::list<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']"};
  std::list<std::string> retDbListNarrower{"$['Vehicle']['children']['Drivetrain']['children']['Transmission']"};
  VSSPath vsspath = VSSPath::fromVSSGen1("Vehicle.Drivetrain");

  // expectations

  MOCK_EXPECT(dbMock->pathExists)
    .once()
    .with(vsspath)
    .returns(true);
  MOCK_EXPECT(dbMock->pathIsReadable)
    .once()
    .with(vsspath)
    .returns(true);
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .once()
    .with(mock::any, vsspath)
    .returns(true);


  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_release_latest.json");
  is >> dbMock->data_tree__;

  // verify

  SubscriptionId res;
  BOOST_CHECK_NO_THROW(res = subHandler->subscribe(channel, dbMock, vsspath.getVSSPath(), "value"));

  BOOST_TEST(res.version() == 4);
}

BOOST_AUTO_TEST_CASE(Given_SingleClient_When_SubscribeRequestOnDifferentPaths_Shall_SubscribeAll)
{
  KuksaChannel channel;
  unsigned paths = 4;

  // setup

  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']",
                                          "$['Vehicle']['children']['Acceleration']",
                                          "$['Vehicle']['children']['Media']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<VSSPath> vsspath{ VSSPath::fromVSSGen1("Vehicle.Drivetrain"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration"),
                          VSSPath::fromVSSGen1("Vehicle.Media"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Lateral") };


  // expectations

  for (unsigned index = 0; index < paths; index++) {
    MOCK_EXPECT(dbMock->pathExists)
      .once()
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(dbMock->pathIsReadable)
      .once()
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vsspath[index])
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_release_latest.json");
  is >> dbMock->data_tree__;

  // verify

  std::set<SubscriptionId> resSet;
  for (unsigned index = 0; index < paths; index++) {
    SubscriptionId res;

    BOOST_CHECK_NO_THROW(res = subHandler->subscribe(channel, dbMock, vsspath[index].getVSSPath(), "value"));
    BOOST_TEST(res.version() == 4);

    // check that the value is different from previously returned
    auto cmp = (resSet.find(res) == resSet.end());
    BOOST_TEST(cmp);
  }
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_SubscribeRequestOnSinglePath_Shall_SubscribeAllClients)
{
  std::vector<KuksaChannel> channels;
  unsigned clientNum = 4;
  // setup

  // add multiple channels to request subscriptions
  for (unsigned index = 0; index < clientNum; index++) {
    KuksaChannel ch;

    ch.setConnID(index);
    ch.setType(KuksaChannel::Type::WEBSOCKET_SSL);

    channels.push_back(std::move(ch));
  }

  std::list<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']"};
  std::list<std::string> retDbListNarrower{"$['Vehicle']['children']['Drivetrain']['children']['Transmission']"};
  VSSPath vsspath = VSSPath::fromVSSGen1("Vehicle.Drivetrain");

  // expectations

  MOCK_EXPECT(dbMock->pathExists)
    .exactly(clientNum)
    .with(vsspath)
    .returns(true);
  MOCK_EXPECT(dbMock->pathIsReadable)
    .exactly(clientNum)
    .with(vsspath)
    .returns(true);
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .exactly(clientNum)
    .with(mock::any, vsspath)
    .returns(true);

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_release_latest.json");
  is >> dbMock->data_tree__;

  // verify

  std::set<SubscriptionId> resSet;
  for (auto &ch : channels) {
    SubscriptionId res;

    BOOST_CHECK_NO_THROW(res = subHandler->subscribe(ch, dbMock, vsspath.getVSSPath(), "value"));
    BOOST_TEST(res.version() == 4);

    // check that the value is different from previously returned
    auto cmp = (resSet.find(res) == resSet.end());
    BOOST_TEST(cmp);
    resSet.insert(res);
  }
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_SubscribeRequestOnDifferentPaths_Shall_SubscribeAllClients)
{
  std::vector<KuksaChannel> channels;
  unsigned clientNum = 4;
  // setup

  // add multiple channels to request subscriptions
  for (unsigned index = 0; index < clientNum; index++) {
    KuksaChannel ch;

    ch.setConnID(index);
    ch.setType(KuksaChannel::Type::WEBSOCKET_SSL);

    channels.push_back(std::move(ch));
  }

  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']",
                                          "$['Vehicle']['children']['Acceleration']",
                                          "$['Vehicle']['children']['Media']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
    std::vector<VSSPath> vsspath{ VSSPath::fromVSSGen1("Vehicle.Drivetrain"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration"),
                          VSSPath::fromVSSGen1("Vehicle.Media"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Lateral") };

  // expectations

  for (unsigned index = 0; index < clientNum; index++) {
    MOCK_EXPECT(dbMock->pathExists)
      .once()
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(dbMock->pathIsReadable)
      .once()
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vsspath[index])
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_release_latest.json");
  is >> dbMock->data_tree__;

  // verify

  unsigned index = 0;
  std::set<SubscriptionId> resSet;
  for (auto &ch : channels) {
    SubscriptionId res;

    BOOST_CHECK_NO_THROW(res = subHandler->subscribe(ch, dbMock, vsspath[index].getVSSPath(), "value"));
    BOOST_TEST(res.version() == 4);

    // check that the value is different from previously returned
    auto cmp = (resSet.find(res) == resSet.end());
    BOOST_TEST(cmp);
    ++index;
  }
}

BOOST_AUTO_TEST_CASE(Given_SingleClient_When_UnsubscribeRequestOnDifferentPaths_Shall_UnsubscribeAll)
{
  KuksaChannel channel;
  unsigned paths = 4;

  // setup

  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']",
                                          "$['Vehicle']['children']['Acceleration']",
                                          "$['Vehicle']['children']['Media']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<VSSPath> vsspath{ VSSPath::fromVSSGen1("Vehicle.Drivetrain"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration"),
                          VSSPath::fromVSSGen1("Vehicle.Media"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Lateral") };

  // expectations

  for (unsigned index = 0; index < paths; index++) {
    MOCK_EXPECT(dbMock->pathExists)
      .once()
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(dbMock->pathIsReadable)
      .once()
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vsspath[index])
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_release_latest.json");
  is >> dbMock->data_tree__;

  // verify

  std::set<SubscriptionId> resSet;
  for (unsigned index = 0; index < paths; index++) {
    SubscriptionId res;

    BOOST_CHECK_NO_THROW(res = subHandler->subscribe(channel, dbMock, vsspath[index].getVSSPath(), "value"));
    BOOST_TEST(res.version() == 4);

    // check that the value is different from previously returned
    auto cmp = (resSet.find(res) == resSet.end());
    BOOST_TEST(cmp);
  }

  for (auto &subId : resSet) {
    BOOST_TEST(0 == subHandler->unsubscribe(subId));
  }
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_Unsubscribe_Shall_UnsubscribeAllClients)
{
  std::vector<KuksaChannel> channels;
  unsigned clientNum = 4;
  // setup

  // add multiple channels to request subscriptions
  for (unsigned index = 0; index < clientNum; index++) {
    KuksaChannel ch;

    ch.setConnID(index);
    ch.setType(KuksaChannel::Type::WEBSOCKET_SSL);

    channels.push_back(std::move(ch));
  }

  std::list<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']"};
  std::list<std::string> retDbListNarrower{"$['Vehicle']['children']['Drivetrain']['children']['Transmission']"};
  VSSPath vsspath = VSSPath::fromVSSGen1("Vehicle.Drivetrain");
  // expectations

  MOCK_EXPECT(dbMock->pathExists)
    .exactly(clientNum)
    .with(vsspath)
    .returns(true);
  MOCK_EXPECT(dbMock->pathIsReadable)
    .exactly(clientNum)
    .with(vsspath)
    .returns(true);
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .exactly(clientNum)
    .with(mock::any, vsspath)
    .returns(true);

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_release_latest.json");
  is >> dbMock->data_tree__;

  // verify

  std::set<SubscriptionId> resSet;
  for (auto &ch : channels) {
    SubscriptionId res;

    BOOST_CHECK_NO_THROW(res = subHandler->subscribe(ch, dbMock, vsspath.getVSSPath(), "value"));
    BOOST_TEST(res.version() == 4);

    // check that the value is different from previously returned
    auto cmp = (resSet.find(res) == resSet.end());
    BOOST_TEST(cmp);
    resSet.insert(res);
  }

  for (auto &subId : resSet) {
    BOOST_TEST(0 == subHandler->unsubscribe(subId));
  }
}

BOOST_AUTO_TEST_CASE(Given_SingleClient_When_MultipleSignalsSubscribedAndUpdated_Shall_NotifyClient)
{
  KuksaChannel channel;
  jsoncons::json answer;
  unsigned paths = 3;

  // setup

  channel.setConnID(131313);

  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Acceleration']['children']['Vertical']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Longitudinal']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<VSSPath> vsspath{ VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Longitudinal"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Lateral") };

  // expectations

  for (unsigned index = 0; index < paths; index++) {
    MOCK_EXPECT(dbMock->pathExists)
      .once()
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(dbMock->pathIsReadable)
      .once()
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vsspath[index])
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_release_latest.json");
  is >> dbMock->data_tree__;

  // verify

  std::map<unsigned, SubscriptionId> resMap;
  for (unsigned index = 0; index < paths; index++) {
    SubscriptionId subId;

    BOOST_CHECK_NO_THROW(subId = subHandler->subscribe(channel, dbMock, vsspath[index].getVSSPath(), "value"));

    BOOST_TEST(subId.version() == 4);
    resMap[index] = subId;
  }

  answer["action"] = "subscribe";
  answer["ts"] = JsonResponses::getTimeStamp();

  // verify that each updated signal for which single client is subscribed to is called
  for (unsigned index = 0; index < paths; index++) {
    answer["subscriptionId"] = boost::uuids::to_string(resMap[index]);
    jsoncons::json data = packDataInJson(vsspath[index], std::to_string(index));
    //because we force publishForVSSPath with no data (and no timestamp) in VSSDatabase
    data["dp"]["ts"] = JsonResponses::getTimeStampZero();
    answer["data"] = data;

    // custom verifier for returned JSON message
    auto jsonVerify = [&answer]( const std::string &actual ) {
      bool ret = false;
      jsoncons::json response = jsoncons::json::parse(actual);

      ret = (response["subscriptionId"] == answer["subscriptionId"]);

      if (ret) {
        ret = (response["data"]  == answer["data"]);
      }
      return ret;
    };


    MOCK_EXPECT(serverMock->SendToConnection)
      .once()
      .with(channel.getConnID(), jsonVerify)
      .returns(true);

    BOOST_TEST(subHandler->publishForVSSPath(vsspath[index], "float", "value", data) == 0);
    usleep(10000); // allow for subthread handler to run
  }
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_MultipleSignalsSubscribedAndUpdated_Shall_NotifyClients)
{
  std::vector<KuksaChannel> channels;
  jsoncons::json answer;
  unsigned paths = 3;
  unsigned channelCount = 3;

  // setup
  for (unsigned index = 0; index < channelCount; index++) {
    KuksaChannel channel;
    channel.setConnID(111100 + index);

    channels.push_back(std::move(channel));
  }

  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Acceleration']['children']['Vertical']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Longitudinal']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<VSSPath> vsspath{ VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Longitudinal"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Lateral") };

  // expectations

  for (unsigned index = 0; index < paths; index++) {
    MOCK_EXPECT(dbMock->pathExists)
      .exactly(channelCount)
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(dbMock->pathIsReadable)
      .exactly(channelCount)
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .exactly(channelCount)
      .with(mock::any, vsspath[index])
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_release_latest.json");
  is >> dbMock->data_tree__;

  // verify

  // subscribe every client to every signal
  std::map<SubscriptionId, unsigned> resMap;
  for (auto & ch : channels) {
    for (unsigned index = 0; index < paths; index++) {
      SubscriptionId subId;

      BOOST_CHECK_NO_THROW(subId = subHandler->subscribe(ch, dbMock, vsspath[index].getVSSPath(), "value"));

      BOOST_TEST(subId.version() == 4);
      resMap[subId] = index;
    }
  }

  answer["action"] = "subscription";
  answer["ts"] = JsonResponses::getTimeStamp();

  // custom verifier for returned JSON message
  auto jsonVerify = [&resMap, &answer]( const std::string &actual ) -> bool {
    bool ret = false;
    jsoncons::json response = jsoncons::json::parse(actual);
    ret = (answer["action"] == response["action"]);
    if (ret) {
      //because we force publishForVSSPath with no data (and no timestamp) in VSSDatabase
      ret = (response["data"]["dp"]["ts"].as<string>() == JsonResponses::getTimeStampZero());
    }

    if (ret) {
      auto subId = boost::uuids::string_generator()(response["subscriptionId"].as<std::string>());
      auto value = response["data"]["dp"]["value"].as<uint64_t>();
      // check if value match for given subscription id
      ret = (resMap[subId] == value);
    }
    return ret;
  };

  // set custom expectation handler for verifying signal update values
  for (auto & ch : channels) {
    MOCK_EXPECT(serverMock->SendToConnection)
      .exactly(paths)
      .with(ch.getConnID(), jsonVerify)
      .returns(true);
  }

  // call UUT
  for (unsigned index = 0; index < paths; index++) {
    BOOST_TEST(subHandler->publishForVSSPath(vsspath[index], "string", "value", packDataInJson(vsspath[index], std::to_string(index))) == 0);
    std::this_thread::yield();
  }
  sleep(1); // allow for subthread handler to run
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_MultipleSignalsSubscribedAndUpdatedAndClientUnsubscribeAll_Shall_NotifyOnlySubscribedClients)
{
  std::vector<KuksaChannel> channels;
  jsoncons::json answer;
  unsigned paths = 3;
  unsigned channelCount = 3;
  mock::sequence seq1, seq2;

  // setup
  for (unsigned index = 0; index < channelCount; index++) {
    KuksaChannel channel;
    channel.setConnID(111100 + index);

    channels.push_back(std::move(channel));
  }

  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Acceleration']['children']['Vertical']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Longitudinal']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<VSSPath> vsspath{ VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Longitudinal"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Lateral") };

  // expectations

  for (unsigned index = 0; index < paths; index++) {
    MOCK_EXPECT(dbMock->pathExists)
      .exactly(channelCount)
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(dbMock->pathIsReadable)
      .exactly(channelCount)
      .with(vsspath[index])
      .returns(true);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .exactly(channelCount)
      .with(mock::any, vsspath[index])
      .returns(true);
  }

    // subscribe every client to every signal
  std::map<SubscriptionId, unsigned> resMap;

  // custom verifier for returned JSON message
  auto jsonVerify = [&resMap, &answer]( const std::string &actual ) -> bool {
    bool ret = false;
    jsoncons::json response = jsoncons::json::parse(actual);

    BOOST_TEST(ret = (answer["action"] == response["action"]));
    if (ret) {
      BOOST_TEST(ret = (response["data"]["dp"]["ts"] != 0));
    }

    if (ret) {
      auto subId = boost::uuids::string_generator()(response["subscriptionId"].as<std::string>());
      auto value = response["data"]["dp"]["value"].as<uint64_t>();

      // check if value match for given subscription id
      BOOST_TEST(ret = (resMap[subId] == value));
    }
    return ret;
  };

    // set custom expectation handler for verifying signal update values
  for (auto & ch : channels) {
    MOCK_EXPECT(serverMock->SendToConnection)
      .exactly(paths)
      .with(ch.getConnID(), jsonVerify)
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_release_latest.json");
  is >> dbMock->data_tree__;

  // verify

  std::vector<SubscriptionId> removedSubId;
  unsigned channelIndex = 0;

  for (auto & ch : channels) {
    for (unsigned index = 0; index < paths; index++) {
      SubscriptionId subId;

      BOOST_CHECK_NO_THROW(subId = subHandler->subscribe(ch, dbMock, vsspath[index].getVSSPath(), "value"));

      BOOST_TEST(subId.version() == 4);
      resMap[subId] = index;

      // save subscription ids of client who will unsubscribe all later
      if (channelIndex == 0u ) {
        removedSubId.push_back(subId);
      }
    }
    ++channelIndex;
  }

  answer["action"] = "subscription";
  answer["ts"] = JsonResponses::getTimeStamp();

  // call UUT
  for (unsigned index = 0; index < paths; index++) {
    BOOST_TEST(subHandler->publishForVSSPath(vsspath[index], "double" ,"value", packDataInJson(vsspath[index], std::to_string(index))) == 0);
    std::this_thread::yield();
  }
  usleep(100000); // allow for subthread handler to run


  BOOST_TEST(subHandler->unsubscribeAll(channels.front()) == 0);
  channels.erase(channels.begin());

  // now prepare expectations for only left signals
  for (auto & subIdToRemove : removedSubId) {
    resMap.erase(subIdToRemove);
  }

  // set custom expectation handler for verifying signal update values
  for (auto & ch : channels) {
    MOCK_EXPECT(serverMock->SendToConnection)
      .exactly(paths)
      .with(ch.getConnID(), jsonVerify)
      .returns(true);
  }

  // call UUT to verify if removed channel is not called anymore
  for (unsigned index = 0; index < paths; index++) {
    BOOST_TEST(subHandler->publishForVSSPath(vsspath[index], "string", "value", packDataInJson(vsspath[index], std::to_string(index))) == 0);
    std::this_thread::yield();
  }
  usleep(100000); // allow for subthread handler to run
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_MultipleSignalsSubscribedAndUpdatedAndClientUnsubscribeAll_Shall_NotifyOnlySubscribedClient) {
  unsigned index = 0;
  VSSPath vsspath = VSSPath::fromVSSGen1("Vehicle.Acceleration.Vertical");
  BOOST_TEST(subHandler->publishForVSSPath(vsspath, "int16", "value", packDataInJson(vsspath, std::to_string(index))) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
