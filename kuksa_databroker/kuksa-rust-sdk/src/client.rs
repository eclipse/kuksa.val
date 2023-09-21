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

use std::{collections::HashMap, convert::TryFrom};

use databroker_proto::kuksa::val::{self as proto, v1::DataEntry};
use http::Uri;
use tokio_stream::wrappers::BroadcastStream;
use tonic::transport::Channel;

#[derive(Debug)]
pub struct Client {
    uri: Uri,
    token: Option<tonic::metadata::AsciiMetadataValue>,
    #[cfg(feature = "tls")]
    tls_config: Option<tonic::transport::ClientTlsConfig>,
    channel: Option<tonic::transport::Channel>,
    connection_state_subs: Option<tokio::sync::broadcast::Sender<ConnectionState>>,
}

#[derive(Clone)]
pub enum ConnectionState {
    Connected,
    Disconnected,
}

#[derive(Debug)]
pub enum ClientError {
    Connection(String),
    Status(tonic::Status),
}

impl std::error::Error for ClientError {}
impl std::fmt::Display for ClientError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ClientError::Connection(con) => f.pad(con),
            ClientError::Status(status) => f.pad(&format!("{status}")),
        }
    }
}

#[derive(Debug)]
pub enum TokenError {
    MalformedTokenError(String),
}

impl std::error::Error for TokenError {}
impl std::fmt::Display for TokenError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            TokenError::MalformedTokenError(msg) => f.pad(msg),
        }
    }
}

pub fn to_uri(uri: impl AsRef<str>) -> Result<Uri, String> {
    let uri = uri
        .as_ref()
        .parse::<tonic::transport::Uri>()
        .map_err(|err| format!("{err}"))?;
    let mut parts = uri.into_parts();

    if parts.scheme.is_none() {
        parts.scheme = Some("http".parse().expect("http should be valid scheme"));
    }

    match &parts.authority {
        Some(_authority) => {
            // match (authority.port_u16(), port) {
            //     (Some(uri_port), Some(port)) => {
            //         if uri_port != port {
            //             parts.authority = format!("{}:{}", authority.host(), port)
            //                 .parse::<Authority>()
            //                 .map_err(|err| format!("{}", err))
            //                 .ok();
            //         }
            //     }
            //     (_, _) => {}
            // }
        }
        None => return Err("No server uri specified".to_owned()),
    }
    parts.path_and_query = Some("".parse().expect("uri path should be empty string"));
    tonic::transport::Uri::from_parts(parts).map_err(|err| format!("{err}"))
}

impl Client {
    pub fn new(uri: Uri) -> Self {
        Client {
            uri,
            token: None,
            #[cfg(feature = "tls")]
            tls_config: None,
            channel: None,
            connection_state_subs: None,
        }
    }

    pub fn get_uri(&self) -> String {
        self.uri.to_string()
    }

    #[cfg(feature = "tls")]
    pub fn set_tls_config(&mut self, tls_config: tonic::transport::ClientTlsConfig) {
        self.tls_config = Some(tls_config);
    }

    pub fn set_access_token(&mut self, token: impl AsRef<str>) -> Result<(), TokenError> {
        match tonic::metadata::AsciiMetadataValue::try_from(&format!("Bearer {}", token.as_ref())) {
            Ok(token) => {
                self.token = Some(token);
                Ok(())
            }
            Err(err) => Err(TokenError::MalformedTokenError(format!("{err}"))),
        }
    }

    pub fn is_connected(&self) -> bool {
        self.channel.is_some()
    }

    pub fn subscribe_to_connection_state(&mut self) -> BroadcastStream<ConnectionState> {
        match &self.connection_state_subs {
            Some(stream) => BroadcastStream::new(stream.subscribe()),
            None => {
                let (tx, rx1) = tokio::sync::broadcast::channel(1);
                self.connection_state_subs = Some(tx);
                BroadcastStream::new(rx1)
            }
        }
    }

