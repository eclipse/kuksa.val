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

use databroker_proto::kuksa::val::v1 as proto;

use crate::broker;

use std::convert::TryFrom;
use std::time::SystemTime;

impl From<&broker::EntryType> for proto::EntryType {
    fn from(from: &broker::EntryType) -> Self {
        match from {
            broker::EntryType::Sensor => proto::EntryType::Sensor,
            broker::EntryType::Attribute => proto::EntryType::Attribute,
            broker::EntryType::Actuator => proto::EntryType::Actuator,
        }
    }
}

impl From<broker::DataType> for proto::DataType {
    fn from(from: broker::DataType) -> Self {
        match from {
            broker::DataType::String => proto::DataType::String,
            broker::DataType::Bool => proto::DataType::Boolean,
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
            broker::DataType::Timestamp => proto::DataType::Timestamp,
            broker::DataType::StringArray => proto::DataType::StringArray,
            broker::DataType::BoolArray => proto::DataType::BooleanArray,
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
            broker::DataType::TimestampArray => proto::DataType::TimestampArray,
        }
    }
}

impl From<broker::Datapoint> for Option<proto::Datapoint> {
    fn from(from: broker::Datapoint) -> Self {
        match from.value {
            broker::DataValue::NotAvailable => None,
            broker::DataValue::Bool(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Bool(value)),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::String(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::String(value)),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::Int32(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Int32(value)),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::Int64(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Int64(value)),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::Uint32(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Uint32(value)),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::Uint64(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Uint64(value)),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::Float(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Float(value)),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::Double(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Double(value)),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::BoolArray(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::BoolArray(proto::BoolArray {
                    values,
                })),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::StringArray(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::StringArray(proto::StringArray {
                    values,
                })),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::Int32Array(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Int32Array(proto::Int32Array {
                    values,
                })),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::Int64Array(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Int64Array(proto::Int64Array {
                    values,
                })),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::Uint32Array(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Uint32Array(proto::Uint32Array {
                    values,
                })),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::Uint64Array(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Uint64Array(proto::Uint64Array {
                    values,
                })),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::FloatArray(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::FloatArray(proto::FloatArray {
                    values,
                })),
                timestamp: Some(from.ts.into()),
            }),
            broker::DataValue::DoubleArray(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::DoubleArray(proto::DoubleArray {
                    values,
                })),
                timestamp: Some(from.ts.into()),
            }),
        }
    }
}

impl From<broker::DataValue> for Option<proto::Datapoint> {
    fn from(from: broker::DataValue) -> Self {
        match from {
            broker::DataValue::NotAvailable => None,
            broker::DataValue::Bool(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Bool(value)),
                timestamp: None,
            }),
            broker::DataValue::String(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::String(value)),
                timestamp: None,
            }),
            broker::DataValue::Int32(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Int32(value)),
                timestamp: None,
            }),
            broker::DataValue::Int64(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Int64(value)),
                timestamp: None,
            }),
            broker::DataValue::Uint32(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Uint32(value)),
                timestamp: None,
            }),
            broker::DataValue::Uint64(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Uint64(value)),
                timestamp: None,
            }),
            broker::DataValue::Float(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Float(value)),
                timestamp: None,
            }),
            broker::DataValue::Double(value) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Double(value)),
                timestamp: None,
            }),
            broker::DataValue::BoolArray(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::BoolArray(proto::BoolArray {
                    values,
                })),
                timestamp: None,
            }),
            broker::DataValue::StringArray(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::StringArray(proto::StringArray {
                    values,
                })),
                timestamp: None,
            }),
            broker::DataValue::Int32Array(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Int32Array(proto::Int32Array {
                    values,
                })),
                timestamp: None,
            }),
            broker::DataValue::Int64Array(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Int64Array(proto::Int64Array {
                    values,
                })),
                timestamp: None,
            }),
            broker::DataValue::Uint32Array(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Uint32Array(proto::Uint32Array {
                    values,
                })),
                timestamp: None,
            }),
            broker::DataValue::Uint64Array(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::Uint64Array(proto::Uint64Array {
                    values,
                })),
                timestamp: None,
            }),
            broker::DataValue::FloatArray(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::FloatArray(proto::FloatArray {
                    values,
                })),
                timestamp: None,
            }),
            broker::DataValue::DoubleArray(values) => Some(proto::Datapoint {
                value: Some(proto::datapoint::Value::DoubleArray(proto::DoubleArray {
                    values,
                })),
                timestamp: None,
            }),
        }
    }
}

