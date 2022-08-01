/**********************************************************************
 * Copyright (c) 2020-2021 Robert Bosch GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Contributors:
 *      Robert Bosch GmbH
 **********************************************************************/


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
    std::unique_ptr<jsoncons::jsonschema::json_validator<jsoncons::json>> validator;

  public: 
    MessageValidator(const char* SCHEMA) {
        this->schema = jsoncons::jsonschema::make_schema(jsoncons::json::parse(SCHEMA), vss_resolver);
        this->validator = std::make_unique<jsoncons::jsonschema::json_validator<jsoncons::json>>(this->schema);
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


VSSRequestValidator::VSSRequestValidator(std::shared_ptr<ILogger> loggerUtil) : 
  logger(loggerUtil) {
  
  this->getValidator            = std::make_unique<MessageValidator>( VSS_JSON::SCHEMA_GET);
  this->setValidator            = std::make_unique<MessageValidator>( VSS_JSON::SCHEMA_SET);
  this->subscribeValidator      = std::make_unique<MessageValidator>( VSS_JSON::SCHEMA_SUBSCRIBE);
  this->unsubscribeValidator    = std::make_unique<MessageValidator>( VSS_JSON::SCHEMA_UNSUBSCRIBE);

  this->updateMetadataValidator = std::make_unique<MessageValidator>( VSS_JSON::SCHEMA_UPDATE_METADATA);
  this->updateVSSTreeValidator  = std::make_unique<MessageValidator>( VSS_JSON::SCHEMA_UPDATE_VSS_TREE);
}

VSSRequestValidator::~VSSRequestValidator() {  
}

void VSSRequestValidator::validateGet(jsoncons::json& request) {
  getValidator->validate(request);
}

void VSSRequestValidator::validateSet(jsoncons::json& request) {
  setValidator->validate(request);
}

void VSSRequestValidator::validateSubscribe(jsoncons::json& request) {
  subscribeValidator->validate(request);
}

void VSSRequestValidator::validateUnsubscribe(jsoncons::json& request) {
  unsubscribeValidator->validate(request);
}

void VSSRequestValidator::validateUpdateMetadata(jsoncons::json& request) {
  updateMetadataValidator->validate(request);
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
