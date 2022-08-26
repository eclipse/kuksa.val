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

use std::fmt;

use crate::types;

#[derive(Debug)]
pub struct VssEntry {
    pub name: String,
    pub data_type: types::DataType,
    pub description: Option<String>,
    pub default: Option<types::DataValue>,
}

enum VssEntryType {
    Branch,
    Sensor,
    Actuator,
    Attribute,
}

#[derive(Debug)]
pub enum Error {
    ParseError(String),
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Error::ParseError(error) => write!(f, "{}", error),
        }
    }
}

impl std::error::Error for Error {}

pub fn parse_vss(data: &str) -> Result<Vec<VssEntry>, Error> {
    // Parse the string of data into serde_json::Value.
    match serde_json::from_str::<serde_json::Value>(data) {
        Ok(root) => {
            if let Some(root) = root.as_object() {
                let mut entries = Vec::new();
                for (key, value) in root {
                    parse_entry(key, value, &mut entries)?;
                }
                Ok(entries)
            } else {
                Err(Error::ParseError("Expected object at root".to_owned()))
            }
        }
        Err(err) => Err(Error::ParseError(err.to_string())),
    }
}

fn parse_entry(
    name: &str,
    entry: &serde_json::Value,
    entries: &mut Vec<VssEntry>,
) -> Result<(), Error> {
    if let Some(entry) = entry.as_object() {
        if !entry.contains_key("type") {
            return Err(Error::ParseError(format!(
                "Key \"type\" not found in {}",
                name
            )));
        }
        let entry_type = parse_entry_type(entry["type"].as_str())?;
        match entry_type {
            VssEntryType::Branch => {
                if let Some(children) = entry["children"].as_object() {
                    for (key, value) in children {
                        let name = format!("{}.{}", name, key);
                        parse_entry(&name, value, entries)?;
                    }
                } else {
                    return Err(Error::ParseError("Expected an object".to_owned()));
                }
            }
            VssEntryType::Sensor | VssEntryType::Actuator | VssEntryType::Attribute => {
                let data_type = parse_data_type(name, entry)?;
                let default_value = parse_default_value(name, &data_type, entry)?;
                entries.push(VssEntry {
                    name: name.to_owned(),
                    data_type,
                    description: entry["description"].as_str().map(|s| s.to_owned()),
                    default: default_value,
                });
            }
        }
        Ok(())
    } else {
        Err(Error::ParseError("Expected an object".to_owned()))
    }
}

