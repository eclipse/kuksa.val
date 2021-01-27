/*
 * ******************************************************************************
 * Copyright (c) 2019-2020 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *    Robert Bosch GmbH - initial API and functionality
 *	   Expleo Germany - further tests and assertion checks
 * *****************************************************************************
 */
#define BOOST_TEST_MODULE kuksaval-unit-test

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/test/included/unit_test.hpp>
//can not undefine here, needs to be on for whole compilation unit to prevent warning
 

#include <string>
#include <memory>
#include <limits>
#include "KuksavalUnitTest.hpp"
#include "exception.hpp"


#include "IAccessCheckerMock.hpp"
#include "Authenticator.hpp"
#include "AccessChecker.hpp"
#include "SigningHandler.hpp"
#include "SubscriptionHandler.hpp"
#include "VssDatabase.hpp"
#include "VssCommandProcessor.hpp"
#include "WsChannel.hpp"
#include "ILogger.hpp"
#include "BasicLogger.hpp"
#include "IServerMock.hpp"
#include "IClientMock.hpp"


namespace utf = boost::unit_test;
// using namespace jsoncons;
// using namespace jsoncons::jsonpath;
// using jsoncons::json;

/* AUTH_JWT permission token looks like this
{
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Signal.OBD.*": "rw",
    "Signal.Chassis.Axle.*": "rw",
    "Vehicle.Drivetrain.*": "rw",
    "Signal.Cabin.Infotainment.*": "rw"
  }
}

This has be signed using a local private/pub key pair using RS256 Algorithm (asymetric). You could create one using www.jwt.io website.

*/
#define AUTH_JWT "eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJzdWIiOiJrdWtzYS52YWwiLCJpc3MiOiJFY2xpcHNlIEtVS1NBIERldiIsImFkbWluIjp0cnVlLCJtb2RpZnlUcmVlIjp0cnVlLCJpYXQiOjE1MTYyMzkwMjIsImV4cCI6MTc2NzIyNTU5OSwia3Vrc2EtdnNzIjp7IioiOiJydyJ9fQ.p2cnFGH16QoQ14l6ljPVKggFXZKmD-vrw8G6Vs6DvAokjsUG8FHh-F53cMsE-GDjyZH_1_CrlDCnbGlqjsFbgAylqA7IAJWp9_N6dL5p8DHZTwlZ4IV8L1CtCALs7XVqvcQKHCCzB63Y8PgVDCAqpQSRb79JPVD4pZwkBKpOknfEY5y9wfbswZiRKdgz7o61_oFnd-yywpse-23HD6v0htThVF1SuGL1PuvGJ8p334nt9bpkZO3gaTh1xVD_uJMwHzbuBCF33_f-I5QMZO6bVooXqGfe1zvl3nDrPEjq1aPulvtP8RgREYEqE6b2hB8jouTiC_WpE3qrdMw9sfWGFbm04qC-2Zjoa1yYSXoxmYd0SnliSYHAad9aXoEmFENezQV-of7sc-NX1-2nAXRAEhaqh0IRuJwB4_sG7SvQmnanwkz-sBYxKqkoFpOsZ6hblgPDOPYY2NAsZlYkjvAL2mpiInrsmY_GzGsfwPeAx31iozImX75rao8rm-XucAmCIkRlpBz6MYKCjQgyRz3UtZCJ2DYF4lKqTjphEAgclbYZ7KiCuTn9HualwtEmVzHHFneHMKl7KnRQk-9wjgiyQ5nlsVpCCblg6JKr9of4utuPO3cBvbjhB4_ueQ40cpWVOICcOLS7_w0i3pCq1ZKDEMrYDJfz87r2sU9kw1zeFQk";

namespace bt = boost::unit_test;
#define PORT 8090

std::shared_ptr<ILogger> logger;
std::shared_ptr<ISubscriptionHandler> subhandler;
std::shared_ptr<IAuthenticator> authhandler;
std::shared_ptr<IAccessCheckerMock> accesshandler;
std::shared_ptr<IVssDatabase> database;
std::shared_ptr<ISigningHandler> json_signer;
std::shared_ptr<IVssCommandProcessor> commandProc;
std::shared_ptr<IServer> httpServer;
std::shared_ptr<IClientMock> mqttClient;

std::shared_ptr<VssCommandProcessor> commandProc_auth;


kuksavalunittest unittestObj;

kuksavalunittest::kuksavalunittest() {
  std::string docRoot{"/vss/api/v1/"};

  logger = std::make_shared<BasicLogger>(static_cast<uint8_t>(LogLevel::NONE));
  // we do not need actual implementation of server, so use mock
  httpServer = std::make_shared<IServerMock>();
  mqttClient = std::make_shared<IClientMock>();
  string jwtPubkey=Authenticator::getPublicKeyFromFile("jwt.key.pub",logger);
  authhandler = std::make_shared<Authenticator>(logger, jwtPubkey,"RS256");
  accesshandler = std::make_shared<IAccessCheckerMock>();
  subhandler = std::make_shared<SubscriptionHandler>(logger, httpServer, mqttClient, authhandler, accesshandler);
  database = std::make_shared<VssDatabase>(logger, subhandler, accesshandler);
  commandProc = std::make_shared<VssCommandProcessor>(logger, database, authhandler , subhandler);
  json_signer = std::make_shared<SigningHandler>(logger);
  database->initJsonTree("test_vss_rel_2.0.json");

   //we can not mock for testing authentication
   auto accesshandler_real = std::make_shared<AccessChecker>(authhandler);
   auto subhandler_auth = std::make_shared<SubscriptionHandler>(logger, httpServer, mqttClient, authhandler, accesshandler_real);
   auto database_auth = std::make_shared<VssDatabase>(logger, subhandler_auth, accesshandler_real);
   database_auth->initJsonTree("test_vss_rel_2.0.json");

   commandProc_auth = std::make_shared<VssCommandProcessor>(logger, database_auth, authhandler , subhandler_auth);


}

kuksavalunittest::~kuksavalunittest() {
}

list<string> kuksavalunittest::test_wrap_getPathForGet(string path , bool &isBranch) {
    return database->getPathForGet(path , isBranch);
}

json kuksavalunittest::test_wrap_getPathForSet(string path,  json value) {
    return database->getPathForSet(path , value);
}

//--------Do not change the order of the tests in this file, because some results are dependent on the previous tests and data in the db-------
//----------------------------------------------------VssDatabase Tests ------------------------------------------------------------------------

// Get method tests
BOOST_AUTO_TEST_CASE(path_for_get_without_wildcard_simple)
{
    string test_path = "Vehicle.OBD.EngineSpeed"; // Pass a path to signal without wildcard.
    bool isBranch = false;

    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);
    string result = paths.back();

    BOOST_TEST(paths.size() ==  1u);
    BOOST_TEST(result == "$['Vehicle']['children']['OBD']['children']['EngineSpeed']");
    BOOST_TEST(isBranch == false);

}

BOOST_AUTO_TEST_CASE(path_for_get_with_wildcard_simple)
{

    string test_path = "Vehicle.*.EngineSpeed"; // Pass a path to signal with wildcard.
    bool isBranch = false;

    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);
    string result = paths.back();

    BOOST_TEST(paths.size() == 1u);
    BOOST_TEST(result == "$['Vehicle']['children']['OBD']['children']['EngineSpeed']");
    BOOST_TEST(isBranch == false);

}

BOOST_AUTO_TEST_CASE(path_for_get_with_wildcard_multiple)
{

    string test_path = "Vehicle.Chassis.Axle.*.*.Left.*.Pressure"; // Pass a path to signal with wildcard.
    bool isBranch = false;

    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);
    BOOST_TEST(paths.size() == 2u);
    string result2 = paths.back();
    paths.pop_back();
    string result1 = paths.back();

    BOOST_TEST(result1 == "$['Vehicle']['children']['Chassis']['children']['Axle']['children']['Row1']['children']['Wheel']['children']['Left']['children']['Tire']['children']['Pressure']");
    BOOST_TEST(result2 == "$['Vehicle']['children']['Chassis']['children']['Axle']['children']['Row2']['children']['Wheel']['children']['Left']['children']['Tire']['children']['Pressure']");
    BOOST_TEST(isBranch == false);

}

BOOST_AUTO_TEST_CASE(test_get_branch)
{
    string test_path = "Vehicle.Acceleration"; // Pass a path to branch
    bool isBranch = false;

    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);
    string result = paths.back();

    BOOST_TEST(paths.size() == 3u);
    BOOST_TEST(isBranch == true);

}

BOOST_AUTO_TEST_CASE(test_get_branch_with_wildcard)
{
    string test_path = "Vehicle.*.Transmission"; // Pass a path to branch with wildcard
    bool isBranch = false;

    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);
    string result = paths.back();

    BOOST_TEST(paths.size() == 10u);
    BOOST_TEST(isBranch == true);

}


BOOST_AUTO_TEST_CASE(test_get_invalidpath)
{
    string test_path = "scdKC"; // pass an invalid path with or without wildcard.
    bool isBranch = false;

    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);

    BOOST_TEST(paths.size() == 0u);
    BOOST_TEST(isBranch == false);

}

// set method tests

BOOST_AUTO_TEST_CASE(path_for_set_without_wildcard_simple)
{
    string test_path = "Vehicle.OBD.EngineSpeed"; // pass a valid path without wildcard.
    json test_value;
    test_value["value"] = 123;

    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value);

    BOOST_TEST(paths.size() == 1u);

    BOOST_TEST(paths[0]["path"].as<std::string>() == "$['Vehicle']['children']['OBD']['children']['EngineSpeed']");


}

BOOST_AUTO_TEST_CASE(path_for_set_with_wildcard_simple)
{
    string test_path = "Vehicle.*"; // pass a valid path with wildcard.

    json test_value_array = json::make_array(2);
    json test_value1, test_value2;
    test_value1["OBD.EngineSpeed"] = 123;
    test_value2["OBD.Speed"] = 234;

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;

    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value_array);

    BOOST_TEST(paths.size() == 2u);

    BOOST_TEST(paths[0]["path"].as_string() == "$['Vehicle']['children']['OBD']['children']['EngineSpeed']");
    BOOST_TEST(paths[1]["path"].as_string() == "$['Vehicle']['children']['OBD']['children']['Speed']");

}


BOOST_AUTO_TEST_CASE(path_for_set_with_wildcard_with_invalid_values)
{
    string test_path = "Vehicle.*"; // pass a valid path with wildcard.

    json test_value_array = json::make_array(2);
    json test_value1, test_value2;
    test_value1["OBD1.EngineSpeed"] = 123;   // pass invalid sub-path
    test_value2["OBD.Speed1"] = 234;  // pass invalid sub-path

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;

    BOOST_CHECK_THROW( {unittestObj.test_wrap_getPathForSet(test_path , test_value_array);}, noPathFoundonTree );
    
}

BOOST_AUTO_TEST_CASE(path_for_set_with_wildcard_with_one_valid_value)
{
    string test_path = "Vehicle.*"; // pass a valid path with wildcard.

    json test_value_array = json::make_array(2);
    json test_value1, test_value2;
    test_value1["OBD.EngineSpeed"] = 123;   // pass valid sub-path
    test_value2["OBD.Speed1"] = 234;  // pass invalid sub-path

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;

    
    BOOST_CHECK_THROW( {unittestObj.test_wrap_getPathForSet(test_path , test_value_array);}, noPathFoundonTree );

}

BOOST_AUTO_TEST_CASE(path_for_set_with_wildcard_with_invalid_path_valid_values)
{
    string test_path = "Signal.Vehicle.*"; // pass an invalid path.

    json test_value_array = json::make_array(2);
    json test_value1, test_value2;
    test_value1["OBD.EngineSpeed"] = 123;   // pass valid sub-path
    test_value2["OBD.Speed"] = 234;  // pass valid sub-path

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;


    BOOST_CHECK_THROW( {unittestObj.test_wrap_getPathForSet(test_path , test_value_array);}, noPathFoundonTree );

}

BOOST_AUTO_TEST_CASE(test_set_value_on_branch)
{
    string test_path = "Vehicle.OBD"; // pass a valid branch path without wildcard.
    json test_value;
    bool isException = false;
    test_value["value"] = 123;
    try {
       json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value);
    }catch (exception &e) {
       isException = true;
    }

    // An exception is thrown if a set value is attempted on a branch.
    BOOST_TEST(isException ==  true);
}

BOOST_AUTO_TEST_CASE(test_set_value_on_branch_with_valid_values)
{
    string test_path = "Vehicle.Cabin.HVAC.Row2"; // pass a valid path with wildcard.

    json test_value_array = json::make_array(2);
    json test_value1, test_value2;
    test_value1["Left.Temperature"] = 24;
    test_value2["Right.Temperature"] = 25;

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;

    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value_array);

    BOOST_TEST(paths.size() == 2u);

    BOOST_TEST(paths[0]["path"].as_string() == "$['Vehicle']['children']['Cabin']['children']['HVAC']['children']['Row2']['children']['Left']['children']['Temperature']");
    BOOST_TEST(paths[1]["path"].as_string() == "$['Vehicle']['children']['Cabin']['children']['HVAC']['children']['Row2']['children']['Right']['children']['Temperature']");

}

