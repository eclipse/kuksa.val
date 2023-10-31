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

use cucumber::{cli, given, then, when, writer, World as _};
use databroker_proto::kuksa::val::v1::{
    datapoint::Value, DataEntry, DataType, Datapoint, EntryRequest, EntryUpdate, Field, GetRequest,
    SetRequest, View,
};
use tracing::debug;
use world::DataBrokerWorld;

mod world;

#[given("a running Databroker server")]
async fn start_databroker_server(w: &mut DataBrokerWorld) {
    w.start_databroker().await;
    assert!(w.broker_client.is_some())
}

#[given(expr = "a Data Entry {word} of type {word} having value {word}")]
async fn a_known_data_entry_has_value(
    w: &mut DataBrokerWorld,
    path: String,
    data_type: DataType,
    value: String,
) {
    set_current_value(w, path, data_type, value).await;
    w.assert_set_response_has_succeeded()
}

#[when(expr = "a client sets the current value of {word} of type {word} to {word}")]
async fn set_current_value(
    w: &mut DataBrokerWorld,
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

    let req = SetRequest {
        updates: vec![EntryUpdate {
            entry: Some(DataEntry {
                path: path.clone(),
                value: Some(datapoint),
                actuator_target: None,
                metadata: None,
            }),
            fields: vec![Field::Value.into(), Field::Path.into()],
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
        Err(e) => panic!("failed to invoke Databroker's set operation: {:?}", e),
    }
}

#[when(expr = "a client gets the current value of {word}")]
async fn get_current_value(w: &mut DataBrokerWorld, path: String) {
    let client = w
        .broker_client
        .as_mut()
        .expect("no Databroker client available, broker not started?");
    let get_request = GetRequest {
        entries: vec![EntryRequest {
            path: path.to_string(),
            view: View::CurrentValue.into(),
            fields: vec![Field::Value.into(), Field::Metadata.into()],
        }],
    };
    match client.get(get_request).await {
        Ok(res) => w.current_get_response = Some(res.into_inner()),
        Err(e) => panic!("failed to invoke Databroker's get operation: {:?}", e),
    }
}

#[then(expr = "the current value for {word} is {word} having type {word}")]
fn assert_current_value(
    w: &mut DataBrokerWorld,
    path: String,
    expected_value: String,
    expected_type: DataType,
) {
    match w.get_current_data_entry(path.clone()) {
        Some(data_entry) => {
            match data_entry.value.and_then(|dp| dp.value) {
                Some(current_value) => {
                    let expected_value = Value::new(expected_type, expected_value.as_str())
                        .expect("unsupported data type");
                    assert_eq!(current_value, expected_value)
                }
                None => panic!("no current value for path: {:?}", path),
            };
            match data_entry
                .metadata
                .and_then(|m| DataType::from_i32(m.data_type))
            {
                None => panic!("no metadata for path: {:?}", path),
                Some(current_type) => assert_eq!(current_type, expected_type),
            };
        }
        None => panic!(
            "failed to retrieve entry for path {:?} from Databroker response",
            path
        ),
    }
}

#[then(expr = "the current value for {word} is not specified")]
fn assert_current_value_is_unspecified(w: &mut DataBrokerWorld, path: String) {
    assert!(w.get_current_value(path).is_none())
}

#[then("the current value is not found")]
fn assert_current_value_not_found(w: &mut DataBrokerWorld) {
    let error_code = w
        .current_get_response
        .clone()
        .and_then(|res| res.error)
        .map(|error| error.code);

    assert_eq!(error_code, Some(404));
}

#[then(expr = "the current value for {word} has type {word}")]
fn assert_current_value_type(w: &mut DataBrokerWorld, path: String, expected_type: DataType) {
    match w.get_current_value_type(path) {
        Some(data_type) => assert_eq!(
            data_type, expected_type,
            "current value's type is not expected type"
        ),
        None => panic!("failed to retrieve current value's type from Databroker response"),
    }
}

#[then(expr = "setting the value for {word} fails with error code {int}")]
fn assert_set_request_failure(w: &mut DataBrokerWorld, path: String, expected_error_code: u32) {
    w.assert_set_response_has_error_code(path, expected_error_code)
}

#[tokio::main]
async fn main() {
    // databroker::init_logging();

    let opts = cli::Opts::<_, _, _, world::UnsupportedLibtestArgs>::parsed();
    if let Some(thread_count) = opts.custom.test_threads {
        println!("Ignoring command line parameter \"--test-threads {thread_count}\" passed in by test runner");
    }

    DataBrokerWorld::cucumber()
        // support "--format json" argument being passed into test
        .with_writer(writer::Normalize::new(writer::Libtest::or_basic()))
        .with_cli(opts)
        .after(|_feature, _rule, _scenario, _ev, world| {
            if let Some(w) = world {
                w.stop_databroker();
            }
            Box::pin(future::ready(()))
        })
        .run_and_exit("tests/features/current_values.feature")
        .await;
}
