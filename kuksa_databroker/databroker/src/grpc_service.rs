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
use tonic::{transport::Server, Code, Request, Response, Status};

use databroker_api::proto;

use tokio::select;
use tokio::sync::mpsc;
use tokio_stream::wrappers::ReceiverStream;
use tokio_stream::{Stream, StreamExt};

use std::collections::HashMap;
use std::convert::TryInto;
use std::future::Future;
use std::pin::Pin;
use std::time::{Duration, SystemTime};

use crate::broker;
use crate::types::{ChangeType, DataType, DataValue};

use tracing::{debug, info};

impl From<&proto::v1::Datapoint> for broker::DataPoint {
    fn from(datapoint: &proto::v1::Datapoint) -> Self {
        let value = match &datapoint.value {
            Some(value) => match value {
                proto::v1::datapoint::Value::StringValue(value) => {
                    DataValue::String(value.to_owned())
                }
                proto::v1::datapoint::Value::BoolValue(value) => DataValue::Bool(*value),
                proto::v1::datapoint::Value::Int32Value(value) => DataValue::Int32(*value),
                proto::v1::datapoint::Value::Int64Value(value) => DataValue::Int64(*value),
                proto::v1::datapoint::Value::Uint32Value(value) => DataValue::Uint32(*value),
                proto::v1::datapoint::Value::Uint64Value(value) => DataValue::Uint64(*value),
                proto::v1::datapoint::Value::FloatValue(value) => DataValue::Float(*value),
                proto::v1::datapoint::Value::DoubleValue(value) => DataValue::Double(*value),
                proto::v1::datapoint::Value::StringArray(array) => {
                    DataValue::StringArray(array.values.clone())
                }
                proto::v1::datapoint::Value::BoolArray(array) => {
                    DataValue::BoolArray(array.values.clone())
                }
                proto::v1::datapoint::Value::Int32Array(array) => {
                    DataValue::Int32Array(array.values.clone())
                }
                proto::v1::datapoint::Value::Int64Array(array) => {
                    DataValue::Int64Array(array.values.clone())
                }
                proto::v1::datapoint::Value::Uint32Array(array) => {
                    DataValue::Uint32Array(array.values.clone())
                }
                proto::v1::datapoint::Value::Uint64Array(array) => {
                    DataValue::Uint64Array(array.values.clone())
                }
                proto::v1::datapoint::Value::FloatArray(array) => {
                    DataValue::FloatArray(array.values.clone())
                }
                proto::v1::datapoint::Value::DoubleArray(array) => {
                    DataValue::DoubleArray(array.values.clone())
                }
                proto::v1::datapoint::Value::FailureValue(_) => DataValue::NotAvailable,
            },
            None => DataValue::NotAvailable,
        };

        let ts = match &datapoint.timestamp {
            Some(ts) => ts.clone().try_into().unwrap_or_else(|_| SystemTime::now()),
            None => SystemTime::now(),
        };

        broker::DataPoint { ts, value }
    }
}

