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

use std::fmt::Write;
use std::io::Read;

use tracing::{debug, info};
use tracing_subscriber::filter::EnvFilter;

use tokio::select;
use tokio::signal::unix::{signal, SignalKind};

use clap::{Arg, Command};

use databroker::{broker, grpc, vss};

// Hardcoded datapoints
const DATAPOINTS: &[(
    &str,
    broker::DataType,
    broker::ChangeType,
    broker::EntryType,
    &str,
)] = &[
    (
        "Vehicle.Cabin.Seat.Row1.Pos1.Position",
        broker::DataType::Uint32,
        broker::ChangeType::OnChange,
        broker::EntryType::Actuator,
        "Vehicle cabin seat position. Row 1 Position 1",
    ),
    (
        "Vehicle.Cabin.Seat.Row1.Pos2.Position",
        broker::DataType::Uint32,
        broker::ChangeType::OnChange,
        broker::EntryType::Actuator,
        "Vehicle cabin seat position. Row 1 Position 2",
    ),
    (
        "Vehicle.Speed",
        broker::DataType::Float,
        broker::ChangeType::Continuous,
        broker::EntryType::Sensor,
        "Vehicle speed",
    ),
    (
        "Vehicle.ADAS.ABS.Error",
        broker::DataType::String,
        broker::ChangeType::OnChange,
        broker::EntryType::Attribute,
        "ADAS ABS error message",
    ),
    (
        "Vehicle.ADAS.ABS.IsActive",
        broker::DataType::Bool,
        broker::ChangeType::OnChange,
        broker::EntryType::Actuator,
        "Whether ADAS ABS is active",
    ),
    (
        "Vehicle.ADAS.ABS.IsEngaged",
        broker::DataType::Bool,
        broker::ChangeType::OnChange,
        broker::EntryType::Actuator,
        "Whether ADAS ABS is engaged",
    ),
    (
        "Vehicle.ADAS.CruiseControl.Error",
        broker::DataType::String,
        broker::ChangeType::OnChange,
        broker::EntryType::Attribute,
        "ADAS Cruise control error message",
    ),
    (
        "Vehicle.ADAS.CruiseControl.IsActive",
        broker::DataType::Bool,
        broker::ChangeType::OnChange,
        broker::EntryType::Actuator,
        "Whether ADAS Cruise control is active",
    ),
    (
        "Vehicle.ADAS.CruiseControl.SpeedSet",
        broker::DataType::Bool,
        broker::ChangeType::OnChange,
        broker::EntryType::Actuator,
        "Whether ADAS Cruise control has the speed set",
    ),
    (
        "Vehicle.TestArray",
        broker::DataType::StringArray,
        broker::ChangeType::OnChange,
        broker::EntryType::Sensor,
        "Run of the mill test array",
    ),
];

fn init_logging() {
    let mut output = String::from("Init logging from RUST_LOG");
    let filter = EnvFilter::try_from_default_env().unwrap_or_else(|err| {
        output.write_fmt(format_args!(" ({})", err)).unwrap();
        // If no environment variable set, this is the default
        EnvFilter::new("info")
    });
    tracing_subscriber::fmt::Subscriber::builder()
        .with_env_filter(filter)
        .try_init()
        .expect("Unable to install global logging subscriber");

    info!("{}", output);
}