BOOST_AUTO_TEST_CASE(test_set_value_on_branch_with_invalid_values)
{
    string test_path = "Vehicle.Cabin.HVAC.Row2"; // pass a valid branch path with wildcard.

    json test_value_array = json::make_array(2);
    json test_value1, test_value2;
    test_value1["Left.NotTemperature"] = 24;  //Invalid leaf
    test_value2["Right.NotTemperature"] = 25; //Invalid leaf

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;

    BOOST_CHECK_THROW( {unittestObj.test_wrap_getPathForSet(test_path , test_value_array);}, noPathFoundonTree );
}

BOOST_AUTO_TEST_CASE(test_set_value_on_branch_with_one_invalid_value)
{
    string test_path = "Vehicle.Cabin.HVAC.Row2"; // pass a valid branch path with wildcard.

    json test_value_array = json::make_array(2);
    json test_value1, test_value2;
    test_value1["Left.Temperature"] = true;
    test_value2["Right.NotTemperature"] = false; //invalid leaf. Leads to fail

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;
    MOCK_EXPECT(accesshandler->checkReadDeprecated).returns(true);

    
    BOOST_CHECK_THROW( {unittestObj.test_wrap_getPathForSet(test_path , test_value_array);}, noPathFoundonTree );

    //Old conditon before it was atomic    
    //BOOST_TEST(paths[0]["path"].as_string() == "$['Vehicle']['children']['Cabin']['children']['HVAC']['children']['Row2']['children']['Left']['children']['Temperature']");  

}

BOOST_AUTO_TEST_CASE(set_get_test_all_datatypes_boundary_conditions)
{
    string test_path_Uint8 = "Vehicle.OBD.AcceleratorPositionE";
    string test_path_Uint16 = "Vehicle.OBD.WarmupsSinceDTCClear";
    string test_path_Uint32 = "Vehicle.OBD.TimeSinceDTCCleared";
    string test_path_int8 = "Vehicle.OBD.ShortTermFuelTrim2";
    string test_path_int16 = "Vehicle.OBD.FuelInjectionTiming";
    string test_path_int32 = "Vehicle.Drivetrain.Transmission.Speed";
    string test_path_boolean = "Vehicle.OBD.Status.MIL";
    string test_path_Float = "Vehicle.OBD.FuelRate";
    string test_path_Double =  "Vehicle.Cabin.Infotainment.Navigation.DestinationSet.Latitude";
    string test_path_string = "Vehicle.Cabin.Infotainment.Media.Played.URI";

    MOCK_EXPECT(accesshandler->checkWriteAccess).returns(true);
    MOCK_EXPECT(accesshandler->checkReadNew).returns(true);
    MOCK_EXPECT(mqttClient->sendPathValue).returns(true);
    json result;
    WsChannel channel;
    channel.setConnID(1234);
    string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
    json authReqJson = json::parse(authReq);
    authReqJson["tokens"] = AUTH_JWT;
    commandProc->processQuery(authReqJson.as_string(),channel);

    string get_request(R"({
		"action": "get",
		"path": "Vehicle.OBD.Speed",
		"requestId": "8756"
	})");
    string response = commandProc->processQuery(get_request,channel);