    async fn try_create_channel(&mut self) -> Result<&Channel, ClientError> {
        #[cfg(feature = "tls")]
        let mut builder = tonic::transport::Channel::builder(self.uri.clone());
        #[cfg(not(feature = "tls"))]
        let builder = tonic::transport::Channel::builder(self.uri.clone());

        #[cfg(feature = "tls")]
        if let Some(tls_config) = &self.tls_config {
            match builder.tls_config(tls_config.clone()) {
                Ok(new_builder) => {
                    builder = new_builder;
                }
                Err(err) => {
                    return Err(ClientError::Connection(format!(
                        "Failed to configure TLS: {err}"
                    )));
                }
            }
        }

        match builder.connect().await {
            Ok(channel) => {
                if let Some(subs) = &self.connection_state_subs {
                    subs.send(ConnectionState::Connected).map_err(|err| {
                        ClientError::Connection(format!(
                            "Failed to notify connection state change: {err}"
                        ))
                    })?;
                }
                self.channel = Some(channel);
                Ok(self.channel.as_ref().expect("Channel should exist"))
            }
            Err(err) => {
                if let Some(subs) = &self.connection_state_subs {
                    subs.send(ConnectionState::Disconnected).unwrap_or_default();
                }
                Err(ClientError::Connection(format!(
                    "Failed to connect to {}: {}",
                    self.uri, err
                )))
            }
        }
    }

    pub async fn try_connect(&mut self) -> Result<(), ClientError> {
        self.try_create_channel().await?;
        Ok(())
    }

    pub async fn try_connect_to(&mut self, uri: tonic::transport::Uri) -> Result<(), ClientError> {
        self.uri = uri;
        self.try_create_channel().await?;
        Ok(())
    }

    pub async fn get_channel(&mut self) -> Result<&Channel, ClientError> {
        if self.channel.is_none() {
            self.try_create_channel().await
        } else {
            match &self.channel {
                Some(channel) => Ok(channel),
                None => unreachable!(),
            }
        }
    }

