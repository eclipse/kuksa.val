/********************************************************************************
* Copyright (c) 2022, 2023 Contributors to the Eclipse Foundation
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

#![allow(unknown_lints)]
#![allow(clippy::derive_partial_eq_without_eq)]
pub mod sdv {
    pub mod databroker {
        pub mod v1 {
            tonic::include_proto!("sdv.databroker.v1");
        }
    }
}

pub mod kuksa {
    pub mod val {
        pub mod v1 {
            tonic::include_proto!("kuksa.val.v1");

            use datapoint::Value;
            use std::{any::Any, fmt::Display, str::FromStr};

            #[derive(Debug)]
            pub struct ParsingError {
                message: String,
            }

            impl ParsingError {
                pub fn new<T: Into<String>>(message: T) -> Self {
                    ParsingError {
                        message: message.into(),
                    }
                }
            }

            impl Display for ParsingError {
                fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                    self.message.fmt(f)
                }
            }

            impl std::error::Error for ParsingError {}

            impl FromStr for DataType {
                type Err = ParsingError;
                fn from_str(s: &str) -> Result<Self, Self::Err> {
                    match s.to_lowercase().as_str() {
                        "string" => Ok(DataType::String),
                        "string[]" => Ok(DataType::StringArray),
                        "bool" => Ok(DataType::Boolean),
                        "bool[]" => Ok(DataType::BooleanArray),
                        "int8" => Ok(DataType::Int8),
                        "int8[]" => Ok(DataType::Int8Array),
                        "int16" => Ok(DataType::Int16),
                        "int16[]" => Ok(DataType::Int16Array),
                        "int32" => Ok(DataType::Int32),
                        "int32[]" => Ok(DataType::Int32Array),
                        "int64" => Ok(DataType::Int64),
                        "int64[]" => Ok(DataType::Int64Array),
                        "uint8" => Ok(DataType::Uint8),
                        "uint8[]" => Ok(DataType::Uint8Array),
                        "uint16" => Ok(DataType::Uint16),
                        "uint16[]" => Ok(DataType::Uint16Array),
                        "uint32" => Ok(DataType::Uint32),
                        "uint32[]" => Ok(DataType::Uint32Array),
                        "uint64" => Ok(DataType::Uint64),
                        "uint64[]" => Ok(DataType::Uint64Array),
                        "float" => Ok(DataType::Float),
                        "float[]" => Ok(DataType::FloatArray),
                        "double" => Ok(DataType::Double),
                        "double[]" => Ok(DataType::DoubleArray),
                        "timestamp" => Ok(DataType::Timestamp),
                        "timestamp[]" => Ok(DataType::TimestampArray),
                        _ => Err(ParsingError::new(format!("unsupported data type '{s}'"))),
                    }
                }
            }

            impl Value {
                pub fn new<T: Into<DataType>>(
                    vss_type: T,
                    value: &str,
                ) -> Result<Self, ParsingError> {
                    let dt: DataType = vss_type.into();
                    match dt {
                        DataType::String => Ok(Value::String(value.to_string())),
                        DataType::Boolean => value
                            .parse::<bool>()
                            .map(Value::Bool)
                            .map_err(|e| ParsingError::new(e.to_string())),
                        DataType::Int8 => value
                            .parse::<i8>()
                            .map(|v| Value::Int32(v as i32))
                            .map_err(|e| ParsingError::new(e.to_string())),
                        DataType::Int16 => value
                            .parse::<i16>()
                            .map(|v| Value::Int32(v as i32))
                            .map_err(|e| ParsingError::new(e.to_string())),
                        DataType::Int32 => value
                            .parse::<i32>()
                            .map(Value::Int32)
                            .map_err(|e| ParsingError::new(e.to_string())),
                        DataType::Int64 => value
                            .parse::<i64>()
                            .map(Value::Int64)
                            .map_err(|e| ParsingError::new(e.to_string())),
                        DataType::Uint8 => value
                            .parse::<u8>()
                            .map(|v| Value::Uint32(v as u32))
                            .map_err(|e| ParsingError::new(e.to_string())),
                        DataType::Uint16 => value
                            .parse::<u16>()
                            .map(|v| Value::Uint32(v as u32))
                            .map_err(|e| ParsingError::new(e.to_string())),
                        DataType::Uint32 => value
                            .parse::<u32>()
                            .map(Value::Uint32)
                            .map_err(|e| ParsingError::new(e.to_string())),
                        DataType::Uint64 => value
                            .parse::<u64>()
                            .map(Value::Uint64)
                            .map_err(|e| ParsingError::new(e.to_string())),
                        DataType::Float => value
                            .parse::<f32>()
                            .map(Value::Float)
                            .map_err(|e| ParsingError::new(e.to_string())),
                        DataType::Double => value
                            .parse::<f64>()
                            .map(Value::Double)
                            .map_err(|e| ParsingError::new(e.to_string())),
                        _ => Err(ParsingError::new(format!(
                            "data type '{:?}' not supported for parsing string into typed value",
                            dt.type_id()
                        ))),
                    }
                }
            }
        }
    }
}