async fn shutdown_handler() {
    let mut sigint =
        signal(SignalKind::interrupt()).expect("failed to setup SIGINT signal handler");
    let mut sighup = signal(SignalKind::hangup()).expect("failed to setup SIGHUP signal handler");

    select! {
        _ = sigint.recv() => info!("received SIGINT"),
        _ = sighup.recv() => info!("received SIGHUP"),
    };
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let version = option_env!("VERGEN_GIT_SEMVER").unwrap_or("");

    let about = format!(
        concat!(
            "{}\n",
            "\n  Commit Date:      {}",
            "\n  Commit SHA:       {}",
            "\n  Commit Branch:    {}",
            "\n",
            "\n  Package version:  {}",
            "\n  Cargo Profile:    {}"
        ),
        option_env!("CARGO_PKG_DESCRIPTION").unwrap_or(""),
        option_env!("VERGEN_GIT_COMMIT_TIMESTAMP").unwrap_or(""),
        option_env!("VERGEN_GIT_SHA").unwrap_or(""),
        option_env!("VERGEN_GIT_BRANCH").unwrap_or(""),
        option_env!("CARGO_PKG_VERSION").unwrap_or(""),
        option_env!("VERGEN_CARGO_PROFILE").unwrap_or(""),
    );

    let parser = Command::new("Kuksa Data Broker")
        .version(version)
        .about(&*about)
        .arg(
            Arg::new("address")
                .display_order(1)
                .long("address")
                .alias("addr")
                .help("Bind address")
                .takes_value(true)
                .value_name("ADDR")
                .required(false)
                .env("KUKSA_DATA_BROKER_ADDR")
                .default_value("127.0.0.1"),
        )
        .arg(
            Arg::new("port")
                .display_order(2)
                .long("port")
                .help("Bind port")
                .takes_value(true)
                .value_name("PORT")
                .required(false)
                .env("KUKSA_DATA_BROKER_PORT")
                .default_value("55555"),
        )
        .arg(
            Arg::new("metadata")
                .display_order(4)
                .long("metadata")
                .help("Populate data broker with metadata from file")
                .takes_value(true)
                .value_name("FILE")
                .env("KUKSA_DATA_BROKER_METADATA_FILE")
                .required(false),
        )
        .arg(
            Arg::new("dummy-metadata")
                .display_order(10)
                .long("dummy-metadata")
                .takes_value(false)
                .help("Populate data broker with dummy metadata")
                .required(false),
        );
    let args = parser.get_matches();

    // install global collector configured based on RUST_LOG env var.
    init_logging();

    info!("Starting Kuksa Data Broker {}", version);
    let addr = format!(
        "{}:{}",
        args.value_of("address").unwrap(),
        args.value_of("port").unwrap()
    );

    let broker = broker::DataBroker::new();

    if args.is_present("dummy-metadata") {
        info!("Populating (hardcoded) metadata");
        for (name, data_type, change_type, entry_type, description) in DATAPOINTS {
            if let Ok(id) = broker
                .add_entry(
                    name.to_string(),
                    data_type.clone(),
                    change_type.clone(),
                    entry_type.clone(),
                    description.to_string(),
                )
                .await
            {
                if name == &"Vehicle.TestArray" {
                    match broker
                        .update_entries([(
                            id,
                            broker::EntryUpdate {
                                path: None,
                                datapoint: Some(broker::Datapoint {
                                    ts: std::time::SystemTime::now(),
                                    value: databroker::broker::DataValue::StringArray(vec![
                                        String::from("yes"),
                                        String::from("no"),
                                        String::from("maybe"),
                                        String::from("nah"),
                                    ]),
                                }),
                                actuator_target: None,
                                entry_type: None,
                                data_type: None,
                                description: None,
                            },
                        )])
                        .await
                    {
                        Ok(_) => {}
                        Err(e) => println!("{:?}", e),
                    }
                }
            }
        }
    }

    if let Some(filename) = args.value_of("metadata") {
        info!("Populating metadata... from file '{}'", filename);
        let mut metadata_file = std::fs::OpenOptions::new().read(true).open(filename)?;
        // metadata_file
        let mut data = String::new();
        metadata_file.read_to_string(&mut data)?;
        let entries = vss::parse_vss(&data)?;
        for entry in entries {
            debug!("Adding {}", entry.name);

            let id = broker
                .add_entry(
                    entry.name.clone(),
                    entry.data_type,
                    databroker::broker::ChangeType::OnChange,
                    entry.entry_type,
                    entry.description.unwrap_or_else(|| "".to_owned()),
                )
                .await;

            if let (Ok(id), Some(default)) = (id, entry.default) {
                let ids = [(
                    id,
                    broker::EntryUpdate {
                        datapoint: Some(broker::Datapoint {
                            ts: std::time::SystemTime::now(),
                            value: default,
                        }),
                        path: None,
                        actuator_target: None,
                        entry_type: None,
                        data_type: None,
                        description: None,
                    },
                )];
                if let Err(errors) = broker.update_entries(ids).await {
                    // There's only one error (since we're only trying to set one)
                    if let Some(error) = errors.get(0) {
                        info!(
                            "Failed to set default value for {}: {:?}",
                            entry.name, error.1
                        );
                    }
                }
            }
        }
    }

    info!("Listening on {}", addr);

    grpc::server::serve_with_shutdown(&addr, broker, shutdown_handler()).await?;

    Ok(())
}
