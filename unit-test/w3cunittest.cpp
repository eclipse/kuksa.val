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
#include <limits> 
#include "w3cunittest.hpp"
#include "exception.hpp"
#define BOOST_TEST_MODULE w3c-unit-test
#include <boost/test/included/unit_test.hpp>

#define AUTH_JWT "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJFeGFtcGxlIEpXVCIsImlzcyI6IkVjbGlwc2Uga3Vrc2EiLCJhZG1pbiI6dHJ1ZSwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjE2MDkzNzI4MDB9.lxYf-N0C66NgJiinrY_pgVpZOxgH0ppnbMsdKfNiQjbMqYTY_nEpXufmjquvBaxiYsRb_3ScXA__gmviWj_PQoU3apuMVxJjAY6xrwaXFw7g-daUi21yqLSfLptH2vqQzhA_Y1LU6u7SjeiEcGRZrgq_bHnjMM61EVDSnKGCClo"


namespace bt = boost::unit_test;
#define PORT 8090

wsserver* webSocket;
subscriptionhandler* subhandler;
authenticator* authhandler;
accesschecker* accesshandler;
vssdatabase* database;
vsscommandprocessor* commandProc;

class w3cunittest unittestObj(false);

w3cunittest::w3cunittest(bool secure) {
   webSocket = new wsserver(PORT, secure);
   authhandler = new authenticator("","");
   subhandler = new subscriptionhandler(webSocket, authhandler); 
   database = new vssdatabase(subhandler);
   commandProc = new vsscommandprocessor(database, authhandler , subhandler);
   database->initJsonTree();
}

w3cunittest::~w3cunittest() {
   delete webSocket;
   delete authhandler;
   delete subhandler; 
   delete database;
}

list<string> w3cunittest::test_wrap_getPathForGet(string path , bool &isBranch) {
    return database->getPathForGet(path , isBranch);
}

json w3cunittest::test_wrap_getPathForSet(string path,  json value) {
    return database->getPathForSet(path , value);
} 

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

    BOOST_TEST(paths[0]["path"].as<string>() == "$.Signal.children.OBD.children.RPM");
    

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

    BOOST_TEST(paths[0]["path"].as<string>() == "$.Signal.children.OBD.children.RPM");
    BOOST_TEST(paths[1]["path"].as<string>() == "$.Signal.children.OBD.children.Speed");
    
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

    BOOST_TEST(paths[0]["path"].as<string>() == "$.Signal.children.OBD.children.RPM");
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

    BOOST_TEST(paths[0]["path"].as<string>() == "$.Signal.children.OBD.children.O2SensorsAlt.children.Bank2.children.Sensor1.children.Present");
    BOOST_TEST(paths[1]["path"].as<string>() == "$.Signal.children.OBD.children.O2SensorsAlt.children.Bank2.children.Sensor2.children.Present");
    
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

    BOOST_TEST(paths[0]["path"].as<string>() == "$.Signal.children.OBD.children.O2SensorsAlt.children.Bank2.children.Sensor1.children.Present");
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

