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

use http::uri::Uri;
use prost_types::Timestamp;
use tokio_stream::StreamExt;

use std::collections::HashMap;
use std::io::Write;
use std::sync::Arc;
use std::time::{Duration, SystemTime};
use std::{fmt, io};

use ansi_term::Color;

use clap::{Parser, Subcommand};

use linefeed::complete::{Completer, Completion, Suffix};
use linefeed::terminal::Terminal;
use linefeed::{Command, DefaultTerminal, Function, Interface, Prompter, ReadResult};

const TIMEOUT: Duration = Duration::from_millis(500);

const CLI_COMMANDS: &[(&str, &str, &str)] = &[
    ("connect", "[URI]", "Connect to server"),
    ("get", "<PATH> [[PATH] ...]", "Get signal value(s)"),
    ("set", "<PATH> <VALUE>", "Set actuator signal"),
    (
        "subscribe",
        "<QUERY>",
        "Subscribe to signals with QUERY, if you use kuksa feature comma separated list",
    ),
    ("feed", "<PATH> <VALUE>", "Publish signal value"),
    (
        "metadata",
        "[PATTERN]",
        "Fetch metadata. Provide PATTERN to list metadata of signals matching pattern.",
    ),
    ("token", "<TOKEN>", "Use TOKEN as access token"),
    (
        "token-file",
        "<FILE>",
        "Use content of FILE as access token",
    ),
    ("help", "", "You're looking at it."),
    ("quit", "", "Quit"),
];

#[cfg(feature = "feature_kuksa")]
mod root {
    pub use databroker_proto::kuksa::val as proto;
    pub use kuksa::*;
}

#[cfg(feature = "feature_sdv")]
mod root {
    pub use databroker_proto::sdv::databroker as proto;
    pub use sdv::*;
}

#[derive(Debug, Parser)]
#[clap(author, version, about, long_about = None)]
struct Cli {
    /// Server to connect to
    #[clap(long, display_order = 1, default_value = "http://127.0.0.1:55555")]
    server: String,

    // #[clap(short, long)]
    // port: Option<u16>,
    /// File containing access token
    #[clap(long, value_name = "FILE", display_order = 2)]
    token_file: Option<String>,

    /// CA certificate used to verify server certificate
    #[cfg(feature = "tls")]
    #[clap(long, value_name = "CERT", display_order = 3)]
    ca_cert: Option<String>,

    // Sub command
    #[clap(subcommand)]
    command: Option<Commands>,
}

#[derive(Debug, Subcommand)]
enum Commands {
    /// Get one or more datapoint(s)
    Get {
        #[clap(value_name = "PATH")]
        paths: Vec<String>,
    },
    // Subscribe,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    #[allow(unused_assignments)]
    let mut properties = Vec::new();
    #[cfg(feature = "feature_sdv")]
    {
        println!("Using sdv");
        properties = Vec::<root::proto::v1::Metadata>::new();
    }
    #[cfg(feature = "feature_kuksa")]
    {
        println!("Using kuksa");
        properties = Vec::<root::proto::v1::DataEntry>::new();
    }

    let mut subscription_nbr = 1;

    let completer = CliCompleter::new();
    let interface = Arc::new(Interface::new("client")?);
    interface.set_completer(Arc::new(completer));

    interface.define_function("enter-function", Arc::new(EnterFunction));
    interface.bind_sequence("\r", Command::from_str("enter-function"));
    interface.bind_sequence("\n", Command::from_str("enter-function"));

    set_disconnected_prompt(&interface);

    let cli = Cli::parse();

    let basic_client = common::Client::new(to_uri(cli.server)?);
    let mut client;
    #[cfg(feature = "feature_sdv")]
    {
        client = root::SDVClient::new(basic_client);
    }

    #[cfg(feature = "feature_kuksa")]
    {
        client = root::KuksaClient::new(basic_client);
    }

    if let Some(token_filename) = cli.token_file {
        let token = std::fs::read_to_string(token_filename)?;
        client.basic_client.set_access_token(token)?;
    }

    #[cfg(feature = "tls")]
    if let Some(ca_cert_filename) = cli.ca_cert {
        let pem = std::fs::read(ca_cert_filename)?;
        let ca_cert = tonic::transport::Certificate::from_pem(pem);

        let tls_config = tonic::transport::ClientTlsConfig::new().ca_certificate(ca_cert);

        client.basic_client.set_tls_config(tls_config);
    }

    let mut connection_state_subscription = client.basic_client.subscribe_to_connection_state();
    let interface_ref = interface.clone();

    tokio::spawn(async move {
        while let Some(state) = connection_state_subscription.next().await {
            match state {
                Ok(state) => match state {
                    common::ConnectionState::Connected => {
                        set_connected_prompt(&interface_ref);
                    }
                    common::ConnectionState::Disconnected => {
                        set_disconnected_prompt(&interface_ref);
                    }
                },
                Err(err) => {
                    print_error(
                        "connection",
                        format!("Connection state subscription failed: {err}"),
                    )
                    .unwrap_or_default();
                }
            }
        }
    });

    match cli.command {
        Some(Commands::Get { paths }) => {
            #[cfg(feature = "feature_sdv")]
            {
                match client.get_datapoints(paths).await {
                    Ok(datapoints) => {
                        for (name, datapoint) in datapoints {
                            println!("{}: {}", name, DisplayDatapoint(datapoint),);
                        }
                    }
                    Err(err) => {
                        eprintln!("{err}");
                    }
                }
            }
            #[cfg(feature = "feature_kuksa")]
            {
                match client.get_current_values(paths).await {
                    Ok(data_entries) => {
                        for entry in data_entries {
                            if let Some(val) = entry.value {
                                println!("{}: {}", entry.path, DisplayDatapoint(val),);
                            } else {
                                println!("{}: NotAvailable", entry.path);
                            }
                        }
                    }
                    Err(err) => {
                        eprintln!("{err}");
                    }
                }
            }
            return Ok(());
        }
        None => {
            // No subcommand => run interactive client
            let version = match option_env!("CARGO_PKG_VERSION") {
                Some(version) => format!("v{version}"),
                None => String::new(),
            };
            print_logo(version);

            match client.basic_client.try_connect().await {
                Ok(()) => {
                    print_info(format!(
                        "Successfully connected to {}",
                        client.basic_client.get_uri()
                    ))?;
                    #[allow(unused_mut)]
                    let mut pattern = vec![];
                    #[cfg(feature = "feature_kuksa")]
                    {
                        pattern = vec!["**"];
                    }
                    match client.get_metadata(pattern).await {
                        Ok(metadata) => {
                            interface
                                .set_completer(Arc::new(CliCompleter::from_metadata(&metadata)));
                            properties = metadata;
                        }
                        Err(common::ClientError::Status(status)) => {
                            print_resp_err("metadata", &status)?;
                        }
                        Err(common::ClientError::Connection(msg)) => {
                            print_error("metadata", msg)?;
                        }
                        Err(common::ClientError::Function(msg)) => {
                            print_resp_err_fmt("metadata", format_args!("Error {msg:?}"))?;
                        }
                    }
                }
                Err(err) => {
                    print_error("connect", format!("{err}"))?;
                }
            }
        }
    };

