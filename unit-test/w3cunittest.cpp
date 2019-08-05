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
#define BOOST_TEST_MODULE w3c-unit-test
#include <boost/test/included/unit_test.hpp>

#include <string>
#include <limits>
#include "w3cunittest.hpp"
#include "exception.hpp"
// #include <jsoncons/json.hpp>

#include "accesschecker.hpp"
#include "authenticator.hpp"
#include "signing.hpp"
#include "subscriptionhandler.hpp"
#include "vssdatabase.hpp"
#include "vsscommandprocessor.hpp"
#include "wsserver.hpp"

using namespace std;
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
  "w3c-vss": {
    "Signal.OBD.*": "rw",
    "Signal.Chassis.Axle.*": "rw",
    "Signal.Drivetrain.*": "rw",
    "Signal.Cabin.Infotainment.*": "rw"
  }
}

This has be signed using a local private/pub key pair using RS256 Algorithm (asymetric). You could create one using www.jwt.io website.

*/
#define AUTH_JWT "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC4qIjoicnciLCJTaWduYWwuQ2hhc3Npcy5BeGxlLioiOiJydyIsIlNpZ25hbC5Ecml2ZXRyYWluLioiOiJydyIsIlNpZ25hbC5DYWJpbi5JbmZvdGFpbm1lbnQuKiI6InJ3In19.T27JfnRSlTUfRhSKruFjhMXdFT-Jo8eDYTeK-0BnoM9y3YUEV-ZAE0fAh4lLcB1PpfTwMw3kHkGHxXWK1YyN4IJeZDVkFR63bQuUHN5OpOVJxtZPN8oOmmA8F4aV8H54pPgGremsbEFuzzZuCxhQWk8rV0cuWA6675k3rxgnnQrsbFCRCWNQIxOSI6dhlhNS0SWOJg2Px2b5V88ddCPD04hYXZHSRK9uvE-5p40n8Rs5x8VyHNXfPOkTry7hUauddWQ6Nomj4iyJFodhXalQt22x8u7OR-iuV6CDp2gup-NWV1gCAO7GfXZgScrT1yjIbKEXKA8g98RUGxuxeEQY9SH65zVshP5brCrBERdo2dXNyrNJ3AuZt8ycSpk7R6TElRsf9jsGPX-N61MxBfsjMF0mUSZNDS_6UaAhEFskFRLDEYsS5HKMIwt5jFfgZ1E86TWH8yCR_zi71a69YdR_zc6FXl5L7B2C0Gghfih8Lzt1YCerAs9fsn4F6WYE1esiZHexPVV-uMYrPPMTec0GswEjuoM880zX6P7ZYuulyqm1ri4glrx18blYVaTNoROs_BtaP0Ac5Jr-SOKePxKuuAbc6xTjW9E7AmXCsmyl2sYfnmvVAOEQG1G7GSvVZCbsBxYWonCp1jn4Tf5nUy4ZZ5bTnoV2zathZJesk5epaps"


namespace bt = boost::unit_test;
#define PORT 8090

wsserver* webSocket;
subscriptionhandler* subhandler;
authenticator* authhandler;
accesschecker* accesshandler;
vssdatabase* database;
signing* json_signer;
vsscommandprocessor* commandProc;

w3cunittest unittestObj(false);

w3cunittest::w3cunittest(bool secure) {
  webSocket = new wsserver(PORT, secure);
  authhandler = new authenticator("","");
  accesshandler = new accesschecker(authhandler);
  subhandler = new subscriptionhandler(webSocket, authhandler, accesshandler);
  database = new vssdatabase(subhandler, accesshandler);
  commandProc = new vsscommandprocessor(database, authhandler , subhandler);
  json_signer = new signing();
  database->initJsonTree();
}

w3cunittest::~w3cunittest() {
   // delete json_signer;
   // delete database;
   // delete webSocket;
   // delete commandProc;
}

list<string> w3cunittest::test_wrap_getPathForGet(string path , bool &isBranch) {
    return database->getPathForGet(path , isBranch);
}

json w3cunittest::test_wrap_getPathForSet(string path,  json value) {
    return database->getPathForSet(path , value);
}

//--------Do not change the order of the tests in this file, because some results are dependent on the previous tests and data in the db-------
//----------------------------------------------------vssdatabase Tests ------------------------------------------------------------------------

// Get method tests
BOOST_AUTO_TEST_CASE(path_for_get_without_wildcard_simple)
{
    string test_path = "Signal.OBD.RPM"; // Pass a path to signal without wildcard.
    bool isBranch = false;
   
    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);
    string result = paths.back();

    BOOST_TEST(paths.size() ==  1);
    BOOST_TEST(result == "$['Signal']['children']['OBD']['children']['RPM']");
    BOOST_TEST(isBranch == false);

}

BOOST_AUTO_TEST_CASE(path_for_get_with_wildcard_simple)
{

    string test_path = "Signal.*.RPM"; // Pass a path to signal with wildcard.
    bool isBranch = false;
   
    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);
    string result = paths.back();

    BOOST_TEST(paths.size() ==  1);
    BOOST_TEST(result == "$['Signal']['children']['OBD']['children']['RPM']");
    BOOST_TEST(isBranch == false);

}

BOOST_AUTO_TEST_CASE(path_for_get_with_wildcard_multiple)
{

    string test_path = "Signal.Chassis.Axle.*.*.Left.*.Pressure"; // Pass a path to signal with wildcard.
    bool isBranch = false;
   
    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);
    BOOST_TEST(paths.size() ==  2);
    string result2 = paths.back();
    paths.pop_back();
    string result1 = paths.back();

    BOOST_TEST(result1 == "$['Signal']['children']['Chassis']['children']['Axle']['children']['Row1']['children']['Wheel']['children']['Left']['children']['Tire']['children']['Pressure']");
    BOOST_TEST(result2 == "$['Signal']['children']['Chassis']['children']['Axle']['children']['Row2']['children']['Wheel']['children']['Left']['children']['Tire']['children']['Pressure']");
    BOOST_TEST(isBranch == false);

}

BOOST_AUTO_TEST_CASE(test_get_branch)
{
    string test_path = "Signal.Vehicle"; // Pass a path to branch
    bool isBranch = false;
   
    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);
    string result = paths.back();

    BOOST_TEST(paths.size() ==  14);
    BOOST_TEST(isBranch == true);

}

BOOST_AUTO_TEST_CASE(test_get_branch_with_wildcard)
{
    string test_path = "Signal.*.Acceleration"; // Pass a path to branch with wildcard
    bool isBranch = false;
   
    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);
    string result = paths.back();

    BOOST_TEST(paths.size() ==  5);
    BOOST_TEST(isBranch == true);

}


BOOST_AUTO_TEST_CASE(test_get_random)
{
    string test_path = "scdKC"; // pass an invalid path with or without wildcard.
    bool isBranch = false;
   
    list<string> paths = unittestObj.test_wrap_getPathForGet(test_path , isBranch);

    BOOST_TEST(paths.size() ==  0);
    BOOST_TEST(isBranch == false);

}

// set method tests

BOOST_AUTO_TEST_CASE(path_for_set_without_wildcard_simple)
{
    string test_path = "Signal.OBD.RPM"; // pass a valid path without wildcard.
    json test_value;
    test_value["value"] = 123;
   
    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value);

    BOOST_TEST(paths.size() ==  1);

    BOOST_TEST(paths[0]["path"].as<string>() == "$['Signal']['children']['OBD']['children']['RPM']");
    

}

BOOST_AUTO_TEST_CASE(path_for_set_with_wildcard_simple)
{
    string test_path = "Signal.*"; // pass a valid path with wildcard.

    json test_value_array = json::make_array(2); 
    json test_value1, test_value2;
    test_value1["OBD.RPM"] = 123;
    test_value2["OBD.Speed"] = 234;

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;
   
    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value_array);

    BOOST_TEST(paths.size() ==  2);

    BOOST_TEST(paths[0]["path"].as<string>() == "$['Signal']['children']['OBD']['children']['RPM']");
    BOOST_TEST(paths[1]["path"].as<string>() == "$['Signal']['children']['OBD']['children']['Speed']");
    
}


BOOST_AUTO_TEST_CASE(path_for_set_with_wildcard_with_invalid_values)
{
    string test_path = "Signal.*"; // pass a valid path with wildcard.

    json test_value_array = json::make_array(2); 
    json test_value1, test_value2;
    test_value1["OBD1.RPM"] = 123;   // pass invalid sub-path
    test_value2["OBD.Speed1"] = 234;  // pass invalid sub-path

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;
   
    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value_array);

    BOOST_TEST(paths.size() ==  2);

    BOOST_TEST(paths[0]["path"].as<string>() == "");
    BOOST_TEST(paths[1]["path"].as<string>() == "");
    
}

BOOST_AUTO_TEST_CASE(path_for_set_with_wildcard_with_one_valid_value)
{
    string test_path = "Signal.*"; // pass a valid path with wildcard.

    json test_value_array = json::make_array(2); 
    json test_value1, test_value2;
    test_value1["OBD.RPM"] = 123;   // pass valid sub-path
    test_value2["OBD.Speed1"] = 234;  // pass invalid sub-path

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;
   
    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value_array);

    BOOST_TEST(paths.size() ==  2);

    BOOST_TEST(paths[0]["path"].as<string>() == "$['Signal']['children']['OBD']['children']['RPM']");
    BOOST_TEST(paths[1]["path"].as<string>() == "");
    
}

BOOST_AUTO_TEST_CASE(path_for_set_with_wildcard_with_invalid_path_valid_values)
{
    string test_path = "Signal.Vehicle.*"; // pass an invalid path.

    json test_value_array = json::make_array(2); 
    json test_value1, test_value2;
    test_value1["OBD.RPM"] = 123;   // pass valid sub-path
    test_value2["OBD.Speed"] = 234;  // pass valid sub-path

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;
   
    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value_array);

    BOOST_TEST(paths.size() ==  2);

    BOOST_TEST(paths[0]["path"].as<string>() == "");
    BOOST_TEST(paths[1]["path"].as<string>() == "");
    
}

BOOST_AUTO_TEST_CASE(test_set_value_on_branch)
{
    string test_path = "Signal.OBD"; // pass a valid branch path without wildcard.
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
    string test_path = "Signal.OBD.O2SensorsAlt.Bank2"; // pass a valid path with wildcard.

    json test_value_array = json::make_array(2); 
    json test_value1, test_value2;
    test_value1["Sensor1.Present"] = true;
    test_value2["Sensor2.Present"] = false;

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;
   
    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value_array);

    BOOST_TEST(paths.size() ==  2);

    BOOST_TEST(paths[0]["path"].as<string>() == "$['Signal']['children']['OBD']['children']['O2SensorsAlt']['children']['Bank2']['children']['Sensor1']['children']['Present']");
    BOOST_TEST(paths[1]["path"].as<string>() == "$['Signal']['children']['OBD']['children']['O2SensorsAlt']['children']['Bank2']['children']['Sensor2']['children']['Present']");
    
}