//---------------------  Uint8 SET/GET TEST ------------------------------------
    json test_value_Uint8_boundary_low;
    test_value_Uint8_boundary_low = numeric_limits<uint8_t>::min();

    database->setSignal(channel,test_path_Uint8, test_value_Uint8_boundary_low);
    result = database->getSignal(channel, test_path_Uint8);

    BOOST_TEST(result["value"].as<uint8_t>() == numeric_limits<uint8_t>::min());

    json test_value_Uint8_boundary_high;
    test_value_Uint8_boundary_high = numeric_limits<uint8_t>::max();

    database->setSignal(channel,test_path_Uint8, test_value_Uint8_boundary_high);
    result = database->getSignal(channel, test_path_Uint8);

   BOOST_TEST(result["value"].as<uint8_t>() == numeric_limits<uint8_t>::max());

    json test_value_Uint8_boundary_middle;
    test_value_Uint8_boundary_middle = numeric_limits<uint8_t>::max() / 2;

    database->setSignal(channel,test_path_Uint8, test_value_Uint8_boundary_middle);
    result = database->getSignal(channel, test_path_Uint8);

    BOOST_TEST(result["value"].as<uint8_t>() == numeric_limits<uint8_t>::max() / 2);

    // Test out of bound
    bool isExceptionThrown = false;
    json test_value_Uint8_boundary_low_outbound;
    test_value_Uint8_boundary_low_outbound = -1;
    try {
       database->setSignal(channel,test_path_Uint8, test_value_Uint8_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;

    json test_value_Uint8_boundary_high_outbound;
    test_value_Uint8_boundary_high_outbound = numeric_limits<uint8_t>::max() + 1;
    try {
       database->setSignal(channel,test_path_Uint8, test_value_Uint8_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);


//---------------------  Uint16 SET/GET TEST ------------------------------------

    json test_value_Uint16_boundary_low;
    test_value_Uint16_boundary_low = numeric_limits<uint16_t>::min();

    database->setSignal(channel,test_path_Uint16, test_value_Uint16_boundary_low);
    result = database->getSignal(channel, test_path_Uint16);

    BOOST_TEST(result["value"].as<uint16_t>() == numeric_limits<uint16_t>::min());

    json test_value_Uint16_boundary_high;
    test_value_Uint16_boundary_high = numeric_limits<uint16_t>::max();

    database->setSignal(channel,test_path_Uint16, test_value_Uint16_boundary_high);
    result = database->getSignal(channel, test_path_Uint16);

    BOOST_TEST(result["value"].as<uint16_t>() == numeric_limits<uint16_t>::max());

    json test_value_Uint16_boundary_middle;
    test_value_Uint16_boundary_middle = numeric_limits<uint16_t>::max() / 2;

    database->setSignal(channel,test_path_Uint16, test_value_Uint16_boundary_middle);
    result = database->getSignal(channel, test_path_Uint16);

    BOOST_TEST(result["value"].as<uint16_t>() == numeric_limits<uint16_t>::max() / 2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_Uint16_boundary_low_outbound;
    test_value_Uint16_boundary_low_outbound = -1;
    try {
       database->setSignal(channel,test_path_Uint16, test_value_Uint16_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;

    json test_value_Uint16_boundary_high_outbound;
    test_value_Uint16_boundary_high_outbound = numeric_limits<uint16_t>::max() + 1;
    try {
       database->setSignal(channel,test_path_Uint16, test_value_Uint16_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);


//---------------------  Uint32 SET/GET TEST ------------------------------------

    json test_value_Uint32_boundary_low;
    test_value_Uint32_boundary_low = numeric_limits<uint32_t>::min();

    database->setSignal(channel,test_path_Uint32, test_value_Uint32_boundary_low);
    result = database->getSignal(channel, test_path_Uint32);

    BOOST_TEST(result["value"].as<uint32_t>() == numeric_limits<uint32_t>::min());

    json test_value_Uint32_boundary_high;
    test_value_Uint32_boundary_high = numeric_limits<uint32_t>::max();

    database->setSignal(channel,test_path_Uint32, test_value_Uint32_boundary_high);
    result = database->getSignal(channel, test_path_Uint32);

    BOOST_TEST(result["value"].as<uint32_t>() == numeric_limits<uint32_t>::max());

    json test_value_Uint32_boundary_middle;
    test_value_Uint32_boundary_middle = numeric_limits<uint32_t>::max() / 2;

    database->setSignal(channel,test_path_Uint32, test_value_Uint32_boundary_middle);
    result = database->getSignal(channel, test_path_Uint32);

    BOOST_TEST(result["value"].as<uint32_t>() == numeric_limits<uint32_t>::max() / 2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_Uint32_boundary_low_outbound;
    test_value_Uint32_boundary_low_outbound = -1;
    try {
       database->setSignal(channel,test_path_Uint32, test_value_Uint32_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;

    json test_value_Uint32_boundary_high_outbound;
    uint64_t maxU32_value = numeric_limits<uint32_t>::max();
    test_value_Uint32_boundary_high_outbound = maxU32_value + 1;
    try {
       database->setSignal(channel,test_path_Uint32, test_value_Uint32_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);

//---------------------  int8 SET/GET TEST ------------------------------------

    json test_value_int8_boundary_low;
    test_value_int8_boundary_low = numeric_limits<int8_t>::min();

    database->setSignal(channel,test_path_int8, test_value_int8_boundary_low);
    result = database->getSignal(channel, test_path_int8);

    BOOST_TEST(result["value"].as<int8_t>() == numeric_limits<int8_t>::min());

    json test_value_int8_boundary_high;
    test_value_int8_boundary_high = numeric_limits<int8_t>::max();

    database->setSignal(channel,test_path_int8, test_value_int8_boundary_high);
    result = database->getSignal(channel, test_path_int8);

    BOOST_TEST(result["value"].as<int8_t>() == numeric_limits<int8_t>::max());

    json test_value_int8_boundary_middle;
    test_value_int8_boundary_middle = numeric_limits<int8_t>::max() / 2;

    database->setSignal(channel,test_path_int8, test_value_int8_boundary_middle);
    result = database->getSignal(channel, test_path_int8);

    BOOST_TEST(result["value"].as<int8_t>() == numeric_limits<int8_t>::max() / 2);

    json test_value_int8_boundary_middle_neg;
    test_value_int8_boundary_middle_neg = numeric_limits<int8_t>::min() / 2;

    database->setSignal(channel,test_path_int8, test_value_int8_boundary_middle_neg);
    result = database->getSignal(channel, test_path_int8);

    BOOST_TEST(result["value"].as<int8_t>() == numeric_limits<int8_t>::min() / 2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_int8_boundary_low_outbound;
    test_value_int8_boundary_low_outbound = numeric_limits<int8_t>::min() - 1;
    try {
       database->setSignal(channel,test_path_int8, test_value_int8_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;

    json test_value_int8_boundary_high_outbound;
    test_value_int8_boundary_high_outbound = numeric_limits<int8_t>::max() + 1;
    try {
       database->setSignal(channel,test_path_int8, test_value_int8_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);

//---------------------  int16 SET/GET TEST ------------------------------------

    json test_value_int16_boundary_low;
    test_value_int16_boundary_low = numeric_limits<int16_t>::min();

    database->setSignal(channel,test_path_int16, test_value_int16_boundary_low);
    result = database->getSignal(channel, test_path_int16);

    BOOST_TEST(result["value"].as<int16_t>() == numeric_limits<int16_t>::min());

    json test_value_int16_boundary_high;
    test_value_int16_boundary_high = numeric_limits<int16_t>::max();

    database->setSignal(channel,test_path_int16, test_value_int16_boundary_high);
    result = database->getSignal(channel, test_path_int16);

    BOOST_TEST(result["value"].as<int16_t>() == numeric_limits<int16_t>::max());

    json test_value_int16_boundary_middle;
    test_value_int16_boundary_middle = numeric_limits<int16_t>::max()/2;

    database->setSignal(channel,test_path_int16, test_value_int16_boundary_middle);
    result = database->getSignal(channel, test_path_int16);

    BOOST_TEST(result["value"].as<int16_t>() == numeric_limits<int16_t>::max()/2);

    json test_value_int16_boundary_middle_neg;
    test_value_int16_boundary_middle_neg = numeric_limits<int16_t>::min()/2;

    database->setSignal(channel,test_path_int16, test_value_int16_boundary_middle_neg);
    result = database->getSignal(channel, test_path_int16);

    BOOST_TEST(result["value"].as<int16_t>() == numeric_limits<int16_t>::min()/2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_int16_boundary_low_outbound;
    test_value_int16_boundary_low_outbound = numeric_limits<int16_t>::min() - 1;
    try {
       database->setSignal(channel,test_path_int16, test_value_int16_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;

    json test_value_int16_boundary_high_outbound;
    test_value_int16_boundary_high_outbound = numeric_limits<int16_t>::max() + 1;
    try {
       database->setSignal(channel,test_path_int16, test_value_int16_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);


//---------------------  int32 SET/GET TEST ------------------------------------

    json test_value_int32_boundary_low;
    test_value_int32_boundary_low = numeric_limits<int32_t>::min();

    database->setSignal(channel,test_path_int32, test_value_int32_boundary_low);
    result = database->getSignal(channel, test_path_int32);

    BOOST_TEST(result["value"].as<int32_t>() == numeric_limits<int32_t>::min());

    json test_value_int32_boundary_high;
    test_value_int32_boundary_high = numeric_limits<int32_t>::max() ;

    database->setSignal(channel,test_path_int32, test_value_int32_boundary_high);
    result = database->getSignal(channel, test_path_int32);

    BOOST_TEST(result["value"].as<int32_t>() == numeric_limits<int32_t>::max());

    json test_value_int32_boundary_middle;
    test_value_int32_boundary_middle = numeric_limits<int32_t>::max() / 2;

    database->setSignal(channel,test_path_int32, test_value_int32_boundary_middle);
    result = database->getSignal(channel, test_path_int32);

    BOOST_TEST(result["value"].as<int32_t>() == numeric_limits<int32_t>::max() / 2);

    json test_value_int32_boundary_middle_neg;
    test_value_int32_boundary_middle_neg = numeric_limits<int32_t>::min() / 2;

    database->setSignal(channel,test_path_int32, test_value_int32_boundary_middle_neg);
    result = database->getSignal(channel, test_path_int32);

    BOOST_TEST(result["value"].as<int32_t>() == numeric_limits<int32_t>::min() / 2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_int32_boundary_low_outbound;
    int64_t minInt32_value = numeric_limits<int32_t>::min();
    test_value_int32_boundary_low_outbound = minInt32_value - 1;
    try {
       database->setSignal(channel,test_path_int32, test_value_int32_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;

    json test_value_int32_boundary_high_outbound;
    int64_t maxInt32_value = numeric_limits<int32_t>::max();
    test_value_int32_boundary_high_outbound = maxInt32_value + 1;
    try {
       database->setSignal(channel,test_path_int32, test_value_int32_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);

//---------------------  Float SET/GET TEST ------------------------------------
    json test_value_Float_boundary_low;
    test_value_Float_boundary_low = std::numeric_limits<float>::min();

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_low);
    result = database->getSignal(channel, test_path_Float);


    BOOST_TEST(result["value"].as<float>() == std::numeric_limits<float>::min());

    json test_value_Float_boundary_high;
    test_value_Float_boundary_high = std::numeric_limits<float>::max();

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_high);
    result = database->getSignal(channel, test_path_Float);

    BOOST_TEST(result["value"].as<float>() == std::numeric_limits<float>::max());


    json test_value_Float_boundary_low_neg;
    test_value_Float_boundary_low_neg = std::numeric_limits<float>::min() * -1;

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_low_neg);
    result = database->getSignal(channel, test_path_Float);


    BOOST_TEST(result["value"].as<float>() == (std::numeric_limits<float>::min() * -1));

    json test_value_Float_boundary_high_neg;
    test_value_Float_boundary_high_neg = std::numeric_limits<float>::max() * -1;

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_high_neg);
    result = database->getSignal(channel, test_path_Float);

    BOOST_TEST(result["value"].as<float>() == (std::numeric_limits<float>::max() * -1));


    json test_value_Float_boundary_middle;
    test_value_Float_boundary_middle = std::numeric_limits<float>::max() / 2;

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_middle);
    result = database->getSignal(channel, test_path_Float);


    BOOST_TEST(result["value"].as<float>() == std::numeric_limits<float>::max() / 2);

    json test_value_Float_boundary_middle_neg;
    test_value_Float_boundary_middle_neg = std::numeric_limits<float>::min() * 2;

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_middle_neg);
    result = database->getSignal(channel, test_path_Float);

    BOOST_TEST(result["value"].as<float>() == std::numeric_limits<float>::min() * 2);

    
    
//---------------------  Double SET/GET TEST ------------------------------------

    json test_value_Double_boundary_low;
    test_value_Double_boundary_low = std::numeric_limits<double>::min();

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_low);
    result = database->getSignal(channel, test_path_Double);

    BOOST_TEST(result["value"].as<double>() == std::numeric_limits<double>::min());

    json test_value_Double_boundary_high;
    test_value_Double_boundary_high = std::numeric_limits<double>::max();

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_high);
    result = database->getSignal(channel, test_path_Double);

    BOOST_TEST(result["value"].as<double>() == std::numeric_limits<double>::max());


    json test_value_Double_boundary_low_neg;
    test_value_Double_boundary_low_neg = std::numeric_limits<double>::min() * -1;

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_low_neg);
    result = database->getSignal(channel, test_path_Double);

    BOOST_TEST(result["value"].as<double>() == (std::numeric_limits<double>::min() * -1));

    json test_value_Double_boundary_high_neg;
    test_value_Double_boundary_high_neg = std::numeric_limits<double>::max() * -1;

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_high_neg);
    result = database->getSignal(channel ,test_path_Double);

    BOOST_TEST(result["value"].as<double>() == (std::numeric_limits<double>::max() * -1));



    json test_value_Double_boundary_middle;
    test_value_Double_boundary_middle = std::numeric_limits<double>::max() / 2;

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_middle);
    result = database->getSignal(channel ,test_path_Double);

    BOOST_TEST(result["value"].as<double>() == std::numeric_limits<double>::max() / 2);

    json test_value_Double_boundary_middle_neg;
    test_value_Double_boundary_middle_neg = std::numeric_limits<double>::min() * 2;

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_middle_neg);
    result = database->getSignal(channel, test_path_Double);

    BOOST_TEST(result["value"].as<double>() == std::numeric_limits<double>::min() * 2);

    


//---------------------  String SET/GET TEST ------------------------------------

    json test_value_String_empty;
    test_value_String_empty = "";

    database->setSignal(channel,test_path_string, test_value_String_empty);
    result = database->getSignal(channel,test_path_string);

    BOOST_TEST(result["value"].as<std::string>() == "");

    json test_value_String_null;
    test_value_String_null = "\0";

    database->setSignal(channel,test_path_string, test_value_String_null);
    result = database->getSignal(channel,test_path_string);

    BOOST_TEST(result["value"].as<std::string>() == "\0");

    json test_value_String_long;
    test_value_String_long = "hello to w3c vis server unit test with boost libraries! This is a test string to test string data type without special characters, but this string is pretty long";

    database->setSignal(channel,test_path_string, test_value_String_long);
    result = database->getSignal(channel,test_path_string);

    BOOST_TEST(result["value"].as<std::string>() == test_value_String_long);

    json test_value_String_long_with_special_chars;
    test_value_String_long_with_special_chars = "hello to w3c vis server unit test with boost libraries! This is a test string conatains special chars like üö Ä? $ % #";

    database->setSignal(channel,test_path_string, test_value_String_long_with_special_chars);
    result = database->getSignal(channel,test_path_string);

    BOOST_TEST(result["value"].as<std::string>() == test_value_String_long_with_special_chars);

//---------------------  Boolean SET/GET TEST ------------------------------------
    json test_value_bool_false;
    test_value_bool_false = false;

    database->setSignal(channel,test_path_boolean, test_value_bool_false);
    result = database->getSignal(channel,test_path_boolean);

    BOOST_TEST(result["value"].as<bool>() == test_value_bool_false);

    json test_value_bool_true;
    test_value_bool_true = true;

    database->setSignal(channel,test_path_boolean, test_value_bool_true);
    result = database->getSignal(channel,test_path_boolean);

    BOOST_TEST(result["value"].as<bool>() == test_value_bool_true);
}


BOOST_AUTO_TEST_CASE(test_set_invalid_path)
{
    string test_path = "Signal.lksdcl"; // pass an invalid path without wildcard.
    json test_value;
    test_value["value"] = 123;

    BOOST_CHECK_THROW( {unittestObj.test_wrap_getPathForSet(test_path , test_value);}, noPathFoundonTree );
}

// -------------------------------- Metadata test ----------------------------------

BOOST_AUTO_TEST_CASE(test_metadata_simple)
{
    string test_path = "Vehicle.OBD.EngineSpeed"; // pass a valid path without wildcard.

    json result = database->getMetaData(test_path);

    json expected = json::parse(R"({
    "Vehicle": {
        "children": {
            "OBD": {
                "children": {
                    "EngineSpeed": {
                        "description": "PID 0C - Engine speed measured as rotations per minute",
                        "type": "sensor",
                        "datatype": "float",
                        "uuid": "45b85b6ba8555ccb8ca5c9b96ab5f94e",
                        "unit": "rpm"
                    }
                },
                "description": "OBD data.",
                "uuid": "423b844a2f3b51a688b0ab74ecb6eae4",
                "type": "branch"
            }
        },
        "description": "High-level vehicle data.",
        "uuid": "1c72453e738511e9b29ad46a6a4b77e9",
        "type": "branch"
    }
    })");

    BOOST_TEST(result ==  expected);
}

BOOST_AUTO_TEST_CASE(test_metadata_with_wildcard)
{
    string test_path = "Vehicle.*.EngineSpeed"; // pass a valid path with wildcard.

    json result = database->getMetaData(test_path);

    json expected = json::parse(R"({
    "Vehicle": {
        "children": {
            "OBD": {
                "children": {
                    "EngineSpeed": {
                        "description": "PID 0C - Engine speed measured as rotations per minute",
                        "uuid": "45b85b6ba8555ccb8ca5c9b96ab5f94e",
                        "datatype": "float",
                        "type": "sensor",
                        "unit": "rpm"
                    }
                },
                "description": "OBD data.",
                "uuid": "423b844a2f3b51a688b0ab74ecb6eae4",
                "type": "branch"
            }
        },
        "description": "High-level vehicle data.",
        "uuid": "1c72453e738511e9b29ad46a6a4b77e9",
        "type": "branch"
    }
    })");

    BOOST_TEST(result ==  expected);
}

BOOST_AUTO_TEST_CASE(test_metadata_branch)
{
    string test_path = "Vehicle.Chassis.SteeringWheel"; // pass a valid branch path without wildcard.

    json result = database->getMetaData(test_path);

    json expected = json::parse(R"({
    "Vehicle": {
        "children": {
            "Chassis": {
                "children": {
                    "SteeringWheel": {
                        "children": {
                            "Angle": {
                                "description": "Steering wheel angle. Positive = degrees to the left. Negative = degrees to the right.",
                                "type": "sensor",
                                "datatype": "int16",
                                "unit": "degrees",
                                "uuid": "b4d1437aad9d5be38fe325954deb131a"
                            },
                            "Extension": {
                                "description": "Steering wheel column extension from dashboard. 0 = Closest to dashboard. 100 = Furthest from dashboard.",
                                "type": "actuator",
                                "datatype": "uint8",
                                "max": 100,
                                "min": 0,
                                "unit": "percent",
                                "uuid": "2704da815c8a5ce798d1db0042a040e7"
                            },
                            "Tilt": {
                                "description": "Steering wheel column tilt. 0 = Lowest position. 100 = Highest position.",
                                "type": "actuator",
                                "datatype": "uint8",
                                "max": 100,
                                "min": 0,
                                "unit": "percent",
                                "uuid": "927d3bca395a568ab282486579351704"
                            }
                        },
                        "description": "Steering wheel signals",
                        "type": "branch",
                        "uuid": "c7c33837f61559378630a6a021591c45"
                    }
                },
                "description": "All data concerning steering, suspension, wheels, and brakes.",
                "type": "branch",
                "uuid": "b7b7566dd46951eb8a321babc6e236ec"
            }
        },
        "description": "High-level vehicle data.",
        "type": "branch",
        "uuid": "1c72453e738511e9b29ad46a6a4b77e9"
    }
  }
)");

    BOOST_TEST(result ==  expected);
}

BOOST_AUTO_TEST_CASE(test_metadata_branch_with_wildcard)
{
    string test_path = "Vehicle.*.SteeringWheel"; // pass a valid branch path with wildcard.

    json result = database->getMetaData(test_path);

    json expected = json::parse(R"({
  "Vehicle": {
    "children": {
      "Cabin": {
        "children": {
          "SteeringWheel": {
            "children": {
              "Position": {
                "datatype": "string",
                "description": "Position of the steering wheel inside the cabin",
                "enum": [
                  "front_left",
                  "front_right"
                ],
                "type": "attribute",
                "uuid": "e3e2fad9ee16547c89881fce84ba381f",
                "value": "front_left"
              }
            },
            "description": "Steering wheel configuration attributes",
            "type": "branch",
            "uuid": "b6e4c824083a58a794386fd41dc854b5"
          }
        },
        "description": "All in-cabin components, including doors.",
        "type": "branch",
        "uuid": "2a9e2e147fd7517baee92a29c5462ddd"
      }
    },
    "description": "High-level vehicle data.",
    "type": "branch",
    "uuid": "1c72453e738511e9b29ad46a6a4b77e9"
  }
}
)");

    BOOST_TEST(result ==  expected);
}


//----------------------------------------------------VssCommandProcessor Tests ------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(process_query_set_get_simple)
{
   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as_string(),channel);

   string set_request(R"({
		"action": "set",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8750",
                "value" : 2345.0
	})");

   string set_response = commandProc->processQuery(set_request,channel);
   json set_response_json = json::parse(set_response);

   json set_response_expected = json::parse(R"({
    "action": "set",
    "requestId": "8750"
     }
     )");


   BOOST_TEST(set_response_json.contains("timestamp") == true);
   // remove timestamp to match
   set_response_json.erase("timestamp");

   BOOST_TEST(set_response_json == set_response_expected);


   string get_request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(get_request,channel);
#ifdef JSON_SIGNING_ON
   response = json_signer->decode(response);
#endif

   json expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": 2345.0
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}


BOOST_AUTO_TEST_CASE(process_query_get_withwildcard)
{
   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.*.EngineSpeed",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": 2345
    })");

#ifdef JSON_SIGNING_ON
   response = json_signer->decode(response);
#endif

   json response_json = json::parse(response);




   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(process_query_set_get_withwildcard)
{
   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as_string(),channel);

   string set_request(R"({
		"action": "set",
		"path": "Vehicle.Chassis.Axle.*.Wheel.*.Tire.Temperature",
		"requestId": "8750",
                "value" : [{"Row1.Wheel.Right.Tire.Temperature":35}, {"Row1.Wheel.Left.Tire.Temperature":50}, {"Row2.Wheel.Right.Tire.Temperature":65}, {"Row2.Wheel.Left.Tire.Temperature":80}]
	})");

   string set_response = commandProc->processQuery(set_request,channel);
   json set_response_json = json::parse(set_response);

   json set_response_expected = json::parse(R"({
    "action": "set",
    "requestId": "8750"
     }
     )");


   BOOST_TEST(set_response_json.contains("timestamp") == true);
   // remove timestamp to match
   set_response_json.erase("timestamp");

   BOOST_TEST(set_response_json == set_response_expected);


   string get_request(R"({
		"action": "get",
		"path": "Vehicle.Chassis.Axle.*.Wheel.*.Tire.Temperature",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(get_request,channel);



   json expected = json::parse(R"({
    "action": "get",
    "requestId": "8756",
    "value": [
        {
            "Vehicle.Chassis.Axle.Row2.Wheel.Right.Tire.Temperature": 65
        },
        {
            "Vehicle.Chassis.Axle.Row2.Wheel.Left.Tire.Temperature": 80
        },
        {
            "Vehicle.Chassis.Axle.Row1.Wheel.Right.Tire.Temperature": 35
        },
        {
            "Vehicle.Chassis.Axle.Row1.Wheel.Left.Tire.Temperature": 50
        }
      ]
    }
    )");

