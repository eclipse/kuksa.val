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

extern crate ansi_term;
extern crate linefeed;

use databroker_proto::sdv::databroker as proto;
use prost_types::Timestamp;

use std::collections::HashMap;
use std::sync::Arc;
use std::time::{Duration, SystemTime};
use std::{fmt, io};

use ansi_term::Color;

use linefeed::complete::{Completer, Completion, Suffix};
use linefeed::terminal::Terminal;
use linefeed::{Command, DefaultTerminal, Function, Interface, Prompter, ReadResult};

const TIMEOUT: Duration = Duration::from_millis(500);

const CLI_COMMANDS: &[(&str, &str)] = &[
    ("get", "Get a HAL property"),
    ("set", "Set a HAL property"),
    ("feed", "Set a HAL property (from vehicle side)"),
    ("connect", "Connect"),
    ("subscribe", "Subscribe to properties"),
    ("metadata", "Get datapoint metadata"),
    ("help", "You're looking at it"),
    ("quit", "Quit the demo"),
];

static APP_NAME: &str = "client";

fn set_connected_prompt(interface: &Arc<Interface<DefaultTerminal>>) {
    let connected_prompt = format!(
        "\x01{prefix}\x02{text}\x01{suffix}\x02> ",
        prefix = Color::Green.bold().prefix(),
        text = APP_NAME,
        suffix = Color::Green.bold().suffix()
    );
    interface.set_prompt(&connected_prompt).unwrap();
}

fn set_disconnected_prompt(interface: &Arc<Interface<DefaultTerminal>>) {
    let disconnected_prompt = format!(
        "\x01{prefix}\x02{text}\x01{suffix}\x02> ",
        prefix = Color::Red.bold().prefix(),
        text = APP_NAME,
        suffix = Color::Red.bold().suffix()
    );
    interface.set_prompt(&disconnected_prompt).unwrap();
}

fn addr_to_uri(addr: impl AsRef<str>) -> Option<tonic::transport::Uri> {
    match addr.as_ref().parse::<tonic::transport::Uri>() {
        Ok(uri) => {
            let mut parts = uri.into_parts();

            if parts.scheme.is_none() {
                parts.scheme = Some("http".parse().unwrap());
            }
            if parts.path_and_query.is_none() {
                parts.path_and_query = Some("".parse().unwrap());
            }
            Some(tonic::transport::Uri::from_parts(parts).unwrap())
        }
        Err(_) => None,
    }
}

async fn connect(
    uri: &tonic::transport::Uri,
    interface: &Arc<Interface<DefaultTerminal>>,
) -> Option<tonic::transport::Channel> {
    match tonic::transport::Channel::builder(uri.clone())
        .connect()
        .await
    {
        Ok(connection) => {
            set_connected_prompt(interface);
            Some(connection)
        }
        Err(err) => {
            set_disconnected_prompt(interface);
            writeln!(interface, "{err}").unwrap();
            None
        }
    }
}

