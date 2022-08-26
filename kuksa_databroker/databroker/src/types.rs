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

use std::convert::TryFrom;

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum DataType {
    String,
    Bool,
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
    Timestamp,
    StringArray,
    BoolArray,
    Int8Array,
    Int16Array,
    Int32Array,
    Int64Array,
    Uint8Array,
    Uint16Array,
    Uint32Array,
    Uint64Array,
    FloatArray,
    DoubleArray,
    TimestampArray,
}

#[derive(Debug, Clone)]
pub enum EntryType {
    Sensor,
    Attribute,
    Actuator,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ChangeType {
    Static,
    OnChange,
    Continuous,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Value {
    NotAvailable,
    Bool(bool),
    String(String),
    Int32(i32),
    Int64(i64),
    Uint32(u32),
    Uint64(u64),
    Float(f32),
    Double(f64),
    BoolArray(Vec<bool>),
    StringArray(Vec<String>),
    Int32Array(Vec<i32>),
    Int64Array(Vec<i64>),
    Uint32Array(Vec<u32>),
    Uint64Array(Vec<u64>),
    FloatArray(Vec<f32>),
    DoubleArray(Vec<f64>),
}

#[derive(Debug)]
pub struct CastError {}

impl Value {
    pub fn greater_than(&self, other: &Value) -> Result<bool, CastError> {
        match (&self, other) {
            (Value::Int32(value), Value::Int32(other_value)) => Ok(value > other_value),
            (Value::Int32(value), Value::Int64(other_value)) => {
                Ok(i64::from(*value) > *other_value)
            }
            (Value::Int32(value), Value::Uint32(other_value)) => {
                Ok(i64::from(*value) > i64::from(*other_value))
            }
            (Value::Int32(value), Value::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(false) // Negative value cannot be greater than unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value > *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Int32(value), Value::Float(other_value)) => {
                Ok(f64::from(*value) > f64::from(*other_value))
            }
            (Value::Int32(value), Value::Double(other_value)) => {
                Ok(f64::from(*value) > *other_value)
            }
            (Value::Int64(value), Value::Int32(other_value)) => {
                Ok(*value > i64::from(*other_value))
            }
            (Value::Int64(value), Value::Int64(other_value)) => Ok(value > other_value),
            (Value::Int64(value), Value::Uint32(other_value)) => {
                Ok(*value > i64::from(*other_value))
            }
            (Value::Int64(value), Value::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(false) // Negative value cannot be greater than unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value > *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Int64(value), Value::Float(other_value)) => match i32::try_from(*value) {
                Ok(value) => Ok(f64::from(value) > f64::from(*other_value)),
                Err(_) => Err(CastError {}),
            },
            (Value::Int64(value), Value::Double(other_value)) => match i32::try_from(*value) {
                Ok(value) => Ok(f64::from(value) > *other_value),
                Err(_) => Err(CastError {}),
            },
            (Value::Uint32(value), Value::Int32(other_value)) => {
                Ok(i64::from(*value) > i64::from(*other_value))
            }
            (Value::Uint32(value), Value::Int64(other_value)) => {
                Ok(i64::from(*value) > *other_value)
            }
            (Value::Uint32(value), Value::Uint32(other_value)) => Ok(value > other_value),
            (Value::Uint32(value), Value::Uint64(other_value)) => {
                Ok(u64::from(*value) > *other_value)
            }
            (Value::Uint32(value), Value::Float(other_value)) => {
                Ok(f64::from(*value) > f64::from(*other_value))
            }
            (Value::Uint32(value), Value::Double(other_value)) => {
                Ok(f64::from(*value) > *other_value)
            }
            (Value::Uint64(value), Value::Int32(other_value)) => {
                if *other_value < 0 {
                    Ok(true) // Unsigned must be greater than a negative
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value > other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Uint64(value), Value::Int64(other_value)) => {
                if *other_value < 0 {
                    Ok(true) // Unsigned must be greater than a negative
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value > other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Uint64(value), Value::Uint32(other_value)) => {
                Ok(*value > u64::from(*other_value))
            }
            (Value::Uint64(value), Value::Uint64(other_value)) => Ok(value > other_value),
            (Value::Uint64(value), Value::Float(other_value)) => match u32::try_from(*value) {
                Ok(value) => Ok(f64::from(value) > f64::from(*other_value)),
                Err(_) => Err(CastError {}),
            },
            (Value::Uint64(value), Value::Double(other_value)) => match u32::try_from(*value) {
                Ok(value) => Ok(f64::from(value) > *other_value),
                Err(_) => Err(CastError {}),
            },
            (Value::Float(value), Value::Int32(other_value)) => {
                Ok(f64::from(*value) > f64::from(*other_value))
            }
            (Value::Float(value), Value::Int64(other_value)) => match i32::try_from(*other_value) {
                Ok(other_value) => Ok(f64::from(*value) > f64::from(other_value)),
                Err(_) => Err(CastError {}),
            },
            (Value::Float(value), Value::Uint32(other_value)) => {
                Ok(f64::from(*value) > f64::from(*other_value))
            }
            (Value::Float(value), Value::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => Ok(f64::from(*value) > f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (Value::Float(value), Value::Float(other_value)) => Ok(value > other_value),
            (Value::Float(value), Value::Double(other_value)) => {
                Ok(f64::from(*value) > *other_value)
            }
            (Value::Double(value), Value::Int32(other_value)) => {
                Ok(*value > f64::from(*other_value))
            }
            (Value::Double(value), Value::Int64(other_value)) => {
                match i32::try_from(*other_value) {
                    Ok(other_value) => Ok(*value > f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (Value::Double(value), Value::Uint32(other_value)) => {
                Ok(*value > f64::from(*other_value))
            }
            (Value::Double(value), Value::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => Ok(*value > f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (Value::Double(value), Value::Float(other_value)) => {
                Ok(*value > f64::from(*other_value))
            }
            (Value::Double(value), Value::Double(other_value)) => Ok(value > other_value),
            _ => Err(CastError {}),
        }
    }

    pub fn less_than(&self, other: &Value) -> Result<bool, CastError> {
        match (&self, other) {
            (Value::Int32(value), Value::Int32(other_value)) => Ok(value < other_value),
            (Value::Int32(value), Value::Int64(other_value)) => {
                Ok(i64::from(*value) < *other_value)
            }
            (Value::Int32(value), Value::Uint32(other_value)) => {
                Ok(i64::from(*value) < i64::from(*other_value))
            }
            (Value::Int32(value), Value::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(true) // Negative value must be less than unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value < *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Int32(value), Value::Float(other_value)) => {
                Ok(f64::from(*value) < f64::from(*other_value))
            }
            (Value::Int32(value), Value::Double(other_value)) => {
                Ok(f64::from(*value) < *other_value)
            }

            (Value::Int64(value), Value::Int32(other_value)) => {
                Ok(*value < i64::from(*other_value))
            }
            (Value::Int64(value), Value::Int64(other_value)) => Ok(value < other_value),
            (Value::Int64(value), Value::Uint32(other_value)) => {
                Ok(*value < i64::from(*other_value))
            }
            (Value::Int64(value), Value::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(true) // Negative value must be less than unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value < *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Int64(value), Value::Float(other_value)) => match i32::try_from(*value) {
                Ok(value) => Ok(f64::from(value) < f64::from(*other_value)),
                Err(_) => Err(CastError {}),
            },
            (Value::Int64(value), Value::Double(other_value)) => match i32::try_from(*value) {
                Ok(value) => Ok(f64::from(value) < *other_value),
                Err(_) => Err(CastError {}),
            },

            (Value::Uint32(value), Value::Int32(other_value)) => {
                Ok(i64::from(*value) < i64::from(*other_value))
            }
            (Value::Uint32(value), Value::Int64(other_value)) => {
                Ok(i64::from(*value) < *other_value)
            }
            (Value::Uint32(value), Value::Uint32(other_value)) => Ok(value < other_value),
            (Value::Uint32(value), Value::Uint64(other_value)) => {
                Ok(u64::from(*value) < *other_value)
            }
            (Value::Uint32(value), Value::Float(other_value)) => {
                Ok(f64::from(*value) < f64::from(*other_value))
            }
            (Value::Uint32(value), Value::Double(other_value)) => {
                Ok(f64::from(*value) < *other_value)
            }
            (Value::Uint64(value), Value::Int32(other_value)) => {
                if *other_value < 0 {
                    Ok(false) // Unsigned cannot be less than a negative value
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value < other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Uint64(value), Value::Int64(other_value)) => {
                if *other_value < 0 {
                    Ok(false) // Unsigned cannot be less than a negative value
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value < other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Uint64(value), Value::Uint32(other_value)) => {
                Ok(*value < u64::from(*other_value))
            }
            (Value::Uint64(value), Value::Uint64(other_value)) => Ok(value < other_value),
            (Value::Uint64(value), Value::Float(other_value)) => match u32::try_from(*value) {
                Ok(value) => Ok(f64::from(value) < f64::from(*other_value)),
                Err(_) => Err(CastError {}),
            },
            (Value::Uint64(value), Value::Double(other_value)) => match u32::try_from(*value) {
                Ok(value) => Ok(f64::from(value) < *other_value),
                Err(_) => Err(CastError {}),
            },
            (Value::Float(value), Value::Int32(other_value)) => {
                Ok(f64::from(*value) < f64::from(*other_value))
            }
            (Value::Float(value), Value::Int64(other_value)) => match i32::try_from(*other_value) {
                Ok(other_value) => Ok(f64::from(*value) < f64::from(other_value)),
                Err(_) => Err(CastError {}),
            },
            (Value::Float(value), Value::Uint32(other_value)) => {
                Ok(f64::from(*value) < f64::from(*other_value))
            }
            (Value::Float(value), Value::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => Ok(f64::from(*value) < f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (Value::Float(value), Value::Float(other_value)) => Ok(value < other_value),
            (Value::Float(value), Value::Double(other_value)) => {
                Ok(f64::from(*value) < *other_value)
            }
            (Value::Double(value), Value::Int32(other_value)) => {
                Ok(*value < f64::from(*other_value))
            }
            (Value::Double(value), Value::Int64(other_value)) => {
                match i32::try_from(*other_value) {
                    Ok(other_value) => Ok(*value < f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (Value::Double(value), Value::Uint32(other_value)) => {
                Ok(*value < f64::from(*other_value))
            }
            (Value::Double(value), Value::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => Ok(*value < f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (Value::Double(value), Value::Float(other_value)) => {
                Ok(*value < f64::from(*other_value))
            }
            (Value::Double(value), Value::Double(other_value)) => Ok(value < other_value),
            _ => Err(CastError {}),
        }
    }

    pub fn equals(&self, other: &Value) -> Result<bool, CastError> {
        match (&self, other) {
            (Value::Bool(value), Value::Bool(other_value)) => Ok(value == other_value),
            (Value::String(value), Value::String(other_value)) => Ok(value == other_value),
            (Value::Int32(value), Value::Int32(other_value)) => Ok(value == other_value),
            (Value::Int32(value), Value::Int64(other_value)) => {
                Ok(i64::from(*value) == *other_value)
            }
            (Value::Int32(value), Value::Uint32(other_value)) => {
                Ok(i64::from(*value) == i64::from(*other_value))
            }
            (Value::Int32(value), Value::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(false) // Negative value cannot be equal to unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value == *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Int32(value), Value::Float(other_value)) => {
                Ok((f64::from(*value) - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (Value::Int32(value), Value::Double(other_value)) => {
                Ok((f64::from(*value) - *other_value).abs() < f64::EPSILON)
            }
            (Value::Int64(value), Value::Int32(other_value)) => {
                Ok(*value == i64::from(*other_value))
            }
            (Value::Int64(value), Value::Int64(other_value)) => Ok(value == other_value),
            (Value::Int64(value), Value::Uint32(other_value)) => {
                Ok(*value == i64::from(*other_value))
            }
            (Value::Int64(value), Value::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(false) // Negative value cannot be equal to unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value == *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Int64(value), Value::Float(other_value)) => match i32::try_from(*value) {
                Ok(value) => Ok((f64::from(value) - f64::from(*other_value)).abs() < f64::EPSILON),
                Err(_) => Err(CastError {}),
            },
            (Value::Int64(value), Value::Double(other_value)) => match i32::try_from(*value) {
                Ok(value) => Ok((f64::from(value) - *other_value).abs() < f64::EPSILON),
                Err(_) => Err(CastError {}),
            },
            (Value::Uint32(value), Value::Int32(other_value)) => {
                Ok(i64::from(*value) == i64::from(*other_value))
            }
            (Value::Uint32(value), Value::Int64(other_value)) => {
                Ok(i64::from(*value) == *other_value)
            }
            (Value::Uint32(value), Value::Uint32(other_value)) => Ok(value == other_value),
            (Value::Uint32(value), Value::Uint64(other_value)) => {
                Ok(u64::from(*value) == *other_value)
            }
            (Value::Uint32(value), Value::Float(other_value)) => {
                Ok((f64::from(*value) - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (Value::Uint32(value), Value::Double(other_value)) => {
                Ok((f64::from(*value) - *other_value).abs() < f64::EPSILON)
            }
            (Value::Uint64(value), Value::Int32(other_value)) => {
                if *other_value < 0 {
                    Ok(false) // Unsigned cannot be equal to negative value
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value == other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Uint64(value), Value::Int64(other_value)) => {
                if *other_value < 0 {
                    Ok(false) // Unsigned cannot be equal to negative value
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value == other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (Value::Uint64(value), Value::Uint32(other_value)) => {
                Ok(*value == u64::from(*other_value))
            }
            (Value::Uint64(value), Value::Uint64(other_value)) => Ok(value == other_value),
            (Value::Uint64(value), Value::Float(other_value)) => match u32::try_from(*value) {
                Ok(value) => Ok((f64::from(value) - f64::from(*other_value)).abs() < f64::EPSILON),
                Err(_) => Err(CastError {}),
            },
            (Value::Uint64(value), Value::Double(other_value)) => match u32::try_from(*value) {
                Ok(value) => Ok((f64::from(value) - *other_value).abs() < f64::EPSILON),
                Err(_) => Err(CastError {}),
            },
            (Value::Float(value), Value::Int32(other_value)) => {
                Ok((f64::from(*value) - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (Value::Float(value), Value::Int64(other_value)) => match i32::try_from(*other_value) {
                Ok(other_value) => {
                    Ok((f64::from(*value) - f64::from(other_value)).abs() < f64::EPSILON)
                }
                Err(_) => Err(CastError {}),
            },
            (Value::Float(value), Value::Uint32(other_value)) => {
                Ok((f64::from(*value) - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (Value::Float(value), Value::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => {
                        Ok((f64::from(*value) - f64::from(other_value)).abs() < f64::EPSILON)
                    }
                    Err(_) => Err(CastError {}),
                }
            }
            (Value::Float(value), Value::Float(other_value)) => {
                // TODO: Implement better floating point comparison
                Ok((value - other_value).abs() < f32::EPSILON)
            }
            (Value::Float(value), Value::Double(other_value)) => {
                // TODO: Implement better floating point comparison
                Ok((f64::from(*value) - *other_value).abs() < f64::EPSILON)
            }
            (Value::Double(value), Value::Int32(other_value)) => {
                Ok((*value - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (Value::Double(value), Value::Int64(other_value)) => {
                match i32::try_from(*other_value) {
                    Ok(other_value) => Ok((*value - f64::from(other_value)).abs() < f64::EPSILON),
                    Err(_) => Err(CastError {}),
                }
            }
            (Value::Double(value), Value::Uint32(other_value)) => {
                Ok((*value - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (Value::Double(value), Value::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => Ok((*value - f64::from(other_value)).abs() < f64::EPSILON),
                    Err(_) => Err(CastError {}),
                }
            }
            (Value::Double(value), Value::Float(other_value)) => {
                // TODO: Implement better floating point comparison
                Ok((*value - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (Value::Double(value), Value::Double(other_value)) => {
                // TODO: Implement better floating point comparison
                Ok((value - other_value).abs() < f64::EPSILON)
            }
            _ => Err(CastError {}),
        }
    }
}

#[test]
fn test_string_greater_than() {
    assert!(matches!(
        Value::String("string".to_owned()).greater_than(&Value::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        Value::String("string".to_owned()).greater_than(&Value::Bool(false)),
        Err(_)
    ));
    assert!(matches!(
        Value::String("string".to_owned()).greater_than(&Value::String("string2".to_owned())),
        Err(_)
    ));
    assert!(matches!(
        Value::String("string".to_owned()).greater_than(&Value::Int32(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::String("string".to_owned()).greater_than(&Value::Int64(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::String("string".to_owned()).greater_than(&Value::Uint32(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::String("string".to_owned()).greater_than(&Value::Uint64(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::String("string".to_owned()).greater_than(&Value::Float(100.0)),
        Err(_)
    ));
    assert!(matches!(
        Value::String("string".to_owned()).greater_than(&Value::Double(100.0)),
        Err(_)
    ));
}

#[test]
fn test_not_available_greater_than() {
    assert!(matches!(
        Value::NotAvailable.greater_than(&Value::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        Value::NotAvailable.greater_than(&Value::Bool(false)),
        Err(_)
    ));
    assert!(matches!(
        Value::NotAvailable.greater_than(&Value::String("string".to_owned())),
        Err(_)
    ));
    assert!(matches!(
        Value::NotAvailable.greater_than(&Value::Int32(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::NotAvailable.greater_than(&Value::Int64(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::NotAvailable.greater_than(&Value::Uint32(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::NotAvailable.greater_than(&Value::Uint64(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::NotAvailable.greater_than(&Value::Float(100.0)),
        Err(_)
    ));
    assert!(matches!(
        Value::NotAvailable.greater_than(&Value::Double(100.0)),
        Err(_)
    ));
}

#[test]
fn test_bool_greater_than() {
    assert!(matches!(
        Value::Bool(false).greater_than(&Value::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        Value::Bool(false).greater_than(&Value::Bool(false)),
        Err(_)
    ));
    assert!(matches!(
        Value::Bool(false).greater_than(&Value::String("string".to_owned())),
        Err(_)
    ));
    assert!(matches!(
        Value::Bool(false).greater_than(&Value::Int32(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::Bool(false).greater_than(&Value::Int64(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::Bool(false).greater_than(&Value::Uint32(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::Bool(false).greater_than(&Value::Uint64(100)),
        Err(_)
    ));
    assert!(matches!(
        Value::Bool(false).greater_than(&Value::Float(100.0)),
        Err(_)
    ));
    assert!(matches!(
        Value::Bool(false).greater_than(&Value::Double(100.0)),
        Err(_)
    ));
}

#[test]
fn test_int32_greater_than() {
    // Comparison not possible
    //
    assert!(matches!(
        Value::Int32(5000).greater_than(&Value::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        Value::Int32(5000).greater_than(&Value::Bool(true)),
        Err(_)
    ));
    assert!(matches!(
        Value::Int32(5000).greater_than(&Value::String("string".to_owned())),
        Err(_)
    ));

    // Should be greater than
    //
    assert!(matches!(
        Value::Int32(5000).greater_than(&Value::Int32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int32(5000).greater_than(&Value::Int32(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int32(-4000).greater_than(&Value::Int32(-5000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int32(5000).greater_than(&Value::Int64(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int32(5000).greater_than(&Value::Int64(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int32(5000).greater_than(&Value::Uint32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int32(-4000).greater_than(&Value::Int64(-5000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int32(i32::MAX).greater_than(&Value::Uint64(u64::try_from(i32::MAX - 1).unwrap())),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int32(30).greater_than(&Value::Uint64(20)),
        Ok(true)
    ));

    // Should not be greater than
    //

    assert!(matches!(
        Value::Int32(-5000).greater_than(&Value::Uint32(4000)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Int32(4000).greater_than(&Value::Uint32(5000)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Int32(20).greater_than(&Value::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Int32(10).greater_than(&Value::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Int32(-10).greater_than(&Value::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Int32(-10).greater_than(&Value::Uint64(u64::MAX)),
        Ok(false)
    ));
}

#[test]
fn test_int64_greater_than() {
    // Comparison not possible
    //

    assert!(matches!(
        Value::Int64(5000).greater_than(&Value::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        Value::Int64(5000).greater_than(&Value::Bool(true)),
        Err(_)
    ));
    assert!(matches!(
        Value::Int64(5000).greater_than(&Value::String("string".to_owned())),
        Err(_)
    ));

    // Should be greater than
    //

    assert!(matches!(
        Value::Int64(5000).greater_than(&Value::Int32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int64(5000).greater_than(&Value::Int32(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int64(-4000).greater_than(&Value::Int32(-5000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int64(5000).greater_than(&Value::Int64(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int64(5000).greater_than(&Value::Int64(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int64(5000).greater_than(&Value::Uint32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int64(-4000).greater_than(&Value::Int64(-5000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int64(i64::from(i32::MAX))
            .greater_than(&Value::Uint64(u64::try_from(i32::MAX - 1).unwrap())),
        Ok(true)
    ));
    assert!(matches!(
        Value::Int64(30).greater_than(&Value::Uint64(20)),
        Ok(true)
    ));

    // Should not be greater than
    //

    assert!(matches!(
        Value::Int64(-10).greater_than(&Value::Uint64(u64::MAX)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Int64(-5000).greater_than(&Value::Uint32(4000)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Int64(4000).greater_than(&Value::Uint32(5000)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Int64(20).greater_than(&Value::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Int64(10).greater_than(&Value::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Int64(-10).greater_than(&Value::Uint64(20)),
        Ok(false)
    ));
}

#[test]
fn test_uint32_greater_than() {
    // Comparison not possible
    //

    assert!(matches!(
        Value::Uint32(5000).greater_than(&Value::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        Value::Uint32(5000).greater_than(&Value::Bool(true)),
        Err(_)
    ));
    assert!(matches!(
        Value::Uint32(5000).greater_than(&Value::String("string".to_owned())),
        Err(_)
    ));

    // Should be greater than
    //

    assert!(matches!(
        Value::Uint32(5000).greater_than(&Value::Int32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint32(5000).greater_than(&Value::Int32(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint32(5000).greater_than(&Value::Int64(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint32(5000).greater_than(&Value::Int64(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint32(5000).greater_than(&Value::Float(4000.0)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint32(5000).greater_than(&Value::Double(-4000.0)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint32(5000).greater_than(&Value::Uint32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint32(u32::MAX).greater_than(&Value::Int32(-5000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint32(u32::MAX).greater_than(&Value::Int64(i64::MIN)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint32(30).greater_than(&Value::Uint64(20)),
        Ok(true)
    ));

    // Should not be greater than
    //

    assert!(matches!(
        Value::Uint32(4000).greater_than(&Value::Int32(5000)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Uint32(4000).greater_than(&Value::Uint32(5000)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Uint32(20).greater_than(&Value::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Uint32(10).greater_than(&Value::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Uint32(20).greater_than(&Value::Float(20.0)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Uint32(10).greater_than(&Value::Double(20.0)),
        Ok(false)
    ));
}

#[test]
fn test_uint64_greater_than() {
    // Comparison not possible
    //

    assert!(matches!(
        Value::Uint64(5000).greater_than(&Value::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        Value::Uint64(5000).greater_than(&Value::Bool(true)),
        Err(_)
    ));
    assert!(matches!(
        Value::Uint64(5000).greater_than(&Value::String("string".to_owned())),
        Err(_)
    ));

    // Should be greater than
    //
    assert!(matches!(
        Value::Uint64(5000).greater_than(&Value::Int32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint64(5000).greater_than(&Value::Int32(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint64(5000).greater_than(&Value::Int64(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint64(5000).greater_than(&Value::Int64(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint64(5000).greater_than(&Value::Float(4000.0)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint64(5000).greater_than(&Value::Double(-4000.0)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint64(5000).greater_than(&Value::Uint32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint64(u64::MAX).greater_than(&Value::Int64(i64::MAX - 1)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Uint64(30).greater_than(&Value::Uint64(20)),
        Ok(true)
    ));

    // Should not be greater than
    //

    assert!(matches!(
        Value::Uint64(4000).greater_than(&Value::Uint32(5000)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Uint64(20).greater_than(&Value::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Uint64(10).greater_than(&Value::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Uint64(20).greater_than(&Value::Float(20.0)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Uint64(10).greater_than(&Value::Double(20.0)),
        Ok(false)
    ));
}

#[test]
fn test_float_greater_than() {}

#[test]
fn test_double_greater_than() {}

#[test]
fn test_int32_less_than() {}

#[test]
fn test_int64_less_than() {}

#[test]
fn test_uint32_less_than() {}

#[test]
fn test_uint64_less_than() {}

#[test]
fn test_float_less_than() {}

#[test]
fn test_double_less_than() {}

#[test]
fn test_float_equals() {
    assert!(matches!(
        Value::Float(5000.0).greater_than(&Value::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        Value::Float(5000.0).greater_than(&Value::Bool(true)),
        Err(_)
    ));
    assert!(matches!(
        Value::Float(5000.0).greater_than(&Value::String("string".to_owned())),
        Err(_)
    ));

    assert!(matches!(
        Value::Float(32.0).equals(&Value::Int32(32)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Float(32.0).equals(&Value::Int64(32)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Float(32.0).equals(&Value::Uint32(32)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Float(32.0).equals(&Value::Uint64(32)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Float(32.0).equals(&Value::Float(32.0)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Float(32.0).equals(&Value::Double(32.0)),
        Ok(true)
    ));

    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Int32(-32)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Int64(-32)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Uint32(32)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Uint64(32)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Float(-32.0)),
        Ok(true)
    ));
    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Double(-32.0)),
        Ok(true)
    ));

    assert!(matches!(
        Value::Float(32.0).equals(&Value::Int32(33)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(32.0).equals(&Value::Int64(33)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(32.0).equals(&Value::Uint32(33)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(32.0).equals(&Value::Uint64(33)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(32.0).equals(&Value::Float(33.0)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(32.0).equals(&Value::Double(33.0)),
        Ok(false)
    ));

    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Int32(-33)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Int64(-33)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Uint32(33)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Uint64(33)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Float(-33.0)),
        Ok(false)
    ));
    assert!(matches!(
        Value::Float(-32.0).equals(&Value::Double(-33.0)),
        Ok(false)
    ));
}
