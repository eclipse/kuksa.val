#ifndef __VSSDATABASE_RECORD_HPP__
#define __VSSDATABASE_RECORD_HPP__

#include "interface/IVssDatabase.hpp"
#include "VssDatabase"

#include <memory>
#include <iostream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink;

class VssDatabase_Record : public IVssDatabase
{
public:
    VssDatabase_Record(std::shared_ptr<ILogger> loggerUtil,
              std::shared_ptr<ISubscriptionHandler> subHandle);
    ~VssDatabase_Record();

    src::logger lg;

private:
    std::unique_ptr<VssDatabase> overBase;

    void logger_init();
    void log(std::string msg);

    std::string dir;
    std::string target_name;
    std::string logfile_name;
};

#endif
