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

#include <string>
#include <jsoncons/json.hpp>
#include "wschannel.hpp"
#include "authenticator.hpp"

using namespace std;
using namespace jsoncons;
using jsoncons::json;

class accesschecker {

  private:
    class  authenticator* tokenValidator;

  public:  
     accesschecker(class  authenticator* vdator);
     bool checkReadAccess (class wschannel& channel, string path);
     bool checkWriteAccess (class wschannel& channel, string path);
     bool checkPathWriteAccess (class wschannel& channel, json paths);  

};



#endif
