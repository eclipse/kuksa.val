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

#include "VssDatabase_Record.hpp"

#include <string>
#include <memory>
#include <iostream>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

VssDatabase_Record::VssDatabase_Record(std::shared_ptr<ILogger> loggerUtil, std::shared_ptr<ISubscriptionHandler> subHandle, const std::string recordPath, std::string logMode)
:VssDatabase(loggerUtil,subHandle), logMode_(logMode)
{
    
    std::string workingDir = recordPath;

    if(workingDir==".")
        workingDir += "/logs";

    logfile_name_ = "record-%Y%m%d_%H%M%S.log.csv";
    dir_ = workingDir;

    std::cout << "Saving record file to " << dir_ << std::endl;

    logger_init();
    logging::core::get() -> add_global_attribute("TimeStamp",attrs::local_clock());
    logging::core::get() -> add_global_attribute("RecordID",attrs::counter<unsigned int>());
}
VssDatabase_Record::~VssDatabase_Record()
{
    
}

void VssDatabase_Record::logger_init()
{
    boost::shared_ptr<file_sink> sink
    (
        new file_sink
        (
            keywords::file_name = logfile_name_,
            keywords::target_file_name = logfile_name_,
            keywords::target = dir_,
            keywords::auto_flush = true
        )
    );
    sink->locked_backend()->set_file_collector(sinks::file::make_collector(keywords::target = dir_));
    sink->locked_backend()->scan_for_files();
    //rotates files for every start of application

    sink->set_formatter(
        expr::format("%1%;%2%;%3%") 
        % expr::attr< boost::posix_time::ptime >("TimeStamp")
        % expr::attr< unsigned int > ("RecordID")
        % expr::smessage
    );

    logging::core::get() -> add_sink(sink);
}

jsoncons::json VssDatabase_Record::setSignal(const VSSPath &path, const std::string& attr, jsoncons::json &value)
{
    std::string json_val;
    value.dump_pretty(json_val);
    BOOST_LOG(lg) << "set;" << attr << ";" << path.to_string() << ";" + json_val;
    return VssDatabase::setSignal(path, attr, value);
}

jsoncons::json VssDatabase_Record::getSignal(const VSSPath &path, const std::string& attr, bool as_string)
{
    if(logMode_ == "recordSetAndGet")
        BOOST_LOG(lg) << "get;" << attr << ";" << path.to_string();

    return VssDatabase::getSignal(path, attr);
}