BOOST_AUTO_TEST_CASE(test_set_value_on_branch_with_invalid_values)
{
    string test_path = "Signal.OBD.O2SensorsAlt.Bank2"; // pass a valid branch path with wildcard.

    json test_value_array = json::make_array(2); 
    json test_value1, test_value2;
    test_value1["Sensor1.Presentsdc"] = true;
    test_value2["Sensor2.Presentsadc"] = false;

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;
   
    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value_array);

    BOOST_TEST(paths.size() ==  2);

    BOOST_TEST(paths[0]["path"].as<string>() == "");
    BOOST_TEST(paths[1]["path"].as<string>() == "");
    
}

BOOST_AUTO_TEST_CASE(test_set_value_on_branch_with_one_invalid_value)
{
    string test_path = "Signal.OBD.O2SensorsAlt.Bank2"; // pass a valid branch path with wildcard.

    json test_value_array = json::make_array(2); 
    json test_value1, test_value2;
    test_value1["Sensor1.Present"] = true;
    test_value2["Sensor2.Presentsadc"] = false;

    test_value_array[0] = test_value1;
    test_value_array[1] = test_value2;
   
    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value_array);

    BOOST_TEST(paths.size() ==  2);

    BOOST_TEST(paths[0]["path"].as<string>() == "$['Signal']['children']['OBD']['children']['O2SensorsAlt']['children']['Bank2']['children']['Sensor1']['children']['Present']");
    BOOST_TEST(paths[1]["path"].as<string>() == "");
    
}

BOOST_AUTO_TEST_CASE(set_get_test_all_datatypes_boundary_conditions)
{
    string test_path_Uint8 = "Signal.OBD.AcceleratorPositionE"; 
    string test_path_Uint16 = "Signal.OBD.WarmupsSinceDTCClear";
    string test_path_Uint32 = "Signal.OBD.TimeSinceDTCCleared";
    string test_path_int8 = "Signal.OBD.ShortTermFuelTrim2";
    string test_path_int16 = "Signal.OBD.FuelInjectionTiming";
    string test_path_int32 = "Signal.Drivetrain.Transmission.Speed";
    string test_path_boolean = "Signal.OBD.Status.MIL";
    string test_path_Float = "Signal.OBD.FuelRate";
    string test_path_Double =  "Signal.Cabin.Infotainment.Navigation.DestinatonSet.Latitude";      
    string test_path_string = "Signal.Cabin.Infotainment.Media.Played.URI";

    json result;
    wschannel channel;
    channel.setConnID(1234);
    string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
    json authReqJson = json::parse(authReq);
    authReqJson["tokens"] = AUTH_JWT;
    commandProc->processQuery(authReqJson.as<string>(),channel);
   
    string get_request(R"({
		"action": "get",
		"path": "Signal.OBD.Speed",
		"requestId": "8756"
	})");
    string response = commandProc->processQuery(get_request,channel);
    