#ifdef JSON_SIGNING_ON
   response = json_signer->decode(response);
#endif

   json response_json = json::parse(response);

   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(process_query_get_withwildcard_invalid)
{
   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.*.RPM1",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);
#ifdef JSON_SIGNING_ON
   response = json_signer->decode(response);
#endif

   json expected = json::parse(R"({
                         "action":"get",
                         "error":{"message":"I can not find Signal/*/RPM1 in my db","number":404,"reason":"Path not found"},
                         "requestId":"8756"
                         })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(process_query_set_withwildcard_invalid)
{
   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "set",
		"path": "Signal.OBD.*",
                "value" : [{"RPM1" : 345} , {"Speed1" : 546}],
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
                              "action":"set",
                              "error":{"message":"I can not find Signal.OBD.* in my db","number":404,"reason":"Path not found"},
                              "requestId":"8756"
                              })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

///HERE
BOOST_AUTO_TEST_CASE(process_query_set_invalid_value)
{
   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "set",
		"path": "Vehicle.OBD.*",
                "value" : [{"ThrottlePosition" : 34563843034034845945054}],
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
                               "action":"set",
                               "error":{"message":"The type uint8 with value 3.45638e+22 is out of bound","number":400,"reason":"Value passed is out of bounds"},
                               "requestId":"8756"
                               })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(process_query_set_one_valid_one_invalid_value)
{
   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as_string(),channel);

   string request(R"({
		"action": "set",
		"path": "Vehicle.OBD.*",
                "value" : [{"ThrottlePosition" : 34563843034034845945054},{"EngineSpeed" : 1000}],
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
                               "action":"set",
                               "error":{"message":"The type uint8 with value 3.45638e+22 is out of bound","number":400,"reason":"Value passed is out of bounds"},
                               "requestId":"8756"
                               })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);


   string request_getvalid(R"({
		"action": "get",
		"path": "Vehicle.*.EngineSpeed",
		"requestId": "8756"
	})");

   string response_getvalid = commandProc->processQuery(request_getvalid,channel);

   // This shows that the value from the previous test is not set because of out of bound exception for the ThrottlePosition value, Therefore the RPM was not set despite being   valid. if you swap the order in which the signals are set in above test case then the value here would be 1000.
   json expected_getvalid = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": 2345
    })");

#ifdef JSON_SIGNING_ON
   response_getvalid = json_signer->decode(response_getvalid);
#endif

   json response_response_getvalid_json = json::parse(response_getvalid);


   BOOST_TEST(response_response_getvalid_json.contains("timestamp") == true);

   // remove timestamp to match
   response_response_getvalid_json.erase("timestamp");
   BOOST_TEST(response_response_getvalid_json == expected_getvalid);
}

//----------------------------------------------------json SigningHandler Tests ------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(json_SigningHandler)
{

   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as_string(),channel);

   string get_request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(get_request,channel);
#ifdef JSON_SIGNING_ON
   response = json_signer->decode(response);
#endif
   json response_json = json::parse(response);

   BOOST_TEST(response_json.contains("timestamp") == true);
   // remove timestamp to match
   response_json.erase("timestamp");

   json expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": 2345
    })");

   // Pre-check
   BOOST_TEST(response_json == expected);

#ifdef JSON_SIGNING_ON
   // sign the response json
   string signedData = json_signer->sign(response_json);


   // now decode the signed json
   string decoded_json_as_string = json_signer->decode(signedData);
   json decoded_json = json::parse(decoded_json_as_string);


   // Assert decodes and the expected json
   BOOST_TEST(decoded_json == expected);
#endif
}

