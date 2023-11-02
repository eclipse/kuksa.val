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

use http::Uri;
use std::io::Write;
use std::sync::Arc;
use std::{fmt, io};

use ansi_term::Color;

use clap::{Parser, Subcommand, ValueEnum};

use linefeed::{DefaultTerminal, Interface, Terminal, Prompter, Function};

#[derive(Debug)]
pub struct ParseError {}

impl std::error::Error for ParseError {}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("parse error")
    }
}

#[derive(Debug, Parser, Clone)]
#[clap(author, version, about, long_about = None)]
pub struct Cli {
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

    #[arg(value_enum)]
    #[clap(long, short = 'p', value_enum, default_value = "sdv-databroker-v1")]
    protocol: CliAPI,

    // Sub command
    #[clap(subcommand)]
    command: Option<Commands>,
}

impl Cli{
    pub fn get_ca_cert(&mut self) -> Option<String>{
        return self.ca_cert.clone();
    }

    pub fn get_token_file(&mut self) -> Option<String>{
        return self.token_file.clone();
    }

    pub fn get_command(&mut self) -> Option<Commands>{
        return self.command.clone();
    }

    pub fn get_server(&mut self) -> String{
        return self.server.clone();
    }

    pub fn get_protocol(&mut self) -> CliAPI{
        return self.protocol;
    }
}

#[derive(Debug, Subcommand, Clone)]
pub enum Commands {
    /// Get one or more datapoint(s)
    Get {
        #[clap(value_name = "PATH")]
        paths: Vec<String>,
    },
    // Subscribe,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum)]
pub enum CliAPI {
    KuksaValV1 = 1,
    SdvDatabrokerV1 = 2,
}

pub fn set_connected_prompt(interface: &Arc<Interface<DefaultTerminal>>, text: String) {
    let _text = text;
    let connected_prompt = format!(
        "\x01{prefix}\x02{text}\x01{suffix}\x02 > ",
        prefix = Color::Green.prefix(),
        text = _text,
        suffix = Color::Green.suffix()
    );
    interface.set_prompt(&connected_prompt).unwrap();
}

pub fn set_disconnected_prompt(interface: &Arc<Interface<DefaultTerminal>>) {
    let disconnected_prompt = format!(
        "\x01{prefix}\x02{text}\x01{suffix}\x02 > ",
        prefix = Color::Red.prefix(),
        text = "not connected",
        suffix = Color::Red.suffix()
    );
    interface.set_prompt(&disconnected_prompt).unwrap();
}

pub fn print_logo(version: impl fmt::Display) {
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

pub fn print_resp_err(operation: impl AsRef<str>, err: &tonic::Status) -> io::Result<()> {
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

pub fn print_resp_err_fmt(operation: impl AsRef<str>, fmt: fmt::Arguments<'_>) -> io::Result<()> {
    let mut stderr = io::stderr().lock();
    let mut stdout = io::stdout().lock();
    write_resp_ok(&mut stderr, operation)?;
    stdout.write_fmt(fmt)?;
    stdout.write_all(b"\n")?;
    stdout.flush()
}

#[allow(dead_code)]
pub fn print_resp_ok_fmt(operation: impl AsRef<str>, fmt: fmt::Arguments<'_>) -> io::Result<()> {
    let mut stderr = io::stderr().lock();
    let mut stdout = io::stdout().lock();
    write_resp_ok(&mut stderr, operation)?;
    stdout.write_fmt(fmt)?;
    stdout.write_all(b"\n")?;
    stdout.flush()
}

pub fn print_resp_ok(operation: impl AsRef<str>) -> io::Result<()> {
    let mut output = io::stderr().lock();
    write_resp_ok(&mut output, operation)?;
    output.write_all(b"\n")?;
    output.flush()
}

pub fn write_resp_ok(output: &mut impl Write, operation: impl AsRef<str>) -> io::Result<()> {
    output.write_fmt(format_args!(
        "{} {} ",
        Color::White
            .dimmed()
            .paint(format!("[{}]", operation.as_ref())),
        Color::Black.on(Color::Green).paint(" OK "),
    ))
}

pub fn print_info(info: impl AsRef<str>) -> io::Result<()> {
    let mut output = io::stderr().lock();
    output.write_fmt(format_args!(
        "{}\n",
        Color::White.dimmed().paint(info.as_ref()),
    ))?;
    output.flush()
}

pub fn print_error(operation: impl AsRef<str>, msg: impl AsRef<str>) -> io::Result<()> {
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

pub fn split_first_word(s: &str) -> (&str, &str) {
    let s = s.trim();

    match s.find(|ch: char| ch.is_whitespace()) {
        Some(pos) => (&s[..pos], s[pos..].trim_start()),
        None => (s, ""),
    }
}

pub fn code_to_text(code: &tonic::Code) -> &str {
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

pub fn get_array_from_input<T: std::str::FromStr>(values: String) -> Result<Vec<T>, ParseError> {
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

pub struct EnterFunction;

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

pub fn to_uri(uri: impl AsRef<str>) -> Result<Uri, String> {
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
