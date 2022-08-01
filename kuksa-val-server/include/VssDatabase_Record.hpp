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
 **********************************************************************/

#ifndef __VSSDATABASE_RECORD_HPP__
#define __VSSDATABASE_RECORD_HPP__

#include "IVssDatabase.hpp"
#include "VssDatabase.hpp"

#include <memory>
#include <iostream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>

typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_file_backend > file_sink;

class VssDatabase_Record : public VssDatabase
{
#ifdef UNIT_TEST
  friend class w3cunittest;
#endif

public:
    VssDatabase_Record(std::shared_ptr<ILogger> loggerUtil, std::shared_ptr<ISubscriptionHandler> subHandle, const std::string recordPath, std::string logMode);
    ~VssDatabase_Record();

    boost::log::sources::logger_mt lg;
  
    jsoncons::json setSignal(const VSSPath &path, const std::string& attr, jsoncons::json &value) override; //gen2 version
    jsoncons::json getSignal(const VSSPath &path, const std::string& attr, bool as_string=false) override; //Gen2 version

private:

    void logger_init();

    std::string dir_;
    std::string logfile_name_;
    std::string logMode_;

};

#endif
