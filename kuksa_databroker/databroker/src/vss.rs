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

use std::collections::{BTreeMap, HashMap};
use std::fmt;

use serde::Deserialize;

use crate::types;

#[derive(Debug, Deserialize)]
struct RootEntry(HashMap<String, Entry>);

// While it's possible to use serde's "tagged enum" for parsing VSS json,
// doing that breaks serde's ability to track line numbers when reporting
// parsing errors. (https://github.com/dtolnay/serde-yaml/issues/153)
//
// #[derive(Debug, Deserialize)]
// #[serde(tag = "type", rename_all = "camelCase")]
// pub enum Entry {
//     Branch(BranchEntry),
//     Actuator(ActuatorEntry),
//     Attribute(AttributeEntry),
//     Sensor(SensorEntry),
// }
//
// Instead, a single Entry consisting of the superset of possible fields is used.
//
#[derive(Debug, Deserialize)]
pub struct Entry {
    // all
    #[serde(rename = "type")]
    entry_type: EntryType,
    description: String,
    comment: Option<String>,

    // branch only
    children: Option<HashMap<String, Entry>>,

    // all data entry types
    #[serde(rename = "datatype")]
    data_type: Option<DataType>,
    unit: Option<String>,
    min: Option<serde_json::Value>,
    max: Option<serde_json::Value>,
    allowed: Option<Vec<serde_json::Value>>,

    // attribute entry type only
    default: Option<serde_json::Value>,
}

pub struct DataEntry {
    pub data_type: types::DataType,
    pub entry_type: types::EntryType,
    pub description: String,
    pub comment: Option<String>,
    pub unit: Option<String>,
    pub min: Option<types::DataValue>,
    pub max: Option<types::DataValue>,
    pub allowed: Option<types::DataValue>,
    pub default: Option<types::DataValue>,
}

#[derive(Debug, Deserialize)]
pub enum EntryType {
    #[serde(rename = "actuator")]
    Actuator,

    #[serde(rename = "attribute")]
    Attribute,

    #[serde(rename = "branch")]
    Branch,

    #[serde(rename = "sensor")]
    Sensor,
}

