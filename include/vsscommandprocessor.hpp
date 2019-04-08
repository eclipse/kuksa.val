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
#include "visconf.hpp"
using namespace std;
using namespace jsoncons;
using namespace jsoncons::jsonpath;
using jsoncons::json;


class vsscommandprocessor {

private:
   class  vssdatabase* database = NULL;
   class  subscriptionhandler* subHandler = NULL;
   class  authenticator* tokenValidator = NULL;
   class  accesschecker* accessValidator = NULL;
#ifdef JSON_SIGNING_ON
   class signing* signer = NULL;
#endif

   string processGet(class wschannel& channel,uint32_t request_id, string path);
   string processSet(class wschannel& channel,uint32_t request_id, string path, json value);
   string processSubscribe(class wschannel& channel, uint32_t request_id, string path, uint32_t connectionID);
   string processUnsubscribe(uint32_t request_id, uint32_t subscribeID);
   string processGetMetaData(uint32_t request_id, string path); 
   string processAuthorize (class wschannel& channel,uint32_t request_id, string token);

public:
   vsscommandprocessor(class vssdatabase* database, class  authenticator* vdator , class subscriptionhandler* subhandler);
   string processQuery(string req_json , class wschannel& channel);
};


#endif
