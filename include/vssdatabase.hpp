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
#include <list>
#include <mutex>

#include <jsoncons/json.hpp>

class subscriptionhandler;
class accesschecker;
class wschannel;

class vssdatabase {
  friend class subscriptionhandler;
  friend class authenticator;
#ifdef UNIT_TEST
  friend class w3cunittest;
#endif

 private:
  std::mutex rwMutex;
  jsoncons::json data_tree;
  jsoncons::json meta_tree;
  subscriptionhandler* subHandler;
  accesschecker* accessValidator;
  std::string getVSSSpecificPath(std::string path, bool& isBranch, jsoncons::json& tree);
  std::string getPathForMetadata(std::string path, bool& isBranch);
  std::list<std::string> getPathForGet(std::string path, bool& isBranch);
  jsoncons::json getPathForSet(std::string path, jsoncons::json value);
  std::string getReadablePath(std::string jsonpath);

 public:
  vssdatabase(subscriptionhandler* subHandle,
              accesschecker* accValidator);
  ~vssdatabase();
  void initJsonTree();
  jsoncons::json getMetaData(std::string path);
  void setSignal(wschannel& channel, std::string path, jsoncons::json value);
  jsoncons::json getSignal(wschannel& channel, std::string path);
};
#endif
