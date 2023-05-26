/********************************************************************************
* Copyright (c) 2022-2023 Contributors to the Eclipse Foundation
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

use anyhow::Result;

use vergen::EmitBuilder;

// Extract build info (at build time)
fn main() -> Result<()> {
    EmitBuilder::builder().all_cargo().all_git().emit()?;
    Ok(())
}
