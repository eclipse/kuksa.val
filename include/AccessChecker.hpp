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

#include "IAccessChecker.hpp"

class IAuthenticator;


class AccessChecker : public IAccessChecker {
 private:
  std::shared_ptr<IAuthenticator> tokenValidator;
  bool checkSignalAccess(const WsChannel& channel, const std::string& path, const std::string& requiredPermission);

 public:
  AccessChecker(std::shared_ptr<IAuthenticator> vdator);
  bool checkReadAccess(WsChannel &channel, const std::string &path) override;
  bool checkReadAccess(WsChannel &channel, const VSSPath &path) override;

  bool checkWriteAccess(WsChannel &channel, const std::string &path) override;
  bool checkWriteAccess(WsChannel &channel, const VSSPath &path) override;

  bool checkPathWriteAccess(WsChannel &channel, const jsoncons::json &paths) override;
};

#endif