//---------------------  Uint8 SET/GET TEST ------------------------------------
    json test_value_Uint8_boundary_low;
    test_value_Uint8_boundary_low = numeric_limits<uint8_t>::min();

    database->setSignal(channel,test_path_Uint8, test_value_Uint8_boundary_low);
    result = database->getSignal(channel, test_path_Uint8);
    
    BOOST_TEST(result["value"] == numeric_limits<uint8_t>::min());
    
    json test_value_Uint8_boundary_high;
    test_value_Uint8_boundary_high = numeric_limits<uint8_t>::max();

    database->setSignal(channel,test_path_Uint8, test_value_Uint8_boundary_high);
    result = database->getSignal(channel, test_path_Uint8);

    BOOST_TEST(result["value"] == numeric_limits<uint8_t>::max());

    json test_value_Uint8_boundary_middle;
    test_value_Uint8_boundary_middle = numeric_limits<uint8_t>::max() / 2;

    database->setSignal(channel,test_path_Uint8, test_value_Uint8_boundary_middle);
    result = database->getSignal(channel, test_path_Uint8);

    BOOST_TEST(result["value"] == numeric_limits<uint8_t>::max() / 2);

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
    
    BOOST_TEST(result["value"] == numeric_limits<uint16_t>::min());
    
    json test_value_Uint16_boundary_high;
    test_value_Uint16_boundary_high = numeric_limits<uint16_t>::max();

    database->setSignal(channel,test_path_Uint16, test_value_Uint16_boundary_high);
    result = database->getSignal(channel, test_path_Uint16);

    BOOST_TEST(result["value"] == numeric_limits<uint16_t>::max());

    json test_value_Uint16_boundary_middle;
    test_value_Uint16_boundary_middle = numeric_limits<uint16_t>::max() / 2;

    database->setSignal(channel,test_path_Uint16, test_value_Uint16_boundary_middle);
    result = database->getSignal(channel, test_path_Uint16);

    BOOST_TEST(result["value"] == numeric_limits<uint16_t>::max() / 2);

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
    
    BOOST_TEST(result["value"] == numeric_limits<uint32_t>::min());
    
    json test_value_Uint32_boundary_high;
    test_value_Uint32_boundary_high = numeric_limits<uint32_t>::max();

    database->setSignal(channel,test_path_Uint32, test_value_Uint32_boundary_high);
    result = database->getSignal(channel, test_path_Uint32);

    BOOST_TEST(result["value"] == numeric_limits<uint32_t>::max());

    json test_value_Uint32_boundary_middle;
    test_value_Uint32_boundary_middle = numeric_limits<uint32_t>::max() / 2;

    database->setSignal(channel,test_path_Uint32, test_value_Uint32_boundary_middle);
    result = database->getSignal(channel, test_path_Uint32);

    BOOST_TEST(result["value"] == numeric_limits<uint32_t>::max() / 2);

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
    
    BOOST_TEST(result["value"] == numeric_limits<int8_t>::min());
    
    json test_value_int8_boundary_high;
    test_value_int8_boundary_high = numeric_limits<int8_t>::max();

    database->setSignal(channel,test_path_int8, test_value_int8_boundary_high);
    result = database->getSignal(channel, test_path_int8);

    BOOST_TEST(result["value"] == numeric_limits<int8_t>::max());

    json test_value_int8_boundary_middle;
    test_value_int8_boundary_middle = numeric_limits<int8_t>::max() / 2;

    database->setSignal(channel,test_path_int8, test_value_int8_boundary_middle);
    result = database->getSignal(channel, test_path_int8);

    BOOST_TEST(result["value"] == numeric_limits<int8_t>::max() / 2);

    json test_value_int8_boundary_middle_neg;
    test_value_int8_boundary_middle_neg = numeric_limits<int8_t>::min() / 2;

    database->setSignal(channel,test_path_int8, test_value_int8_boundary_middle_neg);
    result = database->getSignal(channel, test_path_int8);

    BOOST_TEST(result["value"] == numeric_limits<int8_t>::min() / 2);

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
    
    BOOST_TEST(result["value"] == numeric_limits<int16_t>::min());
    
    json test_value_int16_boundary_high;
    test_value_int16_boundary_high = numeric_limits<int16_t>::max();

    database->setSignal(channel,test_path_int16, test_value_int16_boundary_high);
    result = database->getSignal(channel, test_path_int16);

    BOOST_TEST(result["value"] == numeric_limits<int16_t>::max());

    json test_value_int16_boundary_middle;
    test_value_int16_boundary_middle = numeric_limits<int16_t>::max()/2;

    database->setSignal(channel,test_path_int16, test_value_int16_boundary_middle);
    result = database->getSignal(channel, test_path_int16);

    BOOST_TEST(result["value"] == numeric_limits<int16_t>::max()/2);

    json test_value_int16_boundary_middle_neg;
    test_value_int16_boundary_middle_neg = numeric_limits<int16_t>::min()/2;

    database->setSignal(channel,test_path_int16, test_value_int16_boundary_middle_neg);
    result = database->getSignal(channel, test_path_int16);

    BOOST_TEST(result["value"] == numeric_limits<int16_t>::min()/2);

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
    
    BOOST_TEST(result["value"] == numeric_limits<int32_t>::min());
    
    json test_value_int32_boundary_high;
    test_value_int32_boundary_high = numeric_limits<int32_t>::max() ;

    database->setSignal(channel,test_path_int32, test_value_int32_boundary_high);
    result = database->getSignal(channel, test_path_int32);

    BOOST_TEST(result["value"] == numeric_limits<int32_t>::max());

    json test_value_int32_boundary_middle;
    test_value_int32_boundary_middle = numeric_limits<int32_t>::max() / 2;

    database->setSignal(channel,test_path_int32, test_value_int32_boundary_middle);
    result = database->getSignal(channel, test_path_int32);

    BOOST_TEST(result["value"] == numeric_limits<int32_t>::max() / 2);

    json test_value_int32_boundary_middle_neg;
    test_value_int32_boundary_middle_neg = numeric_limits<int32_t>::min() / 2;

    database->setSignal(channel,test_path_int32, test_value_int32_boundary_middle_neg);
    result = database->getSignal(channel, test_path_int32);

    BOOST_TEST(result["value"] == numeric_limits<int32_t>::min() / 2);
   
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

    
    BOOST_TEST(result["value"] == std::numeric_limits<float>::min());
    
    json test_value_Float_boundary_high;
    test_value_Float_boundary_high = std::numeric_limits<float>::max();

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_high);
    result = database->getSignal(channel, test_path_Float);

    BOOST_TEST(result["value"] == std::numeric_limits<float>::max());


    json test_value_Float_boundary_low_neg;
    test_value_Float_boundary_low_neg = std::numeric_limits<float>::min() * -1;

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_low_neg);
    result = database->getSignal(channel, test_path_Float);

    
    BOOST_TEST(result["value"] == (std::numeric_limits<float>::min() * -1));
    
    json test_value_Float_boundary_high_neg;
    test_value_Float_boundary_high_neg = std::numeric_limits<float>::max() * -1;

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_high_neg);
    result = database->getSignal(channel, test_path_Float);

    BOOST_TEST(result["value"] == (std::numeric_limits<float>::max() * -1));


    json test_value_Float_boundary_middle;
    test_value_Float_boundary_middle = std::numeric_limits<float>::max() / 2;

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_middle);
    result = database->getSignal(channel, test_path_Float);

    
    BOOST_TEST(result["value"] == std::numeric_limits<float>::max() / 2);

    json test_value_Float_boundary_middle_neg;
    test_value_Float_boundary_middle_neg = std::numeric_limits<float>::min() * 2;

    database->setSignal(channel,test_path_Float, test_value_Float_boundary_middle_neg);
    result = database->getSignal(channel, test_path_Float);

    BOOST_TEST(result["value"] == std::numeric_limits<float>::min() * 2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_Float_boundary_low_outbound;
    double minFloat_value = numeric_limits<float>::min();
    test_value_Float_boundary_low_outbound = minFloat_value / 2;
    
    try {
       database->setSignal(channel,test_path_Float, test_value_Float_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_Float_boundary_high_outbound;
    double maxFloat_value = numeric_limits<float>::max();
    test_value_Float_boundary_high_outbound = maxFloat_value * 2;
    try {
       database->setSignal(channel,test_path_Float, test_value_Float_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);

    // Test negative out of bound
    isExceptionThrown = false;
    json test_value_Float_boundary_low_outbound_neg;
    double minFloat_value_neg = numeric_limits<float>::min() * -1;
    test_value_Float_boundary_low_outbound_neg = minFloat_value_neg / 2;
    
    try {
       database->setSignal(channel,test_path_Float, test_value_Float_boundary_low_outbound_neg);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_Float_boundary_high_outbound_neg;
    double maxFloat_value_neg = numeric_limits<float>::max() * -1;
    test_value_Float_boundary_high_outbound_neg = maxFloat_value_neg * 2;
    try {
       database->setSignal(channel,test_path_Float, test_value_Float_boundary_high_outbound_neg);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);

//---------------------  Double SET/GET TEST ------------------------------------

    json test_value_Double_boundary_low;
    test_value_Double_boundary_low = std::numeric_limits<double>::min();

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_low);
    result = database->getSignal(channel, test_path_Double);
    
    BOOST_TEST(result["value"] == std::numeric_limits<double>::min());
    
    json test_value_Double_boundary_high;
    test_value_Double_boundary_high = std::numeric_limits<double>::max();

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_high);
    result = database->getSignal(channel, test_path_Double);

    BOOST_TEST(result["value"] == std::numeric_limits<double>::max());

    
    json test_value_Double_boundary_low_neg;
    test_value_Double_boundary_low_neg = std::numeric_limits<double>::min() * -1;

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_low_neg);
    result = database->getSignal(channel, test_path_Double);
    
    BOOST_TEST(result["value"] == (std::numeric_limits<double>::min() * -1));
    
    json test_value_Double_boundary_high_neg;
    test_value_Double_boundary_high_neg = std::numeric_limits<double>::max() * -1;

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_high_neg);
    result = database->getSignal(channel ,test_path_Double);

    BOOST_TEST(result["value"] == (std::numeric_limits<double>::max() * -1));

   

    json test_value_Double_boundary_middle;
    test_value_Double_boundary_middle = std::numeric_limits<double>::max() / 2;

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_middle);
    result = database->getSignal(channel ,test_path_Double);

    BOOST_TEST(result["value"] == std::numeric_limits<double>::max() / 2);

    json test_value_Double_boundary_middle_neg;
    test_value_Double_boundary_middle_neg = std::numeric_limits<double>::min() * 2;

    database->setSignal(channel,test_path_Double, test_value_Double_boundary_middle_neg);
    result = database->getSignal(channel, test_path_Double);

    BOOST_TEST(result["value"] == std::numeric_limits<double>::min() * 2);

    // Test positive out of bound
    isExceptionThrown = false;
    json test_value_Double_boundary_low_outbound;
    long double minDouble_value = numeric_limits<double>::min();
    test_value_Double_boundary_low_outbound = minDouble_value / 2;
    try {
       database->setSignal(channel,test_path_Double, test_value_Double_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_Double_boundary_high_outbound;
    long double maxDouble_value = numeric_limits<double>::max();
    test_value_Double_boundary_high_outbound = maxDouble_value * 2;
    try {
       database->setSignal(channel,test_path_Double, test_value_Double_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);

    // Test negative out of bound
    isExceptionThrown = false;
    json test_value_Double_boundary_low_outbound_neg;
    long double minDouble_value_neg = numeric_limits<double>::min() * -1;
    test_value_Double_boundary_low_outbound_neg = minDouble_value_neg / 2;
    try {
       database->setSignal(channel,test_path_Double, test_value_Double_boundary_low_outbound_neg);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    // no negative infinite test


//---------------------  String SET/GET TEST ------------------------------------

    json test_value_String_empty;
    test_value_String_empty = "";

    database->setSignal(channel,test_path_string, test_value_String_empty);
    result = database->getSignal(channel,test_path_string);
    
    BOOST_TEST(result["value"] == "");

    json test_value_String_null;
    test_value_String_null = "\0";

    database->setSignal(channel,test_path_string, test_value_String_null);
    result = database->getSignal(channel,test_path_string);
    
    BOOST_TEST(result["value"] == "\0");

    json test_value_String_long;
    test_value_String_long = "hello to w3c vis server unit test with boost libraries! This is a test string to test string data type without special characters, but this string is pretty long";

    database->setSignal(channel,test_path_string, test_value_String_long);
    result = database->getSignal(channel,test_path_string);
    
    BOOST_TEST(result["value"] == test_value_String_long);

    json test_value_String_long_with_special_chars;
    test_value_String_long_with_special_chars = "hello to w3c vis server unit test with boost libraries! This is a test string conatains special chars like üö Ä? $ % #";

    database->setSignal(channel,test_path_string, test_value_String_long_with_special_chars);
    result = database->getSignal(channel,test_path_string);
    
    BOOST_TEST(result["value"] == test_value_String_long_with_special_chars);

//---------------------  Boolean SET/GET TEST ------------------------------------
    json test_value_bool_false;
    test_value_bool_false = false;

    database->setSignal(channel,test_path_boolean, test_value_bool_false);
    result = database->getSignal(channel,test_path_boolean);
    
    BOOST_TEST(result["value"] == test_value_bool_false);

    json test_value_bool_true;
    test_value_bool_true = true;

    database->setSignal(channel,test_path_boolean, test_value_bool_true);
    result = database->getSignal(channel,test_path_boolean);
    
    BOOST_TEST(result["value"] == test_value_bool_true);
}


BOOST_AUTO_TEST_CASE(test_set_random)
{
    string test_path = "Signal.lksdcl"; // pass an invalid path without wildcard.
    json test_value;
    test_value["value"] = 123;
   
    json paths = unittestObj.test_wrap_getPathForSet(test_path , test_value);

    BOOST_TEST(paths.size() ==  1);

    BOOST_TEST(paths[0]["path"].as<string>() == "");
}

// -------------------------------- Metadata test ----------------------------------

BOOST_AUTO_TEST_CASE(test_metadata_simple)
{
    string test_path = "Signal.OBD.RPM"; // pass a valid path without wildcard.   

    json result = database->getMetaData(test_path);

    json expected = json::parse(R"({
    "Signal": {
        "children": {
            "OBD": {
                "children": {
                    "RPM": {
                        "description": "PID 0C - Engine speed measured as rotations per minute",
                        "id": 1008,
                        "type": "Float",
                        "unit": "rpm"
                    }
                },
                "description": "OBD data.",
                "type": "branch"
            }
        },
        "description": "All signals that can dynamically be updated by the vehicle",
        "type": "branch"
    }
    })");

    BOOST_TEST(result ==  expected);  
}

BOOST_AUTO_TEST_CASE(test_metadata_with_wildcard)
{
    string test_path = "Signal.*.RPM"; // pass a valid path with wildcard.   

    json result = database->getMetaData(test_path);

    json expected = json::parse(R"({
    "Signal": {
        "children": {
            "OBD": {
                "children": {
                    "RPM": {
                        "description": "PID 0C - Engine speed measured as rotations per minute",
                        "id": 1008,
                        "type": "Float",
                        "unit": "rpm"
                    }
                },
                "description": "OBD data.",
                "type": "branch"
            }
        },
        "description": "All signals that can dynamically be updated by the vehicle",
        "type": "branch"
    }
    })");

    BOOST_TEST(result ==  expected);  
}

BOOST_AUTO_TEST_CASE(test_metadata_branch)
{
    string test_path = "Signal.Chassis.SteeringWheel"; // pass a valid branch path without wildcard.   

    json result = database->getMetaData(test_path);

    json expected = json::parse(R"({
    "Signal": {
        "children": {
            "Chassis": {
                "children": {
                    "SteeringWheel": {
                        "children": {
                            "Angle": {
                                "description": "Steering wheel angle. Positive = degrees to the left. Negative = degrees to the right.",
                                "id": 990,
                                "type": "Int16",
                                "unit": "degree"
                            },
                            "Extension": {
                                "description": "Steering wheel column extension from dashboard. 0 = Closest to dashboard. 100 = Furthest from dashboard.",
                                "id": 992,
                                "type": "UInt8",
                                "unit": "percentage"
                            },
                            "Tilt": {
                                "description": "Steering wheel column tilt. 0 = Lowest position. 100 = Highest position.",
                                "id": 991,
                                "type": "UInt8",
                                "unit": "percentage"
                            }
                        },
                        "description": "Steering wheel signals",
                        "type": "branch"
                    }
                },
                "description": "All signals concerning steering, suspension, wheels, and brakes.",
                "type": "branch"
            }
        },
        "description": "All signals that can dynamically be updated by the vehicle",
        "type": "branch"
    }
  }
)");

    BOOST_TEST(result ==  expected);  
}

BOOST_AUTO_TEST_CASE(test_metadata_branch_with_wildcard)
{
    string test_path = "Signal.*.SteeringWheel"; // pass a valid branch path with wildcard.   

    json result = database->getMetaData(test_path);

    json expected = json::parse(R"({
    "Signal": {
        "children": {
            "Chassis": {
                "children": {
                    "SteeringWheel": {
                        "children": {
                            "Angle": {
                                "description": "Steering wheel angle. Positive = degrees to the left. Negative = degrees to the right.",
                                "id": 990,
                                "type": "Int16",
                                "unit": "degree"
                            },
                            "Extension": {
                                "description": "Steering wheel column extension from dashboard. 0 = Closest to dashboard. 100 = Furthest from dashboard.",
                                "id": 992,
                                "type": "UInt8",
                                "unit": "percentage"
                            },
                            "Tilt": {
                                "description": "Steering wheel column tilt. 0 = Lowest position. 100 = Highest position.",
                                "id": 991,
                                "type": "UInt8",
                                "unit": "percentage"
                            }
                        },
                        "description": "Steering wheel signals",
                        "type": "branch"
                    }
                },
                "description": "All signals concerning steering, suspension, wheels, and brakes.",
                "type": "branch"
            }
        },
        "description": "All signals that can dynamically be updated by the vehicle",
        "type": "branch"
    }
  }
)");

    BOOST_TEST(result ==  expected);  
}


//----------------------------------------------------vsscommandprocessor Tests ------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(process_query_set_get_simple)
{
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   
   string set_request(R"({
		"action": "set",
		"path": "Signal.OBD.RPM",
		"requestId": "8750",
                "value" : 2345
	})");
   
   string set_response = commandProc->processQuery(set_request,channel);
   json set_response_json = json::parse(set_response);

   json set_response_expected = json::parse(R"({
    "action": "set",
    "requestId": 8750
     }
     )");


   BOOST_TEST(set_response_json.has_key("timestamp") == true);
   // remove timestamp to match
   set_response_json.erase("timestamp");
      
   BOOST_TEST(set_response_json == set_response_expected);


   string get_request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(get_request,channel);
#ifdef JSON_SIGNING_ON   
   response = json_signer->decode(response);
