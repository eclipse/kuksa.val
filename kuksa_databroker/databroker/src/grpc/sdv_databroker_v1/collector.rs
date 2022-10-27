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

use std::collections::HashMap;
use std::pin::Pin;

use databroker_proto::sdv::databroker::v1 as proto;

use tokio::select;
use tokio::sync::mpsc;
use tokio_stream::wrappers::ReceiverStream;
use tokio_stream::{Stream, StreamExt};

use tonic::{Code, Response, Status};
use tracing::debug;

use crate::broker;

#[tonic::async_trait]
impl proto::collector_server::Collector for broker::DataBroker {
    async fn update_datapoints(
        &self,
        request: tonic::Request<proto::UpdateDatapointsRequest>,
    ) -> Result<tonic::Response<proto::UpdateDatapointsReply>, tonic::Status> {
        // Collect errors encountered
        let mut errors = HashMap::new();

        let message = request.into_inner();
        let ids: Vec<(i32, broker::EntryUpdate)> = message
            .datapoints
            .iter()
            .map(|(id, datapoint)| {
                (
                    *id,
                    broker::EntryUpdate {
                        path: None,
                        datapoint: Some(broker::Datapoint::from(datapoint)),
                        actuator_target: None,
                        entry_type: None,
                        data_type: None,
                        description: None,
                    },
                )
            })
            .collect();

        match self.update_entries(ids).await {
            Ok(()) => {}
            Err(err) => {
                debug!("Failed to set datapoint: {:?}", err);
                errors = err
                    .iter()
                    .map(|(id, error)| (*id, proto::DatapointError::from(error) as i32))
                    .collect();
            }
        }

        Ok(Response::new(proto::UpdateDatapointsReply { errors }))
    }

    type StreamDatapointsStream = ReceiverStream<Result<proto::StreamDatapointsReply, Status>>;

    async fn stream_datapoints(
        &self,
        request: tonic::Request<tonic::Streaming<proto::StreamDatapointsRequest>>,
    ) -> Result<tonic::Response<Self::StreamDatapointsStream>, tonic::Status> {
        let mut stream = request.into_inner();

        let mut shutdown_trigger = self.get_shutdown_trigger();
        let databroker = self.clone();

        // Create error stream (to be returned)
        let (error_sender, error_receiver) = mpsc::channel(10);

        // Listening on stream
        tokio::spawn(async move {
            loop {
                select! {
                    message = stream.message() => {
                        match message {
                            Ok(request) => {
                                match request {
                                    Some(req) => {
                                        let ids: Vec<(i32, broker::EntryUpdate)> = req.datapoints
                                            .iter()
                                            .map(|(id, datapoint)|
                                                (
                                                    *id,
                                                    broker::EntryUpdate {
                                                        path: None,
                                                        datapoint: Some(broker::Datapoint::from(datapoint)),
                                                        actuator_target: None,
                                                        entry_type: None,
                                                        data_type: None,
                                                        description: None
                                                    }
                                                )
                                            )
                                            .collect();
                                        // TODO: Check if sender is allowed to provide datapoint with this id
                                        match databroker
                                            .update_entries(ids)
                                            .await
                                        {
                                            Ok(_) => {}
                                            Err(err) => {
                                                if let Err(err) = error_sender.send(
                                                    Ok(proto::StreamDatapointsReply {
                                                        errors: err.iter().map(|(id, error)| {
                                                            (*id, proto::DatapointError::from(error) as i32)
                                                        }).collect(),
                                                    })
                                                ).await {
                                                    debug!("Failed to send errors: {}", err);
                                                }
                                            }
                                        }
                                    },
                                    None => {
                                        debug!("provider: no more messages");
                                        break;
                                    }
                                }
                            },
                            Err(err) => {
                                debug!("provider: connection broken: {:?}", err);
                                break;
                            },
                        }
                    },
                    _ = shutdown_trigger.recv() => {
                        debug!("provider: shutdown received");
                        break;
                    }
                }
            }
        });

        // Return the error stream
        Ok(Response::new(ReceiverStream::new(error_receiver)))
    }

    async fn register_datapoints(
        &self,
        request: tonic::Request<proto::RegisterDatapointsRequest>,
    ) -> Result<tonic::Response<proto::RegisterDatapointsReply>, Status> {
        let mut results = HashMap::new();
        let mut error = None;

        for metadata in request.into_inner().list {
            match (
                proto::DataType::from_i32(metadata.data_type),
                proto::ChangeType::from_i32(metadata.change_type),
            ) {
                (Some(value_type), Some(change_type)) => {
                    match self
                        .add_entry(
                            metadata.name.clone(),
                            broker::DataType::from(&value_type),
                            broker::ChangeType::from(&change_type),
                            broker::types::EntryType::Sensor,
                            metadata.description,
                        )
                        .await
                    {
                        Ok(id) => results.insert(metadata.name, id),
                        Err(_) => {
                            // Registration error
                            error = Some(Status::new(
                                Code::InvalidArgument,
                                format!("Failed to register {}", metadata.name),
                            ));
                            break;
                        }
                    };
                }
                (None, _) => {
                    // Invalid data type
                    error = Some(Status::new(
                        Code::InvalidArgument,
                        format!("Unsupported data type provided for {}", metadata.name),
                    ));
                    break;
                }
                (_, None) => {
                    // Invalid change type
                    error = Some(Status::new(
                        Code::InvalidArgument,
                        format!("Unsupported change type provided for {}", metadata.name),
                    ));
                    break;
                }
            }
        }

        match error {
            Some(error) => Err(error),
            None => Ok(Response::new(proto::RegisterDatapointsReply { results })),
        }
    }

    type SubscribeActuatorTargetsStream = Pin<
        Box<
            dyn Stream<Item = Result<proto::SubscribeActuatorTargetReply, Status>>
                + Send
                + Sync
                + 'static,
        >,
    >;
    async fn subscribe_actuator_targets(
        &self,
        request: tonic::Request<proto::SubscribeActuatorTargetRequest>,
    ) -> Result<tonic::Response<Self::SubscribeActuatorTargetsStream>, tonic::Status> {
        debug!("{:?}", request);
        let request = request.into_inner();
        let paths = request.paths;
        match self
            .subscribe(paths, vec![broker::Field::ActuatorTarget])
            .await
        {
            Ok(stream) => {
                let stream = convert_to_proto_stream(stream);
                debug!("Subscribed to new query");
                Ok(tonic::Response::new(Box::pin(stream)))
            }
            Err(e) => Err(tonic::Status::new(
                tonic::Code::InvalidArgument,
                format!("{:?}", e),
            )),
        }
    }
}

fn convert_to_proto_stream(
    input: impl Stream<Item = broker::EntryUpdates>,
) -> impl tokio_stream::Stream<Item = Result<proto::SubscribeActuatorTargetReply, tonic::Status>> {
    input.map(move |item| {
        let mut actuator_targets = HashMap::new();
        for update in item.updates {
            if let (Some(path), Some(actuator_target)) =
                (&update.update.path, update.get_actuator_target())
            {
                actuator_targets.insert(path.to_owned(), proto::Datapoint::from(&actuator_target));
            }
        }
        let response = proto::SubscribeActuatorTargetReply { actuator_targets };
        Ok(response)
    })
}
