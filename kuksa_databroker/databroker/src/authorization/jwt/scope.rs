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

#[derive(Debug)]
pub struct Scope {
    pub action: Action,
    pub path: Option<String>,
}

#[derive(Debug, Clone)]
pub enum Action {
    Read,
    Actuate,
    Provide,
    Create,
}

#[derive(Debug)]
pub enum Error {
    ParseError,
}

pub fn parse_whitespace_separated(scope: &str) -> Result<Vec<Scope>, Error> {
    let regex = regex::Regex::new(
        r"(?x)
        ^
        (?P<action>([^:]*)) # match action

        (?::
            (?P<path>
                (
                    (
                        [A-Z][a-zA-Z0-1]*
                        |
                        \*
                    )
                    (?:
                        \.
                        (
                            [A-Z][a-zA-Z0-1]*
                            |
                            \*
                        )
                    )*
                )
            )
        )?
        $",
    )
    .unwrap();

    let mut parsed_scopes = Vec::new();
    for scope in scope.split_whitespace() {
        let parsed_scope = match regex.captures(scope) {
            Some(captures) => {
                let action = match captures.name("action") {
                    Some(action) => match action.as_str() {
                        "read" => Action::Read,
                        "actuate" => Action::Actuate,
                        "provide" => Action::Provide,
                        "create" => Action::Create,
                        _ => {
                            // Unknown action
                            return Err(Error::ParseError);
                        }
                    },
                    None => {
                        // No action specified / Unknown action
                        return Err(Error::ParseError);
                    }
                };
                let path = captures.name("path").map(|path| path.as_str().to_owned());

                Scope { action, path }
            }
            None => {
                // Capture groups couldn't be produced
                return Err(Error::ParseError);
            }
        };
        parsed_scopes.push(parsed_scope);
    }

    Ok(parsed_scopes)
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn test_scope_read() {
        match parse_whitespace_separated("read") {
            Ok(scopes) => {
                assert_eq!(scopes.len(), 1);
                assert!(matches!(scopes[0].action, Action::Read));
                assert_eq!(scopes[0].path, None);
            }
            Err(_) => todo!(),
        }
    }

    #[test]
    fn test_scope_read_no_path() {
        match parse_whitespace_separated("read:") {
            Ok(_) => {
                panic!("empty path should result in error")
            }
            Err(Error::ParseError) => {}
        }
    }

    #[test]
    fn test_scope_read_vehicle_test() {
        match parse_whitespace_separated("read:Vehicle.Test") {
            Ok(scopes) => {
                assert_eq!(scopes.len(), 1);
                let scope = &scopes[0];
                assert!(matches!(scope.action, Action::Read));
                assert!(scope.path.is_some());
                assert_eq!(scope.path.as_deref().unwrap(), "Vehicle.Test");
            }
            Err(_) => todo!(),
        }
    }

    #[test]
    fn test_scope_read_vehicle_wildcard_test() {
        match parse_whitespace_separated("read:Vehicle.*.Test") {
            Ok(scopes) => {
                assert_eq!(scopes.len(), 1);
                let scope = &scopes[0];
                assert!(matches!(scope.action, Action::Read));
                assert!(scope.path.is_some());
                assert_eq!(scope.path.as_deref().unwrap(), "Vehicle.*.Test");
            }
            Err(_) => todo!(),
        }
    }

    #[test]
    fn test_scope_read_vehicle_prefixed_wildcard_test() {
        match parse_whitespace_separated("read:Vehicle.As*.Test") {
            Ok(_) => {
                panic!("wildcard is not allowed here");
            }
            Err(Error::ParseError) => {}
        }
    }

    #[test]
    fn test_scope_read_vehicle_test_wildcard_test() {
        match parse_whitespace_separated("read:Vehicle.Test.*") {
            Ok(scopes) => {
                assert_eq!(scopes.len(), 1);
                let scope = &scopes[0];
                assert!(matches!(scope.action, Action::Read));
                assert!(scope.path.is_some());
                assert_eq!(scope.path.as_deref().unwrap(), "Vehicle.Test.*");
            }
            Err(_) => panic!("should not error"),
        }
    }

    #[test]
    fn test_scope_actuate_no_path() {
        match parse_whitespace_separated("actuate") {
            Ok(scopes) => {
                assert_eq!(scopes.len(), 1);
                assert!(matches!(scopes[0].action, Action::Actuate));
                assert_eq!(scopes[0].path, None);
            }
            Err(_) => todo!(),
        }
    }

    #[test]
    fn test_scope_actuate_sep_no_path() {
        match parse_whitespace_separated("actuate:") {
            Ok(_) => {
                panic!("empty path should result in error")
            }
            Err(Error::ParseError) => {}
        }
    }

    #[test]
    fn test_scope_actuate_vehicle_test() {
        match parse_whitespace_separated("actuate:Vehicle.Test") {
            Ok(scopes) => {
                assert_eq!(scopes.len(), 1);
                let scope = &scopes[0];
                assert!(matches!(scope.action, Action::Actuate));
                assert!(scope.path.is_some());
                assert_eq!(scope.path.as_deref().unwrap(), "Vehicle.Test");
            }
            Err(_) => todo!(),
        }
    }

    #[test]
    fn test_scope_provide_no_path() {
        match parse_whitespace_separated("provide") {
            Ok(scopes) => {
                assert_eq!(scopes.len(), 1);
                assert!(matches!(scopes[0].action, Action::Provide));
                assert_eq!(scopes[0].path, None);
            }
            Err(_) => todo!(),
        }
    }

    #[test]
    fn test_scope_provide_sep_no_path() {
        match parse_whitespace_separated("provide:") {
            Ok(_) => {
                panic!("empty path should result in error")
            }
            Err(Error::ParseError) => {}
        }
    }

    #[test]
    fn test_scope_provide_vehicle_test() {
        match parse_whitespace_separated("provide:Vehicle.Test") {
            Ok(scopes) => {
                assert_eq!(scopes.len(), 1);
                let scope = &scopes[0];
                assert!(matches!(scope.action, Action::Provide));
                assert!(scope.path.is_some());
                assert_eq!(scope.path.as_deref().unwrap(), "Vehicle.Test");
            }
            Err(_) => todo!(),
        }
    }

    #[test]
    fn test_scope_create_no_path() {
        match parse_whitespace_separated("create") {
            Ok(scopes) => {
                assert_eq!(scopes.len(), 1);
                assert!(matches!(scopes[0].action, Action::Create));
                assert_eq!(scopes[0].path, None);
            }
            Err(_) => todo!(),
        }
    }

    #[test]
    fn test_scope_create_sep_no_path() {
        match parse_whitespace_separated("create:") {
            Ok(_) => {
                panic!("empty path should result in error")
            }
            Err(Error::ParseError) => {}
        }
    }

    #[test]
    fn test_scope_create_vehicle_test() {
        match parse_whitespace_separated("create:Vehicle.*") {
            Ok(scopes) => {
                assert_eq!(scopes.len(), 1);
                let scope = &scopes[0];
                assert!(matches!(scope.action, Action::Create));
                assert!(scope.path.is_some());
                assert_eq!(scope.path.as_deref().unwrap(), "Vehicle.*");
            }
            Err(_) => todo!(),
        }
    }
}
