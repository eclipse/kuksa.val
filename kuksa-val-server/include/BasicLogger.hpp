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


#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "ILogger.hpp"

#include <mutex>


/**
 * \class Logger
 * \brief Implementation class of logging utility
 *
 * Logger shall provide standardized way to put logging information
 * to standard output (stdout)
 *
 */
class BasicLogger : public ILogger {
  private:
    std::mutex    accessMutex;
    const uint8_t logLevels;

  public:
    BasicLogger(uint8_t logEventsEnabled);
    ~BasicLogger();

    void Log(LogLevel level, std::string logString);
};

#endif /* __LOGGER_H__ */
