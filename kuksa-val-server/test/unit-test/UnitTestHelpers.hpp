/*
 * ******************************************************************************
 * Copyright (c) 2021 Robert Bosch GmbH.
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


/** Includes helper functions that can be used by different test modules */

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <jsoncons/json.hpp>
#include "JsonResponses.hpp"
#include "VSSPath.hpp"

//Verifies a timestamp exists and is of type string.
static inline void verify_timestamp(const jsoncons::json &result) {
  std::string timestampKeyWord = "ts";
  BOOST_TEST(result.contains(timestampKeyWord));
  BOOST_TEST(result[timestampKeyWord].is_string());

  BOOST_CHECK_NO_THROW(boost::posix_time::from_iso_extended_string(result[timestampKeyWord].as_string()));
  BOOST_TEST(boost::posix_time::from_iso_extended_string(result[timestampKeyWord].as_string()) > boost::posix_time::from_iso_extended_string(JsonResponses::getTimeStampZero()));
  BOOST_TEST(boost::posix_time::from_iso_extended_string(result[timestampKeyWord].as_string()) <= boost::posix_time::from_iso_extended_string(JsonResponses::getTimeStamp()));
}

//Verifies a timestamp exists and is of type string.
static inline void verify_timestampZero(const jsoncons::json &result) {
  std::string timestampKeyWord = "ts";
  BOOST_TEST(result.contains(timestampKeyWord));
  BOOST_TEST(result[timestampKeyWord].is_string());
  BOOST_CHECK_NO_THROW(boost::posix_time::from_iso_extended_string(result[timestampKeyWord].as_string()));
  BOOST_TEST(boost::posix_time::from_iso_extended_string(result[timestampKeyWord].as_string()) == boost::posix_time::from_iso_extended_string(JsonResponses::getTimeStampZero()));
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