fn parse_default_value(
    name: &str,
    data_type: &types::DataType,
    entry: &serde_json::Map<String, serde_json::Value>,
) -> Result<Option<types::DataValue>, Error> {
    match entry.get("default") {
        Some(value) => match data_type {
            types::DataType::String => match serde_json::from_value::<String>(value.to_owned()) {
                Ok(value) => Ok(Some(types::DataValue::String(value))),
                Err(_) => Err(Error::ParseError(format!(
                    "Could not parse default value ({}) as {:?} for {}",
                    value, data_type, name
                ))),
            },
            types::DataType::Bool => match serde_json::from_value::<bool>(value.to_owned()) {
                Ok(value) => Ok(Some(types::DataValue::Bool(value))),
                Err(_) => Err(Error::ParseError(format!(
                    "Could not parse default value ({}) as {:?} for {}",
                    value, data_type, name
                ))),
            },
            types::DataType::Int8 | types::DataType::Int16 | types::DataType::Int32 => {
                match serde_json::from_value::<i32>(value.to_owned()) {
                    Ok(value) => Ok(Some(types::DataValue::Int32(value))),
                    Err(_) => Err(Error::ParseError(format!(
                        "Could not parse default value ({}) as {:?} for {}",
                        value, data_type, name
                    ))),
                }
            }
            types::DataType::Int64 => match serde_json::from_value::<i64>(value.to_owned()) {
                Ok(value) => Ok(Some(types::DataValue::Int64(value))),
                Err(_) => Err(Error::ParseError(format!(
                    "Could not parse default value ({}) as {:?} for {}",
                    value, data_type, name
                ))),
            },
            types::DataType::Uint8 | types::DataType::Uint16 | types::DataType::Uint32 => {
                match serde_json::from_value::<u32>(value.to_owned()) {
                    Ok(value) => Ok(Some(types::DataValue::Uint32(value))),
                    Err(_) => Err(Error::ParseError(format!(
                        "Could not parse default value ({}) as {:?} for {}",
                        value, data_type, name
                    ))),
                }
            }
            types::DataType::Uint64 => match serde_json::from_value::<u64>(value.to_owned()) {
                Ok(value) => Ok(Some(types::DataValue::Uint64(value))),
                Err(_) => Err(Error::ParseError(format!(
                    "Could not parse default value ({}) as {:?} for {}",
                    value, data_type, name
                ))),
            },
            types::DataType::Float => match serde_json::from_value::<f32>(value.to_owned()) {
                Ok(value) => Ok(Some(types::DataValue::Float(value))),
                Err(_) => Err(Error::ParseError(format!(
                    "Could not parse default value ({}) as {:?} for {}",
                    value, data_type, name
                ))),
            },
            types::DataType::Double => match serde_json::from_value::<f64>(value.to_owned()) {
                Ok(value) => Ok(Some(types::DataValue::Double(value))),
                Err(_) => Err(Error::ParseError(format!(
                    "Could not parse default value ({}) as {:?} for {}",
                    value, data_type, name
                ))),
            },
            // types::DataType::Timestamp => todo!(),
            types::DataType::StringArray => {
                match serde_json::from_value::<Vec<String>>(value.to_owned()) {
                    Ok(value) => Ok(Some(types::DataValue::StringArray(value))),
                    Err(_) => Err(Error::ParseError(format!(
                        "Could not parse default value ({}) as {:?} for {}",
                        value, data_type, name
                    ))),
                }
            }
            types::DataType::BoolArray => {
                match serde_json::from_value::<Vec<bool>>(value.to_owned()) {
                    Ok(value) => Ok(Some(types::DataValue::BoolArray(value))),
                    Err(_) => Err(Error::ParseError(format!(
                        "Could not parse default value ({}) as {:?} for {}",
                        value, data_type, name
                    ))),
                }
            }
            types::DataType::Int8Array
            | types::DataType::Int16Array
            | types::DataType::Int32Array => {
                match serde_json::from_value::<Vec<i32>>(value.to_owned()) {
                    Ok(value) => Ok(Some(types::DataValue::Int32Array(value))),
                    Err(_) => Err(Error::ParseError(format!(
                        "Could not parse default value ({}) as {:?} for {}",
                        value, data_type, name
                    ))),
                }
            }
            types::DataType::Int64Array => {
                match serde_json::from_value::<Vec<i64>>(value.to_owned()) {
                    Ok(value) => Ok(Some(types::DataValue::Int64Array(value))),
                    Err(_) => Err(Error::ParseError(format!(
                        "Could not parse default value ({}) as {:?} for {}",
                        value, data_type, name
                    ))),
                }
            }
            types::DataType::Uint8Array
            | types::DataType::Uint16Array
            | types::DataType::Uint32Array => {
                match serde_json::from_value::<Vec<u32>>(value.to_owned()) {
                    Ok(value) => Ok(Some(types::DataValue::Uint32Array(value))),
                    Err(_) => Err(Error::ParseError(format!(
                        "Could not parse default value ({}) as {:?} for {}",
                        value, data_type, name
                    ))),
                }
            }
            types::DataType::Uint64Array => {
                match serde_json::from_value::<Vec<u64>>(value.to_owned()) {
                    Ok(value) => Ok(Some(types::DataValue::Uint64Array(value))),
                    Err(_) => Err(Error::ParseError(format!(
                        "Could not parse default value ({}) as {:?} for {}",
                        value, data_type, name
                    ))),
                }
            }
            types::DataType::FloatArray => {
                match serde_json::from_value::<Vec<f32>>(value.to_owned()) {
                    Ok(value) => Ok(Some(types::DataValue::FloatArray(value))),
                    Err(_) => Err(Error::ParseError(format!(
                        "Could not parse default value ({}) as {:?} for {}",
                        value, data_type, name
                    ))),
                }
            }
            types::DataType::DoubleArray => {
                match serde_json::from_value::<Vec<f64>>(value.to_owned()) {
                    Ok(value) => Ok(Some(types::DataValue::DoubleArray(value))),
                    Err(_) => Err(Error::ParseError(format!(
                        "Could not parse default value ({}) as {:?} for {}",
                        value, data_type, name
                    ))),
                }
            }
            // types::DataType::TimestampArray => todo!(),
            _ => Err(Error::ParseError(format!(
                "Could not parse default value ({}) as {:?} for {} (NOT IMPLEMENTED)",
                value, data_type, name
            ))),
        },
        None => Ok(None), // "default" is not mandatory,
    }
}

