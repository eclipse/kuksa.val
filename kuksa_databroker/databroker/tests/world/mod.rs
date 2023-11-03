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
    str::FromStr,
    sync::{Arc, Mutex},
    task::{Poll, Waker},
};

use databroker::{
    broker,
    grpc::{self, server::ServerTLS},
    permissions,
};
use databroker_proto::kuksa::val::v1::{
    datapoint::Value, val_client::ValClient, DataEntry, GetResponse, SetResponse,
};
use tokio::net::TcpListener;
use tonic::{
    transport::{Certificate, Channel, ClientTlsConfig, Endpoint, Identity},
    Code, Status,
};
use tracing::debug;

use lazy_static::lazy_static;

#[cfg(feature = "tls")]
lazy_static! {
    pub static ref CERTS: DataBrokerCertificates = DataBrokerCertificates::new();
}

#[derive(clap::Args)] // re-export of `clap::Args`
pub struct UnsupportedLibtestArgs {
    // allow "--test-threads" parameter being passed into the test
    #[arg(long)]
    pub test_threads: Option<u16>,
}

#[derive(Debug, Default)]
pub enum ValueType {
    #[default]
    Current,
    Target,
}

impl FromStr for ValueType {
    type Err = String;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Ok(match s.to_lowercase().as_str() {
            "current" => Self::Current,
            "target" => Self::Target,
            invalid => return Err(format!("Invalid `ValueType`: {invalid}")),
        })
    }
}

#[cfg(feature = "tls")]
pub struct DataBrokerCertificates {
    server_identity: Identity,
    ca_certs: Certificate,
}

#[cfg(feature = "tls")]
impl DataBrokerCertificates {
    fn new() -> Self {
        let manifest_dir = env!("CARGO_MANIFEST_DIR");
        let cert_dir = format!("{manifest_dir}/../../kuksa_certificates");
        debug!("reading key material from {}", cert_dir);
        let key_file = format!("{cert_dir}/Server.key");
        let server_key = std::fs::read(key_file).expect("could not read server key");
        let cert_file = format!("{cert_dir}/Server.pem");
        let server_cert = std::fs::read(cert_file).expect("could not read server certificate");
        let server_identity = tonic::transport::Identity::from_pem(server_cert, server_key);
        let ca_file = format!("{cert_dir}/CA.pem");
        let ca_store = std::fs::read(ca_file).expect("could not read root CA file");
        let ca_certs = Certificate::from_pem(ca_store);
        DataBrokerCertificates {
            server_identity,
            ca_certs,
        }
    }

    fn server_tls_config(&self) -> ServerTLS {
        ServerTLS::Enabled {
            tls_config: tonic::transport::ServerTlsConfig::new()
                .identity(self.server_identity.clone()),
        }
    }

    fn client_tls_config(&self) -> ClientTlsConfig {
        ClientTlsConfig::new().ca_certificate(self.ca_certs.clone())
    }
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
    pub current_status: Option<Status>,
    pub broker_client: Option<ValClient<Channel>>,
    data_broker_state: Arc<Mutex<DataBrokerState>>,
}

impl DataBrokerWorld {
    pub fn new() -> DataBrokerWorld {
        DataBrokerWorld {
            current_get_response: None,
            current_set_response: None,
            current_status: None,
            data_broker_state: Arc::new(Mutex::new(DataBrokerState {
                running: false,
                address: None,
                waker: None,
            })),
            broker_client: None,
        }
    }

    pub async fn start_databroker(
        &mut self,
        data_entries: Vec<(
            String,
            broker::DataType,
            broker::ChangeType,
            broker::EntryType,
        )>,
    ) {
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
            for (name, data_type, change_type, entry_type) in data_entries {
                if let Err(_error) = database
                    .add_entry(
                        name,
                        data_type,
                        change_type,
                        entry_type,
                        "N/A".to_string(),
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
                listener,
                data_broker,
                #[cfg(feature = "tls")]
                CERTS.server_tls_config(),
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
        #[cfg(not(feature = "tls"))]
        let client_endpoint =
            Endpoint::from_shared(format!("http://{}:{}", addr.ip(), addr.port()))
                .expect("cannot create client endpoint");
        #[cfg(feature = "tls")]
        let client_endpoint =
            Endpoint::from_shared(format!("https://{}:{}", addr.ip(), addr.port()))
                .and_then(|conf| conf.tls_config(CERTS.client_tls_config()))
                .expect("cannot create client endpoint");

        self.broker_client = Some(
            ValClient::connect(client_endpoint)
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

    pub fn get_target_value(&self, path: String) -> Option<Value> {
        self.get_current_data_entry(path)
            .and_then(|data_entry| data_entry.actuator_target)
            .and_then(|datapoint| datapoint.value)
    }

    /// https://github.com/grpc/grpc/blob/master/doc/statuscodes.md#status-codes-and-their-use-in-grpc
    pub fn assert_status_has_code(&self, expected_status_code: i32) {
        assert!(
            self.current_status.is_some(),
            "operation did not result in an error"
        );
        if let Some(status) = self.current_status.clone() {
            assert_eq!(status.code(), Code::from_i32(expected_status_code));
        }
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
