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

#include "BasicLogger.hpp"

#include <iostream>


using namespace std;

namespace {
  std::string GetLevelString(LogLevel level) {
    std::string retStr;
    switch (level) {
    case LogLevel::VERBOSE:
      retStr = "VERBOSE: ";
      break;
    case LogLevel::INFO:
      retStr = "INFO: ";
      break;
    case LogLevel::WARNING:
      retStr = "WARNING: ";
      break;
    case LogLevel::ERROR:
      retStr = "ERROR: ";
      break;
    default:
      retStr = "UNKNOWN: ";
      break;
    }
    return retStr;
  }
}


BasicLogger::BasicLogger(uint8_t logEventsEnabled) : logLevels(logEventsEnabled) {
  cout << "Log START" << endl;
}


BasicLogger::~BasicLogger() {
  cout << "Log END" << endl;
}

void BasicLogger::Log(LogLevel level, std::string logString) {
  std::lock_guard<std::mutex> guard(accessMutex);

  /* log only if level in supported */
  if (static_cast<uint8_t>(level) & logLevels) {
    cout << GetLevelString(level) + logString << endl;
  }
}
