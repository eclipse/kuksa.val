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

use super::expr::*;

use sqlparser::ast;

use crate::types::{DataType, DataValue};

use std::collections::{HashMap, HashSet};

#[derive(Debug)]
pub enum CompilationError {
    UnknownField(String),
    MalformedNumber(String),
    UnsupportedOperator(String),
    UnsupportedOperation(String),
    TypeError(String),
    InvalidLogic(String),
    InvalidComparison(String),
    ParseError(String),
}

pub trait CompilationInput {
    fn get_datapoint_type(&self, field: &str) -> Result<DataType, CompilationError>;
}

pub struct CompilationInputImpl {
    pub schema: HashMap<String, DataType>, // Needed datapoints (types) for compilation
}

impl CompilationInputImpl {
    pub fn new() -> Self {
        Self {
            schema: Default::default(),
        }
    }

    pub fn add_entry(&mut self, name: &str, data_type: &DataType) {
        self.schema.insert(name.to_owned(), data_type.to_owned());
    }
}

impl Default for CompilationInputImpl {
    fn default() -> Self {
        Self::new()
    }
}
impl CompilationInput for CompilationInputImpl {
    fn get_datapoint_type(&self, field: &str) -> Result<DataType, CompilationError> {
        match self.schema.get(field) {
            Some(v) => Ok(v.clone()),
            None => Err(CompilationError::UnknownField(field.to_owned())),
        }
    }
}

pub struct CompiledQuery {
    /// A single expression (tree) evaluating to either
    /// true or false.
    pub selection: Option<Expr>, // WHERE {selection}

    /// Each `Expr` in this vector produces a value upon
    /// execution.
    pub projection: Vec<Expr>, // SELECT {projection}

    /// Full list of datapoint id:s of interest,
    /// either specified in the SELECT statement,
    /// or as part of a condition.
    ///
    /// These needs to be provided in the `input` when
    /// executing the query.
    pub input_spec: HashSet<String>, // Needed datapoints (values) for execution
}

impl CompiledQuery {
    pub fn new() -> Self {
        CompiledQuery {
            selection: None,
            projection: Vec::new(),
            input_spec: HashSet::new(),
        }
    }
}

impl Default for CompiledQuery {
    fn default() -> Self {
        Self::new()
    }
}

