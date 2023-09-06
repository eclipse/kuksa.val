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

use thiserror::Error;

pub mod jwt;

#[derive(Clone)]
#[allow(clippy::large_enum_variant)]
pub enum Authorization {
    Disabled,
    Enabled { token_decoder: jwt::Decoder },
}

#[derive(Error, Debug)]
pub enum Error {
    #[error("Invalid public key")]
    InvalidPublicKey,
}

impl Authorization {
    pub fn new(public_key: String) -> Result<Authorization, Error> {
        Ok(Authorization::Enabled {
            token_decoder: jwt::Decoder::new(public_key).map_err(|_| Error::InvalidPublicKey)?,
        })
    }
}
