/*
 * ******************************************************************************
 * Copyright (c) 2020 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH
 * *****************************************************************************
 */

#include "VSSPath.hpp"

#include <algorithm>
#include <regex>
#include <sstream>
#include <vector>

#include <boost/algorithm/string.hpp>

VSSPath::VSSPath(std::string vss, std::string vssgen1, std::string jsonpath, bool gen1origin)
    : vsspath(vss), vssgen1path(vssgen1), jsonpath(jsonpath), gen1(gen1origin) {}

std::string VSSPath::getVSSPath() { return this->vsspath; }

std::string VSSPath::getVSSGen1Path() { return this->vssgen1path; }

std::string VSSPath::getJSONPath() { return this->jsonpath; }

bool VSSPath::isGen1Origin() { return this->gen1;}

VSSPath VSSPath::fromVSSAuto(std::string input) {
  bool gen1=false;
  std::string vss, vssgen1, jsonpath;
  if (input.find(".") == std::string::npos) {  // If no "." in we assume a GEN2 "/" seperated path
    vss = input;
    vssgen1 = VSSPath::gen2togen1(vss);
  } else {
    vssgen1 = input;
    gen1=true;
    vss = VSSPath::gen1togen2(vssgen1);
  }

  jsonpath = VSSPath::gen2tojson(vss);
  return VSSPath(vss, vssgen1, jsonpath,gen1);
}

std::string VSSPath::gen1togen2(std::string input) {
  std::string gen2 = input;
  std::replace(gen2.begin(), gen2.end(), '.', '/');
  return gen2;
}

std::string VSSPath::gen2togen1(std::string input) {
  std::string gen1 = input;
  std::replace(gen1.begin(), gen1.end(), '/', '.');
  return gen1;
}

std::string VSSPath::gen2tojson(std::string input) {
  std::vector<std::string> elements;
  boost::split(elements, input, boost::is_any_of("/"));

  std::stringstream jsonpath;
  jsonpath << "$";
  bool first = true;
  for (auto element : elements) {
    if (!first) {
      jsonpath << "[\'children\']";
    }
    if (element == "*") {
      jsonpath << "[*]";
    } else {
      jsonpath << "[\'" << element << "\']";
    }
    first = false;
  }
  return jsonpath.str();
}

std::string VSSPath::jsontogen2(std::string input) {
    std::string gen2=input;
    boost::erase_all(gen2,"['children']");
    boost::erase_all(gen2,"'");

    boost::replace_all(gen2, "][", "/");
    boost::erase_first(gen2,"$[");
    boost::erase_last(gen2,"]");
    return gen2;
}

VSSPath VSSPath::fromVSS(std::string input) {
  return VSSPath(input, gen2togen1(input), gen2tojson(input),false);
}

VSSPath VSSPath::fromVSSGen1(std::string input) {
  std::string vss = gen1togen2(input);
  return VSSPath(vss, input, gen2tojson(vss),true);
}

VSSPath VSSPath::fromJSON(std::string input) {
  std::string vss = VSSPath::jsontogen2(input);
  return VSSPath(vss, VSSPath::gen2togen1(vss), input,false);
}
