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

use prost_types::Timestamp;
use tonic::{Code, Request, Response, Status};

use databroker_api::sdv::databroker::v1 as proto;

use tokio::select;
use tokio::sync::mpsc;
use tokio_stream::wrappers::ReceiverStream;
use tokio_stream::{Stream, StreamExt};

use std::collections::HashMap;
use std::convert::TryInto;
use std::pin::Pin;
use std::time::SystemTime;

use crate::broker;
use crate::types::{ChangeType, DataType, DataValue};

use tracing::debug;

impl From<&proto::Datapoint> for broker::Datapoint {
    fn from(datapoint: &proto::Datapoint) -> Self {
        let value = match &datapoint.value {
            Some(value) => match value {
                proto::datapoint::Value::StringValue(value) => DataValue::String(value.to_owned()),
                proto::datapoint::Value::BoolValue(value) => DataValue::Bool(*value),
                proto::datapoint::Value::Int32Value(value) => DataValue::Int32(*value),
                proto::datapoint::Value::Int64Value(value) => DataValue::Int64(*value),
                proto::datapoint::Value::Uint32Value(value) => DataValue::Uint32(*value),
                proto::datapoint::Value::Uint64Value(value) => DataValue::Uint64(*value),
                proto::datapoint::Value::FloatValue(value) => DataValue::Float(*value),
                proto::datapoint::Value::DoubleValue(value) => DataValue::Double(*value),
                proto::datapoint::Value::StringArray(array) => {
                    DataValue::StringArray(array.values.clone())
                }
                proto::datapoint::Value::BoolArray(array) => {
                    DataValue::BoolArray(array.values.clone())
                }
                proto::datapoint::Value::Int32Array(array) => {
                    DataValue::Int32Array(array.values.clone())
                }
                proto::datapoint::Value::Int64Array(array) => {
                    DataValue::Int64Array(array.values.clone())
                }
                proto::datapoint::Value::Uint32Array(array) => {
                    DataValue::Uint32Array(array.values.clone())
                }
                proto::datapoint::Value::Uint64Array(array) => {
                    DataValue::Uint64Array(array.values.clone())
                }
                proto::datapoint::Value::FloatArray(array) => {
                    DataValue::FloatArray(array.values.clone())
                }
                proto::datapoint::Value::DoubleArray(array) => {
                    DataValue::DoubleArray(array.values.clone())
                }
                proto::datapoint::Value::FailureValue(_) => DataValue::NotAvailable,
            },
            None => DataValue::NotAvailable,
        };

        let ts = match &datapoint.timestamp {
            Some(ts) => ts.clone().try_into().unwrap_or_else(|_| SystemTime::now()),
            None => SystemTime::now(),
        };

        broker::Datapoint { ts, value }
    }
}

impl From<&broker::Datapoint> for proto::Datapoint {
    fn from(datapoint: &broker::Datapoint) -> Self {
        let value = match &datapoint.value {
            DataValue::Bool(value) => proto::datapoint::Value::BoolValue(*value),
            DataValue::String(value) => proto::datapoint::Value::StringValue(value.to_owned()),
            DataValue::Int32(value) => proto::datapoint::Value::Int32Value(*value),
            DataValue::Int64(value) => proto::datapoint::Value::Int64Value(*value),
            DataValue::Uint32(value) => proto::datapoint::Value::Uint32Value(*value),
            DataValue::Uint64(value) => proto::datapoint::Value::Uint64Value(*value),
            DataValue::Float(value) => proto::datapoint::Value::FloatValue(*value),
            DataValue::Double(value) => proto::datapoint::Value::DoubleValue(*value),
            DataValue::BoolArray(array) => proto::datapoint::Value::BoolArray(proto::BoolArray {
                values: array.clone(),
            }),
            DataValue::StringArray(array) => {
                proto::datapoint::Value::StringArray(proto::StringArray {
                    values: array.clone(),
                })
            }
            DataValue::Int32Array(array) => {
                proto::datapoint::Value::Int32Array(proto::Int32Array {
                    values: array.clone(),
                })
            }
            DataValue::Int64Array(array) => {
                proto::datapoint::Value::Int64Array(proto::Int64Array {
                    values: array.clone(),
                })
            }
            DataValue::Uint32Array(array) => {
                proto::datapoint::Value::Uint32Array(proto::Uint32Array {
                    values: array.clone(),
                })
            }
            DataValue::Uint64Array(array) => {
                proto::datapoint::Value::Uint64Array(proto::Uint64Array {
                    values: array.clone(),
                })
            }
            DataValue::FloatArray(array) => {
                proto::datapoint::Value::FloatArray(proto::FloatArray {
                    values: array.clone(),
                })
            }
            DataValue::DoubleArray(array) => {
                proto::datapoint::Value::DoubleArray(proto::DoubleArray {
                    values: array.clone(),
                })
            }
            DataValue::NotAvailable => proto::datapoint::Value::FailureValue(
                proto::datapoint::Failure::NotAvailable as i32,
            ),
        };

        proto::Datapoint {
            timestamp: Some(datapoint.ts.into()),
            value: Some(value),
        }
    }
}

