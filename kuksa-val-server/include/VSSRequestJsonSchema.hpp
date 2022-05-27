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

#ifndef __VSSREQUEST_JSON_SCHEMA_H__
#define __VSSREQUEST_JSON_SCHEMA_H__

#include <jsoncons/json.hpp>

using jsoncons::json;

namespace VSS_JSON {


static const char* SCHEMA_GET=R"(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "title": "Get Request",
    "description": "Get the value of one or more vehicle signals and data attributes",
    "type": "object",
    "required": ["action", "path", "requestId" ],
    "properties": {
        "action": {
            "enum": [ "get", "getMetaData" ],
            "description": "The identifier for the get request"
        },
        "attribute": {
            "enum": [ "targetValue", "value" ],
            "description": "The attributes to be fetched for the get request"
        },
        "path": {
            "$ref": "viss#/definitions/path"
        },
        "requestId": {
            "$ref": "viss#/definitions/requestId"
        }
    }
}
)";


static const char* SCHEMA_SET=R"(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "title": "Set Request",
    "description": "Enables the client to set one or more values once.",
    "type": "object",
    "required": ["action", "path", "requestId"],
    "properties": {
        "action": {
            "enum": [ "set" ],
            "description": "The identifier for the set request"
        },
        "attribute": {
            "enum": [ "targetValue", "value" ],
            "description": "The attributes to be fetched for the get request"
        },
        "path": {
            "$ref": "viss#/definitions/path"
        },
        "requestId": {
            "$ref": "viss#/definitions/requestId"
        }
    }
}
)";


static const char* SCHEMA_SUBSCRIBE=R"(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "title": "Subscribe Request",
    "description": "Allows the client to subscribe to time-varying signal notifications on the server.",
    "type": "object",
    "required": ["action", "path", "requestId"],
    "properties": {
        "action": {
            "enum": [ "subscribe" ],
            "description": "The identifier for the subscription request"
        },
        "path": {
            "$ref": "viss#/definitions/path"
        },
        "attribute": {
            "enum": [ "targetValue", "value" ],
            "description": "The attributes to be fetched for the get request"
        },
        "filters": {
            "$ref": "viss#/definitions/filters"
        },
        "requestId": {
            "$ref": "viss#/definitions/requestId"
        }
    }
}
)";

static const char* SCHEMA_UNSUBSCRIBE=R"(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "title": "Unsubscribe Request",
    "description": "Allows the client to unsubscribe to time-varying signal notifications on the server.",
    "type": "object",
    "required": ["action", "subscriptionId", "requestId"],
    "properties": {
        "action": {
            "enum": [ "unsubscribe" ],
            "description": "The identifier for the unsubscribe request"
        },
        "subscriptionId": {
            "$ref": "viss#/definitions/subscriptionId"
        }, 
        "requestId": {
            "$ref": "viss#/definitions/requestId"
        }
    }
}
)";

static const char* SCHEMA_UPDATE_VSS_TREE=R"(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "title": "Update tree Request",
    "description": "Enables the client to update data model",
    "type": "object",
    "required": ["action", "metadata", "requestId"],
    "properties": {
        "action": {
            "enum": [ "updateVSSTree"],
            "description": "The identifier for the set request"
        },
        "metadata": {
            "$ref": "viss#/definitions/metadata"
        },
        "requestId": {
            "$ref": "viss#/definitions/requestId"
        }
    }
}
)";


static const char* SCHEMA_UPDATE_METADATA=R"(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "title": "Update tree Request",
    "description": "Enables the client to update data model",
    "type": "object",
    "required": ["action", "path", "metadata", "requestId"],
    "properties": {
        "action": {
            "enum": [ "updateMetaData"],
            "description": "The identifier for the set request"
        },
        "path": {
            "$ref": "viss#/definitions/path"
        },
        "metadata": {
            "$ref": "viss#/definitions/metadata"
        },
        "requestId": {
            "$ref": "viss#/definitions/requestId"
        }
    }
}
)";


static const char* SCHEMA = (R"(
{
    "definitions": {
        "action": {
            "enum": [ "authorize", "getMetaData", "updateMetaData", "get", "set", "subscribe", "subscription", "unsubscribe", "unsubscribeAll"],
            "description": "The type of action requested by the client and/or delivered by the server"
        },
        "requestId": {
            "description": "Returned by the server in the response and used by the client to link the request and response messages.",
            "type": "string"
        },
        "path": {
            "description": "The path to the desired vehicle signal(s), as defined by the metadata schema.",
            "type": "string"
        },
        "value": {
            "description": "The data value returned by the server. This could either be a basic type, or a complex type comprised of nested name/value pairs in JSON format.",
            "type": ["number", "string", "array"]
        },
        "timestamp": {
            "description": "The Coordinated Universal Time (UTC) time that the server returned the response (expressed as number of milliseconds).",
            "type": "integer"
        },
        "filters": {
            "description": "May be specified in order to throttle the demands of subscriptions on the server.",
            "type": ["object", "null"],
            "properties": {
                "interval": {
                    "description": "The server is requested to provide notifications with a period equal to this field's value.",
                    "type": "integer"                   
                },
                "range": {
                    "description": "The server is requested to provide notifications only whilst a value is within a given range.",
                    "type": "object",
                    "properties":{
                      "below": {
                        "description": "The server is requested to provide notifications when the value is less than or equal to this field's value.",
                        "type": "integer"  
                      },
                      "above": {
                        "description": "The server is requested to provide notifications when the value is greater than or equal to this field's value.",
                        "type": "integer"  
                      }
                    }                   
                },
                "minChange": {
                    "description": "The subscription will provide notifications when a value has changed by the amount specified in this field.",
                    "type": "integer"                   
                }
            }
        },
        "subscriptionId":{
            "description": "Integer handle value which is used to uniquely identify the subscription.",
            "type": "string"
        },
        "metadata":{
            "description": "Metadata describing the potentially available signal tree.",
            "type": "object"
        },
        "error": {
            "description": "Server response for error cases",
            "type": "object",
            "properties": {
                "number": {
                    "description": "HTTP Status Code Number",
                    "type": "integer"                   
                },
                "reason": {
                    "description": "Pre-defined string value that can be used to distinguish between errors that have the same code",
                    "type": "string"                    
                },
                "message": {
                    "description": "Message text describing the cause in more detail",
                    "type": "string"                    
                }
            }   
        }
    }
}
)");

}  // namespace VSS_JSON

#endif
