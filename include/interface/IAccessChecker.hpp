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
#ifndef __IACCESSCHECKER_H__
#define __IACCESSCHECKER_H__

#include <string>
#include <jsoncons/json.hpp>

#include "VSSPath.hpp"

class WsChannel;

class IAccessChecker {
public:
  virtual ~IAccessChecker() {}

  virtual bool checkPathWriteAccess(WsChannel &channel, const jsoncons::json &paths) = 0;
  virtual bool checkReadAccess(WsChannel &channel, const VSSPath &path) = 0;
  virtual bool checkWriteAccess(WsChannel &channel, const VSSPath &path) = 0;


};

#endif
