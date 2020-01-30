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
