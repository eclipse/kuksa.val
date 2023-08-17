/********************************************************************************
* Copyright (c) 2022 Contributors to the Eclipse Foundation
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

use tonic::{Code, Request, Response, Status};

use databroker_proto::sdv::databroker::v1 as proto;

use tokio_stream::{Stream, StreamExt};

use std::collections::HashMap;
use std::pin::Pin;

use crate::broker::{self, ReadError};
use crate::permissions::Permissions;

use tracing::debug;

#[tonic::async_trait]
impl proto::broker_server::Broker for broker::DataBroker {
    async fn get_datapoints(
        &self,
        request: Request<proto::GetDatapointsRequest>,
    ) -> Result<Response<proto::GetDatapointsReply>, Status> {
        debug!(?request);
        let permissions = match request.extensions().get::<Permissions>() {
            Some(permissions) => {
                debug!(?permissions);
                permissions.clone()
            }
            None => return Err(tonic::Status::unauthenticated("Unauthenticated")),
        };
        let broker = self.authorized_access(&permissions);

        let requested = request.into_inner();
        if requested.datapoints.is_empty() {
            Err(Status::new(
                Code::InvalidArgument,
                "No datapoints requested".to_string(),
            ))
        } else {
            let mut datapoints = HashMap::new();

            for name in requested.datapoints {
                match broker.get_datapoint_by_path(&name).await {
                    Ok(datapoint) => {
                        datapoints.insert(name, proto::Datapoint::from(&datapoint));
                    }
                    Err(err) => {
                        // Datapoint doesn't exist
                        datapoints.insert(
                            name,
                            proto::Datapoint {
                                timestamp: None,
                                value: match err {
                                    ReadError::NotFound => {
                                        Some(proto::datapoint::Value::FailureValue(
                                            proto::datapoint::Failure::UnknownDatapoint.into(),
                                        ))
                                    }
                                    ReadError::PermissionDenied | ReadError::PermissionExpired => {
                                        Some(proto::datapoint::Value::FailureValue(
                                            proto::datapoint::Failure::AccessDenied.into(),
                                        ))
                                    }
                                },
                            },
                        );
                    }
                }
            }

            let reply = proto::GetDatapointsReply { datapoints };

            Ok(Response::new(reply))
        }
    }

    async fn set_datapoints(
        &self,
        request: tonic::Request<proto::SetDatapointsRequest>,
    ) -> Result<tonic::Response<proto::SetDatapointsReply>, Status> {
        debug!(?request);
        let permissions = match request.extensions().get::<Permissions>() {
            Some(permissions) => {
                debug!(?permissions);
                permissions.clone()
            }
            None => return Err(tonic::Status::unauthenticated("Unauthenticated")),
        };
        let broker = self.authorized_access(&permissions);

        // Collect errors encountered
        let mut errors = HashMap::<String, i32>::new();
        let mut id_to_path = HashMap::<i32, String>::new(); // Map id to path for errors
        let message = request.into_inner();

        let ids = {
            let mut ids = Vec::new();
            for (path, datapoint) in message.datapoints {
                match broker.get_metadata_by_path(&path).await {
                    Some(metadata) => {
                        match metadata.entry_type {
                            broker::EntryType::Sensor | broker::EntryType::Attribute => {
                                // Cannot set sensor / attribute through the `Broker` API.
                                debug!("Cannot set sensor / attribute through the `Broker` API.");
                                errors.insert(
                                    path.clone(),
                                    proto::DatapointError::AccessDenied as i32,
                                );
                            }
                            broker::EntryType::Actuator => {
                                ids.push((
                                    metadata.id,
                                    broker::EntryUpdate {
                                        path: None,
                                        datapoint: None,
                                        actuator_target: Some(Some(broker::Datapoint::from(
                                            &datapoint,
                                        ))),
                                        entry_type: None,
                                        data_type: None,
                                        description: None,
                                        allowed: None,
                                    },
                                ));
                            }
                        }
                        id_to_path.insert(metadata.id, path);
                    }
                    None => {
                        errors.insert(path.clone(), proto::DatapointError::UnknownDatapoint as i32);
                    }
                };
            }
            ids
        };

        match broker.update_entries(ids).await {
            Ok(()) => {}
            Err(err) => {
                debug!("Failed to set datapoint: {:?}", err);
                errors.extend(err.iter().map(|(id, error)| {
                    (
                        id_to_path[id].clone(),
                        proto::DatapointError::from(error) as i32,
                    )
                }))
            }
        }

        Ok(Response::new(proto::SetDatapointsReply { errors }))
    }

    type SubscribeStream =
        Pin<Box<dyn Stream<Item = Result<proto::SubscribeReply, Status>> + Send + Sync + 'static>>;

    async fn subscribe(
        &self,
        request: tonic::Request<proto::SubscribeRequest>,
    ) -> Result<tonic::Response<Self::SubscribeStream>, tonic::Status> {
        debug!(?request);
        let permissions = match request.extensions().get::<Permissions>() {
            Some(permissions) => {
                debug!(?permissions);
                permissions.clone()
            }
            None => return Err(tonic::Status::unauthenticated("Unauthenticated")),
        };
        let broker = self.authorized_access(&permissions);

        let query = request.into_inner().query;
        match broker.subscribe_query(&query).await {
            Ok(stream) => {
                let stream = convert_to_proto_stream(stream);
                debug!("Subscribed to new query");
                Ok(Response::new(Box::pin(stream)))
            }
            Err(e) => Err(Status::new(Code::InvalidArgument, format!("{e:?}"))),
        }
    }

    async fn get_metadata(
        &self,
        request: tonic::Request<proto::GetMetadataRequest>,
    ) -> Result<tonic::Response<proto::GetMetadataReply>, tonic::Status> {
        debug!(?request);
        let permissions = match request.extensions().get::<Permissions>() {
            Some(permissions) => {
                debug!(?permissions);
                permissions.clone()
            }
            None => return Err(tonic::Status::unauthenticated("Unauthenticated")),
        };

        let broker = self.authorized_access(&permissions);

        let request = request.into_inner();

        let list = if request.names.is_empty() {
            broker
                .map_metadata(|metadata| proto::Metadata::from(metadata))
                .await
        } else {
            let mut list = Vec::new();

            for name in request.names {
                if let Some(metadata) = broker.get_metadata_by_path(&name).await {
                    list.push(proto::Metadata::from(&metadata));
                }
            }
            list
        };
        let reply = proto::GetMetadataReply { list };
        Ok(Response::new(reply))
    }
}

fn convert_to_proto_stream(
    input: impl Stream<Item = broker::QueryResponse>,
) -> impl Stream<Item = Result<proto::SubscribeReply, Status>> {
    input.map(move |item| {
        // debug!("item.id: {:?}", item.value);
        let mut datapoints = HashMap::new();
        for field in item.fields {
            let value = proto::Datapoint::from(&field);
            datapoints.insert(field.name, value);
        }
        let notification = proto::SubscribeReply { fields: datapoints };
        Ok(notification)
    })
}
