#include "VssDatabase_Record.hpp"

VssDatabase_Record::VssDatabase_Record(std::shared_ptr<ILogger> loggerUtil, std::shared_ptr<ISubscriptionHandler> subHandle)
{
    overBase = make_unique<VssDatabase>(loggerUtil,subHandle);
    logfile_name = "sample_log%Y%m%d_%H%M%S.log";
    target_name = "logs";
    dir = "logs";

    logger_init();
    logging::core::get() -> add_global_attribute("TimeStamp",attrs::local_clock());
    logging::core::get() -> add_global_attribute("RecordID",attrs::counter<unsigned int>());
}

void VssDatabase_Record::logger_init()
{
    boost::shared_ptr<file_sink> sink
    (
        new file_sink
        (
            keywords::file_name = logfile_name,
            keywords::target_file_name = logfile_name,
            keywords::target=target_name
        )
    );
    sink->locked_backend()->set_file_collector(sinks::file::make_collector(keywords::target = target_name));
    sink->locked_backend()->scan_for_files();
    //rotates files for every start of application

    sink->set_formatter(
        expr::format("[\"%1%\"]ID=\"%2%\" \"%3%\"") 
        % expr::attr< boost::posix_time::ptime >("TimeStamp")
        % expr::attr< unsigned int > ("RecordID")
        % expr::xml_decor[ expr::stream << expr::smessage ]
    );

    logging::core::get() -> add_sink(sink);
}

void VssDatabase_Record::log(std::string msg)
{
    BOOST_LOG(lg) << msg;
}