//---------------------  Uint8 SET/GET TEST ------------------------------------
    json test_value_Uint8_boundary_low;
    test_value_Uint8_boundary_low = numeric_limits<uint8_t>::min();

    database->setSignal(test_path_Uint8, test_value_Uint8_boundary_low);
    result = database->getSignal(test_path_Uint8);
    
    BOOST_TEST(result["value"] == numeric_limits<uint8_t>::min());
    
    json test_value_Uint8_boundary_high;
    test_value_Uint8_boundary_high = numeric_limits<uint8_t>::max();

    database->setSignal(test_path_Uint8, test_value_Uint8_boundary_high);
    result = database->getSignal(test_path_Uint8);

    BOOST_TEST(result["value"] == numeric_limits<uint8_t>::max());

    json test_value_Uint8_boundary_middle;
    test_value_Uint8_boundary_middle = numeric_limits<uint8_t>::max() / 2;

    database->setSignal(test_path_Uint8, test_value_Uint8_boundary_middle);
    result = database->getSignal(test_path_Uint8);

    BOOST_TEST(result["value"] == numeric_limits<uint8_t>::max() / 2);

    // Test out of bound
    bool isExceptionThrown = false;
    json test_value_Uint8_boundary_low_outbound;
    test_value_Uint8_boundary_low_outbound = -1;
    try {
       database->setSignal(test_path_Uint8, test_value_Uint8_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_Uint8_boundary_high_outbound;
    test_value_Uint8_boundary_high_outbound = numeric_limits<uint8_t>::max() + 1;
    try {
       database->setSignal(test_path_Uint8, test_value_Uint8_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);
   

//---------------------  Uint16 SET/GET TEST ------------------------------------

    json test_value_Uint16_boundary_low;
    test_value_Uint16_boundary_low = numeric_limits<uint16_t>::min();

    database->setSignal(test_path_Uint16, test_value_Uint16_boundary_low);
    result = database->getSignal(test_path_Uint16);
    
    BOOST_TEST(result["value"] == numeric_limits<uint16_t>::min());
    
    json test_value_Uint16_boundary_high;
    test_value_Uint16_boundary_high = numeric_limits<uint16_t>::max();

    database->setSignal(test_path_Uint16, test_value_Uint16_boundary_high);
    result = database->getSignal(test_path_Uint16);

    BOOST_TEST(result["value"] == numeric_limits<uint16_t>::max());

    json test_value_Uint16_boundary_middle;
    test_value_Uint16_boundary_middle = numeric_limits<uint16_t>::max() / 2;

    database->setSignal(test_path_Uint16, test_value_Uint16_boundary_middle);
    result = database->getSignal(test_path_Uint16);

    BOOST_TEST(result["value"] == numeric_limits<uint16_t>::max() / 2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_Uint16_boundary_low_outbound;
    test_value_Uint16_boundary_low_outbound = -1;
    try {
       database->setSignal(test_path_Uint16, test_value_Uint16_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_Uint16_boundary_high_outbound;
    test_value_Uint16_boundary_high_outbound = numeric_limits<uint16_t>::max() + 1;
    try {
       database->setSignal(test_path_Uint16, test_value_Uint16_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);


//---------------------  Uint32 SET/GET TEST ------------------------------------

    json test_value_Uint32_boundary_low;
    test_value_Uint32_boundary_low = numeric_limits<uint32_t>::min();

    database->setSignal(test_path_Uint32, test_value_Uint32_boundary_low);
    result = database->getSignal(test_path_Uint32);
    
    BOOST_TEST(result["value"] == numeric_limits<uint32_t>::min());
    
    json test_value_Uint32_boundary_high;
    test_value_Uint32_boundary_high = numeric_limits<uint32_t>::max();

    database->setSignal(test_path_Uint32, test_value_Uint32_boundary_high);
    result = database->getSignal(test_path_Uint32);

    BOOST_TEST(result["value"] == numeric_limits<uint32_t>::max());

    json test_value_Uint32_boundary_middle;
    test_value_Uint32_boundary_middle = numeric_limits<uint32_t>::max() / 2;

    database->setSignal(test_path_Uint32, test_value_Uint32_boundary_middle);
    result = database->getSignal(test_path_Uint32);

    BOOST_TEST(result["value"] == numeric_limits<uint32_t>::max() / 2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_Uint32_boundary_low_outbound;
    test_value_Uint32_boundary_low_outbound = -1;
    try {
       database->setSignal(test_path_Uint32, test_value_Uint32_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_Uint32_boundary_high_outbound;
    uint64_t maxU32_value = numeric_limits<uint32_t>::max();
    test_value_Uint32_boundary_high_outbound = maxU32_value + 1;
    try {
       database->setSignal(test_path_Uint32, test_value_Uint32_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);

//---------------------  int8 SET/GET TEST ------------------------------------

    json test_value_int8_boundary_low;
    test_value_int8_boundary_low = numeric_limits<int8_t>::min();

    database->setSignal(test_path_int8, test_value_int8_boundary_low);
    result = database->getSignal(test_path_int8);
    
    BOOST_TEST(result["value"] == numeric_limits<int8_t>::min());
    
    json test_value_int8_boundary_high;
    test_value_int8_boundary_high = numeric_limits<int8_t>::max();

    database->setSignal(test_path_int8, test_value_int8_boundary_high);
    result = database->getSignal(test_path_int8);

    BOOST_TEST(result["value"] == numeric_limits<int8_t>::max());

    json test_value_int8_boundary_middle;
    test_value_int8_boundary_middle = numeric_limits<int8_t>::max() / 2;

    database->setSignal(test_path_int8, test_value_int8_boundary_middle);
    result = database->getSignal(test_path_int8);

    BOOST_TEST(result["value"] == numeric_limits<int8_t>::max() / 2);

    json test_value_int8_boundary_middle_neg;
    test_value_int8_boundary_middle_neg = numeric_limits<int8_t>::min() / 2;

    database->setSignal(test_path_int8, test_value_int8_boundary_middle_neg);
    result = database->getSignal(test_path_int8);

    BOOST_TEST(result["value"] == numeric_limits<int8_t>::min() / 2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_int8_boundary_low_outbound;
    test_value_int8_boundary_low_outbound = numeric_limits<int8_t>::min() - 1;
    try {
       database->setSignal(test_path_int8, test_value_int8_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_int8_boundary_high_outbound;
    test_value_int8_boundary_high_outbound = numeric_limits<int8_t>::max() + 1;
    try {
       database->setSignal(test_path_int8, test_value_int8_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);

//---------------------  int16 SET/GET TEST ------------------------------------

    json test_value_int16_boundary_low;
    test_value_int16_boundary_low = numeric_limits<int16_t>::min();

    database->setSignal(test_path_int16, test_value_int16_boundary_low);
    result = database->getSignal(test_path_int16);
    
    BOOST_TEST(result["value"] == numeric_limits<int16_t>::min());
    
    json test_value_int16_boundary_high;
    test_value_int16_boundary_high = numeric_limits<int16_t>::max();

    database->setSignal(test_path_int16, test_value_int16_boundary_high);
    result = database->getSignal(test_path_int16);

    BOOST_TEST(result["value"] == numeric_limits<int16_t>::max());

    json test_value_int16_boundary_middle;
    test_value_int16_boundary_middle = numeric_limits<int16_t>::max()/2;

    database->setSignal(test_path_int16, test_value_int16_boundary_middle);
    result = database->getSignal(test_path_int16);

    BOOST_TEST(result["value"] == numeric_limits<int16_t>::max()/2);

    json test_value_int16_boundary_middle_neg;
    test_value_int16_boundary_middle_neg = numeric_limits<int16_t>::min()/2;

    database->setSignal(test_path_int16, test_value_int16_boundary_middle_neg);
    result = database->getSignal(test_path_int16);

    BOOST_TEST(result["value"] == numeric_limits<int16_t>::min()/2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_int16_boundary_low_outbound;
    test_value_int16_boundary_low_outbound = numeric_limits<int16_t>::min() - 1;
    try {
       database->setSignal(test_path_int16, test_value_int16_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_int16_boundary_high_outbound;
    test_value_int16_boundary_high_outbound = numeric_limits<int16_t>::max() + 1;
    try {
       database->setSignal(test_path_int16, test_value_int16_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);


//---------------------  int32 SET/GET TEST ------------------------------------

    json test_value_int32_boundary_low;
    test_value_int32_boundary_low = numeric_limits<int32_t>::min();

    database->setSignal(test_path_int32, test_value_int32_boundary_low);
    result = database->getSignal(test_path_int32);
    
    BOOST_TEST(result["value"] == numeric_limits<int32_t>::min());
    
    json test_value_int32_boundary_high;
    test_value_int32_boundary_high = numeric_limits<int32_t>::max() ;

    database->setSignal(test_path_int32, test_value_int32_boundary_high);
    result = database->getSignal(test_path_int32);

    BOOST_TEST(result["value"] == numeric_limits<int32_t>::max());

    json test_value_int32_boundary_middle;
    test_value_int32_boundary_middle = numeric_limits<int32_t>::max() / 2;

    database->setSignal(test_path_int32, test_value_int32_boundary_middle);
    result = database->getSignal(test_path_int32);

    BOOST_TEST(result["value"] == numeric_limits<int32_t>::max() / 2);

    json test_value_int32_boundary_middle_neg;
    test_value_int32_boundary_middle_neg = numeric_limits<int32_t>::min() / 2;

    database->setSignal(test_path_int32, test_value_int32_boundary_middle_neg);
    result = database->getSignal(test_path_int32);

    BOOST_TEST(result["value"] == numeric_limits<int32_t>::min() / 2);
   
    // Test out of bound
    isExceptionThrown = false;
    json test_value_int32_boundary_low_outbound;
    int64_t minInt32_value = numeric_limits<int32_t>::min();
    test_value_int32_boundary_low_outbound = minInt32_value - 1;
    try {
       database->setSignal(test_path_int32, test_value_int32_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_int32_boundary_high_outbound;
    int64_t maxInt32_value = numeric_limits<int32_t>::max();
    test_value_int32_boundary_high_outbound = maxInt32_value + 1;
    try {
       database->setSignal(test_path_int32, test_value_int32_boundary_high_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);

//---------------------  Float SET/GET TEST ------------------------------------
    json test_value_Float_boundary_low;
    test_value_Float_boundary_low = std::numeric_limits<float>::min();

    database->setSignal(test_path_Float, test_value_Float_boundary_low);
    result = database->getSignal(test_path_Float);

    
    BOOST_TEST(result["value"] == std::numeric_limits<float>::min());
    
    json test_value_Float_boundary_high;
    test_value_Float_boundary_high = std::numeric_limits<float>::max();

    database->setSignal(test_path_Float, test_value_Float_boundary_high);
    result = database->getSignal(test_path_Float);

    BOOST_TEST(result["value"] == std::numeric_limits<float>::max());


    json test_value_Float_boundary_low_neg;
    test_value_Float_boundary_low_neg = std::numeric_limits<float>::min() * -1;

    database->setSignal(test_path_Float, test_value_Float_boundary_low_neg);
    result = database->getSignal(test_path_Float);

    
    BOOST_TEST(result["value"] == (std::numeric_limits<float>::min() * -1));
    
    json test_value_Float_boundary_high_neg;
    test_value_Float_boundary_high_neg = std::numeric_limits<float>::max() * -1;

    database->setSignal(test_path_Float, test_value_Float_boundary_high_neg);
    result = database->getSignal(test_path_Float);

    BOOST_TEST(result["value"] == (std::numeric_limits<float>::max() * -1));


    json test_value_Float_boundary_middle;
    test_value_Float_boundary_middle = std::numeric_limits<float>::max() / 2;

    database->setSignal(test_path_Float, test_value_Float_boundary_middle);
    result = database->getSignal(test_path_Float);

    
    BOOST_TEST(result["value"] == std::numeric_limits<float>::max() / 2);

    json test_value_Float_boundary_middle_neg;
    test_value_Float_boundary_middle_neg = std::numeric_limits<float>::min() * 2;

    database->setSignal(test_path_Float, test_value_Float_boundary_middle_neg);
    result = database->getSignal(test_path_Float);

    BOOST_TEST(result["value"] == std::numeric_limits<float>::min() * 2);

    // Test out of bound
    isExceptionThrown = false;
    json test_value_Float_boundary_low_outbound;
    double minFloat_value = numeric_limits<float>::min();
    test_value_Float_boundary_low_outbound = minFloat_value / 2;
    
    try {
       database->setSignal(test_path_Float, test_value_Float_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_Float_boundary_high_outbound;
    double maxFloat_value = numeric_limits<float>::max();
    test_value_Float_boundary_high_outbound = maxFloat_value * 2;
    try {
       database->setSignal(test_path_Float, test_value_Float_boundary_high_outbound);
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
       database->setSignal(test_path_Float, test_value_Float_boundary_low_outbound_neg);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_Float_boundary_high_outbound_neg;
    double maxFloat_value_neg = numeric_limits<float>::max() * -1;
    test_value_Float_boundary_high_outbound_neg = maxFloat_value_neg * 2;
    try {
       database->setSignal(test_path_Float, test_value_Float_boundary_high_outbound_neg);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }

    BOOST_TEST(isExceptionThrown == true);

//---------------------  Double SET/GET TEST ------------------------------------

    json test_value_Double_boundary_low;
    test_value_Double_boundary_low = std::numeric_limits<double>::min();

    database->setSignal(test_path_Double, test_value_Double_boundary_low);
    result = database->getSignal(test_path_Double);
    
    BOOST_TEST(result["value"] == std::numeric_limits<double>::min());
    
    json test_value_Double_boundary_high;
    test_value_Double_boundary_high = std::numeric_limits<double>::max();

    database->setSignal(test_path_Double, test_value_Double_boundary_high);
    result = database->getSignal(test_path_Double);

    BOOST_TEST(result["value"] == std::numeric_limits<double>::max());

    
    json test_value_Double_boundary_low_neg;
    test_value_Double_boundary_low_neg = std::numeric_limits<double>::min() * -1;

    database->setSignal(test_path_Double, test_value_Double_boundary_low_neg);
    result = database->getSignal(test_path_Double);
    
    BOOST_TEST(result["value"] == (std::numeric_limits<double>::min() * -1));
    
    json test_value_Double_boundary_high_neg;
    test_value_Double_boundary_high_neg = std::numeric_limits<double>::max() * -1;

    database->setSignal(test_path_Double, test_value_Double_boundary_high_neg);
    result = database->getSignal(test_path_Double);

    BOOST_TEST(result["value"] == (std::numeric_limits<double>::max() * -1));

   

    json test_value_Double_boundary_middle;
    test_value_Double_boundary_middle = std::numeric_limits<double>::max() / 2;

    database->setSignal(test_path_Double, test_value_Double_boundary_middle);
    result = database->getSignal(test_path_Double);

    BOOST_TEST(result["value"] == std::numeric_limits<double>::max() / 2);

    json test_value_Double_boundary_middle_neg;
    test_value_Double_boundary_middle_neg = std::numeric_limits<double>::min() * 2;

    database->setSignal(test_path_Double, test_value_Double_boundary_middle_neg);
    result = database->getSignal(test_path_Double);

    BOOST_TEST(result["value"] == std::numeric_limits<double>::min() * 2);

    // Test positive out of bound
    isExceptionThrown = false;
    json test_value_Double_boundary_low_outbound;
    long double minDouble_value = numeric_limits<double>::min();
    test_value_Double_boundary_low_outbound = minDouble_value / 2;
    try {
       database->setSignal(test_path_Double, test_value_Double_boundary_low_outbound);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    json test_value_Double_boundary_high_outbound;
    long double maxDouble_value = numeric_limits<double>::max();
    test_value_Double_boundary_high_outbound = maxDouble_value * 2;
    try {
       database->setSignal(test_path_Double, test_value_Double_boundary_high_outbound);
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
       database->setSignal(test_path_Double, test_value_Double_boundary_low_outbound_neg);
    } catch (outOfBoundException & e) {
       isExceptionThrown =  true;
    }
    BOOST_TEST(isExceptionThrown == true);

    isExceptionThrown = false;
    
    // no negative infinite test


//---------------------  String SET/GET TEST ------------------------------------

    json test_value_String_empty;
    test_value_String_empty = "";

    database->setSignal(test_path_string, test_value_String_empty);
    result = database->getSignal(test_path_string);
    
    BOOST_TEST(result["value"] == "");

    json test_value_String_null;
    test_value_String_null = "\0";

    database->setSignal(test_path_string, test_value_String_null);
    result = database->getSignal(test_path_string);
    
    BOOST_TEST(result["value"] == "\0");

    json test_value_String_long;
    test_value_String_long = "hello to w3c vis server unit test with boost libraries! This is a test string to test string data type without special characters, but this string is pretty long";

    database->setSignal(test_path_string, test_value_String_long);
    result = database->getSignal(test_path_string);
    
    BOOST_TEST(result["value"] == test_value_String_long);

    json test_value_String_long_with_special_chars;
    test_value_String_long_with_special_chars = "hello to w3c vis server unit test with boost libraries! This is a test string conatains special chars like üö Ä? $ % #";

    database->setSignal(test_path_string, test_value_String_long_with_special_chars);
    result = database->getSignal(test_path_string);
    
    BOOST_TEST(result["value"] == test_value_String_long_with_special_chars);

//---------------------  Boolean SET/GET TEST ------------------------------------
    json test_value_bool_false;
    test_value_bool_false = false;

    database->setSignal(test_path_boolean, test_value_bool_false);
    result = database->getSignal(test_path_boolean);
    
    BOOST_TEST(result["value"] == test_value_bool_false);

    json test_value_bool_true;
    test_value_bool_true = true;

    database->setSignal(test_path_boolean, test_value_bool_true);
    result = database->getSignal(test_path_boolean);
    
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
   channel.setAuthorized(true);
   channel.setAuthToken(AUTH_JWT);
   
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
   channel.setAuthorized(true);
   channel.setAuthToken(AUTH_JWT);
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
   channel.setAuthorized(true);
   channel.setAuthToken(AUTH_JWT);
   
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
   channel.setAuthorized(true);
   channel.setAuthToken(AUTH_JWT);
   string request(R"({
		"action": "get",
		"path": "Signal.*.RPM1",
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);
   
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
   channel.setAuthorized(true);
   channel.setAuthToken(AUTH_JWT);
   string request(R"({
		"action": "set",
		"path": "Signal.OBD.*",
                "value" : [{"RPM1" : 345} , {"Speed1" : 546}],
		"requestId": "8756"
	})");
   
   string response = commandProc->processQuery(request,channel);
   
   json expected = json::parse(R"({
                              "action":"set",
                              "error":{"message":"Path  has 0 signals, the path needs refinement","number":401,"reason":"Unknown error"},
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
   channel.setAuthorized(true);
   channel.setAuthToken(AUTH_JWT);
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
   channel.setAuthorized(true);
   channel.setAuthToken(AUTH_JWT);
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
   
   json response_response_getvalid_json = json::parse(response_getvalid);
   

   BOOST_TEST(response_response_getvalid_json.has_key("timestamp") == true);

   // remove timestamp to match
   response_response_getvalid_json.erase("timestamp");
   BOOST_TEST(response_response_getvalid_json == expected_getvalid);
}