impl From<&DataValue> for proto::Datapoint {
    fn from(data_value: &DataValue) -> Self {
        let value = match &data_value {
            DataValue::Bool(value) => proto::datapoint::Value::BoolValue(*value),
            DataValue::String(value) => proto::datapoint::Value::StringValue(value.to_owned()),
            DataValue::Int32(value) => proto::datapoint::Value::Int32Value(*value),
            DataValue::Int64(value) => proto::datapoint::Value::Int64Value(*value),
            DataValue::Uint32(value) => proto::datapoint::Value::Uint32Value(*value),
            DataValue::Uint64(value) => proto::datapoint::Value::Uint64Value(*value),
            DataValue::Float(value) => proto::datapoint::Value::FloatValue(*value),
            DataValue::Double(value) => proto::datapoint::Value::DoubleValue(*value),
            DataValue::BoolArray(array) => proto::datapoint::Value::BoolArray(proto::BoolArray {
                values: array.clone(),
            }),
            DataValue::StringArray(array) => {
                proto::datapoint::Value::StringArray(proto::StringArray {
                    values: array.clone(),
                })
            }
            DataValue::Int32Array(array) => {
                proto::datapoint::Value::Int32Array(proto::Int32Array {
                    values: array.clone(),
                })
            }
            DataValue::Int64Array(array) => {
                proto::datapoint::Value::Int64Array(proto::Int64Array {
                    values: array.clone(),
                })
            }
            DataValue::Uint32Array(array) => {
                proto::datapoint::Value::Uint32Array(proto::Uint32Array {
                    values: array.clone(),
                })
            }
            DataValue::Uint64Array(array) => {
                proto::datapoint::Value::Uint64Array(proto::Uint64Array {
                    values: array.clone(),
                })
            }
            DataValue::FloatArray(array) => {
                proto::datapoint::Value::FloatArray(proto::FloatArray {
                    values: array.clone(),
                })
            }
            DataValue::DoubleArray(array) => {
                proto::datapoint::Value::DoubleArray(proto::DoubleArray {
                    values: array.clone(),
                })
            }
            DataValue::NotAvailable => proto::datapoint::Value::FailureValue(
                proto::datapoint::Failure::NotAvailable as i32,
            ),
        };

        proto::Datapoint {
            timestamp: Some(Timestamp::from(SystemTime::now())),
            value: Some(value),
        }
    }
}

impl From<&proto::DataType> for DataType {
    fn from(data_type: &proto::DataType) -> Self {
        match data_type {
            proto::DataType::Bool => DataType::Bool,
            proto::DataType::String => DataType::String,
            proto::DataType::Int8 => DataType::Int8,
            proto::DataType::Int16 => DataType::Int16,
            proto::DataType::Int32 => DataType::Int32,
            proto::DataType::Int64 => DataType::Int64,
            proto::DataType::Uint8 => DataType::Uint8,
            proto::DataType::Uint16 => DataType::Uint16,
            proto::DataType::Uint32 => DataType::Uint32,
            proto::DataType::Uint64 => DataType::Uint64,
            proto::DataType::Float => DataType::Float,
            proto::DataType::Double => DataType::Double,
            proto::DataType::Timestamp => DataType::Timestamp,
            proto::DataType::StringArray => DataType::StringArray,
            proto::DataType::BoolArray => DataType::BoolArray,
            proto::DataType::Int8Array => DataType::Int8Array,
            proto::DataType::Int16Array => DataType::Int16Array,
            proto::DataType::Int32Array => DataType::Int32Array,
            proto::DataType::Int64Array => DataType::Int64Array,
            proto::DataType::Uint8Array => DataType::Uint8Array,
            proto::DataType::Uint16Array => DataType::Uint16Array,
            proto::DataType::Uint32Array => DataType::Uint32Array,
            proto::DataType::Uint64Array => DataType::Uint64Array,
            proto::DataType::FloatArray => DataType::FloatArray,
            proto::DataType::DoubleArray => DataType::DoubleArray,
            proto::DataType::TimestampArray => DataType::TimestampArray,
        }
    }
}

impl From<&DataType> for proto::DataType {
    fn from(value_type: &DataType) -> Self {
        match value_type {
            DataType::Bool => proto::DataType::Bool,
            DataType::String => proto::DataType::String,
            DataType::Int8 => proto::DataType::Int8,
            DataType::Int16 => proto::DataType::Int16,
            DataType::Int32 => proto::DataType::Int32,
            DataType::Int64 => proto::DataType::Int64,
            DataType::Uint8 => proto::DataType::Uint8,
            DataType::Uint16 => proto::DataType::Uint16,
            DataType::Uint32 => proto::DataType::Uint32,
            DataType::Uint64 => proto::DataType::Uint64,
            DataType::Float => proto::DataType::Float,
            DataType::Double => proto::DataType::Double,
            DataType::Timestamp => proto::DataType::Timestamp,
            DataType::StringArray => proto::DataType::StringArray,
            DataType::BoolArray => proto::DataType::BoolArray,
            DataType::Int8Array => proto::DataType::Int8Array,
            DataType::Int16Array => proto::DataType::Int16Array,
            DataType::Int32Array => proto::DataType::Int32Array,
            DataType::Int64Array => proto::DataType::Int64Array,
            DataType::Uint8Array => proto::DataType::Uint8Array,
            DataType::Uint16Array => proto::DataType::Uint16Array,
            DataType::Uint32Array => proto::DataType::Uint32Array,
            DataType::Uint64Array => proto::DataType::Uint64Array,
            DataType::FloatArray => proto::DataType::FloatArray,
            DataType::DoubleArray => proto::DataType::DoubleArray,
            DataType::TimestampArray => proto::DataType::TimestampArray,
        }
    }
}