async fn get_metadata(channel: &tonic::transport::Channel) -> Option<Vec<proto::v1::Metadata>> {
    let mut client = proto::v1::broker_client::BrokerClient::new(channel.clone());
    // Empty vec == all property metadata
    let args = tonic::Request::new(proto::v1::GetMetadataRequest { names: Vec::new() });
    match client.get_metadata(args).await {
        Ok(response) => {
            let message = response.into_inner();
            Some(message.list)
        }
        Err(err) => {
            println!("-> status: {}", code_to_text(&err.code()));
            println!("{}", err.message());
            None
        }
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut properties = Vec::<proto::v1::Metadata>::new();
    let mut subscription_nbr = 1;

    let cli = CliCompleter::new();
    let interface = Arc::new(Interface::new("client")?);
    interface.set_completer(Arc::new(cli));

    interface.define_function("enter-function", Arc::new(EnterFunction));
    interface.bind_sequence("\r", Command::from_str("enter-function"));
    interface.bind_sequence("\n", Command::from_str("enter-function"));

    let addr = std::env::var("KUKSA_DATA_BROKER_ADDR").unwrap_or_else(|_| "127.0.0.1".to_owned());
    let port = std::env::var("KUKSA_DATA_BROKER_PORT").unwrap_or_else(|_| "55555".to_owned());
    let mut uri = match addr_to_uri(format!("{addr}:{port}")) {
        Some(uri) => uri,
        None => return Err(Box::new(ParseError {}).into()),
    };

    let mut channel = connect(&uri, &interface).await;

    if let Some(connection) = &channel {
        if let Some(metadata) = get_metadata(connection).await {
            interface.set_completer(Arc::new(CliCompleter::from_metadata(&metadata)));
            properties = metadata;
        }
    }

    loop {
        if let Some(res) = interface.read_line_step(Some(TIMEOUT))? {
            match res {
                ReadResult::Input(line) => {
                    // println!("input: {:?}", line);
                    let (cmd, args) = split_first_word(&line);
                    match cmd {
                        "help" => {
                            println!("{APP_NAME} commands:");
                            println!();
                            for &(cmd, help) in CLI_COMMANDS {
                                println!("  {cmd:15} - {help}");
                            }
                            println!();
                        }
                        "get" => {
                            interface.add_history_unique(line.clone());

                            if args.is_empty() {
                                println!("Usage: get PROPERTY_NAME");
                                continue;
                            }

                            match &channel {
                                Some(channel) => {
                                    let mut client = proto::v1::broker_client::BrokerClient::new(
                                        channel.clone(),
                                    );
                                    let args =
                                        tonic::Request::new(proto::v1::GetDatapointsRequest {
                                            datapoints: vec![args.to_owned()],
                                        });
                                    match client.get_datapoints(args).await {
                                        Ok(response) => {
                                            let message = response.into_inner();
                                            for (name, datapoint) in message.datapoints {
                                                println!(
                                                    "-> {}: {}",
                                                    name,
                                                    DisplayDatapoint(datapoint)
                                                )
                                            }
                                            set_connected_prompt(&interface);
                                        }
                                        Err(err) => {
                                            println!("-> status: {}", code_to_text(&err.code()));
                                            println!("{}", err.message());
                                            // interface.set_prompt(&disconnected_prompt)?;
                                        }
                                    }
                                }
                                None => println!("Not connected"),
                            }
                        }

                        "set" => {
                            interface.add_history_unique(line.clone());

                            let (prop, value) = split_first_word(args);

                            if value.is_empty() {
                                println!("Usage: set ID VALUE");
                                continue;
                            }

                            let datapoint_metadata = {
                                let mut datapoint_metadata = None;
                                for metadata in properties.iter() {
                                    if metadata.name == prop {
                                        datapoint_metadata = Some(metadata)
                                    }
                                }
                                datapoint_metadata
                            };

                            if datapoint_metadata.is_none() {
                                println!("Could not map name to id");
                                continue;
                            }

                            if let Some(metadata) = datapoint_metadata {
                                let data_value = try_into_data_value(
                                    value,
                                    proto::v1::DataType::from_i32(metadata.data_type).unwrap(),
                                );
                                if data_value.is_err() {
                                    println!(
                                        "Could not parse \"{}\" as {:?}",
                                        value,
                                        proto::v1::DataType::from_i32(metadata.data_type).unwrap()
                                    );
                                    continue;
                                }
                                match &channel {
                                    Some(channel) => {
                                        let mut client =
                                            proto::v1::broker_client::BrokerClient::new(
                                                channel.clone(),
                                            );
                                        let ts = Timestamp::from(SystemTime::now());
                                        let datapoints = HashMap::from([(
                                            metadata.name.clone(),
                                            proto::v1::Datapoint {
                                                timestamp: Some(ts),
                                                value: Some(data_value.unwrap()),
                                            },
                                        )]);
                                        let request =
                                            tonic::Request::new(proto::v1::SetDatapointsRequest {
                                                datapoints,
                                            });

                                        match client.set_datapoints(request).await {
                                            Ok(response) => {
                                                let message = response.into_inner();
                                                if message.errors.is_empty() {
                                                    println!("-> Ok")
                                                } else {
                                                    for (id, error) in message.errors {
                                                        match proto::v1::DatapointError::from_i32(
                                                            error,
                                                        ) {
                                                            Some(error) => {
                                                                println!(
                                                                    "-> Error setting id {id}: {error:?}"
                                                                )
                                                            }
                                                            None => {
                                                                println!("-> Error setting id {id}")
                                                            }
                                                        }
                                                    }
                                                }

                                                // interface.set_prompt(&connected_prompt)?;
                                            }
                                            Err(err) => {
                                                println!(
                                                    "-> status: {}",
                                                    code_to_text(&err.code())
                                                );
                                                println!("{}", err.message());
                                                // interface.set_prompt(&disconnected_prompt)?;
                                            }
                                        }
                                    }
                                    None => println!("Not connected"),
                                }
                            }
                        }
                        "feed" => {
                            interface.add_history_unique(line.clone());

                            let (prop, value) = split_first_word(args);

                            if value.is_empty() {
                                println!("Usage: feed ID VALUE");
                                continue;
                            }

                            let datapoint_metadata = {
                                let mut datapoint_metadata = None;
                                for metadata in properties.iter() {
                                    if metadata.name == prop {
                                        datapoint_metadata = Some(metadata)
                                    }
                                }
                                datapoint_metadata
                            };

                            if datapoint_metadata.is_none() {
                                println!("Could not map name to id");
                                continue;
                            }

                            if let Some(metadata) = datapoint_metadata {
                                let data_value = try_into_data_value(
                                    value,
                                    proto::v1::DataType::from_i32(metadata.data_type).unwrap(),
                                );
                                if data_value.is_err() {
                                    println!(
                                        "Could not parse \"{}\" as {:?}",
                                        value,
                                        proto::v1::DataType::from_i32(metadata.data_type).unwrap()
                                    );
                                    continue;
                                }
                                match &channel {
                                    Some(channel) => {
                                        let mut client =
                                            proto::v1::collector_client::CollectorClient::new(
                                                channel.clone(),
                                            );
                                        let ts = Timestamp::from(SystemTime::now());
                                        let datapoints = HashMap::from([(
                                            metadata.id,
                                            proto::v1::Datapoint {
                                                timestamp: Some(ts),
                                                value: Some(data_value.unwrap()),
                                            },
                                        )]);
                                        let request = tonic::Request::new(
                                            proto::v1::UpdateDatapointsRequest { datapoints },
                                        );
                                        match client.update_datapoints(request).await {
                                            Ok(response) => {
                                                let message = response.into_inner();
                                                if message.errors.is_empty() {
                                                    println!("-> Ok")
                                                } else {
                                                    for (id, error) in message.errors {
                                                        match proto::v1::DatapointError::from_i32(
                                                            error,
                                                        ) {
                                                            Some(error) => {
                                                                println!(
                                                                    "-> Error setting id {id}: {error:?}"
                                                                )
                                                            }
                                                            None => {
                                                                println!("-> Error setting id {id}")
                                                            }
                                                        }
                                                    }
                                                }

                                                // interface.set_prompt(&connected_prompt)?;
                                            }
                                            Err(err) => {
                                                println!(
                                                    "-> status: {}",
                                                    code_to_text(&err.code())
                                                );
                                                println!("{}", err.message());
                                                // interface.set_prompt(&disconnected_prompt)?;
                                            }
                                        }
                                    }
                                    None => println!("Not connected"),
                                }
                            }
                        }
                        "subscribe" => {
                            interface.add_history_unique(line.clone());

                            if args.is_empty() {
                                println!("Usage: subscribe QUERY");
                                continue;
                            }

                            let query = args.to_owned();

                            match &channel {
                                Some(channel) => {
                                    let mut client = proto::v1::broker_client::BrokerClient::new(
                                        channel.clone(),
                                    );
                                    let iface = interface.clone();
                                    let args =
                                        tonic::Request::new(proto::v1::SubscribeRequest { query });
                                    match client.subscribe(args).await {
                                        Ok(response) => {
                                            let sub_id = format!("subscription{subscription_nbr}");
                                            subscription_nbr += 1;
                                            tokio::spawn(async move {
                                                let mut stream = response.into_inner();
                                                loop {
                                                    match stream.message().await {
                                                        Ok(subscribe_resp) => {
                                                            if let Some(resp) = subscribe_resp {
                                                                // Build output before writing it
                                                                // (to avoid interleaving confusion)
                                                                use std::fmt::Write;
                                                                let mut output = String::new();
                                                                for (name, value) in resp.fields {
                                                                    writeln!(
                                                                        output,
                                                                        "{}: {}",
                                                                        name,
                                                                        DisplayDatapoint(value)
                                                                    )
                                                                    .unwrap();
                                                                }
                                                                writeln!(
                                                                    iface,
                                                                    "-> {sub_id}:\n{output}"
                                                                )
                                                                .unwrap();
                                                            }
                                                        }
                                                        Err(_) => {
                                                            writeln!(
                                                                iface,
                                                                "Server gone. Subscription stopped"
                                                            )
                                                            .unwrap();
                                                            break;
                                                        }
                                                    }
                                                }
                                            });
                                            println!("-> status: OK")
                                        }
                                        Err(err) => {
                                            println!("-> status: {}", code_to_text(&err.code()));
                                            println!("{}", err.message());
                                            // interface.set_prompt(&disconnected_prompt)?;
                                        }
                                    }
                                }
                                None => println!("Not connected"),
                            }
                            // }
                        }
                        "connect" => {
                            interface.add_history_unique(line.clone());
                            if channel.is_none() || !args.is_empty() {
                                if args.is_empty() {
                                    channel = connect(&uri, &interface).await
                                } else {
                                    match addr_to_uri(args) {
                                        Some(valid_uri) => {
                                            uri = valid_uri;
                                            channel = connect(&uri, &interface).await
                                        }
                                        None => println!("Failed to parse endpoint address"),
                                    }
                                };
                                if let Some(connection) = &channel {
                                    if let Some(metadata) = get_metadata(connection).await {
                                        interface.set_completer(Arc::new(
                                            CliCompleter::from_metadata(&metadata),
                                        ));
                                        properties = metadata;
                                    }
                                }
                            };
                        }
                        "metadata" => {
                            interface.add_history_unique(line.clone());
                            match &channel {
                                Some(channel) => {
                                    if let Some(metadata) = get_metadata(channel).await {
                                        for entry in &metadata {
                                            println!(
                                                "{}, data_type: {}",
                                                entry.name, entry.data_type
                                            );
                                        }
                                        properties = metadata;
                                        interface.set_completer(Arc::new(
                                            CliCompleter::from_metadata(&properties),
                                        ));
                                    }
                                }
                                None => println!("Not connected"),
                            }
                        }
                        "quit" => {
                            println!("Bye bye!");
                            break;
                        }
                        "" => {} // Ignore empty input
                        _ => {
                            println!("unknown command");
                            interface.add_history_unique(line.clone());
                        }
                    }
                }
                ReadResult::Eof => {
                    println!("Bye bye!");
                    break;
                }
                ReadResult::Signal(sig) => {
                    println!("received signal: {sig:?}");
                }
            }
        }

        // interface.set_prompt("disconnected> ")?;
    }

    Ok(())
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

    fn from_metadata(metadata: &[proto::v1::Metadata]) -> CliCompleter {
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

                for &(cmd, _) in CLI_COMMANDS {
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
            Some("get") | Some("set") | Some("feed") => {
                if words.count() == 0 {
                    self.complete_entry_path(word)
                } else {
                    None
                }
            }
            Some("subscribe") => match words.next() {
                None => Some(vec![Completion::simple("SELECT".to_owned())]),
                Some(next) => {
                    if next == "SELECT" {
                        self.complete_entry_path(word)
                    } else {
                        None
                    }
                }
            },
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
                prompter.accept_input()
            } else if count > 0 {
                prompter.insert(count as usize, '\n')
            } else {
                Ok(())
            }
        } else {
            prompter.accept_input()
        }
    }
}

