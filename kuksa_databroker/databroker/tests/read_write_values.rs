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

use core::panic;
use std::{future, time::SystemTime, vec};

use cucumber::{cli, gherkin::Step, given, then, when, writer, World as _};
use databroker::broker;
use databroker_proto::kuksa::val::v1::{
    datapoint::Value, DataEntry, DataType, Datapoint, EntryRequest, EntryUpdate, Field, GetRequest,
    SetRequest, View,
};
use tracing::debug;
use world::{DataBrokerWorld, ValueType};

mod world;

fn get_data_entries_from_table(
    step: &Step,
) -> Vec<(
    String,
    broker::DataType,
    broker::ChangeType,
    broker::EntryType,
)> {
    let mut data_entries = vec![];
    if let Some(table) = step.table.as_ref() {
        for row in table.rows.iter().skip(1) {
            // skip header
            let path = row[0].clone();
            let data_type_string = row[1].to_lowercase();
            let data_type = match data_type_string.as_str() {
                "bool" => broker::DataType::Bool,
                "double" => broker::DataType::Double,
                "float" => broker::DataType::Float,
                "int8" => broker::DataType::Int8,
                "int16" => broker::DataType::Int16,
                "int32" => broker::DataType::Int32,
                "uint8" => broker::DataType::Uint8,
                "uint16" => broker::DataType::Uint16,
                "uint32" => broker::DataType::Uint32,
                "uint64" => broker::DataType::Uint64,
                _ => panic!(
                    "data type {} is not (yet) supported by this step implementation",
                    data_type_string
                ),
            };
            let change_type_string = row[2].to_lowercase();
            let change_type = match change_type_string.as_str() {
                "onchange" => broker::ChangeType::OnChange,
                "continuous" => broker::ChangeType::Continuous,
                "static" => broker::ChangeType::Static,
                _ => panic!("unsupported change type: {}", change_type_string),
            };
            let entry_type_string = row[3].to_lowercase();
            let entry_type = match entry_type_string.as_str() {
                "actuator" => broker::EntryType::Actuator,
                "attribute" => broker::EntryType::Attribute,
                "sensor" => broker::EntryType::Sensor,
                _ => panic!("unsupported entry type: {}", entry_type_string),
            };
            data_entries.push((path, data_type, change_type, entry_type));
        }
    }
    data_entries
}

#[given(regex = "^a running Databroker server.*$")]
async fn start_databroker_server(w: &mut DataBrokerWorld, step: &Step) {
    w.start_databroker(get_data_entries_from_table(step)).await;
    assert!(w.broker_client.is_some())
}

#[given(expr = "a Data Entry {word} of type {word} having {word} value {word}")]
async fn a_known_data_entry_has_value(
    w: &mut DataBrokerWorld,
    path: String,
    data_type: DataType,
    value_type: ValueType,
    value: String,
) {
    set_value(w, value_type, path, data_type, value).await;
    w.assert_set_response_has_succeeded()
}

#[when(expr = "a client sets the {word} value of {word} of type {word} to {word}")]
async fn set_value(
    w: &mut DataBrokerWorld,
    value_type: ValueType,
    path: String,
    data_type: DataType,
    value: String,
) {
    let client = w
        .broker_client
        .as_mut()
        .expect("no Databroker client available, broker not started?");
    let value = Value::new(data_type, value.as_str()).expect("cannot parse value into given type");
    let datapoint = Datapoint {
        timestamp: Some(SystemTime::now().into()),
        value: Some(value),
    };

    let data_entry = match value_type {
        ValueType::Current => DataEntry {
            path: path.clone(),
            value: Some(datapoint),
            actuator_target: None,
            metadata: None,
        },
        ValueType::Target => DataEntry {
            path: path.clone(),
            value: None,
            actuator_target: Some(datapoint),
            metadata: None,
        },
    };
    let req = SetRequest {
        updates: vec![EntryUpdate {
            entry: Some(data_entry),
            fields: vec![
                Field::ActuatorTarget.into(),
                Field::Value.into(),
                Field::Path.into(),
            ],
        }],
    };
    match client.set(req).await {
        Ok(res) => {
            let set_response = res.into_inner();
            debug!(
                "response from Databroker [global error: {:?}, Data Entry errors: {:?}]",
                set_response.error, set_response.errors
            );
            w.current_set_response = Some(set_response);
        }
        Err(e) => {
            debug!("failed to invoke Databroker's set operation: {:?}", e);
            w.current_status = Some(e);
        }
    }
}

