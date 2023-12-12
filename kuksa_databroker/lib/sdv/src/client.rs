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

use databroker_proto::sdv::databroker as proto;
use http::Uri;
use sdk_common::{Client, ClientError};

pub struct SDVClient {
    pub basic_client: Client,
}

impl SDVClient {
    pub fn new(uri: Uri) -> Self {
        SDVClient {
            basic_client: Client::new(uri),
        }
    }

    pub async fn get_metadata(
        &mut self,
        paths: Vec<String>,
    ) -> Result<Vec<proto::v1::Metadata>, ClientError> {
        let mut client = proto::v1::broker_client::BrokerClient::with_interceptor(
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
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
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
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
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
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
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
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
            self.basic_client.get_channel().await?.clone(),
            self.basic_client.get_auth_interceptor(),
        );

        let request = tonic::Request::new(proto::v1::UpdateDatapointsRequest { datapoints });
        match client.update_datapoints(request).await {
            Ok(response) => Ok(response.into_inner()),
            Err(err) => Err(ClientError::Status(err)),
        }
    }
}
