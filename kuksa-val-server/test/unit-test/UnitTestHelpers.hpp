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



/** Includes helper functions that can be used by different test modules */

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <jsoncons/json.hpp>
#include "JsonResponses.hpp"
#include "VSSPath.hpp"

#include <string>
#include <boost/regex.hpp>

//Verifies a timestamp exists and is of type string.
static inline void verify_timestamp(const jsoncons::json &result) {
  std::string timestampKeyWord = "ts";
  BOOST_TEST(result.contains(timestampKeyWord));
  BOOST_TEST(result[timestampKeyWord].is_string());

  static const boost::regex iso_expr("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}.\\d+Z");
  BOOST_TEST(boost::regex_match(result[timestampKeyWord].as_string(), iso_expr));
/* This code is useful, but as of writing the from_iso_extended_string is bugged, and is unreliable parses 
 * fractional seconds (seems to always require a 9 digits or none). 
/*
  BOOST_CHECK_NO_THROW(boost::posix_time::from_iso_extended_string(result[timestampKeyWord].as_string()));
  BOOST_TEST(boost::posix_time::from_iso_extended_string(result[timestampKeyWord].as_string()) > boost::posix_time::from_iso_extended_string(JsonResponses::getTimeStampZero()));
  BOOST_TEST(boost::posix_time::from_iso_extended_string(result[timestampKeyWord].as_string()) <= boost::posix_time::from_iso_extended_string(JsonResponses::getTimeStamp()));
*/
}

//Verifies a timestamp exists and is of type string.
static inline void verify_timestampZero(const jsoncons::json &result) {
  std::string timestampKeyWord = "ts";
  BOOST_TEST(result.contains(timestampKeyWord));
  BOOST_TEST(result[timestampKeyWord].is_string());
  static const boost::regex iso_expr("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}.\\d+Z");

  std::string str=result[timestampKeyWord].as_string();

  BOOST_TEST(boost::regex_match(result[timestampKeyWord].as_string(), iso_expr));
  BOOST_TEST(result[timestampKeyWord].as_string() == JsonResponses::getTimeStampZero());
}


//Verifies a timestamp exists and is of type string and erase the item for easy comparison
static inline void verify_and_erase_timestamp(jsoncons::json &result) { 
  std::string timestampKeyWord = "ts";
  verify_timestamp(result);
  result.erase(timestampKeyWord);
}

//Verifies a timestamp exists and is of type string and erase the item for easy comparison
static inline void verify_and_erase_timestampZero(jsoncons::json &result) { 
  std::string timestampKeyWord = "ts";
  verify_timestampZero(result);
  result.erase(timestampKeyWord);
}

static inline jsoncons::json packDataInJson(const VSSPath& path, const std::string& value) { 
  jsoncons::json data, datapoint;
  data.insert_or_assign("path", path.to_string());
  datapoint.insert_or_assign("value", value);
  datapoint.insert_or_assign("ts", JsonResponses::getTimeStamp());
  data.insert_or_assign("dp", datapoint);
  return data;
}