#[when(expr = "a client gets the {word} value of {word}")]
async fn get_value(w: &mut DataBrokerWorld, value_type: ValueType, path: String) {
    let client = w
        .broker_client
        .as_mut()
        .expect("no Databroker client available, broker not started?");
    let get_request = GetRequest {
        entries: vec![EntryRequest {
            path: path.to_string(),
            view: match value_type {
                ValueType::Current => View::CurrentValue.into(),
                ValueType::Target => View::TargetValue.into(),
            },
            fields: vec![Field::Value.into(), Field::Metadata.into()],
        }],
    };
    match client.get(get_request).await {
        Ok(res) => w.current_get_response = Some(res.into_inner()),
        Err(e) => {
            debug!("failed to invoke Databroker's get operation: {:?}", e);
            w.current_status = Some(e);
        }
    }
}

#[then(expr = "the {word} value for {word} is {word} having type {word}")]
fn assert_value(
    w: &mut DataBrokerWorld,
    value_type: ValueType,
    path: String,
    expected_value: String,
    expected_type: DataType,
) {
    match w.get_current_data_entry(path.clone()) {
        Some(data_entry) => {
            let data_point = match value_type {
                ValueType::Current => data_entry.value.clone(),
                ValueType::Target => data_entry.actuator_target.clone(),
            };
            match data_point.and_then(|dp| dp.value) {
                Some(actual_value) => {
                    let expected_value = Value::new(expected_type, expected_value.as_str())
                        .expect("unsupported data type");
                    assert_eq!(actual_value, expected_value)
                }
                None => panic!("no current/target value for path: {:?}", path),
            };
            match data_entry
                .metadata
                .and_then(|m| DataType::from_i32(m.data_type))
            {
                None => panic!("no metadata for path: {:?}", path),
                Some(actual_type) => assert_eq!(actual_type, expected_type),
            };
        }
        None => panic!(
            "failed to retrieve entry for path {:?} from Databroker response",
            path
        ),
    }
}

#[then(expr = "the {word} value for {word} is not specified")]
fn assert_value_is_unspecified(w: &mut DataBrokerWorld, value_type: ValueType, path: String) {
    let value = match value_type {
        ValueType::Current => w.get_current_value(path),
        ValueType::Target => w.get_target_value(path),
    };
    assert!(value.is_none())
}

#[then(regex = r"^the (current|target) value is not found$")]
fn assert_value_not_found(w: &mut DataBrokerWorld) {
    let error_code = w
        .current_get_response
        .clone()
        .and_then(|res| res.error)
        .map(|error| error.code);

    assert_eq!(error_code, Some(404));
}

#[then(expr = "setting the value for {word} fails with error code {int}")]
fn assert_set_request_failure(w: &mut DataBrokerWorld, path: String, expected_error_code: u32) {
    w.assert_set_response_has_error_code(path, expected_error_code)
}

/// https://github.com/grpc/grpc/blob/master/doc/statuscodes.md#status-codes-and-their-use-in-grpc
#[then(expr = "the operation fails with status code {int}")]
fn assert_request_failure(w: &mut DataBrokerWorld, expected_status_code: i32) {
    w.assert_status_has_code(expected_status_code)
}

#[tokio::main]
async fn main() {
    // databroker::init_logging();

    let opts = cli::Opts::<_, _, _, world::UnsupportedLibtestArgs>::parsed();
    if let Some(thread_count) = opts.custom.test_threads {
        println!("Ignoring command line parameter \"--test-threads {thread_count}\" passed in by test runner");
    }

    DataBrokerWorld::cucumber()
        .fail_on_skipped()
        // support "--format json" argument being passed into test
        .with_writer(writer::Normalize::new(writer::Libtest::or_basic()))
        .with_cli(opts)
        .after(|_feature, _rule, _scenario, _ev, world| {
            if let Some(w) = world {
                w.stop_databroker();
            }
            Box::pin(future::ready(()))
        })
        .run_and_exit("tests/features/read_write_values.feature")
        .await;
}
