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

use lazy_static::lazy_static;
use regex::RegexSet;

use crate::glob;

lazy_static! {
    pub static ref ALLOW_ALL: Permissions = Permissions {
        expires_at: None,
        read: PathMatcher::Everything,
        actuate: PathMatcher::Everything,
        provide: PathMatcher::Everything,
        create: PathMatcher::Everything,
    };
    pub static ref ALLOW_NONE: Permissions = Permissions {
        expires_at: None,
        read: PathMatcher::Nothing,
        actuate: PathMatcher::Nothing,
        provide: PathMatcher::Nothing,
        create: PathMatcher::Nothing,
    };
}

#[derive(Debug, Clone)]
pub struct Permissions {
    expires_at: Option<SystemTime>,
    read: PathMatcher,
    actuate: PathMatcher,
    provide: PathMatcher,
    create: PathMatcher,
}

pub struct PermissionBuilder {
    expiration: Option<SystemTime>,
    read: PathMatchBuilder,
    actuate: PathMatchBuilder,
    provide: PathMatchBuilder,
    create: PathMatchBuilder,
}

pub enum Permission {
    Nothing,
    All,
    Glob(String),
}

pub enum PathMatchBuilder {
    Nothing,
    Everything,
    Globs(Vec<String>),
}

#[derive(Debug, Clone)]
pub enum PathMatcher {
    Nothing,
    Everything,
    Regexps(regex::RegexSet),
}

#[derive(Debug)]
pub enum PermissionError {
    Denied,
    Expired,
}

#[derive(Debug)]
pub enum PermissionsBuildError {
    BuildError,
}

impl Default for PermissionBuilder {
    fn default() -> Self {
        Self::new()
    }
}

impl PermissionBuilder {
    pub fn new() -> Self {
        Self {
            expiration: None,
            read: PathMatchBuilder::Nothing,
            actuate: PathMatchBuilder::Nothing,
            provide: PathMatchBuilder::Nothing,
            create: PathMatchBuilder::Nothing,
        }
    }

    pub fn expires_at(mut self, expiration: SystemTime) -> Self {
        self.expiration = Some(expiration);
        self
    }

    pub fn add_read_permission(mut self, permission: Permission) -> Self {
        match permission {
            Permission::Nothing => {
                // Adding nothing
            }
            Permission::All => self.read.extend_with(PathMatchBuilder::Everything),
            Permission::Glob(path) => self.read.extend_with_glob(path),
        };
        self
    }

    pub fn add_actuate_permission(mut self, permission: Permission) -> Self {
        match permission {
            Permission::Nothing => {
                // Adding nothing
            }
            Permission::All => self.actuate.extend_with(PathMatchBuilder::Everything),
            Permission::Glob(path) => self.actuate.extend_with_glob(path),
        };
        self
    }

    pub fn add_provide_permission(mut self, permission: Permission) -> Self {
        match permission {
            Permission::Nothing => {
                // Adding nothing
            }
            Permission::All => self.provide.extend_with(PathMatchBuilder::Everything),
            Permission::Glob(path) => self.provide.extend_with_glob(path),
        };
        self
    }

    pub fn add_create_permission(mut self, permission: Permission) -> Self {
        match permission {
            Permission::Nothing => {
                // Adding nothing
            }
            Permission::All => self.create.extend_with(PathMatchBuilder::Everything),
            Permission::Glob(path) => self.create.extend_with_glob(path),
        };
        self
    }

    pub fn build(self) -> Result<Permissions, PermissionsBuildError> {
        Ok(Permissions {
            expires_at: self.expiration,
            read: self.read.build()?,
            actuate: self.actuate.build()?,
            provide: self.provide.build()?,
            create: self.create.build()?,
        })
    }
}

impl Permissions {
    pub fn builder() -> PermissionBuilder {
        PermissionBuilder::new()
    }

    pub fn can_read(&self, path: &str) -> Result<(), PermissionError> {
        self.expired()?;

        if self.read.is_match(path) {
            return Ok(());
        }

        // Read permissions are included (by convention) in the
        // other permissions as well.
        if self.actuate.is_match(path) {
            return Ok(());
        }
        if self.provide.is_match(path) {
            return Ok(());
        }
        if self.create.is_match(path) {
            return Ok(());
        }

        Err(PermissionError::Denied)
    }

    pub fn can_write_actuator_target(&self, path: &str) -> Result<(), PermissionError> {
        self.expired()?;

        if self.actuate.is_match(path) {
            return Ok(());
        }
        Err(PermissionError::Denied)
    }

    pub fn can_write_datapoint(&self, path: &str) -> Result<(), PermissionError> {
        self.expired()?;

        if self.provide.is_match(path) {
            return Ok(());
        }
        Err(PermissionError::Denied)
    }

    pub fn can_create(&self, path: &str) -> Result<(), PermissionError> {
        self.expired()?;

        if self.create.is_match(path) {
            return Ok(());
        }
        Err(PermissionError::Denied)
    }

    #[inline]
    pub fn expired(&self) -> Result<(), PermissionError> {
        if let Some(expires_at) = self.expires_at {
            if expires_at < SystemTime::now() {
                return Err(PermissionError::Expired);
            }
        }
        Ok(())
    }
}

impl PathMatcher {
    pub fn is_match(&self, path: &str) -> bool {
        match self {
            PathMatcher::Nothing => false,
            PathMatcher::Everything => true,
            PathMatcher::Regexps(regexps) => regexps.is_match(path),
        }
    }
}

impl PathMatchBuilder {
    pub fn extend_with(&mut self, other: PathMatchBuilder) {
        if let PathMatchBuilder::Everything = self {
            // We already allow everything
            return;
        }

        match other {
            PathMatchBuilder::Nothing => {
                // No change
            }
            PathMatchBuilder::Everything => {
                // We now allow everything
                *self = PathMatchBuilder::Everything
            }
            PathMatchBuilder::Globs(mut other_regexps) => match self {
                PathMatchBuilder::Nothing => *self = PathMatchBuilder::Globs(other_regexps),
                PathMatchBuilder::Everything => {
                    // We already allow everything
                }
                PathMatchBuilder::Globs(regexps) => {
                    // Combine regexps
                    regexps.append(&mut other_regexps)
                }
            },
        }
    }

    pub fn extend_with_glob(&mut self, glob: String) {
        match self {
            PathMatchBuilder::Nothing => *self = PathMatchBuilder::Globs(vec![glob]),
            PathMatchBuilder::Everything => {
                // We already allow everything
            }
            PathMatchBuilder::Globs(globs) => {
                // Combine regexps
                globs.push(glob)
            }
        }
    }

    pub fn build(self) -> Result<PathMatcher, PermissionsBuildError> {
        match self {
            PathMatchBuilder::Nothing => Ok(PathMatcher::Nothing),
            PathMatchBuilder::Everything => Ok(PathMatcher::Everything),
            PathMatchBuilder::Globs(globs) => {
                let regexps = globs.iter().map(|glob| glob::to_regex_string(glob));

                Ok(PathMatcher::Regexps(RegexSet::new(regexps).map_err(
                    |err| match err {
                        regex::Error::Syntax(_) => PermissionsBuildError::BuildError,
                        regex::Error::CompiledTooBig(_) => PermissionsBuildError::BuildError,
                        _ => PermissionsBuildError::BuildError,
                    },
                )?))
            }
        }
    }
}
