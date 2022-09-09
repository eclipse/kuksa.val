/********************************************************************************
* Copyright (c) 2022 Contributors to the Eclipse Foundation
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

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Only format generated code if rustfmt is available
    let rustfmt_available = std::process::Command::new("rustfmt")
        .arg("--version")
        .status()
        .is_ok();

    tonic_build::configure()
        .compile_well_known_types(false)
        .format(rustfmt_available)
        .compile(
            &[
                "proto/sdv/databroker/v1/broker.proto",
                "proto/sdv/databroker/v1/types.proto",
                "proto/sdv/databroker/v1/collector.proto",
                "proto/kuksa/val/v1/val.proto",
                "proto/kuksa/val/v1/types.proto",
            ],
            &["proto"],
        )?;
    Ok(())
}