//----------------------------------------------------Token permissions Tests ------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(permission_basic_read)
{
/*
    Token looks like this.

  {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "r"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQuRW5naW5lU3BlZWQiOiJyIn19.jbhdMq5hEWXXNfZn9xE4rECIWEVsw-q3g-jxp5lLS0VAZ2WYOGoSd5JX2P9YG0Lals7Ue0wdXtgLSFtvIGU4Ol2MuPaF-Rbb-Q5O4njxg60AZ00kr6RywpyGZHK0eT9MuFCnVMN8Krf3lo2pPB1ms-WAHX6rfArbXxx0FsMau9Ewn_3m3Sc-6sz5alQw1Y7Rk0GD9Y7WP_mbICU__gd40Ypu_ki1i59M8ba5GNfd8RytEIJXAg7RTcKREWDZfMdFH5X7F6gAPA7h_tVH3-bsbT-nOsKCbM-3PM0EKAOl104SwmKcqoWnfXbUow5zt25O8LYwmrukuRBtWiLI5FxeW6ovmS-1acvS3w1LXlQZVGWtM_ZC7shtHh-iz7nyL1WCTpZswHgoqVrvnJ0PVZQkBMBFKvsbu9WkWPUqHe0sx2cDUOdolelspfClO6iP7CYTUQQqyDov9zByDiBfQ7rILQ_LcwPG6UAAbEgM0pC_lntsPzbdcq0V-rE_OMO6y7HtmGN7GPhYHGU0K4qQBuYI_Pdn2gqyCEciI6_awB1LG4RwfoC8ietGUuGmxdcl2PWm0DI-Kj1f1bNlwc-54LKg8v5K54zsBdmK4SrrJ6Nt6OgCqq3On7zHfTDFN01dqWP6EoQHhEn6Akx5HiioTW3CHSVq6pd09Po5cgAAIoQE2U0";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": 2345
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}


BOOST_AUTO_TEST_CASE(permission_basic_read_with_wildcard_path)
{
/*
    Token looks like this.

  {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "r"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQuRW5naW5lU3BlZWQiOiJyIn19.jbhdMq5hEWXXNfZn9xE4rECIWEVsw-q3g-jxp5lLS0VAZ2WYOGoSd5JX2P9YG0Lals7Ue0wdXtgLSFtvIGU4Ol2MuPaF-Rbb-Q5O4njxg60AZ00kr6RywpyGZHK0eT9MuFCnVMN8Krf3lo2pPB1ms-WAHX6rfArbXxx0FsMau9Ewn_3m3Sc-6sz5alQw1Y7Rk0GD9Y7WP_mbICU__gd40Ypu_ki1i59M8ba5GNfd8RytEIJXAg7RTcKREWDZfMdFH5X7F6gAPA7h_tVH3-bsbT-nOsKCbM-3PM0EKAOl104SwmKcqoWnfXbUow5zt25O8LYwmrukuRBtWiLI5FxeW6ovmS-1acvS3w1LXlQZVGWtM_ZC7shtHh-iz7nyL1WCTpZswHgoqVrvnJ0PVZQkBMBFKvsbu9WkWPUqHe0sx2cDUOdolelspfClO6iP7CYTUQQqyDov9zByDiBfQ7rILQ_LcwPG6UAAbEgM0pC_lntsPzbdcq0V-rE_OMO6y7HtmGN7GPhYHGU0K4qQBuYI_Pdn2gqyCEciI6_awB1LG4RwfoC8ietGUuGmxdcl2PWm0DI-Kj1f1bNlwc-54LKg8v5K54zsBdmK4SrrJ6Nt6OgCqq3On7zHfTDFN01dqWP6EoQHhEn6Akx5HiioTW3CHSVq6pd09Po5cgAAIoQE2U0";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.VehicleIdentification.*",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
    "action": "get",
    "requestId": "8756",
    "value": [{"Vehicle.VehicleIdentification.vehicleinteriorType":"---"},{"Vehicle.VehicleIdentification.vehicleinteriorColor":"---"},{"Vehicle.VehicleIdentification.vehicleSpecialUsage":"---"},{"Vehicle.VehicleIdentification.vehicleSeatingCapacity":"---"},{"Vehicle.VehicleIdentification.vehicleModelDate":"---"},{"Vehicle.VehicleIdentification.vehicleConfiguration":"---"},{"Vehicle.VehicleIdentification.purchaseDate":"---"},{"Vehicle.VehicleIdentification.productionDate":"---"},{"Vehicle.VehicleIdentification.meetsEmissionStandard":"---"},{"Vehicle.VehicleIdentification.knownVehicleDamages":"---"},{"Vehicle.VehicleIdentification.dateVehicleFirstRegistered":"---"},{"Vehicle.VehicleIdentification.bodyType":"---"},{"Vehicle.VehicleIdentification.Year":"---"},{"Vehicle.VehicleIdentification.WMI":"---"},{"Vehicle.VehicleIdentification.VIN":"---"},{"Vehicle.VehicleIdentification.Model":"---"},{"Vehicle.VehicleIdentification.Brand":"---"},{"Vehicle.VehicleIdentification.ACRISSCode":"---"}]
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_read_with_branch_path)
{
/*
    Token looks like this.

  {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "r"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQuRW5naW5lU3BlZWQiOiJyIn19.jbhdMq5hEWXXNfZn9xE4rECIWEVsw-q3g-jxp5lLS0VAZ2WYOGoSd5JX2P9YG0Lals7Ue0wdXtgLSFtvIGU4Ol2MuPaF-Rbb-Q5O4njxg60AZ00kr6RywpyGZHK0eT9MuFCnVMN8Krf3lo2pPB1ms-WAHX6rfArbXxx0FsMau9Ewn_3m3Sc-6sz5alQw1Y7Rk0GD9Y7WP_mbICU__gd40Ypu_ki1i59M8ba5GNfd8RytEIJXAg7RTcKREWDZfMdFH5X7F6gAPA7h_tVH3-bsbT-nOsKCbM-3PM0EKAOl104SwmKcqoWnfXbUow5zt25O8LYwmrukuRBtWiLI5FxeW6ovmS-1acvS3w1LXlQZVGWtM_ZC7shtHh-iz7nyL1WCTpZswHgoqVrvnJ0PVZQkBMBFKvsbu9WkWPUqHe0sx2cDUOdolelspfClO6iP7CYTUQQqyDov9zByDiBfQ7rILQ_LcwPG6UAAbEgM0pC_lntsPzbdcq0V-rE_OMO6y7HtmGN7GPhYHGU0K4qQBuYI_Pdn2gqyCEciI6_awB1LG4RwfoC8ietGUuGmxdcl2PWm0DI-Kj1f1bNlwc-54LKg8v5K54zsBdmK4SrrJ6Nt6OgCqq3On7zHfTDFN01dqWP6EoQHhEn6Akx5HiioTW3CHSVq6pd09Po5cgAAIoQE2U0";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.VehicleIdentification",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
                   "action":"get",
                   "requestId":"8756",
                   "value":[{"Vehicle.VehicleIdentification.vehicleinteriorType":"---"},{"Vehicle.VehicleIdentification.vehicleinteriorColor":"---"},{"Vehicle.VehicleIdentification.vehicleSpecialUsage":"---"},{"Vehicle.VehicleIdentification.vehicleSeatingCapacity":"---"},{"Vehicle.VehicleIdentification.vehicleModelDate":"---"},{"Vehicle.VehicleIdentification.vehicleConfiguration":"---"},{"Vehicle.VehicleIdentification.purchaseDate":"---"},{"Vehicle.VehicleIdentification.productionDate":"---"},{"Vehicle.VehicleIdentification.meetsEmissionStandard":"---"},{"Vehicle.VehicleIdentification.knownVehicleDamages":"---"},{"Vehicle.VehicleIdentification.dateVehicleFirstRegistered":"---"},{"Vehicle.VehicleIdentification.bodyType":"---"},{"Vehicle.VehicleIdentification.Year":"---"},{"Vehicle.VehicleIdentification.WMI":"---"},{"Vehicle.VehicleIdentification.VIN":"---"},{"Vehicle.VehicleIdentification.Model":"---"},{"Vehicle.VehicleIdentification.Brand":"---"},{"Vehicle.VehicleIdentification.ACRISSCode":"---"}]
        })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_read_with_non_permitted_path)
{
/*
    Token looks like this.

  {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "r"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQuRW5naW5lU3BlZWQiOiJyIn19.VDr_UU_efYVaotMSRs-Ga7xf1a7ArEb0oTfJixTR4ah9aXnXYNHL-f4N4YZQ1A4mX3YhdcxifgV2kE1XwXE0XBoYjydy9g8pZhPm-fMF3z9zTeWRhPBSSHYLgcJQRINChtipmn1RIQxeeEqDX3E0n4utb0HfNXcEDTOKSeP4sFygjiDyEJYl_zn1JoMWfV8HJ9beYgVybKyMRLkM7FsGqT-aUOVfdxhH3nQSGFleI-nzFh6fFIaVbNdZo9u4moeIcaoeZJLJEe2410-xTtByxp_fN0OTxntHbloLSRLjY8MxC1hu1Uhbxs5GKPfJDV7ZhhbULzqqiRMSGn13ELJO-yPEnaHV8NZ8V9U6My-rBkEGgVcwCsbyDu-i7hAP8fepFCpyYfmZkypXxrZQHj8idJ16SXHNzNLL4Q7uiXSwc9oPSRI8FcbEwk-Ul7sD-X7kUqfFukO49NGUdobc-JhVzsJl-eofwe0H0Rq3hme6Rj1kFjitLYU2SyZjUsYrrGpYrghahh7MfSjf2lqNi159wfYtZtopoBrbPmAJ3HnzWXYM2ai0kroELRCaHz4adodMtem4qTBGJoYgG7hAg1OxvnbOdYOD7yZ46-RxBGgaoWuxAnQRHsnGs4j0SaXRTVWvTBHg21tI6AHo4wwyfUD4pGEaxB3M3bEH9m2hWl4HZpk";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   string response=commandProc_auth->processQuery(authReqJson.as_string(),channel);
   
   string request(R"({
		"action": "get",
		"path": "Vehicle.OBD.Speed",
		"requestId": "8756"
	})");

   response = commandProc_auth->processQuery(request,channel);

   json expected = json::parse(R"({
                   "action":"get",
                   "error":{"message":"No read access to Vehicle.OBD.Speed","number":403,"reason":"Forbidden"},
                   "requestId":"8756"
        })");

   json response_json = json::parse(response);


   //BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_read_with_invalid_permission_valid_path)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed1": "r"    (non-existing path)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLk9CRC5FbmdpbmVTcGVlZDEiOiJyIn19.rST_a1LunN5PT1UbqFfiSS5-75nX29fXh0ZUFbIGWADk7JInDDo7EP6tt2HmiX6vW3RoeKVoZCM_UB9fKfKmJMA9-pgBkeGjVcf8PmGzIJUKJPjPp3vRUrWop0UABeCqlnlQ7SFO3IlfcSarHg0P9SdmPQm6WOtJSqv806KIe5cLVhl9Ikr0cmYMO5IJ5IQzQP6cDcWl5zro75v5MtQbjHrNBKAm7qclQnBtx0oeMmdSmLQLJR0RLy10VeORtqqrSksy2QKmVkrmK2vX1GuwR-kOOEqBZWz9j8HJ4I05XpCdltKx1P41mQIWxZKUp87uKsqZBe_8V5Bd2bqYbP3MKyIxsZEUGUjjpbLogu1DBS05oJHc4_Al6AMclX5DVkWDL7M1HMGwgOAqIfsQwwiGJah6n9c43K2oDGHmsc0zrKmNcx-UDA7dqbg1PnrAWx7Ex__nI95zhoDYnbsoxLhg_tOVaCzvvA3pCU8IEDcTBSyDp-PUVzF6m3TbzJlwrLRP2_kzl48asn5U1fiMvGTFiVzRUv34uvnelQuK0954NsqnOi9tHAP2ljNrP75KuehAqmdWhHXWMkxoUFbQ974bWP6J0eYu1SnuQs2mR-3bf_HhxxPNI-5tgNOZ0ROwfDfHrOLdMP1RgoweLrpvmffwhV1aFTAQyvqjSJYVl9tZKfk";

   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc_auth->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string response = commandProc_auth->processQuery(request,channel);

   json expected = json::parse(R"({
                   "action":"get",
                   "error":{"message":"No read access to Vehicle.OBD.EngineSpeed","number":403,"reason":"Forbidden"},
                   "requestId":"8756"
        })");

   json response_json = json::parse(response);
   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");

   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_read_with_branch_permission_valid_path)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD": "r"    (branch permission)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQiOiJyIn19.aFM4kmqmupJmxkCXXPHUnmC5JC4ioIYwczk_YcGLy8ULfT4As6fzZNnAFTZVk-axBbIHnU3aUTigI5wZroF0ZkdWzJ1FA1-ZEXRLwUug9pigOC4hvTn4layT4tSEylHm3Mhh8M3Z0O4QKr-TuKZgZcbIICjrI3GQvm0kdkAO1hUTDPF2yQv16qCPuWvmJEDmy70MjzZfKIy94LCXL6Gf1OdIo4hXIbhLFPOR4ea8iaEz35VEjEZ828KbP8DCXRNWlad-CjAx7f4whS_YHctVZelFoa5G-MzNXRcr54VmkGbYM4WGDn6Twamsfb7YmwROzNmsI8DNahWWXciWxVZElsAexKx5yFXr3CslxG8dbgZgHWQ1tZe0Nq1b-4XUkomz6e6hd0iUF913D-TjvBACz4fpl5_28Wr0TMy84w1DvkpfNmgQ_1fiZNbho2uBxDoisfDWq_sI_ph0xO5mu-HKrs2T_tkuFveCPDHvIm1uZIzT8hX2cFDoBNClUlf4BOo6EHw3Ax9ISr28WzRxpZPOs9bYh4AIlnkqh28P91emK7pdb4eXhZKm3ZVWGEA17pLUUraxEhTYWXAFMsafwqTAaUhYt-JUBiExvHRvZpa49UDkXd4lJvl9gYO9YPiyrG6dioIsK9QSw-Mhob2WacRUzbG0O8V9uVApjw73tK4FYhA";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": 2345
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_read_with_branch_permission_valid_path_2)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD.*": "r"    (branch permission with a *)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQiOiJyIn19.aFM4kmqmupJmxkCXXPHUnmC5JC4ioIYwczk_YcGLy8ULfT4As6fzZNnAFTZVk-axBbIHnU3aUTigI5wZroF0ZkdWzJ1FA1-ZEXRLwUug9pigOC4hvTn4layT4tSEylHm3Mhh8M3Z0O4QKr-TuKZgZcbIICjrI3GQvm0kdkAO1hUTDPF2yQv16qCPuWvmJEDmy70MjzZfKIy94LCXL6Gf1OdIo4hXIbhLFPOR4ea8iaEz35VEjEZ828KbP8DCXRNWlad-CjAx7f4whS_YHctVZelFoa5G-MzNXRcr54VmkGbYM4WGDn6Twamsfb7YmwROzNmsI8DNahWWXciWxVZElsAexKx5yFXr3CslxG8dbgZgHWQ1tZe0Nq1b-4XUkomz6e6hd0iUF913D-TjvBACz4fpl5_28Wr0TMy84w1DvkpfNmgQ_1fiZNbho2uBxDoisfDWq_sI_ph0xO5mu-HKrs2T_tkuFveCPDHvIm1uZIzT8hX2cFDoBNClUlf4BOo6EHw3Ax9ISr28WzRxpZPOs9bYh4AIlnkqh28P91emK7pdb4eXhZKm3ZVWGEA17pLUUraxEhTYWXAFMsafwqTAaUhYt-JUBiExvHRvZpa49UDkXd4lJvl9gYO9YPiyrG6dioIsK9QSw-Mhob2WacRUzbG0O8V9uVApjw73tK4FYhA";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": 2345
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_read_with_wildcard_permission)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.*.EngineSpeed": "r"    (permission with a *)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS4qLkVuZ2luZVNwZWVkIjoiciJ9fQ.fUyHFOh3rD2IwOVMIYzNKdR4Y6IbKmQhN3Y2pGfcOy8SjXcE5MS6owIYRUVCxnlnH9-ywpNrwvePgKPHnjnWq8wSHr6I22zh3dNty0dFn-gJ82LQ-aNRKcweFqZXXP7-b-__Lc_ivEZpl-w0T9IzPWsUhZyt82XIPkzOZrfULv-DhXpoVIFTr-nz7KSpypcp0j_zXvbkf35bLLwcca7nMY5a9ftMKzMcv4xWekjPQQYvGchtLi1lOG1k8iHlf_cGsVEE4CK55x3bkrEjOYagT7WlRkMgR4F4HOzG0LNHsiXEVpNf17cs1Fsy6K5ObY-_J4BCx8wWGc7Bnkg4yap_3jG1IW_o7gcINcx4WiTNHz42LU6d0GQ9spc3vSP5vPm7J1aglBqazYf-tWRHp7QF8WDtAgenLpb4Ld4n_Aq5gHBWfOlt4tCyMgOgLlnzUJT1yc65vNesB7zUAFCdJ49kSV4Lwf0hv4W-hXl3wUPvb06yaff4U2_zrDQOc7GhoVLMzHmAYccNlDEMfM6HjQAnGLLIdvMxfs5g4a2CLKfxbOusRAQYNd4XwU4CpNAWabiu0FHIC43vy578zY3dpCHBOtpEC5csNEnHqyTSWdJwMy9BLmPneNM04NIHni-4ir4ExzK1TUmIDisk5_KBWmcjKyW-HX8k_u2gxylCf9I82Y0";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": 2345
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_read_with_wildcard_write_permission)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.*.EngineSpeed": "w"    (permission with a * but only to write)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLiouRW5naW5lU3BlZWQiOiJ3In19.ILMNoPpsF9OD_EhBRvm679kGFFzQVOWpxtws9flHjyoXBUuswfTYo6VTZySksyIv2EEVsFwgHG6EpYU18dXvpBXWI7KtUlkUy0ewiwNXZDsrpPVD1_dhlSs5aNvHsGJr2X_aHF-gZ89XtZ1sBBcza7FdtoMQMn9hyWs3rPi5d552uwxCpMO7EDwzb8rNrndEbtuORQmSCSHb74gVP5_thZvKmfCvJLOYaqXbtIff7CNJc5JtmOp7Suv2ICVhDAyiVRAF8wC5yXCRS_MfHRHT-VVO6PvnOzUkZU4VskkCz10L4XyHhj-2GEnXaPnVV76H1BZscLAnshoiR1I6rWAA4yvRcriJkmnM3DSJqvPJ4wQ7pZsZE48n6jfrPU1fZPGrCIMbJ67-YntL9XL0GHO2AdJTD9nj4sYpVVSPJPheQSLVbQwUJq7JPczkJ04kvDpLIOp27A8GnT0JN78liQSH4VUuExY3et2f5VbyGKZbEbwtV_R8WuVOuFKGlrI07KhpYsz3avVNwNZonjEuU7f6Uadad2zZh96nJ623BgbdNhMCC-WkjoXZos7JjKIXKKLH1_LyHIVpubZFlzh2GUJNItr7k-VlLodjrvqsoBcz-Zi0KyUco69X5qz9qousN4E3LPSP5btefq-D2nhFUkn2lW_YPyWA6Wtfi6Fauby0ZD8";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc_auth->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string response = commandProc_auth->processQuery(request,channel);

   json expected = json::parse(R"({
                   "action":"get",
                   "error":{"message":"No read access to Vehicle.OBD.EngineSpeed","number":403,"reason":"Forbidden"},
                   "requestId":"8756"
        })");

   json response_json = json::parse(response);
   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");

   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_read_with_wildcard_permission_wildcard_request)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.*.EngineSpeed": "r"    (permission with a *)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLiouRW5naW5lU3BlZWQiOiJyIn19.4L_gleoOJ29SR75go0qtBgYjwkwWE0ofYMMQJm884-ifCNihl3aSNhxjVy-6KUQckTd-OMiGkd1RRXElHfnaLfVSmpaFAIjupzvXaKwEoLa6duXfh3-qaEQPSCUxSjr8Tur3aXkj8nJEcSYBYUo_RTX-Srgto0dSTLQza0Lvm0aYPHhE6oIy2dniMIxwCSNxtarFt6-DWIxR3RqFcTVcuGtgn3A6JCm-WurI2zHsmF2BHrIfUYWuTdzUH16QLSfviTVcMjBLgGhVGdQPz22t4nKtdwcVMJkYfLsaU-GYuRi-ZMIrbi7F1oU8pU21U4Ja9C9CuR-H4Xhy2uG_FVQmog-_K-kR6_tndbM-AL7BnfOt-T_QpD-CSfCY0K5Y1bS_IuiM6MrXu5J2q1lfLp_TAK9YBWpuKRGhxdke7zHlSB37dpbiqZFIdGwfJ7Kq6XzqZNFRkWw7_XM2U4s5OXcw2JDklJYJ_EA1bUBhoPb-UHCqYxTP6G5OAHgD1Ydji93o3IsRSt6cX8o7hsmF79L3HvSzCm-qTN_EPhvbVfucJFj5phOd_9GjoFn3ySkwTINX_Pe3aT-Kz9qn5_Sb6utxw4eYvf_e_TYH6bAICjb9OmOyvH5gS9u2ieJ_1ra4SIGuZ25d86m3aRBybJSnFCprpsv63ziKXNGcL2vIGlCzKUc";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc_auth->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.OBD.*",
		"requestId": "8756"
	})");

   string response = commandProc_auth->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "requestId": "8756",
    "value":[{"Vehicle.OBD.EngineSpeed":"---"}]
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_read_with_wildcard_permission_branch_path_request)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.*.EngineSpeed": "r"    (permission with a *)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLiouRW5naW5lU3BlZWQiOiJyIn19.4L_gleoOJ29SR75go0qtBgYjwkwWE0ofYMMQJm884-ifCNihl3aSNhxjVy-6KUQckTd-OMiGkd1RRXElHfnaLfVSmpaFAIjupzvXaKwEoLa6duXfh3-qaEQPSCUxSjr8Tur3aXkj8nJEcSYBYUo_RTX-Srgto0dSTLQza0Lvm0aYPHhE6oIy2dniMIxwCSNxtarFt6-DWIxR3RqFcTVcuGtgn3A6JCm-WurI2zHsmF2BHrIfUYWuTdzUH16QLSfviTVcMjBLgGhVGdQPz22t4nKtdwcVMJkYfLsaU-GYuRi-ZMIrbi7F1oU8pU21U4Ja9C9CuR-H4Xhy2uG_FVQmog-_K-kR6_tndbM-AL7BnfOt-T_QpD-CSfCY0K5Y1bS_IuiM6MrXu5J2q1lfLp_TAK9YBWpuKRGhxdke7zHlSB37dpbiqZFIdGwfJ7Kq6XzqZNFRkWw7_XM2U4s5OXcw2JDklJYJ_EA1bUBhoPb-UHCqYxTP6G5OAHgD1Ydji93o3IsRSt6cX8o7hsmF79L3HvSzCm-qTN_EPhvbVfucJFj5phOd_9GjoFn3ySkwTINX_Pe3aT-Kz9qn5_Sb6utxw4eYvf_e_TYH6bAICjb9OmOyvH5gS9u2ieJ_1ra4SIGuZ25d86m3aRBybJSnFCprpsv63ziKXNGcL2vIGlCzKUc";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc_auth->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.OBD",
		"requestId": "8756"
	})");

   string response = commandProc_auth->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "requestId": "8756",
    "value": [{"Vehicle.OBD.EngineSpeed":"---"}]
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

// This test will take comparatively long time to process, therefore full permissions should be avoided. If required we could by-pass checks if full-permission granted.
BOOST_AUTO_TEST_CASE(permission_basic_read_with_full_read_permission)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle": "r"    (permission to read everything)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZSI6InIifX0.AKCWRDrtl4UkFzAGEzsyUZIKH4Q7jHg_Y5p8xVMLWtGTXIeFdQlz7FW-6rAy1Os9ad-H8ghEepcy2gs6DK3kr3FNDxqUyIaz8p_CDkwrgqp0Wczi1DsgwbyTsY2wsDa94Mja3ng2VszQgs4FZmsdbnzFzJeAsGucOZeIQj8w68up6YI6KXiCjO1094I2eixkclNnb1psPiucTkyListTLHxK3029fZT1EGGcrt7ziFYGiT5Z0Zk7x0PSN7dvmaT1rMOBWjbpluLCepkWZYt73ipO8YoAWhluMiW0sJI7ezrIU3UAKDkna_6kjAapm-9vAVTA63Tv4ovnRaALb_V2oxbIGCPTNoAY7ui00uFcxuN2B5l4M05OFgMIcxwS9-UEVQIWbFUxvPTQLNkOp12jd73ic786lUOs7fvuwseMZ2KX9cpV03TSAN1BIwG_TJ0iOQJ_5wuyDFRqwBK8zubg-zohUPMwsLpZgc7fTVI7AhXzGLZ57fE977NsluzfjFS0smtuzN-8JTvAMnCrgTQwu5GyTiL64a3NlYcQ2qt5K6D8xOIs7xSe8i0_PlKSgG8ECMOBE3JwSCwU1q83mTPCTSGHxTaNYnPhputc6gEiPv2VYmppGrMCeG2d5oLNzGhecIaLf_PGTTSmrUUZNK6HkuybgZStgMdN5uMTXrBkEvw";

   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": 2345
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_write)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD.Speed": "w"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQuU3BlZWQiOiJ3In19.dQGq3-OFFZSdhDYYd0IpoNvDh-p9Ak6twajzQVqY_MAWqdwc5BG6_AE6QJzvALAlCp2d-I8kE46YFM9kMGkyR7G5LUhT3h3_iEbsceyFBoPFgAwjgff-c5mZ7l7zFUEJAwtPPFysIku_uNOqmQnvUMLCJ6mBApfAaUc_08S7IUe6e6n3eu7mUdG7ZXBsg1ufcFzPZJHEjzwS4c5Nc2ZqJ4zJBMJJqq0K-TgcjsvNDYBz9is2_ClaeoWqwIZmKLszXfVViFww1Ou81ftuAfUcozyCvK3U1LIloIDxyhQ4kuz5J-Hzy2Y2SAWIZs_zwy34fijKTAj9AJSkZz_3X1dd0P0pFPkeuJUL0BVkC0PZRgtVk3IAWgSoCXOg5ra-vEPKemyoZVxGyUEbDM5D3hpSw01bl_YkZZEKvGniYGmrHelofp2NLLq9_ZkO_7_mYXedXoG82CxO315imv2x31tLg-9_oJSYP40PCRd2n4zALnywJEegn143GvSurOAwPRERtJhiI5WKbhPPwuhnhmvZVYkicPO6ZTO-8RwAB0CVaV9HjrY8Cutqb9gXppW3ajZEVRMxN4BHObqfAHeD_snZIp7GeGPRKEvshCKy7A3lynXpNYUAtkSSXUaqesjp9YoOvLM3ka1zQWNFCuLvpOFjCnR-myE4Vw4Kp-iC__Y1OxU";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "set",
		"path": "Vehicle.OBD.Speed",
                "value" : 200,
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "requestId":"8756"
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_write_not_permitted)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.OBD.Speed": "w"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLk9CRC5TcGVlZCI6IncifX0.HOAb7fh1IRTtIXGI3GlptjIZgUYGXKkF8Zp0A2CrqhYY3-5RzuuQ4cTzc_FQfn8hRO0CzF5hARGpvECR7fMxTyKMOelnqI9DJbCL_wyHIrizOo_fZwoEt_GH5AMKawryn3XkFsyOwCl74V8coYboycCpQXKyMHfz4zXGOh6Re9iXX7eBU6U0RCxCUso1pALmMvF2PoUHlZRtFJg1VhhErRoCSu5F-dCDykd04rorbzTOsgRuzHfAXoRQVprb4NwoT99jw1qg-Su0q8LxsDUtRfxdDHBmAXJVhmTzWSwzzD6da08PLoIojyIuSPk2AWTH8u0tMz9anI13I2R4nS6ikXeg3oVaxcXcj4gTnhYfJmwx0zyThuHSJ4lEpxYMAOgP49s6jpHdSBamOiTnB4Nkrm_stCcT4bCZCZ-9bjVxXkXZe49fLVDOrg1FhGJ5sJazYZC6z7JKly8BE-FO2bQiYiMB-XD7AEgPSp61nAY_JtSQTsxa3lEBSGJJQYLqcQMZ_aLIQY0fSUjrtjG4jQpi-walriUUyP87-1SqdIV12rgvOtn1sSHm0rwGfSKf43f3HnwngbotVkWaZ7t4NvBkLU2FMrMeiG_8x5Xjq1mnZlI_K3bMRg7yAzznrrDIe3sUxP0Etnjky0CNYpeqwYGZmHNRdHAUNnWaNVGRt_tJBZc";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc_auth->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "set",
		"path": "Vehicle.OBD.RPM",
                "value" : 200,
		"requestId": "8756"
	})");

   string response = commandProc_auth->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "error":{"message":"Path(s) in set request do not have write access or is invalid","number":403,"reason":"Forbidden"},
    "requestId":"8756"
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(permission_basic_write_with_wildcard_permission)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD.*": "rw"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQuKiI6InJ3In19.LLdJFIuoViNF4uVv1IL30fXPySoM1oCHxLrWm6xM_4eynvXqPvrCI9fW__GMI0d8PxBLpM8FhgG3ynVVlOuLV_Sl6lDImZlkNQiR02lJwFqf67RWVdI4f4uhdHdKjEpe0a0F-6e7McS__3qQYyjNuQAZIIXkZUIDyXye9upwNARj1wGPtZyNzSY1uyxmuc7MMPaILAIzL8ZnY_D9qgbpbiInGavZtDE_X1iy9GhxbUguP8oiVYn14-H6RBDIF0s5dXwXnJ0cm9Q2DTFpb0YRq4sMgTC4PT1Smdda_6Pj2ELmBjGbH7PYlqfVk1jVdSPGcUpU48e__qVitSRkEK_unM5CihrDVIy7nw3_KclIZJa8_af3kQ4x-leg-ErRCt78j5l0DDxIo5EtCxAeLbO7baZD6D1tPrb3vYkI_9zl1vzydp_nmNMS9QzCRq5yEJfP07gpvG0Z1O0tSLedSCG9cZJ-lJ3Oj3bqxNxk3ih6vKfjnvjAgli7wEP_etHofZmqs3NI-qtP6ldz93fBfAK_iApsSnG4PV7oS_-3UQIow6fUHAA8szn4Ad1ZYiaDsXRcHbdIoLEemGDllkRTYNBe_5vFDT3s1gY82L3fvgwAzTGZ2k46eh66Zx3SmuPgHlCQK6gR-6eAVn0jh_Tjk5vubtin6UdRjHF0Y4BvCwvE0bk";


   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "set",
		"path": "Vehicle.OBD.Speed",
                "value" : 345,
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "requestId":"8756"
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);

   // verify with a get request

    string get_request(R"({
		"action": "get",
		"path": "Vehicle.OBD.Speed",
		"requestId": "8756"
	})");

   string get_response = commandProc->processQuery(get_request,channel);

  json get_expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.Speed",
    "requestId": "8756",
    "value": 345
    })");

   json get_response_json = json::parse(get_response);


   BOOST_TEST(get_response_json.contains("timestamp") == true);

   // remove timestamp to match
   get_response_json.erase("timestamp");
   BOOST_TEST(get_response_json == get_expected);
}


BOOST_AUTO_TEST_CASE(permission_basic_write_with_branch_permission)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.OBD": "w"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLk9CRCI6IncifX0.SEWdQbdA8GIbxyC_2f1vV5CB_k7JF-krXnBplROgmvG5qLhZn8QTWkfjdEpPLhsnClpc3meX-bkAJA1T25ISJGRwg5fugRBtjgbpk9sSP3grRuhQVjZSvWoajIB7sHrRkI0J9ggS7jegQpZjGnrBh0nFuEAAbCQY6ToQCYWnz5phTqO9YKZK3AZ3JfPKIqxZ0_MSrrfK_VTWtSE5TDzo8_NVu9Mmqzz_2_EcPiF5r9te-Pe_y4WsVj1WGkbXQoSlLw4b5aTk19DipQAqTS7nL-b0ce1ljuKnSE5-ksM0zMMWQ2Fh_hJyppkUi-vDBQPL4ZHTBDl33AaZnwwlNbUbWOqbuEDKmJk7jRQBfLqKkh2NsifI4wY0Bhmbm9v8-wJESItBw3AVQI1uyan9KHO5OBaH-qKnY1CcGCnNq04-BEsXT5vlVbGlFoFXzB1pw4d27oHkm73Yz2or9MxO5Uv3UPMjqRy-czIdncfewLxBzgT13owfuAP9Fm2t5YXaRpSPiki-QI4Zf02iSRETIM6l5CdSLHAdqOnujYkZjjRauhulp1nuCMQuXxotmn-Mp9OHENtSonddmXC3NtDc5WDMir9lf1NdAItEFfeS8TmeyyTiqxERmxyu8heAeKiOKYQAIrbcNgap7ggiCAjKviQfv-sQIJvFJ0gmHNN9xqQkNTg";

   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc_auth->processQuery(authReqJson.as_string(),channel);
   
   string request(R"({
		"action": "set",
		"path": "Vehicle.OBD.Speed",
                "value" : 345,
		"requestId": "8756"
	})");

   string response = commandProc_auth->processQuery(request,channel);

/*
  json expected = json::parse(R"({
    "action":"set",
    "requestId":"8756"
    })");
    */
   //Current (12/2020) in server is, that "branch" node permission does not grant the right to modify
   //childs. So we test accordingly 
   json expected = json::parse(R"({
    "action":"set",
    "error":{"message":"Path(s) in set request do not have write access or is invalid","number":403,"reason":"Forbidden"},
    "requestId":"8756"
    })");
    

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);

   // verify with a get request

   string get_request(R"({
		"action": "get",
		"path": "Vehicle.OBD.Speed",
		"requestId": "8757"
	})");
   
   string get_response = commandProc_auth->processQuery(get_request,channel);
  // because only write access in the token.
  // Also: Current (12/2020) in server is, that "branch" node permission does not grant the right to modify
  // childs. So we test accordingly 

  json get_expected = json::parse(R"({
    "action": "get",
    "error":{"message":"No read access to Vehicle.OBD.Speed","number":403,"reason":"Forbidden"},
    "requestId": "8757"
    })");

   json get_response_json = json::parse(get_response);


   BOOST_TEST(get_response_json.contains("timestamp") == true);

   // remove timestamp to match
   get_response_json.erase("timestamp");
   BOOST_TEST(get_response_json == get_expected);

}


