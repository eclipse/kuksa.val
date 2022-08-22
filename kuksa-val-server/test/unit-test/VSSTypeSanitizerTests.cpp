/**********************************************************************
 * Copyright (c) 2021 Robert Bosch GmbH.
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


/** This are tests for the type and limit checks during set */

#include <boost/test/unit_test.hpp>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <turtle/mock.hpp>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <memory>
#include <string>

#include "IAccessCheckerMock.hpp"
#include "IAuthenticatorMock.hpp"
#include "ILoggerMock.hpp"
#include "ISubscriptionHandlerMock.hpp"
#include "exception.hpp"

#include "VSSPath.hpp"

#include "JsonResponses.hpp"
#include "VssCommandProcessor.hpp"
#include "VssDatabase.hpp"

#include "exception.hpp"

namespace {
// common resources for tests
std::string validFilename{"test_vss_release_latest.json"};

std::shared_ptr<ILoggerMock> logMock;
std::shared_ptr<IAccessCheckerMock> accCheckMock;

std::shared_ptr<ISubscriptionHandlerMock> subHandlerMock;

std::shared_ptr<VssDatabase> db;

// Pre-test initialization and post-test desctruction of common resources
struct TestSuiteFixture {
  TestSuiteFixture() {
    logMock = std::make_shared<ILoggerMock>();

    accCheckMock = std::make_shared<IAccessCheckerMock>();
    subHandlerMock = std::make_shared<ISubscriptionHandlerMock>();

    std::string vss_file{"test_vss_release_latest.json"};

    MOCK_EXPECT(logMock->Log).at_least(0);  // ignore log events
    db = std::make_shared<VssDatabase>(logMock, subHandlerMock);
    db->initJsonTree(vss_file);
  }
  ~TestSuiteFixture() {
    logMock.reset();
    accCheckMock.reset();
    subHandlerMock.reset();
  }
};
}  // namespace

static jsoncons::json createEnumMeta(const std::string& enumValues) {
  jsoncons::json meta;
  meta["datatype"] = "string";
  meta["enum"] = json::parse(enumValues);
  return meta;
}

static jsoncons::json createAllowedMeta(const std::string& enumValues) {
  jsoncons::json meta;
  meta["datatype"] = "string";
  meta["allowed"] = json::parse(enumValues);
  return meta;
}

static jsoncons::json createUnlimitedMeta(std::string datatype) {
  jsoncons::json meta;
  meta["datatype"] = datatype;
  return meta;
}

static jsoncons::json createDoublelimitedMeta(std::string datatype,
                                              double min, double max) {
  jsoncons::json meta;
  meta["datatype"] = datatype;
  meta["min"] = min;
  meta["max"] = max;
  return meta;
}

static jsoncons::json createMinlimitedMeta(std::string datatype,
                                              double min) {
  jsoncons::json meta;
  meta["datatype"] = datatype;
  meta["min"] = min;
  return meta;
}

static jsoncons::json createMaxlimitedMeta(std::string datatype,
                                              double max) {
  jsoncons::json meta;
  meta["datatype"] = datatype;
  meta["max"] = max;
  return meta;
}

BOOST_FIXTURE_TEST_SUITE(VSSTypeSanitizerTests, TestSuiteFixture)

