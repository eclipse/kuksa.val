/*
 * ******************************************************************************
 * Copyright (c) 2020-2021 Robert Bosch GmbH.
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

/** This will validate VSS requests in one place, giving some basic guarantees
 *  regarding request validity such as mandatory fields and required datatypes
 */

#include "VSSRequestValidator.hpp"

#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <string>

#include "VSSRequestJsonSchema.hpp"


class VSSRequestValidator::MessageValidator {
  private:
    std::shared_ptr<jsoncons::jsonschema::json_schema<jsoncons::json> > schema;
    jsoncons::jsonschema::json_validator<jsoncons::json>* validator;

  public: 
    MessageValidator(const char* SCHEMA) {
        this->schema = jsoncons::jsonschema::make_schema(jsoncons::json::parse(SCHEMA), vss_resolver);
        this->validator = new jsoncons::jsonschema::json_validator<jsoncons::json>(this->schema);
    }

  void validate(jsoncons::json& request) {
    std::stringstream ss;
    bool valid = true;
    auto reporter = [&ss, &valid](const jsoncons::jsonschema::validation_output& o) {
      valid = false;
      ss << o.instance_location() << ": " << o.message() << "\n";
    };
    this->validator->validate(request, reporter);
    if (!valid) {
      throw jsoncons::jsonschema::schema_error(ss.str());
    }
    return;
  }

  static jsoncons::json vss_resolver(const jsoncons::uri& uri) {
    if (std::string(uri.path()) == std::string("/viss")) {
      return jsoncons::json::parse(VSS_JSON::SCHEMA);
    } else {
      throw jsoncons::jsonschema::schema_error("Could not open " + std::string(uri.base()) + " for schema loading\n");
    }
  }
  
};


VSSRequestValidator::VSSRequestValidator(std::shared_ptr<ILogger> loggerUtil)  {
  this->logger = loggerUtil;

  this->getValidator           = new MessageValidator(VSS_JSON::SCHEMA_GET);
  this->setValidator           = new MessageValidator(VSS_JSON::SCHEMA_SET);
  this->updateTreeValidator    = new MessageValidator(VSS_JSON::SCHEMA_UPDATE_TREE);
  this->updateVSSTreeValidator = new MessageValidator(VSS_JSON::SCHEMA_UPDATE_VSS_TREE);
}

VSSRequestValidator::~VSSRequestValidator() {  
    delete getValidator;
    delete setValidator;
    delete updateTreeValidator;
    delete updateVSSTreeValidator;
}

void VSSRequestValidator::validateGet(jsoncons::json& request) {
  getValidator->validate(request);
}

void VSSRequestValidator::validateSet(jsoncons::json& request) {
  setValidator->validate(request);
}

void VSSRequestValidator::validateUpdateTree(jsoncons::json& request) {
  updateTreeValidator->validate(request);
}

void VSSRequestValidator::validateUpdateVSSTree(jsoncons::json& request) {
 updateVSSTreeValidator->validate(request);
}

/* When JSON schema validation fails, we can not be sure if the request contains
 * a request_id This hleper tries to extract one in a save way (Helpful in error
 * response), or returning "Unknown" if none is present
 * */
std::string VSSRequestValidator::tryExtractRequestId(jsoncons::json& request) {
  return request.get_value_or<std::string>("requestId", "UNKNOWN");
  ;
}
