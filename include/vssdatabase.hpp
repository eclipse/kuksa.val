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
#include <stdint.h>
#include <fstream>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>
#include <list>
#include <string>
#include <vector>
#include "authenticator.hpp"
#include "subscriptionhandler.hpp"
#include "wschannel.hpp"

#ifdef UNIT_TEST
#include "w3cunittest.hpp"
#endif

using namespace std;
using namespace jsoncons;
using namespace jsoncons::jsonpath;
using jsoncons::json;

class vssdatabase {
  friend class subscriptionhandler;
  friend class authenticator;
#ifdef UNIT_TEST
  friend class w3cunittest;
#endif

 private:
  pthread_mutex_t* rwMutex;
  json data_tree;
  json meta_tree;
  class subscriptionhandler* subHandler;
  class accesschecker* accessValidator;
  string getVSSSpecificPath(string path, bool& isBranch, json& tree);
  string getPathForMetadata(string path, bool& isBranch);
  list<string> getPathForGet(string path, bool& isBranch);
  json getPathForSet(string path, json value);
  string getReadablePath(string jsonpath);

 public:
  vssdatabase(class subscriptionhandler* subHandle,
              class accesschecker* accValidator);
  void initJsonTree();
  json getMetaData(string path);
  void setSignal(class wschannel& channel, string path, json value);
  json getSignal(class wschannel& channel, string path);
};
#endif
