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

mod compiler;
mod executor;
mod expr;

pub use compiler::compile;
pub use compiler::{CompilationError, CompilationInput, CompiledQuery};

pub use executor::{ExecutionError, ExecutionInput};

// Example trait implementations
pub use compiler::CompilationInputImpl;
pub use executor::ExecutionInputImpl;
