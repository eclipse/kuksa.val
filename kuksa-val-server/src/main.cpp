/**********************************************************************
 * Copyright (c) 2019-2022 Robert Bosch GmbH.
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


#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <exception>
#include <iostream>
#include <string>
#include <csignal>

#include <boost/program_options.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/filesystem.hpp>
#include <jsoncons/json.hpp>
#include <jsonpath/json_query.hpp>
#include <thread>                                                               

#include "AccessChecker.hpp"
#include "Authenticator.hpp"
#include "BasicLogger.hpp"
#include "SubscriptionHandler.hpp"
#include "VssCommandProcessor.hpp"
#include "VssDatabase.hpp"
#include "VssDatabase_Record.hpp"
#include "WebSockHttpFlexServer.hpp"
#include "MQTTPublisher.hpp"
#include "exception.hpp"
#include "grpcHandler.hpp"
#include "OverlayLoader.hpp"


#include "../buildinfo.h"

using namespace boost;
using namespace jsoncons;
using namespace jsoncons::jsonpath;
using jsoncons::json;

// Websocket port
#define PORT 8090
sighandler_t former_handler;
void ctrlC_Handler(sig_atomic_t signal)
{
  try
  {
    std::cout << "STOP ..." << std::endl;
    boost::log::core::get()->remove_all_sinks();                                                 
    std::signal(signal, former_handler);
    std::raise(signal);
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    std::cerr << "Try searching for log file in src folder \n";
  }
}

static void print_usage(const char *prog_name,
                        program_options::options_description &desc) {
  cerr << "Usage: " << prog_name << " OPTIONS" << endl;
  cerr << desc << std::endl;
}

void httpRunServer(boost::program_options::variables_map variables, std::shared_ptr<WebSockHttpFlexServer> httpServer, std::shared_ptr<VssCommandProcessor> cmdProcessor){

    auto port = variables["port"].as<int>();
    auto insecure = variables[("insecure")].as<bool>();
    httpServer->AddListener(ObserverType::ALL, cmdProcessor);
    httpServer->Initialize(variables["address"].as<string>(), port,
                           variables["cert-path"].as<boost::filesystem::path>().string(), insecure);
    httpServer->Start();
}

int main(int argc, const char *argv[]) {
  vector<string> logLevels{"NONE"};
  uint8_t logLevelsActive = static_cast<uint8_t>(LogLevel::NONE);

  std::cout << "kuksa.val server" << std::endl;
  std::cout << "Commit "  << string(GIT_HEAD_SHA1).substr(0,7);
  if (GIT_IS_DIRTY)
    std::cout << "-dirty";
  std::cout << " from " << GIT_COMMIT_DATE_ISO8601 << std::endl;

  former_handler = signal(SIGINT,ctrlC_Handler);

  program_options::options_description desc{"OPTIONS"};
  desc.add_options()
    ("help,h", "Help screen")
    ("config-file,c", program_options::value<boost::filesystem::path>()->default_value(boost::filesystem::path{"config.ini"}),
      "Configuration file with `kuksa-val-server` input parameters."
      "Configuration file can replace command-line parameters and through different files multiple configurations can be handled more easily (e.g. test and production setup)."
      "Sample of configuration file parameters looks like:\n"
      "vss = vss_release_4.0.json\n"
      "cert-path = . \n"
      "log-level = ALL\n")
    ("vss", program_options::value<boost::filesystem::path>()->required(), "[mandatory] Path to VSS data file describing VSS data tree structure which `kuksa-val-server` shall handle. Sample 'vss_release_4.0.json' file can be found under [data](./data/vss-core/vss_release_4.0.json)")
    ("overlays", program_options::value<boost::filesystem::path>(), "Path to a directory cotaiing additional VSS models. All json files will be applied on top of the main vss file given by the -vss parameter in alphanumerical order")
    ("cert-path", program_options::value<boost::filesystem::path>()->required()->default_value(boost::filesystem::path(".")),
      "[mandatory] Directory path where 'Server.pem', 'Server.key' and 'jwt.key.pub' are located. ")
    ("insecure", program_options::bool_switch()->default_value(false), "By default, `kuksa-val-server` shall accept only SSL (TLS) secured connections. If provided, `kuksa-val-server` shall also accept plain un-secured connections for Web-Socket and GRPC API connections, and also shall not fail connections due to self-signed certificates.")
    ("use-keycloak", "Use KeyCloak for permission management")
    ("address", program_options::value<string>()->default_value("127.0.0.1"),
      "If provided, `kuksa-val-server` shall use different server address than default _'localhost'_")
    ("port", program_options::value<int>()->default_value(8090),
        "If provided, `kuksa-val-server` shall use different server port than default '8090' value")
    ("record", program_options::value<string>() -> default_value("noRecord"), 
        "Enables recording into log file, for later being replayed into the server \nnoRecord: no data will be recorded\nrecordSet: record setting values only\nrecordSetAndGet: record getting value and setting value")
    ("record-path",program_options::value<string>() -> default_value("."),
        "Specifies record file path.")
    ("log-level",
      program_options::value<vector<string>>(&logLevels)->composing(),
      "Enable selected log level value. To allow for different log level "
      "combinations, parameter can be provided multiple times with different "
      "log level values.\n"
      "Supported log levels: NONE, VERBOSE, INFO, WARNING, ERROR, ALL");
  desc.add(MQTTPublisher::getOptions());
  program_options::variables_map variables;
  program_options::store(parse_command_line(argc, argv, desc), variables);
  // if config file passed, get configuration from it
  if (variables.count("config-file")) {
    auto configFile = variables["config-file"].as<boost::filesystem::path>();
    auto configFilePath = boost::filesystem::path(configFile);
    std::cout << "Read configs from " <<  configFile.string() <<std::endl;
    std::ifstream ifs(configFile.string());
    if (ifs) {
      // update path only, if these options is not defined via command line, but
      // via config file
      program_options::store(parse_config_file(ifs, desc), variables);
      auto vss_path = variables["vss"].as<boost::filesystem::path>();
      variables.at("vss").value() =
          boost::filesystem::absolute(vss_path, configFilePath.parent_path());
      std::cout << "Update vss path to "
                << variables["vss"].as<boost::filesystem::path>().string()
                << std::endl;
      auto cert_path = variables["cert-path"].as<boost::filesystem::path>();
      variables.at("cert-path").value() =
          boost::filesystem::absolute(cert_path, configFilePath.parent_path());
      std::cout << "Update cert-path to "
                << variables["cert-path"].as<boost::filesystem::path>().string()
                << std::endl;
    } else if (!variables["config-file"].defaulted()) {
      std::cerr << "Could not open config file: " << configFile << std::endl;
    }
    // store again, because command line argument prefered
    program_options::store(parse_command_line(argc, argv, desc), variables);
  }
  // print usage
  if (variables.count("help")) {
    print_usage(argv[0], desc);
    return 0;
  }
  program_options::notify(variables);

  for (auto const &token : logLevels) {
    if (token == "NONE")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::NONE);
    else if (token == "VERBOSE")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::VERBOSE);
    else if (token == "INFO")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::INFO);
    else if (token == "WARNING")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::WARNING);
    else if (token == "ERROR")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::ERROR);
    else if (token == "ALL")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::ALL);
    else {
      cerr << "Invalid input parameter for LogLevel" << std::endl;
      return -1;
    }
  }

  // Initialize logging
  auto logger = std::make_shared<BasicLogger>(logLevelsActive);

  try {
    // initialize server
    auto vss_path = variables["vss"].as<boost::filesystem::path>();

    //check overlaypath
    std::vector<boost::filesystem::path> overlayfiles;
    if ( variables.count("overlays") )
    {
      overlayfiles = gatherOverlays(logger, variables["overlays"].as<boost::filesystem::path>());
    }

    // initialize pseudo random number generator
    std::srand(std::time(nullptr));


    auto pubKeyFile =
        variables["cert-path"].as<boost::filesystem::path>() / "jwt.key.pub";
    string jwtPubkey =
        Authenticator::getPublicKeyFromFile(pubKeyFile.string(), logger);
    auto httpServer = std::make_shared<WebSockHttpFlexServer>(
        logger);

    auto tokenValidator =
        std::make_shared<Authenticator>(logger, jwtPubkey, "RS256");
    auto accessCheck = std::make_shared<AccessChecker>(tokenValidator);

    auto mqttPublisher = std::make_shared<MQTTPublisher>(
        logger, "vss", variables);

    auto subHandler = std::make_shared<SubscriptionHandler>(
        logger, httpServer, tokenValidator, accessCheck);
    subHandler->addPublisher(mqttPublisher);

    std::shared_ptr<VssDatabase> database = std::make_shared<VssDatabase>(logger,subHandler);

    if(variables["record"].as<string>() == "recordSet" || variables["record"].as<string>()=="recordSetAndGet")
    {
        if(variables["record"].as<string>() == "recordSet")
          std::cout << "Recording inputs\n";
        else if(variables["record"].as<string>() == "recordSetAndGet")
          std::cout << "Recording in- and outputs\n";
    
        database.reset(new VssDatabase_Record(logger,subHandler,variables["record-path"].as<string>(),variables["record"].as<string>()));
    }
    else if(variables["record"].as<string>() !="noRecord")
      throw std::runtime_error("record option \"" + variables["record"].as<string>() + "\" is invalid");


    auto cmdProcessor = std::make_shared<VssCommandProcessor>(
        logger, database, tokenValidator, accessCheck, subHandler);

    database->initJsonTree(vss_path);
    applyOverlays(logger, overlayfiles ,database);

    if(variables.count("mqtt.publish")){
        string path_to_publish = variables["mqtt.publish"].as<string>();

        // remove whitespace and "
        path_to_publish = std::regex_replace(path_to_publish, std::regex("\\s+"), std::string(""));
        path_to_publish = std::regex_replace(path_to_publish, std::regex("\""), std::string(""));

      if (!path_to_publish.empty()) {
        std::stringstream topicsstream(path_to_publish);
        std::string token;
        while (std::getline(topicsstream, token, ';')) {
          if (database->checkPathValid(VSSPath::fromVSSGen1(token))) {
            mqttPublisher->addPublishPath(token);
          } else {
            logger->Log(LogLevel::ERROR,
                        string("main: ") + token +
                            string(" is not a valid path to publish"));
          }
        }
      }
      bool insecureConn;
      if(variables.count("insecure")){
        insecureConn = variables["insecure"].as<bool>();
      }
      std::thread http(httpRunServer, variables, httpServer, cmdProcessor);
      std::thread grpc(grpcHandler::RunServer, cmdProcessor, database, subHandler, logger, variables["cert-path"].as<boost::filesystem::path>().string(),insecureConn);
      http.join();
      grpc.join();
      

    }
  } catch (const program_options::error &ex) {
    print_usage(argv[0], desc);
    cerr << ex.what() << std::endl;
    return -1;
  } catch (const std::runtime_error &ex) {
    logger->Log(LogLevel::ERROR, "Fatal runtime error: " + string(ex.what()));
    return -1;
  }
  return 0;
}