impl From<&broker::DataPoint> for proto::v1::Datapoint {
    fn from(datapoint: &broker::DataPoint) -> Self {
        let value = match &datapoint.value {
            DataValue::Bool(value) => proto::v1::datapoint::Value::BoolValue(*value),
            DataValue::String(value) => proto::v1::datapoint::Value::StringValue(value.to_owned()),
            DataValue::Int32(value) => proto::v1::datapoint::Value::Int32Value(*value),
            DataValue::Int64(value) => proto::v1::datapoint::Value::Int64Value(*value),
            DataValue::Uint32(value) => proto::v1::datapoint::Value::Uint32Value(*value),
            DataValue::Uint64(value) => proto::v1::datapoint::Value::Uint64Value(*value),
            DataValue::Float(value) => proto::v1::datapoint::Value::FloatValue(*value),
            DataValue::Double(value) => proto::v1::datapoint::Value::DoubleValue(*value),
            DataValue::BoolArray(array) => {
                proto::v1::datapoint::Value::BoolArray(proto::v1::BoolArray {
                    values: array.clone(),
                })
            }
            DataValue::StringArray(array) => {
                proto::v1::datapoint::Value::StringArray(proto::v1::StringArray {
                    values: array.clone(),
                })
            }
            DataValue::Int32Array(array) => {
                proto::v1::datapoint::Value::Int32Array(proto::v1::Int32Array {
                    values: array.clone(),
                })
            }
            DataValue::Int64Array(array) => {
                proto::v1::datapoint::Value::Int64Array(proto::v1::Int64Array {
                    values: array.clone(),
                })
            }
            DataValue::Uint32Array(array) => {
                proto::v1::datapoint::Value::Uint32Array(proto::v1::Uint32Array {
                    values: array.clone(),
                })
            }
            DataValue::Uint64Array(array) => {
                proto::v1::datapoint::Value::Uint64Array(proto::v1::Uint64Array {
                    values: array.clone(),
                })
            }
            DataValue::FloatArray(array) => {
                proto::v1::datapoint::Value::FloatArray(proto::v1::FloatArray {
                    values: array.clone(),
                })
            }
            DataValue::DoubleArray(array) => {
                proto::v1::datapoint::Value::DoubleArray(proto::v1::DoubleArray {
                    values: array.clone(),
                })
            }
            DataValue::NotAvailable => proto::v1::datapoint::Value::FailureValue(
                proto::v1::datapoint::Failure::NotAvailable as i32,
            ),
        };

        proto::v1::Datapoint {
            timestamp: Some(datapoint.ts.into()),
            value: Some(value),
        }
    }
}

impl From<&DataValue> for proto::v1::Datapoint {
    fn from(data_value: &DataValue) -> Self {
        let value = match &data_value {
            DataValue::Bool(value) => proto::v1::datapoint::Value::BoolValue(*value),
            DataValue::String(value) => proto::v1::datapoint::Value::StringValue(value.to_owned()),
            DataValue::Int32(value) => proto::v1::datapoint::Value::Int32Value(*value),
            DataValue::Int64(value) => proto::v1::datapoint::Value::Int64Value(*value),
            DataValue::Uint32(value) => proto::v1::datapoint::Value::Uint32Value(*value),
            DataValue::Uint64(value) => proto::v1::datapoint::Value::Uint64Value(*value),
            DataValue::Float(value) => proto::v1::datapoint::Value::FloatValue(*value),
            DataValue::Double(value) => proto::v1::datapoint::Value::DoubleValue(*value),
            DataValue::BoolArray(array) => {
                proto::v1::datapoint::Value::BoolArray(proto::v1::BoolArray {
                    values: array.clone(),
                })
            }
            DataValue::StringArray(array) => {
                proto::v1::datapoint::Value::StringArray(proto::v1::StringArray {
                    values: array.clone(),
                })
            }
            DataValue::Int32Array(array) => {
                proto::v1::datapoint::Value::Int32Array(proto::v1::Int32Array {
                    values: array.clone(),
                })
            }
            DataValue::Int64Array(array) => {
                proto::v1::datapoint::Value::Int64Array(proto::v1::Int64Array {
                    values: array.clone(),
                })
            }
            DataValue::Uint32Array(array) => {
                proto::v1::datapoint::Value::Uint32Array(proto::v1::Uint32Array {
                    values: array.clone(),
                })
            }
            DataValue::Uint64Array(array) => {
                proto::v1::datapoint::Value::Uint64Array(proto::v1::Uint64Array {
                    values: array.clone(),
                })
            }
            DataValue::FloatArray(array) => {
                proto::v1::datapoint::Value::FloatArray(proto::v1::FloatArray {
                    values: array.clone(),
                })
            }
            DataValue::DoubleArray(array) => {
                proto::v1::datapoint::Value::DoubleArray(proto::v1::DoubleArray {
                    values: array.clone(),
                })
            }
            DataValue::NotAvailable => proto::v1::datapoint::Value::FailureValue(
                proto::v1::datapoint::Failure::NotAvailable as i32,
            ),
        };

        proto::v1::Datapoint {
            timestamp: Some(Timestamp::from(SystemTime::now())),
            value: Some(value),
        }
    }
}

