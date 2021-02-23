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


#include <jwt-cpp/jwt.h>

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <list>
#include <thread>

#include "WsChannel.hpp"
#include "ILoggerMock.hpp"
#include "IVssDatabaseMock.hpp"
#include "IServerMock.hpp"
#include "IAuthenticatorMock.hpp"
#include "IAccessCheckerMock.hpp"
#include "JsonResponses.hpp"

#include "SubscriptionHandler.hpp"


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
  WsChannel channel;

  // setup

  std::list<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']"};
  std::list<std::string> retDbListNarrower{"$['Vehicle']['children']['Drivetrain']['children']['Transmission']"};
  std::string path{"Vehicle.Drivetrain.*"};
  VSSPath vsspath = VSSPath::fromVSSGen1("Vehicle.Drivetrain");

  // expectations

  MOCK_EXPECT(dbMock->getVSSSpecificPath)
    .once()
    .with(mock::equal(path), mock::assign(true), mock::any)
    .returns(retDbListWider.front());
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .once()
    .with(mock::any, vsspath)
    .returns(true);

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_rel_2.0.json");
  is >> dbMock->data_tree__;

  // verify

  SubscriptionId res;
  BOOST_CHECK_NO_THROW(res = subHandler->subscribe(channel, dbMock, path));

  BOOST_TEST(res != 0u);
}

