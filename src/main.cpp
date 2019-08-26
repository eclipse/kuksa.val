/*
 * ******************************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 * *****************************************************************************
 */

#include "wsserver.hpp"

#include <boost/program_options.hpp>
#include <iostream>
#include <string>

using namespace std;
using namespace boost;

static void print_usage(const char *prog_name,
                        program_options::options_description desc) {
  cerr << "Usage: " << prog_name << " OPTIONS" << endl;
  cerr << desc << std::endl;
}

int main(int argc, const char *argv[]) {
  program_options::options_description desc{"Options"};
  desc.add_options()
    ("help,h", "Help screen")
    ("vss", program_options::value<string>(), "vss_rel*.json file")
    ("insecure", "Run insecure")
    ("port", program_options::value<int>()->default_value(8090), "Port")
    ("address", program_options::value<string>()->default_value("localhost"), "Address");

  try {
    program_options::variables_map variables;
    program_options::store(parse_command_line(argc, argv, desc), variables);
    program_options::notify(variables);

    if (!variables.count("vss")) {
      print_usage(argv[0], desc);
      cerr << "vss file (--vss) must be specified" << std::endl;
      return -1;
    }

    if (variables.count("help")) {
      print_usage(argv[0], desc);
      return -1;
    }
    auto port = variables["port"].as<int>();
    auto secure = !variables.count("insecure");
    auto vss_filename = variables["vss"].as<string>();
    wsserver server(port, vss_filename, secure);
    server.start();

    while (1) {
      usleep(1000000);
    };
  } catch (const program_options::error &ex) {
    print_usage(argv[0], desc);
    cerr << ex.what() << std::endl;
    return -1;
  }
}
