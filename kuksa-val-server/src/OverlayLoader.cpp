/**********************************************************************
 * Copyright (c) 2021 Robert Bosch GmbH.
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


/* helper fuctions to load overlays during server startup */

#include <stdexcept>
#include <jsoncons/json.hpp>
#include <fstream>

#include "OverlayLoader.hpp"
#include "kuksa.pb.h"

using jsoncons::json;

std::vector<boost::filesystem::path> gatherOverlays(
    std::shared_ptr<ILogger> logger, boost::filesystem::path overlaydir) {
  std::vector<boost::filesystem::path> overlayfiles;
  logger->Log(LogLevel::VERBOSE,
              "Searching overlays in \"" + overlaydir.generic_string() + "\"");
  if (!boost::filesystem::is_directory(overlaydir)) {
    throw std::runtime_error("Overlays need to be a directory. \"" +
                             overlaydir.generic_string() + "\" is not.");
  }
  for (auto &p : boost::filesystem::directory_iterator(overlaydir)) {
    if (boost::filesystem::is_regular_file(p.path()) &&
        p.path().extension() == ".json") {
      logger->Log(LogLevel::VERBOSE, "Found " + p.path().generic_string());
      overlayfiles.push_back(p);
    }
  }

  // Sort according to filename
  std::sort(overlayfiles.begin(), overlayfiles.end(),
            [](boost::filesystem::path a, boost::filesystem::path b) {
              return a.filename() < b.filename();
            });

  return overlayfiles;
}

void applyOverlays(std::shared_ptr<ILogger> log,
                   const std::vector<boost::filesystem::path> overlayfiles,
                   std::shared_ptr<IVssDatabase> db) {
  // Mock a channel with permissions to change VSS tree
  auto mockChannel = std::make_shared<KuksaChannel>();
  mockChannel->enableModifyTree();

  for (auto const &p : overlayfiles) {
    log->Log(LogLevel::INFO, "Loading overlay \"" + p.generic_string() + "\"");
    ;
    try {
      std::ifstream is(p.generic_string());
      jsoncons::json overlay = json::parse(is);
      db->updateJsonTree(*mockChannel, overlay);
    } catch (std::exception &e) {
      throw std::runtime_error("Error loading \"" + p.generic_string() +
                               "\": " + e.what());
    }
  }
}
