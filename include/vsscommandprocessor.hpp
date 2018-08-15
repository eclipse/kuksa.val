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
#ifndef __VSSCOMMANDPROCESSOR_H__
#define __VSSCOMMANDPROCESSOR_H__

#include <string>
#include "wschannel.hpp"
#include "vssdatabase.hpp"
#include "subscriptionhandler.hpp"
#include "accesschecker.hpp"
using namespace std;

class vsscommandprocessor {

private:
   class  vssdatabase* database = NULL;
   class  subscriptionhandler* subHandler = NULL;
   class  authenticator* tokenValidator = NULL;
   class  accesschecker* accessValidator = NULL;

   string processGet(uint32_t request_id, string path);
   string processSet(uint32_t request_id, string path, string value);
   string processSubscribe(uint32_t request_id, string path, uint32_t connectionID);
   string processUnsubscribe(uint32_t request_id, uint32_t subscribeID);
   string processGetMetaData(uint32_t request_id, string path); 
   string processAuthorize (uint32_t request_id, string token ,class wschannel& channel);

public:
   vsscommandprocessor(class vssdatabase* database, class  authenticator* vdator , class subscriptionhandler* subhandler);
   string processQuery(string req_json , class wschannel& channel);
};


#endif