#endif 
   
   json expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756,
    "value": 2345
    })");
   
   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}


BOOST_AUTO_TEST_CASE(process_query_get_withwildcard)
{
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.*.RPM",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);
   
   json expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756,
    "value": 2345
    })");
   
#ifdef JSON_SIGNING_ON   
   response = json_signer->decode(response);
#endif 

   json response_json = json::parse(response);
   
   
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(process_query_set_get_withwildcard)
{
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   
   string set_request(R"({
		"action": "set",
		"path": "Signal.Chassis.Axle.*.Wheel.*.Tire.Temperature",
		"requestId": "8750",
                "value" : [{"Row1.Wheel.Right.Tire.Temperature":35}, {"Row1.Wheel.Left.Tire.Temperature":50}, {"Row2.Wheel.Right.Tire.Temperature":65}, {"Row2.Wheel.Left.Tire.Temperature":80}]
	})");
   
   string set_response = commandProc->processQuery(set_request,channel);
   json set_response_json = json::parse(set_response);

   json set_response_expected = json::parse(R"({
    "action": "set",
    "requestId": 8750
     }
     )");


   BOOST_TEST(set_response_json.has_key("timestamp") == true);
   // remove timestamp to match
   set_response_json.erase("timestamp");
      
   BOOST_TEST(set_response_json == set_response_expected);


   string get_request(R"({
		"action": "get",
		"path": "Signal.Chassis.Axle.*.Wheel.*.Tire.Temperature",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(get_request,channel);

  
   
   json expected = json::parse(R"({
    "action": "get",
    "requestId": 8756,
    "value": [
        {
            "Signal.Chassis.Axle.Row2.Wheel.Right.Tire.Temperature": 65
        },
        {
            "Signal.Chassis.Axle.Row2.Wheel.Left.Tire.Temperature": 80
        },
        {
            "Signal.Chassis.Axle.Row1.Wheel.Right.Tire.Temperature": 35
        },
        {
            "Signal.Chassis.Axle.Row1.Wheel.Left.Tire.Temperature": 50
        }
      ]
    }
    )");

#ifdef JSON_SIGNING_ON   
   response = json_signer->decode(response);
#endif 

   json response_json = json::parse(response);
   
   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(process_query_get_withwildcard_invalid)
{
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as<string>(),channel);
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
                         "error":{"message":"I can not find Signal.*.RPM1 in my db","number":404,"reason":"Path not found"},
                         "requestId":8756
                         })");
   
   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(process_query_set_withwildcard_invalid)
{
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "set",
		"path": "Signal.OBD.*",
                "value" : [{"RPM1" : 345} , {"Speed1" : 546}],
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);
   
   json expected = json::parse(R"({
                              "action":"set",
                              "error":{"message":"Path(s) in set request do not have write access or is invalid","number":403,"reason":"Forbidden"},
                              "requestId":8756
                              })");
   
   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(process_query_set_invalid_value)
{
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "set",
		"path": "Signal.OBD.*",
                "value" : [{"ThrottlePosition" : 34563843034034845945054}],
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);
   
   json expected = json::parse(R"({
                               "action":"set",
                               "error":{"message":"The type UInt8 with value 3.45638e+22 is out of bound","number":400,"reason":"Value passed is out of bounds"},
                               "requestId":8756
                               })");
   
   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}

BOOST_AUTO_TEST_CASE(process_query_set_one_valid_one_invalid_value)
{
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   
   string request(R"({
		"action": "set",
		"path": "Signal.OBD.*",
                "value" : [{"ThrottlePosition" : 34563843034034845945054},{"RPM" : 1000}],
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);
   
   json expected = json::parse(R"({
                               "action":"set",
                               "error":{"message":"The type UInt8 with value 3.45638e+22 is out of bound","number":400,"reason":"Value passed is out of bounds"},
                               "requestId":8756
                               })");
   
   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);

   
   string request_getvalid(R"({
		"action": "get",
		"path": "Signal.*.RPM",
		"requestId": "8756"
	})");
   
   string response_getvalid = commandProc->processQuery(request_getvalid,channel);
   
   // This shows that the value from the previous test is not set because of out of bound exception for the ThrottlePosition value, Therefore the RPM was not set despite being   valid. if you swap the order in which the signals are set in above test case then the value here would be 1000.
   json expected_getvalid = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756, 
    "value": 2345 
    })");

#ifdef JSON_SIGNING_ON   
   response_getvalid = json_signer->decode(response_getvalid);
#endif           
   
   json response_response_getvalid_json = json::parse(response_getvalid);
   

   BOOST_TEST(response_response_getvalid_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_response_getvalid_json.erase("timestamp");
   BOOST_TEST(response_response_getvalid_json == expected_getvalid);
}

