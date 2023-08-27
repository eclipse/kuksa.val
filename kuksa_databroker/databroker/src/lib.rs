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

pub mod authorization;
pub mod broker;
pub mod glob;
pub mod grpc;
pub mod permissions;
pub mod query;
pub mod types;
pub mod vss;

#[cfg(feature = "viss")]
pub mod viss;

use std::fmt::Write;

use tracing::info;
use tracing_subscriber::filter::EnvFilter;

pub fn init_logging() {
    let mut output = String::from("Init logging from RUST_LOG");
    let filter = EnvFilter::try_from_default_env().unwrap_or_else(|err| {
        output.write_fmt(format_args!(" ({err})")).unwrap();
        // If no environment variable set, this is the default
        EnvFilter::new("info")
    });
    tracing_subscriber::fmt::Subscriber::builder()
        .with_env_filter(filter)
        .try_init()
        .expect("Unable to install global logging subscriber");

    info!("{}", output);
}
