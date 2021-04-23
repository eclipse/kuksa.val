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

#include "VSSRequestJsonSchema.hpp"  
#include "VSSRequestValidator.hpp"


#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <string>


json vss_resolver(const jsoncons::uri& uri) {
    if ( std::string(uri.path()) == std::string("/viss" )) {
        return json::parse(VSS_JSON::SCHEMA);
    }
    else {
        throw  jsoncons::jsonschema::schema_error("Could not open " + std::string(uri.base()) + " for schema loading\n");
    }
}


VSSRequestValidator::VSSRequestValidator(std::shared_ptr<ILogger> loggerUtil) {
    this->logger=loggerUtil;  

    this->getSchema = jsoncons::jsonschema::make_schema(json::parse(VSS_JSON::SCHEMA_GET), vss_resolver);
    this->getValidator =  new jsoncons::jsonschema::json_validator<json>(this->getSchema);

    this->setSchema = jsoncons::jsonschema::make_schema(json::parse(VSS_JSON::SCHEMA_SET), vss_resolver);
    this->setValidator =  new jsoncons::jsonschema::json_validator<json>(this->setSchema);

    this->updateTreeSchema = jsoncons::jsonschema::make_schema(json::parse(VSS_JSON::SCHEMA_UPDATE_TREE), vss_resolver);
    this->updateTreeValidator =  new jsoncons::jsonschema::json_validator<json>(this->updateTreeSchema);
}

VSSRequestValidator::~VSSRequestValidator() {
    delete this->getValidator;
}


void VSSRequestValidator::validateGet(jsoncons::json &request) {
    std::stringstream ss;
    bool valid=true;
    auto reporter = [&ss,&valid](const jsoncons::jsonschema::validation_output& o)
        {
            valid=false;
            ss << o.instance_location() << ": " << o.message() << "\n";
    };

    this->getValidator->validate(request, reporter);

    if (!valid) {
        throw jsoncons::jsonschema::schema_error("VSS get malformed: "+ss.str());
    }
    return;
}

void VSSRequestValidator::validateSet(jsoncons::json &request) {
    std::stringstream ss;
    bool valid=true;
    auto reporter = [&ss,&valid](const jsoncons::jsonschema::validation_output& o)
        {
            valid=false;
            ss << o.instance_location() << ": " << o.message() << "\n";
    };

    this->setValidator->validate(request, reporter);

    if (!valid) {
        throw jsoncons::jsonschema::schema_error("VSS set malformed: "+ss.str());
    }
}

void VSSRequestValidator::validateUpdateTree(jsoncons::json &request) {
    std::stringstream ss;
    bool valid=true;
    auto reporter = [&ss,&valid](const jsoncons::jsonschema::validation_output& o)
        {
            valid=false;
            ss << o.instance_location() << ": " << o.message() << "\n";
    };

    this->setValidator->validate(request, reporter);

    if (!valid) {
        throw jsoncons::jsonschema::schema_error("VSS update tree malformed: "+ss.str());
    }
}

/* When JSON schema validation fails, we can not be sure if the request contains a request_id 
 * This hleper tries to extract one in a save way (Helpful in error response), or 
 * returning "Unknown" if none is present
 * */
std::string VSSRequestValidator::tryExtractRequestId(jsoncons::json &request) {
    return request.get_value_or<std::string>("requestId", "UNKNOWN");;
}