BOOST_AUTO_TEST_CASE(Given_SingleClient_When_SubscribeRequestOnDifferentPaths_Shall_SubscribeAll)
{
  WsChannel channel;
  unsigned paths = 4;

  // setup

  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']",
                                          "$['Vehicle']['children']['Acceleration']",
                                          "$['Vehicle']['children']['Media']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<std::string> path{"Vehicle.Drivetrain.*",
                                "Vehicle.Acceleration.*",
                                "Vehicle.Media.*",
                                "Vehicle.Acceleration.Lateral" };
  std::vector<VSSPath> vss_acces_path{ VSSPath::fromVSSGen1("Vehicle.Drivetrain"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration"),
                          VSSPath::fromVSSGen1("Vehicle.Media"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Lateral") };
  

  // expectations

  for (unsigned index = 0; index < paths; index++) {
    MOCK_EXPECT(dbMock->getVSSSpecificPath)
      .once()
      .with(mock::equal(path[index]), mock::assign((index == 3 ? false : true)), mock::any)
      .returns(retDbListWider[index]);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vss_acces_path[index])
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_rel_2.0.json");
  is >> dbMock->data_tree__;

  // verify

  std::set<SubscriptionId> resSet;
  for (unsigned index = 0; index < paths; index++) {
    SubscriptionId res;

    BOOST_CHECK_NO_THROW(res = subHandler->subscribe(channel, dbMock, path[index]));
    BOOST_TEST(res != 0u);

    // check that the value is different from previously returned
    auto cmp = (resSet.find(res) == resSet.end());
    BOOST_TEST(cmp);
  }
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_SubscribeRequestOnSinglePath_Shall_SubscribeAllClients)
{
  std::vector<WsChannel> channels;
  unsigned clientNum = 4;
  // setup

  // add multiple channels to request subscriptions
  for (unsigned index = 0; index < clientNum; index++) {
    WsChannel ch;

    ch.setConnID(index);
    ch.setType(WsChannel::Type::WEBSOCKET_SSL);

    channels.push_back(std::move(ch));
  }

  std::list<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']"};
  std::list<std::string> retDbListNarrower{"$['Vehicle']['children']['Drivetrain']['children']['Transmission']"};
  std::string path{"Vehicle.Drivetrain.*"};
  VSSPath vsspath = VSSPath::fromVSSGen1("Vehicle.Drivetrain");

  // expectations

  MOCK_EXPECT(dbMock->getVSSSpecificPath)
    .exactly(clientNum)
    .with(mock::equal(path), mock::assign(true), mock::any)
    .returns(retDbListWider.front());
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .exactly(clientNum)
    .with(mock::any, vsspath)
    .returns(true);

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_rel_2.0.json");
  is >> dbMock->data_tree__;

  // verify

  std::set<SubscriptionId> resSet;
  for (auto &ch : channels) {
    SubscriptionId res;

    BOOST_CHECK_NO_THROW(res = subHandler->subscribe(ch, dbMock, path));
    BOOST_TEST(res != 0u);

    // check that the value is different from previously returned
    auto cmp = (resSet.find(res) == resSet.end());
    BOOST_TEST(cmp);
    resSet.insert(res);
  }
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_SubscribeRequestOnDifferentPaths_Shall_SubscribeAllClients)
{
  std::vector<WsChannel> channels;
  unsigned clientNum = 4;
  // setup

  // add multiple channels to request subscriptions
  for (unsigned index = 0; index < clientNum; index++) {
    WsChannel ch;

    ch.setConnID(index);
    ch.setType(WsChannel::Type::WEBSOCKET_SSL);

    channels.push_back(std::move(ch));
  }

  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']",
                                          "$['Vehicle']['children']['Acceleration']",
                                          "$['Vehicle']['children']['Media']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<std::string> path{"Vehicle.Drivetrain.*",
                                "Vehicle.Acceleration.*",
                                "Vehicle.Media.*",
                                "Vehicle.Acceleration.Lateral" };
    std::vector<VSSPath> vss_acces_path{ VSSPath::fromVSSGen1("Vehicle.Drivetrain"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration"),
                          VSSPath::fromVSSGen1("Vehicle.Media"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Lateral") };

  // expectations

  for (unsigned index = 0; index < clientNum; index++) {
    MOCK_EXPECT(dbMock->getVSSSpecificPath)
      .once()
      .with(mock::equal(path[index]), mock::assign((index == 3 ? false : true)), mock::any)
      .returns(retDbListWider[index]);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vss_acces_path[index])
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_rel_2.0.json");
  is >> dbMock->data_tree__;

  // verify

  unsigned index = 0;
  std::set<SubscriptionId> resSet;
  for (auto &ch : channels) {
    SubscriptionId res;

    BOOST_CHECK_NO_THROW(res = subHandler->subscribe(ch, dbMock, path[index]));
    BOOST_TEST(res != 0u);

    // check that the value is different from previously returned
    auto cmp = (resSet.find(res) == resSet.end());
    BOOST_TEST(cmp);
    ++index;
  }
}

BOOST_AUTO_TEST_CASE(Given_SingleClient_When_UnsubscribeRequestOnDifferentPaths_Shall_UnsubscribeAll)
{
  WsChannel channel;
  unsigned paths = 4;

  // setup

  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']",
                                          "$['Vehicle']['children']['Acceleration']",
                                          "$['Vehicle']['children']['Media']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<std::string> path{"Vehicle.Drivetrain.*",
                                "Vehicle.Acceleration.*",
                                "Vehicle.Media.*",
                                "Vehicle.Acceleration.Lateral" };
  std::vector<VSSPath> vss_acces_path{ VSSPath::fromVSSGen1("Vehicle.Drivetrain"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration"),
                          VSSPath::fromVSSGen1("Vehicle.Media"),
                          VSSPath::fromVSSGen1("Vehicle.Acceleration.Lateral") };

  // expectations

  for (unsigned index = 0; index < paths; index++) {
    MOCK_EXPECT(dbMock->getVSSSpecificPath)
      .once()
      .with(mock::equal(path[index]), mock::assign((index == 3 ? false : true)), mock::any)
      .returns(retDbListWider[index]);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, vss_acces_path[index])
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_rel_2.0.json");
  is >> dbMock->data_tree__;

  // verify

  std::set<SubscriptionId> resSet;
  for (unsigned index = 0; index < paths; index++) {
    SubscriptionId res;

    BOOST_CHECK_NO_THROW(res = subHandler->subscribe(channel, dbMock, path[index]));
    BOOST_TEST(res != 0u);

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
  std::vector<WsChannel> channels;
  unsigned clientNum = 4;
  // setup

  // add multiple channels to request subscriptions
  for (unsigned index = 0; index < clientNum; index++) {
    WsChannel ch;

    ch.setConnID(index);
    ch.setType(WsChannel::Type::WEBSOCKET_SSL);

    channels.push_back(std::move(ch));
  }

  std::list<std::string> retDbListWider{"$['Vehicle']['children']['Drivetrain']"};
  std::list<std::string> retDbListNarrower{"$['Vehicle']['children']['Drivetrain']['children']['Transmission']"};
  std::string path{"Vehicle.Drivetrain.*"};
  VSSPath vsspath = VSSPath::fromVSSGen1("Vehicle.Drivetrain");
  // expectations

  MOCK_EXPECT(dbMock->getVSSSpecificPath)
    .exactly(clientNum)
    .with(mock::equal(path), mock::assign(true), mock::any)
    .returns(retDbListWider.front());
  MOCK_EXPECT(accCheckMock->checkReadAccess)
    .exactly(clientNum)
    .with(mock::any, vsspath)
    .returns(true);

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_rel_2.0.json");
  is >> dbMock->data_tree__;

  // verify

  std::set<SubscriptionId> resSet;
  for (auto &ch : channels) {
    SubscriptionId res;

    BOOST_CHECK_NO_THROW(res = subHandler->subscribe(ch, dbMock, path));
    BOOST_TEST(res != 0u);

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
  WsChannel channel;
  jsoncons::json answer;
  unsigned paths = 3;

  // setup

  channel.setConnID(131313);

  std::vector<std::string> uuids{"9521e8d36a9b546d9414a779f5dd9bef",
                                 "c83f0c12653b5e7baf000799052f5533",
                                 "5c28427f79ca5fe394b47fe057a2af9b"};
  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Acceleration']['children']['Vertical']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Longitudinal']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<std::string> path{"Vehicle.Acceleration.Vertical",
                                "Vehicle.Acceleration.Longitudinal",
                                "Vehicle.Acceleration.Lateral" };

  // expectations

  for (unsigned index = 0; index < paths; index++) {
    MOCK_EXPECT(dbMock->getVSSSpecificPath)
      .once()
      .with(mock::equal(path[index]), mock::assign(true), mock::any)
      .returns(retDbListWider[index]);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .once()
      .with(mock::any, VSSPath::fromVSSGen1(path[index]))
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_rel_2.0.json");
  is >> dbMock->data_tree__;

  // verify

  std::map<unsigned, SubscriptionId> resMap;
  for (unsigned index = 0; index < paths; index++) {
    SubscriptionId subId;

    BOOST_CHECK_NO_THROW(subId = subHandler->subscribe(channel, dbMock, path[index]));

    BOOST_TEST(subId != 0u);
    resMap[index] = subId;
  }

  answer["action"] = "subscribe";
  answer["timestamp"] = JsonResponses::getTimeStamp();

  // verify that each updated signal for which single client is subscribed to is called
  for (unsigned index = 0; index < paths; index++) {
    answer["subscriptionId"] = resMap[index];
    answer["value"] = index;

    // custom verifier for returned JSON message
    auto jsonVerify = [&answer]( const std::string &actual ) {
      bool ret = false;
      jsoncons::json response = jsoncons::json::parse(actual);

      ret = (response["subscriptionId"] == answer["subscriptionId"]);
      if (ret) {
        ret = (response["value"]  == answer["value"]);
      }
      return ret;
    };

    MOCK_EXPECT(serverMock->SendToConnection)
      .once()
      .with(channel.getConnID(), jsonVerify);

    BOOST_TEST(subHandler->updateByUUID(uuids[index], index) == 0);
    usleep(10000); // allow for subthread handler to run
  }
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_MultipleSignalsSubscribedAndUpdated_Shall_NotifyClients)
{
  std::vector<WsChannel> channels;
  jsoncons::json answer;
  unsigned paths = 3;
  unsigned channelCount = 3;

  // setup
  for (unsigned index = 0; index < channelCount; index++) {
    WsChannel channel;
    channel.setConnID(111100 + index);

    channels.push_back(std::move(channel));
  }

  std::vector<std::string> uuids{"9521e8d36a9b546d9414a779f5dd9bef",
                                 "c83f0c12653b5e7baf000799052f5533",
                                 "5c28427f79ca5fe394b47fe057a2af9b"};
  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Acceleration']['children']['Vertical']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Longitudinal']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<std::string> path{"Vehicle.Acceleration.Vertical",
                                "Vehicle.Acceleration.Longitudinal",
                                "Vehicle.Acceleration.Lateral" };

  // expectations

  for (unsigned index = 0; index < paths; index++) {
    MOCK_EXPECT(dbMock->getVSSSpecificPath)
      .exactly(channelCount)
      .with(mock::equal(path[index]), mock::assign(true), mock::any)
      .returns(retDbListWider[index]);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .exactly(channelCount)
      .with(mock::any, VSSPath::fromVSSGen1(path[index]))
      .returns(true);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_rel_2.0.json");
  is >> dbMock->data_tree__;

  // verify

  // subscribe every client to every signal
  std::map<SubscriptionId, unsigned> resMap;
  for (auto & ch : channels) {
    for (unsigned index = 0; index < paths; index++) {
      SubscriptionId subId;

      BOOST_CHECK_NO_THROW(subId = subHandler->subscribe(ch, dbMock, path[index]));

      BOOST_TEST(subId != 0u);
      resMap[subId] = index;
    }
  }

  answer["action"] = "subscribe";
  answer["timestamp"] = JsonResponses::getTimeStamp();

  // custom verifier for returned JSON message
  auto jsonVerify = [&resMap, &answer]( const std::string &actual ) -> bool {
    bool ret = false;
    jsoncons::json response = jsoncons::json::parse(actual);

    ret = (answer["action"] == response["action"]);
    if (ret) {
      ret = (response["timestamp"] >= answer["timestamp"] );
    }

    if (ret) {
      auto subId = response["subscriptionId"].as<uint64_t>();
      auto value = response["value"].as<uint64_t>();
      // check if value match for given subscription id
      ret = (resMap[subId] == value);
    }
    return ret;
  };

  // set custom expectation handler for verifying signal update values
  for (auto & ch : channels) {
    MOCK_EXPECT(serverMock->SendToConnection)
      .exactly(paths)
      .with(ch.getConnID(), jsonVerify);
  }

  // call UUT
  for (unsigned index = 0; index < paths; index++) {
    BOOST_TEST(subHandler->updateByUUID(uuids[index], index) == 0);
    std::this_thread::yield();
  }
  usleep(100000); // allow for subthread handler to run
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_MultipleSignalsSubscribedAndUpdatedAndClientUnsubscribeAll_Shall_NotifyOnlySubscribedClients)
{
  std::vector<WsChannel> channels;
  jsoncons::json answer;
  unsigned paths = 3;
  unsigned channelCount = 3;
  mock::sequence seq1, seq2;

  // setup
  for (unsigned index = 0; index < channelCount; index++) {
    WsChannel channel;
    channel.setConnID(111100 + index);

    channels.push_back(std::move(channel));
  }

  std::vector<std::string> uuids{"9521e8d36a9b546d9414a779f5dd9bef",
                                 "c83f0c12653b5e7baf000799052f5533",
                                 "5c28427f79ca5fe394b47fe057a2af9b"};
  std::vector<std::string> retDbListWider{"$['Vehicle']['children']['Acceleration']['children']['Vertical']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Longitudinal']",
                                          "$['Vehicle']['children']['Acceleration']['children']['Lateral']"};
  std::vector<std::string> path{"Vehicle.Acceleration.Vertical",
                                "Vehicle.Acceleration.Longitudinal",
                                "Vehicle.Acceleration.Lateral" };

  // expectations

  for (unsigned index = 0; index < paths; index++) {
    MOCK_EXPECT(dbMock->getVSSSpecificPath)
      .exactly(channelCount)
      .with(mock::equal(path[index]), mock::assign(true), mock::any)
      .returns(retDbListWider[index]);
    MOCK_EXPECT(accCheckMock->checkReadAccess)
      .exactly(channelCount)
      .with(mock::any, VSSPath::fromVSSGen1(path[index]))
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
      BOOST_TEST(ret = (response["timestamp"] != 0));
    }

    if (ret) {
      auto subId = response["subscriptionId"].as<uint64_t>();
      auto value = response["value"].as<uint64_t>();

      // check if value match for given subscription id
      BOOST_TEST(ret = (resMap[subId] == value));
    }
    return ret;
  };

    // set custom expectation handler for verifying signal update values
  for (auto & ch : channels) {
    MOCK_EXPECT(serverMock->SendToConnection)
      .exactly(paths)
      .with(ch.getConnID(), jsonVerify);
  }

  // load data tree
  // TODO: remove when this implicit dependency is removed
  std::ifstream is("test_vss_rel_2.0.json");
  is >> dbMock->data_tree__;

  // verify

  std::vector<SubscriptionId> removedSubId;
  unsigned channelIndex = 0;

  for (auto & ch : channels) {
    for (unsigned index = 0; index < paths; index++) {
      SubscriptionId subId;

      BOOST_CHECK_NO_THROW(subId = subHandler->subscribe(ch, dbMock, path[index]));

      BOOST_TEST(subId != 0u);
      resMap[subId] = index;

      // save subscription ids of client who will unsubscribe all later
      if (channelIndex == 0u ) {
        removedSubId.push_back(subId);
      }
    }
    ++channelIndex;
  }

  answer["action"] = "subscribe";
  answer["timestamp"] = JsonResponses::getTimeStamp();

  // call UUT
  for (unsigned index = 0; index < paths; index++) {
    BOOST_TEST(subHandler->updateByUUID(uuids[index], index) == 0);
    std::this_thread::yield();
  }
  usleep(100000); // allow for subthread handler to run


  BOOST_TEST(subHandler->unsubscribeAll(channels.begin()->getConnID()) == 0);
  channels.erase(channels.begin());

  // now prepare expectations for only left signals
  for (auto & subIdToRemove : removedSubId) {
    resMap.erase(subIdToRemove);
  }

  // set custom expectation handler for verifying signal update values
  for (auto & ch : channels) {
    MOCK_EXPECT(serverMock->SendToConnection)
      .exactly(paths)
      .with(ch.getConnID(), jsonVerify);
  }

  // call UUT to verify if removed channel is not called anymore
  for (unsigned index = 0; index < paths; index++) {
    BOOST_TEST(subHandler->updateByUUID(uuids[index], index) == 0);
    std::this_thread::yield();
  }
  usleep(100000); // allow for subthread handler to run
}

BOOST_AUTO_TEST_CASE(Given_MultipleClients_When_MultipleSignalsSubscribedAndUpdatedAndClientUnsubscribeAll_Shall_NotifyOnlySubscribedClient) {
    std::vector<std::string> uuids{"9521e8d36a9b546d9414a779f5dd9bef",
                                 "c83f0c12653b5e7baf000799052f5533",
                                 "5c28427f79ca5fe394b47fe057a2af9b"};
  unsigned index = 0;
BOOST_TEST(subHandler->updateByUUID(uuids[index], index) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
