/********************************************************************************
* Copyright (c) 2022-2023 Contributors to the Eclipse Foundation
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

#[cfg(feature = "jemalloc")]
#[global_allocator]
static ALLOC: jemallocator::Jemalloc = jemallocator::Jemalloc;

use std::fmt::Write;

use tracing::{debug, error, info};
use tracing_subscriber::filter::EnvFilter;

use tokio::select;
use tokio::signal::unix::{signal, SignalKind};

use clap::{Arg, Command};

use databroker::{auth, broker, grpc, vss};

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

async fn shutdown_handler() {
    let mut sigint =
        signal(SignalKind::interrupt()).expect("failed to setup SIGINT signal handler");
    let mut sighup = signal(SignalKind::hangup()).expect("failed to setup SIGHUP signal handler");
    let mut sigterm =
        signal(SignalKind::terminate()).expect("failed to setup SIGTERM signal handler");

    select! {
        _ = sigint.recv() => info!("received SIGINT"),
        _ = sighup.recv() => info!("received SIGHUP"),
        _ = sigterm.recv() => info!("received SIGTERM"),
    };
}

async fn read_metadata_file(
    broker: &broker::DataBroker,
    filename: &str,
) -> Result<(), Box<dyn std::error::Error>> {
    let path = filename.trim();
    info!("Populating metadata from file '{}'", path);
    let metadata_file = std::fs::OpenOptions::new().read(true).open(filename)?;
    let entries = vss::parse_vss_from_reader(&metadata_file)?;
    for (path, entry) in entries {
        debug!("Adding VSS datapoint type {}", path);

        let id = broker
            .add_entry(
                path.clone(),
                entry.data_type,
                databroker::broker::ChangeType::OnChange,
                entry.entry_type,
                entry.description,
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
                    info!("Failed to set default value for {}: {:?}", path, error.1);
                }
            }
        }
    }
    Ok(())
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let version = option_env!("VERGEN_GIT_SEMVER_LIGHTWEIGHT")
        .unwrap_or(option_env!("VERGEN_GIT_SHA").unwrap_or("unknown"));

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
                .value_parser(clap::value_parser!(u16).range(1..))
                .default_value("55555"),
        )
        .arg(
            Arg::new("metadata")
                .display_order(4)
                .long("metadata")
                .help("Populate data broker with metadata from (comma-separated) list of files")
                .takes_value(true)
                .use_value_delimiter(true)
                .require_value_delimiter(true)
                .multiple_values(true)
                .value_name("FILE")
                .env("KUKSA_DATA_BROKER_METADATA_FILE")
                .value_parser(clap::builder::NonEmptyStringValueParser::new())
                .required(false),
        )
        .arg(
            Arg::new("jwt-public-key")
                .display_order(5)
                .long("jwt-public-key")
                .help("Public key used to verify JWT access tokens")
                .takes_value(true)
                .value_name("FILE")
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
        args.get_one::<u16>("port").unwrap()
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
                        Err(e) => println!("{e:?}"),
                    }
                }
            }
        }
    }

    if let Some(metadata_filenames) = args.get_many::<String>("metadata") {
        for filename in metadata_filenames {
            read_metadata_file(&broker, filename).await?;
        }
    }

    let jwt_public_key = match args.get_one::<String>("jwt-public-key") {
        Some(pub_key_filename) => match std::fs::read_to_string(pub_key_filename) {
            Ok(pub_key) => {
                info!("Using '{pub_key_filename}' to authenticate access tokens");
                Some(pub_key)
            }
            Err(err) => {
                error!("Failed to open file {:?}: {}", pub_key_filename, err);
                None
            }
        },
        None => None,
    };

    match jwt_public_key {
        Some(pub_key) => {
            let token_decoder = auth::token_decoder::TokenDecoder::new(pub_key)?;
            grpc::server::serve_authorized(&addr, broker, token_decoder, shutdown_handler())
                .await?;
        }
        None => {
            grpc::server::serve(&addr, broker, shutdown_handler()).await?;
        }
    }

    Ok(())
}
