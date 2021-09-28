/*
 * ******************************************************************************
 * Copyright (c) 2021 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 * *****************************************************************************
 */

/* helper fuctions to load overlays during server startup */

#include <stdexcept>
#include <jsoncons/json.hpp>

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
  auto mockChannel = std::make_shared<kuksa::kuksaChannel>();
  mockChannel->set_modifytree(true);

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