impl From<Option<proto::datapoint::Value>> for broker::DataValue {
    fn from(from: Option<proto::datapoint::Value>) -> Self {
        match from {
            Some(value) => match value {
                proto::datapoint::Value::String(value) => broker::DataValue::String(value),
                proto::datapoint::Value::Bool(value) => broker::DataValue::Bool(value),
                proto::datapoint::Value::Int32(value) => broker::DataValue::Int32(value),
                proto::datapoint::Value::Int64(value) => broker::DataValue::Int64(value),
                proto::datapoint::Value::Uint32(value) => broker::DataValue::Uint32(value),
                proto::datapoint::Value::Uint64(value) => broker::DataValue::Uint64(value),
                proto::datapoint::Value::Float(value) => broker::DataValue::Float(value),
                proto::datapoint::Value::Double(value) => broker::DataValue::Double(value),
                proto::datapoint::Value::StringArray(array) => {
                    broker::DataValue::StringArray(array.values)
                }
                proto::datapoint::Value::BoolArray(array) => {
                    broker::DataValue::BoolArray(array.values)
                }
                proto::datapoint::Value::Int32Array(array) => {
                    broker::DataValue::Int32Array(array.values)
                }
                proto::datapoint::Value::Int64Array(array) => {
                    broker::DataValue::Int64Array(array.values)
                }
                proto::datapoint::Value::Uint32Array(array) => {
                    broker::DataValue::Uint32Array(array.values)
                }
                proto::datapoint::Value::Uint64Array(array) => {
                    broker::DataValue::Uint64Array(array.values)
                }
                proto::datapoint::Value::FloatArray(array) => {
                    broker::DataValue::FloatArray(array.values)
                }
                proto::datapoint::Value::DoubleArray(array) => {
                    broker::DataValue::DoubleArray(array.values)
                }
            },
            None => broker::DataValue::NotAvailable,
        }
    }
}

impl From<&broker::Field> for proto::Field {
    fn from(from: &broker::Field) -> Self {
        match from {
            broker::Field::Datapoint => proto::Field::Value,
            broker::Field::ActuatorTarget => proto::Field::ActuatorTarget,
        }
    }
}

impl TryFrom<&proto::Field> for broker::Field {
    type Error = &'static str;

    fn try_from(from: &proto::Field) -> Result<Self, Self::Error> {
        match from {
            proto::Field::Value => Ok(broker::Field::Datapoint),
            proto::Field::ActuatorTarget => Ok(broker::Field::ActuatorTarget),
            _ => Err("Unknown field"),
        }
    }
}

impl From<proto::Datapoint> for broker::Datapoint {
    fn from(from: proto::Datapoint) -> Self {
        Self {
            ts: match from.timestamp {
                Some(ts) => match std::convert::TryInto::try_into(ts) {
                    Ok(ts) => ts,
                    Err(_) => SystemTime::now(),
                },
                None => SystemTime::now(),
            },
            value: broker::DataValue::from(from.value),
        }
    }
}

impl From<broker::EntryUpdate> for proto::DataEntry {
    fn from(from: broker::EntryUpdate) -> Self {
        Self {
            path: from.path.unwrap_or_else(|| "".to_string()),
            value: match from.datapoint {
                Some(datapoint) => Option::<proto::Datapoint>::from(datapoint),
                None => None,
            },
            actuator_target: match from.actuator_target {
                Some(Some(actuator_target)) => Option::<proto::Datapoint>::from(actuator_target),
                Some(None) => None,
                None => None,
            },
            metadata: None,
        }
    }
}