pub fn compile_expr(
    expr: &ast::Expr,
    input: &impl CompilationInput,
    output: &mut CompiledQuery,
) -> Result<Expr, CompilationError> {
    match expr {
        ast::Expr::Value(ast::Value::Number(n, _)) => {
            Ok(Expr::UnresolvedLiteral { raw: n.to_owned() })
        }
        ast::Expr::Value(ast::Value::SingleQuotedString(ref s)) => Ok(Expr::ResolvedLiteral {
            value: DataValue::String(s.clone()),
            data_type: DataType::String,
        }),

        ast::Expr::Value(ast::Value::Boolean(n)) => Ok(Expr::ResolvedLiteral {
            value: DataValue::Bool(*n),
            data_type: DataType::Bool,
        }),

        ast::Expr::Value(ast::Value::DoubleQuotedString(s)) => Ok(Expr::ResolvedLiteral {
            value: DataValue::String(s.clone()),
            data_type: DataType::String,
        }),

        ast::Expr::Identifier(ref id) => {
            let name = &id.value;

            // Does the datapoint exist?
            let data_type = input.get_datapoint_type(name)?;
            output.input_spec.insert(name.clone());
            Ok(Expr::Datapoint {
                name: name.clone(),
                data_type,
            })
        }

        ast::Expr::CompoundIdentifier(ids) => {
            let string_ids: Vec<&str> = ids.iter().map(|id| id.value.as_ref()).collect();
            let field = string_ids.join(".");

            // Does the datapoint exist?
            let data_type = input.get_datapoint_type(&field)?;
            output.input_spec.insert(field.clone());
            Ok(Expr::Datapoint {
                name: field,
                data_type,
            })
        }

        ast::Expr::BinaryOp {
            ref left,
            ref op,
            ref right,
        } => {
            // Resolve unresolved literals if any
            let (left_expr, right_expr) = {
                let left_expr = compile_expr(left, input, output)?;
                let right_expr = compile_expr(right, input, output)?;

                match (left_expr.get_type(), right_expr.get_type()) {
                    (Err(literal), Ok(right_type)) => match resolve_literal(literal, right_type) {
                        Ok(left_expr) => (Box::new(left_expr), Box::new(right_expr)),
                        Err(_) => {
                            return Err(CompilationError::TypeError(format!(
                                "left side is incompatible with right side in expression \"{expr}\""
                            )))
                        }
                    },
                    (Ok(left_type), Err(literal)) => match resolve_literal(literal, left_type) {
                        Ok(right_expr) => (Box::new(left_expr), Box::new(right_expr)),
                        Err(_) => {
                            return Err(CompilationError::TypeError(format!(
                                "right side is incompatible with left side in expression \"{expr}\""
                            )))
                        }
                    },
                    (Err(left_literal), Err(right_literal)) => {
                        match (left_literal, right_literal) {
                            (
                                UnresolvedLiteral::Number(raw_left),
                                UnresolvedLiteral::Number(raw_right),
                            ) => {
                                // Try to resolve to unresolved literals by parsing them together
                                match (raw_left.parse::<i64>(), raw_right.parse::<i64>()) {
                                    (Ok(left_value), Ok(right_value)) => (
                                        Box::new(Expr::ResolvedLiteral {
                                            value: DataValue::Int64(left_value),
                                            data_type: DataType::Int64,
                                        }),
                                        Box::new(Expr::ResolvedLiteral {
                                            value: DataValue::Int64(right_value),
                                            data_type: DataType::Int64,
                                        }),
                                    ),
                                    _ => {
                                        match (raw_left.parse::<u64>(), raw_right.parse::<u64>()) {
                                            (Ok(left_value), Ok(right_value)) => (
                                                Box::new(Expr::ResolvedLiteral {
                                                    value: DataValue::Uint64(left_value),
                                                    data_type: DataType::Uint64,
                                                }),
                                                Box::new(Expr::ResolvedLiteral {
                                                    value: DataValue::Uint64(right_value),
                                                    data_type: DataType::Uint64,
                                                }),
                                            ),
                                            _ => match (
                                                raw_left.parse::<f64>(),
                                                raw_right.parse::<f64>(),
                                            ) {
                                                (Ok(left_value), Ok(right_value)) => (
                                                    Box::new(Expr::ResolvedLiteral {
                                                        value: DataValue::Double(left_value),
                                                        data_type: DataType::Double,
                                                    }),
                                                    Box::new(Expr::ResolvedLiteral {
                                                        value: DataValue::Double(right_value),
                                                        data_type: DataType::Double,
                                                    }),
                                                ),
                                                _ => return Err(CompilationError::TypeError(format!(
                                                    "right side is incompatible with left side in expression \"{expr}\""
                                                )))
                                            },
                                        }
                                    }
                                }
                            }
                            _ => {
                                return Err(CompilationError::TypeError(format!(
                                "right side is incompatible with left side in expression \"{expr}\""
                            )))
                            }
                        }
                    }
                    (Ok(_), Ok(_)) => (Box::new(left_expr), Box::new(right_expr)),
                }
            };
            match op {
                ast::BinaryOperator::Gt => Ok(Expr::BinaryOperation {
                    left: left_expr,
                    operator: Operator::Gt,
                    right: right_expr,
                }),
                ast::BinaryOperator::GtEq => Ok(Expr::BinaryOperation {
                    left: left_expr,
                    operator: Operator::Ge,
                    right: right_expr,
                }),
                ast::BinaryOperator::Lt => Ok(Expr::BinaryOperation {
                    left: left_expr,
                    operator: Operator::Lt,
                    right: right_expr,
                }),
                ast::BinaryOperator::LtEq => Ok(Expr::BinaryOperation {
                    left: left_expr,
                    operator: Operator::Le,
                    right: right_expr,
                }),
                ast::BinaryOperator::Eq => Ok(Expr::BinaryOperation {
                    left: left_expr,
                    operator: Operator::Eq,
                    right: right_expr,
                }),
                ast::BinaryOperator::NotEq => Ok(Expr::BinaryOperation {
                    left: left_expr,
                    operator: Operator::NotEq,
                    right: right_expr,
                }),
                ast::BinaryOperator::And => match (left_expr.get_type(), right_expr.get_type()) {
                    (Ok(DataType::Bool), Ok(DataType::Bool)) => Ok(Expr::BinaryOperation {
                        left: left_expr,
                        operator: Operator::And,
                        right: right_expr,
                    }),
                    _ => Err(CompilationError::TypeError(
                        "AND requires boolean expressions on both sides".to_string(),
                    )),
                },
                ast::BinaryOperator::Or => match (left_expr.get_type(), right_expr.get_type()) {
                    (Ok(DataType::Bool), Ok(DataType::Bool)) => {
                        // It's all good man
                        Ok(Expr::BinaryOperation {
                            left: left_expr,
                            operator: Operator::Or,
                            right: right_expr,
                        })
                    }
                    _ => Err(CompilationError::TypeError(
                        "OR requires boolean expressions on both sides".to_string(),
                    )),
                },
                operator => Err(CompilationError::UnsupportedOperator(
                    format!("{operator}",),
                )),
            }
        }
        ast::Expr::Nested(e) => compile_expr(e, input, output),

        ast::Expr::UnaryOp { ref op, ref expr } => match op {
            ast::UnaryOperator::Not => Ok(Expr::UnaryOperation {
                expr: Box::new(compile_expr(expr, input, output)?),
                operator: UnaryOperator::Not,
            }),
            operator => Err(CompilationError::UnsupportedOperator(format!(
                "Unsupported unary operator \"{operator}\""
            ))),
        },

        ast::Expr::Between {
            ref expr,
            ref negated,
            ref low,
            ref high,
        } => Ok(Expr::Between {
            expr: Box::new(compile_expr(expr, input, output)?),
            negated: *negated,
            low: Box::new(compile_expr(low, input, output)?),
            high: Box::new(compile_expr(high, input, output)?),
        }),
        operator => Err(CompilationError::UnsupportedOperator(format!(
            "Unsupported operator \"{operator}\""
        ))),
    }
}

