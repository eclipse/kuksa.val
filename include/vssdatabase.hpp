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
#ifndef __VSSDATABASE_HPP__
#define __VSSDATABASE_HPP__
#include <string>
#include <fstream>
#include <vector>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>

using namespace std;
using namespace jsoncons;
using namespace jsoncons::jsonpath;

void initJsonTree();
json getMetaData(string path);
int setSignal(string path, void* value);
json getSignal(string path);
int jAddSubscription (string path , uint32_t subId);
void jSetSubscribeCallback(void(*sendMessageToClient)(std::string, uint32_t));


#endif
