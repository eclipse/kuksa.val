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

use std::time::SystemTime;

#[derive(Debug)]
pub struct Permissions {
    pub expires_at: SystemTime,
    pub read: PathMatcher,
    pub actuate: PathMatcher,
    pub provide: PathMatcher,
}

#[derive(Debug)]
pub enum PathMatcher {
    Nothing,
    Everything,
    Regexps(Vec<regex::Regex>),
}

#[derive(Debug)]
pub enum Error {
    AccessDenied,
    Expired,
}

impl Permissions {
    pub fn read_allowed(&self, path: &str) -> Result<(), Error> {
        if self.expires_at < SystemTime::now() {
            return Err(Error::Expired);
        }
        if self.read.is_match(path) {
            return Ok(());
        }
        Err(Error::AccessDenied)
    }

    pub fn actuate_allowed(&self, path: &str) -> Result<(), Error> {
        if self.expires_at < SystemTime::now() {
            return Err(Error::Expired);
        }
        if self.actuate.is_match(path) {
            return Ok(());
        }
        Err(Error::AccessDenied)
    }

    pub fn provide_allowed(&self, path: &str) -> Result<(), Error> {
        if self.expires_at < SystemTime::now() {
            return Err(Error::Expired);
        }
        if self.provide.is_match(path) {
            return Ok(());
        }
        Err(Error::AccessDenied)
    }
}

impl PathMatcher {
    pub fn is_match(&self, path: &str) -> bool {
        match self {
            PathMatcher::Nothing => false,
            PathMatcher::Everything => true,
            PathMatcher::Regexps(regexps) => {
                for re in regexps {
                    if re.is_match(path) {
                        return true;
                    }
                }
                false
            }
        }
    }

    pub fn extend_with(&mut self, other: PathMatcher) {
        if let PathMatcher::Everything = self {
            // We already allow everything
            return;
        }

        match other {
            PathMatcher::Nothing => {
                // No change
            }
            PathMatcher::Everything => {
                // We now allow everything
                *self = PathMatcher::Everything
            }
            PathMatcher::Regexps(mut other_regexps) => match self {
                PathMatcher::Nothing => *self = PathMatcher::Regexps(other_regexps),
                PathMatcher::Everything => {
                    // We already allow everything
                }
                PathMatcher::Regexps(regexps) => {
                    // Combine regexps
                    regexps.append(&mut other_regexps)
                }
            },
        }
    }
}
