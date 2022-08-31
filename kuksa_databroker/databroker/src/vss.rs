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

use tracing::{debug};

use serde_json::Value;

#[derive(Debug)]
pub struct MetadataEntry {
    pub name: String,
    pub data_type: types::DataType,
    pub description: Option<String>,
    pub default: Option<Value>,
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

pub fn parse_vss(data: &str) -> Result<Vec<MetadataEntry>, Error> {
    // Parse the string of data into serde_json::Value.
    match serde_json::from_str::<Value>(data) {
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

fn parse_entry(name: &str, entry: &Value, entries: &mut Vec<MetadataEntry>) -> Result<(), Error> {
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
                if !entry.contains_key("datatype") {
                    return Err(Error::ParseError(format!(
                        "Key \"datatype\" not found in {}",
                        name
                    )));
                }
                let data_type = parse_data_type(entry["datatype"].as_str())?;
                entries.push(MetadataEntry {
                    name: name.to_owned(),
                    data_type,
                    description: entry["description"].as_str().map(|s| s.to_owned()),
                    default: match entry.get("default") {
                        Some(x) =>  {
                            debug!("Parsing default value {} for path {}",x,name);
                            Some(x.clone())
                        },
                        None => None,
                    }
                });
            }
        }
        Ok(())
    } else {
        Err(Error::ParseError("Expected an object".to_owned()))
    }
}

fn parse_data_type(data_type: Option<&str>) -> Result<types::DataType, Error> {
    match data_type {
        Some(data_type) => match data_type {
            "string" => Ok(types::DataType::String),
            "boolean" => Ok(types::DataType::Bool),
            "int8" => Ok(types::DataType::Int8),
            "int16" => Ok(types::DataType::Int16),
            "int32" => Ok(types::DataType::Int32),
            "int64" => Ok(types::DataType::Int64),
            "uint8" => Ok(types::DataType::Uint8),
            "uint16" => Ok(types::DataType::Uint16),
            "uint32" => Ok(types::DataType::Uint32),
            "uint64" => Ok(types::DataType::Uint64),
            "float" => Ok(types::DataType::Float),
            "double" => Ok(types::DataType::Double),
            "string[]" => Ok(types::DataType::StringArray),
            "boolean[]" => Ok(types::DataType::BoolArray),
            "int8[]" => Ok(types::DataType::Int8Array),
            "int16[]" => Ok(types::DataType::Int16Array),
            "int32[]" => Ok(types::DataType::Int32Array),
            "int64[]" => Ok(types::DataType::Int64Array),
            "uint8[]" => Ok(types::DataType::Uint8Array),
            "uint16[]" => Ok(types::DataType::Uint16Array),
            "uint32[]" => Ok(types::DataType::Uint32Array),
            "uint64[]" => Ok(types::DataType::Uint64Array),
            "float[]" => Ok(types::DataType::FloatArray),
            "double[]" => Ok(types::DataType::DoubleArray),
            _ => Err(Error::ParseError(format!(
                "Unrecognized data type: {}",
                data_type
            ))),
        },
        None => Err(Error::ParseError(
            "Key \"datatype\" is not a string".to_owned(),
        )),
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