fn resolve_literal(
    unresolved_literal: UnresolvedLiteral,
    wanted_type: DataType,
) -> Result<Expr, UnresolvedLiteral> {
    match (wanted_type, unresolved_literal) {
        (DataType::Int8, UnresolvedLiteral::Number(raw)) => match raw.parse::<i8>() {
            Ok(value) => Ok(Expr::ResolvedLiteral {
                value: DataValue::Int32(value as i32),
                data_type: DataType::Int8,
            }),
            Err(_) => Err(UnresolvedLiteral::Error),
        },
        (DataType::Int16, UnresolvedLiteral::Number(raw)) => match raw.parse::<i16>() {
            Ok(value) => Ok(Expr::ResolvedLiteral {
                value: DataValue::Int32(value as i32),
                data_type: DataType::Int16,
            }),
            Err(_) => Err(UnresolvedLiteral::Error),
        },
        (DataType::Int32, UnresolvedLiteral::Number(raw)) => match raw.parse::<i32>() {
            Ok(value) => Ok(Expr::ResolvedLiteral {
                value: DataValue::Int32(value),
                data_type: DataType::Int32,
            }),
            Err(_) => Err(UnresolvedLiteral::Error),
        },
        (DataType::Int64, UnresolvedLiteral::Number(raw)) => match raw.parse::<i64>() {
            Ok(value) => Ok(Expr::ResolvedLiteral {
                value: DataValue::Int64(value),
                data_type: DataType::Int64,
            }),
            Err(_) => Err(UnresolvedLiteral::Error),
        },
        (DataType::Uint8, UnresolvedLiteral::Number(raw)) => match raw.parse::<u8>() {
            Ok(value) => Ok(Expr::ResolvedLiteral {
                value: DataValue::Uint32(value as u32),
                data_type: DataType::Uint8,
            }),
            Err(_) => Err(UnresolvedLiteral::Error),
        },
        (DataType::Uint16, UnresolvedLiteral::Number(raw)) => match raw.parse::<u16>() {
            Ok(value) => Ok(Expr::ResolvedLiteral {
                value: DataValue::Uint32(value as u32),
                data_type: DataType::Uint16,
            }),
            Err(_) => Err(UnresolvedLiteral::Error),
        },
        (DataType::Uint32, UnresolvedLiteral::Number(raw)) => match raw.parse::<u32>() {
            Ok(value) => Ok(Expr::ResolvedLiteral {
                value: DataValue::Uint32(value),
                data_type: DataType::Uint32,
            }),
            Err(_) => Err(UnresolvedLiteral::Error),
        },
        (DataType::Uint64, UnresolvedLiteral::Number(raw)) => match raw.parse::<u64>() {
            Ok(value) => Ok(Expr::ResolvedLiteral {
                value: DataValue::Uint64(value),
                data_type: DataType::Uint64,
            }),
            Err(_) => Err(UnresolvedLiteral::Error),
        },
        (DataType::Float, UnresolvedLiteral::Number(raw)) => match raw.parse::<f32>() {
            Ok(value) => Ok(Expr::ResolvedLiteral {
                value: DataValue::Float(value),
                data_type: DataType::Float,
            }),
            Err(_) => Err(UnresolvedLiteral::Error),
        },
        (DataType::Double, UnresolvedLiteral::Number(raw)) => match raw.parse::<f64>() {
            Ok(value) => Ok(Expr::ResolvedLiteral {
                value: DataValue::Double(value),
                data_type: DataType::Double,
            }),
            Err(_) => Err(UnresolvedLiteral::Error),
        },
        _ => Err(UnresolvedLiteral::Error),
    }
}

