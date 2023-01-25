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

use std::{future::Future, time::Duration};

use tonic::transport::Server;
use tracing::{debug, info, warn};

use databroker_proto::{kuksa, sdv};

use crate::{
    auth::{self, token_decoder::TokenDecoder},
    broker,
};

#[derive(Clone)]
struct AuthorizationInterceptor {
    token_decoder: auth::token_decoder::TokenDecoder,
}

impl tonic::service::Interceptor for AuthorizationInterceptor {
    fn call(
        &mut self,
        mut request: tonic::Request<()>,
    ) -> Result<tonic::Request<()>, tonic::Status> {
        match request.metadata().get("authorization") {
            Some(header) => match header.to_str() {
                Ok(header) if header.starts_with("Bearer ") => {
                    let token: &str = header[7..].into();
                    match self.token_decoder.decode(token) {
                        Ok(claims) => {
                            request.extensions_mut().insert(claims);
                            Ok(request)
                        }
                        Err(_) => Err(tonic::Status::unauthenticated("Invalid auth token")),
                    }
                }
                Ok(_) | Err(_) => Err(tonic::Status::unauthenticated("Invalid auth token")),
            },
            None => {
                debug!("No auth token provided");
                Err(tonic::Status::unauthenticated("No auth token provided"))
            }
        }
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

pub async fn serve_authorized<F>(
    addr: &str,
    broker: broker::DataBroker,
    token_decoder: TokenDecoder,
    signal: F,
) -> Result<(), Box<dyn std::error::Error>>
where
    F: Future<Output = ()>,
{
    let addr = addr.parse()?;

    let authorization_interceptor = AuthorizationInterceptor { token_decoder };
    broker.start_housekeeping_task();

    info!("Listening on {}", addr);

    Server::builder()
        .http2_keepalive_interval(Some(Duration::from_secs(10)))
        .http2_keepalive_timeout(Some(Duration::from_secs(20)))
        .add_service(
            sdv::databroker::v1::broker_server::BrokerServer::with_interceptor(
                broker.clone(),
                authorization_interceptor.clone(),
            ),
        )
        .add_service(
            sdv::databroker::v1::collector_server::CollectorServer::with_interceptor(
                broker.clone(),
                authorization_interceptor.clone(),
            ),
        )
        .add_service(kuksa::val::v1::val_server::ValServer::with_interceptor(
            broker.clone(),
            authorization_interceptor.clone(),
        ))
        .serve_with_shutdown(addr, shutdown(broker, signal))
        .await?;

    Ok(())
}

pub async fn serve<F>(
    addr: &str,
    broker: broker::DataBroker,
    signal: F,
) -> Result<(), Box<dyn std::error::Error>>
where
    F: Future<Output = ()>,
{
    let addr = addr.parse()?;

    broker.start_housekeeping_task();

    warn!("Authorization is not enabled");
    info!("Listening on {}", addr);

    Server::builder()
        .http2_keepalive_interval(Some(Duration::from_secs(10)))
        .http2_keepalive_timeout(Some(Duration::from_secs(20)))
        .add_service(sdv::databroker::v1::broker_server::BrokerServer::new(
            broker.clone(),
        ))
        .add_service(sdv::databroker::v1::collector_server::CollectorServer::new(
            broker.clone(),
        ))
        .add_service(kuksa::val::v1::val_server::ValServer::new(broker.clone()))
        .serve_with_shutdown(addr, shutdown(broker, signal))
        .await?;

    Ok(())
}
