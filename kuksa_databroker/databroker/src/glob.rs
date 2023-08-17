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

    // Replace all "standalone" wildcards with ".*"
    re.push_str(
        &glob
            .split('.')
            .map(|part| match part {
                "*" => ".*",
                other => other,
            })
            .collect::<Vec<&str>>()
            .join(r"\."),
    );

    // If it doesn't already end with a wildcard, add it
    if !re.ends_with(".*") {
        re.push_str(".*");
    }

    // And finally, make sure we match until EOL
    re.push('$');

    re
}

pub fn to_regex(glob: &str) -> Result<Regex, Error> {
    let re = to_regex_string(glob);
    Regex::new(&re).map_err(|_err| Error::RegexError)
}

pub fn matches_path_pattern(input: &str) -> bool {
    if !input.starts_with('*') {
        let pattern = r"^(?:\w+(?:\.\w+)*)(?:\.\*+)?$";
        let regex = Regex::new(pattern).unwrap();

        let pattern2 = r"^[^.]+(\.[^.]+)*\.(\*{2}|\*|[A-Za-z]+)(\.[^.]+)*\.[^.]+$";
        let regex2 = Regex::new(pattern2).unwrap();

        regex.is_match(input) || regex2.is_match(input)
    } else {
        false
    }
}

#[tokio::test]
async fn test_valid_request_path() {
    let inputs = vec![
        "String.*",
        "String.**",
        "String.String.String.String.*",
        "String.String.String.String.**",
        "String.String.String.String",
        "String.String.String.String.String.**",
        "String.String.String.*.String",
        "String.String.String.**.String",
        "String.String.String.String.String.**.String",
        "String.String.String.String.*.String.String",
        "String.*.String.String",
        "String.String.**.String.String",
        "String.**.String.String",
        "String.String.String.",
        "String.String.String.String..",
        "String.*.String.String..",
        "*.String.String.String..",
        "*.String.String.String",
        "**.String.String.String.**",
        "**.String.String.String.*",
    ];

    assert!(matches_path_pattern(inputs[0]));
    assert!(matches_path_pattern(inputs[1]));
    assert!(matches_path_pattern(inputs[2]));
    assert!(matches_path_pattern(inputs[3]));
    assert!(matches_path_pattern(inputs[4]));
    assert!(matches_path_pattern(inputs[5]));
    assert!(matches_path_pattern(inputs[6]));
    assert!(matches_path_pattern(inputs[7]));
    assert!(matches_path_pattern(inputs[8]));
    assert!(matches_path_pattern(inputs[9]));
    assert!(matches_path_pattern(inputs[10]));
    assert!(matches_path_pattern(inputs[11]));
    assert!(matches_path_pattern(inputs[12]));
    assert!(!matches_path_pattern(inputs[13]));
    assert!(!matches_path_pattern(inputs[14]));
    assert!(!matches_path_pattern(inputs[15]));
    assert!(!matches_path_pattern(inputs[16]));
    assert!(!matches_path_pattern(inputs[17]));
    assert!(!matches_path_pattern(inputs[18]));
    assert!(!matches_path_pattern(inputs[19]));
}