pub fn compile(
    sql: &str,
    input: &impl CompilationInput,
) -> Result<CompiledQuery, CompilationError> {
    let dialect = sqlparser::dialect::GenericDialect {};

    match sqlparser::parser::Parser::parse_sql(&dialect, sql) {
        Ok(ast) => {
            let select_statement = match &ast.first() {
                Some(sqlparser::ast::Statement::Query(q)) => match &q.body {
                    sqlparser::ast::SetExpr::Select(query) => Some(query.clone()),
                    _ => None,
                },
                _ => None,
            };

            if let Some(select) = select_statement {
                let mut query = CompiledQuery::new();

                match &select.selection {
                    None => {}
                    Some(expr) => {
                        let condition = compile_expr(expr, input, &mut query)?;
                        if let Ok(data_type) = condition.get_type() {
                            if data_type != DataType::Bool {
                                return Err(CompilationError::TypeError(
                                    "WHERE statement doesn't evaluate to a boolean expression"
                                        .to_string(),
                                ));
                            }
                        }

                        query.selection = Some(condition);
                    }
                };

                for c in &select.projection {
                    match c {
                        ast::SelectItem::UnnamedExpr(expr) => {
                            let expr = compile_expr(expr, input, &mut query)?;
                            query.projection.push(expr);
                        }
                        ast::SelectItem::ExprWithAlias { expr, alias } => {
                            let expr = compile_expr(expr, input, &mut query)?;
                            let name = alias.value.clone();
                            query.projection.push(Expr::Alias {
                                expr: Box::new(expr),
                                alias: name,
                            });
                        }
                        _ => {
                            return Err(CompilationError::UnsupportedOperation(
                                "unrecognized entry in SELECT statement".to_string(),
                            ))
                        }
                    }
                }

                Ok(query)
            } else {
                Err(CompilationError::UnsupportedOperation(
                    "unrecognized SELECT statement".to_string(),
                ))
            }
        }
        Err(e) => Err(CompilationError::ParseError(format!("{e}"))),
    }
}