BOOST_AUTO_TEST_CASE(permission_basic_write_with_wildcard_in_permitted_path)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "wr",                     ("wr" or "rw" both work!)
    "Vehicle.OBD.Speed": "w"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQuRW5naW5lU3BlZWQiOiJ3ciIsIlZlaGljbGUuT0JELlNwZWVkIjoidyJ9fQ.R4Ulq0T84oiTJFb8scj-t4C-GnFQ0QvYVCd4glsXxiOlaNUIovZUehQwJAO5WK3b3Phz86yILuFCxNO7fsdHMmyUzNLhjiXMrL7Y2PU3gvr20EIoWYKyh52BFTH_YT6sB1EWfyhPb63_tWP0P2aa1JcXhBjAlXtmnIghjcj7KloH8MQGzKArjXa4R2NaKLH0FrO5aK8hBH3tevWp38Wae-fIypr4MgG-tXoKMt8juaE7RVDVTRiYyHJkCHjbZ0EZB9gAmy-_FyMiPxHNo8f49UtCGdBq82ZlQ_SKF6cMfH3iPw19BYG9ayIgzfEIm3HFhW8RdnxuxHzHYRtqaQKFYr37qNNk3lg4NRS3g9Mn4XA3ubi07JxBUcFl8_2ReJkcVqhua3ZiTcISkBmje6CUg1DmbH8-7SMaZhC-LJsZc8K9DBZN1cYCId7smhln5LcfjkZRh8N3d-hamrVRvfbdbee7_Ua-2SiJpWlPiIEgx65uYTV7flMgdnng0KVxv5-t_8QjySfKFruXE-HkYKN7TH8EqQA1RXuiDhj8bdFGtrB36HAlVah-cHnCCgL-p-29GceNIEoWJQT9hKWk8kQieXfJfiFUZPOxInDxHyUQEjblY049qMbU2kVSNvQ7nrmwP9OTjcXfnp7bndbstTHCGsVj1ixq8QF3tOdEGlC3Brg";

   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "set",
		"path": "Vehicle.*",
                "value" : [{"OBD.Speed":35}, {"OBD.EngineSpeed":50}],
		"requestId": "8756"
	})");

   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "requestId":"8756"
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);

   // verify with a get request the the previous set has not worked.

    string get_request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string get_response = commandProc->processQuery(get_request,channel);

  json get_expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": 50
    })");

   json get_response_json = json::parse(get_response);


   BOOST_TEST(get_response_json.contains("timestamp") == true);

   // remove timestamp to match
   get_response_json.erase("timestamp");
   BOOST_TEST(get_response_json == get_expected);
}


