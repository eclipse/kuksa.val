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

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink;

class VssDatabase_Record : public IVssDatabase
{
#ifdef UNIT_TEST
  friend class w3cunittest;
#endif

public:
    VssDatabase_Record(std::shared_ptr<ILogger> loggerUtil, std::shared_ptr<ISubscriptionHandler> subHandle);
    ~VssDatabase_Record();

    src::logger_mt lg;

    //helpers
    bool pathExists(const VSSPath &path) override;
    bool pathIsWritable(const VSSPath &path) override;
    std::list<VSSPath> getLeafPaths(const VSSPath& path) override;

    void checkAndSanitizeType(jsoncons::json &meta, jsoncons::json &val) override;

    void initJsonTree(const boost::filesystem::path &fileName) override;
  
    bool checkPathValid(const VSSPath& path) override;
    bool isActor(const jsoncons::json &element);
    bool isSensor(const jsoncons::json &element);
    bool isAttribute(const jsoncons::json &element);


    void updateJsonTree(jsoncons::json& sourceTree, const jsoncons::json& jsonTree);
    void updateJsonTree(WsChannel& channel, jsoncons::json& value) override;
    void updateMetaData(WsChannel& channel, const VSSPath& path, const jsoncons::json& newTree) override;
    jsoncons::json getMetaData(const VSSPath& path) override;
  
    jsoncons::json setSignal(const VSSPath &path, jsoncons::json &value) override; //gen2 version
    jsoncons::json getSignal(const VSSPath &path) override; //Gen2 version

    void applyDefaultValues(jsoncons::json &tree, VSSPath currentPath);

private:

    VssDatabase overClass_;

    void logger_init();
    std::string getexepath();
    void log(std::string msg);

    std::string dir_;
    std::string target_name_;
    std::string logfile_name_;

    std::shared_ptr<ILogger> logger_;
    std::mutex rwMutex_;
    std::shared_ptr<ISubscriptionHandler> subHandler_;

};

#endif
