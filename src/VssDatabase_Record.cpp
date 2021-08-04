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

VssDatabase_Record::VssDatabase_Record(std::shared_ptr<ILogger> loggerUtil, std::shared_ptr<ISubscriptionHandler> subHandle, const std::string recordPath, int logMode)
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

jsoncons::json VssDatabase_Record::setSignal(const VSSPath &path, jsoncons::json &value)
{
    std::string json_val;
    value.dump_pretty(json_val);
    BOOST_LOG(lg) << "set;" << path.to_string() << ";" + json_val;
    return VssDatabase::setSignal(path,value);
}

jsoncons::json VssDatabase_Record::getSignal(const VSSPath &path)
{
    if(logMode_ == withGet)
        BOOST_LOG(lg) << "get;" << path.to_string();

    return VssDatabase::getSignal(path);
}
