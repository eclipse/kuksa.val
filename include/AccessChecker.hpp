/*
 * ******************************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
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
#ifndef __ACCESSCHECKER_H__
#define __ACCESSCHECKER_H__

#include <jsoncons/json.hpp>
#include <string>
#include "Authenticator.hpp"
#include "wschannel.hpp"

using namespace std;
using namespace jsoncons;
using jsoncons::json;

class AccessChecker {
 private:
  Authenticator *tokenValidator;

 public:
  AccessChecker(Authenticator *vdator);
  bool checkReadAccess(wschannel &channel, string path);
  bool checkWriteAccess(wschannel &channel, string path);
  bool checkPathWriteAccess(wschannel &channel, json paths);
};

#endif
