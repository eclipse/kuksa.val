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

use crate::types::{DataType, DataValue};

#[derive(Debug, Clone)]
pub enum Expr {
    Datapoint {
        name: String,
        data_type: DataType,
        lag: bool,
    },
    Alias {
        expr: Box<Expr>,
        alias: String,
    },
    Cast {
        expr: Box<Expr>,
        data_type: DataType,
    },
    UnresolvedLiteral {
        raw: String,
    },
    ResolvedLiteral {
        value: DataValue,
        data_type: DataType,
    },
    BinaryOperation {
        left: Box<Expr>,
        operator: Operator,
        right: Box<Expr>,
    },
    UnaryOperation {
        expr: Box<Expr>,
        operator: UnaryOperator,
    },
    Subquery {
        index: u32,
    },
    Between {
        expr: Box<Expr>,
        negated: bool,
        low: Box<Expr>,
        high: Box<Expr>,
    },
}

#[derive(Debug, Clone)]
pub enum Operator {
    And,
    Or,
    Eq,
    NotEq,
    Gt,
    Ge,
    Lt,
    Le,
}

#[derive(Debug, Clone)]
pub enum UnaryOperator {
    Not,
}

pub enum UnresolvedLiteral {
    Number(String),
    Error,
}

impl Expr {
    pub fn get_type(&self) -> Result<DataType, UnresolvedLiteral> {
        match self {
            Expr::Datapoint {
                name: _, data_type, ..
            } => Ok(data_type.clone()),
            Expr::Alias { expr, alias: _ } => expr.get_type(),
            Expr::Cast { expr: _, data_type } => Ok(data_type.clone()),
            Expr::UnresolvedLiteral { raw } => Err(UnresolvedLiteral::Number(raw.clone())),
            Expr::Subquery { index: _ } => Ok(DataType::Uint32),
            Expr::ResolvedLiteral {
                value: _,
                data_type,
            } => Ok(data_type.clone()),
            Expr::BinaryOperation {
                left: _,
                operator: _,
                right: _,
            } => Ok(DataType::Bool),
            Expr::UnaryOperation { expr: _, operator } => match operator {
                UnaryOperator::Not => Ok(DataType::Bool),
            },
            Expr::Between {
                expr: _,
                negated: _,
                low: _,
                high: _,
            } => Ok(DataType::Bool),
        }
    }
}
