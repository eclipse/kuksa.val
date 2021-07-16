#include "VssDatabase_Record.hpp"

#include <string>
#include <memory>
#include <iostream>

VssDatabase_Record::VssDatabase_Record(std::shared_ptr<ILogger> loggerUtil, std::shared_ptr<ISubscriptionHandler> subHandle)
:overClass_(loggerUtil,subHandle)
{
    std::string workingDir = getexepath();
    workingDir.replace(workingDir.find("kuksa-val-server"),workingDir.size()-workingDir.find("kuksa-val-server"),"");
    logfile_name_ = "record-%Y%m%d_%H%M%S.log";
    target_name_ = "logs";
    dir_ = workingDir + "logs";
    std::cout << dir_ << std::endl; //debugging

    logger_init();
    logging::core::get() -> add_global_attribute("TimeStamp",attrs::local_clock());
    logging::core::get() -> add_global_attribute("RecordID",attrs::counter<unsigned int>());
}
VssDatabase_Record::~VssDatabase_Record()
{
    log("Destructor called");
}

void VssDatabase_Record::logger_init()
{
    boost::shared_ptr<file_sink> sink
    (
        new file_sink
        (
            keywords::file_name = logfile_name_,
            keywords::target_file_name = logfile_name_,
            keywords::target = target_name_,
            keywords::auto_flush = true
        )
    );
    sink->locked_backend()->set_file_collector(sinks::file::make_collector(keywords::target = target_name_));
    sink->locked_backend()->scan_for_files();
    //rotates files for every start of application

    sink->set_formatter(
        expr::format("[\"%1%\"]ID=\"%2%\" \"%3%\"") 
        % expr::attr< boost::posix_time::ptime >("TimeStamp")
        % expr::attr< unsigned int > ("RecordID")
        % expr::smessage
    );

    logging::core::get() -> add_sink(sink);
}

void VssDatabase_Record::log(std::string msg)
{
    BOOST_LOG(lg) << msg;
}

jsoncons::json VssDatabase_Record::setSignal(const VSSPath &path, jsoncons::json &value)
{
    std::string json_val;
    value.dump_pretty(json_val);
    log("action:set " + path.to_string() + " to " + json_val);
    std::cout << "action:set " << path.to_string() << " to " << "value" << std::endl;
    return overClass_.setSignal(path,value);
}

jsoncons::json VssDatabase_Record::getSignal(const VSSPath &path)
{
    log("action:get " + path.to_string());
    std::cout << "action:get " << path.to_string() << std::endl;
    return overClass_.getSignal(path);
}

std::string VssDatabase_Record::getexepath()
{
    char result[ PATH_MAX ];
    ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
    return std::string( result, (count > 0) ? count : 0 );
}

bool VssDatabase_Record::pathExists(const VSSPath &path){return overClass_.pathExists(path);}
bool VssDatabase_Record::pathIsWritable(const VSSPath &path){return overClass_.pathIsWritable(path);}
std::list<VSSPath> VssDatabase_Record::getLeafPaths(const VSSPath& path){return overClass_.getLeafPaths(path);}
void VssDatabase_Record::checkAndSanitizeType(jsoncons::json &meta, jsoncons::json &val){overClass_.checkAndSanitizeType(meta,val);}
void VssDatabase_Record::initJsonTree(const boost::filesystem::path &fileName){overClass_.initJsonTree(fileName);}
bool VssDatabase_Record::checkPathValid(const VSSPath& path){return overClass_.checkPathValid(path);}
bool VssDatabase_Record::isActor(const jsoncons::json &element){return overClass_.isActor(element);}
bool VssDatabase_Record::isSensor(const jsoncons::json &element){return overClass_.isSensor(element);}
bool VssDatabase_Record::isAttribute(const jsoncons::json &element){return overClass_.isAttribute(element);}
void VssDatabase_Record::updateJsonTree(jsoncons::json& sourceTree, const jsoncons::json& jsonTree){overClass_.updateJsonTree(sourceTree,jsonTree);}
void VssDatabase_Record::updateJsonTree(WsChannel& channel, jsoncons::json& value){overClass_.updateJsonTree(channel,value);}
void VssDatabase_Record::updateMetaData(WsChannel& channel, const VSSPath& path, const jsoncons::json& newTree){overClass_.updateMetaData(channel,path,newTree);}
jsoncons::json VssDatabase_Record::getMetaData(const VSSPath& path){return overClass_.getMetaData(path);}