    loop {
        if let Some(res) = interface.read_line_step(Some(TIMEOUT))? {
            match res {
                ReadResult::Input(line) => {
                    let (cmd, args) = split_first_word(&line);
                    match cmd {
                        "help" => {
                            println!();
                            for &(cmd, args, help) in CLI_COMMANDS {
                                println!("  {:24} {}", format!("{cmd} {args}"), help);
                            }
                            println!();
                        }
                        "get" => {
                            interface.add_history_unique(line.clone());

                            if args.is_empty() {
                                print_usage(cmd);
                                continue;
                            }
                            let paths = args
                                .split_whitespace()
                                .map(|path| path.to_owned())
                                .collect();
                            #[cfg(feature = "feature_sdv")]
                            {
                                match client.get_datapoints(paths).await {
                                    Ok(datapoints) => {
                                        print_resp_ok(cmd)?;
                                        for (name, datapoint) in datapoints {
                                            println!("{}: {}", name, DisplayDatapoint(datapoint),);
                                        }
                                    }
                                    Err(common::ClientError::Status(err)) => {
                                        print_resp_err(cmd, &err)?;
                                    }
                                    Err(common::ClientError::Connection(msg)) => {
                                        print_error(cmd, msg)?;
                                    }
                                    Err(common::ClientError::Function(msg)) => {
                                        print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                    }
                                }
                            }
                            #[cfg(feature = "feature_kuksa")]
                            {
                                match client.get_current_values(paths).await {
                                    Ok(data_entries) => {
                                        print_resp_ok(cmd)?;
                                        for entry in data_entries {
                                            if let Some(val) = entry.value {
                                                println!(
                                                    "{}: {}",
                                                    entry.path,
                                                    DisplayDatapoint(val),
                                                );
                                            } else {
                                                println!("{}: NotAvailable", entry.path);
                                            }
                                        }
                                    }
                                    Err(common::ClientError::Status(err)) => {
                                        print_resp_err(cmd, &err)?;
                                    }
                                    Err(common::ClientError::Connection(msg)) => {
                                        print_error(cmd, msg)?;
                                    }
                                    Err(common::ClientError::Function(msg)) => {
                                        print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                    }
                                }
                            }
                        }
                        "token" => {
                            interface.add_history_unique(line.clone());

                            if args.is_empty() {
                                print_usage(cmd);
                                continue;
                            }

                            match client.basic_client.set_access_token(args) {
                                Ok(()) => {
                                    print_info("Access token set.")?;
                                    match client.get_metadata(vec![]).await {
                                        Ok(metadata) => {
                                            interface.set_completer(Arc::new(
                                                CliCompleter::from_metadata(&metadata),
                                            ));
                                            properties = metadata;
                                        }
                                        Err(common::ClientError::Status(status)) => {
                                            print_resp_err("metadata", &status)?;
                                        }
                                        Err(common::ClientError::Connection(msg)) => {
                                            print_error("metadata", msg)?;
                                        }
                                        Err(common::ClientError::Function(msg)) => {
                                            print_resp_err_fmt("metadata", format_args!("Error {msg:?}"))?;
                                        }
                                    }
                                }
                                Err(err) => print_error(cmd, &format!("Malformed token: {err}"))?,
                            }
                        }
                        "token-file" => {
                            interface.add_history_unique(line.clone());

                            if args.is_empty() {
                                print_usage(cmd);
                                continue;
                            }

                            let token_filename = args.trim();
                            match std::fs::read_to_string(token_filename) {
                                Ok(token) => match client.basic_client.set_access_token(token) {
                                    Ok(()) => {
                                        print_info("Access token set.")?;
                                        match client.get_metadata(vec![]).await {
                                            Ok(metadata) => {
                                                interface.set_completer(Arc::new(
                                                    CliCompleter::from_metadata(&metadata),
                                                ));
                                                properties = metadata;
                                            }
                                            Err(common::ClientError::Status(status)) => {
                                                print_resp_err("metadata", &status)?;
                                            }
                                            Err(common::ClientError::Connection(msg)) => {
                                                print_error("metadata", msg)?;
                                            }
                                            Err(common::ClientError::Function(msg)) => {
                                                print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                            }
                                        }
                                    }
                                    Err(err) => {
                                        print_error(cmd, &format!("Malformed token: {err}"))?
                                    }
                                },
                                Err(err) => print_error(
                                    cmd,
                                    &format!(
                                        "Failed to open token file \"{token_filename}\": {err}"
                                    ),
                                )?,
                            }
                        }
                        "set" => {
                            interface.add_history_unique(line.clone());

                            let (path, value) = split_first_word(args);

                            if value.is_empty() {
                                print_usage(cmd);
                                continue;
                            }

                            #[cfg(feature = "feature_sdv")]
                            {
                                let datapoint_metadata = {
                                    let mut datapoint_metadata = None;
                                    for metadata in properties.iter() {
                                        if metadata.name == path {
                                            datapoint_metadata = Some(metadata)
                                        }
                                    }
                                    datapoint_metadata
                                };

                                if datapoint_metadata.is_none() {
                                    print_info(format!(
                                        "No metadata available for {path}. Needed to determine data type for serialization."
                                    ))?;
                                    continue;
                                }

                                if metadata.entry_type != proto::v1::EntryType::Actuator as i32 {
                                    print_error(
                                        cmd,
                                        format!("{} is not an actuator.", metadata.name),
                                    )?;
                                    print_info(
                                        "If you want to provide the signal value, use `feed`.",
                                    )?;
                                    continue;
                                }

                                    if metadata.entry_type
                                        != root::proto::v1::EntryType::Actuator.into()
                                    {
                                        print_error(
                                            cmd,
                                            format!("{} is not an actuator.", metadata.name),
                                        )?;
                                        print_info(
                                            "If you want to provide the signal value, use `feed`.",
                                        )?;
                                        continue;
                                    }

                                    let ts = Timestamp::from(SystemTime::now());
                                    let datapoints = HashMap::from([(
                                        metadata.name.clone(),
                                        root::proto::v1::Datapoint {
                                            timestamp: Some(ts),
                                            value: Some(data_value.unwrap()),
                                        },
                                    )]);

                                    match client.set_datapoints(datapoints).await {
                                        Ok(message) => {
                                            if message.errors.is_empty() {
                                                print_resp_ok(cmd)?;
                                            } else {
                                                for (id, error) in message.errors {
                                                    match root::proto::v1::DatapointError::from_i32(
                                                        error,
                                                    ) {
                                                        Some(error) => {
                                                            print_resp_ok(cmd)?;
                                                            println!(
                                                                "Error setting {}: {}",
                                                                id,
                                                                Color::Red
                                                                    .paint(format!("{error:?}")),
                                                            );
                                                        }
                                                        None => print_resp_ok_fmt(
                                                            cmd,
                                                            format_args!("Error setting id {id}"),
                                                        )?,
                                                    }
                                                }
                                            }
                                        }
                                        Err(common::ClientError::Status(status)) => {
                                            print_resp_err(cmd, &status)?
                                        }
                                        Err(common::ClientError::Connection(msg)) => {
                                            print_error(cmd, msg)?
                                        }
                                        Err(common::ClientError::Function(msg)) => {
                                            print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                        }
                                    }
                                }
                            }
                            #[cfg(feature = "feature_kuksa")]
                            {
                                let datapoint_entries = match client.get_metadata(vec![path]).await
                                {
                                    Ok(data_entries) => Some(data_entries),
                                    Err(common::ClientError::Status(status)) => {
                                        print_resp_err("metadata", &status)?;
                                        None
                                    }
                                    Err(common::ClientError::Connection(msg)) => {
                                        print_error("metadata", msg)?;
                                        None
                                    }
                                    Err(common::ClientError::Function(msg)) => {
                                        print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                        None
                                    }
                                };

                                if let Some(entries) = datapoint_entries {
                                    for entry in entries {
                                        if let Some(metadata) = entry.metadata {
                                            let data_value = try_into_data_value(
                                                value,
                                                root::proto::v1::DataType::from_i32(
                                                    metadata.data_type,
                                                )
                                                .unwrap(),
                                            );
                                            if data_value.is_err() {
                                                println!(
                                                    "Could not parse \"{value}\" as {:?}",
                                                    root::proto::v1::DataType::from_i32(
                                                        metadata.data_type
                                                    )
                                                    .unwrap()
                                                );
                                                continue;
                                            }

                                            if metadata.entry_type
                                                != root::proto::v1::EntryType::Actuator.into()
                                            {
                                                print_error(
                                                    cmd,
                                                    format!("{} is not an actuator.", path),
                                                )?;
                                                print_info(
                                                    "If you want to provide the signal value, use `feed`.",
                                                )?;
                                                continue;
                                            }

                                            let ts = Timestamp::from(SystemTime::now());
                                            let datapoints = HashMap::from([(
                                                path.to_string().clone(),
                                                root::proto::v1::Datapoint {
                                                    timestamp: Some(ts),
                                                    value: Some(data_value.unwrap()),
                                                },
                                            )]);

                                            match client.set_current_values(datapoints).await {
                                                Ok(res) => {
                                                    print_resp_ok(cmd)?
                                                }
                                                Err(common::ClientError::Status(status)) => {
                                                    print_resp_err(cmd, &status)?
                                                }
                                                Err(common::ClientError::Connection(msg)) => {
                                                    print_error(cmd, msg)?
                                                }
                                                Err(common::ClientError::Function(msg)) => {
                                                    print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        "feed" => {
                            interface.add_history_unique(line.clone());

                            let (path, value) = split_first_word(args);

                            if value.is_empty() {
                                print_usage(cmd);
                                continue;
                            }

                            #[cfg(feature = "feature_sdv")]
                            {
                                let datapoint_metadata = {
                                    let mut datapoint_metadata = None;
                                    for metadata in properties.iter() {
                                        if metadata.name == path {
                                            datapoint_metadata = Some(metadata)
                                        }
                                    }
                                    datapoint_metadata
                                };

                                if datapoint_metadata.is_none() {
                                    print_info(
                                        format!("No metadata available for {path}. Needed to determine data type for serialization."),
                                    )?;
                                    continue;
                                }

                                if let Some(metadata) = datapoint_metadata {
                                    let data_value = try_into_data_value(
                                        value,
                                        root::proto::v1::DataType::from_i32(metadata.data_type)
                                            .unwrap(),
                                    );
                                    if data_value.is_err() {
                                        println!(
                                            "Could not parse \"{}\" as {:?}",
                                            value,
                                            root::proto::v1::DataType::from_i32(metadata.data_type)
                                                .unwrap()
                                        );
                                        continue;
                                    }
                                    let ts = Timestamp::from(SystemTime::now());
                                    let datapoints = HashMap::from([(
                                        metadata.id,
                                        root::proto::v1::Datapoint {
                                            timestamp: Some(ts),
                                            value: Some(data_value.unwrap()),
                                        },
                                    )]);

                                    match client.update_datapoints(datapoints).await {
                                        Ok(message) => {
                                            if message.errors.is_empty() {
                                                print_resp_ok(cmd)?
                                            } else {
                                                for (id, error) in message.errors {
                                                    let identifier = if id == metadata.id {
                                                        metadata.name.to_string()
                                                    } else {
                                                        format!("id {id}")
                                                    };
                                                    match root::proto::v1::DatapointError::from_i32(error) {
                                                        Some(error) => print_resp_ok_fmt(
                                                            cmd,
                                                            format_args!(
                                                                "Error providing {identifier}: {error:?}",
                                                            ),
                                                        )?,
                                                        None => print_resp_ok_fmt(
                                                            cmd,
                                                            format_args!("Error providing {identifier}",),
                                                        )?,
                                                    }
                                                }
                                            }
                                        }
                                        Err(common::ClientError::Status(status)) => {
                                            print_resp_err(cmd, &status)?
                                        }
                                        Err(common::ClientError::Connection(msg)) => {
                                            print_error(cmd, msg)?
                                        }
                                        Err(common::ClientError::Function(msg)) => {
                                            print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                        }
                                    }
                                }
                            }
                            #[cfg(feature = "feature_kuksa")]
                            {
                                let datapoint_entries = match client.get_metadata(vec![path]).await
                                {
                                    Ok(data_entries) => Some(data_entries),
                                    Err(common::ClientError::Status(status)) => {
                                        print_resp_err("metadata", &status)?;
                                        None
                                    }
                                    Err(common::ClientError::Connection(msg)) => {
                                        print_error("metadata", msg)?;
                                        None
                                    }
                                    Err(common::ClientError::Function(msg)) => {
                                        print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                        None
                                    }
                                };

                                if let Some(entries) = datapoint_entries {
                                    for entry in entries {
                                        if let Some(metadata) = entry.metadata {
                                            let data_value = try_into_data_value(
                                                value,
                                                root::proto::v1::DataType::from_i32(
                                                    metadata.data_type,
                                                )
                                                .unwrap(),
                                            );
                                            if data_value.is_err() {
                                                println!(
                                                    "Could not parse \"{}\" as {:?}",
                                                    value,
                                                    root::proto::v1::DataType::from_i32(
                                                        metadata.data_type
                                                    )
                                                    .unwrap()
                                                );
                                                continue;
                                            }
                                            let ts = Timestamp::from(SystemTime::now());
                                            let datapoints = HashMap::from([(
                                                path.to_string().clone(),
                                                root::proto::v1::Datapoint {
                                                    timestamp: Some(ts),
                                                    value: Some(data_value.unwrap()),
                                                },
                                            )]);

                                            match client.set_current_values(datapoints).await {
                                                Ok(res) => {
                                                    print_resp_ok(cmd);
                                                }
                                                Err(common::ClientError::Status(status)) => {
                                                    print_resp_err(cmd, &status)?
                                                }
                                                Err(common::ClientError::Connection(msg)) => {
                                                    print_error(cmd, msg)?
                                                }
                                                Err(common::ClientError::Function(msg)) => {
                                                    print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        "subscribe" => {
                            interface.add_history_unique(line.clone());

                            if args.is_empty() {
                                print_usage(cmd);
                                continue;
                            }

                            #[allow(unused_mut)]
                            let mut input;

                            #[cfg(feature = "feature_sdv")]
                            {
                                input = args.to_owned();
                            }
                            #[cfg(feature = "feature_kuksa")]
                            {
                                input = args.split_whitespace().collect::<Vec<_>>();
                            }

                            match client.subscribe(input).await {
                                Ok(mut subscription) => {
                                    let iface = interface.clone();
                                    tokio::spawn(async move {
                                        let sub_disp = format!("[{subscription_nbr}]");
                                        let sub_disp_pad = " ".repeat(sub_disp.len());
                                        let sub_disp_color =
                                            format!("{}", Color::White.dimmed().paint(&sub_disp));

                                        loop {
                                            match subscription.message().await {
                                                Ok(subscribe_resp) => {
                                                    if let Some(resp) = subscribe_resp {
                                                        // Build output before writing it
                                                        // (to avoid interleaving confusion)
                                                        use std::fmt::Write;
                                                        let mut output = String::new();
                                                        let mut first_line = true;
                                                        #[cfg(feature = "feature_sdv")]
                                                        {
                                                            for (name, value) in resp.fields {
                                                                if first_line {
                                                                    first_line = false;
                                                                    write!(
                                                                        output,
                                                                        "{} ",
                                                                        &sub_disp_color,
                                                                    )
                                                                    .unwrap();
                                                                } else {
                                                                    write!(
                                                                        output,
                                                                        "{} ",
                                                                        &sub_disp_pad,
                                                                    )
                                                                    .unwrap();
                                                                }
                                                                writeln!(
                                                                    output,
                                                                    "{}: {}",
                                                                    name,
                                                                    DisplayDatapoint(value)
                                                                )
                                                                .unwrap();
                                                            }
                                                            write!(iface, "{output}").unwrap();
                                                        }
                                                        #[cfg(feature = "feature_kuksa")]
                                                        {
                                                            for update in resp.updates {
                                                                if first_line {
                                                                    first_line = false;
                                                                    write!(
                                                                        output,
                                                                        "{} ",
                                                                        &sub_disp_color,
                                                                    )
                                                                    .unwrap();
                                                                } else {
                                                                    write!(
                                                                        output,
                                                                        "{} ",
                                                                        &sub_disp_pad,
                                                                    )
                                                                    .unwrap();
                                                                }
                                                                if let Some(entry) = update.entry {
                                                                    if let Some(value) = entry.value
                                                                    {
                                                                        writeln!(
                                                                            output,
                                                                            "{}: {}",
                                                                            entry.path,
                                                                            DisplayDatapoint(value)
                                                                        )
                                                                        .unwrap();
                                                                    }
                                                                }
                                                            }
                                                            write!(iface, "{output}").unwrap();
                                                        }
                                                    } else {
                                                        writeln!(
                                                            iface,
                                                            "{} {}",
                                                            Color::Red.dimmed().paint(&sub_disp),
                                                            Color::White.dimmed().paint(
                                                                "Server gone. Subscription stopped"
                                                            ),
                                                        )
                                                        .unwrap();
                                                        break;
                                                    }
                                                }
                                                Err(err) => {
                                                    write!(
                                                        iface,
                                                        "{} {}",
                                                        &sub_disp_color,
                                                        Color::Red
                                                            .dimmed()
                                                            .paint(format!("Channel error: {err}"))
                                                    )
                                                    .unwrap();
                                                    break;
                                                }
                                            }
                                        }
                                    });

                                    print_resp_ok(cmd)?;
                                    print_info(format!(
                                                    "Subscription is now running in the background. Received data is identified by [{subscription_nbr}]."
                                                )
                                            )?;
                                    subscription_nbr += 1;
                                }
                                Err(common::ClientError::Status(status)) => {
                                    print_resp_err(cmd, &status)?
                                }
                                Err(common::ClientError::Connection(msg)) => print_error(cmd, msg)?,
                                Err(common::ClientError::Function(msg)) => print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?,
                            }
                        }
                        "connect" => {
                            interface.add_history_unique(line.clone());
                            if !client.basic_client.is_connected() || !args.is_empty() {
                                if args.is_empty() {
                                    match client.basic_client.try_connect().await {
                                        Ok(()) => {
                                            print_info(format!(
                                                "[{cmd}] Successfully connected to {}",
                                                client.basic_client.get_uri()
                                            ))?;
                                        }
                                        Err(err) => {
                                            print_error(cmd, format!("{err}"))?;
                                        }
                                    }
                                } else {
                                    match to_uri(args) {
                                        Ok(valid_uri) => {
                                            match client
                                                .basic_client
                                                .try_connect_to(valid_uri)
                                                .await
                                            {
                                                Ok(()) => {
                                                    print_info(format!(
                                                        "[{cmd}] Successfully connected to {}",
                                                        client.basic_client.get_uri()
                                                    ))?;
                                                }
                                                Err(err) => {
                                                    print_error(cmd, format!("{err}"))?;
                                                }
                                            }
                                        }
                                        Err(err) => {
                                            print_error(
                                                cmd,
                                                format!("Failed to parse endpoint address: {err}"),
                                            )?;
                                        }
                                    }
                                };
                                if client.basic_client.is_connected() {
                                    match client.get_metadata(vec![]).await {
                                        Ok(metadata) => {
                                            interface.set_completer(Arc::new(
                                                CliCompleter::from_metadata(&metadata),
                                            ));
                                            properties = metadata;
                                        }
                                        Err(common::ClientError::Status(status)) => {
                                            print_resp_err("metadata", &status)?;
                                        }
                                        Err(common::ClientError::Connection(msg)) => {
                                            print_error("metadata", msg)?;
                                        }
                                        Err(common::ClientError::Function(msg)) => {
                                            print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                        }
                                    }
                                }
                            };
                        }
                        "metadata" => {
                            interface.add_history_unique(line.clone());

                            let paths = args.split_whitespace().collect::<Vec<_>>();

                            #[cfg(feature = "feature_sdv")]
                            {
                                match client.get_metadata(vec![]).await {
                                    Ok(mut metadata) => {
                                        metadata.sort_by(|a, b| a.name.cmp(&b.name));
                                        properties = metadata;
                                        interface.set_completer(Arc::new(
                                            CliCompleter::from_metadata(&properties),
                                        ));
                                        print_resp_ok(cmd)?;
                                    }
                                    Err(common::ClientError::Status(status)) => {
                                        print_resp_err(cmd, &status)?;
                                        continue;
                                    }
                                    Err(common::ClientError::Connection(msg)) => {
                                        print_error(cmd, msg)?;
                                        continue;
                                    }
                                    Err(common::ClientError::Function(msg)) => {
                                        print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                        continue;
                                    }
                                }
                                let mut filtered_metadata = Vec::new();
                                if paths.is_empty() {
                                    print_info("If you want to list metadata of signals, use `metadata PATTERN`")?;
                                    // filtered_metadata.extend(&properties);
                                } else {
                                    for path in &paths {
                                        let path_re = path_to_regex(path);
                                        let filtered =
                                            properties.iter().filter(|item| match &path_re {
                                                Ok(re) => re.is_match(&item.name),
                                                Err(err) => {
                                                    print_info(format!("Invalid path: {err}"))
                                                        .unwrap_or_default();
                                                    false
                                                }
                                            });
                                        filtered_metadata.extend(filtered);
                                    }
                                }

                                if !filtered_metadata.is_empty() {
                                    let max_len_path =
                                        filtered_metadata.iter().fold(0, |mut max_len, item| {
                                            if item.name.len() > max_len {
                                                max_len = item.name.len();
                                            }
                                            max_len
                                        });

                                    print_info(format!(
                                        "{:<max_len_path$} {:<10} {:<9}",
                                        "Path", "Entry type", "Data type"
                                    ))?;

                                    for entry in &filtered_metadata {
                                        println!(
                                            "{:<max_len_path$} {:<10} {:<9}",
                                            entry.name,
                                            DisplayEntryType::from(
                                                root::proto::v1::EntryType::from_i32(
                                                    entry.entry_type
                                                )
                                            ),
                                            DisplayDataType::from(
                                                root::proto::v1::DataType::from_i32(
                                                    entry.data_type
                                                )
                                            ),
                                        );
                                    }
                                }
                            }
                            #[cfg(feature = "feature_kuksa")]
                            {
                                if paths.is_empty() {
                                    print_info("If you want to list metadata of signals, use `metadata PATTERN`")?;
                                } else {
                                    match client.get_metadata(paths).await {
                                        Ok(metadata) => {
                                            print_resp_ok(cmd)?;
                                            if !metadata.is_empty() {
                                                let max_len_path =
                                                    metadata.iter().fold(0, |mut max_len, item| {
                                                        if item.path.len() > max_len {
                                                            max_len = item.path.len();
                                                        }
                                                        max_len
                                                    });

                                                print_info(format!(
                                                    "{:<max_len_path$} {:<10} {:<9}",
                                                    "Path", "Entry type", "Data type"
                                                ))?;

                                                for entry in &metadata {
                                                    if let Some(entry_metadata) = &entry.metadata {
                                                        println!(
                                                            "{:<max_len_path$} {:<10} {:<9}",
                                                            entry.path,
                                                            DisplayEntryType::from(root::proto::v1::EntryType::from_i32(
                                                                entry_metadata.entry_type
                                                            )),
                                                            DisplayDataType::from(root::proto::v1::DataType::from_i32(
                                                                entry_metadata.data_type
                                                            )),
                                                        );
                                                    } else {
                                                        let name = entry.path.clone();
                                                        println!("No entry metadata for {name}");
                                                    }
                                                }
                                            }
                                        }
                                        Err(common::ClientError::Status(status)) => {
                                            print_resp_err(cmd, &status)?;
                                            continue;
                                        }
                                        Err(common::ClientError::Connection(msg)) => {
                                            print_error(cmd, msg)?;
                                            continue;
                                        }
                                        Err(common::ClientError::Function(msg)) => {
                                            print_resp_err_fmt(cmd, format_args!("Error {msg:?}"))?;
                                        }
                                    }
                                }
                            }
                        }
                        "quit" | "exit" => {
                            println!("Bye bye!");
                            break;
                        }
                        "" => {} // Ignore empty input
                        _ => {
                            println!(
                                "Unknown command. See `help` for a list of available commands."
                            );
                            interface.add_history_unique(line.clone());
                        }
                    }
                }
                ReadResult::Eof => {
                    println!("Bye bye!");
                    break;
                }
                ReadResult::Signal(sig) => {
                    // println!("received signal: {:?}", sig);
                    if sig == linefeed::Signal::Interrupt {
                        interface.cancel_read_line()?;
                    }

                    let _ = writeln!(interface, "signal received: {sig:?}");
                }
            }
        }
    }

    Ok(())
}

fn print_usage(command: impl AsRef<str>) {
    for (cmd, usage, _) in CLI_COMMANDS {
        if *cmd == command.as_ref() {
            println!("Usage: {cmd} {usage}");
        }
    }
}

fn print_logo(version: impl fmt::Display) {
    let mut output = io::stderr().lock();
    writeln!(output).unwrap();
    writeln!(
        output,
        "  {}{}",
        Color::Fixed(23).paint("⠀⠀⠀⢀⣤⣶⣾⣿"),
        Color::White.dimmed().paint("⢸⣿⣿⣷⣶⣤⡀")
    )
    .unwrap();
    writeln!(
        output,
        "  {}{}",
        Color::Fixed(23).paint("⠀⠀⣴⣿⡿⠋⣿⣿"),
        Color::White.dimmed().paint("⠀⠀⠀⠈⠙⢿⣿⣦⠀")
    )
    .unwrap();
    writeln!(
        output,
        "  {}{}",
        Color::Fixed(23).paint("⠀⣾⣿⠋⠀⠀⣿⣿"),
        Color::White.dimmed().paint("⠀⠀⣶⣿⠀⠀⠙⣿⣷   ")
    )
    .unwrap();
    writeln!(
        output,
        "  {}{}",
        Color::Fixed(23).paint("⣸⣿⠇⠀⠀⠀⣿⣿"),
        Color::White
            .dimmed()
            .paint("⠠⣾⡿⠃⠀⠀⠀⠸⣿⣇⠀⠀⣶⠀⣠⡶⠂⠀⣶⠀⠀⢰⡆⠀⢰⡆⢀⣴⠖⠀⢠⡶⠶⠶⡦⠀⠀⠀⣰⣶⡀")
    )
    .unwrap();
    writeln!(
        output,
        "  {}{}",
        Color::Fixed(23).paint("⣿⣿⠀⠀⠀⠀⠿⢿⣷⣦⡀"),
        Color::White
            .dimmed()
            .paint("⠀⠀⠀⠀⠀⣿⣿⠀⠀⣿⢾⣏⠀⠀⠀⣿⠀⠀⢸⡇⠀⢸⡷⣿⡁⠀⠀⠘⠷⠶⠶⣦⠀⠀⢠⡟⠘⣷")
    )
    .unwrap();
    writeln!(
        output,
        "  {}{}{}{}",
        Color::Fixed(23).paint("⢹⣿⡆⠀⠀⠀"),
        Color::White.dimmed().paint("⣿⣶"),
        Color::Fixed(23).paint("⠈⢻⣿⡆"),
        Color::White
            .dimmed()
            .paint("⠀⠀⠀⢰⣿⡏⠀⠀⠿⠀⠙⠷⠄⠀⠙⠷⠶⠟⠁⠀⠸⠇⠈⠻⠦⠀⠐⠷⠶⠶⠟⠀⠠⠿⠁⠀⠹⠧")
    )
    .unwrap();
    writeln!(
        output,
        "  {}{}{}{}",
        Color::Fixed(23).paint("⠀⢿⣿⣄⠀⠀"),
        Color::White.dimmed().paint("⣿⣿"),
        Color::Fixed(23).paint("⠀⠀⠿⣿"),
        Color::White.dimmed().paint("⠀⠀⣠⣿⡿"),
    )
    .unwrap();
    writeln!(
        output,
        "  {}{}    {}",
        Color::Fixed(23).paint("⠀⠀⠻⣿⣷⡄"),
        Color::White.dimmed().paint("⣿⣿⠀⠀⠀⢀⣠⣾⣿⠟"),
        Color::White
            .dimmed()
            .paint(format!("{:<30}", "databroker-cli")),
    )
    .unwrap();
    writeln!(
        output,
        "  {}{}     {}",
        Color::Fixed(23).paint("⠀⠀⠀⠈⠛⠇"),
        Color::White.dimmed().paint("⢿⣿⣿⣿⣿⡿⠿⠛⠁"),
        Color::White.dimmed().paint(format!("{version:<30}")),
    )
    .unwrap();
    writeln!(output).unwrap();
}

fn split_first_word(s: &str) -> (&str, &str) {
    let s = s.trim();

    match s.find(|ch: char| ch.is_whitespace()) {
        Some(pos) => (&s[..pos], s[pos..].trim_start()),
        None => (s, ""),
    }
}

fn code_to_text(code: &tonic::Code) -> &str {
    match code {
        tonic::Code::Ok => "Ok",
        tonic::Code::Cancelled => "Cancelled",
        tonic::Code::Unknown => "Unknown",
        tonic::Code::InvalidArgument => "InvalidArgument",
        tonic::Code::DeadlineExceeded => "DeadlineExceeded",
        tonic::Code::NotFound => "NotFound",
        tonic::Code::AlreadyExists => "AlreadyExists",
        tonic::Code::PermissionDenied => "PermissionDenied",
        tonic::Code::ResourceExhausted => "ResourceExhausted",
        tonic::Code::FailedPrecondition => "FailedPrecondition",
        tonic::Code::Aborted => "Aborted",
        tonic::Code::OutOfRange => "OutOfRange",
        tonic::Code::Unimplemented => "Unimplemented",
        tonic::Code::Internal => "Internal",
        tonic::Code::Unavailable => "Unavailable",
        tonic::Code::DataLoss => "DataLoss",
        tonic::Code::Unauthenticated => "Unauthenticated",
    }
}

struct CliCompleter {
    paths: PathPart,
}

#[derive(Debug)]
struct PathPart {
    rel_path: String,
    full_path: String,
    children: HashMap<String, PathPart>,
}

impl PathPart {
    fn new() -> Self {
        PathPart {
            rel_path: "".into(),
            full_path: "".into(),
            children: HashMap::new(),
        }
    }
}
impl CliCompleter {
    fn new() -> CliCompleter {
        CliCompleter {
            paths: PathPart::new(),
        }
    }

    #[cfg(feature = "feature_sdv")]
    fn from_metadata(metadata: &[root::proto::v1::Metadata]) -> CliCompleter {
        let mut root = PathPart::new();
        for entry in metadata {
            let mut parent = &mut root;
            let parts = entry.name.split('.');
            for part in parts {
                let full_path = match parent.full_path.as_str() {
                    "" => part.to_owned(),
                    _ => format!("{}.{}", parent.full_path, part),
                };
                let entry = parent
                    .children
                    .entry(part.to_lowercase())
                    .or_insert(PathPart {
                        rel_path: part.to_owned(),
                        full_path,
                        children: HashMap::new(),
                    });

                parent = entry;
            }
        }
        CliCompleter { paths: root }
    }
    #[cfg(feature = "feature_kuksa")]
    fn from_metadata(entries: &Vec<root::proto::v1::DataEntry>) -> CliCompleter {
        let mut root = PathPart::new();
        for entry in entries {
            let mut parent = &mut root;
            let parts = entry.path.split('.');
            for part in parts {
                let full_path = match parent.full_path.as_str() {
                    "" => part.to_owned(),
                    _ => format!("{}.{}", parent.full_path, part),
                };
                let entry = parent
                    .children
                    .entry(part.to_lowercase())
                    .or_insert(PathPart {
                        rel_path: part.to_owned(),
                        full_path,
                        children: HashMap::new(),
                    });

                parent = entry;
            }
        }
        CliCompleter { paths: root }
    }

    fn complete_entry_path(&self, word: &str) -> Option<Vec<Completion>> {
        if !self.paths.children.is_empty() {
            let mut res = Vec::new();

            let lowercase_word = word.to_lowercase();
            let mut parts = lowercase_word.split('.');
            let mut path = &self.paths;
            loop {
                match parts.next() {
                    Some(part) => {
                        match path.children.get(part) {
                            Some(matching_path) => {
                                path = matching_path;
                            }
                            None => {
                                // match partial
                                for (path_part_lower, path_spec) in &path.children {
                                    if path_part_lower.starts_with(part) {
                                        if !path_spec.children.is_empty() {
                                            // This is a branch
                                            res.push(Completion {
                                                completion: format!("{}.", path_spec.full_path),
                                                display: Some(format!("{}.", path_spec.rel_path)),
                                                suffix: Suffix::None,
                                            });
                                        } else {
                                            res.push(Completion {
                                                completion: path_spec.full_path.to_owned(),
                                                display: Some(path_spec.rel_path.to_owned()),
                                                suffix: Suffix::Default,
                                            });
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                    None => {
                        for path_spec in path.children.values() {
                            if !path_spec.children.is_empty() {
                                // This is a branch
                                res.push(Completion {
                                    completion: format!("{}.", path_spec.full_path),
                                    display: Some(format!("{}.", path_spec.rel_path)),
                                    suffix: Suffix::None,
                                });
                            } else {
                                res.push(Completion {
                                    completion: path_spec.full_path.to_owned(),
                                    display: Some(path_spec.rel_path.to_owned()),
                                    suffix: Suffix::Default,
                                });
                            }
                        }
                        break;
                    }
                }
            }

            res.sort_by(|a, b| a.display().cmp(&b.display()));
            Some(res)
        } else {
            None
        }
    }
}

fn set_connected_prompt(interface: &Arc<Interface<DefaultTerminal>>) {
    let mut _text;
    #[cfg(feature = "feature_sdv")]{
        _text = "sdv.databroker.v1"
    }
    #[cfg(feature = "feature_kuksa")]{
        _text = "kuksa.val.v1"
    }
    let connected_prompt = format!(
        "\x01{prefix}\x02{text}\x01{suffix}\x02 > ",
        prefix = Color::Green.prefix(),
        text = _text,
        suffix = Color::Green.suffix()
    );
    interface.set_prompt(&connected_prompt).unwrap();
}

fn set_disconnected_prompt(interface: &Arc<Interface<DefaultTerminal>>) {
    let disconnected_prompt = format!(
        "\x01{prefix}\x02{text}\x01{suffix}\x02 > ",
        prefix = Color::Red.prefix(),
        text = "not connected",
        suffix = Color::Red.suffix()
    );
    interface.set_prompt(&disconnected_prompt).unwrap();
}

fn to_uri(uri: impl AsRef<str>) -> Result<Uri, String> {
    let uri = uri
        .as_ref()
        .parse::<tonic::transport::Uri>()
        .map_err(|err| format!("{err}"))?;
    let mut parts = uri.into_parts();

    if parts.scheme.is_none() {
        parts.scheme = Some("http".parse().expect("http should be valid scheme"));
    }

    match &parts.authority {
        Some(_authority) => {
            // match (authority.port_u16(), port) {
            //     (Some(uri_port), Some(port)) => {
            //         if uri_port != port {
            //             parts.authority = format!("{}:{}", authority.host(), port)
            //                 .parse::<Authority>()
            //                 .map_err(|err| format!("{}", err))
            //                 .ok();
            //         }
            //     }
            //     (_, _) => {}
            // }
        }
        None => return Err("No server uri specified".to_owned()),
    }
    parts.path_and_query = Some("".parse().expect("uri path should be empty string"));
    tonic::transport::Uri::from_parts(parts).map_err(|err| format!("{err}"))
}

fn path_to_regex(path: impl AsRef<str>) -> Result<regex::Regex, regex::Error> {
    let path_as_re = format!(
        // Match the whole line (from left '^' to right '$')
        "^{}$",
        path.as_ref().replace('.', r"\.").replace('*', r"(.*)")
    );
    regex::Regex::new(&path_as_re)
}

fn print_resp_err(operation: impl AsRef<str>, err: &tonic::Status) -> io::Result<()> {
    let mut output = io::stderr().lock();
    output.write_fmt(format_args!(
        "{} {} {}",
        Color::White
            .dimmed()
            .paint(format!("[{}]", operation.as_ref())),
        Color::White
            .on(Color::Red)
            .paint(format!(" {} ", code_to_text(&err.code()))),
        err.message(),
    ))?;
    output.write_all(b"\n")?;
    output.flush()
}

fn print_resp_err_fmt(operation: impl AsRef<str>, fmt: fmt::Arguments<'_>) -> io::Result<()> {
    let mut stderr = io::stderr().lock();
    let mut stdout = io::stdout().lock();
    write_resp_ok(&mut stderr, operation)?;
    stdout.write_fmt(fmt)?;
    stdout.write_all(b"\n")?;
    stdout.flush()
}

fn print_resp_ok_fmt(operation: impl AsRef<str>, fmt: fmt::Arguments<'_>) -> io::Result<()> {
    let mut stderr = io::stderr().lock();
    let mut stdout = io::stdout().lock();
    write_resp_ok(&mut stderr, operation)?;
    stdout.write_fmt(fmt)?;
    stdout.write_all(b"\n")?;
    stdout.flush()
}

fn print_resp_ok(operation: impl AsRef<str>) -> io::Result<()> {
    let mut output = io::stderr().lock();
    write_resp_ok(&mut output, operation)?;
    output.write_all(b"\n")?;
    output.flush()
}

fn write_resp_ok(output: &mut impl Write, operation: impl AsRef<str>) -> io::Result<()> {
    output.write_fmt(format_args!(
        "{} {} ",
        Color::White
            .dimmed()
            .paint(format!("[{}]", operation.as_ref())),
        Color::Black.on(Color::Green).paint(" OK "),
    ))
}

fn print_info(info: impl AsRef<str>) -> io::Result<()> {
    let mut output = io::stderr().lock();
    output.write_fmt(format_args!(
        "{}\n",
        Color::White.dimmed().paint(info.as_ref()),
    ))?;
    output.flush()
}

fn print_error(operation: impl AsRef<str>, msg: impl AsRef<str>) -> io::Result<()> {
    let mut output = io::stderr().lock();
    output.write_fmt(format_args!(
        "{} {} {}\n",
        Color::White
            .dimmed()
            .paint(format!("[{}]", operation.as_ref())),
        Color::White.on(Color::Red).paint(" Error "),
        msg.as_ref(),
    ))?;
    output.flush()
}

impl<Term: Terminal> Completer<Term> for CliCompleter {
    fn complete(
        &self,
        word: &str,
        prompter: &Prompter<Term>,
        start: usize,
        _end: usize,
    ) -> Option<Vec<Completion>> {
        let line = prompter.buffer();

        let mut words = line[..start].split_whitespace();

        match words.next() {
            // Complete command name
            None => {
                let mut compls = Vec::new();

                for &(cmd, _, _) in CLI_COMMANDS {
                    if cmd.starts_with(word) {
                        compls.push(Completion {
                            completion: cmd.to_owned(),
                            display: None,
                            suffix: Suffix::default(), //Suffix::Some('('),
                        });
                    }
                }

                Some(compls)
            }
            // Complete command parameters
            Some("set") | Some("feed") => {
                if words.count() == 0 {
                    self.complete_entry_path(word)
                } else {
                    None
                }
            }
            Some("get") | Some("metadata") => self.complete_entry_path(word),
            Some("subscribe") => {
                #[cfg(feature = "feature_sdv")]{
                    match words.next() {
                        None => Some(vec![Completion::simple("SELECT".to_owned())]),
                        Some(next) => {
                            if next == "SELECT" {
                                self.complete_entry_path(word)
                            } else {
                                None
                            }
                        }
                    }
                }
                #[cfg(feature = "feature_kuksa")]{
                    if words.count() == 0 {
                        self.complete_entry_path(word)
                    } else {
                        None
                    }
                }
            },
            Some("token-file") => {
                let path_completer = linefeed::complete::PathCompleter;
                path_completer.complete(word, prompter, start, _end)
            }
            _ => None,
        }
    }
}

struct EnterFunction;

impl<Term: Terminal> Function<Term> for EnterFunction {
    fn execute(&self, prompter: &mut Prompter<Term>, count: i32, _ch: char) -> io::Result<()> {
        if prompter
            .buffer()
            .trim()
            // .to_lowercase()
            .starts_with("subscribe")
        {
            if prompter.buffer().ends_with('\n') {
                let len = prompter.buffer().len();
                prompter.delete_range(len - 1..len)?;
                prompter.accept_input()
            } else if count > 0 {
                // Start multiline
                prompter.insert_str("\n")
            } else {
                Ok(())
            }
        } else {
            prompter.accept_input()
        }
    }
}

struct DisplayDataType(Option<root::proto::v1::DataType>);
struct DisplayEntryType(Option<root::proto::v1::EntryType>);
// !!! ChangeType currently just exists in old API needs to be removed or added later !!!
struct DisplayChangeType(Option<databroker_proto::sdv::databroker::v1::ChangeType>);
struct DisplayDatapoint(root::proto::v1::Datapoint);

fn display_array<T>(f: &mut fmt::Formatter<'_>, array: &[T]) -> fmt::Result
where
    T: fmt::Display,
{
    f.write_str("[")?;
    let real_delimiter = ", ";
    let mut delimiter = "";
    for value in array {
        write!(f, "{delimiter}")?;
        delimiter = real_delimiter;
        write!(f, "{value}")?;
    }
    f.write_str("]")
}

impl fmt::Display for DisplayDatapoint {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        #[cfg(feature = "feature_sdv")]
        {
            match &self.0.value {
                Some(value) => match value {
                    root::proto::v1::datapoint::Value::BoolValue(value) => {
                        f.pad(&format!("{value}"))
                    }
                    root::proto::v1::datapoint::Value::FailureValue(failure) => f.pad(&format!(
                        "( {:?} )",
                        root::proto::v1::datapoint::Failure::from_i32(*failure).unwrap()
                    )),
                    root::proto::v1::datapoint::Value::Int32Value(value) => {
                        f.pad(&format!("{value}"))
                    }
                    root::proto::v1::datapoint::Value::Int64Value(value) => {
                        f.pad(&format!("{value}"))
                    }
                    root::proto::v1::datapoint::Value::Uint32Value(value) => {
                        f.pad(&format!("{value}"))
                    }
                    root::proto::v1::datapoint::Value::Uint64Value(value) => {
                        f.pad(&format!("{value}"))
                    }
                    root::proto::v1::datapoint::Value::FloatValue(value) => {
                        f.pad(&format!("{value:.2}"))
                    }
                    root::proto::v1::datapoint::Value::DoubleValue(value) => {
                        f.pad(&format!("{value}"))
                    }
                    root::proto::v1::datapoint::Value::StringValue(value) => {
                        f.pad(&format!("'{value}'"))
                    }
                    root::proto::v1::datapoint::Value::StringArray(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::BoolArray(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::Int32Array(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::Int64Array(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::Uint32Array(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::Uint64Array(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::FloatArray(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::DoubleArray(array) => {
                        display_array(f, &array.values)
                    }
                },
                None => f.pad("None"),
            }
        }
        #[cfg(feature = "feature_kuksa")]
        {
            match &self.0.value {
                Some(value) => match value {
                    root::proto::v1::datapoint::Value::Bool(value) => f.pad(&format!("{value}")),
                    root::proto::v1::datapoint::Value::Int32(value) => f.pad(&format!("{value}")),
                    root::proto::v1::datapoint::Value::Int64(value) => f.pad(&format!("{value}")),
                    root::proto::v1::datapoint::Value::Uint32(value) => f.pad(&format!("{value}")),
                    root::proto::v1::datapoint::Value::Uint64(value) => f.pad(&format!("{value}")),
                    root::proto::v1::datapoint::Value::Float(value) => {
                        f.pad(&format!("{value:.2}"))
                    }
                    root::proto::v1::datapoint::Value::Double(value) => f.pad(&format!("{value}")),
                    root::proto::v1::datapoint::Value::String(value) => {
                        f.pad(&format!("'{value}'"))
                    }
                    root::proto::v1::datapoint::Value::StringArray(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::BoolArray(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::Int32Array(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::Int64Array(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::Uint32Array(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::Uint64Array(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::FloatArray(array) => {
                        display_array(f, &array.values)
                    }
                    root::proto::v1::datapoint::Value::DoubleArray(array) => {
                        display_array(f, &array.values)
                    }
                },
                None => f.pad("None"),
            }
        }
    }
}

impl From<Option<root::proto::v1::EntryType>> for DisplayEntryType {
    fn from(input: Option<root::proto::v1::EntryType>) -> Self {
        DisplayEntryType(input)
    }
}

impl fmt::Display for DisplayEntryType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.0 {
            Some(entry_type) => f.pad(&format!("{entry_type:?}")),
            None => f.pad("Unknown"),
        }
    }
}

impl From<Option<root::proto::v1::DataType>> for DisplayDataType {
    fn from(input: Option<root::proto::v1::DataType>) -> Self {
        DisplayDataType(input)
    }
}

impl fmt::Display for DisplayDataType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.0 {
            Some(data_type) => f.pad(&format!("{data_type:?}")),
            None => f.pad("Unknown"),
        }
    }
}

impl From<Option<databroker_proto::sdv::databroker::v1::ChangeType>> for DisplayChangeType {
    fn from(input: Option<databroker_proto::sdv::databroker::v1::ChangeType>) -> Self {
        DisplayChangeType(input)
    }
}
impl fmt::Display for DisplayChangeType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.0 {
            Some(data_type) => f.pad(&format!("{data_type:?}")),
            None => f.pad("Unknown"),
        }
    }
}

#[derive(Debug)]
struct ParseError {}

impl std::error::Error for ParseError {}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("parse error")
    }
}

fn get_array_from_input<T: std::str::FromStr>(values: String) -> Result<Vec<T>, ParseError> {
    let raw_input = values
        .strip_prefix('[')
        .and_then(|s| s.strip_suffix(']'))
        .ok_or(ParseError {})?;

    let pattern = r#"(?:\\.|[^",])*"(?:\\.|[^"])*"|[^",]+"#;

    let regex = regex::Regex::new(pattern).unwrap();
    let inputs = regex.captures_iter(raw_input);

    let mut array: Vec<T> = vec![];
    for part in inputs {
        match part[0]
            .trim()
            .replace('\"', "")
            .replace('\\', "\"")
            .parse::<T>()
        {
            Ok(value) => array.push(value),
            Err(_) => return Err(ParseError {}),
        }
    }
    Ok(array)
}

fn try_into_data_value(
    input: &str,
    data_type: root::proto::v1::DataType,
) -> Result<root::proto::v1::datapoint::Value, ParseError> {
    if input == "NotAvailable" {
        #[cfg(feature = "feature_sdv")]
        {
            return Ok(root::proto::v1::datapoint::Value::FailureValue(
                root::proto::v1::datapoint::Failure::NotAvailable as i32,
            ));
        }
        #[cfg(feature = "feature_kuksa")]
        {
            return Ok(root::proto::v1::datapoint::Value::String(input.to_string()));
        }
    }

    #[cfg(feature = "feature_sdv")]
    {
        match data_type {
            root::proto::v1::DataType::String => Ok(
                root::proto::v1::datapoint::Value::StringValue(input.to_owned()),
            ),
            root::proto::v1::DataType::StringArray => {
                match get_array_from_input(input.to_owned()) {
                    Ok(value) => Ok(root::proto::v1::datapoint::Value::StringArray(
                        root::proto::v1::StringArray { values: value },
                    )),
                    Err(err) => Err(err),
                }
            }
            root::proto::v1::DataType::Bool => match input.parse::<bool>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::BoolValue(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::BoolArray => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::BoolArray(
                    root::proto::v1::BoolArray { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Int8 => match input.parse::<i8>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32Value(value as i32)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Int8Array => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32Array(
                    root::proto::v1::Int32Array { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Int16 => match input.parse::<i16>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32Value(value as i32)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Int16Array => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32Array(
                    root::proto::v1::Int32Array { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Int32 => match input.parse::<i32>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32Value(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Int32Array => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32Array(
                    root::proto::v1::Int32Array { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Int64 => match input.parse::<i64>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int64Value(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Int64Array => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int64Array(
                    root::proto::v1::Int64Array { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Uint8 => match input.parse::<u8>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32Value(value as u32)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Uint8Array => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32Array(
                    root::proto::v1::Uint32Array { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Uint16 => match input.parse::<u16>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32Value(value as u32)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Uint16Array => {
                match get_array_from_input(input.to_owned()) {
                    Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32Array(
                        root::proto::v1::Uint32Array { values: value },
                    )),
                    Err(err) => Err(err),
                }
            }
            root::proto::v1::DataType::Uint32 => match input.parse::<u32>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32Value(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Uint32Array => {
                match get_array_from_input(input.to_owned()) {
                    Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32Array(
                        root::proto::v1::Uint32Array { values: value },
                    )),
                    Err(err) => Err(err),
                }
            }
            root::proto::v1::DataType::Uint64 => match input.parse::<u64>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint64Value(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Uint64Array => {
                match get_array_from_input(input.to_owned()) {
                    Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint64Array(
                        root::proto::v1::Uint64Array { values: value },
                    )),
                    Err(err) => Err(err),
                }
            }
            root::proto::v1::DataType::Float => match input.parse::<f32>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::FloatValue(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::FloatArray => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::FloatArray(
                    root::proto::v1::FloatArray { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Double => match input.parse::<f64>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::DoubleValue(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::DoubleArray => {
                match get_array_from_input(input.to_owned()) {
                    Ok(value) => Ok(root::proto::v1::datapoint::Value::DoubleArray(
                        root::proto::v1::DoubleArray { values: value },
                    )),
                    Err(err) => Err(err),
                }
            }
            _ => Err(ParseError {}),
        }
        proto::v1::DataType::StringArray => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::StringArray(
                databroker_proto::sdv::databroker::v1::StringArray { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Bool => match input.parse::<bool>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::BoolValue(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::BoolArray => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::BoolArray(
                databroker_proto::sdv::databroker::v1::BoolArray { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Int8 => match input.parse::<i8>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int32Value(value as i32)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Int8Array => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int32Array(
                databroker_proto::sdv::databroker::v1::Int32Array { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Int16 => match input.parse::<i16>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int32Value(value as i32)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Int16Array => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int32Array(
                databroker_proto::sdv::databroker::v1::Int32Array { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Int32 => match input.parse::<i32>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int32Value(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Int32Array => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int32Array(
                databroker_proto::sdv::databroker::v1::Int32Array { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Int64 => match input.parse::<i64>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int64Value(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Int64Array => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int64Array(
                databroker_proto::sdv::databroker::v1::Int64Array { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Uint8 => match input.parse::<u8>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint32Value(value as u32)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Uint8Array => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint32Array(
                databroker_proto::sdv::databroker::v1::Uint32Array { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Uint16 => match input.parse::<u16>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint32Value(value as u32)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Uint16Array => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint32Array(
                databroker_proto::sdv::databroker::v1::Uint32Array { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Uint32 => match input.parse::<u32>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint32Value(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Uint32Array => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint32Array(
                databroker_proto::sdv::databroker::v1::Uint32Array { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Uint64 => match input.parse::<u64>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint64Value(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Uint64Array => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint64Array(
                databroker_proto::sdv::databroker::v1::Uint64Array { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Float => match input.parse::<f32>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::FloatValue(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::FloatArray => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::FloatArray(
                databroker_proto::sdv::databroker::v1::FloatArray { values: value },
            )),
            Err(err) => Err(err),
        },
        proto::v1::DataType::Double => match input.parse::<f64>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::DoubleValue(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::DoubleArray => match get_array_from_input(input.to_owned()) {
            Ok(value) => Ok(proto::v1::datapoint::Value::DoubleArray(
                databroker_proto::sdv::databroker::v1::DoubleArray { values: value },
            )),
            Err(err) => Err(err),
        },
    }
    #[cfg(feature = "feature_kuksa")]
    {
        match data_type {
            root::proto::v1::DataType::String => {
                Ok(root::proto::v1::datapoint::Value::String(input.to_owned()))
            }
            root::proto::v1::DataType::StringArray => {
                match get_array_from_input(input.to_owned()) {
                    Ok(value) => Ok(root::proto::v1::datapoint::Value::StringArray(
                        root::proto::v1::StringArray { values: value },
                    )),
                    Err(err) => Err(err),
                }
            }
            root::proto::v1::DataType::Boolean => match input.parse::<bool>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Bool(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::BooleanArray => match get_array_from_input(input.to_owned())
            {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::BoolArray(
                    root::proto::v1::BoolArray { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Int8 => match input.parse::<i8>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32(value as i32)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Int8Array => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32Array(
                    root::proto::v1::Int32Array { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Int16 => match input.parse::<i16>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32(value as i32)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Int16Array => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32Array(
                    root::proto::v1::Int32Array { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Int32 => match input.parse::<i32>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Int32Array => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int32Array(
                    root::proto::v1::Int32Array { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Int64 => match input.parse::<i64>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int64(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Int64Array => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Int64Array(
                    root::proto::v1::Int64Array { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Uint8 => match input.parse::<u8>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32(value as u32)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Uint8Array => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32Array(
                    root::proto::v1::Uint32Array { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Uint16 => match input.parse::<u16>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32(value as u32)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Uint16Array => {
                match get_array_from_input(input.to_owned()) {
                    Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32Array(
                        root::proto::v1::Uint32Array { values: value },
                    )),
                    Err(err) => Err(err),
                }
            }
            root::proto::v1::DataType::Uint32 => match input.parse::<u32>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Uint32Array => {
                match get_array_from_input(input.to_owned()) {
                    Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint32Array(
                        root::proto::v1::Uint32Array { values: value },
                    )),
                    Err(err) => Err(err),
                }
            }
            root::proto::v1::DataType::Uint64 => match input.parse::<u64>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint64(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::Uint64Array => {
                match get_array_from_input(input.to_owned()) {
                    Ok(value) => Ok(root::proto::v1::datapoint::Value::Uint64Array(
                        root::proto::v1::Uint64Array { values: value },
                    )),
                    Err(err) => Err(err),
                }
            }
            root::proto::v1::DataType::Float => match input.parse::<f32>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Float(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::FloatArray => match get_array_from_input(input.to_owned()) {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::FloatArray(
                    root::proto::v1::FloatArray { values: value },
                )),
                Err(err) => Err(err),
            },
            root::proto::v1::DataType::Double => match input.parse::<f64>() {
                Ok(value) => Ok(root::proto::v1::datapoint::Value::Double(value)),
                Err(_) => Err(ParseError {}),
            },
            root::proto::v1::DataType::DoubleArray => {
                match get_array_from_input(input.to_owned()) {
                    Ok(value) => Ok(root::proto::v1::datapoint::Value::DoubleArray(
                        root::proto::v1::DoubleArray { values: value },
                    )),
                    Err(err) => Err(err),
                }
            }
            _ => Err(ParseError {}),
        }
    }
}

#[cfg(test)]
mod test {

    use super::*;

    #[test]
    fn test_parse_values() {
        #[cfg(feature = "feature_sdv")]
        {
            // String
            assert!(matches!(
                try_into_data_value("test", root::proto::v1::DataType::String),
                Ok(root::proto::v1::datapoint::Value::StringValue(value)) if value == "test"
            ));

            // StringArray
            assert!(matches!(
                try_into_data_value("[test, test2, test4]", root::proto::v1::DataType::StringArray),
                Ok(root::proto::v1::datapoint::Value::StringArray(value)) if value == root::proto::v1::StringArray{values: vec!["test".to_string(), "test2".to_string(), "test4".to_string()]}
            ));

            // Bool
            assert!(matches!(
                try_into_data_value("true", root::proto::v1::DataType::Bool),
                Ok(root::proto::v1::datapoint::Value::BoolValue(value)) if value
            ));

            assert!(matches!(
                try_into_data_value("false", root::proto::v1::DataType::Bool),
                Ok(root::proto::v1::datapoint::Value::BoolValue(value)) if !value
            ));
            assert!(try_into_data_value("truefalse", root::proto::v1::DataType::Bool).is_err());
            // BoolArray
            assert!(matches!(
                try_into_data_value("[true, false, true]", root::proto::v1::DataType::BoolArray),
                Ok(root::proto::v1::datapoint::Value::BoolArray(value)) if value == root::proto::v1::BoolArray{values: vec![true, false, true]}
            ));

            // Int8
            assert!(matches!(
                try_into_data_value("100", root::proto::v1::DataType::Int8),
                Ok(root::proto::v1::datapoint::Value::Int32Value(value)) if value == 100
            ));
            assert!(matches!(
                try_into_data_value("-100", root::proto::v1::DataType::Int8),
                Ok(root::proto::v1::datapoint::Value::Int32Value(value)) if value == -100
            ));
            assert!(try_into_data_value("300", root::proto::v1::DataType::Int8).is_err());
            assert!(try_into_data_value("-300", root::proto::v1::DataType::Int8).is_err());
            assert!(try_into_data_value("-100.1", root::proto::v1::DataType::Int8).is_err());

            // Int16
            assert!(matches!(
                try_into_data_value("100", root::proto::v1::DataType::Int16),
                Ok(root::proto::v1::datapoint::Value::Int32Value(value)) if value == 100
            ));
            assert!(matches!(
                try_into_data_value("-100", root::proto::v1::DataType::Int16),
                Ok(root::proto::v1::datapoint::Value::Int32Value(value)) if value == -100
            ));
            assert!(matches!(
                try_into_data_value("32000", root::proto::v1::DataType::Int16),
                Ok(root::proto::v1::datapoint::Value::Int32Value(value)) if value == 32000
            ));
            assert!(matches!(
                try_into_data_value("-32000", root::proto::v1::DataType::Int16),
                Ok(root::proto::v1::datapoint::Value::Int32Value(value)) if value == -32000
            ));
            assert!(try_into_data_value("33000", root::proto::v1::DataType::Int16).is_err());
            assert!(try_into_data_value("-33000", root::proto::v1::DataType::Int16).is_err());
            assert!(try_into_data_value("-32000.1", root::proto::v1::DataType::Int16).is_err());
        }
        #[cfg(feature = "feature_kuksa")]
        {
            // String
            assert!(matches!(
                try_into_data_value("test", root::proto::v1::DataType::String),
                Ok(root::proto::v1::datapoint::Value::String(value)) if value == "test"
            ));

            // StringArray
            assert!(matches!(
                try_into_data_value("[test, test2, test4]", root::proto::v1::DataType::StringArray),
                Ok(root::proto::v1::datapoint::Value::StringArray(value)) if value == root::proto::v1::StringArray{values: vec!["test".to_string(), "test2".to_string(), "test4".to_string()]}
            ));

            // Bool
            assert!(matches!(
                try_into_data_value("true", root::proto::v1::DataType::Boolean),
                Ok(root::proto::v1::datapoint::Value::Bool(value)) if value
            ));

            assert!(matches!(
                try_into_data_value("false", root::proto::v1::DataType::Boolean),
                Ok(root::proto::v1::datapoint::Value::Bool(value)) if !value
            ));
            assert!(try_into_data_value("truefalse", root::proto::v1::DataType::Boolean).is_err());
            // BoolArray
            assert!(matches!(
                try_into_data_value("[true, false, true]", root::proto::v1::DataType::BooleanArray),
                Ok(root::proto::v1::datapoint::Value::BoolArray(value)) if value == root::proto::v1::BoolArray{values: vec![true, false, true]}
            ));

            // Int8
            assert!(matches!(
                try_into_data_value("100", root::proto::v1::DataType::Int8),
                Ok(root::proto::v1::datapoint::Value::Int32(value)) if value == 100
            ));
            assert!(matches!(
                try_into_data_value("-100", root::proto::v1::DataType::Int8),
                Ok(root::proto::v1::datapoint::Value::Int32(value)) if value == -100
            ));
            assert!(try_into_data_value("300", root::proto::v1::DataType::Int8).is_err());
            assert!(try_into_data_value("-300", root::proto::v1::DataType::Int8).is_err());
            assert!(try_into_data_value("-100.1", root::proto::v1::DataType::Int8).is_err());

            // Int16
            assert!(matches!(
                try_into_data_value("100", root::proto::v1::DataType::Int16),
                Ok(root::proto::v1::datapoint::Value::Int32(value)) if value == 100
            ));
            assert!(matches!(
                try_into_data_value("-100", root::proto::v1::DataType::Int16),
                Ok(root::proto::v1::datapoint::Value::Int32(value)) if value == -100
            ));
            assert!(matches!(
                try_into_data_value("32000", root::proto::v1::DataType::Int16),
                Ok(root::proto::v1::datapoint::Value::Int32(value)) if value == 32000
            ));
            assert!(matches!(
                try_into_data_value("-32000", root::proto::v1::DataType::Int16),
                Ok(root::proto::v1::datapoint::Value::Int32(value)) if value == -32000
            ));
            assert!(try_into_data_value("33000", root::proto::v1::DataType::Int16).is_err());
            assert!(try_into_data_value("-33000", root::proto::v1::DataType::Int16).is_err());
            assert!(try_into_data_value("-32000.1", root::proto::v1::DataType::Int16).is_err());
        }
    }

    #[test]
    fn test_entry_path_completion() {
        #[allow(unused_mut, unused_assignments)]
        let mut metadata = Vec::new();
        #[cfg(feature = "feature_sdv")]
        {
            metadata = [
                root::proto::v1::Metadata {
                    id: 1,
                    name: "Vehicle.Test.Test1".into(),
                    data_type: root::proto::v1::DataType::Bool.into(),
                    entry_type: root::proto::v1::EntryType::Sensor.into(),
                    change_type: root::proto::v1::ChangeType::OnChange.into(),
                    description: "".into(),
                },
                root::proto::v1::Metadata {
                    id: 2,
                    name: "Vehicle.AnotherTest.AnotherTest1".into(),
                    data_type: root::proto::v1::DataType::Bool.into(),
                    entry_type: root::proto::v1::EntryType::Sensor.into(),
                    change_type: root::proto::v1::ChangeType::OnChange.into(),
                    description: "".into(),
                },
                root::proto::v1::Metadata {
                    id: 3,
                    name: "Vehicle.AnotherTest.AnotherTest2".into(),
                    data_type: root::proto::v1::DataType::Bool.into(),
                    entry_type: root::proto::v1::EntryType::Sensor.into(),
                    change_type: root::proto::v1::ChangeType::OnChange.into(),
                    description: "".into(),
                },
            ]
            .to_vec();
        }

        #[cfg(feature = "feature_kuksa")]
        {
            metadata.push(root::proto::v1::DataEntry {
                path: "Vehicle.Test.Test1".into(),
                value: None,
                actuator_target: None,
                metadata: Some(root::proto::v1::Metadata {
                    data_type: root::proto::v1::DataType::Boolean.into(),
                    entry_type: root::proto::v1::EntryType::Sensor.into(),
                    comment: None,
                    deprecation: None,
                    unit: None,
                    value_restriction: None,
                    entry_specific: None,
                    description: Some("".to_string()),
                }),
            });
            metadata.push(root::proto::v1::DataEntry {
                path: "Vehicle.Test.AnotherTest1".into(),
                value: None,
                actuator_target: None,
                metadata: Some(root::proto::v1::Metadata {
                    data_type: root::proto::v1::DataType::Boolean.into(),
                    entry_type: root::proto::v1::EntryType::Sensor.into(),
                    comment: None,
                    deprecation: None,
                    unit: None,
                    value_restriction: None,
                    entry_specific: None,
                    description: Some("".to_string()),
                }),
            });
            metadata.push(root::proto::v1::DataEntry {
                path: "Vehicle.Test.AnotherTest2".into(),
                value: None,
                actuator_target: None,
                metadata: Some(root::proto::v1::Metadata {
                    data_type: root::proto::v1::DataType::Boolean.into(),
                    entry_type: root::proto::v1::EntryType::Sensor.into(),
                    comment: None,
                    deprecation: None,
                    unit: None,
                    value_restriction: None,
                    entry_specific: None,
                    description: Some("".to_string()),
                }),
            });
        }

        let completer = CliCompleter::from_metadata(&metadata);

        assert_eq!(completer.paths.children.len(), 1);
        assert_eq!(completer.paths.children["vehicle"].children.len(), 2);

        match completer.complete_entry_path("") {
            Some(completions) => {
                assert_eq!(completions.len(), 1);
                assert_eq!(completions[0].display(), "Vehicle.");
            }
            None => panic!("expected completions, got None"),
        }

        match completer.complete_entry_path("v") {
            Some(completions) => {
                assert_eq!(completions.len(), 1);
                assert_eq!(completions[0].display(), "Vehicle.");
            }
            None => panic!("expected completions, got None"),
        }

        match completer.complete_entry_path("vehicle.") {
            Some(completions) => {
                assert_eq!(completions.len(), 2);
                assert_eq!(completions[0].display(), "AnotherTest.");
                assert_eq!(completions[1].display(), "Test.");
            }
            None => panic!("expected completions, got None"),
        }

        match completer.complete_entry_path("vehicle") {
            Some(completions) => {
                assert_eq!(completions.len(), 2);
                assert_eq!(completions[0].display(), "AnotherTest.");
                assert_eq!(completions[1].display(), "Test.");
            }
            None => panic!("expected completions, got None"),
        }
    }

    #[test]
    fn test_alignment() {
        let max = 7;
        assert_eq!("hej     1    4", format!("{:<max$} {:<4} {}", "hej", 1, 4));
    }
}
