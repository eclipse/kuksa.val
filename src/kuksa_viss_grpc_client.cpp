/*
 * ******************************************************************************
 * Copyright (c) 2021 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH
 * *****************************************************************************
 */


#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "grpcHandler.hpp"



int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  boost::program_options::options_description desc{"OPTIONS"};
  desc.add_options()
    ("help,h", "Help screen")
    ("config-file,c", boost::program_options::value<boost::filesystem::path>()->default_value(boost::filesystem::path{"config_grpc_client.ini"}),
      "Configuration file with `kuksa-val-server` input parameters."
      "Configuration file can replace command-line parameters and through different files multiple configurations can be handled more easily (e.g. test and production setup)."
      "Sample of configuration file parameters looks like:\n"
      "cert-path = . \n")
    ("cert-path", boost::program_options::value<boost::filesystem::path>()->required()->default_value(boost::filesystem::path(".")),
      "[mandatory] Directory path where 'Client.pem', 'Client.key', and 'CA.pem' are located. ")
    ("log-level","Log Level")
    ("target",boost::program_options::value<std::string>()->default_value("127.0.0.1:50051"),"Value for a server connection");
  boost::program_options::variables_map var;
  boost::program_options::store(parse_command_line(argc, argv, desc), var);
  if (var.count("config-file")) {
    auto configFile = var["config-file"].as<boost::filesystem::path>();
    auto configFilePath = boost::filesystem::path(configFile);
    std::cout << "Read configs from " <<  configFile.string() <<std::endl;
    std::ifstream ifs(configFile.string());
    if (ifs) {
      // update path only, if these options is not defined via command line, but
      // via config file
      boost::program_options::store(parse_config_file(ifs, desc), var);
      auto cert_path = var["cert-path"].as<boost::filesystem::path>();
      var.at("cert-path").value() =
          boost::filesystem::absolute(cert_path, configFilePath.parent_path());
      std::cout << "Update cert-path to "
                << var["cert-path"].as<boost::filesystem::path>().string()
                << std::endl;
    } else if (!var["config-file"].defaulted()) {
      std::cerr << "Could not open config file: " << configFile << std::endl;
    }
    // store again, because command line argument prefered
    boost::program_options::store(parse_command_line(argc, argv, desc), var);
  }
  grpcHandler::startClient(var["target"].as<std::string>(),var["cert-path"].as<boost::filesystem::path>().string());
  return 0;
}
