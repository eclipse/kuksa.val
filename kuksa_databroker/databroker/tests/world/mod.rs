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

use std::{
    future::poll_fn,
    net::SocketAddr,
    sync::{Arc, Mutex},
    task::{Poll, Waker},
};

use databroker::{broker, grpc, permissions};
use databroker_proto::kuksa::val::v1::{
    datapoint::Value, val_client::ValClient, DataEntry, DataType, GetResponse, SetResponse,
};
use tokio::net::TcpListener;
use tonic::transport::Channel;
use tracing::debug;

const DATAPOINTS: &[(
    &str,
    broker::DataType,
    broker::ChangeType,
    broker::EntryType,
    &str,
)] = &[
    (
        "Vehicle.Cabin.Lights.AmbientLight",
        broker::DataType::Uint8,
        broker::ChangeType::OnChange,
        broker::EntryType::Sensor,
        "How much ambient light is detected in cabin. 0 = No ambient light. 100 = Full brightness",
    ),
    (
        "Vehicle.Cabin.Sunroof.Position",
        broker::DataType::Int8,
        broker::ChangeType::OnChange,
        broker::EntryType::Sensor,
        "Sunroof position. 0 = Fully closed 100 = Fully opened. -100 = Fully tilted.",
    ),
    (
        "Vehicle.CurrentLocation.Longitude",
        broker::DataType::Double,
        broker::ChangeType::Continuous,
        broker::EntryType::Sensor,
        "Current longitude of vehicle in WGS 84 geodetic coordinates, as measured at the position of GNSS receiver antenna.",
    ),
    (
        "Vehicle.IsMoving",
        broker::DataType::Bool,
        broker::ChangeType::OnChange,
        broker::EntryType::Sensor,
        "Whether the vehicle is moving",
    ),
    (
        "Vehicle.Powertrain.ElectricMotor.Power",
        broker::DataType::Int16,
        broker::ChangeType::Continuous,
        broker::EntryType::Sensor,
        "Current motor power output. Negative values indicate regen mode.",
    ),
    (
        "Vehicle.Powertrain.ElectricMotor.Speed",
        broker::DataType::Int32,
        broker::ChangeType::Continuous,
        broker::EntryType::Sensor,
        "Motor rotational speed measured as rotations per minute. Negative values indicate reverse driving mode.",
    ),
    (
        "Vehicle.Powertrain.Range",
        broker::DataType::Uint32,
        broker::ChangeType::Continuous,
        broker::EntryType::Sensor,
        "Remaining range in meters using all energy sources available in the vehicle.",
    ),
    (
        "Vehicle.Speed",
        broker::DataType::Float,
        broker::ChangeType::Continuous,
        broker::EntryType::Sensor,
        "Vehicle speed",
    ),
    (
        "Vehicle.TraveledDistanceHighRes",
        broker::DataType::Uint64,
        broker::ChangeType::Continuous,
        broker::EntryType::Sensor,
        "Accumulated distance travelled by the vehicle during its operation.",
    ),
    (
        "Vehicle.Width",
        broker::DataType::Uint16,
        broker::ChangeType::OnChange,
        broker::EntryType::Attribute,
        "Overall vehicle width.",
    ),
];

#[derive(clap::Args)] // re-export of `clap::Args`
pub struct UnsupportedLibtestArgs {
    #[arg(long)]
    report_time: Option<bool>,
    #[arg(long)]
    test_threads: Option<u16>,
}

#[derive(Debug)]
struct DataBrokerState {
    running: bool,
    address: Option<SocketAddr>,
    waker: Option<Waker>,
}

#[derive(cucumber::World, Debug)]
#[world(init = Self::new)]
pub struct DataBrokerWorld {
    pub current_get_response: Option<GetResponse>,
    pub current_set_response: Option<SetResponse>,
    pub broker_client: Option<ValClient<Channel>>,
    data_broker_state: Arc<Mutex<DataBrokerState>>,
}

impl DataBrokerWorld {
    pub fn new() -> DataBrokerWorld {
        DataBrokerWorld {
            current_get_response: None,
            current_set_response: None,
            data_broker_state: Arc::new(Mutex::new(DataBrokerState {
                running: false,
                address: None,
                waker: None,
            })),
            broker_client: None,
        }
    }

