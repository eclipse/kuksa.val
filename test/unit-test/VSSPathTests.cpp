/*
 * ******************************************************************************
 * Copyright (c) 2020 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH 
 * *****************************************************************************
 */

#include <boost/test/unit_test.hpp>

#include "VSSPath.hpp"


BOOST_AUTO_TEST_SUITE( VSSPathTests )

BOOST_AUTO_TEST_CASE(From_Gen1_Creation_Simple) {
    VSSPath p = VSSPath::fromVSSGen1("Vehicle.Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == true);
}

BOOST_AUTO_TEST_CASE(From_Gen1_Creation_Wildcard_End) {
    VSSPath p = VSSPath::fromVSSGen1("Vehicle.*");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]");
    BOOST_TEST(p.isGen1Origin() == true);
}

BOOST_AUTO_TEST_CASE(From_Gen1_Creation_Wildcard_Middle) {
    VSSPath p = VSSPath::fromVSSGen1("Vehicle.*.Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == true);
}

BOOST_AUTO_TEST_CASE(From_Gen1_Empty) {
    VSSPath p = VSSPath::fromVSSGen1("");
    BOOST_TEST(p.getVSSGen1Path() == "");
    BOOST_TEST(p.getVSSPath() == "");
    BOOST_TEST(p.getJSONPath() == "$['']");
    BOOST_TEST(p.isGen1Origin() == true);
}


BOOST_AUTO_TEST_CASE(From_Gen2_Creation_Simple) {
    VSSPath p = VSSPath::fromVSS("Vehicle/Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Gen2_Creation_Wildcard_End) {
    VSSPath p = VSSPath::fromVSS("Vehicle/*");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]");
}

BOOST_AUTO_TEST_CASE(From_Gen2_Creation_Wildcard_Middle) {
    VSSPath p = VSSPath::fromVSS("Vehicle/*/Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Gen2_Empty) {
    VSSPath p = VSSPath::fromVSS("");
    BOOST_TEST(p.getVSSGen1Path() == "");
    BOOST_TEST(p.getVSSPath() == "");
    BOOST_TEST(p.getJSONPath() == "$['']");
    BOOST_TEST(p.isGen1Origin() == false);
}


BOOST_AUTO_TEST_CASE(From_Json_Creation_Simple) {
    VSSPath p = VSSPath::fromJSON("$['Vehicle']['children']['Speed']");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Json_Creation_Wildcard_End) {
    VSSPath p = VSSPath::fromJSON("$['Vehicle']['children'][*]");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Json_Creation_Wildcard_Middle) {
    VSSPath p = VSSPath::fromJSON("$['Vehicle']['children'][*]['children']['Speed']");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Gen1Auto_Creation_Simple) {
    VSSPath p = VSSPath::fromVSSAuto("Vehicle.Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == true);
}

BOOST_AUTO_TEST_CASE(From_Gen1Auto_Creation_Wildcard_End) {
    VSSPath p = VSSPath::fromVSSAuto("Vehicle.*");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]");
    BOOST_TEST(p.isGen1Origin() == true);
}

BOOST_AUTO_TEST_CASE(From_Gen1Auto_Creation_Wildcard_Middle) {
    VSSPath p = VSSPath::fromVSSAuto("Vehicle.*.Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == true);
}

BOOST_AUTO_TEST_CASE(From_Gen2Auto_Creation_Simple) {
    VSSPath p = VSSPath::fromVSSAuto("Vehicle/Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Gen2Auto_Creation_Wildcard_End) {
    VSSPath p = VSSPath::fromVSSAuto("Vehicle/*");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Gen2Auto_Creation_Wildcard_Middle) {
    VSSPath p = VSSPath::fromVSSAuto("Vehicle/*/Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Auto_Empty) {
    VSSPath p = VSSPath::fromVSSAuto("");
    BOOST_TEST(p.getVSSGen1Path() == "");
    BOOST_TEST(p.getVSSPath() == "");
    BOOST_TEST(p.getJSONPath() == "$['']");
    BOOST_TEST(p.isGen1Origin() == false);
}


BOOST_AUTO_TEST_SUITE_END()