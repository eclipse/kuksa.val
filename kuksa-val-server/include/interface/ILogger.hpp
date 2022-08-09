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
 **********************************************************************/

#ifndef __ILOGGER_H__
#define __ILOGGER_H__

#include <cstdint>
#include <string>

/**
 * \class LogLevel
 * \brief Supported logging levels
 *
 * Enumeration shall be used by implementations of \ref ILogger class to define
 * which events to log.
 * As enumeration is bitwise compatible, implementations can allow for combining
 * different log levels (e.g. INFO & ERROR, other levels shall be ignored)
 */
enum class LogLevel : uint8_t {
  NONE    = 0x0, //!< No logging allowed
  VERBOSE = 0x1, //!< Used for logging verbose or debug information, not relevant in normal execution
  INFO    = 0x2, //!< Used for logging e.g. positive information relevant to execution
  WARNING = 0x4, //!< Used for logging warning states and information that is not critical
  ERROR   = 0x8, //!< Used for logging any error or fail information
  ALL     = 0xF  //!< Used for selecting all log levels
};


/**
 * \class ILogger
 * \brief Interface definition for logging utility
 *
 * Interface for logger allows to easily add support for logging through different
 * means or requirements (e.g. resource scarce platforms, socket, serial, ...)
 *
 */
class ILogger {
  public:
    virtual ~ILogger() {}

    /**
     * \brief Log message of specified notification level
     *
     * \param[in] Level of importance of log message
     * \param[in] Log message
     */
    virtual void Log(LogLevel, std::string) = 0;

};

#endif /* __ILOGGER_H__ */