BOOST_AUTO_TEST_CASE(uint8_nolimits) {
  jsoncons::json meta = createUnlimitedMeta("uint8");
  jsoncons::json value = "127";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-10";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "400";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "-10";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(uint8_limits) {
  jsoncons::json meta = createDoublelimitedMeta("uint8", 10, 100);
  jsoncons::json value = "50";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "6";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "120";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  meta = createMinlimitedMeta("uint8", 10);
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  meta = createMaxlimitedMeta("uint8", 10);
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(uint8_array) {
  jsoncons::json meta = createUnlimitedMeta("uint8[]");
  jsoncons::json value(jsoncons::json_array_arg, {"10", "127"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value.push_back("-10");
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}


BOOST_AUTO_TEST_CASE(int8_nolimits) {
  jsoncons::json meta = createUnlimitedMeta("int8");
  jsoncons::json value = "-10";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-200";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "400";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int8_limits) {
  jsoncons::json meta = createDoublelimitedMeta("int8", 10, 100);
  jsoncons::json value = "50";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "6";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "120";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  meta = createMinlimitedMeta("int8", 10);
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  meta = createMaxlimitedMeta("int8", -5);
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int8_array) {
  jsoncons::json meta = createUnlimitedMeta("int8[]");
  jsoncons::json value(jsoncons::json_array_arg, {"10", "127"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value.push_back("-129");
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(uint16_nolimits) {
  jsoncons::json meta = createUnlimitedMeta("uint16");
  jsoncons::json value = "4096";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-10";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "65600";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(uint16_limits) {
  jsoncons::json meta = createDoublelimitedMeta("uint16", 10, 800);
  jsoncons::json value = "50";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "6";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "4096";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  meta = createMinlimitedMeta("uint16", 10);
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  meta = createMaxlimitedMeta("uint16", 10);
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(uint16_array) {
  jsoncons::json meta = createUnlimitedMeta("uint16[]");
  jsoncons::json value(jsoncons::json_array_arg, {"10", "4096"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value.push_back("-129");
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int16_nolimits) {
  jsoncons::json meta = createUnlimitedMeta("int16");
  jsoncons::json value = "-10";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-65600";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "65600";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int16_limits) {
  jsoncons::json meta = createDoublelimitedMeta("int16", -50, 100);
  jsoncons::json value = "50";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-51";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "120";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  meta = createMinlimitedMeta("int16", -10);
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  meta = createMaxlimitedMeta("int16", 10);
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int16_array) {
  jsoncons::json meta = createUnlimitedMeta("int16[]");
  jsoncons::json value(jsoncons::json_array_arg, {"10", "-10"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value.push_back("65600");
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}


BOOST_AUTO_TEST_CASE(uint32_nolimits) {
  jsoncons::json meta = createUnlimitedMeta("uint32");
  jsoncons::json value = "100000";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-10";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "4294967297";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(uint32_limits) {
  jsoncons::json meta = createDoublelimitedMeta("uint32", 10, 70000);
  jsoncons::json value = "50";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "6";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "80000";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  meta = createMinlimitedMeta("uint32", 10);
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  meta = createMaxlimitedMeta("uint32", 10);
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(uint32_array) {
  jsoncons::json meta = createUnlimitedMeta("uint32[]");
  jsoncons::json value(jsoncons::json_array_arg, {"10", "1000"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value.push_back("-10");
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int32_nolimits) {
  jsoncons::json meta = createUnlimitedMeta("int32");
  jsoncons::json value = "10";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-4294967297";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "4294967297";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int32_limits) {
  jsoncons::json meta = createDoublelimitedMeta("int32", -50, 100);
  jsoncons::json value = "50";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-51";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "45000";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  meta = createMinlimitedMeta("int32", -10);
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  meta = createMaxlimitedMeta("int32", 38000);
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int32_array) {
  jsoncons::json meta = createUnlimitedMeta("int32[]");
  jsoncons::json value(jsoncons::json_array_arg, {"10", "300"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value.push_back("-4294967297");
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}


BOOST_AUTO_TEST_CASE(uint64_nolimits) {
  jsoncons::json meta = createUnlimitedMeta("uint64");
  jsoncons::json value = "100000";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-10";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "36893488147419103232";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "0xffffffffffffffff";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));
  value = "0x10000000000000000";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
  value = "0.5";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

}

BOOST_AUTO_TEST_CASE(uint64_limits) {
  jsoncons::json meta = createDoublelimitedMeta("uint64", 10, 8589934592);
  jsoncons::json value = "50";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "6";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "8589934593";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  meta = createMinlimitedMeta("uint64", 10);
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  meta = createMaxlimitedMeta("uint64", 10);
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(uint64_array) {
  jsoncons::json meta = createUnlimitedMeta("uint64[]");
  jsoncons::json value(jsoncons::json_array_arg, {"10", "100000"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value.push_back("-10");
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int64_nolimits) {
  jsoncons::json meta = createUnlimitedMeta("int64");
  jsoncons::json value = "10";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-18446744073709551617";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "18446744073709551617";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int64_limits) {
  jsoncons::json meta = createDoublelimitedMeta("int64", -50, 8589934592);
  jsoncons::json value = "50";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-51";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "8689934593";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  meta = createMinlimitedMeta("int64", -10);
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  meta = createMaxlimitedMeta("int64", 8589934592);
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(int64_array) {
  jsoncons::json meta = createUnlimitedMeta("int64[]");
  jsoncons::json value(jsoncons::json_array_arg, {"10", "-100000"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value.push_back("-18446744073709551617");
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(float_nolimits) {
  jsoncons::json meta = createUnlimitedMeta("float");
  jsoncons::json value = "10";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-4e38";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "+4e38";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(float_limits) {
  jsoncons::json meta = createDoublelimitedMeta("float", -50, 40000);
  jsoncons::json value = "50";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-51";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "100000";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  meta = createMinlimitedMeta("float", -10);
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  meta = createMaxlimitedMeta("float", 99000);
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(float_array) {
  jsoncons::json meta = createUnlimitedMeta("float[]");
  jsoncons::json value(jsoncons::json_array_arg, {"10", "-10"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value.push_back("-4e38");
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}
//Only "limited" double tests, as "long double" might not be longer than double
//on all platforms, using som "bigint" library is to expensive
BOOST_AUTO_TEST_CASE(double_limits) {
  jsoncons::json meta = createDoublelimitedMeta("double", -50, 8589934592);
  jsoncons::json value = "50";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "-51";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "8589934593";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  meta = createMinlimitedMeta("double", -10);
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  meta = createMaxlimitedMeta("double", 8589934592);
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}


BOOST_AUTO_TEST_CASE(boolean) {
  jsoncons::json meta = createUnlimitedMeta("boolean");
  jsoncons::json value = "true";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "false";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "BOGUS";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(boolean_array) {
  jsoncons::json meta = createUnlimitedMeta("boolean[]");
  jsoncons::json value(jsoncons::json_array_arg, {"true", "false"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value.push_back("False");
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

// "enum" no longer supported from VSS 3.0 onwards, replaced by "allowed"
BOOST_AUTO_TEST_CASE(enumtype) {
  jsoncons::json meta = createEnumMeta("[\"bla\", \"blu\"]");
  jsoncons::json value = "bla";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "blu";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "b";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
  
  value = "blablu";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

// "allowed" supported from VSS 3.0 onwards, replacing "enum"
BOOST_AUTO_TEST_CASE(allowedtype) {
  jsoncons::json meta = createAllowedMeta("[\"bla\", \"blu\"]");
  jsoncons::json value = "bla";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "blu";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));

  value = "b";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);

  value = "blablu";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), outOfBoundException);
}

BOOST_AUTO_TEST_CASE(string) {
  jsoncons::json meta = createUnlimitedMeta("string");
  jsoncons::json value = "Use Iceoryx!";
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));
}

BOOST_AUTO_TEST_CASE(string_array) {
  jsoncons::json meta = createUnlimitedMeta("string[]");
  jsoncons::json value(jsoncons::json_array_arg, {"Use", "Iceoryx!"});
  BOOST_CHECK_NO_THROW(db->checkAndSanitizeType(meta, value));
}

BOOST_AUTO_TEST_CASE(bogus) {
  jsoncons::json meta = createUnlimitedMeta("not_a_ttype");
  jsoncons::json value = "true";
  BOOST_CHECK_THROW(db->checkAndSanitizeType(meta, value), genException);
}


BOOST_AUTO_TEST_SUITE_END()
