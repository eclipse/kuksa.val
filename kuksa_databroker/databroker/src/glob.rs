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

pub enum Error {
    RegexError,
}

pub fn to_regex(glob: &str) -> Result<Regex, Error> {
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

    Regex::new(&re).map_err(|_err| Error::RegexError)
}
