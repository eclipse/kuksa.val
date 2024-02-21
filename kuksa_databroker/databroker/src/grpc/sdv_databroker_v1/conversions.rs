/********************************************************************************
* Copyright (c) 2022 Contributors to the Eclipse Foundation
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

use databroker_proto::sdv::databroker::v1 as proto;

use prost_types::Timestamp;
use std::convert::TryInto;
use std::time::SystemTime;

use crate::broker;

impl From<&proto::Datapoint> for broker::Datapoint {
    fn from(datapoint: &proto::Datapoint) -> Self {
        let value = broker::DataValue::from(datapoint);
        let ts = SystemTime::now();

        match &datapoint.timestamp {
            Some(source_timestamp) => {
                let source: Option<SystemTime> = match source_timestamp.clone().try_into() {
                    Ok(source) => Some(source),
                    Err(_) => None,
                };
                broker::Datapoint {
                    ts,
                    source_ts: source,
                    value,
                }
            }
            None => broker::Datapoint {
                ts,
                source_ts: None,
                value,
            },
        }
    }
}

impl From<&broker::Datapoint> for proto::Datapoint {
    fn from(datapoint: &broker::Datapoint) -> Self {
        let value = match &datapoint.value {
            broker::DataValue::Bool(value) => proto::datapoint::Value::BoolValue(*value),
            broker::DataValue::String(value) => {
                proto::datapoint::Value::StringValue(value.to_owned())
            }
            broker::DataValue::Int32(value) => proto::datapoint::Value::Int32Value(*value),
            broker::DataValue::Int64(value) => proto::datapoint::Value::Int64Value(*value),
            broker::DataValue::Uint32(value) => proto::datapoint::Value::Uint32Value(*value),
            broker::DataValue::Uint64(value) => proto::datapoint::Value::Uint64Value(*value),
            broker::DataValue::Float(value) => proto::datapoint::Value::FloatValue(*value),
            broker::DataValue::Double(value) => proto::datapoint::Value::DoubleValue(*value),
            broker::DataValue::BoolArray(array) => {
                proto::datapoint::Value::BoolArray(proto::BoolArray {
                    values: array.clone(),
                })
            }
            broker::DataValue::StringArray(array) => {
                proto::datapoint::Value::StringArray(proto::StringArray {
                    values: array.clone(),
                })
            }
            broker::DataValue::Int32Array(array) => {
                proto::datapoint::Value::Int32Array(proto::Int32Array {
                    values: array.clone(),
                })
            }
            broker::DataValue::Int64Array(array) => {
                proto::datapoint::Value::Int64Array(proto::Int64Array {
                    values: array.clone(),
                })
            }
            broker::DataValue::Uint32Array(array) => {
                proto::datapoint::Value::Uint32Array(proto::Uint32Array {
                    values: array.clone(),
                })
            }
            broker::DataValue::Uint64Array(array) => {
                proto::datapoint::Value::Uint64Array(proto::Uint64Array {
                    values: array.clone(),
                })
            }
            broker::DataValue::FloatArray(array) => {
                proto::datapoint::Value::FloatArray(proto::FloatArray {
                    values: array.clone(),
                })
            }
            broker::DataValue::DoubleArray(array) => {
                proto::datapoint::Value::DoubleArray(proto::DoubleArray {
                    values: array.clone(),
                })
            }
            broker::DataValue::NotAvailable => proto::datapoint::Value::FailureValue(
                proto::datapoint::Failure::NotAvailable as i32,
            ),
        };

        proto::Datapoint {
            timestamp: Some(datapoint.ts.into()),
            value: Some(value),
        }
    }
}

impl From<&broker::QueryField> for proto::Datapoint {
    fn from(query_field: &broker::QueryField) -> Self {
        let value = match &query_field.value {
            broker::DataValue::Bool(value) => proto::datapoint::Value::BoolValue(*value),
            broker::DataValue::String(value) => {
                proto::datapoint::Value::StringValue(value.to_owned())
            }
            broker::DataValue::Int32(value) => proto::datapoint::Value::Int32Value(*value),
            broker::DataValue::Int64(value) => proto::datapoint::Value::Int64Value(*value),
            broker::DataValue::Uint32(value) => proto::datapoint::Value::Uint32Value(*value),
            broker::DataValue::Uint64(value) => proto::datapoint::Value::Uint64Value(*value),
            broker::DataValue::Float(value) => proto::datapoint::Value::FloatValue(*value),
            broker::DataValue::Double(value) => proto::datapoint::Value::DoubleValue(*value),
            broker::DataValue::BoolArray(array) => {
                proto::datapoint::Value::BoolArray(proto::BoolArray {
                    values: array.clone(),
                })
            }
            broker::DataValue::StringArray(array) => {
                proto::datapoint::Value::StringArray(proto::StringArray {
                    values: array.clone(),
                })
            }
            broker::DataValue::Int32Array(array) => {
                proto::datapoint::Value::Int32Array(proto::Int32Array {
                    values: array.clone(),
                })
            }
            broker::DataValue::Int64Array(array) => {
                proto::datapoint::Value::Int64Array(proto::Int64Array {
                    values: array.clone(),
                })
            }
            broker::DataValue::Uint32Array(array) => {
                proto::datapoint::Value::Uint32Array(proto::Uint32Array {
                    values: array.clone(),
                })
            }
            broker::DataValue::Uint64Array(array) => {
                proto::datapoint::Value::Uint64Array(proto::Uint64Array {
                    values: array.clone(),
                })
            }
            broker::DataValue::FloatArray(array) => {
                proto::datapoint::Value::FloatArray(proto::FloatArray {
                    values: array.clone(),
                })
            }
            broker::DataValue::DoubleArray(array) => {
                proto::datapoint::Value::DoubleArray(proto::DoubleArray {
                    values: array.clone(),
                })
            }
            broker::DataValue::NotAvailable => proto::datapoint::Value::FailureValue(
                proto::datapoint::Failure::NotAvailable.into(),
            ),
        };

        proto::Datapoint {
            timestamp: Some(Timestamp::from(SystemTime::now())),
            value: Some(value),
        }
    }
}

impl From<&proto::DataType> for broker::DataType {
    fn from(data_type: &proto::DataType) -> Self {
        match data_type {
            proto::DataType::Bool => broker::DataType::Bool,
            proto::DataType::String => broker::DataType::String,
            proto::DataType::Int8 => broker::DataType::Int8,
            proto::DataType::Int16 => broker::DataType::Int16,
            proto::DataType::Int32 => broker::DataType::Int32,
            proto::DataType::Int64 => broker::DataType::Int64,
            proto::DataType::Uint8 => broker::DataType::Uint8,
            proto::DataType::Uint16 => broker::DataType::Uint16,
            proto::DataType::Uint32 => broker::DataType::Uint32,
            proto::DataType::Uint64 => broker::DataType::Uint64,
            proto::DataType::Float => broker::DataType::Float,
            proto::DataType::Double => broker::DataType::Double,
            proto::DataType::StringArray => broker::DataType::StringArray,
            proto::DataType::BoolArray => broker::DataType::BoolArray,
            proto::DataType::Int8Array => broker::DataType::Int8Array,
            proto::DataType::Int16Array => broker::DataType::Int16Array,
            proto::DataType::Int32Array => broker::DataType::Int32Array,
            proto::DataType::Int64Array => broker::DataType::Int64Array,
            proto::DataType::Uint8Array => broker::DataType::Uint8Array,
            proto::DataType::Uint16Array => broker::DataType::Uint16Array,
            proto::DataType::Uint32Array => broker::DataType::Uint32Array,
            proto::DataType::Uint64Array => broker::DataType::Uint64Array,
            proto::DataType::FloatArray => broker::DataType::FloatArray,
            proto::DataType::DoubleArray => broker::DataType::DoubleArray,
        }
    }
}

impl From<&proto::Datapoint> for broker::DataValue {
    fn from(datapoint: &proto::Datapoint) -> Self {
        match &datapoint.value {
            Some(value) => match value {
                proto::datapoint::Value::StringValue(value) => {
                    broker::DataValue::String(value.to_owned())
                }
                proto::datapoint::Value::BoolValue(value) => broker::DataValue::Bool(*value),
                proto::datapoint::Value::Int32Value(value) => broker::DataValue::Int32(*value),
                proto::datapoint::Value::Int64Value(value) => broker::DataValue::Int64(*value),
                proto::datapoint::Value::Uint32Value(value) => broker::DataValue::Uint32(*value),
                proto::datapoint::Value::Uint64Value(value) => broker::DataValue::Uint64(*value),
                proto::datapoint::Value::FloatValue(value) => broker::DataValue::Float(*value),
                proto::datapoint::Value::DoubleValue(value) => broker::DataValue::Double(*value),
                proto::datapoint::Value::StringArray(array) => {
                    broker::DataValue::StringArray(array.values.clone())
                }
                proto::datapoint::Value::BoolArray(array) => {
                    broker::DataValue::BoolArray(array.values.clone())
                }
                proto::datapoint::Value::Int32Array(array) => {
                    broker::DataValue::Int32Array(array.values.clone())
                }
                proto::datapoint::Value::Int64Array(array) => {
                    broker::DataValue::Int64Array(array.values.clone())
                }
                proto::datapoint::Value::Uint32Array(array) => {
                    broker::DataValue::Uint32Array(array.values.clone())
                }
                proto::datapoint::Value::Uint64Array(array) => {
                    broker::DataValue::Uint64Array(array.values.clone())
                }
                proto::datapoint::Value::FloatArray(array) => {
                    broker::DataValue::FloatArray(array.values.clone())
                }
                proto::datapoint::Value::DoubleArray(array) => {
                    broker::DataValue::DoubleArray(array.values.clone())
                }
                proto::datapoint::Value::FailureValue(_) => broker::DataValue::NotAvailable,
            },
            None => broker::DataValue::NotAvailable,
        }
    }
}

impl From<&broker::DataType> for proto::DataType {
    fn from(value_type: &broker::DataType) -> Self {
        match value_type {
            broker::DataType::Bool => proto::DataType::Bool,
            broker::DataType::String => proto::DataType::String,
            broker::DataType::Int8 => proto::DataType::Int8,
            broker::DataType::Int16 => proto::DataType::Int16,
            broker::DataType::Int32 => proto::DataType::Int32,
            broker::DataType::Int64 => proto::DataType::Int64,
            broker::DataType::Uint8 => proto::DataType::Uint8,
            broker::DataType::Uint16 => proto::DataType::Uint16,
            broker::DataType::Uint32 => proto::DataType::Uint32,
            broker::DataType::Uint64 => proto::DataType::Uint64,
            broker::DataType::Float => proto::DataType::Float,
            broker::DataType::Double => proto::DataType::Double,
            broker::DataType::StringArray => proto::DataType::StringArray,
            broker::DataType::BoolArray => proto::DataType::BoolArray,
            broker::DataType::Int8Array => proto::DataType::Int8Array,
            broker::DataType::Int16Array => proto::DataType::Int16Array,
            broker::DataType::Int32Array => proto::DataType::Int32Array,
            broker::DataType::Int64Array => proto::DataType::Int64Array,
            broker::DataType::Uint8Array => proto::DataType::Uint8Array,
            broker::DataType::Uint16Array => proto::DataType::Uint16Array,
            broker::DataType::Uint32Array => proto::DataType::Uint32Array,
            broker::DataType::Uint64Array => proto::DataType::Uint64Array,
            broker::DataType::FloatArray => proto::DataType::FloatArray,
            broker::DataType::DoubleArray => proto::DataType::DoubleArray,
        }
    }
}

impl From<&broker::EntryType> for proto::EntryType {
    fn from(entry_type: &broker::EntryType) -> Self {
        match entry_type {
            broker::EntryType::Sensor => proto::EntryType::Sensor,
            broker::EntryType::Attribute => proto::EntryType::Attribute,
            broker::EntryType::Actuator => proto::EntryType::Actuator,
        }
    }
}

impl From<&proto::ChangeType> for broker::ChangeType {
    fn from(change_type: &proto::ChangeType) -> Self {
        match change_type {
            proto::ChangeType::OnChange => broker::ChangeType::OnChange,
            proto::ChangeType::Continuous => broker::ChangeType::Continuous,
            proto::ChangeType::Static => broker::ChangeType::Static,
        }
    }
}

impl From<&broker::Metadata> for proto::Metadata {
    fn from(metadata: &broker::Metadata) -> Self {
        proto::Metadata {
            id: metadata.id,
            entry_type: proto::EntryType::from(&metadata.entry_type) as i32,
            name: metadata.path.to_owned(),
            data_type: proto::DataType::from(&metadata.data_type) as i32,
            change_type: proto::ChangeType::Continuous as i32, // TODO: Add to metadata
            description: metadata.description.to_owned(),
        }
    }
}

impl From<&broker::UpdateError> for proto::DatapointError {
    fn from(error: &broker::UpdateError) -> Self {
        match error {
            broker::UpdateError::NotFound => proto::DatapointError::UnknownDatapoint,
            broker::UpdateError::WrongType | broker::UpdateError::UnsupportedType => {
                proto::DatapointError::InvalidType
            }
            broker::UpdateError::OutOfBounds => proto::DatapointError::OutOfBounds,
            broker::UpdateError::PermissionDenied => proto::DatapointError::AccessDenied,
            broker::UpdateError::PermissionExpired => proto::DatapointError::AccessDenied,
        }
    }
}
