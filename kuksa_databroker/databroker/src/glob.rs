/********************************************************************************
* Copyright (c) 2023 Contributors to the Eclipse Foundation
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

use lazy_static::lazy_static;
use regex::Regex;

#[derive(Debug)]
pub enum Error {
    RegexError,
}

pub fn to_regex_string(glob: &str) -> String {
    // Construct regular expression

    // Make sure we match the whole thing
    // Start from the beginning
    let mut re = String::from("^");

    let mut regex_string = glob.replace('.', "\\.");

    if glob.starts_with("**.") {
        regex_string = regex_string.replace("**\\.", ".*\\.");
    } else if glob.starts_with("*.") {
        regex_string = regex_string.replace("*\\.", "[A-Z][a-zA-Z0-9]*\\.");
    }

    if glob.ends_with(".*") {
        regex_string = regex_string.replace("\\.*", "\\.[A-Z][a-zA-Z0-9]*");
    } else if glob.ends_with(".**") {
        regex_string = regex_string.replace("\\.**", "\\..*");
    }

    if glob.contains(".**.") {
        regex_string = regex_string.replace("\\.**", "[\\.[A-Z][a-zA-Z0-9]*]*");
    }

    if glob.contains(".*.") {
        regex_string = regex_string.replace("\\.*", "\\.[A-Z][a-zA-Z0-9]*");
    }

    if !glob.contains('*') {
        regex_string += "\\..*";
    }

    re.push_str(regex_string.as_str());

    // And finally, make sure we match until EOL
    re.push('$');

    re
}

pub fn to_regex(glob: &str) -> Result<Regex, Error> {
    let re = to_regex_string(glob);
    Regex::new(&re).map_err(|_err| Error::RegexError)
}

lazy_static! {
    static ref REGEX_VALID_PATTERN: regex::Regex = regex::Regex::new(
        r"(?x)
        ^  # anchor at start (only match full paths)
        # At least one subpath (consisting of either of three):
        (?:
            [A-Z][a-zA-Z0-9]*  # An alphanumeric name (first letter capitalized)
            |
            \*                 # An asterisk
            |
            \*\*               # A double asterisk
        )
        # which can be followed by ( separator + another subpath )
        # repeated any number of times
        (?:
            \. # Separator, literal dot
            (?:
                [A-Z][a-zA-Z0-9]*  # Alphanumeric name (first letter capitalized)
                |
                \*                 # An asterisk
                |
                \*\*               # A double asterisk
            )
        )*
        $  # anchor at end (to match only full paths)
        ",
    )
    .expect("regex compilation (of static pattern) should always succeed");
}

pub fn is_valid_pattern(input: &str) -> bool {
    REGEX_VALID_PATTERN.is_match(input)
}

#[tokio::test]
async fn test_valid_request_path() {
    assert!(is_valid_pattern("String.*"));
    assert!(is_valid_pattern("String.**"));
    assert!(is_valid_pattern("Vehicle.Chassis.Axle.Row2.Wheel.*"));
    assert!(is_valid_pattern("String.String.String.String.*"));
    assert!(is_valid_pattern("String.String.String.String.**"));
    assert!(is_valid_pattern("String.String.String.String"));
    assert!(is_valid_pattern("String.String.String.String.String.**"));
    assert!(is_valid_pattern("String.String.String.*.String"));
    assert!(is_valid_pattern("String.String.String.**.String"));
    assert!(is_valid_pattern(
        "String.String.String.String.String.**.String"
    ));
    assert!(is_valid_pattern(
        "String.String.String.String.*.String.String"
    ));

    assert!(is_valid_pattern("String.*.String.String"));
    assert!(is_valid_pattern("String.String.**.String.String"));
    assert!(is_valid_pattern("String.**.String.String"));
    assert!(is_valid_pattern("**.String.String.String.**"));
    assert!(is_valid_pattern("**.String.String.String.*"));
    assert!(is_valid_pattern("**.String"));
    assert!(is_valid_pattern("*.String.String.String"));
    assert!(is_valid_pattern("*.String"));
    assert!(!is_valid_pattern("String.String.String."));
    assert!(!is_valid_pattern("String.String.String.String.."));
    assert!(!is_valid_pattern("String.*.String.String.."));
    assert!(!is_valid_pattern("*.String.String.String.."));
}

#[tokio::test]
async fn test_valid_regex_path() {
    assert_eq!(to_regex_string("String.*"), "^String\\.[A-Z][a-zA-Z0-9]*$");
    assert_eq!(to_regex_string("String.**"), "^String\\..*$");
    assert_eq!(
        to_regex_string("String.String.String.String.*"),
        "^String\\.String\\.String\\.String\\.[A-Z][a-zA-Z0-9]*$"
    );
    assert_eq!(
        to_regex_string("String.String.String.String.**"),
        "^String\\.String\\.String\\.String\\..*$"
    );
    assert_eq!(
        to_regex_string("String.String.String.String"),
        "^String\\.String\\.String\\.String\\..*$"
    );
    assert_eq!(
        to_regex_string("String.String.String.String.String.**"),
        "^String\\.String\\.String\\.String\\.String\\..*$"
    );
    assert_eq!(
        to_regex_string("String.String.String.*.String"),
        "^String\\.String\\.String\\.[A-Z][a-zA-Z0-9]*\\.String$"
    );
    assert_eq!(
        to_regex_string("String.String.String.**.String"),
        "^String\\.String\\.String[\\.[A-Z][a-zA-Z0-9]*]*\\.String$"
    );
    assert_eq!(
        to_regex_string("String.String.String.String.String.**.String"),
        "^String\\.String\\.String\\.String\\.String[\\.[A-Z][a-zA-Z0-9]*]*\\.String$"
    );
    assert_eq!(
        to_regex_string("String.String.String.String.*.String.String"),
        "^String\\.String\\.String\\.String\\.[A-Z][a-zA-Z0-9]*\\.String\\.String$"
    );
    assert_eq!(
        to_regex_string("String.*.String.String"),
        "^String\\.[A-Z][a-zA-Z0-9]*\\.String\\.String$"
    );
    assert_eq!(
        to_regex_string("String.String.**.String.String"),
        "^String\\.String[\\.[A-Z][a-zA-Z0-9]*]*\\.String\\.String$"
    );
    assert_eq!(
        to_regex_string("String.**.String.String"),
        "^String[\\.[A-Z][a-zA-Z0-9]*]*\\.String\\.String$"
    );
    assert_eq!(
        to_regex_string("**.String.String.String.**"),
        "^.*\\.String\\.String\\.String\\..*$"
    );
    assert_eq!(
        to_regex_string("**.String.String.String.*"),
        "^.*\\.String\\.String\\.String\\.[A-Z][a-zA-Z0-9]*$"
    );
    assert_eq!(to_regex_string("**.String"), "^.*\\.String$");
    assert_eq!(to_regex_string("*.String"), "^[A-Z][a-zA-Z0-9]*\\.String$");
    assert_eq!(
        to_regex_string("*.String.String.String"),
        "^[A-Z][a-zA-Z0-9]*\\.String\\.String\\.String$"
    );
}
