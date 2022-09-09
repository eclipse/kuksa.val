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

use crate::broker;

use tracing::debug;

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
