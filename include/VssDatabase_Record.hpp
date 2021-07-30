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

typedef enum
{
  noRecord=0,
  noGet = 1,
  withGet = 2
} RecordDef_t;

typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_file_backend > file_sink;

class VssDatabase_Record : public VssDatabase
{
#ifdef UNIT_TEST
  friend class w3cunittest;
#endif

public:
    VssDatabase_Record(std::shared_ptr<ILogger> loggerUtil, std::shared_ptr<ISubscriptionHandler> subHandle, const std::string recordPath, RecordDef_t logMode);
    ~VssDatabase_Record();

    boost::log::sources::logger_mt lg;
  
    jsoncons::json setSignal(const VSSPath &path, jsoncons::json &value) override; //gen2 version
    jsoncons::json getSignal(const VSSPath &path) override; //Gen2 version


private:

    void logger_init();

    std::string dir_;
    std::string logfile_name_;
    RecordDef_t logMode_;

};

std::istream& operator>>(std::istream& in, RecordDef_t& enumType);

#endif