impl From<&proto::ChangeType> for ChangeType {
    fn from(change_type: &proto::ChangeType) -> Self {
        match change_type {
            proto::ChangeType::OnChange => ChangeType::OnChange,
            proto::ChangeType::Continuous => ChangeType::Continuous,
            proto::ChangeType::Static => ChangeType::Static,
        }
    }
}

impl From<&broker::Metadata> for proto::Metadata {
    fn from(metadata: &broker::Metadata) -> Self {
        proto::Metadata {
            id: metadata.id,
            name: metadata.path.to_owned(),
            data_type: proto::DataType::from(&metadata.data_type) as i32,
            change_type: proto::ChangeType::Continuous as i32, // TODO: Add to metadata
            description: metadata.description.to_owned(),
        }
    }
}

impl From<&broker::UpdateError> for proto::DatapointError {
    fn from(error: &broker::UpdateError) -> Self {
        match error {
            broker::UpdateError::NotFound => proto::DatapointError::UnknownDatapoint,
            broker::UpdateError::WrongType | broker::UpdateError::UnsupportedType => {
                proto::DatapointError::InvalidType
            }
            broker::UpdateError::OutOfBounds => proto::DatapointError::OutOfBounds,
        }
    }
}

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
                            DataType::from(&value_type),
                            ChangeType::from(&change_type),
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
}

fn convert_to_proto_stream(
    input: impl Stream<Item = broker::QueryResponse>,
) -> impl Stream<Item = Result<proto::SubscribeReply, Status>> {
    input.map(move |item| {
        // debug!("item.id: {:?}", item.value);
        let mut datapoints = HashMap::new();
        for field in item.fields {
            datapoints.insert(field.name, proto::Datapoint::from(&field.value));
        }
        let notification = proto::SubscribeReply { fields: datapoints };
        Ok(notification)
    })
}

#[tonic::async_trait]
impl proto::broker_server::Broker for broker::DataBroker {
    async fn get_datapoints(
        &self,
        request: Request<proto::GetDatapointsRequest>,
    ) -> Result<Response<proto::GetDatapointsReply>, Status> {
        debug!("Got a request: {:?}", request);

        let requested = request.into_inner();
        if requested.datapoints.is_empty() {
            Err(Status::new(
                Code::InvalidArgument,
                "No datapoints requested".to_string(),
            ))
        } else {
            let mut datapoints = HashMap::new();

            for name in requested.datapoints {
                match self.get_datapoint_by_path(&name).await {
                    Some(datapoint) => {
                        datapoints.insert(name, proto::Datapoint::from(&datapoint));
                    }
                    None => {
                        // Datapoint doesn't exist
                        datapoints.insert(
                            name,
                            proto::Datapoint {
                                timestamp: None,
                                value: Some(proto::datapoint::Value::FailureValue(
                                    proto::datapoint::Failure::UnknownDatapoint as i32,
                                )),
                            },
                        );
                    }
                }
            }

            let reply = proto::GetDatapointsReply { datapoints };

            Ok(Response::new(reply))
        }
    }

    type SubscribeStream =
        Pin<Box<dyn Stream<Item = Result<proto::SubscribeReply, Status>> + Send + Sync + 'static>>;

    async fn subscribe(
        &self,
        request: tonic::Request<proto::SubscribeRequest>,
    ) -> Result<tonic::Response<Self::SubscribeStream>, tonic::Status> {
        let query = request.into_inner().query;
        match self.subscribe_query(&query).await {
            Ok(stream) => {
                let stream = convert_to_proto_stream(stream);
                debug!("Subscribed to new query");
                Ok(Response::new(Box::pin(stream)))
            }
            Err(e) => Err(Status::new(Code::InvalidArgument, format!("{:?}", e))),
        }
    }

    async fn get_metadata(
        &self,
        request: tonic::Request<proto::GetMetadataRequest>,
    ) -> Result<tonic::Response<proto::GetMetadataReply>, tonic::Status> {
        let request = request.into_inner();

        let list = if request.names.is_empty() {
            self.entries
                .read()
                .await
                .iter()
                .map(|entry| proto::Metadata::from(&entry.metadata))
                .collect()
        } else {
            let mut list = Vec::new();

            let entries = self.entries.read().await;
            for name in request.names {
                if let Some(entry) = entries.get_by_path(&name) {
                    list.push(proto::Metadata::from(&entry.metadata));
                }
            }
            list
        };
        let reply = proto::GetMetadataReply { list };
        Ok(Response::new(reply))
    }
}
