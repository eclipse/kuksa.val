/**********************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
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
 *      Robert Bosch GmbH - initial API and functionality
 **********************************************************************/


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
