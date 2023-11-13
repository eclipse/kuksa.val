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

use common::ClientError;
use databroker_proto::kuksa::val::v1::{
    datapoint::Value, DataEntry,
};

use databroker::{
    broker,
    grpc::{self, server::ServerTLS},
    permissions,
};

use tokio::net::TcpListener;
use tracing::debug;

use lazy_static::lazy_static;

use tonic::Code;
use tonic::transport::{Identity, Certificate, ClientTlsConfig};

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
    pub current_data_entries: Option<Vec<DataEntry>>,
    pub current_client_error: Option<ClientError>,
    pub broker_client: Option<kuksa::KuksaClient>,
    data_broker_state: Arc<Mutex<DataBrokerState>>,
}

impl DataBrokerWorld {
    pub fn new() -> DataBrokerWorld {
        DataBrokerWorld {
            current_data_entries: Some(vec![]),
            current_client_error: None,
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
        authorization_enabled: bool,
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

            let mut _authorization = databroker::authorization::Authorization::Disabled;

            if authorization_enabled{
                // public key comes from kuksa.val/kuksa_certificates/jwt/jwt.key.pub
                match databroker::authorization::Authorization::new("-----BEGIN PUBLIC KEY----- MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA6ScE9EKXEWVyYhzfhfvg+LC8NseiuEjfrdFx3HKkb31bRw/SeS0Rye0KDP7uzffwreKf6wWYGxVUPYmyKC7jPji5MpDBGM9r3pIZSvPUFdpTE5TiRHFBxWbqPSYt954BTLq4rMu/W+oq5PdfnugbvoYpLf0dclBl1g9KyszkDnItz3TYbWhGMbsUSfyeSPzH0IADzLoifxbc5mgiR73NCA/4yNSpfLoqWgQ2vdTM1182sMSmxfqSgMzIMUX/tiaXGdkoKITF1sULlLyWfTo979XRZ0hmUwvfzr3OjMZNoClpYSVbKY+vtxHyux9KOOtv9lPMsgYIaPXvisrsneDZfCS0afOfjgR96uHIe2UPSGAXru3yGziqEfpRZoxsgXaOe905ordLD5bSX14xkN7NCz7rxDLlxPQyxp4Vhog7p/QeUyydBpZjq2bAE5GAJtiu+XGvG8RypzJFKFQwMNswg1BoZVD0mb0MtU8KQmHcZIfY0FVer/CR0mUjfl1rHbtoJB+RY03lQvYNAD04ibAGNI1RhlTziu35Xo6NDEgs9hVs9k3WrtF+ZUxhivWmP2VXhWruRakVkC1NzKGh54e5/KlluFbBNpWgvWZqzWo9Jr7/fzHtR0Q0IZwkxh+Vd/bUZya1uLKqP+sTcc+aTHbnAEiqOjPq0D6X45wCzIwjILUCAwEAAQ== -----END PUBLIC KEY-----".to_string()){
                        Ok(auth) => _authorization = auth,
                        Err(e) => println!("Error: {e}"),
                    }
            }

            grpc::server::serve_with_incoming_shutdown(
                listener,
                data_broker,
                #[cfg(feature = "tls")]
                CERTS.server_tls_config(),
                _authorization,
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

        self.broker_client = match common::to_uri(data_broker_url.clone()){
            Ok(uri) => Some(kuksa::KuksaClient::new(uri)),
            Err(e) => {
                println!("Error connecting to {data_broker_url}: {e}");
                None
            }
        };

        #[cfg(feature = "tls")]
        if let Some(client) = self.broker_client.as_mut(){
            client.basic_client.set_tls_config(CERTS.client_tls_config());
        }
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
        self.current_data_entries.clone().and_then(|res| {
            res.into_iter()
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
        match &self.current_client_error{
            Some(ClientError::Connection(_)) => assert!(false, "Connection error shall not occur"),
            Some(ClientError::Function(_)) => assert!(false, "Fucntion has an error that shall not occur"),
            Some(ClientError::Status(status)) => assert_eq!(status.code(), Code::from_i32(expected_status_code)),
            None => assert!(false, "No error, but an errror is expected")
        }
    }

    pub fn assert_response_has_error_code(&self, error_codes: Vec<u32>) {
        let mut code = Vec::new();

        if let Some(client_error) = self.current_client_error.clone() {
            match client_error{
                ClientError::Connection(_) => assert!(false, "response contains connection error"),
                ClientError::Function(e) => {
                    for element in e{
                        if !code.contains(&element.code){
                            code.push(element.code)
                        }
                    }
                },
                ClientError::Status(_) => assert!(false, "response contains channel error"),
            }

            assert!(!code.is_empty(), "response contains no error code {:?}", code);
            assert_eq!(code, error_codes, "response contains unexpected error code");
        }
        else{
            assert!(false, "response contains no error code");
        }
    }

    pub fn assert_set_succeeded(&self) {
        if let Some(error) = self.current_client_error.clone() {
            match error{
                ClientError::Connection(e) => assert!(false, "No connection error {:?} should occcur", e),
                ClientError::Function(e) => assert!(false, "No function error {:?} should occur", e),
                ClientError::Status(status) => assert!(false, "No status error {:?} should occur", status)
            }
        }
        else{
            assert!(true, "Succeeded")
        }
    }
}
