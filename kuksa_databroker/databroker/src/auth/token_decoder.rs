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

use jsonwebtoken::{decode, Algorithm, DecodingKey, Validation};
use serde::Deserialize;

#[derive(Debug)]
pub enum TokenError {
    PublicKeyError(String),
    DecodeError(String),
}

impl std::error::Error for TokenError {}
impl std::fmt::Display for TokenError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!("{self:?}"))
    }
}

#[derive(Clone)]
pub struct TokenDecoder {
    decoding_key: DecodingKey,
    validator: Validation,
}

#[derive(Debug, Deserialize)]
pub struct Claims {
    #[allow(dead_code)]
    sub: String, // Subject (whom token refers to)
    #[allow(dead_code)]
    iss: String, // Issuer
    // aud: String, // Audience
    #[allow(dead_code)]
    iat: u64, // Issued at (as UTC timestamp)
    // nbf: usize, // Optional. Not Before (as UTC timestamp)
    #[allow(dead_code)]
    exp: u64, // Expiration time (as UTC timestamp)
}

impl TokenDecoder {
    pub fn new(public_key: impl Into<String>) -> Result<TokenDecoder, TokenError> {
        let decoding_key = match DecodingKey::from_rsa_pem(public_key.into().as_bytes()) {
            Ok(decoding_key) => decoding_key,
            Err(err) => {
                return Err(TokenError::PublicKeyError(format!(
                    "Error processing public key: {err}"
                )))
            }
        };

        let validator = Validation::new(Algorithm::RS256);
        // validator.leeway = 5;
        // validator.set_audience(..);
        // validator.set_issuer(..);

        Ok(TokenDecoder {
            decoding_key,
            validator,
        })
    }

    pub fn decode(&self, token: impl AsRef<str>) -> Result<Claims, TokenError> {
        match decode::<Claims>(token.as_ref(), &self.decoding_key, &self.validator) {
            Ok(token) => Ok(token.claims),
            Err(err) => Err(TokenError::DecodeError(err.to_string())),
        }
    }
}