BOOST_AUTO_TEST_CASE(permission_basic_write_with_wildcard_in_unpermitted_path)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "wr"    ("wr" or "rw" both work!)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLk9CRC5FbmdpbmVTcGVlZCI6IndyIn19.O43MpewKMaYLyR0v_1c8STIiGt8fZMfkELda57RmVQxurdtOugT4iObXYF4xIDe68OOJPTk5KY4bYfCjHpYHuh9Cx1AeBAEya3x99KqIJsUhvzLuzvFO2iKbqgZOBt9miVPUC33tYuITNy3avUfzd12uC7a3fSGMlSoidtOfyBaUaxZJav-kGimgvhCUZvb4uHrjQW8ixDlXHgVtKddSmFj3F4fViJ2C0yFx_38c5F_sne5z55Y5zkDaXHVPRmskRZ5YyGpO4IF5KP5LgKR45iFU4_Pm3SmKfI3gpeF4AifyVT5zoE96Woj3KIvfnHG9Jog3ip-Kl4ikVkx_hyuSsvANGWaKx2xfmtrJ0GYQZqitMGBdW_VsCShInDJAkadSHzft6Efi8p0ysvKr5KvLUDb5vu28jSoAbMQs9ufOgUPNZvnTsYdVqaof-7Z7VEo_hJV3nQH5tFlvoMGrqQH2mGQTjDhHpNTWPotq3rMzDl8WbUjrJbDkFikKMByU-xv9pUWSWZ-ndxk45yLbpWUShZVsueoTDRO65kJvxuXPgmt68fiDSNJp4ZnmjeuC3TApv7UrqqPYKGUwpkz9gQF3_MLg2glEJUry5w9P-gEBObQ0mY5nM4W_5fMY_nDy1j1qUY4SsKi-0Xu45Dej3k-cqWoqWVwFEh3CiI4v7wQR_hY";

   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc_auth->processQuery(authReqJson.as_string(),channel);
   string request(R"({
		"action": "set",
		"path": "Vehicle.OBD.*",
                "value" : [{"Speed":35}, {"EngineSpeed":444}],
		"requestId": "8756"
	})");

   string response = commandProc_auth->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "error":{"message":"Path(s) in set request do not have write access or is invalid","number":403,"reason":"Forbidden"},
    "requestId":"8756"
    })");

   json response_json = json::parse(response);


   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);

   // verify with a get request the the previous set has not worked.

    string get_request(R"({
		"action": "get",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8756"
	})");

   string get_response = commandProc_auth->processQuery(get_request,channel);

  json get_expected = json::parse(R"({
    "action": "get",
    "path": "Vehicle.OBD.EngineSpeed",
    "requestId": "8756",
    "value": "---"
    })");

   json get_response_json = json::parse(get_response);


   BOOST_TEST(get_response_json.contains("timestamp") == true);

   // remove timestamp to match
   get_response_json.erase("timestamp");
   BOOST_TEST(get_response_json == get_expected);
}

