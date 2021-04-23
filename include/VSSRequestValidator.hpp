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
  
#ifndef __VSSREQUESTVALIDATOR_H__
#define __VSSREQUESTVALIDATOR_H__



#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <string>

#include "ILogger.hpp"




class VSSRequestValidator {
    public:
        VSSRequestValidator(std::shared_ptr<ILogger> loggerUtil);        
        ~VSSRequestValidator();

        void validateGet(jsoncons::json &request);
        void validateSet(jsoncons::json &request);
        void validateUpdateTree(jsoncons::json &request);
        std::string tryExtractRequestId(jsoncons::json &request);

    private:
        std::shared_ptr<jsoncons::jsonschema::json_schema<jsoncons::json> > getSchema;
        std::shared_ptr<jsoncons::jsonschema::json_schema<jsoncons::json> > setSchema;
        std::shared_ptr<jsoncons::jsonschema::json_schema<jsoncons::json> > updateTreeSchema;
        jsoncons::jsonschema::json_validator<jsoncons::json> *getValidator;
        jsoncons::jsonschema::json_validator<jsoncons::json> *setValidator;
        jsoncons::jsonschema::json_validator<jsoncons::json> *updateTreeValidator;
        std::shared_ptr<ILogger> logger;
};

#endif