fn parse_data_type(
    name: &str,
    entry: &serde_json::Map<String, serde_json::Value>,
) -> Result<types::DataType, Error> {
    match entry.get("datatype") {
        Some(data_type) => match data_type.as_str() {
            Some("string") => Ok(types::DataType::String),
            Some("boolean") => Ok(types::DataType::Bool),
            Some("int8") => Ok(types::DataType::Int8),
            Some("int16") => Ok(types::DataType::Int16),
            Some("int32") => Ok(types::DataType::Int32),
            Some("int64") => Ok(types::DataType::Int64),
            Some("uint8") => Ok(types::DataType::Uint8),
            Some("uint16") => Ok(types::DataType::Uint16),
            Some("uint32") => Ok(types::DataType::Uint32),
            Some("uint64") => Ok(types::DataType::Uint64),
            Some("float") => Ok(types::DataType::Float),
            Some("double") => Ok(types::DataType::Double),
            Some("string[]") => Ok(types::DataType::StringArray),
            Some("boolean[]") => Ok(types::DataType::BoolArray),
            Some("int8[]") => Ok(types::DataType::Int8Array),
            Some("int16[]") => Ok(types::DataType::Int16Array),
            Some("int32[]") => Ok(types::DataType::Int32Array),
            Some("int64[]") => Ok(types::DataType::Int64Array),
            Some("uint8[]") => Ok(types::DataType::Uint8Array),
            Some("uint16[]") => Ok(types::DataType::Uint16Array),
            Some("uint32[]") => Ok(types::DataType::Uint32Array),
            Some("uint64[]") => Ok(types::DataType::Uint64Array),
            Some("float[]") => Ok(types::DataType::FloatArray),
            Some("double[]") => Ok(types::DataType::DoubleArray),
            None => Err(Error::ParseError(
                "Key \"datatype\" is not a string".to_owned(),
            )),
            _ => Err(Error::ParseError(format!(
                "Unrecognized data type: {}",
                data_type
            ))),
        },
        None => Err(Error::ParseError(format!(
            "Key \"datatype\" not found in {}",
            name
        ))),
    }
}

fn parse_entry_type(entry_type: Option<&str>) -> Result<VssEntryType, Error> {
    match entry_type {
        Some(entry_type) => match entry_type {
            "branch" => Ok(VssEntryType::Branch),
            "actuator" => Ok(VssEntryType::Actuator),
            "sensor" => Ok(VssEntryType::Sensor),
            "attribute" => Ok(VssEntryType::Attribute),
            _ => Err(Error::ParseError(format!(
                "Unknown entry type: {}",
                entry_type
            ))),
        },
        None => Err(Error::ParseError("Key \"type\" is not a string".to_owned())),
    }
}

#[test]
fn test_parse_vss() {
    let data = r#"
{
    "Vehicle": {
        "children": {
            "ADAS": {
                "children": {
                    "ABS": {
                        "children": {
                            "Error": {
                                "datatype": "boolean",
                                "description": "Indicates if ABS incurred an error condition. True = Error. False = No Error.",
                                "type": "sensor",
                                "uuid": "cd2b0e86aa1f5021a9bb7f6bda1cbe0f"
                            },
                            "IsActive": {
                                "datatype": "boolean",
                                "description": "Indicates if ABS is enabled. True = Enabled. False = Disabled.",
                                "type": "actuator",
                                "uuid": "433b7039199357178688197d6e264725"
                            },
                            "IsEngaged": {
                                "datatype": "boolean",
                                "description": "Indicates if ABS is currently regulating brake pressure. True = Engaged. False = Not Engaged.",
                                "type": "sensor",
                                "uuid": "6dd21979a2225e31940dc2ece1aa9a04"
                            }
                        },
                        "description": "Antilock Braking System signals",
                        "type": "branch",
                        "uuid": "219270ef27c4531f874bbda63743b330"
                    }
                },
                "description": "All Advanced Driver Assist Systems data.",
                "type": "branch",
                "uuid": "14c2b2e1297b513197d320a5ce58f42e"
            }
        },
        "description": "High-level vehicle data.",
        "type": "branch",
        "uuid": "ccc825f94139544dbb5f4bfd033bece6"
    }
}"#;

    match parse_vss(data) {
        Ok(entries) => {
            assert_eq!(entries.len(), 3);
            assert_eq!(entries[0].name, "Vehicle.ADAS.ABS.Error");
            assert_eq!(entries[0].data_type, types::DataType::Bool);
            assert_eq!(entries[1].name, "Vehicle.ADAS.ABS.IsActive");
            assert_eq!(entries[1].data_type, types::DataType::Bool);
            assert_eq!(entries[2].name, "Vehicle.ADAS.ABS.IsEngaged");
            assert_eq!(entries[2].data_type, types::DataType::Bool);
        }
        Err(err) => panic!("Expected parsing to work: {:?}", err),
    }
}