#[derive(Debug, Deserialize)]
pub enum DataType {
    #[serde(rename = "string")]
    String,
    #[serde(rename = "boolean")]
    Boolean,
    #[serde(rename = "int8")]
    Int8,
    #[serde(rename = "int16")]
    Int16,
    #[serde(rename = "int32")]
    Int32,
    #[serde(rename = "int64")]
    Int64,
    #[serde(rename = "uint8")]
    Uint8,
    #[serde(rename = "uint16")]
    Uint16,
    #[serde(rename = "uint32")]
    Uint32,
    #[serde(rename = "uint64")]
    Uint64,
    #[serde(rename = "float")]
    Float,
    #[serde(rename = "double")]
    Double,
    #[serde(rename = "string[]")]
    StringArray,
    #[serde(rename = "boolean[]")]
    BooleanArray,
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

impl From<serde_json::Error> for Error {
    fn from(from: serde_json::Error) -> Self {
        Error::ParseError(from.to_string())
    }
}

impl std::error::Error for Error {}

impl From<DataType> for types::DataType {
    fn from(from: DataType) -> Self {
        match from {
            DataType::String => types::DataType::String,
            DataType::Boolean => types::DataType::Bool,
            DataType::Int8 => types::DataType::Int8,
            DataType::Int16 => types::DataType::Int16,
            DataType::Int32 => types::DataType::Int32,
            DataType::Int64 => types::DataType::Int64,
            DataType::Uint8 => types::DataType::Uint8,
            DataType::Uint16 => types::DataType::Uint16,
            DataType::Uint32 => types::DataType::Uint32,
            DataType::Uint64 => types::DataType::Uint64,
            DataType::Float => types::DataType::Float,
            DataType::Double => types::DataType::Double,
            DataType::StringArray => types::DataType::StringArray,
            DataType::BooleanArray => types::DataType::BoolArray,
            DataType::Int8Array => types::DataType::Int8Array,
            DataType::Int16Array => types::DataType::Int16Array,
            DataType::Int32Array => types::DataType::Int32Array,
            DataType::Int64Array => types::DataType::Int64Array,
            DataType::Uint8Array => types::DataType::Uint8Array,
            DataType::Uint16Array => types::DataType::Uint16Array,
            DataType::Uint32Array => types::DataType::Uint32Array,
            DataType::Uint64Array => types::DataType::Uint64Array,
            DataType::FloatArray => types::DataType::FloatArray,
            DataType::DoubleArray => types::DataType::DoubleArray,
        }
    }
}

fn try_from_json_array(
    array: Option<Vec<serde_json::Value>>,
    data_type: &types::DataType,
) -> Result<Option<types::DataValue>, Error> {
    match array {
        Some(array) => {
            let value = Some(serde_json::Value::Array(array));
            match data_type {
                types::DataType::String => {
                    try_from_json_value(value, &types::DataType::StringArray)
                }
                types::DataType::Bool => try_from_json_value(value, &types::DataType::BoolArray),
                types::DataType::Int8 => try_from_json_value(value, &types::DataType::Int8Array),
                types::DataType::Int16 => try_from_json_value(value, &types::DataType::Int16Array),
                types::DataType::Int32 => try_from_json_value(value, &types::DataType::Int32Array),
                types::DataType::Int64 => try_from_json_value(value, &types::DataType::Int64Array),
                types::DataType::Uint8 => try_from_json_value(value, &types::DataType::Uint8Array),
                types::DataType::Uint16 => {
                    try_from_json_value(value, &types::DataType::Uint16Array)
                }
                types::DataType::Uint32 => {
                    try_from_json_value(value, &types::DataType::Uint32Array)
                }
                types::DataType::Uint64 => {
                    try_from_json_value(value, &types::DataType::Uint64Array)
                }
                types::DataType::Float => try_from_json_value(value, &types::DataType::FloatArray),
                types::DataType::Double => {
                    try_from_json_value(value, &types::DataType::DoubleArray)
                }
                types::DataType::Timestamp => {
                    try_from_json_value(value, &types::DataType::TimestampArray)
                }
                types::DataType::StringArray
                | types::DataType::BoolArray
                | types::DataType::Int8Array
                | types::DataType::Int16Array
                | types::DataType::Int32Array
                | types::DataType::Int64Array
                | types::DataType::Uint8Array
                | types::DataType::Uint16Array
                | types::DataType::Uint32Array
                | types::DataType::Uint64Array
                | types::DataType::FloatArray
                | types::DataType::DoubleArray
                | types::DataType::TimestampArray => try_from_json_value(value, data_type),
            }
        }
        None => Ok(None),
    }
}

fn try_from_json_value(
    value: Option<serde_json::Value>,
    data_type: &types::DataType,
) -> Result<Option<types::DataValue>, Error> {
    match value {
        Some(value) => match data_type {
            types::DataType::String => serde_json::from_value::<String>(value)
                .map(|value| Some(types::DataValue::String(value)))
                .map_err(|err| err.into()),
            types::DataType::Bool => serde_json::from_value::<bool>(value)
                .map(|value| Some(types::DataValue::Bool(value)))
                .map_err(|err| err.into()),
            types::DataType::Int8 => serde_json::from_value::<i8>(value)
                .map(|value| Some(types::DataValue::Int32(value.into())))
                .map_err(|err| err.into()),
            types::DataType::Int16 => serde_json::from_value::<i16>(value)
                .map(|value| Some(types::DataValue::Int32(value.into())))
                .map_err(|err| err.into()),
            types::DataType::Int32 => serde_json::from_value::<i32>(value)
                .map(|value| Some(types::DataValue::Int32(value)))
                .map_err(|err| err.into()),
            types::DataType::Int64 => serde_json::from_value::<i64>(value)
                .map(|value| Some(types::DataValue::Int64(value)))
                .map_err(|err| err.into()),
            types::DataType::Uint8 => serde_json::from_value::<u8>(value)
                .map(|value| Some(types::DataValue::Uint32(value.into())))
                .map_err(|err| err.into()),
            types::DataType::Uint16 => serde_json::from_value::<u16>(value)
                .map(|value| Some(types::DataValue::Uint32(value.into())))
                .map_err(|err| err.into()),
            types::DataType::Uint32 => serde_json::from_value::<u32>(value)
                .map(|value| Some(types::DataValue::Uint32(value)))
                .map_err(|err| err.into()),
            types::DataType::Uint64 => serde_json::from_value::<u64>(value)
                .map(|value| Some(types::DataValue::Uint64(value)))
                .map_err(|err| err.into()),
            types::DataType::Float => serde_json::from_value::<f32>(value)
                .map(|value| Some(types::DataValue::Float(value)))
                .map_err(|err| err.into()),
            types::DataType::Double => serde_json::from_value::<f64>(value)
                .map(|value| Some(types::DataValue::Double(value)))
                .map_err(|err| err.into()),
            types::DataType::Timestamp => {
                Err(Error::ParseError("timestamp is not supported".to_owned()))
            }
            types::DataType::StringArray => serde_json::from_value::<Vec<String>>(value)
                .map(|array| Some(types::DataValue::StringArray(array)))
                .map_err(|err| err.into()),
            types::DataType::BoolArray => serde_json::from_value::<Vec<bool>>(value)
                .map(|array| Some(types::DataValue::BoolArray(array)))
                .map_err(|err| err.into()),
            types::DataType::Int8Array => serde_json::from_value::<Vec<i8>>(value)
                .map(|array| {
                    Some(types::DataValue::Int32Array(
                        array.iter().map(|value| (*value).into()).collect(),
                    ))
                })
                .map_err(|err| err.into()),
            types::DataType::Int16Array => serde_json::from_value::<Vec<i16>>(value)
                .map(|array| {
                    Some(types::DataValue::Int32Array(
                        array.iter().map(|value| (*value).into()).collect(),
                    ))
                })
                .map_err(|err| err.into()),
            types::DataType::Int32Array => serde_json::from_value::<Vec<i32>>(value)
                .map(|array| Some(types::DataValue::Int32Array(array)))
                .map_err(|err| err.into()),
            types::DataType::Int64Array => serde_json::from_value::<Vec<i64>>(value)
                .map(|array| Some(types::DataValue::Int64Array(array)))
                .map_err(|err| err.into()),
            types::DataType::Uint8Array => serde_json::from_value::<Vec<u8>>(value)
                .map(|array| {
                    Some(types::DataValue::Uint32Array(
                        array.iter().map(|value| (*value).into()).collect(),
                    ))
                })
                .map_err(|err| err.into()),
            types::DataType::Uint16Array => serde_json::from_value::<Vec<u16>>(value)
                .map(|array| {
                    Some(types::DataValue::Uint32Array(
                        array.iter().map(|value| (*value).into()).collect(),
                    ))
                })
                .map_err(|err| err.into()),
            types::DataType::Uint32Array => serde_json::from_value::<Vec<u32>>(value)
                .map(|array| Some(types::DataValue::Uint32Array(array)))
                .map_err(|err| err.into()),
            types::DataType::Uint64Array => serde_json::from_value::<Vec<u64>>(value)
                .map(|array| Some(types::DataValue::Uint64Array(array)))
                .map_err(|err| err.into()),
            types::DataType::FloatArray => serde_json::from_value::<Vec<f32>>(value)
                .map(|array| Some(types::DataValue::FloatArray(array)))
                .map_err(|err| err.into()),
            types::DataType::DoubleArray => serde_json::from_value::<Vec<f64>>(value)
                .map(|array| Some(types::DataValue::DoubleArray(array)))
                .map_err(|err| err.into()),
            types::DataType::TimestampArray => Err(Error::ParseError(
                "timestamp array is not supported".to_owned(),
            )),
        },
        None => Ok(None),
    }
}

fn flatten_vss_tree(root: RootEntry) -> Result<BTreeMap<String, DataEntry>, Error> {
    let mut entries = BTreeMap::new();

    for (path, entry) in root.0 {
        add_entry(&mut entries, path, entry)?;
    }
    Ok(entries)
}

fn add_entry(
    entries: &mut BTreeMap<String, DataEntry>,
    path: String,
    entry: Entry,
) -> Result<(), Error> {
    match entry.entry_type {
        EntryType::Branch => match entry.children {
            Some(children) => {
                for (name, child) in children {
                    add_entry(entries, format!("{}.{}", path, name), child)?;
                }
                Ok(())
            }
            None => Err(Error::ParseError(
                "children required for type branch".to_owned(),
            )),
        },
        EntryType::Actuator => {
            let data_type = match entry.data_type {
                Some(data_type) => data_type.into(),
                None => {
                    return Err(Error::ParseError(
                        "data_type required for actuator".to_owned(),
                    ))
                }
            };
            let _ = entries.insert(
                path,
                DataEntry {
                    entry_type: types::EntryType::Actuator,
                    description: entry.description,
                    comment: entry.comment,
                    unit: entry.unit,
                    min: try_from_json_value(entry.min, &data_type)?,
                    max: try_from_json_value(entry.max, &data_type)?,
                    allowed: try_from_json_array(entry.allowed, &data_type)?,
                    default: None, // isn't used by actuators
                    data_type,
                },
            );
            Ok(())
        }
        EntryType::Attribute => {
            let data_type = match entry.data_type {
                Some(data_type) => data_type.into(),
                None => {
                    return Err(Error::ParseError(
                        "data_type required for actuator".to_owned(),
                    ))
                }
            };
            let _ = entries.insert(
                path,
                DataEntry {
                    entry_type: types::EntryType::Attribute,
                    description: entry.description,
                    comment: entry.comment,
                    unit: entry.unit,
                    min: try_from_json_value(entry.min, &data_type)?,
                    max: try_from_json_value(entry.max, &data_type)?,
                    allowed: try_from_json_array(entry.allowed, &data_type)?,
                    default: try_from_json_value(entry.default, &data_type)?,
                    data_type,
                },
            );
            Ok(())
        }
        EntryType::Sensor => {
            let data_type = match entry.data_type {
                Some(data_type) => data_type.into(),
                None => {
                    return Err(Error::ParseError(
                        "data_type required for actuator".to_owned(),
                    ))
                }
            };
            let _ = entries.insert(
                path,
                DataEntry {
                    entry_type: types::EntryType::Sensor,
                    description: entry.description,
                    comment: entry.comment,
                    unit: entry.unit,
                    min: try_from_json_value(entry.min, &data_type)?,
                    max: try_from_json_value(entry.max, &data_type)?,
                    allowed: try_from_json_array(entry.allowed, &data_type)?,
                    default: None, // isn't used by sensors
                    data_type,
                },
            );
            Ok(())
        }
    }
}

pub fn parse_vss_from_reader<R>(reader: R) -> Result<BTreeMap<String, DataEntry>, Error>
where
    R: std::io::Read,
{
    let root_entry = match serde_json::from_reader::<R, RootEntry>(reader) {
        Ok(root_entry) => root_entry,
        Err(err) => return Err(err.into()),
    };

    flatten_vss_tree(root_entry)
}

pub fn parse_vss_from_str(data: &str) -> Result<BTreeMap<String, DataEntry>, Error> {
    let root_entry = match serde_json::from_str::<RootEntry>(data) {
        Ok(root_entry) => root_entry,
        Err(err) => return Err(err.into()),
    };

    flatten_vss_tree(root_entry)
}

#[test]
fn test_parse_vss() {
    let data = r#"
{
    "Vehicle": {
        "children": {
            "ADAS": {
                "children": {
                    "ESC": {
                        "children": {
                            "IsEnabled": {
                                "datatype": "boolean",
                                "description": "Indicates if ESC is enabled. True = Enabled. False = Disabled.",
                                "type": "actuator",
                                "uuid": "3f4f39b8d8c05c97a6de685282ba74b7"
                            },
                            "IsEngaged": {
                                "datatype": "boolean",
                                "description": "Indicates if ESC is currently regulating vehicle stability. True = Engaged. False = Not Engaged.",
                                "type": "sensor",
                                "uuid": "2088953a28385353a9d46b3a3dc11cac"
                            },
                            "RoadFriction": {
                                "children": {
                                    "MostProbable": {
                                        "datatype": "float",
                                        "description": "Most probable road friction, as calculated by the ESC system. Exact meaning of most probable is implementation specific. 0 = no friction, 100 = maximum friction.",
                                        "max": 100,
                                        "min": 0,
                                        "type": "sensor",
                                        "unit": "percent",
                                        "uuid": "b0eb72430cd95bfbba0d187fcb6e2a62"
                                    }
                                },
                                "description": "Road friction values reported by the ESC system.",
                                "type": "branch",
                                "uuid": "71a32e4eb131532c82195508d93807ed"
                            }
                        },
                        "description": "Electronic Stability Control System signals.",
                        "type": "branch",
                        "uuid": "636b4586ce7854b4b270a2f3b6c0af4f"
                    },
                    "SupportedAutonomyLevel": {
                        "allowed": [
                            "SAE_0",
                            "SAE_1",
                            "SAE_2",
                            "SAE_3",
                            "SAE_4",
                            "SAE_5"
                        ],
                        "datatype": "string",
                        "description": "Indicates the highest level of autonomy according to SAE J3016 taxonomy the vehicle is capable of.",
                        "type": "attribute",
                        "uuid": "020410189ab4517cb85ceda268b40f51"
                    }
                },
                "description": "All Advanced Driver Assist Systems data.",
                "type": "branch",
                "uuid": "14c2b2e1297b513197d320a5ce58f42e"
            },
            "MaxTowWeight": {
                "datatype": "uint16",
                "default": 1000,
                "description": "Maximum weight of trailer.",
                "type": "attribute",
                "unit": "kg",
                "uuid": "a1b8fd65897654aa8a418bccf443f1f3"
            }
        },
        "description": "High-level vehicle data.",
        "type": "branch",
        "uuid": "ccc825f94139544dbb5f4bfd033bece6"
    }
}"#;

    let root_entry = match serde_json::from_str::<RootEntry>(data) {
        Ok(root_entry) => root_entry,
        Err(err) => panic!("{}", err),
    };

    match flatten_vss_tree(root_entry) {
        Ok(entries) => {
            assert_eq!(entries.len(), 5);
            match entries.get("Vehicle.ADAS.ESC.IsEnabled") {
                Some(entry) => {
                    assert_eq!(entry.data_type, types::DataType::Bool);
                    assert_eq!(entry.entry_type, types::EntryType::Actuator);
                    assert_eq!(
                        entry.description,
                        "Indicates if ESC is enabled. True = Enabled. False = Disabled."
                    );
                }
                None => panic!("Vehicle.ADAS.ESC.IsEnabled expected"),
            }
            match entries.get("Vehicle.ADAS.ESC.IsEngaged") {
                Some(entry) => {
                    assert_eq!(entry.data_type, types::DataType::Bool);
                    assert_eq!(entry.entry_type, types::EntryType::Sensor);
                }
                None => panic!("Vehicle.ADAS.ESC.IsEngaged expected"),
            }

            match entries.get("Vehicle.ADAS.ESC.RoadFriction.MostProbable") {
                Some(entry) => {
                    assert_eq!(entry.data_type, types::DataType::Float);
                    assert_eq!(entry.entry_type, types::EntryType::Sensor);
                    assert_eq!(entry.min, Some(types::DataValue::Float(0.0)));
                    assert_eq!(entry.max, Some(types::DataValue::Float(100.0)));
                    assert_eq!(entry.unit, Some("percent".to_owned()));
                }
                None => panic!("Vehicle.ADAS.ESC.RoadFriction.MostProbable expected"),
            }

            match entries.get("Vehicle.MaxTowWeight") {
                Some(entry) => {
                    assert_eq!(entry.data_type, types::DataType::Uint16);
                    assert_eq!(entry.entry_type, types::EntryType::Attribute);
                    assert_eq!(entry.default, Some(types::DataValue::Uint32(1000)));
                    assert_eq!(entry.unit, Some("kg".to_owned()));
                }
                None => panic!("Vehicle.ADAS.ESC.RoadFriction.MostProbable expected"),
            }

            match entries.get("Vehicle.ADAS.SupportedAutonomyLevel") {
                Some(entry) => {
                    assert_eq!(entry.data_type, types::DataType::String);
                    assert_eq!(entry.entry_type, types::EntryType::Attribute);
                    match &entry.allowed {
                        Some(allowed) => match allowed {
                            types::DataValue::StringArray(array) => {
                                assert_eq!(array.len(), 6);
                                assert_eq!(array[0], "SAE_0");
                                assert_eq!(array[1], "SAE_1");
                                assert_eq!(array[2], "SAE_2");
                                assert_eq!(array[3], "SAE_3");
                                assert_eq!(array[4], "SAE_4");
                                assert_eq!(array[5], "SAE_5");
                            }
                            _ => panic!("allowed expected to be of type StringArray()"),
                        },
                        None => panic!("allowed expected to be Some(...)"),
                    }
                }
                None => panic!("Vehicle.ADAS.SupportedAutonomyLevel expected"),
            }
        }
        Err(err) => panic!("Expected parsing to work: {:?}", err),
    }
}
