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


#include <stdio.h>
#include <string>
#include "wschannel.hpp"
#include "authenticator.hpp"

using namespace std;

class accesschecker {

  private:
    class  authenticator* tokenValidator;

  public:  
     accesschecker(class  authenticator* vdator);
     bool checkAccess (class wschannel& channel, string path); 

};



#endif
