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


/** This will validate VSS requests in one place, giving some basic guarantees 
 *  regarding request validity such as mandatory fields and required datatypes*
 */
  
#include "VSSRequestValidator.hpp"
#include "VSSRequestJsonSchema.hpp"

#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <string>


json local = json::parse(R"(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "title": "Get Request",
    "description": "Get the value of one or more vehicle signals and data attributes",
    "type": "object",
    "required": ["action", "path", "requestId" ],
    "properties": {
        "action": {
            "enum": [ "get" ],
            "description": "The identifier for the get request"
        },
        "path": {
             "$ref": "viss#/definitions/path"
        },
        "requestId": {
            "$ref": "viss#/definitions/requestId"
        }
    }
}
)");


json vss_resolver(const jsoncons::uri& uri) {
    if ( std::string(uri.path()) == std::string("/viss" )) {
        return vss_schema;
    }
    else {
        throw  jsonschema::schema_error("Could not open " + std::string(uri.base()) + " for schema loading\n");
    }
}


VSSRequestValidator::VSSRequestValidator() {
    std::string s;
    local.dump(s);
    this->getSchema = jsoncons::jsonschema::make_schema(local, vss_resolver);
    this->getValidator =  new jsonschema::json_validator<json>(this->getSchema);
}

VSSRequestValidator::~VSSRequestValidator() {
    delete this->getValidator;
}


void VSSRequestValidator::validateGet(jsoncons::json &request) {
    std::stringstream ss;
    bool valid=true;
    auto reporter = [&ss,&valid](const jsonschema::validation_output& o)
        {
            valid=false;
            ss << o.instance_location() << ": " << o.message() << "\n";
    };

    this->getValidator->validate(request, reporter);

    if (!valid) {
        throw jsonschema::schema_error("VSS get malformed: "+ss.str());
    }
    return;
}