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

using namespace std;

class WsChannel;
class IVssDatabase;
class ILogger;

class IAuthenticator {
public:
  virtual ~IAuthenticator() {}

  virtual int validate(WsChannel &channel,
                       std::shared_ptr<IVssDatabase> database,
                       string authToken) = 0;
  virtual void updatePubKey(string key) = 0;
  virtual bool isStillValid(WsChannel &channel) = 0;
  virtual void resolvePermissions(WsChannel &channel, std::shared_ptr<IVssDatabase> database) = 0;
};

#endif
