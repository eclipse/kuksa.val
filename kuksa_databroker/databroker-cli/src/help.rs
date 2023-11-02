fn set_connected_prompt(interface: &Arc<Interface<DefaultTerminal>>, text: str) {
    let mut _text;
    _text = text;
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

#[allow(dead_code)]
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

#[allow(dead_code)]
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
