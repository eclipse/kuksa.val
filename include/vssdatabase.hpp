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
#include <list>
#include <stdint.h>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>
#include "subscriptionhandler.hpp"

#ifdef UNIT_TEST
#include "w3cunittest.hpp"
#endif

using namespace std;
using namespace jsoncons;
using namespace jsoncons::jsonpath;
using jsoncons::json;

class vssdatabase {

friend class subscriptionhandler;

#ifdef UNIT_TEST
     friend class w3cunittest;
#endif

 private:
  pthread_mutex_t* rwMutex;
  json data_tree;
  json meta_tree;
  class subscriptionhandler* subHandler;
  string getVSSSpecificPath (string path, bool &isBranch, json& tree);
  string getPathForMetadata(string path , bool &isBranch);
  list<string>getPathForGet(string path , bool &isBranch);
  json getPathForSet(string path,  json value);
 public:
  vssdatabase(class subscriptionhandler* subHandle);
  void initJsonTree();
  json getMetaData(string path);
  void setSignal(string path, json value);
  json getSignal(string path); 
};
#endif