impl From<&proto::v1::DataType> for DataType {
    fn from(data_type: &proto::v1::DataType) -> Self {
        match data_type {
            proto::v1::DataType::Bool => DataType::Bool,
            proto::v1::DataType::String => DataType::String,
            proto::v1::DataType::Int8 => DataType::Int8,
            proto::v1::DataType::Int16 => DataType::Int16,
            proto::v1::DataType::Int32 => DataType::Int32,
            proto::v1::DataType::Int64 => DataType::Int64,
            proto::v1::DataType::Uint8 => DataType::Uint8,
            proto::v1::DataType::Uint16 => DataType::Uint16,
            proto::v1::DataType::Uint32 => DataType::Uint32,
            proto::v1::DataType::Uint64 => DataType::Uint64,
            proto::v1::DataType::Float => DataType::Float,
            proto::v1::DataType::Double => DataType::Double,
            proto::v1::DataType::Timestamp => DataType::Timestamp,
            proto::v1::DataType::StringArray => DataType::StringArray,
            proto::v1::DataType::BoolArray => DataType::BoolArray,
            proto::v1::DataType::Int8Array => DataType::Int8Array,
            proto::v1::DataType::Int16Array => DataType::Int16Array,
            proto::v1::DataType::Int32Array => DataType::Int32Array,
            proto::v1::DataType::Int64Array => DataType::Int64Array,
            proto::v1::DataType::Uint8Array => DataType::Uint8Array,
            proto::v1::DataType::Uint16Array => DataType::Uint16Array,
            proto::v1::DataType::Uint32Array => DataType::Uint32Array,
            proto::v1::DataType::Uint64Array => DataType::Uint64Array,
            proto::v1::DataType::FloatArray => DataType::FloatArray,
            proto::v1::DataType::DoubleArray => DataType::DoubleArray,
            proto::v1::DataType::TimestampArray => DataType::TimestampArray,
        }
    }
}

impl From<&DataType> for proto::v1::DataType {
    fn from(value_type: &DataType) -> Self {
        match value_type {
            DataType::Bool => proto::v1::DataType::Bool,
            DataType::String => proto::v1::DataType::String,
            DataType::Int8 => proto::v1::DataType::Int8,
            DataType::Int16 => proto::v1::DataType::Int16,
            DataType::Int32 => proto::v1::DataType::Int32,
            DataType::Int64 => proto::v1::DataType::Int64,
            DataType::Uint8 => proto::v1::DataType::Uint8,
            DataType::Uint16 => proto::v1::DataType::Uint16,
            DataType::Uint32 => proto::v1::DataType::Uint32,
            DataType::Uint64 => proto::v1::DataType::Uint64,
            DataType::Float => proto::v1::DataType::Float,
            DataType::Double => proto::v1::DataType::Double,
            DataType::Timestamp => proto::v1::DataType::Timestamp,
            DataType::StringArray => proto::v1::DataType::StringArray,
            DataType::BoolArray => proto::v1::DataType::BoolArray,
            DataType::Int8Array => proto::v1::DataType::Int8Array,
            DataType::Int16Array => proto::v1::DataType::Int16Array,
            DataType::Int32Array => proto::v1::DataType::Int32Array,
            DataType::Int64Array => proto::v1::DataType::Int64Array,
            DataType::Uint8Array => proto::v1::DataType::Uint8Array,
            DataType::Uint16Array => proto::v1::DataType::Uint16Array,
            DataType::Uint32Array => proto::v1::DataType::Uint32Array,
            DataType::Uint64Array => proto::v1::DataType::Uint64Array,
            DataType::FloatArray => proto::v1::DataType::FloatArray,
            DataType::DoubleArray => proto::v1::DataType::DoubleArray,
            DataType::TimestampArray => proto::v1::DataType::TimestampArray,
        }
    }
}