struct DisplayDatapoint(proto::v1::Datapoint);

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
        match &self.0.value {
            Some(value) => match value {
                proto::v1::datapoint::Value::BoolValue(value) => {
                    f.write_fmt(format_args!("{value}"))
                }
                proto::v1::datapoint::Value::FailureValue(failure) => f.write_fmt(format_args!(
                    "( {:?} )",
                    proto::v1::datapoint::Failure::from_i32(*failure).unwrap()
                )),
                proto::v1::datapoint::Value::Int32Value(value) => {
                    f.write_fmt(format_args!("{value}"))
                }
                proto::v1::datapoint::Value::Int64Value(value) => {
                    f.write_fmt(format_args!("{value}"))
                }
                proto::v1::datapoint::Value::Uint32Value(value) => {
                    f.write_fmt(format_args!("{value}"))
                }
                proto::v1::datapoint::Value::Uint64Value(value) => {
                    f.write_fmt(format_args!("{value}"))
                }
                proto::v1::datapoint::Value::FloatValue(value) => {
                    f.write_fmt(format_args!("{value:.2}"))
                }
                proto::v1::datapoint::Value::DoubleValue(value) => {
                    f.write_fmt(format_args!("{value}"))
                }
                proto::v1::datapoint::Value::StringValue(value) => {
                    f.write_fmt(format_args!("'{value}'"))
                }
                proto::v1::datapoint::Value::StringArray(array) => display_array(f, &array.values),
                proto::v1::datapoint::Value::BoolArray(array) => display_array(f, &array.values),
                proto::v1::datapoint::Value::Int32Array(array) => display_array(f, &array.values),
                proto::v1::datapoint::Value::Int64Array(array) => display_array(f, &array.values),
                proto::v1::datapoint::Value::Uint32Array(array) => display_array(f, &array.values),
                proto::v1::datapoint::Value::Uint64Array(array) => display_array(f, &array.values),
                proto::v1::datapoint::Value::FloatArray(array) => display_array(f, &array.values),
                proto::v1::datapoint::Value::DoubleArray(array) => display_array(f, &array.values),
            },
            None => f.write_fmt(format_args!("None")),
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

fn try_into_data_value(
    input: &str,
    data_type: proto::v1::DataType,
) -> Result<proto::v1::datapoint::Value, ParseError> {
    match data_type {
        proto::v1::DataType::String => {
            Ok(proto::v1::datapoint::Value::StringValue(input.to_owned()))
        }
        proto::v1::DataType::Bool => match input.parse::<bool>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::BoolValue(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Int8 => match input.parse::<i8>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int32Value(value as i32)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Int16 => match input.parse::<i16>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int32Value(value as i32)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Int32 => match input.parse::<i32>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int32Value(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Int64 => match input.parse::<i64>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Int64Value(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Uint8 => match input.parse::<u8>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint32Value(value as u32)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Uint16 => match input.parse::<u16>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint32Value(value as u32)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Uint32 => match input.parse::<u32>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint32Value(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Uint64 => match input.parse::<u64>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::Uint64Value(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Float => match input.parse::<f32>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::FloatValue(value)),
            Err(_) => Err(ParseError {}),
        },
        proto::v1::DataType::Double => match input.parse::<f64>() {
            Ok(value) => Ok(proto::v1::datapoint::Value::DoubleValue(value)),
            Err(_) => Err(ParseError {}),
        },
        _ => Err(ParseError {}),
    }
}

#[test]
fn test_parse_values() {
    // String
    assert!(matches!(
        try_into_data_value("test", proto::v1::DataType::String),
        Ok(proto::v1::datapoint::Value::StringValue(value)) if value == "test"
    ));

    // Bool
    assert!(matches!(
        try_into_data_value("true", proto::v1::DataType::Bool),
        Ok(proto::v1::datapoint::Value::BoolValue(value)) if value
    ));
    assert!(matches!(
        try_into_data_value("false", proto::v1::DataType::Bool),
        Ok(proto::v1::datapoint::Value::BoolValue(value)) if !value
    ));
    assert!(matches!(
        try_into_data_value("truefalse", proto::v1::DataType::Bool),
        Err(_)
    ));

    // Int8
    assert!(matches!(
        try_into_data_value("100", proto::v1::DataType::Int8),
        Ok(proto::v1::datapoint::Value::Int32Value(value)) if value == 100
    ));
    assert!(matches!(
        try_into_data_value("-100", proto::v1::DataType::Int8),
        Ok(proto::v1::datapoint::Value::Int32Value(value)) if value == -100
    ));
    assert!(matches!(
        try_into_data_value("300", proto::v1::DataType::Int8),
        Err(_)
    ));
    assert!(matches!(
        try_into_data_value("-300", proto::v1::DataType::Int8),
        Err(_)
    ));
    assert!(matches!(
        try_into_data_value("-100.1", proto::v1::DataType::Int8),
        Err(_)
    ));

    // Int16
    assert!(matches!(
        try_into_data_value("100", proto::v1::DataType::Int16),
        Ok(proto::v1::datapoint::Value::Int32Value(value)) if value == 100
    ));
    assert!(matches!(
        try_into_data_value("-100", proto::v1::DataType::Int16),
        Ok(proto::v1::datapoint::Value::Int32Value(value)) if value == -100
    ));
    assert!(matches!(
        try_into_data_value("32000", proto::v1::DataType::Int16),
        Ok(proto::v1::datapoint::Value::Int32Value(value)) if value == 32000
    ));
    assert!(matches!(
        try_into_data_value("-32000", proto::v1::DataType::Int16),
        Ok(proto::v1::datapoint::Value::Int32Value(value)) if value == -32000
    ));
    assert!(matches!(
        try_into_data_value("33000", proto::v1::DataType::Int16),
        Err(_)
    ));
    assert!(matches!(
        try_into_data_value("-33000", proto::v1::DataType::Int16),
        Err(_)
    ));
    assert!(matches!(
        try_into_data_value("-32000.1", proto::v1::DataType::Int16),
        Err(_)
    ));
}

#[test]
fn test_entry_path_completion() {
    let metadata = &[
        proto::v1::Metadata {
            id: 1,
            name: "Vehicle.Test.Test1".into(),
            data_type: proto::v1::DataType::Bool as i32,
            change_type: proto::v1::ChangeType::OnChange as i32,
            description: "".into(),
        },
        proto::v1::Metadata {
            id: 2,
            name: "Vehicle.AnotherTest.AnotherTest1".into(),
            data_type: proto::v1::DataType::Bool as i32,
            change_type: proto::v1::ChangeType::OnChange as i32,
            description: "".into(),
        },
        proto::v1::Metadata {
            id: 3,
            name: "Vehicle.AnotherTest.AnotherTest2".into(),
            data_type: proto::v1::DataType::Bool as i32,
            change_type: proto::v1::ChangeType::OnChange as i32,
            description: "".into(),
        },
    ];

    let completer = CliCompleter::from_metadata(metadata);

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
