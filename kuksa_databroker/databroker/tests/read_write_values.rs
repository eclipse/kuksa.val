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
use std::{collections::HashMap, future, time::SystemTime, vec};

use common::ClientError;
use cucumber::{cli, gherkin::Step, given, then, when, writer, World as _};
use databroker::broker;
use databroker_proto::kuksa::val::v1::{datapoint::Value, DataType, Datapoint};
use tonic::Code;
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

#[given(regex = "^a running Databroker server with authorization (true|false).*$")]
async fn start_databroker_server(w: &mut DataBrokerWorld, auth: bool, step: &Step) {
    w.start_databroker(get_data_entries_from_table(step), auth)
        .await;
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
    w.assert_set_succeeded()
}

#[when(expr = "a client uses a token with auhtorization {word}")]
async fn authorize_client(w: &mut DataBrokerWorld, scope: String) {
    let token = w.create_token(scope);
    w.broker_client
        .as_mut()
        .and_then(|client| match client.basic_client.set_access_token(token) {
            Ok(()) => Some(client),
            Err(e) => {
                println!("Error: {e}");
                None
            }
        })
        .expect("no Databroker client available, broker not started?");
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

    match value_type {
        ValueType::Target => {
            match client
                .set_target_values(HashMap::from([(path.clone(), datapoint.clone())]))
                .await
            {
                Ok(_) => {
                    w.current_client_error = None;
                }
                Err(e) => {
                    debug!("failed to invoke Databroker's set operation: {:?}", e);
                    w.current_client_error = Some(e);
                }
            }
        }
        ValueType::Current => {
            match client
                .set_current_values(HashMap::from([(path.clone(), datapoint.clone())]))
                .await
            {
                Ok(_) => {
                    w.current_client_error = None;
                }
                Err(e) => {
                    debug!("failed to invoke Databroker's set operation: {:?}", e);
                    w.current_client_error = Some(e);
                }
            }
        }
    }
}

#[when(expr = "a client gets the {word} value of {word}")]
async fn get_value(w: &mut DataBrokerWorld, value_type: ValueType, path: String) {
    let client = w
        .broker_client
        .as_mut()
        .expect("no Databroker client available, broker not started?");
    match value_type {
        ValueType::Target => match client.get_target_values(vec![&path]).await {
            Ok(res) => w.current_data_entries = Some(res),
            Err(e) => {
                debug!("failed to invoke Databroker's get operation: {:?}", e);
                w.current_client_error = Some(e);
            }
        },
        ValueType::Current => match client.get_current_values(vec![path]).await {
            Ok(res) => w.current_data_entries = Some(res),
            Err(e) => {
                debug!("failed to invoke Databroker's get operation: {:?}", e);
                w.current_client_error = Some(e);
            }
        },
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
    w.assert_response_has_error_code(vec![404]);
}

#[then(expr = "setting the value for {word} fails with error code {int}")]
fn assert_set_request_failure(w: &mut DataBrokerWorld, _path: String, expected_error_code: u32) {
    w.assert_response_has_error_code(vec![expected_error_code])
}

/// https://github.com/grpc/grpc/blob/master/doc/statuscodes.md#status-codes-and-their-use-in-grpc
#[then(expr = "the operation fails with status code {int}")]
fn assert_request_failure(w: &mut DataBrokerWorld, expected_status_code: i32) {
    w.assert_status_has_code(expected_status_code)
}

#[then(expr = "the current value for {word} can not be accessed because we are unauthorized")]
fn assert_current_value_unauthenticated(w: &mut DataBrokerWorld) {
    if let Some(error) = w.current_client_error.clone() {
        match error {
            ClientError::Connection(e) => {
                panic!("No connection error {:?} should occcur", e)
            }
            ClientError::Function(e) => panic!("No function error {:?} should occur", e),
            ClientError::Status(status) => assert_eq!(status.code(), Code::Unauthenticated),
        }
    }
}

#[then(expr = "the set operation succeeds without an error")]
fn assert_set_succeeds(w: &mut DataBrokerWorld) {
    w.assert_set_succeeded()
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