impl From<&proto::v1::ChangeType> for ChangeType {
    fn from(change_type: &proto::v1::ChangeType) -> Self {
        match change_type {
            proto::v1::ChangeType::OnChange => ChangeType::OnChange,
            proto::v1::ChangeType::Continuous => ChangeType::Continuous,
            proto::v1::ChangeType::Static => ChangeType::Static,
        }
    }
}

impl From<&broker::Metadata> for proto::v1::Metadata {
    fn from(metadata: &broker::Metadata) -> Self {
        proto::v1::Metadata {
            id: metadata.id,
            name: metadata.name.to_owned(),
            data_type: proto::v1::DataType::from(&metadata.data_type) as i32,
            change_type: proto::v1::ChangeType::Continuous as i32, // TODO: Add to metadata
            description: metadata.description.to_owned(),
        }
    }
}

impl From<&broker::DatapointError> for proto::v1::DatapointError {
    fn from(error: &broker::DatapointError) -> Self {
        match error {
            broker::DatapointError::NotFound => proto::v1::DatapointError::UnknownDatapoint,
            broker::DatapointError::WrongType | broker::DatapointError::UnsupportedType => {
                proto::v1::DatapointError::InvalidType
            }
            broker::DatapointError::OutOfBounds => proto::v1::DatapointError::OutOfBounds,
        }
    }
}

#[tonic::async_trait]
impl proto::v1::collector_server::Collector for broker::DataBroker {
    async fn update_datapoints(
        &self,
        request: tonic::Request<proto::v1::UpdateDatapointsRequest>,
    ) -> Result<tonic::Response<proto::v1::UpdateDatapointsReply>, tonic::Status> {
        // Collect errors encountered
        let mut errors = HashMap::new();

        let message = request.into_inner();
        let ids = message
            .datapoints
            .iter()
            .map(|(id, datapoint)| (*id, broker::DataPoint::from(datapoint)))
            .collect::<Vec<(i32, broker::DataPoint)>>();

        match self.set_datapoints(&ids).await {
            Ok(()) => {}
            Err(err) => {
                debug!("Failed to set datapoint: {:?}", err);
                errors = err
                    .iter()
                    .map(|(id, error)| (*id, proto::v1::DatapointError::from(error) as i32))
                    .collect();
            }
        }

        Ok(Response::new(proto::v1::UpdateDatapointsReply { errors }))
    }

    type StreamDatapointsStream = ReceiverStream<Result<proto::v1::StreamDatapointsReply, Status>>;

