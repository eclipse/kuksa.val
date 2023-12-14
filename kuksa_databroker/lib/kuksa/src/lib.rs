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

use http::Uri;
use std::collections::HashMap;

use databroker_proto::kuksa::val::{self as proto, v1::DataEntry};

use kuksa_common::{Client, ClientError};

#[derive(Debug)]
pub struct KuksaClient {
    pub basic_client: Client,
}

impl KuksaClient {
    pub fn new(uri: Uri) -> Self {
        KuksaClient {
            basic_client: Client::new(uri),
        }
    }

    async fn set(&mut self, entry: DataEntry, _fields: Vec<i32>) -> Result<(), ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
        );
        let set_request = proto::v1::SetRequest {
            updates: vec![proto::v1::EntryUpdate {
                entry: Some(entry),
                fields: _fields,
            }],
        };
        match client.set(set_request).await {
            Ok(response) => {
                let message = response.into_inner();
                let mut errors: Vec<proto::v1::Error> = Vec::new();
                if let Some(err) = message.error {
                    errors.push(err);
                }
                for error in message.errors {
                    if let Some(err) = error.error {
                        errors.push(err);
                    }
                }
                if errors.is_empty() {
                    Ok(())
                } else {
                    Err(ClientError::Function(errors))
                }
            }
            Err(err) => Err(ClientError::Status(err)),
        }
    }

    async fn get(
        &mut self,
        path: &str,
        view: proto::v1::View,
        _fields: Vec<i32>,
    ) -> Result<Vec<DataEntry>, ClientError> {
        let mut client = proto::v1::val_client::ValClient::with_interceptor(
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
        );

        let get_request = proto::v1::GetRequest {
            entries: vec![proto::v1::EntryRequest {
                path: path.to_string(),
                view: view.into(),
                fields: _fields,
            }],
        };

        match client.get(get_request).await {
            Ok(response) => {
                let message = response.into_inner();
                let mut errors = Vec::new();
                if let Some(err) = message.error {
                    errors.push(err);
                }
                for error in message.errors {
                    if let Some(err) = error.error {
                        errors.push(err);
                    }
                }
                if !errors.is_empty() {
                    Err(ClientError::Function(errors))
                } else {
                    // since there is only one DataEntry in the vector return only the according DataEntry
                    Ok(message.entries.clone())
                }
            }
            Err(err) => Err(ClientError::Status(err)),
        }
    }

    pub async fn get_metadata(&mut self, paths: Vec<&str>) -> Result<Vec<DataEntry>, ClientError> {
        let mut metadata_result = Vec::new();

        for path in paths {
            match self
                .get(
                    path,
                    proto::v1::View::Metadata,
                    vec![proto::v1::Field::Metadata.into()],
                )
                .await
            {
                Ok(mut entry) => metadata_result.append(&mut entry),
                Err(err) => return Err(err),
            }
        }

        Ok(metadata_result)
    }

    pub async fn get_current_values(
        &mut self,
        paths: Vec<String>,
    ) -> Result<Vec<DataEntry>, ClientError> {
        let mut get_result = Vec::new();

        for path in paths {
            match self
                .get(
                    &path,
                    proto::v1::View::CurrentValue,
                    vec![
                        proto::v1::Field::Value.into(),
                        proto::v1::Field::Metadata.into(),
                    ],
                )
                .await
            {
                Ok(mut entry) => get_result.append(&mut entry),
                Err(err) => return Err(err),
            }
        }

        Ok(get_result)
    }

    pub async fn get_target_values(
        &mut self,
        paths: Vec<&str>,
    ) -> Result<Vec<DataEntry>, ClientError> {
        let mut get_result = Vec::new();

        for path in paths {
            match self
                .get(
                    path,
                    proto::v1::View::TargetValue,
                    vec![
                        proto::v1::Field::ActuatorTarget.into(),
                        proto::v1::Field::Metadata.into(),
                    ],
                )
                .await
            {
                Ok(mut entry) => get_result.append(&mut entry),
                Err(err) => return Err(err),
            }
        }

        Ok(get_result)
    }

    pub async fn set_current_values(
        &mut self,
        datapoints: HashMap<String, proto::v1::Datapoint>,
    ) -> Result<(), ClientError> {
        for (path, datapoint) in datapoints {
            match self
                .set(
                    proto::v1::DataEntry {
                        path: path.clone(),
                        value: Some(datapoint),
                        actuator_target: None,
                        metadata: None,
                    },
                    vec![
                        proto::v1::Field::Value.into(),
                        proto::v1::Field::Path.into(),
                    ],
                )
                .await
            {
                Ok(_) => {
                    continue;
                }
                Err(err) => return Err(err),
            }
        }

        Ok(())
    }

    pub async fn set_target_values(
        &mut self,
        datapoints: HashMap<String, proto::v1::Datapoint>,
    ) -> Result<(), ClientError> {
        for (path, datapoint) in datapoints {
            match self
                .set(
                    proto::v1::DataEntry {
                        path: path.clone(),
                        value: None,
                        actuator_target: Some(datapoint),
                        metadata: None,
                    },
                    vec![
                        proto::v1::Field::ActuatorTarget.into(),
                        proto::v1::Field::Path.into(),
                    ],
                )
                .await
            {
                Ok(_) => {
                    continue;
                }
                Err(err) => return Err(err),
            }
        }

        Ok(())
    }

    pub async fn set_metadata(
        &mut self,
        metadatas: HashMap<String, proto::v1::Metadata>,
    ) -> Result<(), ClientError> {
        for (path, metadata) in metadatas {
            match self
                .set(
                    proto::v1::DataEntry {
                        path: path.clone(),
                        value: None,
                        actuator_target: None,
                        metadata: Some(metadata),
                    },
                    vec![
                        proto::v1::Field::Metadata.into(),
                        proto::v1::Field::Path.into(),
                    ],
                )
                .await
            {
                Ok(_) => {
                    continue;
                }
                Err(err) => return Err(err),
            }
        }

        Ok(())
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

        let req = proto::v1::SubscribeRequest { entries };

        match client.subscribe(req).await {
            Ok(response) => Ok(response.into_inner()),
            Err(err) => Err(ClientError::Status(err)),
        }
    }

    //masking subscribe curent values with subscribe due to plugability
    pub async fn subscribe(
        &mut self,
        paths: Vec<&str>,
    ) -> Result<tonic::Streaming<proto::v1::SubscribeResponse>, ClientError> {
        self.subscribe_current_values(paths).await
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

        let req = proto::v1::SubscribeRequest { entries };

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

        let req = proto::v1::SubscribeRequest { entries };

        match client.subscribe(req).await {
            Ok(response) => Ok(response.into_inner()),
            Err(err) => Err(ClientError::Status(err)),
        }
    }
}
