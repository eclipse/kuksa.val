/********************************************************************************
* Copyright (c) 2023 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License 2.0 which is available at
* http://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/

use std::{collections::HashMap, time::SystemTime};

use serde::{Deserialize, Serialize};

#[derive(Deserialize)]
#[serde(tag = "action")]
pub enum Request {
    #[serde(rename = "get")]
    Get(GetRequest),
    #[serde(rename = "set")]
    Set(SetRequest),
    #[serde(rename = "subscribe")]
    Subscribe(SubscribeRequest),
    #[serde(rename = "unsubscribe")]
    Unsubscribe(UnsubscribeRequest),
}

// Identify responses using the `Response` trait to prevent
// responding with other serializable things.
pub trait Response: Serialize {}

impl Response for GetSuccessResponse {}
impl Response for GetErrorResponse {}

impl Response for SetSuccessResponse {}
impl Response for SetErrorResponse {}

impl Response for SubscribeSuccessResponse {}
impl Response for SubscribeErrorResponse {}

impl Response for UnsubscribeSuccessResponse {}
impl Response for UnsubscribeErrorResponse {}

impl Response for SubscriptionEvent {}
impl Response for SubscriptionErrorEvent {}

impl Response for GenericErrorResponse {}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct GetRequest {
    pub path: Path,
    pub request_id: RequestId,
    pub authorization: Option<String>,
    pub filter: Option<Filter>,
}

#[derive(Serialize)]
#[serde(untagged)]
pub enum GetSuccessResponse {
    Data(DataResponse),
    Metadata(MetadataResponse),
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "get", rename_all = "camelCase")]
pub struct DataResponse {
    pub request_id: RequestId,
    pub data: Data,
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "get", rename_all = "camelCase")]
pub struct MetadataResponse {
    pub request_id: RequestId,
    pub metadata: HashMap<String, MetadataEntry>,
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "get", rename_all = "camelCase")]
pub struct GetErrorResponse {
    pub request_id: RequestId,
    pub error: Error,
    pub ts: Timestamp,
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct SetRequest {
    pub path: Path,
    pub value: Value,
    pub request_id: RequestId,
    pub authorization: Option<String>,
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "set", rename_all = "camelCase")]
pub struct SetSuccessResponse {
    pub request_id: RequestId,
    pub ts: Timestamp,
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "set", rename_all = "camelCase")]
pub struct SetErrorResponse {
    pub request_id: RequestId,
    pub error: Error,
    pub ts: Timestamp,
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct SubscribeRequest {
    pub path: Path,
    pub request_id: RequestId,
    pub authorization: Option<String>,
    // filter: Option<Filter>,
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "subscribe", rename_all = "camelCase")]
pub struct SubscribeSuccessResponse {
    pub request_id: RequestId,
    pub subscription_id: SubscriptionId,
    pub ts: Timestamp,
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "subscribe", rename_all = "camelCase")]
pub struct SubscribeErrorResponse {
    pub request_id: RequestId,
    pub error: Error,
    pub ts: Timestamp,
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "subscription", rename_all = "camelCase")]
pub struct SubscriptionEvent {
    pub subscription_id: SubscriptionId,
    pub data: Data,
    pub ts: Timestamp,
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "subscription", rename_all = "camelCase")]
pub struct SubscriptionErrorEvent {
    pub subscription_id: SubscriptionId,
    pub error: Error,
    pub ts: Timestamp,
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct UnsubscribeRequest {
    pub request_id: RequestId,
    pub subscription_id: SubscriptionId,
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "unsubscribe", rename_all = "camelCase")]
pub struct UnsubscribeSuccessResponse {
    pub request_id: RequestId,
    pub subscription_id: SubscriptionId,
    pub ts: Timestamp,
}

#[derive(Serialize)]
#[serde(tag = "action", rename = "unsubscribe", rename_all = "camelCase")]
pub struct UnsubscribeErrorResponse {
    pub request_id: RequestId,
    pub subscription_id: SubscriptionId,
    pub error: Error,
    pub ts: Timestamp,
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct GenericRequest {
    pub action: Option<String>,
    pub request_id: Option<RequestId>,
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct GenericErrorResponse {
    pub action: Option<String>,
    pub request_id: Option<RequestId>,
    pub error: Error,
}

#[derive(Deserialize)]
#[serde(tag = "type")]
pub enum Filter {
    #[serde(rename = "static-metadata")]
    StaticMetadata(StaticMetadataFilter),
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct StaticMetadataFilter {
    // pub parameters: Option<StaticMetadataParameters>,
}

// Unique id value specified by the client. Returned by the server in the
// response and used by the client to link the request and response messages.
// The value MAY be an integer or a Universally Unique Identifier (UUID).
#[derive(Serialize, Deserialize, Clone)]
#[serde(transparent)]
pub struct RequestId(String);

// The path consists a sequence of VSS node names separated by a delimiter.
// VSS specifies the dot (.) as delimiter, which therefore is the recommended
// choice also in this specification. However, in HTTP URLs the conventional
// delimiter is slash (/), therefore also this delimiter is supported.
// To exemplify, the path expression from traversing the nodes
// Vehicle, Car, Engine, RPM can be "Vehicle.Car.Engine.RPM", or
// "Vehicle/Car/Engine/RPM". A mix of delimiters in the same path expression
// SHOULD be avoided.
// The path MUST not contain any wildcard characters ("*"), for such needs
// see 7.1 Paths Filter Operation.
#[derive(Serialize, Deserialize, Clone)]
#[serde(transparent)]
pub struct Path(String);

// A handle identifying a subscription session.
#[derive(Serialize, Deserialize, Clone, Eq, Hash, PartialEq)]
#[serde(transparent)]
pub struct SubscriptionId(String);

// Timestamps in transport payloads MUST conform to the [ISO8601] standard,
// using the UTC format with a trailing Z. Time resolution SHALL at least be
// seconds, with sub-second resolution as an optional degree of precision when
// desired. The time and date format shall be as shown below, where the
// sub-second data and delimiter is optional.
//
// YYYY-MM-DDTHH:MM:SS.ssssssZ
//
pub struct Timestamp {
    ts: SystemTime,
}

#[derive(Serialize)]
#[serde(untagged)]
pub enum Data {
    Object(DataObject),
    #[allow(dead_code)]
    Array(Vec<DataObject>),
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct DataObject {
    pub path: Path,
    pub dp: DataPoint,
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct DataPoint {
    pub value: Value,
    pub ts: Timestamp,
}

#[derive(Serialize, Deserialize, Clone)]
#[serde(untagged)]
pub enum Value {
    None,
    Scalar(String),
    Array(Vec<String>),
}

#[derive(Serialize)]
#[serde(tag = "type", rename_all = "camelCase")]
pub enum MetadataEntry {
    Branch(BranchEntry),
    Sensor(SensorEntry),
    Attribute(AttributeEntry),
    Actuator(ActuatorEntry),
}

#[derive(Serialize)]
pub struct BranchEntry {
    pub description: String,
    pub children: HashMap<String, MetadataEntry>,
}

#[derive(Serialize)]
pub struct SensorEntry {
    pub datatype: DataType,
    pub description: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub comment: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub unit: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub allowed: Option<Value>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub min: Option<Value>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub max: Option<Value>,
}

#[derive(Serialize)]
pub struct AttributeEntry {
    pub datatype: DataType,
    pub description: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub unit: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub allowed: Option<Value>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub default: Option<Value>,
}

#[derive(Serialize)]
pub struct ActuatorEntry {
    pub datatype: DataType,
    pub description: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub comment: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub unit: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub allowed: Option<Value>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub min: Option<Value>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub max: Option<Value>,
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub enum DataType {
    String,
    Boolean,
    Int8,
    Int16,
    Int32,
    Int64,
    Uint8,
    Uint16,
    Uint32,
    Uint64,
    Float,
    Double,
    #[serde(rename = "string[]")]
    StringArray,
    #[serde(rename = "boolean[]")]
    BoolArray,
    #[serde(rename = "int8[]")]
    Int8Array,
    #[serde(rename = "int16[]")]
    Int16Array,
    #[serde(rename = "int32[]")]
    Int32Array,
    #[serde(rename = "int64[]")]
    Int64Array,
    #[serde(rename = "uint8[]")]
    Uint8Array,
    #[serde(rename = "uint16[]")]
    Uint16Array,
    #[serde(rename = "uint32[]")]
    Uint32Array,
    #[serde(rename = "uint64[]")]
    Uint64Array,
    #[serde(rename = "float[]")]
    FloatArray,
    #[serde(rename = "double[]")]
    DoubleArray,
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ErrorSpec {
    pub number: i32,
    pub reason: String,
    pub message: String,
}

#[derive(Serialize, Clone)]
#[serde(into = "ErrorSpec")]
#[allow(dead_code, clippy::enum_variant_names)]
pub enum Error {
    BadRequest { msg: Option<String> },
    UnauthorizedTokenExpired,
    UnauthorizedTokenInvalid,
    UnauthorizedTokenMissing,
    UnauthorizedReadOnly,
    Forbidden,
    NotFoundInvalidPath,
    NotFoundUnavailableData,
    NotFoundInvalidSubscriptionId,
    InternalServerError,
    NotImplemented,
}

impl From<Error> for ErrorSpec {
    fn from(error: Error) -> Self {
        match error {
            // Error            Number  Reason                    Message
            // NotModified         304  not_modified              No changes have been made by the server.
            // BadRequest          400  bad_request               The server is unable to fulfil the client request because the request is malformed.
            Error::BadRequest{ msg: custom_msg } => ErrorSpec {
                number: 400,
                reason: "bad_request".into(),
                message: custom_msg.unwrap_or("The server is unable to fulfil the client request because the request is malformed.".into()),
            },
            // BadRequest          400  filter_invalid            Filter requested on non-primitive type.
            // BadRequest          400  invalid_duration          Time duration is invalid.
            // BadRequest          400  invalid_value             The requested set value is invalid.
            // Unauthorized        401  token_expired             Access token has expired.
            Error::UnauthorizedTokenExpired => ErrorSpec {
                number: 401,
                reason: "token_expired".into(),
                message: "Access token has expired.".into(),
            },
            // Unauthorized        401  token_invalid             Access token is invalid.
            Error::UnauthorizedTokenInvalid => ErrorSpec {
                number: 401,
                reason: "token_invalid".into(),
                message: "Access token is invalid.".into(),
            },
            // Unauthorized        401  token_missing             Access token is missing.
            Error::UnauthorizedTokenMissing => ErrorSpec {
                number: 401,
                reason: "token_missing".into(),
                message: "Access token is missing.".into(),
            },
            // Unauthorized        401  too_many_attempts         The client has failed to authenticate too many times.
            // Unauthorized        401  read_only                 The desired signal cannot be set since it is a read only signal.
            Error::UnauthorizedReadOnly => ErrorSpec {
                number: 401,
                reason: "read_only".into(),
                message: "The desired signal cannot be set since it is a read only signal.".into(),
            },
            // Forbidden           403  user_forbidden            The user is not permitted to access the requested resource. Retrying does not help.
            Error::Forbidden => ErrorSpec {
                number: 403,
                reason: "user_forbidden".into(),
                message: "The user is not permitted to access the requested resource. Retrying does not help.".into(),
            },
            // Forbidden           403  user_unknown              The user is unknown. Retrying does not help.
            // Forbidden           403  device_forbidden          The device is not permitted to access the requested resource. Retrying does not help.
            // Forbidden           403  device_unknown            The device is unknown. Retrying does not help.
            // NotFound            404  invalid_path              The specified data path does not exist.
            Error::NotFoundInvalidPath => ErrorSpec {
                number: 404,
                reason: "invalid_path".into(),
                message: "The specified data path does not exist.".into(),
            },
            // NotFound            404  private_path              The specified data path is private and the request is not authorized to access signals on this path.
            // NotFound            404  unavailable_data          The requested data was not found.
            Error::NotFoundUnavailableData => ErrorSpec {
                number: 404,
                reason: "unavailable_data".into(),
                message: "The requested data was not found.".into(),
            },
            // NotFound            404  invalid_subscription_id   The specified subscription was not found.
            Error::NotFoundInvalidSubscriptionId => ErrorSpec {
                number: 404,
                reason: "invalid_subscription_id".into(),
                message: "The specified subscription was not found.".into(),
            },
            // NotAcceptable       406  insufficient_privileges   The privileges represented by the access token are not sufficient.
            // NotAcceptable       406  not_acceptable            The server is unable to generate content that is acceptable to the client
            // TooManyRequests     429  too_many_requests         The client has sent the server too many requests in a given amount of time.
            // InternalServerError 500  internal_server_error     The server encountered an unexpected condition which prevented it from fulfilling the request.
            Error::InternalServerError => ErrorSpec {
                number: 500,
                reason: "internal_server_error".into(),
                message: "The server encountered an unexpected condition which prevented it from fulfilling the request.".into(),
            },
            // NotImplemented      501  not_implemented           The server does not support the functionality required to fulfill the request.
            Error::NotImplemented => ErrorSpec {
                number: 501,
                reason: "not_implemented".into(),
                message: "The server does not support the functionality required to fulfill the request.".into(),
            },
            // BadGateway          502  bad_gateway               The server was acting as a gateway or proxy and received an invalid response from an upstream server.
            // ServiceUnavailable  503  service_unavailable       The server is currently unable to handle the request due to a temporary overload or scheduled maintenance (which may be alleviated after some delay).
            // GatewayTimeout      504  gateway_timeout           The server did not receive a timely response from an upstream server it needed to access in order to complete the request.
        }
    }
}

impl AsRef<str> for Path {
    fn as_ref(&self) -> &str {
        &self.0
    }
}

impl From<Path> for String {
    fn from(value: Path) -> Self {
        value.0
    }
}

impl From<String> for Path {
    fn from(value: String) -> Self {
        Path(value)
    }
}

impl From<String> for SubscriptionId {
    fn from(value: String) -> Self {
        SubscriptionId(value)
    }
}

impl From<SubscriptionId> for String {
    fn from(value: SubscriptionId) -> Self {
        value.0
    }
}

impl AsRef<str> for RequestId {
    fn as_ref(&self) -> &str {
        &self.0
    }
}

impl From<SystemTime> for Timestamp {
    fn from(ts: SystemTime) -> Self {
        Timestamp { ts }
    }
}

impl Serialize for Timestamp {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        let utc_time: chrono::DateTime<chrono::Utc> = self.ts.into();
        let use_z = true;
        let serialized = utc_time.to_rfc3339_opts(chrono::SecondsFormat::Millis, use_z);
        serializer.serialize_str(&serialized)
    }
}

impl SubscriptionId {
    pub fn new() -> Self {
        Self(uuid::Uuid::new_v4().to_string())
    }
}

impl Default for SubscriptionId {
    fn default() -> Self {
        Self::new()
    }
}
