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

use databroker_proto::sdv::databroker as proto;
use http::Uri;
use tokio_stream::wrappers::BroadcastStream;
use tonic::transport::Channel;

pub struct Client {
    uri: Uri,
    token: Option<tonic::metadata::AsciiMetadataValue>,
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

impl Client {
    pub fn new(uri: Uri) -> Self {
        Client {
            uri,
            token: None,
            tls_config: None,
            channel: None,
            connection_state_subs: None,
        }
    }

    pub fn get_uri(&self) -> String {
        self.uri.to_string()
    }

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
        let mut builder = tonic::transport::Channel::builder(self.uri.clone());

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
    ) -> Result<Vec<proto::v1::Metadata>, ClientError> {
        let mut client = proto::v1::broker_client::BrokerClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );
        // Empty vec == all property metadata
        let args = tonic::Request::new(proto::v1::GetMetadataRequest { names: paths });
        match client.get_metadata(args).await {
            Ok(response) => {
                let message = response.into_inner();
                Ok(message.list)
            }
            Err(err) => Err(ClientError::Status(err)),
        }
    }

    pub async fn get_datapoints(
        &mut self,
        paths: Vec<impl AsRef<str>>,
    ) -> Result<
        HashMap<std::string::String, databroker_proto::sdv::databroker::v1::Datapoint>,
        ClientError,
    > {
        let mut client = proto::v1::broker_client::BrokerClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );
        let args = tonic::Request::new(proto::v1::GetDatapointsRequest {
            datapoints: paths.iter().map(|path| path.as_ref().into()).collect(),
        });
        match client.get_datapoints(args).await {
            Ok(response) => {
                let message = response.into_inner();
                Ok(message.datapoints)
            }
            Err(err) => Err(ClientError::Status(err)),
        }
    }

    pub async fn set_datapoints(
        &mut self,
        datapoints: HashMap<String, proto::v1::Datapoint>,
    ) -> Result<proto::v1::SetDatapointsReply, ClientError> {
        let args = tonic::Request::new(proto::v1::SetDatapointsRequest { datapoints });
        let mut client = proto::v1::broker_client::BrokerClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );
        match client.set_datapoints(args).await {
            Ok(response) => Ok(response.into_inner()),
            Err(err) => Err(ClientError::Status(err)),
        }
    }

    pub async fn subscribe(
        &mut self,
        query: String,
    ) -> Result<tonic::Streaming<proto::v1::SubscribeReply>, ClientError> {
        let mut client = proto::v1::broker_client::BrokerClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );
        let args = tonic::Request::new(proto::v1::SubscribeRequest { query });

        match client.subscribe(args).await {
            Ok(response) => Ok(response.into_inner()),
            Err(err) => Err(ClientError::Status(err)),
        }
    }

    pub async fn update_datapoints(
        &mut self,
        datapoints: HashMap<i32, proto::v1::Datapoint>,
    ) -> Result<proto::v1::UpdateDatapointsReply, ClientError> {
        let mut client = proto::v1::collector_client::CollectorClient::with_interceptor(
            self.get_channel().await?.clone(),
            self.get_auth_interceptor(),
        );

        let request = tonic::Request::new(proto::v1::UpdateDatapointsRequest { datapoints });
        match client.update_datapoints(request).await {
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
