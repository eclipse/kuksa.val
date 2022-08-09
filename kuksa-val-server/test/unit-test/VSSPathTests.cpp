/**********************************************************************
 * Copyright (c) 2020 Robert Bosch GmbH.
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
    VSSPath p = VSSPath::fromVSSGen2("Vehicle/Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Gen2_Creation_Wildcard_End) {
    VSSPath p = VSSPath::fromVSSGen2("Vehicle/*");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]");
}

BOOST_AUTO_TEST_CASE(From_Gen2_Creation_Wildcard_Middle) {
    VSSPath p = VSSPath::fromVSSGen2("Vehicle/*/Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Gen2_Empty) {
    VSSPath p = VSSPath::fromVSSGen2("");
    BOOST_TEST(p.getVSSGen1Path() == "");
    BOOST_TEST(p.getVSSPath() == "");
    BOOST_TEST(p.getJSONPath() == "$['']");
    BOOST_TEST(p.isGen1Origin() == false);
}


BOOST_AUTO_TEST_CASE(From_Json_Creation_Simple) {
    VSSPath p = VSSPath::fromJSON("$['Vehicle']['children']['Speed']", false);
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Json_Creation_Wildcard_End) {
    VSSPath p = VSSPath::fromJSON("$['Vehicle']['children'][*]", true);
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]");
    BOOST_TEST(p.isGen1Origin() == true);
}

BOOST_AUTO_TEST_CASE(From_Json_Creation_Wildcard_Middle) {
    VSSPath p = VSSPath::fromJSON("$['Vehicle']['children'][*]['children']['Speed']", false);
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Gen1Auto_Creation_Simple) {
    VSSPath p = VSSPath::fromVSS("Vehicle.Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == true);
}

BOOST_AUTO_TEST_CASE(From_Gen1Auto_Creation_Wildcard_End) {
    VSSPath p = VSSPath::fromVSS("Vehicle.*");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]");
    BOOST_TEST(p.isGen1Origin() == true);
}

BOOST_AUTO_TEST_CASE(From_Gen1Auto_Creation_Wildcard_Middle) {
    VSSPath p = VSSPath::fromVSS("Vehicle.*.Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == true);
}

BOOST_AUTO_TEST_CASE(From_Gen2Auto_Creation_Simple) {
    VSSPath p = VSSPath::fromVSS("Vehicle/Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Gen2Auto_Creation_Wildcard_End) {
    VSSPath p = VSSPath::fromVSS("Vehicle/*");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Gen2Auto_Creation_Wildcard_Middle) {
    VSSPath p = VSSPath::fromVSS("Vehicle/*/Speed");
    BOOST_TEST(p.getVSSGen1Path() == "Vehicle.*.Speed");
    BOOST_TEST(p.getVSSPath() == "Vehicle/*/Speed");
    BOOST_TEST(p.getJSONPath() == "$['Vehicle']['children'][*]['children']['Speed']");
    BOOST_TEST(p.isGen1Origin() == false);
}

BOOST_AUTO_TEST_CASE(From_Auto_Empty) {
    VSSPath p = VSSPath::fromVSS("");
    BOOST_TEST(p.getVSSGen1Path() == "");
    BOOST_TEST(p.getVSSPath() == "");
    BOOST_TEST(p.getJSONPath() == "$['']");
    BOOST_TEST(p.isGen1Origin() == false);
}


BOOST_AUTO_TEST_SUITE_END()