    async fn stream_datapoints(
        &self,
        request: tonic::Request<tonic::Streaming<proto::v1::StreamDatapointsRequest>>,
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
                                        let ids = req.datapoints.iter().map(|(id, datapoint)| {
                                            (*id, broker::DataPoint::from(datapoint))
                                        }).collect::<Vec<(i32, broker::DataPoint)>>();
                                        // TODO: Check if sender is allowed to provide datapoint with this id
                                        match databroker
                                            .set_datapoints(&ids)
                                            .await
                                        {
                                            Ok(_) => {}
                                            Err(err) => {
                                                if let Err(err) = error_sender.send(
                                                    Ok(proto::v1::StreamDatapointsReply {
                                                        errors: err.iter().map(|(id, error)| {
                                                            (*id, proto::v1::DatapointError::from(error) as i32)
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
        request: tonic::Request<proto::v1::RegisterDatapointsRequest>,
    ) -> Result<tonic::Response<proto::v1::RegisterDatapointsReply>, Status> {
        let mut results = HashMap::new();
        let mut error = None;

        for metadata in request.into_inner().list {
            match (
                proto::v1::DataType::from_i32(metadata.data_type),
                proto::v1::ChangeType::from_i32(metadata.change_type),
            ) {
                (Some(value_type), Some(change_type)) => {
                    match self
                        .register_datapoint(
                            metadata.name.clone(),
                            DataType::from(&value_type),
                            ChangeType::from(&change_type),
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
            None => Ok(Response::new(proto::v1::RegisterDatapointsReply {
                results,
            })),
        }
    }
}

fn convert_datapoint_stream_to_subscribe_stream(
    input: impl Stream<Item = broker::QueryResponse>,
) -> impl Stream<Item = Result<proto::v1::SubscribeReply, Status>> {
    input.map(move |item| {
        // debug!("item.id: {:?}", item.value);
        let mut datapoints = HashMap::new();
        for field in item.fields {
            datapoints.insert(field.name, proto::v1::Datapoint::from(&field.value));
        }
        let notification = proto::v1::SubscribeReply { fields: datapoints };
        Ok(notification)
    })
}

#[tonic::async_trait]
impl proto::v1::broker_server::Broker for broker::DataBroker {
    async fn get_datapoints(
        &self,
        request: Request<proto::v1::GetDatapointsRequest>,
    ) -> Result<Response<proto::v1::GetDatapointsReply>, Status> {
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
                match self.get_datapoint_by_name(&name).await {
                    Some(datapoint) => {
                        datapoints.insert(name, proto::v1::Datapoint::from(&datapoint));
                    }
                    None => {
                        // Datapoint doesn't exist
                        datapoints.insert(
                            name,
                            proto::v1::Datapoint {
                                timestamp: None,
                                value: Some(proto::v1::datapoint::Value::FailureValue(
                                    proto::v1::datapoint::Failure::UnknownDatapoint as i32,
                                )),
                            },
                        );
                    }
                }
            }

            let reply = proto::v1::GetDatapointsReply { datapoints };

            Ok(Response::new(reply))
        }
    }

    type SubscribeStream = Pin<
        Box<dyn Stream<Item = Result<proto::v1::SubscribeReply, Status>> + Send + Sync + 'static>,
    >;

    async fn subscribe(
        &self,
        request: tonic::Request<proto::v1::SubscribeRequest>,
    ) -> Result<tonic::Response<Self::SubscribeStream>, tonic::Status> {
        let query = request.into_inner().query;
        match self.subscribe(&query).await {
            Ok(stream) => {
                let stream = convert_datapoint_stream_to_subscribe_stream(stream);
                debug!("Subscribed to new query");
                Ok(Response::new(Box::pin(stream)))
            }
            Err(e) => Err(Status::new(Code::InvalidArgument, format!("{:?}", e))),
        }
    }

    async fn get_metadata(
        &self,
        request: tonic::Request<proto::v1::GetMetadataRequest>,
    ) -> Result<tonic::Response<proto::v1::GetMetadataReply>, tonic::Status> {
        let request = request.into_inner();

        let list = if request.names.is_empty() {
            self.entries
                .read()
                .await
                .iter()
                .map(|entry| proto::v1::Metadata::from(&entry.metadata))
                .collect()
        } else {
            let mut list = Vec::new();

            let entries = self.entries.read().await;
            for name in request.names {
                if let Some(entry) = entries.get_by_name(&name) {
                    list.push(proto::v1::Metadata::from(&entry.metadata));
                }
            }
            list
        };
        let reply = proto::v1::GetMetadataReply { list };
        Ok(Response::new(reply))
    }
}

async fn shutdown<F>(databroker: broker::DataBroker, signal: F)
where
    F: Future<Output = ()>,
{
    // Wait for signal
    signal.await;

    info!("Shutting down");
    databroker.shutdown().await;
}

pub async fn serve_with_shutdown<F>(
    addr: &str,
    broker: broker::DataBroker,
    signal: F,
) -> Result<(), Box<dyn std::error::Error>>
where
    F: Future<Output = ()>,
{
    let addr = addr.parse()?;

    broker.start_housekeeping_task();

    Server::builder()
        .http2_keepalive_interval(Some(Duration::from_secs(10)))
        .http2_keepalive_timeout(Some(Duration::from_secs(20)))
        .add_service(proto::v1::broker_server::BrokerServer::new(broker.clone()))
        .add_service(proto::v1::collector_server::CollectorServer::new(
            broker.clone(),
        ))
        .serve_with_shutdown(addr, shutdown(broker, signal))
        .await?;

    Ok(())
}
