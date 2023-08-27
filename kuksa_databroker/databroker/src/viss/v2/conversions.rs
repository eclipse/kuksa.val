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

use std::{convert::TryFrom, time::SystemTime};

use crate::broker;

use super::types::{
    ActuatorEntry, AttributeEntry, DataPoint, DataType, MetadataEntry, SensorEntry, Value,
};

pub enum Error {
    ParseError,
}

impl From<broker::Datapoint> for DataPoint {
    fn from(dp: broker::Datapoint) -> Self {
        DataPoint {
            value: dp.value.into(),
            ts: dp.ts.into(),
        }
    }
}

impl TryFrom<Value> for String {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Array(_) | Value::None => Err(Error::ParseError),
            Value::Scalar(value) => Ok(value),
        }
    }
}

impl TryFrom<Value> for bool {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Array(_) | Value::None => Err(Error::ParseError),
            Value::Scalar(value) => value.parse::<bool>().map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for i32 {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Array(_) | Value::None => Err(Error::ParseError),
            Value::Scalar(value) => value.parse::<i32>().map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for i64 {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Array(_) | Value::None => Err(Error::ParseError),
            Value::Scalar(value) => value.parse::<i64>().map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for u32 {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Array(_) | Value::None => Err(Error::ParseError),
            Value::Scalar(value) => value.parse::<u32>().map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for u64 {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Array(_) | Value::None => Err(Error::ParseError),
            Value::Scalar(value) => value.parse::<u64>().map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for f32 {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Array(_) | Value::None => Err(Error::ParseError),
            Value::Scalar(value) => value.parse::<f32>().map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for f64 {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Array(_) | Value::None => Err(Error::ParseError),
            Value::Scalar(value) => value.parse::<f64>().map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for Vec<String> {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Scalar(_) | Value::None => Err(Error::ParseError),
            Value::Array(array) => Ok(array),
        }
    }
}

impl TryFrom<Value> for Vec<bool> {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Scalar(_) | Value::None => Err(Error::ParseError),
            Value::Array(array) => array
                .iter()
                .map(|value| value.parse::<bool>())
                .collect::<Result<Vec<_>, _>>()
                .map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for Vec<i32> {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Scalar(_) | Value::None => Err(Error::ParseError),
            Value::Array(array) => array
                .iter()
                .map(|value| value.parse::<i32>())
                .collect::<Result<Vec<_>, _>>()
                .map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for Vec<i64> {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Scalar(_) | Value::None => Err(Error::ParseError),
            Value::Array(array) => array
                .iter()
                .map(|value| value.parse::<i64>())
                .collect::<Result<Vec<_>, _>>()
                .map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for Vec<u32> {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Scalar(_) | Value::None => Err(Error::ParseError),
            Value::Array(array) => array
                .iter()
                .map(|value| value.parse::<u32>())
                .collect::<Result<Vec<_>, _>>()
                .map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for Vec<u64> {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Scalar(_) | Value::None => Err(Error::ParseError),
            Value::Array(array) => array
                .iter()
                .map(|value| value.parse::<u64>())
                .collect::<Result<Vec<_>, _>>()
                .map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for Vec<f32> {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Scalar(_) | Value::None => Err(Error::ParseError),
            Value::Array(array) => array
                .iter()
                .map(|value| value.parse::<f32>())
                .collect::<Result<Vec<_>, _>>()
                .map_err(|_| Error::ParseError),
        }
    }
}

impl TryFrom<Value> for Vec<f64> {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Scalar(_) | Value::None => Err(Error::ParseError),
            Value::Array(array) => array
                .iter()
                .map(|value| value.parse::<f64>())
                .collect::<Result<Vec<_>, _>>()
                .map_err(|_| Error::ParseError),
        }
    }
}

impl From<broker::DataValue> for Value {
    fn from(value: broker::DataValue) -> Self {
        match value {
            broker::DataValue::NotAvailable => Value::None,
            broker::DataValue::Bool(value) => Value::Scalar(value.to_string()),
            broker::DataValue::String(value) => Value::Scalar(value),
            broker::DataValue::Int32(value) => Value::Scalar(value.to_string()),
            broker::DataValue::Int64(value) => Value::Scalar(value.to_string()),
            broker::DataValue::Uint32(value) => Value::Scalar(value.to_string()),
            broker::DataValue::Uint64(value) => Value::Scalar(value.to_string()),
            broker::DataValue::Float(value) => Value::Scalar(value.to_string()),
            broker::DataValue::Double(value) => Value::Scalar(value.to_string()),
            broker::DataValue::BoolArray(array) => {
                Value::Array(array.iter().map(|value| value.to_string()).collect())
            }
            broker::DataValue::StringArray(array) => Value::Array(array),
            broker::DataValue::Int32Array(array) => {
                Value::Array(array.iter().map(|value| value.to_string()).collect())
            }
            broker::DataValue::Int64Array(array) => {
                Value::Array(array.iter().map(|value| value.to_string()).collect())
            }
            broker::DataValue::Uint32Array(array) => {
                Value::Array(array.iter().map(|value| value.to_string()).collect())
            }
            broker::DataValue::Uint64Array(array) => {
                Value::Array(array.iter().map(|value| value.to_string()).collect())
            }
            broker::DataValue::FloatArray(array) => {
                Value::Array(array.iter().map(|value| value.to_string()).collect())
            }
            broker::DataValue::DoubleArray(array) => {
                Value::Array(array.iter().map(|value| value.to_string()).collect())
            }
        }
    }
}

impl TryFrom<Value> for SystemTime {
    type Error = Error;

    fn try_from(value: Value) -> Result<Self, Self::Error> {
        match value {
            Value::Array(_) | Value::None => Err(Error::ParseError),
            Value::Scalar(value) => chrono::DateTime::parse_from_rfc3339(&value)
                .map(|datetime| datetime.with_timezone(&chrono::Utc))
                .map(SystemTime::from)
                .map_err(|_| Error::ParseError),
        }
    }
}

impl From<broker::DataType> for DataType {
    fn from(value: broker::DataType) -> Self {
        match value {
            broker::DataType::String => DataType::String,
            broker::DataType::Bool => DataType::Boolean,
            broker::DataType::Int8 => DataType::Int8,
            broker::DataType::Int16 => DataType::Int16,
            broker::DataType::Int32 => DataType::Int32,
            broker::DataType::Int64 => DataType::Int64,
            broker::DataType::Uint8 => DataType::Uint8,
            broker::DataType::Uint16 => DataType::Uint16,
            broker::DataType::Uint32 => DataType::Uint32,
            broker::DataType::Uint64 => DataType::Uint64,
            broker::DataType::Float => DataType::Float,
            broker::DataType::Double => DataType::Double,
            broker::DataType::StringArray => DataType::StringArray,
            broker::DataType::BoolArray => DataType::BoolArray,
            broker::DataType::Int8Array => DataType::Int8Array,
            broker::DataType::Int16Array => DataType::Int16Array,
            broker::DataType::Int32Array => DataType::Int32Array,
            broker::DataType::Int64Array => DataType::Int64Array,
            broker::DataType::Uint8Array => DataType::Uint8Array,
            broker::DataType::Uint16Array => DataType::Uint16Array,
            broker::DataType::Uint32Array => DataType::Uint32Array,
            broker::DataType::Uint64Array => DataType::Uint64Array,
            broker::DataType::FloatArray => DataType::FloatArray,
            broker::DataType::DoubleArray => DataType::DoubleArray,
        }
    }
}
impl From<&broker::Metadata> for MetadataEntry {
    fn from(metadata: &broker::Metadata) -> Self {
        match metadata.entry_type {
            broker::EntryType::Sensor => MetadataEntry::Sensor(SensorEntry {
                datatype: metadata.data_type.clone().into(),
                description: metadata.description.clone(),
                comment: None,
                unit: None,
                allowed: metadata.allowed.clone().map(|allowed| allowed.into()),
                min: None,
                max: None,
            }),
            broker::EntryType::Attribute => MetadataEntry::Attribute(AttributeEntry {
                datatype: metadata.data_type.clone().into(),
                description: metadata.description.clone(),
                unit: None,
                allowed: metadata.allowed.clone().map(|allowed| allowed.into()),
                default: None, // TODO: Add to metadata
            }),
            broker::EntryType::Actuator => MetadataEntry::Actuator(ActuatorEntry {
                description: metadata.description.clone(),
                comment: None,
                datatype: metadata.data_type.clone().into(),
                unit: None,
                allowed: metadata.allowed.clone().map(|allowed| allowed.into()),
                min: None,
                max: None,
            }),
        }
    }
}

impl Value {
    pub fn try_into_type(self, data_type: &broker::DataType) -> Result<broker::DataValue, Error> {
        match data_type {
            broker::DataType::String => String::try_from(self).map(broker::DataValue::String),
            broker::DataType::Bool => bool::try_from(self).map(broker::DataValue::Bool),
            broker::DataType::Int8 | broker::DataType::Int16 | broker::DataType::Int32 => {
                i32::try_from(self).map(broker::DataValue::Int32)
            }
            broker::DataType::Int64 => i64::try_from(self).map(broker::DataValue::Int64),
            broker::DataType::Uint8 | broker::DataType::Uint16 | broker::DataType::Uint32 => {
                u32::try_from(self).map(broker::DataValue::Uint32)
            }
            broker::DataType::Uint64 => u64::try_from(self).map(broker::DataValue::Uint64),
            broker::DataType::Float => f32::try_from(self).map(broker::DataValue::Float),
            broker::DataType::Double => f64::try_from(self).map(broker::DataValue::Double),
            // broker::DataType::Timestamp => {
            //     SystemTime::try_from(self).map(broker::DataValue::Timestamp)
            // }
            broker::DataType::StringArray => {
                Vec::<String>::try_from(self).map(broker::DataValue::StringArray)
            }
            broker::DataType::BoolArray => {
                Vec::<bool>::try_from(self).map(broker::DataValue::BoolArray)
            }
            broker::DataType::Int8Array
            | broker::DataType::Int16Array
            | broker::DataType::Int32Array => {
                Vec::<i32>::try_from(self).map(broker::DataValue::Int32Array)
            }
            broker::DataType::Int64Array => {
                Vec::<i64>::try_from(self).map(broker::DataValue::Int64Array)
            }
            broker::DataType::Uint8Array
            | broker::DataType::Uint16Array
            | broker::DataType::Uint32Array => {
                Vec::<u32>::try_from(self).map(broker::DataValue::Uint32Array)
            }
            broker::DataType::Uint64Array => {
                Vec::<u64>::try_from(self).map(broker::DataValue::Uint64Array)
            }
            broker::DataType::FloatArray => {
                Vec::<f32>::try_from(self).map(broker::DataValue::FloatArray)
            }
            broker::DataType::DoubleArray => {
                Vec::<f64>::try_from(self).map(broker::DataValue::DoubleArray)
            }
        }
    }
}

impl std::fmt::Display for DataType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.pad(match self {
            DataType::String => "string",
            DataType::Boolean => "bool",
            DataType::Int8 => "int8",
            DataType::Int16 => "int16",
            DataType::Int32 => "int32",
            DataType::Int64 => "int64",
            DataType::Uint8 => "uint8",
            DataType::Uint16 => "uint16",
            DataType::Uint32 => "uint32",
            DataType::Uint64 => "uint64",
            DataType::Float => "float",
            DataType::Double => "double",
            DataType::StringArray => "string[]",
            DataType::BoolArray => "bool[]",
            DataType::Int8Array => "int8[]",
            DataType::Int16Array => "int16[]",
            DataType::Int32Array => "int32[]",
            DataType::Int64Array => "int64[]",
            DataType::Uint8Array => "uint8[]",
            DataType::Uint16Array => "uint16[]",
            DataType::Uint32Array => "uint32[]",
            DataType::Uint64Array => "uint64[]",
            DataType::FloatArray => "float[]",
            DataType::DoubleArray => "double[]",
        })
    }
}