//----------------------------------------------------subscription permission Tests ------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(subscription_test)
{
/*
    Token looks like this.
{
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "wr",
    "Vehicle.OBD.Speed": "w"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQuRW5naW5lU3BlZWQiOiJ3ciIsIlZlaGljbGUuT0JELlNwZWVkIjoidyJ9fQ.R4Ulq0T84oiTJFb8scj-t4C-GnFQ0QvYVCd4glsXxiOlaNUIovZUehQwJAO5WK3b3Phz86yILuFCxNO7fsdHMmyUzNLhjiXMrL7Y2PU3gvr20EIoWYKyh52BFTH_YT6sB1EWfyhPb63_tWP0P2aa1JcXhBjAlXtmnIghjcj7KloH8MQGzKArjXa4R2NaKLH0FrO5aK8hBH3tevWp38Wae-fIypr4MgG-tXoKMt8juaE7RVDVTRiYyHJkCHjbZ0EZB9gAmy-_FyMiPxHNo8f49UtCGdBq82ZlQ_SKF6cMfH3iPw19BYG9ayIgzfEIm3HFhW8RdnxuxHzHYRtqaQKFYr37qNNk3lg4NRS3g9Mn4XA3ubi07JxBUcFl8_2ReJkcVqhua3ZiTcISkBmje6CUg1DmbH8-7SMaZhC-LJsZc8K9DBZN1cYCId7smhln5LcfjkZRh8N3d-hamrVRvfbdbee7_Ua-2SiJpWlPiIEgx65uYTV7flMgdnng0KVxv5-t_8QjySfKFruXE-HkYKN7TH8EqQA1RXuiDhj8bdFGtrB36HAlVah-cHnCCgL-p-29GceNIEoWJQT9hKWk8kQieXfJfiFUZPOxInDxHyUQEjblY049qMbU2kVSNvQ7nrmwP9OTjcXfnp7bndbstTHCGsVj1ixq8QF3tOdEGlC3Brg";

   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as_string(),channel);


   string request(R"({
		"action": "subscribe",
		"path": "Vehicle.OBD.EngineSpeed",
		"requestId": "8778"
	})");

   string response = commandProc->processQuery(request,channel);
   json response_json = json::parse(response);

   json expected = json::parse(R"({
     "action":"subscribe",
     "requestId":"8778"
    })");

   BOOST_TEST(response_json.contains("timestamp") == true);
   BOOST_TEST(response_json.contains("subscriptionId") == true);

   int subID = response_json["subscriptionId"].as<int>();

   // checked if subid is available. now remove to assert.
   response_json.erase("subscriptionId");


   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);


   // now unsubscribe
   string request_unsub(R"({
		"action": "unsubscribe",
		"requestId": "8779"
   })");

   json temp_unsubreq = json::parse(request_unsub);
   temp_unsubreq["subscriptionId"] = subID;

   string unsub_response = commandProc->processQuery(temp_unsubreq.as_string(),channel);
   json unsub_response_json = json::parse(unsub_response);

   // assert timestamp and subid.
   BOOST_TEST(unsub_response_json.contains("timestamp") == true);
   BOOST_TEST(unsub_response_json.contains("subscriptionId") == true);

   // compare the subit passed and returned.
   BOOST_TEST(unsub_response_json["subscriptionId"].as<int>() == subID);

   // remove timestamp and subid to assert other part of the response.because these are variables.
   unsub_response_json.erase("subscriptionId");
   unsub_response_json.erase("timestamp");

   json expected_unsub = json::parse(R"({
     "action":"unsubscribe",
     "requestId":"8779"
    })");

   BOOST_TEST(unsub_response_json == expected_unsub);
}

BOOST_AUTO_TEST_CASE(subscription_test_wildcard_permission)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "kuksa-vss": {
    "Vehicle.OBD.*": "rw"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQuKiI6InJ3In19.LLdJFIuoViNF4uVv1IL30fXPySoM1oCHxLrWm6xM_4eynvXqPvrCI9fW__GMI0d8PxBLpM8FhgG3ynVVlOuLV_Sl6lDImZlkNQiR02lJwFqf67RWVdI4f4uhdHdKjEpe0a0F-6e7McS__3qQYyjNuQAZIIXkZUIDyXye9upwNARj1wGPtZyNzSY1uyxmuc7MMPaILAIzL8ZnY_D9qgbpbiInGavZtDE_X1iy9GhxbUguP8oiVYn14-H6RBDIF0s5dXwXnJ0cm9Q2DTFpb0YRq4sMgTC4PT1Smdda_6Pj2ELmBjGbH7PYlqfVk1jVdSPGcUpU48e__qVitSRkEK_unM5CihrDVIy7nw3_KclIZJa8_af3kQ4x-leg-ErRCt78j5l0DDxIo5EtCxAeLbO7baZD6D1tPrb3vYkI_9zl1vzydp_nmNMS9QzCRq5yEJfP07gpvG0Z1O0tSLedSCG9cZJ-lJ3Oj3bqxNxk3ih6vKfjnvjAgli7wEP_etHofZmqs3NI-qtP6ldz93fBfAK_iApsSnG4PV7oS_-3UQIow6fUHAA8szn4Ad1ZYiaDsXRcHbdIoLEemGDllkRTYNBe_5vFDT3s1gY82L3fvgwAzTGZ2k46eh66Zx3SmuPgHlCQK6gR-6eAVn0jh_Tjk5vubtin6UdRjHF0Y4BvCwvE0bk";

   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   string response=commandProc->processQuery(authReqJson.as_string(),channel);
   json response_json=json::parse(response);
   BOOST_TEST(response_json.contains("action") == true);
   BOOST_TEST(response_json["action"].as<std::string>() == "authorize");


   string request(R"({
		"action": "subscribe",
		"path": "Vehicle.OBD.Speed",
		"requestId": "8778"
	})");

   response = commandProc->processQuery(request,channel);
   response_json = json::parse(response);

   json expected = json::parse(R"({
     "action":"subscribe",
     "requestId":"8778"
    })");

   BOOST_TEST(response_json.contains("timestamp") == true);
   BOOST_TEST(response_json.contains("subscriptionId") == true);

   int subID = response_json["subscriptionId"].as<int>();

   // checked if subid is available. now remove to assert.
   response_json.erase("subscriptionId");


   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);


   // now unsubscribe
   string request_unsub(R"({
		"action": "unsubscribe",
		"requestId": "8779"
   })");

   json temp_unsubreq = json::parse(request_unsub);
   temp_unsubreq["subscriptionId"] = subID;

   string unsub_response = commandProc->processQuery(temp_unsubreq.as_string(),channel);
   json unsub_response_json = json::parse(unsub_response);

   // assert timestamp and subid.
   BOOST_TEST(unsub_response_json.contains("timestamp") == true);
   BOOST_TEST(unsub_response_json.contains("subscriptionId") == true);

   // compare the subit passed and returned.
   BOOST_TEST(unsub_response_json["subscriptionId"].as<int>() == subID);

   // remove timestamp and subid to assert other part of the response.because these are variables.
   unsub_response_json.erase("subscriptionId");
   unsub_response_json.erase("timestamp");

   json expected_unsub = json::parse(R"({
     "action":"unsubscribe",
     "requestId":"8779"
    })");

   BOOST_TEST(unsub_response_json == expected_unsub);
}

BOOST_AUTO_TEST_CASE(subscription_test_no_permission)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "wr",                     ("wr" or "rw" both work!)
    "Vehicle.OBD.Speed": "w"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLk9CRC5FbmdpbmVTcGVlZCI6IndyIiwiVmVoaWNsZS5PQkQuU3BlZWQiOiJ3In19.ob9r5oT8R5TEtQ1ZULw2jJZpplv3-hMSd9rgFqU0CUTg-G4zhMLwegTmNDgY2EAyhOQ2LSNXK46b-ftCiVINWfnOIlNAwTmXoRNNqJ5F81aVpt8ub1aMUt0B8iKHH1Vy9sNMDrdjbB8qnNK-9SodsEafxvHAN816b81FZplA8z4S5F1gRjiCeK_tI_ZltiGYHp5FKcu25WuQosasZOUiGS7i_WFLftSn59S4lb2cBgpYIVxsGYjsnDKxc_mTNJRaCF5W5kF9zMaOR2x0sXJaZRWQnl5ioCu6tbneUF9Sb5ri1hR5m720WdNuu9iBaWbYno8QtjS8Di5L6KoIRNAcvGkAXuhLhmhSTd4ANYd9Ccc2xU6v4YWiKR4Sq3PXRvSfI-RUBuniOGi_v4Bpe8571B6EBZ3sPI3lkdgekusjqo_Man0WUc-h56ZttWsaqPVFyrSBz7uVtlcRHCwXrLaTwF-7rvNKwR5n3tIy-QG2YCQqQVr7-2934-NSADY8yTuKg-7bvv0DQv0pvNwwPjXE0Y6DSXGsk2PdujD8qai2aW5bZd9LI2WNxFqftm_t-cdicKgipzZVCzwwK9kTbpPq5KesVUymc8TIhmJynrh7lIVh-1EfvEY8T-v374kVppQ-k6aZmPphMvcWNF04x07_rw1DWxKoLIX4tTBvXiApSWo";

   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc_auth->processQuery(authReqJson.as_string(),channel);


   string request(R"({
		"action": "subscribe",
		"path": "Vehicle.OBD.Speed",
		"requestId": "8778"
	})");

   string response = commandProc_auth->processQuery(request,channel);
   json response_json = json::parse(response);

   json expected = json::parse(R"({
                   "action":"subscribe",
                   "error":{"message":"no permission to subscribe to path Vehicle.OBD.Speed","number":403,"reason":"Forbidden"},
                   "requestId":"8778"
                   })");

   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}


BOOST_AUTO_TEST_CASE(subscription_test_invalidpath)
{
/*
    Token looks like this.

 {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "wr",                     ("wr" or "rw" both work!)
    "Vehicle.OBD.Speed": "w"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLk9CRC5FbmdpbmVTcGVlZCI6IndyIiwiVmVoaWNsZS5PQkQuU3BlZWQiOiJ3In19.ob9r5oT8R5TEtQ1ZULw2jJZpplv3-hMSd9rgFqU0CUTg-G4zhMLwegTmNDgY2EAyhOQ2LSNXK46b-ftCiVINWfnOIlNAwTmXoRNNqJ5F81aVpt8ub1aMUt0B8iKHH1Vy9sNMDrdjbB8qnNK-9SodsEafxvHAN816b81FZplA8z4S5F1gRjiCeK_tI_ZltiGYHp5FKcu25WuQosasZOUiGS7i_WFLftSn59S4lb2cBgpYIVxsGYjsnDKxc_mTNJRaCF5W5kF9zMaOR2x0sXJaZRWQnl5ioCu6tbneUF9Sb5ri1hR5m720WdNuu9iBaWbYno8QtjS8Di5L6KoIRNAcvGkAXuhLhmhSTd4ANYd9Ccc2xU6v4YWiKR4Sq3PXRvSfI-RUBuniOGi_v4Bpe8571B6EBZ3sPI3lkdgekusjqo_Man0WUc-h56ZttWsaqPVFyrSBz7uVtlcRHCwXrLaTwF-7rvNKwR5n3tIy-QG2YCQqQVr7-2934-NSADY8yTuKg-7bvv0DQv0pvNwwPjXE0Y6DSXGsk2PdujD8qai2aW5bZd9LI2WNxFqftm_t-cdicKgipzZVCzwwK9kTbpPq5KesVUymc8TIhmJynrh7lIVh-1EfvEY8T-v374kVppQ-k6aZmPphMvcWNF04x07_rw1DWxKoLIX4tTBvXiApSWo";

   WsChannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc_auth->processQuery(authReqJson.as_string(),channel);


   string request(R"({
		"action": "subscribe",
		"path": "Vehicle.OBD.EngineSpeed1",
		"requestId": "8778"
	})");

   string response = commandProc_auth->processQuery(request,channel);
   json response_json = json::parse(response);

   json expected = json::parse(R"({
                   "action":"subscribe",
                   "error":{"message":"I can not find Vehicle.OBD.EngineSpeed1 in my db","number":404,"reason":"Path not found"},
                   "requestId":"8778"
                   })");

   BOOST_TEST(response_json.contains("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

// SUBSCRIBE Test
BOOST_AUTO_TEST_CASE(process_sub_with_wildcard)
{
   /* token is 
   {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "wr",                     ("wr" or "rw" both work!)
    "Vehicle.OBD.Speed": "w"
  }
}*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLk9CRC5FbmdpbmVTcGVlZCI6IndyIiwiVmVoaWNsZS5PQkQuU3BlZWQiOiJ3In19.ob9r5oT8R5TEtQ1ZULw2jJZpplv3-hMSd9rgFqU0CUTg-G4zhMLwegTmNDgY2EAyhOQ2LSNXK46b-ftCiVINWfnOIlNAwTmXoRNNqJ5F81aVpt8ub1aMUt0B8iKHH1Vy9sNMDrdjbB8qnNK-9SodsEafxvHAN816b81FZplA8z4S5F1gRjiCeK_tI_ZltiGYHp5FKcu25WuQosasZOUiGS7i_WFLftSn59S4lb2cBgpYIVxsGYjsnDKxc_mTNJRaCF5W5kF9zMaOR2x0sXJaZRWQnl5ioCu6tbneUF9Sb5ri1hR5m720WdNuu9iBaWbYno8QtjS8Di5L6KoIRNAcvGkAXuhLhmhSTd4ANYd9Ccc2xU6v4YWiKR4Sq3PXRvSfI-RUBuniOGi_v4Bpe8571B6EBZ3sPI3lkdgekusjqo_Man0WUc-h56ZttWsaqPVFyrSBz7uVtlcRHCwXrLaTwF-7rvNKwR5n3tIy-QG2YCQqQVr7-2934-NSADY8yTuKg-7bvv0DQv0pvNwwPjXE0Y6DSXGsk2PdujD8qai2aW5bZd9LI2WNxFqftm_t-cdicKgipzZVCzwwK9kTbpPq5KesVUymc8TIhmJynrh7lIVh-1EfvEY8T-v374kVppQ-k6aZmPphMvcWNF04x07_rw1DWxKoLIX4tTBvXiApSWo";

   //json perm = json::parse(R"({"$['Vehicle']['children'][*]['children']['EngineSpeed']" : "wr"})");
   json perm = json::parse(R"({"Vehicle.*.EngineSpeed" : "wr"})");

    WsChannel channel;
    channel.setConnID(1234);
    channel.setAuthorized(true);
    channel.setAuthToken(AUTH_TOKEN);
    channel.setPermissions(perm);
    string request(R"({
                   "action": "subscribe",
                   "path": "Vehicle.*.EngineSpeed",
                   "requestId": "8778"
                   })");

   string response = commandProc_auth->processQuery(request,channel);
   
    json expected = json::parse(R"({
                                "action": "subscribe",
                                "requestId": "8778"
                                })");

#ifdef JSON_SIGNING_ON
    response = json_signer->decode(response);
#endif

    json response_json = json::parse(response);
    json request_json = json::parse(request);
    BOOST_TEST(response_json.contains("timestamp") == true);
    BOOST_TEST(response_json.contains("subscriptionId") == true);
    // remove timestamp to match
    response_json.erase("timestamp");
    response_json.erase("subscriptionId");
    request_json.erase("path");
    BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(process_sub_without_wildcard)
{
 /* token is 
   {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1924948800,
  "kuksa-vss": {
    "Vehicle.OBD.EngineSpeed": "wr",                     ("wr" or "rw" both work!)
    "Vehicle.OBD.Speed": "w"
  }
}*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE5MjQ5NDg4MDAsImt1a3NhLXZzcyI6eyJWZWhpY2xlLk9CRC5FbmdpbmVTcGVlZCI6IndyIiwiVmVoaWNsZS5PQkQuU3BlZWQiOiJ3In19.ob9r5oT8R5TEtQ1ZULw2jJZpplv3-hMSd9rgFqU0CUTg-G4zhMLwegTmNDgY2EAyhOQ2LSNXK46b-ftCiVINWfnOIlNAwTmXoRNNqJ5F81aVpt8ub1aMUt0B8iKHH1Vy9sNMDrdjbB8qnNK-9SodsEafxvHAN816b81FZplA8z4S5F1gRjiCeK_tI_ZltiGYHp5FKcu25WuQosasZOUiGS7i_WFLftSn59S4lb2cBgpYIVxsGYjsnDKxc_mTNJRaCF5W5kF9zMaOR2x0sXJaZRWQnl5ioCu6tbneUF9Sb5ri1hR5m720WdNuu9iBaWbYno8QtjS8Di5L6KoIRNAcvGkAXuhLhmhSTd4ANYd9Ccc2xU6v4YWiKR4Sq3PXRvSfI-RUBuniOGi_v4Bpe8571B6EBZ3sPI3lkdgekusjqo_Man0WUc-h56ZttWsaqPVFyrSBz7uVtlcRHCwXrLaTwF-7rvNKwR5n3tIy-QG2YCQqQVr7-2934-NSADY8yTuKg-7bvv0DQv0pvNwwPjXE0Y6DSXGsk2PdujD8qai2aW5bZd9LI2WNxFqftm_t-cdicKgipzZVCzwwK9kTbpPq5KesVUymc8TIhmJynrh7lIVh-1EfvEY8T-v374kVppQ-k6aZmPphMvcWNF04x07_rw1DWxKoLIX4tTBvXiApSWo";

   WsChannel channel;
   channel.setConnID(1234);
    
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   string res=commandProc_auth->processQuery(authReqJson.as_string(),channel);

    string request(R"({
                   "action": "subscribe",
                   "path": "Vehicle.OBD.EngineSpeed",
                   "requestId": "4243"
                   })");

    string response = commandProc_auth->processQuery(request,channel);
  
    json expected = json::parse(R"({
                                "action": "subscribe",
                                "requestId": "4243"
                                })");

#ifdef JSON_SIGNING_ON
    response = json_signer->decode(response);
#endif

    json response_json = json::parse(response);
    json request_json = json::parse(request);
    // TEST response for parameters
    BOOST_TEST(response_json.contains("timestamp") == true);
    BOOST_TEST(response_json.contains("subscriptionId") == true);
    // TEST request for parameters
    BOOST_TEST(request_json.contains("path") == true);
    // remove timestamp to match
    response_json.erase("timestamp");
    response_json.erase("subscriptionId");
    request_json.erase("path");
    BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(subscription_test_invalid_wildcard)
{
    /*
     Token looks like this.

     {
     "sub": "Example JWT",
     "iss": "Eclipse kuksa",
     "admin": true,
     "iat": 1516239022,
     "exp": 1609372800,
     "kuksa-vss": {
     "Signal.OBD.RPM": "wr",                     ("wr" or "rw" both work!)
     "Signal.OBD.Speed": "w"
     }
     }
     */
    string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiVmVoaWNsZS5PQkQuRW5naW5lU3BlZWQiOiJ3ciIsIlZlaGljbGUuT0JELlNwZWVkIjoidyJ9fQ.R4Ulq0T84oiTJFb8scj-t4C-GnFQ0QvYVCd4glsXxiOlaNUIovZUehQwJAO5WK3b3Phz86yILuFCxNO7fsdHMmyUzNLhjiXMrL7Y2PU3gvr20EIoWYKyh52BFTH_YT6sB1EWfyhPb63_tWP0P2aa1JcXhBjAlXtmnIghjcj7KloH8MQGzKArjXa4R2NaKLH0FrO5aK8hBH3tevWp38Wae-fIypr4MgG-tXoKMt8juaE7RVDVTRiYyHJkCHjbZ0EZB9gAmy-_FyMiPxHNo8f49UtCGdBq82ZlQ_SKF6cMfH3iPw19BYG9ayIgzfEIm3HFhW8RdnxuxHzHYRtqaQKFYr37qNNk3lg4NRS3g9Mn4XA3ubi07JxBUcFl8_2ReJkcVqhua3ZiTcISkBmje6CUg1DmbH8-7SMaZhC-LJsZc8K9DBZN1cYCId7smhln5LcfjkZRh8N3d-hamrVRvfbdbee7_Ua-2SiJpWlPiIEgx65uYTV7flMgdnng0KVxv5-t_8QjySfKFruXE-HkYKN7TH8EqQA1RXuiDhj8bdFGtrB36HAlVah-cHnCCgL-p-29GceNIEoWJQT9hKWk8kQieXfJfiFUZPOxInDxHyUQEjblY049qMbU2kVSNvQ7nrmwP9OTjcXfnp7bndbstTHCGsVj1ixq8QF3tOdEGlC3Brg";


    WsChannel channel;
    channel.setConnID(1234);
    string authReq(R"({
                   "action": "authorize",
                   "requestId": "878787"
                   })");
    json authReqJson = json::parse(authReq);
    authReqJson["tokens"] = AUTH_TOKEN;
    commandProc->processQuery(authReqJson.as<string>(),channel);


    string request(R"({
                   "action": "subscribe",
                   "path": "Signal.*.CatCamera",
                   "requestId": "878787"
                   })");

    string response = commandProc->processQuery(request,channel);
    json response_json = json::parse(response);

    json expected = json::parse(R"({
                                "action":"subscribe",
                                "error":{"message":"I can not find Signal.*.CatCamera in my db","number":404,"reason":"Path not found"},
                                "requestId":"878787"
                                })");

    BOOST_TEST(response_json.contains("timestamp") == true);
    // remove timestamp to match
    response_json.erase("timestamp");
    BOOST_TEST(response_json == expected);
}
