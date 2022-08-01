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
        void validateSubscribe(jsoncons::json &request);
        void validateUnsubscribe(jsoncons::json &request);

        void validateUpdateMetadata(jsoncons::json &request);
        void validateUpdateVSSTree(jsoncons::json &request);

        std::string tryExtractRequestId(jsoncons::json &request);

    private:
        class MessageValidator;
        std::unique_ptr<VSSRequestValidator::MessageValidator> getValidator;
        std::unique_ptr<VSSRequestValidator::MessageValidator> setValidator;
        std::unique_ptr<VSSRequestValidator::MessageValidator> subscribeValidator;
        std::unique_ptr<VSSRequestValidator::MessageValidator> unsubscribeValidator;
        std::unique_ptr<VSSRequestValidator::MessageValidator> updateMetadataValidator;
        std::unique_ptr<VSSRequestValidator::MessageValidator> updateVSSTreeValidator;

        std::shared_ptr<ILogger> logger;
};

#endif
