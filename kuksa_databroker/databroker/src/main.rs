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

use databroker::authorization::Authorization;
use databroker::broker::RegistrationError;

#[cfg(feature = "tls")]
use databroker::grpc::server::ServerTLS;

use tokio::select;
use tokio::signal::unix::{signal, SignalKind};
#[cfg(feature = "tls")]
use tracing::warn;
use tracing::{debug, error, info};

use clap::{Arg, ArgAction, Command};

#[cfg(feature = "viss")]
use databroker::viss;
use databroker::{broker, grpc, permissions, vss};

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

async fn add_kuksa_attribute(
    database: &broker::AuthorizedAccess<'_, '_>,
    attribute: String,
    value: String,
    description: String,
) {
    debug!("Adding attribute {}", attribute);

    match database
        .add_entry(
            attribute.clone(),
            databroker::broker::DataType::String,
            databroker::broker::ChangeType::OnChange,
            databroker::broker::EntryType::Attribute,
            description,
            None,
        )
        .await
    {
        Ok(id) => {
            let ids = [(
                id,
                broker::EntryUpdate {
                    datapoint: Some(broker::Datapoint {
                        ts: std::time::SystemTime::now(),
                        value: broker::types::DataValue::String(value),
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
                    info!("Failed to set value for {}: {:?}", attribute, error.1);
                }
            }
        }
        Err(RegistrationError::PermissionDenied) => {
            error!("Failed to add entry {attribute}: Permission denied")
        }
        Err(RegistrationError::PermissionExpired) => {
            error!("Failed to add entry {attribute}: Permission expired")
        }
        Err(RegistrationError::ValidationError) => {
            error!("Failed to add entry {attribute}: Validation failed")
        }
    }
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
            "\n  Commit Date:      {}",
            "\n  Commit SHA:       {}",
            "\n  Commit Branch:    {}",
            "\n",
            "\n  Package version:  {}",
            "\n  Debug build:      {}"
        ),
        option_env!("VERGEN_GIT_COMMIT_TIMESTAMP").unwrap_or(""),
        option_env!("VERGEN_GIT_SHA").unwrap_or(""),
        option_env!("VERGEN_GIT_BRANCH").unwrap_or(""),
        option_env!("CARGO_PKG_VERSION").unwrap_or(""),
        option_env!("VERGEN_CARGO_DEBUG").unwrap_or(""),
    );

    let mut parser = Command::new("Kuksa Data Broker");
    parser = parser
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
            Arg::new("disable-authorization")
                .display_order(6)
                .long("disable-authorization")
                .help("Disable authorization")
                .action(ArgAction::SetTrue),
        );

    #[cfg(feature = "tls")]
    {
        parser = parser
            .arg(
                Arg::new("insecure")
                    .display_order(20)
                    .long("insecure")
                    .help("Allow insecure connections")
                    .action(ArgAction::SetTrue),
            )
            .arg(
                Arg::new("tls-cert")
                    .display_order(21)
                    .long("tls-cert")
                    .help("TLS certificate file (.pem)")
                    .action(ArgAction::Set)
                    .value_name("FILE")
                    .conflicts_with("insecure"),
            )
            .arg(
                Arg::new("tls-private-key")
                    .display_order(22)
                    .long("tls-private-key")
                    .help("TLS private key file (.key)")
                    .action(ArgAction::Set)
                    .value_name("FILE")
                    .conflicts_with("insecure"),
            );
    }

    #[cfg(feature = "viss")]
    {
        parser = parser
            .arg(
                Arg::new("enable-viss")
                    .display_order(30)
                    .long("enable-viss")
                    .help("Enable VISSv2 (websocket) service")
                    .action(ArgAction::SetTrue),
            )
            .arg(
                Arg::new("viss-address")
                    .display_order(31)
                    .long("viss-address")
                    .help("VISS address")
                    .action(ArgAction::Set)
                    .value_name("IP")
                    .required(false)
                    .env("KUKSA_DATABROKER_VISS_ADDR")
                    .default_value("127.0.0.1"),
            )
            .arg(
                Arg::new("viss-port")
                    .display_order(32)
                    .long("viss-port")
                    .help("VISS port")
                    .action(ArgAction::Set)
                    .value_name("PORT")
                    .required(false)
                    .env("KUKSA_DATABROKER_VISS_PORT")
                    .value_parser(clap::value_parser!(u16))
                    .default_value("8090"),
            );
    }

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

    add_kuksa_attribute(
        &database,
        "Kuksa.Databroker.GitVersion".to_owned(),
        option_env!("VERGEN_GIT_SEMVER_LIGHTWEIGHT")
            .unwrap_or("N/A")
            .to_owned(),
        "Databroker version as reported by GIT".to_owned(),
    )
    .await;

    add_kuksa_attribute(
        &database,
        "Kuksa.Databroker.CargoVersion".to_owned(),
        option_env!("CARGO_PKG_VERSION").unwrap_or("N/A").to_owned(),
        "Databroker version as reported by GIT".to_owned(),
    )
    .await;

    add_kuksa_attribute(
        &database,
        "Kuksa.Databroker.CommitSha".to_owned(),
        option_env!("VERGEN_GIT_SHA").unwrap_or("N/A").to_owned(),
        "Commit SHA of current version".to_owned(),
    )
    .await;

    if let Some(metadata_filenames) = args.get_many::<String>("vss-file") {
        for filename in metadata_filenames {
            read_metadata_file(&database, filename).await?;
        }
    }

    #[cfg(feature = "tls")]
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
            (None, None) => {
                warn!(
                    "TLS is not enabled. Default behavior of accepting insecure connections \
                     when TLS is not configured may change in the future! \
                     Please use --insecure to explicitly enable this behavior."
                );
                ServerTLS::Disabled
            }
        }
    };

    let enable_authorization = !args.get_flag("disable-authorization");
    let jwt_public_key = match args.get_one::<String>("jwt-public-key") {
        Some(pub_key_filename) => match std::fs::read_to_string(pub_key_filename) {
            Ok(pub_key) => {
                info!("Using '{pub_key_filename}' to authenticate access tokens");
                Ok(Some(pub_key))
            }
            Err(err) => {
                error!("Failed to open file {:?}: {}", pub_key_filename, err);
                Err(err)
            }
        },
        None => Ok(None),
    }?;

    let authorization = match (enable_authorization, jwt_public_key) {
        (true, Some(pub_key)) => Authorization::new(pub_key)?,
        (true, None) => {
            warn!("Authorization is not enabled.");
            Authorization::Disabled
        }
        (false, _) => Authorization::Disabled,
    };

    #[cfg(feature = "viss")]
    {
        let viss_port = args
            .get_one::<u16>("viss-port")
            .expect("port should be a number");
        let viss_addr = std::net::SocketAddr::new(ip_addr, *viss_port);

        if args.get_flag("enable-viss") {
            let broker = broker.clone();
            let authorization = authorization.clone();
            tokio::spawn(async move {
                if let Err(err) = viss::server::serve(viss_addr, broker, authorization).await {
                    error!("{err}");
                }
            });
        }
    }

    grpc::server::serve(
        addr,
        broker,
        #[cfg(feature = "tls")]
        tls_config,
        authorization,
        shutdown_handler(),
    )
    .await?;

    Ok(())
}
