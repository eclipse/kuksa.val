/********************************************************************************
* Copyright (c) 2022, 2023 Contributors to the Eclipse Foundation
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
    std::env::set_var("PROTOC", protobuf_src::protoc());
    tonic_build::configure()
        .compile_well_known_types(false)
        .protoc_arg("--experimental_allow_proto3_optional")
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
