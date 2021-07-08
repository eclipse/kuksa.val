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

//Verifies a timestamp exists and is of type string. Copies it to the
//expected answer
static inline void verify_timestamp(jsoncons::json &expected, const jsoncons::json &result)  {
  BOOST_TEST(result.contains("ts"));
  BOOST_TEST(result["ts"].is_string());
  BOOST_CHECK_NO_THROW(boost::posix_time::from_iso_extended_string(result["ts"].as_string()));
  expected["ts"]=result["ts"].as_string(); 
}