//----------------------------------------------------json signing Tests ------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(json_signing)
{
   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_JWT;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   
   string get_request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(get_request,channel);
#ifdef JSON_SIGNING_ON   
   response = json_signer->decode(response);
#endif  
   json response_json = json::parse(response);
  
   BOOST_TEST(response_json.has_key("timestamp") == true);
   // remove timestamp to match
   response_json.erase("timestamp");
   
   json expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756,
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
  "w3c-vss": {
    "Signal.OBD.RPM": "r"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5SUE0iOiJyIn19.pm_T3Yktx_b5bv7d-O2FuM0aGCHlb6nlI2HcGigimdVJiRq6SGkWreaRYL9on3N2leMbyRSJa6B700CDBnikrLLWR5ZAceialToEdpnkIuTzRCq3xBvMiZ3Vlc2aAO9i_AdRaPv9BIsx_y6e90EaDzmZxy351bOfdJv8WDfx_E-nAOcUyA8vIC8cd7A1cdtz1AxXeu-C55K-GXxUIWxI-7543cR-grZJHeHdyOQDBcUlOmEFkGdaKlwy76bDOHcB4yCrj_oU7IXNx1mWmqtWQZRVnW4bWFzfgUDvD-cGuUrS0rF89MAt3JsTgeakwj5X4ouWv_p1rj3OwgSGZcDTRbTl2DgYOlP6MSPn4XTIkYSD_3ku-FQws-HbA63nwLiMzuUF7xDIlZnXs9sXwEGqmAoGhxF5nL0pfelbjlKWM-H_7vXABYMDo18b4fOXznJFS5rcxKzJqinosXKWhCPcZ5eTAPkXbH-n_ASBC-gaKYBJrN0r8aIvpLb08-M_5uuHcD7qxlqBKQEk66GnhdpZZEs4EHYv76QFvvCFyNdNPWR1gfnHATMfTsU6sbvRcIulST1scvp3BbG7ZJL5whuCpYfbOvW9sV8nLD1XUEIch1I5x4QSFoL4Z6RUQGfqxqc2sm7spMVomQ4TW_kunF3-nHSWPw06ZvRNMNIDyL8EHbo"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756,
    "value": 2345
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "w3c-vss": {
    "Signal.OBD.RPM": "r"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5SUE0iOiJyIn19.pm_T3Yktx_b5bv7d-O2FuM0aGCHlb6nlI2HcGigimdVJiRq6SGkWreaRYL9on3N2leMbyRSJa6B700CDBnikrLLWR5ZAceialToEdpnkIuTzRCq3xBvMiZ3Vlc2aAO9i_AdRaPv9BIsx_y6e90EaDzmZxy351bOfdJv8WDfx_E-nAOcUyA8vIC8cd7A1cdtz1AxXeu-C55K-GXxUIWxI-7543cR-grZJHeHdyOQDBcUlOmEFkGdaKlwy76bDOHcB4yCrj_oU7IXNx1mWmqtWQZRVnW4bWFzfgUDvD-cGuUrS0rF89MAt3JsTgeakwj5X4ouWv_p1rj3OwgSGZcDTRbTl2DgYOlP6MSPn4XTIkYSD_3ku-FQws-HbA63nwLiMzuUF7xDIlZnXs9sXwEGqmAoGhxF5nL0pfelbjlKWM-H_7vXABYMDo18b4fOXznJFS5rcxKzJqinosXKWhCPcZ5eTAPkXbH-n_ASBC-gaKYBJrN0r8aIvpLb08-M_5uuHcD7qxlqBKQEk66GnhdpZZEs4EHYv76QFvvCFyNdNPWR1gfnHATMfTsU6sbvRcIulST1scvp3BbG7ZJL5whuCpYfbOvW9sV8nLD1XUEIch1I5x4QSFoL4Z6RUQGfqxqc2sm7spMVomQ4TW_kunF3-nHSWPw06ZvRNMNIDyL8EHbo"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD.*",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
    "action": "get",
    "requestId": 8756,
    "value": [{"Signal.OBD.RPM":2345}]
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "w3c-vss": {
    "Signal.OBD.RPM": "r"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5SUE0iOiJyIn19.pm_T3Yktx_b5bv7d-O2FuM0aGCHlb6nlI2HcGigimdVJiRq6SGkWreaRYL9on3N2leMbyRSJa6B700CDBnikrLLWR5ZAceialToEdpnkIuTzRCq3xBvMiZ3Vlc2aAO9i_AdRaPv9BIsx_y6e90EaDzmZxy351bOfdJv8WDfx_E-nAOcUyA8vIC8cd7A1cdtz1AxXeu-C55K-GXxUIWxI-7543cR-grZJHeHdyOQDBcUlOmEFkGdaKlwy76bDOHcB4yCrj_oU7IXNx1mWmqtWQZRVnW4bWFzfgUDvD-cGuUrS0rF89MAt3JsTgeakwj5X4ouWv_p1rj3OwgSGZcDTRbTl2DgYOlP6MSPn4XTIkYSD_3ku-FQws-HbA63nwLiMzuUF7xDIlZnXs9sXwEGqmAoGhxF5nL0pfelbjlKWM-H_7vXABYMDo18b4fOXznJFS5rcxKzJqinosXKWhCPcZ5eTAPkXbH-n_ASBC-gaKYBJrN0r8aIvpLb08-M_5uuHcD7qxlqBKQEk66GnhdpZZEs4EHYv76QFvvCFyNdNPWR1gfnHATMfTsU6sbvRcIulST1scvp3BbG7ZJL5whuCpYfbOvW9sV8nLD1XUEIch1I5x4QSFoL4Z6RUQGfqxqc2sm7spMVomQ4TW_kunF3-nHSWPw06ZvRNMNIDyL8EHbo"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
                   "action":"get",
                   "requestId":8756,
                   "value":{"Signal.OBD.RPM":2345}
        })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "w3c-vss": {
    "Signal.OBD.RPM": "r"
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5SUE0iOiJyIn19.pm_T3Yktx_b5bv7d-O2FuM0aGCHlb6nlI2HcGigimdVJiRq6SGkWreaRYL9on3N2leMbyRSJa6B700CDBnikrLLWR5ZAceialToEdpnkIuTzRCq3xBvMiZ3Vlc2aAO9i_AdRaPv9BIsx_y6e90EaDzmZxy351bOfdJv8WDfx_E-nAOcUyA8vIC8cd7A1cdtz1AxXeu-C55K-GXxUIWxI-7543cR-grZJHeHdyOQDBcUlOmEFkGdaKlwy76bDOHcB4yCrj_oU7IXNx1mWmqtWQZRVnW4bWFzfgUDvD-cGuUrS0rF89MAt3JsTgeakwj5X4ouWv_p1rj3OwgSGZcDTRbTl2DgYOlP6MSPn4XTIkYSD_3ku-FQws-HbA63nwLiMzuUF7xDIlZnXs9sXwEGqmAoGhxF5nL0pfelbjlKWM-H_7vXABYMDo18b4fOXznJFS5rcxKzJqinosXKWhCPcZ5eTAPkXbH-n_ASBC-gaKYBJrN0r8aIvpLb08-M_5uuHcD7qxlqBKQEk66GnhdpZZEs4EHYv76QFvvCFyNdNPWR1gfnHATMfTsU6sbvRcIulST1scvp3BbG7ZJL5whuCpYfbOvW9sV8nLD1XUEIch1I5x4QSFoL4Z6RUQGfqxqc2sm7spMVomQ4TW_kunF3-nHSWPw06ZvRNMNIDyL8EHbo"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD.Speed",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
                   "action":"get",
                   "error":{"message":"No read access to  Signal.OBD.Speed","number":403,"reason":"Forbidden"},
                   "requestId":8756
        })");

   json response_json = json::parse(response);
   

   //BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "exp": 1609372800,
  "w3c-vss": {
    "Signal.OBD.RPM1": "r"    (invalid path)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5SUE0xIjoiciJ9fQ.KBeZPMMZz1ctRM-NExMrGU10oz9IXBC919LOfczfSLfcx9nZnQf_CuSyCZpgbcUBmo7ghirzvtFmYwE_rIMmLmSacZWPnzWmb5WHTSTp_oiLFrTBeLlOL12D97h4fgFm1yTglriu3ZQLCr55V6xE3IUteNBWPI84wya5IFzQJQZwJsfXoNIoq7DJ8cJr81r-5IsYL3K1MbPECTEsfykCV3KS3gRMiS3kctHE5upQHjB5y7Ef65QnlXxSaIF-0Q2Fy2e1Z_VVUQGX4Gky5BfFCtM7T4a8q1b0F-CREo3q_f5cLMWIWVvL5w-muF3kZosaoso-z5VKjWpwAV0A1oHxLlJhyR3Hd7UHLNfrLDC8CiNPk7j8CiwHBqnmrAOUodIZkGjd7AnBdPX659UBIuVeiGjReriTcNMIlmqce7aNw5lUqU-TQx9mJ9K7hgYI58ESeu2vi_IeCsa47-8BdmUDusVbMfMKyJfpom2Q02UEMvtI4kiH58Z20PbwbYa-ZF1Bl1jJ25PkyeCtNyj3bWqg5GVIu7O9kP7QOzCxZfPsrSMHxzlRTmnS1tH-m_ODiW3TaNyQkOYedWNhd4EE4mDcYeR4808H7ZjX4PG8Er2SPJRFxuydCr4O5JIEUyEcEX_CMPvZiZzJ6YS9Zi5pCeccEzPGt1TGfD5M8XbVl2-me3E"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
                   "action":"get",
                   "error":{"message":"No read access to  Signal.OBD.RPM","number":403,"reason":"Forbidden"},
                   "requestId":8756
        })");

   json response_json = json::parse(response);
   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "w3c-vss": {
    "Signal.OBD": "r"    (branch permission)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRCI6InIifX0.bc4Pt2oG5Ca4hfTmUIRHmfSWAVwlmj5_k6q2tG5ps6Nj5g57-6-3A-UE-ab9deKG0CGrr_L4kbO4tNE5Y4B3g2eRIBX5xbJ4YwNjqjUBvs0G-XLzDkTnGAuYA1CILxfJcJotwMrNg8SLp4xWLYXYZ58MTiMhlgOxfOIVArwrpEDomvoswl92dicEaiKV9NcBWR7f8c84ppY_T6ADgTvBp_pu-9tIGr66mWi5Q1xOFkCmMONh82Euq3XE1BmqonO5TcawZcXT9nDLSFpX0zhPGnoliHQUvYKahfBe4OaswLsfB7z7N1yazy4WyyjBxZI4Yd-URuhMCgNiFG-xki83oLzuTPDy9UdI97c57oNzwGXY7xJPAyAnS_72r-4dW4Z2zwTgfnoI1GcpwZ10qOqwq-43B4Hi9PnxBvKoQ6UgJMBKkRlW8vfWBmEoVbtIuEej10dXUg4mxSymj48jEfuQadEhIFF9rAkfF2Z9XuIpx8adjMVi3qZQTPd4-D5MtYEOfjJ2Sg8AS9amquznzACGgpyKCautQCbAK9B1XjU-on1X_HFBwHTzFegKnZqRQO3qycr_-6i0iAW5de__VvF9A0TYM3sTQRaKwJpY-EPBaaHGqSssfCXOIJPAlVOrmMiPlCYsWa-f87iw7iG8yaOy7gFnzGDVzo9I6R1J0nBtdpA"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756,
    "value": 2345
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "w3c-vss": {
    "Signal.OBD.*": "r"    (branch permission with a *)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRCI6InIifX0.bc4Pt2oG5Ca4hfTmUIRHmfSWAVwlmj5_k6q2tG5ps6Nj5g57-6-3A-UE-ab9deKG0CGrr_L4kbO4tNE5Y4B3g2eRIBX5xbJ4YwNjqjUBvs0G-XLzDkTnGAuYA1CILxfJcJotwMrNg8SLp4xWLYXYZ58MTiMhlgOxfOIVArwrpEDomvoswl92dicEaiKV9NcBWR7f8c84ppY_T6ADgTvBp_pu-9tIGr66mWi5Q1xOFkCmMONh82Euq3XE1BmqonO5TcawZcXT9nDLSFpX0zhPGnoliHQUvYKahfBe4OaswLsfB7z7N1yazy4WyyjBxZI4Yd-URuhMCgNiFG-xki83oLzuTPDy9UdI97c57oNzwGXY7xJPAyAnS_72r-4dW4Z2zwTgfnoI1GcpwZ10qOqwq-43B4Hi9PnxBvKoQ6UgJMBKkRlW8vfWBmEoVbtIuEej10dXUg4mxSymj48jEfuQadEhIFF9rAkfF2Z9XuIpx8adjMVi3qZQTPd4-D5MtYEOfjJ2Sg8AS9amquznzACGgpyKCautQCbAK9B1XjU-on1X_HFBwHTzFegKnZqRQO3qycr_-6i0iAW5de__VvF9A0TYM3sTQRaKwJpY-EPBaaHGqSssfCXOIJPAlVOrmMiPlCYsWa-f87iw7iG8yaOy7gFnzGDVzo9I6R1J0nBtdpA"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756,
    "value": 2345
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "w3c-vss": {
    "Signal.*.RPM": "r"    (permission with a *)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLiouUlBNIjoiciJ9fQ.iGOgeWBNMO2_2W9VK4bkodmdkC94Z4iRQqI2UPpIa0VyOtgSVg1weK4qkcdYdduzQYjefgFoEsiYr0U14cVt-0jqyKNIi88v0X7NSx8BTMnWHwg353r18sRVF_bQKQpEUCqURXkhoM0R-mF4MF6yb8TI0xL4Qfavj_AGbYYp7TG0gBm0fHyVcDqsbUE6zQvjR2C8V8vYYcFyt_pbt5Fc_sVz1uT_4JMQ0Dq4ZvyFZVtzJ-8fv89c3o4B2YgkFDhWtUxwO0TPtzi8mOsqfADRaQStmCJOAMiNfyS1AypxESYskMgTgPn-Z77IKwUhP0ZUxbe3uaXVvAvC-uyvWRhczCd1QTUKhC1wlIbbrhL_CGNCJq8m04CEl3QXNLXOQ3Wp8zlE3v6M1sBP4p8b6QYYMk6sYbX9ZxYT-pnFv8Bj1SYX-HnIej0tpSiU0kav6tMUxHLBDSjZmz-G0T_kG9HJPQiszUbIV1y61yOeYLpUNRBewAW0nerF-qo0FuYEX_K-KYC4fsQdQCbtxEH8erlzGoIxQaFbWKOipeotUUToYQNHqY6bXaaZnUl-rlQ-CuNEW4flPvh1gsTnlikWhVF1DviE_OHxdzR-UMtiZNq3oHFJK2bXSLwH7C3zzIs3ReORfYzxjkwv0yb_FOupmZmS_HSh_AqqgtihQ18VuIZBM2s"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756,
    "value": 2345
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "exp": 1609372800,
  "w3c-vss": {
    "Signal.*.RPM": "w"    (permission with a * but only to write)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLiouUlBNIjoidyJ9fQ.ED_82KyKihWuRZtCEotY5zqIfU0srLW1V4cecAZyz-fhAFjFcSlv7U4htYDOileq4PQaRP6Z_xR8WRmUl8APjOiCArUBIA2tl33k0OCDpg-MZfj_NGfDTfwKiwTlG1I6z-XGKo8TGVmTplrFCRoihZyVxFHAEZ4j-xoSoUGWrBHf4qI_x3Opt6s8EmcBieFgqSsZmvT1qjKc_GiPKttGrxQbhF_ylPWcKRfd159NU84Ivd5iyN6Wo1P_QVtOz6kc9LE2SPqS2pv-nt8IE9K0oZkHk8QKbFz_XXVuSwPBfW0F5NRUbP7czjRt9v1ShdwFqSqID3kM6jmMLQcZB1e1qhXNNGgb-urYjZeG5iLS447epsK1Luft-CSOEd6I0KtBD2KPWGdNaafXXJfNfWTXsM_--5BZX3rLNFOlDVssJDW3Rd4oGHnvcL-lzH_AbAauQjCBtD5PltdJaeQb8uDC171D1QtpK-pnFSWjv6L0RFTxF2VPGdPLNr1uWjX1z3ljS2s_ji4dJJsQNqH-8hu0_ikfNuBM3bLTLO2YyGXPriJQoqFaS0obHbxjaoz4N3csvcAMk4j-oyoHuFik1yk_q4TUsgVClNwDs2nLnxKyodsOToZbT59VGx0LUeKS7FafmeSJQZagMg0RGbLZQBVwJy7nNH-8bmGF0Jdul8Nl0Ys"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

   json expected = json::parse(R"({
                   "action":"get",
                   "error":{"message":"No read access to  Signal.OBD.RPM","number":403,"reason":"Forbidden"},
                   "requestId":8756
        })");

   json response_json = json::parse(response);
   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "exp": 1609372800,
  "w3c-vss": {
    "Signal.*.RPM": "r"    (permission with a *)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLiouUlBNIjoiciJ9fQ.iGOgeWBNMO2_2W9VK4bkodmdkC94Z4iRQqI2UPpIa0VyOtgSVg1weK4qkcdYdduzQYjefgFoEsiYr0U14cVt-0jqyKNIi88v0X7NSx8BTMnWHwg353r18sRVF_bQKQpEUCqURXkhoM0R-mF4MF6yb8TI0xL4Qfavj_AGbYYp7TG0gBm0fHyVcDqsbUE6zQvjR2C8V8vYYcFyt_pbt5Fc_sVz1uT_4JMQ0Dq4ZvyFZVtzJ-8fv89c3o4B2YgkFDhWtUxwO0TPtzi8mOsqfADRaQStmCJOAMiNfyS1AypxESYskMgTgPn-Z77IKwUhP0ZUxbe3uaXVvAvC-uyvWRhczCd1QTUKhC1wlIbbrhL_CGNCJq8m04CEl3QXNLXOQ3Wp8zlE3v6M1sBP4p8b6QYYMk6sYbX9ZxYT-pnFv8Bj1SYX-HnIej0tpSiU0kav6tMUxHLBDSjZmz-G0T_kG9HJPQiszUbIV1y61yOeYLpUNRBewAW0nerF-qo0FuYEX_K-KYC4fsQdQCbtxEH8erlzGoIxQaFbWKOipeotUUToYQNHqY6bXaaZnUl-rlQ-CuNEW4flPvh1gsTnlikWhVF1DviE_OHxdzR-UMtiZNq3oHFJK2bXSLwH7C3zzIs3ReORfYzxjkwv0yb_FOupmZmS_HSh_AqqgtihQ18VuIZBM2s"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD.*",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "requestId": 8756,
    "value":[{"Signal.OBD.RPM":2345}]
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "exp": 1609372800,
  "w3c-vss": {
    "Signal.*.RPM": "r"    (permission with a *)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLiouUlBNIjoiciJ9fQ.iGOgeWBNMO2_2W9VK4bkodmdkC94Z4iRQqI2UPpIa0VyOtgSVg1weK4qkcdYdduzQYjefgFoEsiYr0U14cVt-0jqyKNIi88v0X7NSx8BTMnWHwg353r18sRVF_bQKQpEUCqURXkhoM0R-mF4MF6yb8TI0xL4Qfavj_AGbYYp7TG0gBm0fHyVcDqsbUE6zQvjR2C8V8vYYcFyt_pbt5Fc_sVz1uT_4JMQ0Dq4ZvyFZVtzJ-8fv89c3o4B2YgkFDhWtUxwO0TPtzi8mOsqfADRaQStmCJOAMiNfyS1AypxESYskMgTgPn-Z77IKwUhP0ZUxbe3uaXVvAvC-uyvWRhczCd1QTUKhC1wlIbbrhL_CGNCJq8m04CEl3QXNLXOQ3Wp8zlE3v6M1sBP4p8b6QYYMk6sYbX9ZxYT-pnFv8Bj1SYX-HnIej0tpSiU0kav6tMUxHLBDSjZmz-G0T_kG9HJPQiszUbIV1y61yOeYLpUNRBewAW0nerF-qo0FuYEX_K-KYC4fsQdQCbtxEH8erlzGoIxQaFbWKOipeotUUToYQNHqY6bXaaZnUl-rlQ-CuNEW4flPvh1gsTnlikWhVF1DviE_OHxdzR-UMtiZNq3oHFJK2bXSLwH7C3zzIs3ReORfYzxjkwv0yb_FOupmZmS_HSh_AqqgtihQ18VuIZBM2s"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "requestId": 8756,
    "value":{"Signal.OBD.RPM":2345}
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "w3c-vss": {
    "Signal": "r"    (permission to read everything)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsIjoiciJ9fQ.BA-aixUTBo3iDp1r1vrhgAHHGAIdAcjz0oZ7eVtvmMWfrPEk3-k6r4A0_Qa0fSb2L4gZBryLgn6h4eLKvlh_gH8JYMOdf1-hL-pBtVfleJjS6CViWmP-WfGi_Q9h5WO2fCdWjmBYmZDduurZkwOfExbI2k4O40l1N94zjOEtIGQ4HqgvYGo7sIgGi1CrG-WAWHdmD29ZHbB88rIaMr-7-hslzznp0cI-hxffh0SCoGJexVsITIMey9VggcjQ2nFyAnIwI9gnt1RGPLbMkax16e9vMYT-27lUfVl2ecx5GTgLVM41x0qRtFf0Fz1Ev8LTwb7BCPVDTQhBuZ_U952fp8SBChFiYtx-oYWEtFFXmRawHyATLUg-CdgGD5t4KahqYDr_AFkX7UV1yVDKDmPpzdeNezJ4go2ojLJGcls4n_7usB093gZ7J5Y4i4ziF2t0_5zulXv6NvVhrrxR6OH-qLcnhRhuUXsGKc-DhtCR93RkFg8wurPLjTC0zGFR70n6SNJFnTNS8f9nCiOcWw7HwuZnHPPHtv9WOZpHQsK8AUP7d9MN5icf5pcDm_XUVdWy4hvGy5N7B28pn9K6wYIZ4z7NHJ9UwqstgYZgeOgOWpRl7g39jws6OEsbztj95KxUUIYvi1V0dmSWo2TYI3SIONDWUoLSarr2Nq0v_GR_tS8"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756,
    "value": 2345
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "w3c-vss": {
    "Signal.OBD.Speed": "w"    
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5TcGVlZCI6IncifX0.YjLl-AbEwyDK6QpUZbhyq8JVK1zXc4av5rZZePAA-4bygHiH4R_Xo_W0oVVnyqmtA2Z78LGD_qK_NDkUEo5VTFf7Y2kZd5xWONQg6B8G2rDiIwk29Hb5dCai8n4bCBNqv0k_fZIxJ6GDk7pTYkX4kwBHdIo7NAtyun9vByUWZs8Z8rf33CpsHKOvlhdGXbP3T4OBrkKpMryqW9pSxPV7xn17pKTHC7AZHO7pMkA-lNIZYIv-o2lRx8iJPfL8D_CkTbJtuBpWU316CJsld2M0T_uA8wnfSxpDVXWEFEmFBRWMprk83wIUL3ygfJaDix2kjIV24wR-Od7g07WPYWmYKM9n6K6KFSnkvGnows5A9DHMsDbvM58ee4FvDsMT5P6a8mFekrn78aae5ul6OPaaeBMOC0-tNUQ2PRAozi7tFJb4fSYUcsfZHHuedgFfkRRIT-8go3R1a2na9zsic9Y1deOXpfpx8-juFBp7anihEAR0V0Rf0QpMyOgL4BynP-Oj8txsy0zLMYMMc4GnRj7T5Ga0sIS6EkYAYASQBYwW66qRx-vJ1afxPvFARieLQWPVzMqK0UK9etTad2E7Mcf1z4akHxjcxiXQxFc3c4Pg_B3hK29jbk8U2rCLo1WhryiRsaEpboRM5WIICuygAc9dhg1_Ka1dnrim_qlnXnlquhw"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "set",
		"path": "Signal.OBD.Speed",
                "value" : 200,
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "requestId":8756
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "exp": 1609372800,
  "w3c-vss": {
    "Signal.OBD.Speed": "w"    
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5TcGVlZCI6IncifX0.YjLl-AbEwyDK6QpUZbhyq8JVK1zXc4av5rZZePAA-4bygHiH4R_Xo_W0oVVnyqmtA2Z78LGD_qK_NDkUEo5VTFf7Y2kZd5xWONQg6B8G2rDiIwk29Hb5dCai8n4bCBNqv0k_fZIxJ6GDk7pTYkX4kwBHdIo7NAtyun9vByUWZs8Z8rf33CpsHKOvlhdGXbP3T4OBrkKpMryqW9pSxPV7xn17pKTHC7AZHO7pMkA-lNIZYIv-o2lRx8iJPfL8D_CkTbJtuBpWU316CJsld2M0T_uA8wnfSxpDVXWEFEmFBRWMprk83wIUL3ygfJaDix2kjIV24wR-Od7g07WPYWmYKM9n6K6KFSnkvGnows5A9DHMsDbvM58ee4FvDsMT5P6a8mFekrn78aae5ul6OPaaeBMOC0-tNUQ2PRAozi7tFJb4fSYUcsfZHHuedgFfkRRIT-8go3R1a2na9zsic9Y1deOXpfpx8-juFBp7anihEAR0V0Rf0QpMyOgL4BynP-Oj8txsy0zLMYMMc4GnRj7T5Ga0sIS6EkYAYASQBYwW66qRx-vJ1afxPvFARieLQWPVzMqK0UK9etTad2E7Mcf1z4akHxjcxiXQxFc3c4Pg_B3hK29jbk8U2rCLo1WhryiRsaEpboRM5WIICuygAc9dhg1_Ka1dnrim_qlnXnlquhw"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "set",
		"path": "Signal.OBD.RPM",
                "value" : 200,
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "error":{"message":"Path(s) in set request do not have write access or is invalid","number":403,"reason":"Forbidden"},
    "requestId":8756
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

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
  "w3c-vss": {
    "Signal.OBD.*": "rw"    
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC4qIjoicncifX0.o5HF6H4u3boInC2d0NfR1k-dxiPOueblq_gvozpL24r0_z4l6gUYgLWBbI9fHsyjRu3ZXMG2enBzWwsCKi4KG1e5rBAfBh61oAncKdjvZExR0wOx6TF6DMcLCCIiRztV4q97DQgFU-k7FV4BsGtMwKGe1BhAT7_drqo7Vrxl2u-sDZuFeF3flyn-a_vo_NjfMEl7UcSr4aIp0o6uuGnBGYZJk0wYiV-B6s93b0eGWSQAPcvNhpCikbpBmzR1-dtLinQLt--0aQEWzVtjZBrWWAW-5FlhQ42qBYqxbhXGquag1ylhqD3J1nJgIIFZv3b1DBoQQSv7lXY-m2flDEvjaUVXevmDYhwoJvsabVFBdtLf3aURvttbO8XWhCDrA8-ApdnjZPvmoZv8BCwnWSrCECL4V7-NkKuarG_YlrZKswjzO5OFFQYEHaOMo-rSWWSRAIV7fBQP_ryYy6ZCaFuS3K6ZrvotywIqUhSleUxLAit_cwid5XxEwoYbIdykXl-kO8hrr9naJYE6ISvPWPx7N2A71CQJ-djsOlyOTZ6Emuc7lBnGgRybFwjE7YSz8ye-SROEnUq5fhKnIWwce66uGV93NjqWbFW1usH9J5DvG-UcJj41IDXeDUfzygIqHoRGgRNzIyww82JC5JBEzx1svTLigXfYVq2Z1m-sg-bSkQM"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "set",
		"path": "Signal.OBD.Speed",
                "value" : 345,
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "requestId":8756
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);

   // verify with a get request

    string get_request(R"({
		"action": "get",
		"path": "Signal.OBD.Speed",
		"requestId": "8756"
	})");
   
   string get_response = commandProc->processQuery(get_request,channel);

  json get_expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.Speed",
    "requestId": 8756,
    "value": 345
    })");

   json get_response_json = json::parse(get_response);
   

   BOOST_TEST(get_response_json.has_key("timestamp") == true);

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
  "exp": 1609372800,
  "w3c-vss": {
    "Signal.OBD": "w"    
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRCI6IncifX0.CPJuY5XVsz94KemZ7yNtO8j2kUB4lLqrZ6fXwEKy2Cs3bKfxm4MVWd7qwn0uO8I3AGV1C8SUACAmov2Vly7al5lYaNOzhACn7XWiNqsoA-Xh0MOxDW3JetcLNo3qFibQTQepiIf37MrodZYDQwDCLLxk6OkErjg-AIJpSSrythdGBji-Ay0YzrCQRRtI0pwFogPqYwUFickAPOxdMlluaQMspmlA6um7AaWk8HaQfixniyAwMIcZJm-ht6dCIvdTnWohbUwITXVa6v5CGdl2KOAYUF2gYkCmKssEN1h4vscK8F3hD26s3zhXzlmBXVO2c8YoQFK8ZQj9nBS4IG1yWEMB4n0RdvMQnv9wV_tGroqeRNlbniolBiHpWXy_QC52y7Ph-Gcu6FUeT_fT0it6V6latZOAqlFfDPmkEGEfFNXC64fkTd83DERCl4LagncsX5Gx3kh5oGETWSg91WkeibroQDkw3jhqHZI9bFVtaC5yOrSXN2BSGAQ15tSHEvQkaHAXmTPrUwyyzDzasZz5Rl--oNZbLt6nXneqOAI6P7z0E5NZtx7gXpMXlwmVoZLuRUB1fYViI8rOJ8DqumzM-6b114LQyPdgEZONRPnit1Npl31zPK_hC9RWGQ5p8LZQ7mse6HvDjynvxdmOTs3odSRcItjCo5a3nYRMZQbebeQ"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "set",
		"path": "Signal.OBD.Speed",
                "value" : 345,
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "requestId":8756
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);

   // verify with a get request

    string get_request(R"({
		"action": "get",
		"path": "Signal.OBD.Speed",
		"requestId": "8756"
	})");
   
   string get_response = commandProc->processQuery(get_request,channel);
  // because only write access in the token.
  json get_expected = json::parse(R"({
    "action": "get",
    "error":{"message":"No read access to  Signal.OBD.Speed","number":403,"reason":"Forbidden"},
    "requestId": 8756
    })");

   json get_response_json = json::parse(get_response);
   

   BOOST_TEST(get_response_json.has_key("timestamp") == true);

   // remove timestamp to match
   get_response_json.erase("timestamp");
   BOOST_TEST(get_response_json == get_expected);

   // Now try to read with a valid token.

/*
   GET_AUTH_TOKEN looks like this
   
   {
  "sub": "Example JWT",
  "iss": "Eclipse kuksa",
  "admin": true,
  "iat": 1516239022,
  "exp": 1609372800,
  "w3c-vss": {
    "Signal.OBD.Speed": "r"
  }
}
*/
    string GET_AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5TcGVlZCI6InIifX0.NHLLzy9MC1bWSCbwUDmy7CZoTn0RtynQjAg71q722iqwtRm0wryF21FqENh-KP00wLeDSa2daX34LW_AGSoKpSVfNoQkauILcm0srJdNeAXt8qRPG8Lb1QhK7L5kWE6aLMhAuIUCdLd6qMW6cCZTPgHxav2crfcVXybbbzPsSzI7E0LJW34MK7ysLjsX3nmDkUMpMcviZ07v20HiW6PPMo3Fjz2p9sagau3I8INd7P0drRLtgzT4r81xi9qH-Eeo6I47O_u9EowQZHRNK-kwP-s7QbYMezswvq-c0cuchU1Y91OUNGyomyvM4vqIfeLfDwnzkX40qscE3-uhXxd6e1Yf5WY2wF5S0OpymmHiKnxCBvXMO_im6p6Pi1HY2I-lHoQNB-n_YiV9scyNY76HL8mAj_vJlwJWvOxN8C1K4c1vXC2oyZYI5T7MDPQublrJRGhqwZj7Z-DMPvmSnzZgBunUiZb_Qyura2kawgGIApAh4QJdowC8Pa6j5n6HHU1SIEq8cPvKodfTftTzj3qc-TuYU3EuMxTrdIKg5d74FMEDmmOtOguvTZz7imu8Sd4_8rzwlsDFjgxuhvYe3Nk5saQ44zcO6mnR34Qg94xgstTFLZ4OXU_p86bhLNdZQtFWahAF6w5M_FhlmI971w2VvpNM4dAzBTkeFCRl72qe83E"; 

   
   wschannel get_channel;
   get_channel.setConnID(1234);
   string get_authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json get_authReqJson = json::parse(get_authReq);
   get_authReqJson["tokens"] = GET_AUTH_TOKEN;
   commandProc->processQuery(get_authReqJson.as<string>(),get_channel);
   string get_request_2(R"({
		"action": "get",
		"path": "Signal.OBD.Speed",
		"requestId": "8756"
	})");
   
   string get_response_2 = commandProc->processQuery(get_request_2, get_channel);

   json get_expected_2 = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.Speed",
    "requestId": 8756,
    "value": 345
    })");

   json get_response_json_2 = json::parse(get_response_2);
   

   BOOST_TEST(get_response_json_2.has_key("timestamp") == true);

   // remove timestamp to match
   get_response_json_2.erase("timestamp");
   BOOST_TEST(get_response_json_2 == get_expected_2);   
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
  "w3c-vss": { 
    "Signal.OBD.RPM": "wr",                     ("wr" or "rw" both work!)
    "Signal.OBD.Speed": "w"         
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5SUE0iOiJ3ciIsIlNpZ25hbC5PQkQuU3BlZWQiOiJ3In19.LCHg9WPXzbvLEVFKbBgDxHnoef8Dat0p_9OGzrfHxYI-kJchB3YTjjgjOh4q67dlVuypSp4MEV79WYmh5pPjlh8-e22_kZkzfyVYyRH4G2Z1Mi3W0Wd2cYSERw6iiHWN3phW_wzs5Dv_DU3CRJ5R7Cij6JuknMf90cldbhttiEAm4nIdBH3o2RjnJpE5ECJp8VaQwxkhQyeQfcqajX4W4hsBfzLCmV5aRgXlkaHYv2-vkEasn71znrjJ8hbUDQmt1hZgHV_BvvWjLvi_CzbKKRgXfq4ItqD4-_DT_C_dWIilp9w-gtlNbM2Ru9XOmctadzalZW4sVYYxc2SLI_SfHyz7eFJ2AIHG8GRHHw8SfWKUsk1tQfLF5IgGKaHTEobMGXnT7ZtmxeLjJVmsbIqEUPLKbzZM-yLbpnIgWJuxTYA4-rspw6S4Df8WN6yx7nYXXIDf-N0f6FUUGL7camX3gzabhEFZvuK45uZtWRPdxGCK_ioKz89xZmeHaMyaG9jFxJCUhA7jvdyoswn9vA71fqURzZWAu5Z8VKiKEnktlIvpRpZCGdD6kWDXjjD9qsVMuDOUchWONhErlhWlWVSW2MGN4U_Lj22KuJst7LkVdOYhZA7aMrM9hXsybTezvBenHaKu2WAWMCdpS3CcshnsbjM3cw6yEZyNyb4sdbmCZHU"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "set",
		"path": "Signal.*",
                "value" : [{"OBD.Speed":35}, {"OBD.RPM":50}],
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "requestId":8756
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);

   // verify with a get request the the previous set has not worked.

    string get_request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string get_response = commandProc->processQuery(get_request,channel);

  json get_expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756,
    "value": 50
    })");

   json get_response_json = json::parse(get_response);
   

   BOOST_TEST(get_response_json.has_key("timestamp") == true);

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
  "exp": 1609372800,
  "w3c-vss": {
    "Signal.OBD.RPM": "wr"    ("wr" or "rw" both work!)
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5SUE0iOiJ3ciJ9fQ.EGCGJLaZjjAq_H7rlOyXo6tFCShlGAyTuu71zLpMZbiahoVIy_01VTFCYSBbDbtqBRGA70GdLIDwdBWYrpEvaMDA8OPr6u_rBtwStMk5APJUXMrzk9Ay2e_jxsFnrE3w1pJ4BoMOiUoeDQAu66eyQJNctH-VdvsT4UFiezIc4o3RdzHHneB91bOhEN-EY9LyRPzQHCacoyyLcSBtaGxcChQgzEf5ZcSc4-qirs1-Pdh0PNOHr-ATQUoanvfXfdPl_hFtkVNnG-HMcUhpp8qvez3vkrOlIXQJfw-DkiSU9yLU5LRcUhleaEuZUeYdgFmmw9UyvLLrBPA2hdPeCo56vQH38Nd3le3OoHWRgtfjch3HRdEjzP19M3TonOTE0UwZAQDo3HAhz8lvk3iBqO8tYIqGbPOR4zforPEdReOCM2gV53dmbSE2S8zo5dnPbbjCvZlhEkD1JrPnqOQXeBbiVDDuB5QOj1fZLQNL2BPLKXRLud78OLet_44WqSIpMtVSuKVAZadguMDbvtfzeHXdhXF_UhoSezdvYXoToBLWXuoU90X590CNypxEfT9Gri1RIsD6iEhaYCLfedkp8J_vDtPG_d51xDbsqkHrX5y225agM475avI4OHKEsH3O9_Mcz1wBLGf7iVMiwtOp_vQ3otHibb9j5lucQunVtKJFIKQ"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);
   string request(R"({
		"action": "set",
		"path": "Signal.OBD.*",
                "value" : [{"Speed":35}, {"RPM":444}],
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);

  json expected = json::parse(R"({
    "action":"set",
    "error":{"message":"Path(s) in set request do not have write access or is invalid","number":403,"reason":"Forbidden"},
    "requestId":8756
    })");

   json response_json = json::parse(response);
   

   BOOST_TEST(response_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);

   // verify with a get request the the previous set has not worked.

    string get_request(R"({
		"action": "get",
		"path": "Signal.OBD.RPM",
		"requestId": "8756"
	})");
   
   string get_response = commandProc->processQuery(get_request,channel);

  json get_expected = json::parse(R"({
    "action": "get",
    "path": "Signal.OBD.RPM",
    "requestId": 8756,
    "value": 50
    })");

   json get_response_json = json::parse(get_response);
   

   BOOST_TEST(get_response_json.has_key("timestamp") == true);

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
  "w3c-vss": { 
    "Signal.OBD.RPM": "wr",                     ("wr" or "rw" both work!)
    "Signal.OBD.Speed": "w"         
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5SUE0iOiJ3ciIsIlNpZ25hbC5PQkQuU3BlZWQiOiJ3In19.LCHg9WPXzbvLEVFKbBgDxHnoef8Dat0p_9OGzrfHxYI-kJchB3YTjjgjOh4q67dlVuypSp4MEV79WYmh5pPjlh8-e22_kZkzfyVYyRH4G2Z1Mi3W0Wd2cYSERw6iiHWN3phW_wzs5Dv_DU3CRJ5R7Cij6JuknMf90cldbhttiEAm4nIdBH3o2RjnJpE5ECJp8VaQwxkhQyeQfcqajX4W4hsBfzLCmV5aRgXlkaHYv2-vkEasn71znrjJ8hbUDQmt1hZgHV_BvvWjLvi_CzbKKRgXfq4ItqD4-_DT_C_dWIilp9w-gtlNbM2Ru9XOmctadzalZW4sVYYxc2SLI_SfHyz7eFJ2AIHG8GRHHw8SfWKUsk1tQfLF5IgGKaHTEobMGXnT7ZtmxeLjJVmsbIqEUPLKbzZM-yLbpnIgWJuxTYA4-rspw6S4Df8WN6yx7nYXXIDf-N0f6FUUGL7camX3gzabhEFZvuK45uZtWRPdxGCK_ioKz89xZmeHaMyaG9jFxJCUhA7jvdyoswn9vA71fqURzZWAu5Z8VKiKEnktlIvpRpZCGdD6kWDXjjD9qsVMuDOUchWONhErlhWlWVSW2MGN4U_Lj22KuJst7LkVdOYhZA7aMrM9hXsybTezvBenHaKu2WAWMCdpS3CcshnsbjM3cw6yEZyNyb4sdbmCZHU"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);


   string request(R"({
		"action": "subscribe",
		"path": "Signal.OBD.RPM",
		"requestId": "8778"
	})");
   
   string response = commandProc->processQuery(request,channel);
   json response_json = json::parse(response);

   json expected = json::parse(R"({
     "action":"subscribe",
     "requestId":8778  
    })");

   BOOST_TEST(response_json.has_key("timestamp") == true);
   BOOST_TEST(response_json.has_key("subscriptionId") == true);

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

   string unsub_response = commandProc->processQuery(temp_unsubreq.as<string>(),channel);
   json unsub_response_json = json::parse(unsub_response);
   
   // assert timestamp and subid.
   BOOST_TEST(unsub_response_json.has_key("timestamp") == true);
   BOOST_TEST(unsub_response_json.has_key("subscriptionId") == true);
 
   // compare the subit passed and returned.
   BOOST_TEST(unsub_response_json["subscriptionId"].as<int>() == subID);

   // remove timestamp and subid to assert other part of the response.because these are variables.
   unsub_response_json.erase("subscriptionId");
   unsub_response_json.erase("timestamp");
   
   json expected_unsub = json::parse(R"({
     "action":"unsubscribe",
     "requestId":8779  
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
  "w3c-vss": {
    "Signal.OBD.*": "rw"    
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC4qIjoicncifX0.o5HF6H4u3boInC2d0NfR1k-dxiPOueblq_gvozpL24r0_z4l6gUYgLWBbI9fHsyjRu3ZXMG2enBzWwsCKi4KG1e5rBAfBh61oAncKdjvZExR0wOx6TF6DMcLCCIiRztV4q97DQgFU-k7FV4BsGtMwKGe1BhAT7_drqo7Vrxl2u-sDZuFeF3flyn-a_vo_NjfMEl7UcSr4aIp0o6uuGnBGYZJk0wYiV-B6s93b0eGWSQAPcvNhpCikbpBmzR1-dtLinQLt--0aQEWzVtjZBrWWAW-5FlhQ42qBYqxbhXGquag1ylhqD3J1nJgIIFZv3b1DBoQQSv7lXY-m2flDEvjaUVXevmDYhwoJvsabVFBdtLf3aURvttbO8XWhCDrA8-ApdnjZPvmoZv8BCwnWSrCECL4V7-NkKuarG_YlrZKswjzO5OFFQYEHaOMo-rSWWSRAIV7fBQP_ryYy6ZCaFuS3K6ZrvotywIqUhSleUxLAit_cwid5XxEwoYbIdykXl-kO8hrr9naJYE6ISvPWPx7N2A71CQJ-djsOlyOTZ6Emuc7lBnGgRybFwjE7YSz8ye-SROEnUq5fhKnIWwce66uGV93NjqWbFW1usH9J5DvG-UcJj41IDXeDUfzygIqHoRGgRNzIyww82JC5JBEzx1svTLigXfYVq2Z1m-sg-bSkQM";
   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);


   string request(R"({
		"action": "subscribe",
		"path": "Signal.OBD.Speed",
		"requestId": "8778"
	})");
   
   string response = commandProc->processQuery(request,channel);
   json response_json = json::parse(response);

   json expected = json::parse(R"({
     "action":"subscribe",
     "requestId":8778  
    })");

   BOOST_TEST(response_json.has_key("timestamp") == true);
   BOOST_TEST(response_json.has_key("subscriptionId") == true);

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

   string unsub_response = commandProc->processQuery(temp_unsubreq.as<string>(),channel);
   json unsub_response_json = json::parse(unsub_response);
   
   // assert timestamp and subid.
   BOOST_TEST(unsub_response_json.has_key("timestamp") == true);
   BOOST_TEST(unsub_response_json.has_key("subscriptionId") == true);
 
   // compare the subit passed and returned.
   BOOST_TEST(unsub_response_json["subscriptionId"].as<int>() == subID);

   // remove timestamp and subid to assert other part of the response.because these are variables.
   unsub_response_json.erase("subscriptionId");
   unsub_response_json.erase("timestamp");
   
   json expected_unsub = json::parse(R"({
     "action":"unsubscribe",
     "requestId":8779  
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
  "exp": 1609372800,
  "w3c-vss": { 
    "Signal.OBD.RPM": "wr",                     ("wr" or "rw" both work!)
    "Signal.OBD.Speed": "w"         
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5SUE0iOiJ3ciIsIlNpZ25hbC5PQkQuU3BlZWQiOiJ3In19.LCHg9WPXzbvLEVFKbBgDxHnoef8Dat0p_9OGzrfHxYI-kJchB3YTjjgjOh4q67dlVuypSp4MEV79WYmh5pPjlh8-e22_kZkzfyVYyRH4G2Z1Mi3W0Wd2cYSERw6iiHWN3phW_wzs5Dv_DU3CRJ5R7Cij6JuknMf90cldbhttiEAm4nIdBH3o2RjnJpE5ECJp8VaQwxkhQyeQfcqajX4W4hsBfzLCmV5aRgXlkaHYv2-vkEasn71znrjJ8hbUDQmt1hZgHV_BvvWjLvi_CzbKKRgXfq4ItqD4-_DT_C_dWIilp9w-gtlNbM2Ru9XOmctadzalZW4sVYYxc2SLI_SfHyz7eFJ2AIHG8GRHHw8SfWKUsk1tQfLF5IgGKaHTEobMGXnT7ZtmxeLjJVmsbIqEUPLKbzZM-yLbpnIgWJuxTYA4-rspw6S4Df8WN6yx7nYXXIDf-N0f6FUUGL7camX3gzabhEFZvuK45uZtWRPdxGCK_ioKz89xZmeHaMyaG9jFxJCUhA7jvdyoswn9vA71fqURzZWAu5Z8VKiKEnktlIvpRpZCGdD6kWDXjjD9qsVMuDOUchWONhErlhWlWVSW2MGN4U_Lj22KuJst7LkVdOYhZA7aMrM9hXsybTezvBenHaKu2WAWMCdpS3CcshnsbjM3cw6yEZyNyb4sdbmCZHU"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);


   string request(R"({
		"action": "subscribe",
		"path": "Signal.OBD.Speed",
		"requestId": "8778"
	})");
   
   string response = commandProc->processQuery(request,channel);
   json response_json = json::parse(response);

   json expected = json::parse(R"({
                   "action":"subscribe",
                   "error":{"message":"no permission to subscribe to path","number":403,"reason":"Forbidden"},
                   "requestId":8778
                   })");

   BOOST_TEST(response_json.has_key("timestamp") == true);
  
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
  "exp": 1609372800,
  "w3c-vss": { 
    "Signal.OBD.RPM": "wr",                     ("wr" or "rw" both work!)
    "Signal.OBD.Speed": "w"         
  }
}
*/
   string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDAsInczYy12c3MiOnsiU2lnbmFsLk9CRC5SUE0iOiJ3ciIsIlNpZ25hbC5PQkQuU3BlZWQiOiJ3In19.LCHg9WPXzbvLEVFKbBgDxHnoef8Dat0p_9OGzrfHxYI-kJchB3YTjjgjOh4q67dlVuypSp4MEV79WYmh5pPjlh8-e22_kZkzfyVYyRH4G2Z1Mi3W0Wd2cYSERw6iiHWN3phW_wzs5Dv_DU3CRJ5R7Cij6JuknMf90cldbhttiEAm4nIdBH3o2RjnJpE5ECJp8VaQwxkhQyeQfcqajX4W4hsBfzLCmV5aRgXlkaHYv2-vkEasn71znrjJ8hbUDQmt1hZgHV_BvvWjLvi_CzbKKRgXfq4ItqD4-_DT_C_dWIilp9w-gtlNbM2Ru9XOmctadzalZW4sVYYxc2SLI_SfHyz7eFJ2AIHG8GRHHw8SfWKUsk1tQfLF5IgGKaHTEobMGXnT7ZtmxeLjJVmsbIqEUPLKbzZM-yLbpnIgWJuxTYA4-rspw6S4Df8WN6yx7nYXXIDf-N0f6FUUGL7camX3gzabhEFZvuK45uZtWRPdxGCK_ioKz89xZmeHaMyaG9jFxJCUhA7jvdyoswn9vA71fqURzZWAu5Z8VKiKEnktlIvpRpZCGdD6kWDXjjD9qsVMuDOUchWONhErlhWlWVSW2MGN4U_Lj22KuJst7LkVdOYhZA7aMrM9hXsybTezvBenHaKu2WAWMCdpS3CcshnsbjM3cw6yEZyNyb4sdbmCZHU"; 

   
   wschannel channel;
   channel.setConnID(1234);
   string authReq(R"({
		"action": "authorize",
		"requestId": "87568"
	})");
   json authReqJson = json::parse(authReq);
   authReqJson["tokens"] = AUTH_TOKEN;
   commandProc->processQuery(authReqJson.as<string>(),channel);


   string request(R"({
		"action": "subscribe",
		"path": "Signal.OBD.RPM1",
		"requestId": "8778"
	})");
   
   string response = commandProc->processQuery(request,channel);
   json response_json = json::parse(response);

   json expected = json::parse(R"({
                   "action":"subscribe",
                   "error":{"message":"I can not find Signal.OBD.RPM1 in my db","number":404,"reason":"Path not found"},
                   "requestId":8778
                   })");

   BOOST_TEST(response_json.has_key("timestamp") == true);
  
   // remove timestamp to match
   response_json.erase("timestamp");
   BOOST_TEST(response_json == expected);
}