    pub async fn start_databroker(&mut self) {
        {
            let state = self
                .data_broker_state
                .lock()
                .expect("failed to lock shared broker state");
            if state.running {
                if state.address.is_some() {
                    return;
                } else {
                    panic!("Databroker seems to be running but listener address is unknown")
                }
            }
        }
        let owned_state = self.data_broker_state.to_owned();
        let listener = TcpListener::bind("127.0.0.1:0")
            .await
            .expect("failed to bind to socket");
        let addr = listener
            .local_addr()
            .expect("failed to determine listener's port");

        tokio::spawn(async move {
            let version = option_env!("VERGEN_GIT_SEMVER_LIGHTWEIGHT")
                .unwrap_or(option_env!("VERGEN_GIT_SHA").unwrap_or("unknown"));
            let data_broker = broker::DataBroker::new(version);
            let database = data_broker.authorized_access(&permissions::ALLOW_ALL);
            for (name, data_type, change_type, entry_type, description) in DATAPOINTS {
                if let Err(_error) = database
                    .add_entry(
                        name.to_string(),
                        data_type.clone(),
                        change_type.clone(),
                        entry_type.clone(),
                        description.to_string(),
                        None,
                    )
                    .await
                {
                    return Err("failed to register metadata for {name}");
                }
            }

            {
                let mut state = owned_state.lock().unwrap();
                state.running = true;
                state.address = Some(addr);
            }

            grpc::server::serve_with_incoming_shutdown(
                tokio_stream::wrappers::TcpListenerStream::new(listener),
                data_broker,
                databroker::authorization::Authorization::Disabled,
                poll_fn(|cx| {
                    let mut state = owned_state
                        .lock()
                        .expect("failed to lock shared broker state");
                    if state.running {
                        debug!("Databroker is still running");
                        state.waker = Some(cx.waker().clone());
                        Poll::Pending
                    } else {
                        // println!("Databroker has been stopped");
                        Poll::Ready(())
                    }
                }),
            )
            .await
            .map_err(|e| {
                debug!("failed to start Databroker: {e}");
                {
                    let mut state = owned_state
                        .lock()
                        .expect("failed to lock shared broker state");
                    state.running = false;
                    state.address = None;
                }
                "error"
            })
            .map(|_| {
                debug!("Databroker has been stopped");
            })
        });

        debug!("started Databroker [address: {addr}]");
        let data_broker_url = format!("http://{}:{}", addr.ip(), addr.port());
        self.broker_client = Some(
            ValClient::connect(data_broker_url)
                .await
                .expect("failed to create Databroker client"),
        );
    }

    pub fn stop_databroker(&mut self) {
        debug!("stopping Databroker");
        let mut state = self
            .data_broker_state
            .lock()
            .expect("failed to lock shared broker state");
        state.running = false;
        if let Some(waker) = state.waker.take() {
            waker.wake()
        };
        self.broker_client = None
    }

    pub fn get_current_data_entry(&self, path: String) -> Option<DataEntry> {
        self.current_get_response.clone().and_then(|res| {
            res.entries
                .into_iter()
                .find(|data_entry| data_entry.path == path)
        })
    }

    pub fn get_current_value(&self, path: String) -> Option<Value> {
        self.get_current_data_entry(path)
            .and_then(|data_entry| data_entry.value)
            .and_then(|datapoint| datapoint.value)
    }

    pub fn get_current_value_type(&self, path: String) -> Option<DataType> {
        self.get_current_data_entry(path)
            .and_then(|data_entry| data_entry.metadata)
            .and_then(|m| DataType::from_i32(m.data_type))
    }

    pub fn assert_set_response_has_error_code(&self, path: String, error_code: u32) {
        let code = self.current_set_response.clone().and_then(|res| {
            res.errors
                .into_iter()
                .find(|error| error.path == path)
                .and_then(|data_entry_error| data_entry_error.error)
                .map(|error| error.code)
        });
        assert!(code.is_some(), "response contains no error code");
        assert_eq!(
            code,
            Some(error_code),
            "response contains unexpected error code"
        );
    }

    pub fn assert_set_response_has_succeeded(&self) {
        if let Some(res) = self.current_set_response.clone() {
            assert!(res.error.is_none() && res.errors.is_empty())
        }
    }
}
