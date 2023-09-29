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

use std::collections::HashMap;

use databroker_proto::kuksa::val::{self as proto, v1::DataEntry};

use common::{Client, ClientError};

pub struct KuksaClient {
    pub basic_client: Client,
}

impl KuksaClient {
    pub fn new(basic_client: Client) -> Self {
        KuksaClient { basic_client }
    }

    pub async fn get_metadata(
        &mut self,
        paths: Vec<&str>,
    ) -> Result<Vec<proto::v1::DataEntry>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
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
                    metadata_result = message.entries;
                    let mut errors = Vec::new();
                    for error in message.errors{
                        if let Some(err) = error.error{
                            errors.push(err.code.to_string());
                            errors.push(err.reason.to_string());
                            errors.push(err.message.to_string());
                        }
                    }
                    if !errors.is_empty(){
                        return Err(ClientError::Function(errors));
                    }
                }
                Err(err) => return Err(ClientError::Status(err)),
            }
        }

        Ok(metadata_result)
    }

    pub async fn get_current_values(
        &mut self,
        paths: Vec<impl AsRef<str>>,
    ) -> Result<Vec<DataEntry>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
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
                    let mut errors = Vec::new();
                    for error in message.errors{
                        if let Some(err) = error.error{
                            errors.push(err.code.to_string());
                            errors.push(err.reason.to_string());
                            errors.push(err.message.to_string());
                        }
                    }
                    if !errors.is_empty(){
                        return Err(ClientError::Function(errors));
                    }
                }
                Err(err) => return Err(ClientError::Status(err)),
            }
        }

        Ok(get_result)
    }

    pub async fn get_target_values(
        &mut self,
        paths: Vec<impl AsRef<str>>,
    ) -> Result<Vec<DataEntry>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
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
                    let mut errors = Vec::new();
                    for error in message.errors{
                        if let Some(err) = error.error{
                            errors.push(err.code.to_string());
                            errors.push(err.reason.to_string());
                            errors.push(err.message.to_string());
                        }
                    }
                    if !errors.is_empty(){
                        return Err(ClientError::Function(errors));
                    }
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
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
        );

        let mut set_result = Vec::new();

        for (path, datapoint) in datapoints {
            let set_request = proto::v1::SetRequest {
                updates: vec![proto::v1::EntryUpdate {
                    entry: Some(proto::v1::DataEntry {
                        path: path.clone(),
                        value: Some(datapoint),
                        actuator_target: None,
                        metadata: None,
                    }),
                    fields: vec![
                        proto::v1::Field::Value.into(),
                        proto::v1::Field::Path.into(),
                    ],
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
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
        );

        let mut set_result = Vec::new();

        for (path, datapoint) in datapoints {
            let set_request = proto::v1::SetRequest {
                updates: vec![proto::v1::EntryUpdate {
                    entry: Some(proto::v1::DataEntry {
                        path: path.clone(),
                        value: None,
                        actuator_target: Some(datapoint),
                        metadata: None,
                    }),
                    fields: vec![
                        proto::v1::Field::ActuatorTarget.into(),
                        proto::v1::Field::Path.into(),
                    ],
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
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
        );

        let mut set_result = Vec::new();

        for (path, metadata) in metadatas {
            let set_request = proto::v1::SetRequest {
                updates: vec![proto::v1::EntryUpdate {
                    entry: Some(proto::v1::DataEntry {
                        path: path.clone(),
                        value: None,
                        actuator_target: None,
                        metadata: Some(metadata),
                    }),
                    fields: vec![
                        proto::v1::Field::Metadata.into(),
                        proto::v1::Field::Path.into(),
                    ],
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

    pub async fn subscribe_current_values(
        &mut self,
        paths: Vec<&str>,
    ) -> Result<tonic::Streaming<proto::v1::SubscribeResponse>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
        );

        let mut entries = Vec::new();
        for path in paths {
            entries.push(proto::v1::SubscribeEntry {
                path: path.to_string(),
                view: proto::v1::View::CurrentValue.into(),
                fields: vec![proto::v1::Field::Value.into()],
            })
        }

        let req = proto::v1::SubscribeRequest { entries: entries };

        match client.subscribe(req).await {
            Ok(response) => Ok(response.into_inner()),
            Err(err) => Err(ClientError::Status(err)),
        }
    }

    //masking subscribe curent values with subscribe
    pub async fn subscribe(
        &mut self,
        paths: Vec<&str>,
    ) -> Result<tonic::Streaming<proto::v1::SubscribeResponse>, ClientError> {
        return self.subscribe_current_values(paths).await;
    }

    pub async fn subscribe_target_values(
        &mut self,
        paths: Vec<&str>,
    ) -> Result<tonic::Streaming<proto::v1::SubscribeResponse>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
        );
        let mut entries = Vec::new();
        for path in paths {
            entries.push(proto::v1::SubscribeEntry {
                path: path.to_string(),
                view: proto::v1::View::TargetValue.into(),
                fields: vec![proto::v1::Field::ActuatorTarget.into()],
            })
        }

        let req = proto::v1::SubscribeRequest { entries: entries };

        match client.subscribe(req).await {
            Ok(response) => Ok(response.into_inner()),
            Err(err) => Err(ClientError::Status(err)),
        }
    }

    pub async fn subscribe_metadata(
        &mut self,
        paths: Vec<String>,
    ) -> Result<tonic::Streaming<proto::v1::SubscribeResponse>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
        );
        let mut entries = Vec::new();
        for path in paths {
            entries.push(proto::v1::SubscribeEntry {
                path: path.to_string(),
                view: proto::v1::View::Metadata.into(),
                fields: vec![proto::v1::Field::Metadata.into()],
            })
        }

        let req = proto::v1::SubscribeRequest { entries: entries };

        match client.subscribe(req).await {
            Ok(response) => Ok(response.into_inner()),
            Err(err) => Err(ClientError::Status(err)),
        }
    }
}
