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
    jsoncons::json getSignal(const VSSPath &path, const std::string& attr) override; //Gen2 version

private:

    void logger_init();

    std::string dir_;
    std::string logfile_name_;
    std::string logMode_;

};

#endif