    pub async fn get_metadata(
        &mut self,
        paths: Vec<String>,
    ) -> Result<Vec<proto::v1::DataEntry>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );

        let mut metadata_result = Vec::new();

        for path in paths {
            let get_request = proto::v1::GetRequest {
                entries: vec![proto::v1::EntryRequest {
                    path: path.to_string(),
                    view: proto::v1::View::Metadata.into(),
                    fields: vec![proto::v1::Field::Metadata.into()],
                }],
            };

            match client.get(get_request).await {
                Ok(response) => {
                    let message = response.into_inner();
                    let entries = message.entries;
                    metadata_result = entries;
                }
                Err(err) => return Err(ClientError::Status(err)),
            }
        }

        Ok(metadata_result)
    }


    pub async fn get_current_values(
        &mut self,
        paths: Vec<impl AsRef<str>>,
    ) -> Result<Vec<DataEntry>, ClientError>{
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );

        let mut get_result = Vec::new();

        for path in paths {
            let get_request = proto::v1::GetRequest {
                entries: vec![proto::v1::EntryRequest {
                    path: path.as_ref().to_string(),
                    view: proto::v1::View::CurrentValue.into(),
                    fields: vec![proto::v1::Field::Value.into()],
                }],
            };

            match client.get(get_request).await {
                Ok(response) => {
                    let message = response.into_inner();
                    get_result = message.entries;
                }
                Err(err) => return Err(ClientError::Status(err)),
            }
        }

        Ok(get_result)
    }

    pub async fn get_target_values(
        &mut self,
        paths: Vec<impl AsRef<str>>,
    ) -> Result<Vec<DataEntry>, ClientError>{
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );

        let mut get_result = Vec::new();

        for path in paths {
            let get_request = proto::v1::GetRequest {
                entries: vec![proto::v1::EntryRequest {
                    path: path.as_ref().to_string(),
                    view: proto::v1::View::TargetValue.into(),
                    fields: vec![proto::v1::Field::ActuatorTarget.into()],
                }],
            };

            match client.get(get_request).await {
                Ok(response) => {
                    let message = response.into_inner();
                    get_result = message.entries;
                }
                Err(err) => return Err(ClientError::Status(err)),
            }
        }

        Ok(get_result)
    }

    pub async fn set_current_values(
        &mut self,
        datapoints: HashMap<String, proto::v1::Datapoint>,
    ) -> Result<Vec<proto::v1::SetResponse>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );

        let mut set_result = Vec::new(); 

        for (path, datapoint) in datapoints{
            let set_request = proto::v1::SetRequest {
                updates: vec![proto::v1::EntryUpdate {
                    entry: Some(proto::v1::DataEntry {
                        path: path.clone(),
                        value: Some(datapoint),
                        actuator_target: None,
                        metadata: None,
                    }),
                    fields: vec![proto::v1::Field::Value.into(), proto::v1::Field::Path.into()],
                }],
            };
            match client.set(set_request).await {
                Ok(response) => {
                    set_result.push(response.into_inner());
                }
                Err(err) => return Err(ClientError::Status(err)),
            }
        }
        
        Ok(set_result)
    }

    pub async fn set_target_values(
        &mut self,
        datapoints: HashMap<String, proto::v1::Datapoint>,
    ) -> Result<Vec<proto::v1::SetResponse>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );

        let mut set_result = Vec::new(); 

        for (path, datapoint) in datapoints{
            let set_request = proto::v1::SetRequest {
                updates: vec![proto::v1::EntryUpdate {
                    entry: Some(proto::v1::DataEntry {
                        path: path.clone(),
                        value: None,
                        actuator_target: Some(datapoint),
                        metadata: None,
                    }),
                    fields: vec![proto::v1::Field::ActuatorTarget.into(), proto::v1::Field::Path.into()],
                }],
            };
            match client.set(set_request).await {
                Ok(response) => {
                    set_result.push(response.into_inner());
                }
                Err(err) => return Err(ClientError::Status(err)),
            }
        }
        
        Ok(set_result)
    }

    pub async fn set_metadata(
        &mut self,
        metadatas: HashMap<String, proto::v1::Metadata>,
    ) -> Result<Vec<proto::v1::SetResponse>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );

        let mut set_result = Vec::new(); 

        for (path, metadata) in metadatas{
            let set_request = proto::v1::SetRequest {
                updates: vec![proto::v1::EntryUpdate {
                    entry: Some(proto::v1::DataEntry {
                        path: path.clone(),
                        value: None,
                        actuator_target: None,
                        metadata: Some(metadata),
                    }),
                    fields: vec![proto::v1::Field::Metadata.into(), proto::v1::Field::Path.into()],
                }],
            };
            match client.set(set_request).await {
                Ok(response) => {
                    set_result.push(response.into_inner());
                }
                Err(err) => return Err(ClientError::Status(err)),
            }
        }
        
        Ok(set_result)
    }

    pub async fn subscribe(
        &mut self,
        query: String,
    ) -> Result<tonic::Streaming<databroker_proto::sdv::databroker::v1::SubscribeReply>, ClientError> {
        let mut client = databroker_proto::sdv::databroker::v1::broker_client::BrokerClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );
        let args = tonic::Request::new(databroker_proto::sdv::databroker::v1::SubscribeRequest { query });

        match client.subscribe(args).await {
            Ok(response) => Ok(response.into_inner()),
            Err(err) => Err(ClientError::Status(err)),
        }
    }

    fn get_auth_interceptor(
        &mut self,
    ) -> impl FnMut(tonic::Request<()>) -> Result<tonic::Request<()>, tonic::Status> + '_ {
        move |mut req: tonic::Request<()>| {
            if let Some(token) = &self.token {
                // debug!("Inserting auth token: {:?}", token);
                req.metadata_mut().insert("authorization", token.clone());
            }
            Ok(req)
        }
    }
}
