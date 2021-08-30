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
#ifndef __IAUTHENTICATOR_H__
#define __IAUTHENTICATOR_H__

#include <memory>
#include <string>

#include "kuksa.pb.h"

using namespace std;

class ILogger;

class IAuthenticator {
public:
  virtual ~IAuthenticator() {}

  virtual int validate(kuksa::kuksaChannel &channel,
                       string authToken) = 0;
  virtual void updatePubKey(string key) = 0;
  virtual bool isStillValid(kuksa::kuksaChannel &channel) = 0;
  virtual void resolvePermissions(kuksa::kuksaChannel &channel) = 0;
};

#endif
