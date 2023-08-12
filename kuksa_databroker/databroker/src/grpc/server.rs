/********************************************************************************
* Copyright (c) 2022, 2023 Contributors to the Eclipse Foundation
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

use std::{convert::TryFrom, future::Future, time::Duration};

use tokio_stream::wrappers::TcpListenerStream;
use tonic::transport::Server;
#[cfg(feature = "tls")]
use tonic::transport::ServerTlsConfig;
use tracing::{debug, info, warn};

use databroker_proto::{kuksa, sdv};

use crate::{
    broker, jwt,
    permissions::{self, Permissions},
};

#[derive(Clone)]
#[allow(clippy::large_enum_variant)]
pub enum Authorization {
    Disabled,
    Enabled { token_decoder: jwt::Decoder },
}

#[cfg(feature = "tls")]
pub enum ServerTLS {
    Disabled,
    Enabled { tls_config: ServerTlsConfig },
}

impl tonic::service::Interceptor for Authorization {
    fn call(
        &mut self,
        mut request: tonic::Request<()>,
    ) -> Result<tonic::Request<()>, tonic::Status> {
        match self {
            Authorization::Disabled => {
                request
                    .extensions_mut()
                    .insert(permissions::ALLOW_ALL.clone());
                Ok(request)
            }
            Authorization::Enabled { token_decoder } => {
                match request.metadata().get("authorization") {
                    Some(header) => match header.to_str() {
                        Ok(header) if header.starts_with("Bearer ") => {
                            let token: &str = header[7..].into();
                            match token_decoder.decode(token) {
                                Ok(claims) => match Permissions::try_from(claims) {
                                    Ok(permissions) => {
                                        request.extensions_mut().insert(permissions);
                                        Ok(request)
                                    }
                                    Err(err) => Err(tonic::Status::unauthenticated(format!(
                                        "Invalid auth token: {err}"
                                    ))),
                                },
                                Err(err) => Err(tonic::Status::unauthenticated(format!(
                                    "Invalid auth token: {err}"
                                ))),
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

pub async fn serve<F>(
    addr: impl Into<std::net::SocketAddr>,
    broker: broker::DataBroker,
    #[cfg(feature = "tls")] server_tls: ServerTLS,
    authorization: Authorization,
    signal: F,
) -> Result<(), Box<dyn std::error::Error>>
where
    F: Future<Output = ()>,
{
    let socket_addr = addr.into();

    broker.start_housekeeping_task();

    let mut builder = tonic::transport::Server::builder()
        .http2_keepalive_interval(Some(Duration::from_secs(10)))
        .http2_keepalive_timeout(Some(Duration::from_secs(20)));

    #[cfg(feature = "tls")]
    match server_tls {
        ServerTLS::Enabled { tls_config } => {
            info!("Using TLS");
            builder = builder.tls_config(tls_config)?;
        }
        ServerTLS::Disabled => {
            warn!("TLS is not enabled")
        }
    }

    if let Authorization::Disabled = &authorization {
        warn!("Authorization is not enabled");
    }

    let router = builder
        .add_service(
            sdv::databroker::v1::broker_server::BrokerServer::with_interceptor(
                broker.clone(),
                authorization.clone(),
            ),
        )
        .add_service(
            sdv::databroker::v1::collector_server::CollectorServer::with_interceptor(
                broker.clone(),
                authorization.clone(),
            ),
        )
        .add_service(kuksa::val::v1::val_server::ValServer::with_interceptor(
            broker.clone(),
            authorization.clone(),
        ));

    info!("Listening on {}", socket_addr);
    router
        .serve_with_shutdown(socket_addr, shutdown(broker, signal))
        .await?;

    Ok(())
}

pub async fn serve_with_incoming_shutdown<F>(
    stream: TcpListenerStream,
    broker: broker::DataBroker,
    authorization: Authorization,
    signal: F,
) -> Result<(), Box<dyn std::error::Error>>
where
    F: Future<Output = ()>,
{
    broker.start_housekeeping_task();

    Server::builder()
        .http2_keepalive_interval(Some(Duration::from_secs(10)))
        .http2_keepalive_timeout(Some(Duration::from_secs(20)))
        .add_service(
            sdv::databroker::v1::broker_server::BrokerServer::with_interceptor(
                broker.clone(),
                authorization.clone(),
            ),
        )
        .add_service(
            sdv::databroker::v1::collector_server::CollectorServer::with_interceptor(
                broker.clone(),
                authorization.clone(),
            ),
        )
        .add_service(kuksa::val::v1::val_server::ValServer::with_interceptor(
            broker.clone(),
            authorization,
        ))
        .serve_with_incoming_shutdown(stream, shutdown(broker, signal))
        .await?;

    Ok(())
}
