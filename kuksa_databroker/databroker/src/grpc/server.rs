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
use tracing::info;

use databroker_proto::{kuksa, sdv};

use crate::broker;

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
