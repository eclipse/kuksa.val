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

use databroker::broker::RegistrationError;
use databroker::grpc::server::{Authorization, ServerTLS};
use tracing::{debug, error, info};

use tokio::select;
use tokio::signal::unix::{signal, SignalKind};

use clap::{Arg, ArgAction, Command};

use databroker::{broker, grpc, jwt, permissions, vss};

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

async fn read_metadata_file<'a, 'b>(
    database: &broker::AuthorizedAccess<'_, '_>,
    filename: &str,
) -> Result<(), Box<dyn std::error::Error>> {
    let path = filename.trim();
    info!("Populating metadata from file '{}'", path);
    let metadata_file = std::fs::OpenOptions::new().read(true).open(filename)?;
    let entries = vss::parse_vss_from_reader(&metadata_file)?;

    for (path, entry) in entries {
        debug!("Adding VSS datapoint type {}", path);

        match database
            .add_entry(
                path.clone(),
                entry.data_type,
                databroker::broker::ChangeType::OnChange,
                entry.entry_type,
                entry.description,
                entry.allowed,
            )
            .await
        {
            Ok(id) => {
                if let Some(default) = entry.default {
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
                            allowed: None,
                        },
                    )];
                    if let Err(errors) = database.update_entries(ids).await {
                        // There's only one error (since we're only trying to set one)
                        if let Some(error) = errors.get(0) {
                            info!("Failed to set default value for {}: {:?}", path, error.1);
                        }
                    }
                }
            }
            Err(RegistrationError::PermissionDenied) => {
                error!("Failed to add entry {path}: Permission denied")
            }
            Err(RegistrationError::PermissionExpired) => {
                error!("Failed to add entry {path}: Permission expired")
            }
            Err(RegistrationError::ValidationError) => {
                error!("Failed to add entry {path}: Validation failed")
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
        .about(about)
        .arg(
            Arg::new("address")
                .display_order(1)
                .long("address")
                .alias("addr")
                .help("Bind address")
                .action(ArgAction::Set)
                .value_name("IP")
                .required(false)
                .env("KUKSA_DATA_BROKER_ADDR")
                .default_value("127.0.0.1"),
        )
        .arg(
            Arg::new("port")
                .display_order(2)
                .long("port")
                .help("Bind port")
                .action(ArgAction::Set)
                .value_name("PORT")
                .required(false)
                .env("KUKSA_DATA_BROKER_PORT")
                .value_parser(clap::value_parser!(u16))
                .default_value("55555"),
        )
        .arg(
            Arg::new("vss-file")
                .display_order(4)
                .alias("metadata")
                .long("vss")
                .help("Populate data broker with VSS metadata from (comma-separated) list of files")
                .action(ArgAction::Set)
                .value_delimiter(',')
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
                .action(ArgAction::Set)
                .value_name("FILE")
                .required(false),
        )
        .arg(
            Arg::new("insecure")
                .display_order(6)
                .long("insecure")
                .help("Allow insecure connections")
                .action(ArgAction::SetTrue),
        )
        .arg(
            Arg::new("tls-cert")
                .display_order(5)
                .long("tls-cert")
                .help("TLS certificate file (.pem)")
                .action(ArgAction::Set)
                .value_name("FILE")
                .conflicts_with("insecure"),
        )
        .arg(
            Arg::new("tls-private-key")
                .display_order(5)
                .long("tls-private-key")
                .help("TLS private key file (.pem)")
                .action(ArgAction::Set)
                .value_name("FILE")
                .conflicts_with("insecure"),
        )
        .arg(
            Arg::new("dummy-metadata")
                .display_order(10)
                .long("dummy-metadata")
                .action(ArgAction::Set)
                .help("Populate data broker with dummy metadata")
                .required(false),
        );
    let args = parser.get_matches();

    // install global collector configured based on RUST_LOG env var.
    databroker::init_logging();

    info!("Starting Kuksa Databroker {}", version);

    let ip_addr = args.get_one::<String>("address").unwrap().parse()?;
    let port = args
        .get_one::<u16>("port")
        .expect("port should be a number");
    let addr = std::net::SocketAddr::new(ip_addr, *port);

    let broker = broker::DataBroker::new(version);
    let database = broker.authorized_access(&permissions::ALLOW_ALL);

    if args.contains_id("dummy-metadata") {
        info!("Populating (hardcoded) metadata");
        for (name, data_type, change_type, entry_type, description) in DATAPOINTS {
            if let Ok(id) = database
                .add_entry(
                    name.to_string(),
                    data_type.clone(),
                    change_type.clone(),
                    entry_type.clone(),
                    description.to_string(),
                    None,
                )
                .await
            {
                if name == &"Vehicle.TestArray" {
                    match database
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
                                allowed: None,
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

    if let Some(metadata_filenames) = args.get_many::<String>("vss-file") {
        for filename in metadata_filenames {
            read_metadata_file(&database, filename).await?;
        }
    }

    let tls_config = if args.get_flag("insecure") {
        ServerTLS::Disabled
    } else {
        let cert_file = args.get_one::<String>("tls-cert");
        let key_file = args.get_one::<String>("tls-private-key");
        match (cert_file, key_file) {
            (Some(cert_file), Some(key_file)) => {
                let cert = std::fs::read(cert_file)?;
                let key = std::fs::read(key_file)?;
                let identity = tonic::transport::Identity::from_pem(cert, key);
                ServerTLS::Enabled {
                    tls_config: tonic::transport::ServerTlsConfig::new().identity(identity),
                }
            }
            (Some(_), None) => {
                return Err(
                    "TLS private key (--tls-private-key) must be set if --tls-cert is.".into(),
                );
            }
            (None, Some(_)) => {
                return Err(
                    "TLS certificate (--tls-cert) must be set if --tls-private-key is.".into(),
                );
            }
            (None, None) => ServerTLS::Disabled,
        }
    };

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

    let authorization = match jwt_public_key {
        Some(pub_key) => {
            let token_decoder = jwt::Decoder::new(pub_key)?;
            Authorization::Enabled { token_decoder }
        }
        None => Authorization::Disabled,
    };

    grpc::server::serve(addr, broker, tls_config, authorization, shutdown_handler()).await?;

    Ok(())
}
