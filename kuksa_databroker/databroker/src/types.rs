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
pub enum DataValue {
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

impl DataValue {
    pub fn greater_than(&self, other: &DataValue) -> Result<bool, CastError> {
        match (&self, other) {
            (DataValue::Int32(value), DataValue::Int32(other_value)) => Ok(value > other_value),
            (DataValue::Int32(value), DataValue::Int64(other_value)) => {
                Ok(i64::from(*value) > *other_value)
            }
            (DataValue::Int32(value), DataValue::Uint32(other_value)) => {
                Ok(i64::from(*value) > i64::from(*other_value))
            }
            (DataValue::Int32(value), DataValue::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(false) // Negative value cannot be greater than unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value > *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Int32(value), DataValue::Float(other_value)) => {
                Ok(f64::from(*value) > f64::from(*other_value))
            }
            (DataValue::Int32(value), DataValue::Double(other_value)) => {
                Ok(f64::from(*value) > *other_value)
            }
            (DataValue::Int64(value), DataValue::Int32(other_value)) => {
                Ok(*value > i64::from(*other_value))
            }
            (DataValue::Int64(value), DataValue::Int64(other_value)) => Ok(value > other_value),
            (DataValue::Int64(value), DataValue::Uint32(other_value)) => {
                Ok(*value > i64::from(*other_value))
            }
            (DataValue::Int64(value), DataValue::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(false) // Negative value cannot be greater than unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value > *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Int64(value), DataValue::Float(other_value)) => match i32::try_from(*value)
            {
                Ok(value) => Ok(f64::from(value) > f64::from(*other_value)),
                Err(_) => Err(CastError {}),
            },
            (DataValue::Int64(value), DataValue::Double(other_value)) => {
                match i32::try_from(*value) {
                    Ok(value) => Ok(f64::from(value) > *other_value),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Uint32(value), DataValue::Int32(other_value)) => {
                Ok(i64::from(*value) > i64::from(*other_value))
            }
            (DataValue::Uint32(value), DataValue::Int64(other_value)) => {
                Ok(i64::from(*value) > *other_value)
            }
            (DataValue::Uint32(value), DataValue::Uint32(other_value)) => Ok(value > other_value),
            (DataValue::Uint32(value), DataValue::Uint64(other_value)) => {
                Ok(u64::from(*value) > *other_value)
            }
            (DataValue::Uint32(value), DataValue::Float(other_value)) => {
                Ok(f64::from(*value) > f64::from(*other_value))
            }
            (DataValue::Uint32(value), DataValue::Double(other_value)) => {
                Ok(f64::from(*value) > *other_value)
            }
            (DataValue::Uint64(value), DataValue::Int32(other_value)) => {
                if *other_value < 0 {
                    Ok(true) // Unsigned must be greater than a negative
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value > other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Uint64(value), DataValue::Int64(other_value)) => {
                if *other_value < 0 {
                    Ok(true) // Unsigned must be greater than a negative
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value > other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Uint64(value), DataValue::Uint32(other_value)) => {
                Ok(*value > u64::from(*other_value))
            }
            (DataValue::Uint64(value), DataValue::Uint64(other_value)) => Ok(value > other_value),
            (DataValue::Uint64(value), DataValue::Float(other_value)) => {
                match u32::try_from(*value) {
                    Ok(value) => Ok(f64::from(value) > f64::from(*other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Uint64(value), DataValue::Double(other_value)) => {
                match u32::try_from(*value) {
                    Ok(value) => Ok(f64::from(value) > *other_value),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Float(value), DataValue::Int32(other_value)) => {
                Ok(f64::from(*value) > f64::from(*other_value))
            }
            (DataValue::Float(value), DataValue::Int64(other_value)) => {
                match i32::try_from(*other_value) {
                    Ok(other_value) => Ok(f64::from(*value) > f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Float(value), DataValue::Uint32(other_value)) => {
                Ok(f64::from(*value) > f64::from(*other_value))
            }
            (DataValue::Float(value), DataValue::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => Ok(f64::from(*value) > f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Float(value), DataValue::Float(other_value)) => Ok(value > other_value),
            (DataValue::Float(value), DataValue::Double(other_value)) => {
                Ok(f64::from(*value) > *other_value)
            }
            (DataValue::Double(value), DataValue::Int32(other_value)) => {
                Ok(*value > f64::from(*other_value))
            }
            (DataValue::Double(value), DataValue::Int64(other_value)) => {
                match i32::try_from(*other_value) {
                    Ok(other_value) => Ok(*value > f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Double(value), DataValue::Uint32(other_value)) => {
                Ok(*value > f64::from(*other_value))
            }
            (DataValue::Double(value), DataValue::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => Ok(*value > f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Double(value), DataValue::Float(other_value)) => {
                Ok(*value > f64::from(*other_value))
            }
            (DataValue::Double(value), DataValue::Double(other_value)) => Ok(value > other_value),
            _ => Err(CastError {}),
        }
    }

    pub fn less_than(&self, other: &DataValue) -> Result<bool, CastError> {
        match (&self, other) {
            (DataValue::Int32(value), DataValue::Int32(other_value)) => Ok(value < other_value),
            (DataValue::Int32(value), DataValue::Int64(other_value)) => {
                Ok(i64::from(*value) < *other_value)
            }
            (DataValue::Int32(value), DataValue::Uint32(other_value)) => {
                Ok(i64::from(*value) < i64::from(*other_value))
            }
            (DataValue::Int32(value), DataValue::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(true) // Negative value must be less than unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value < *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Int32(value), DataValue::Float(other_value)) => {
                Ok(f64::from(*value) < f64::from(*other_value))
            }
            (DataValue::Int32(value), DataValue::Double(other_value)) => {
                Ok(f64::from(*value) < *other_value)
            }

            (DataValue::Int64(value), DataValue::Int32(other_value)) => {
                Ok(*value < i64::from(*other_value))
            }
            (DataValue::Int64(value), DataValue::Int64(other_value)) => Ok(value < other_value),
            (DataValue::Int64(value), DataValue::Uint32(other_value)) => {
                Ok(*value < i64::from(*other_value))
            }
            (DataValue::Int64(value), DataValue::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(true) // Negative value must be less than unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value < *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Int64(value), DataValue::Float(other_value)) => match i32::try_from(*value)
            {
                Ok(value) => Ok(f64::from(value) < f64::from(*other_value)),
                Err(_) => Err(CastError {}),
            },
            (DataValue::Int64(value), DataValue::Double(other_value)) => {
                match i32::try_from(*value) {
                    Ok(value) => Ok(f64::from(value) < *other_value),
                    Err(_) => Err(CastError {}),
                }
            }

            (DataValue::Uint32(value), DataValue::Int32(other_value)) => {
                Ok(i64::from(*value) < i64::from(*other_value))
            }
            (DataValue::Uint32(value), DataValue::Int64(other_value)) => {
                Ok(i64::from(*value) < *other_value)
            }
            (DataValue::Uint32(value), DataValue::Uint32(other_value)) => Ok(value < other_value),
            (DataValue::Uint32(value), DataValue::Uint64(other_value)) => {
                Ok(u64::from(*value) < *other_value)
            }
            (DataValue::Uint32(value), DataValue::Float(other_value)) => {
                Ok(f64::from(*value) < f64::from(*other_value))
            }
            (DataValue::Uint32(value), DataValue::Double(other_value)) => {
                Ok(f64::from(*value) < *other_value)
            }
            (DataValue::Uint64(value), DataValue::Int32(other_value)) => {
                if *other_value < 0 {
                    Ok(false) // Unsigned cannot be less than a negative value
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value < other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Uint64(value), DataValue::Int64(other_value)) => {
                if *other_value < 0 {
                    Ok(false) // Unsigned cannot be less than a negative value
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value < other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Uint64(value), DataValue::Uint32(other_value)) => {
                Ok(*value < u64::from(*other_value))
            }
            (DataValue::Uint64(value), DataValue::Uint64(other_value)) => Ok(value < other_value),
            (DataValue::Uint64(value), DataValue::Float(other_value)) => {
                match u32::try_from(*value) {
                    Ok(value) => Ok(f64::from(value) < f64::from(*other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Uint64(value), DataValue::Double(other_value)) => {
                match u32::try_from(*value) {
                    Ok(value) => Ok(f64::from(value) < *other_value),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Float(value), DataValue::Int32(other_value)) => {
                Ok(f64::from(*value) < f64::from(*other_value))
            }
            (DataValue::Float(value), DataValue::Int64(other_value)) => {
                match i32::try_from(*other_value) {
                    Ok(other_value) => Ok(f64::from(*value) < f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Float(value), DataValue::Uint32(other_value)) => {
                Ok(f64::from(*value) < f64::from(*other_value))
            }
            (DataValue::Float(value), DataValue::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => Ok(f64::from(*value) < f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Float(value), DataValue::Float(other_value)) => Ok(value < other_value),
            (DataValue::Float(value), DataValue::Double(other_value)) => {
                Ok(f64::from(*value) < *other_value)
            }
            (DataValue::Double(value), DataValue::Int32(other_value)) => {
                Ok(*value < f64::from(*other_value))
            }
            (DataValue::Double(value), DataValue::Int64(other_value)) => {
                match i32::try_from(*other_value) {
                    Ok(other_value) => Ok(*value < f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Double(value), DataValue::Uint32(other_value)) => {
                Ok(*value < f64::from(*other_value))
            }
            (DataValue::Double(value), DataValue::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => Ok(*value < f64::from(other_value)),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Double(value), DataValue::Float(other_value)) => {
                Ok(*value < f64::from(*other_value))
            }
            (DataValue::Double(value), DataValue::Double(other_value)) => Ok(value < other_value),
            _ => Err(CastError {}),
        }
    }

    pub fn equals(&self, other: &DataValue) -> Result<bool, CastError> {
        match (&self, other) {
            (DataValue::Bool(value), DataValue::Bool(other_value)) => Ok(value == other_value),
            (DataValue::String(value), DataValue::String(other_value)) => Ok(value == other_value),
            (DataValue::Int32(value), DataValue::Int32(other_value)) => Ok(value == other_value),
            (DataValue::Int32(value), DataValue::Int64(other_value)) => {
                Ok(i64::from(*value) == *other_value)
            }
            (DataValue::Int32(value), DataValue::Uint32(other_value)) => {
                Ok(i64::from(*value) == i64::from(*other_value))
            }
            (DataValue::Int32(value), DataValue::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(false) // Negative value cannot be equal to unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value == *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Int32(value), DataValue::Float(other_value)) => {
                Ok((f64::from(*value) - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (DataValue::Int32(value), DataValue::Double(other_value)) => {
                Ok((f64::from(*value) - *other_value).abs() < f64::EPSILON)
            }
            (DataValue::Int64(value), DataValue::Int32(other_value)) => {
                Ok(*value == i64::from(*other_value))
            }
            (DataValue::Int64(value), DataValue::Int64(other_value)) => Ok(value == other_value),
            (DataValue::Int64(value), DataValue::Uint32(other_value)) => {
                Ok(*value == i64::from(*other_value))
            }
            (DataValue::Int64(value), DataValue::Uint64(other_value)) => {
                if *value < 0 {
                    Ok(false) // Negative value cannot be equal to unsigned
                } else {
                    match u64::try_from(*value) {
                        Ok(value) => Ok(value == *other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Int64(value), DataValue::Float(other_value)) => match i32::try_from(*value)
            {
                Ok(value) => Ok((f64::from(value) - f64::from(*other_value)).abs() < f64::EPSILON),
                Err(_) => Err(CastError {}),
            },
            (DataValue::Int64(value), DataValue::Double(other_value)) => {
                match i32::try_from(*value) {
                    Ok(value) => Ok((f64::from(value) - *other_value).abs() < f64::EPSILON),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Uint32(value), DataValue::Int32(other_value)) => {
                Ok(i64::from(*value) == i64::from(*other_value))
            }
            (DataValue::Uint32(value), DataValue::Int64(other_value)) => {
                Ok(i64::from(*value) == *other_value)
            }
            (DataValue::Uint32(value), DataValue::Uint32(other_value)) => Ok(value == other_value),
            (DataValue::Uint32(value), DataValue::Uint64(other_value)) => {
                Ok(u64::from(*value) == *other_value)
            }
            (DataValue::Uint32(value), DataValue::Float(other_value)) => {
                Ok((f64::from(*value) - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (DataValue::Uint32(value), DataValue::Double(other_value)) => {
                Ok((f64::from(*value) - *other_value).abs() < f64::EPSILON)
            }
            (DataValue::Uint64(value), DataValue::Int32(other_value)) => {
                if *other_value < 0 {
                    Ok(false) // Unsigned cannot be equal to negative value
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value == other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Uint64(value), DataValue::Int64(other_value)) => {
                if *other_value < 0 {
                    Ok(false) // Unsigned cannot be equal to negative value
                } else {
                    match u64::try_from(*other_value) {
                        Ok(other_value) => Ok(*value == other_value),
                        Err(_) => Err(CastError {}),
                    }
                }
            }
            (DataValue::Uint64(value), DataValue::Uint32(other_value)) => {
                Ok(*value == u64::from(*other_value))
            }
            (DataValue::Uint64(value), DataValue::Uint64(other_value)) => Ok(value == other_value),
            (DataValue::Uint64(value), DataValue::Float(other_value)) => {
                match u32::try_from(*value) {
                    Ok(value) => {
                        Ok((f64::from(value) - f64::from(*other_value)).abs() < f64::EPSILON)
                    }
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Uint64(value), DataValue::Double(other_value)) => {
                match u32::try_from(*value) {
                    Ok(value) => Ok((f64::from(value) - *other_value).abs() < f64::EPSILON),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Float(value), DataValue::Int32(other_value)) => {
                Ok((f64::from(*value) - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (DataValue::Float(value), DataValue::Int64(other_value)) => {
                match i32::try_from(*other_value) {
                    Ok(other_value) => {
                        Ok((f64::from(*value) - f64::from(other_value)).abs() < f64::EPSILON)
                    }
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Float(value), DataValue::Uint32(other_value)) => {
                Ok((f64::from(*value) - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (DataValue::Float(value), DataValue::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => {
                        Ok((f64::from(*value) - f64::from(other_value)).abs() < f64::EPSILON)
                    }
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Float(value), DataValue::Float(other_value)) => {
                // TODO: Implement better floating point comparison
                Ok((value - other_value).abs() < f32::EPSILON)
            }
            (DataValue::Float(value), DataValue::Double(other_value)) => {
                // TODO: Implement better floating point comparison
                Ok((f64::from(*value) - *other_value).abs() < f64::EPSILON)
            }
            (DataValue::Double(value), DataValue::Int32(other_value)) => {
                Ok((*value - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (DataValue::Double(value), DataValue::Int64(other_value)) => {
                match i32::try_from(*other_value) {
                    Ok(other_value) => Ok((*value - f64::from(other_value)).abs() < f64::EPSILON),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Double(value), DataValue::Uint32(other_value)) => {
                Ok((*value - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (DataValue::Double(value), DataValue::Uint64(other_value)) => {
                match u32::try_from(*other_value) {
                    Ok(other_value) => Ok((*value - f64::from(other_value)).abs() < f64::EPSILON),
                    Err(_) => Err(CastError {}),
                }
            }
            (DataValue::Double(value), DataValue::Float(other_value)) => {
                // TODO: Implement better floating point comparison
                Ok((*value - f64::from(*other_value)).abs() < f64::EPSILON)
            }
            (DataValue::Double(value), DataValue::Double(other_value)) => {
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
        DataValue::String("string".to_owned()).greater_than(&DataValue::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        DataValue::String("string".to_owned()).greater_than(&DataValue::Bool(false)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::String("string".to_owned())
            .greater_than(&DataValue::String("string2".to_owned())),
        Err(_)
    ));
    assert!(matches!(
        DataValue::String("string".to_owned()).greater_than(&DataValue::Int32(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::String("string".to_owned()).greater_than(&DataValue::Int64(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::String("string".to_owned()).greater_than(&DataValue::Uint32(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::String("string".to_owned()).greater_than(&DataValue::Uint64(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::String("string".to_owned()).greater_than(&DataValue::Float(100.0)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::String("string".to_owned()).greater_than(&DataValue::Double(100.0)),
        Err(_)
    ));
}

#[test]
fn test_not_available_greater_than() {
    assert!(matches!(
        DataValue::NotAvailable.greater_than(&DataValue::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        DataValue::NotAvailable.greater_than(&DataValue::Bool(false)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::NotAvailable.greater_than(&DataValue::String("string".to_owned())),
        Err(_)
    ));
    assert!(matches!(
        DataValue::NotAvailable.greater_than(&DataValue::Int32(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::NotAvailable.greater_than(&DataValue::Int64(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::NotAvailable.greater_than(&DataValue::Uint32(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::NotAvailable.greater_than(&DataValue::Uint64(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::NotAvailable.greater_than(&DataValue::Float(100.0)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::NotAvailable.greater_than(&DataValue::Double(100.0)),
        Err(_)
    ));
}

#[test]
fn test_bool_greater_than() {
    assert!(matches!(
        DataValue::Bool(false).greater_than(&DataValue::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Bool(false).greater_than(&DataValue::Bool(false)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Bool(false).greater_than(&DataValue::String("string".to_owned())),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Bool(false).greater_than(&DataValue::Int32(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Bool(false).greater_than(&DataValue::Int64(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Bool(false).greater_than(&DataValue::Uint32(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Bool(false).greater_than(&DataValue::Uint64(100)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Bool(false).greater_than(&DataValue::Float(100.0)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Bool(false).greater_than(&DataValue::Double(100.0)),
        Err(_)
    ));
}

#[test]
fn test_int32_greater_than() {
    // Comparison not possible
    //
    assert!(matches!(
        DataValue::Int32(5000).greater_than(&DataValue::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Int32(5000).greater_than(&DataValue::Bool(true)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Int32(5000).greater_than(&DataValue::String("string".to_owned())),
        Err(_)
    ));

    // Should be greater than
    //
    assert!(matches!(
        DataValue::Int32(5000).greater_than(&DataValue::Int32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int32(5000).greater_than(&DataValue::Int32(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int32(-4000).greater_than(&DataValue::Int32(-5000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int32(5000).greater_than(&DataValue::Int64(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int32(5000).greater_than(&DataValue::Int64(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int32(5000).greater_than(&DataValue::Uint32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int32(-4000).greater_than(&DataValue::Int64(-5000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int32(i32::MAX)
            .greater_than(&DataValue::Uint64(u64::try_from(i32::MAX - 1).unwrap())),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int32(30).greater_than(&DataValue::Uint64(20)),
        Ok(true)
    ));

    // Should not be greater than
    //

    assert!(matches!(
        DataValue::Int32(-5000).greater_than(&DataValue::Uint32(4000)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Int32(4000).greater_than(&DataValue::Uint32(5000)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Int32(20).greater_than(&DataValue::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Int32(10).greater_than(&DataValue::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Int32(-10).greater_than(&DataValue::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Int32(-10).greater_than(&DataValue::Uint64(u64::MAX)),
        Ok(false)
    ));
}

#[test]
fn test_int64_greater_than() {
    // Comparison not possible
    //

    assert!(matches!(
        DataValue::Int64(5000).greater_than(&DataValue::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Int64(5000).greater_than(&DataValue::Bool(true)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Int64(5000).greater_than(&DataValue::String("string".to_owned())),
        Err(_)
    ));

    // Should be greater than
    //

    assert!(matches!(
        DataValue::Int64(5000).greater_than(&DataValue::Int32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int64(5000).greater_than(&DataValue::Int32(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int64(-4000).greater_than(&DataValue::Int32(-5000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int64(5000).greater_than(&DataValue::Int64(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int64(5000).greater_than(&DataValue::Int64(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int64(5000).greater_than(&DataValue::Uint32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int64(-4000).greater_than(&DataValue::Int64(-5000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int64(i64::from(i32::MAX))
            .greater_than(&DataValue::Uint64(u64::try_from(i32::MAX - 1).unwrap())),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Int64(30).greater_than(&DataValue::Uint64(20)),
        Ok(true)
    ));

    // Should not be greater than
    //

    assert!(matches!(
        DataValue::Int64(-10).greater_than(&DataValue::Uint64(u64::MAX)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Int64(-5000).greater_than(&DataValue::Uint32(4000)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Int64(4000).greater_than(&DataValue::Uint32(5000)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Int64(20).greater_than(&DataValue::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Int64(10).greater_than(&DataValue::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Int64(-10).greater_than(&DataValue::Uint64(20)),
        Ok(false)
    ));
}

#[test]
fn test_uint32_greater_than() {
    // Comparison not possible
    //

    assert!(matches!(
        DataValue::Uint32(5000).greater_than(&DataValue::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Uint32(5000).greater_than(&DataValue::Bool(true)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Uint32(5000).greater_than(&DataValue::String("string".to_owned())),
        Err(_)
    ));

    // Should be greater than
    //

    assert!(matches!(
        DataValue::Uint32(5000).greater_than(&DataValue::Int32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint32(5000).greater_than(&DataValue::Int32(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint32(5000).greater_than(&DataValue::Int64(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint32(5000).greater_than(&DataValue::Int64(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint32(5000).greater_than(&DataValue::Float(4000.0)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint32(5000).greater_than(&DataValue::Double(-4000.0)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint32(5000).greater_than(&DataValue::Uint32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint32(u32::MAX).greater_than(&DataValue::Int32(-5000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint32(u32::MAX).greater_than(&DataValue::Int64(i64::MIN)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint32(30).greater_than(&DataValue::Uint64(20)),
        Ok(true)
    ));

    // Should not be greater than
    //

    assert!(matches!(
        DataValue::Uint32(4000).greater_than(&DataValue::Int32(5000)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Uint32(4000).greater_than(&DataValue::Uint32(5000)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Uint32(20).greater_than(&DataValue::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Uint32(10).greater_than(&DataValue::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Uint32(20).greater_than(&DataValue::Float(20.0)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Uint32(10).greater_than(&DataValue::Double(20.0)),
        Ok(false)
    ));
}

#[test]
fn test_uint64_greater_than() {
    // Comparison not possible
    //

    assert!(matches!(
        DataValue::Uint64(5000).greater_than(&DataValue::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Uint64(5000).greater_than(&DataValue::Bool(true)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Uint64(5000).greater_than(&DataValue::String("string".to_owned())),
        Err(_)
    ));

    // Should be greater than
    //
    assert!(matches!(
        DataValue::Uint64(5000).greater_than(&DataValue::Int32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint64(5000).greater_than(&DataValue::Int32(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint64(5000).greater_than(&DataValue::Int64(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint64(5000).greater_than(&DataValue::Int64(-4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint64(5000).greater_than(&DataValue::Float(4000.0)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint64(5000).greater_than(&DataValue::Double(-4000.0)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint64(5000).greater_than(&DataValue::Uint32(4000)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint64(u64::MAX).greater_than(&DataValue::Int64(i64::MAX - 1)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Uint64(30).greater_than(&DataValue::Uint64(20)),
        Ok(true)
    ));

    // Should not be greater than
    //

    assert!(matches!(
        DataValue::Uint64(4000).greater_than(&DataValue::Uint32(5000)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Uint64(20).greater_than(&DataValue::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Uint64(10).greater_than(&DataValue::Uint64(20)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Uint64(20).greater_than(&DataValue::Float(20.0)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Uint64(10).greater_than(&DataValue::Double(20.0)),
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
        DataValue::Float(5000.0).greater_than(&DataValue::NotAvailable),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Float(5000.0).greater_than(&DataValue::Bool(true)),
        Err(_)
    ));
    assert!(matches!(
        DataValue::Float(5000.0).greater_than(&DataValue::String("string".to_owned())),
        Err(_)
    ));

    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Int32(32)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Int64(32)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Uint32(32)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Uint64(32)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Float(32.0)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Double(32.0)),
        Ok(true)
    ));

    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Int32(-32)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Int64(-32)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Uint32(32)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Uint64(32)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Float(-32.0)),
        Ok(true)
    ));
    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Double(-32.0)),
        Ok(true)
    ));

    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Int32(33)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Int64(33)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Uint32(33)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Uint64(33)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Float(33.0)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(32.0).equals(&DataValue::Double(33.0)),
        Ok(false)
    ));

    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Int32(-33)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Int64(-33)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Uint32(33)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Uint64(33)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Float(-33.0)),
        Ok(false)
    ));
    assert!(matches!(
        DataValue::Float(-32.0).equals(&DataValue::Double(-33.0)),
        Ok(false)
    ));
}
