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

use std::collections::HashMap;

use super::compiler::CompiledQuery;
use super::expr::*;

use crate::types::{DataValue, ExecutionInputImplData};

#[derive(Debug)]
pub enum ExecutionError {
    GeneralError(String),
    TypeError(String),
    CastError(String),
}

#[derive(Debug)]
pub struct ExecutionInputImpl {
    fields: HashMap<String, ExecutionInputImplData>,
}

pub trait ExecutionInput {
    fn lookup(&self, field: &str) -> &ExecutionInputImplData;

    fn get_fields(&self) -> &HashMap<String, ExecutionInputImplData>;
}

impl CompiledQuery {
    fn execute_internal(
        query: &CompiledQuery,
        input: &impl ExecutionInput,
    ) -> Result<Option<Vec<(String, DataValue)>>, ExecutionError> {
        // Check condition
        let condition_fulfilled = match &query.selection {
            Some(condition) => match condition.execute(input) {
                Ok(DataValue::Bool(b)) => b,
                Ok(_) => {
                    return Err(ExecutionError::GeneralError(
                        "Error evaluating WHERE statement (didn't evaluate to a boolean)"
                            .to_string(),
                    ));
                }
                Err(e) => return Err(e),
            },
            None => true,
        };

        if condition_fulfilled {
            struct NameAndData {
                name: String,
                data: Option<Vec<(String, DataValue)>>,
            }
            let mut fields = Vec::new();
            let mut is_subquery = false;
            for (index, e) in query.projection.iter().enumerate() {
                let expr_info = match e {
                    Expr::Datapoint {
                        name, data_type: _, ..
                    } => NameAndData {
                        name: name.clone(),
                        data: None,
                    },
                    Expr::Alias { alias, expr } => {
                        match expr.as_ref() {
                            Expr::Subquery { index } => {
                                is_subquery = true;
                                match CompiledQuery::execute_internal(
                                    &query.subquery[*index as usize],
                                    input,
                                ) {
                                    Ok(f) => match f {
                                        None => NameAndData {
                                            name: alias.clone(),
                                            data: None,
                                        },
                                        Some(vec) => NameAndData {
                                            name: alias.clone(),
                                            data: Some(vec),
                                        },
                                    },
                                    Err(_) => {
                                        // Don't be rude and just return None
                                        NameAndData {
                                            name: alias.clone(),
                                            data: None,
                                        }
                                    }
                                }
                            }
                            _ => NameAndData {
                                name: alias.clone(),
                                data: None,
                            },
                        }
                    }
                    Expr::Subquery { index } => {
                        is_subquery = true;
                        match CompiledQuery::execute_internal(
                            &query.subquery[*index as usize],
                            input,
                        ) {
                            Ok(f) => match f {
                                None => NameAndData {
                                    name: format!("subquery_{index}"),
                                    data: None,
                                },
                                Some(vec) => NameAndData {
                                    name: format!("subquery_{index}"),
                                    data: Some(vec),
                                },
                            },
                            Err(_) => {
                                // Don't be rude and just return None
                                NameAndData {
                                    name: format!("subquery_{index}"),
                                    data: None,
                                }
                            }
                        }
                    }
                    _ => NameAndData {
                        name: format!("field_{index}"),
                        data: None,
                    },
                };

                match expr_info.data {
                    None => match e.execute(input) {
                        Ok(value) => {
                            if !is_subquery {
                                fields.push((expr_info.name, value))
                            }
                        }
                        Err(e) => return Err(e),
                    },
                    Some(mut vec) => fields.append(&mut vec),
                }
            }
            if !fields.is_empty() {
                Ok(Some(fields))
            } else {
                Ok(None)
            }
        } else {
            // Successful execution, but condition wasn't met
            Ok(None)
        }
    }
    pub fn execute(
        &self,
        input: &impl ExecutionInput,
    ) -> Result<Option<Vec<(String, DataValue)>>, ExecutionError> {
        CompiledQuery::execute_internal(self, input)
    }
}

impl Expr {
    pub fn execute(&self, input: &impl ExecutionInput) -> Result<DataValue, ExecutionError> {
        match &self {
            Expr::Datapoint {
                name,
                data_type: _,
                lag,
            } => {
                let field = input.lookup(name);
                if *lag {
                    Ok(field.lag_value.clone())
                } else {
                    Ok(field.value.clone())
                }
            }
            Expr::Alias { expr, .. } => expr.execute(input),
            Expr::BinaryOperation {
                left,
                operator,
                right,
            } => execute_binary_operation(left, operator, right, input),
            Expr::UnaryOperation { expr, operator } => {
                execute_unary_operation(expr, operator, input)
            }
            Expr::ResolvedLiteral {
                value,
                data_type: _,
            } => Ok(value.clone()),
            Expr::Between {
                expr,
                negated,
                low,
                high,
            } => execute_between_operation(expr, *negated, low, high, input),
            Expr::Cast {
                expr: _,
                data_type: _,
            } => todo!(),
            Expr::UnresolvedLiteral { raw: _ } => {
                debug_assert!(
                    false,
                    "Unresolved literal should result in compilation error"
                );
                Err(ExecutionError::TypeError(
                    "Unresolved literal found while executing query".to_string(),
                ))
            }
            Expr::Subquery { index } => Ok(DataValue::Uint32(*index)),
        }
    }
}

fn execute_binary_operation(
    left: &Expr,
    operator: &Operator,
    right: &Expr,
    input: &impl ExecutionInput,
) -> Result<DataValue, ExecutionError> {
    let left_value = left.execute(input)?;
    let right_value = right.execute(input)?;

    match operator {
        Operator::Or => match (&left_value, &right_value) {
            (DataValue::Bool(left), DataValue::Bool(right)) => Ok(DataValue::Bool(*left || *right)),
            _ => Err(ExecutionError::TypeError(format!(
                "OR is only possible with boolean expressions, tried \"{left_value:?} OR {right_value:?}\""
            ))),
        },
        Operator::And => match (&left_value, &right_value) {
            (DataValue::Bool(left), DataValue::Bool(right)) => Ok(DataValue::Bool(*left && *right)),
            _ => Err(ExecutionError::TypeError(format!(
                "AND is only possible with boolean expressions, tried \"{left_value:?} AND {right_value:?}\""
            ))),
        },
        Operator::Eq => match left_value.equals(&right_value) {
            Ok(equals) => Ok(DataValue::Bool(equals)),
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {left_value:?} = {right_value:?} isn't supported"
            ))),
        },
        Operator::NotEq => match left_value.equals(&right_value) {
            Ok(equals) => Ok(DataValue::Bool(!equals)), // Negate equals
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {left_value:?} != {right_value:?} isn't supported"
            ))),
        },
        Operator::Gt => match left_value.greater_than(&right_value) {
            Ok(greater_than) => Ok(DataValue::Bool(greater_than)),
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {left_value:?} > {right_value:?} isn't supported"
            ))),
        },
        Operator::Ge => match left_value.greater_than(&right_value) {
            Ok(greater_than) => {
                if greater_than {
                    Ok(DataValue::Bool(greater_than))
                } else {
                    match left_value.equals(&right_value) {
                        Ok(equals) => Ok(DataValue::Bool(equals)),
                        Err(_) => Err(ExecutionError::CastError(format!(
                            "comparison {left_value:?} >= {right_value:?} isn't supported"
                        ))),
                    }
                }
            }
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {left_value:?} >= {right_value:?} isn't supported"
            ))),
        },
        Operator::Lt => match left_value.less_than(&right_value) {
            Ok(less_than) => Ok(DataValue::Bool(less_than)),
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {left_value:?} < {right_value:?} isn't supported"
            ))),
        },
        Operator::Le => match left_value.less_than(&right_value) {
            Ok(less_than) => {
                if less_than {
                    Ok(DataValue::Bool(less_than))
                } else {
                    match left_value.equals(&right_value) {
                        Ok(equals) => Ok(DataValue::Bool(equals)),
                        Err(_) => Err(ExecutionError::CastError(format!(
                            "comparison {left_value:?} <= {right_value:?} isn't supported"
                        ))),
                    }
                }
            }
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {left_value:?} <= {right_value:?} isn't supported"
            ))),
        },
    }
}

fn execute_unary_operation(
    expr: &Expr,
    operator: &UnaryOperator,
    fields: &impl ExecutionInput,
) -> Result<DataValue, ExecutionError> {
    match operator {
        UnaryOperator::Not => match expr.execute(fields) {
            Ok(DataValue::Bool(b)) => Ok(DataValue::Bool(!b)),
            Ok(_) => Err(ExecutionError::TypeError(
                "negation of non boolean expression isn't supported".to_string(),
            )),
            Err(e) => Err(e),
        },
    }
}

fn execute_between_operation(
    expr: &Expr,
    negated: bool,
    low: &Expr,
    high: &Expr,
    input: &impl ExecutionInput,
) -> Result<DataValue, ExecutionError> {
    match execute_binary_operation(expr, &Operator::Ge, low, input) {
        Ok(DataValue::Bool(low_cond)) => match low_cond {
            true => {
                if negated {
                    return Ok(DataValue::Bool(false));
                }
            }
            false => {
                if !negated {
                    return Ok(DataValue::Bool(false));
                }
            }
        },
        Ok(data_value) => {
            return Err(ExecutionError::TypeError(format!(
                "comparison BETWEEN {data_value:?} AND ... not supported"
            )))
        }
        Err(e) => return Err(e),
    };

    match execute_binary_operation(expr, &Operator::Le, high, input) {
        Ok(DataValue::Bool(high_cond)) => match high_cond {
            true => {
                if negated {
                    return Ok(DataValue::Bool(false));
                }
            }
            false => {
                if !negated {
                    return Ok(DataValue::Bool(false));
                }
            }
        },
        Ok(data_value) => {
            return Err(ExecutionError::TypeError(format!(
                "comparison BETWEEN ... AND {data_value:?} not supported"
            )))
        }
        Err(e) => return Err(e),
    };
    Ok(DataValue::Bool(true))
}

impl ExecutionInputImpl {
    pub fn new() -> Self {
        Self {
            fields: Default::default(),
        }
    }

    pub fn add(&mut self, name: String, value: ExecutionInputImplData) {
        self.fields.insert(name, value);
    }
}

impl Default for ExecutionInputImpl {
    fn default() -> Self {
        Self::new()
    }
}

impl ExecutionInput for ExecutionInputImpl {
    fn lookup(&self, field: &str) -> &ExecutionInputImplData {
        match self.fields.get(field) {
            Some(value) => value,
            None => &ExecutionInputImplData {
                value: DataValue::NotAvailable,
                lag_value: DataValue::NotAvailable,
            },
        }
    }

    fn get_fields(&self) -> &HashMap<String, ExecutionInputImplData> {
        &self.fields
    }
}

#[cfg(test)]
use super::{CompilationError, CompilationInput};
#[cfg(test)]
use crate::query::compiler;
#[cfg(test)]
use crate::types::DataType;

#[cfg(test)]
struct TestCompilationInput {}

#[cfg(test)]
impl CompilationInput for TestCompilationInput {
    fn get_datapoint_type(&self, field: &str) -> Result<DataType, CompilationError> {
        match field {
            "Vehicle.Cabin.Seat.Row1.Pos1.Position" => Ok(DataType::Int32),
            "Vehicle.Cabin.Seat.Row1.Pos2.Position" => Ok(DataType::Int32),
            "Vehicle.Datapoint1" => Ok(DataType::Int32),
            "Vehicle.Datapoint2" => Ok(DataType::Bool),
            "Vehicle.ADAS.ABS.IsActive" => Ok(DataType::Bool),
            _ => Err(CompilationError::UnknownField(field.to_owned())),
        }
    }
}

#[cfg(test)]
fn assert_expected(res: Option<Vec<(String, DataValue)>>, expected: &[(String, DataValue)]) {
    assert!(res.is_some());
    if let Some(fields) = &res {
        assert_eq!(fields.len(), expected.len());
        for (i, (name, value)) in fields.iter().enumerate() {
            assert_eq!(name, &expected[i].0);
            assert_eq!(value, &expected[i].1);
            println!("{name}: {value:?}")
        }
    }
}

#[test]
fn executor_test() {
    let sql = "
    SELECT
        Vehicle.Cabin.Seat.Row1.Pos1.Position,
        Vehicle.Cabin.Seat.Row1.Pos2.Position
    WHERE
        Vehicle.Datapoint1 > 50
        AND
        Vehicle.Datapoint2 = true
    ";

    let test_compilation_input = TestCompilationInput {};
    let compiled_query = compiler::compile(sql, &test_compilation_input).unwrap();

    assert!(&compiled_query
        .input_spec
        .contains("Vehicle.Cabin.Seat.Row1.Pos1.Position"));
    assert!(&compiled_query
        .input_spec
        .contains("Vehicle.Cabin.Seat.Row1.Pos2.Position"));
    assert!(&compiled_query.input_spec.contains("Vehicle.Datapoint1"));
    assert!(&compiled_query.input_spec.contains("Vehicle.Datapoint2"));

    println!("EXECUTE");
    let mut execution_input1 = ExecutionInputImpl::new();
    execution_input1.add(
        "Vehicle.Cabin.Seat.Row1.Pos1.Position".to_string(),
        ExecutionInputImplData {
            value: DataValue::Int32(230),
            lag_value: DataValue::NotAvailable,
        },
    );
    execution_input1.add(
        "Vehicle.Datapoint1".to_string(),
        ExecutionInputImplData {
            value: DataValue::Int32(61),
            lag_value: DataValue::NotAvailable,
        },
    );
    execution_input1.add(
        "Vehicle.Datapoint2".to_string(),
        ExecutionInputImplData {
            value: DataValue::Bool(true),
            lag_value: DataValue::NotAvailable,
        },
    );
    let res = compiled_query.execute(&execution_input1).unwrap();

    println!("RESULT: ");
    let expected = vec![
        (
            "Vehicle.Cabin.Seat.Row1.Pos1.Position".to_owned(),
            DataValue::Int32(230),
        ),
        (
            "Vehicle.Cabin.Seat.Row1.Pos2.Position".to_owned(),
            DataValue::NotAvailable,
        ),
    ];
    assert_expected(res, &expected);

    println!("EXECUTE");
    let mut execution_input1 = ExecutionInputImpl::new();
    execution_input1.add(
        "Vehicle.Cabin.Seat.Row1.Pos1.Position".to_string(),
        ExecutionInputImplData {
            value: DataValue::Int32(230),
            lag_value: DataValue::NotAvailable,
        },
    );
    execution_input1.add(
        "Vehicle.Datapoint1".to_string(),
        ExecutionInputImplData {
            value: DataValue::Int32(40),
            lag_value: DataValue::NotAvailable,
        },
    );
    execution_input1.add(
        "Vehicle.Datapoint2".to_string(),
        ExecutionInputImplData {
            value: DataValue::Bool(true),
            lag_value: DataValue::NotAvailable,
        },
    );
    let res = compiled_query.execute(&execution_input1).unwrap();

    assert!(res.is_none());
}

#[test]
fn executor_lag_test() {
    let sql = "
    SELECT
        Vehicle.Cabin.Seat.Row1.Pos1.Position,
        LAG(Vehicle.Cabin.Seat.Row1.Pos1.Position) as previousCabinSeatRow1PosPosition
    ";

    let test_compilation_input = TestCompilationInput {};
    let compiled_query = compiler::compile(sql, &test_compilation_input).unwrap();
    if let Some(Expr::Alias { alias, expr }) = compiled_query.projection.get(1) {
        assert_eq!(alias, "previousCabinSeatRow1PosPosition");
        if let Expr::Datapoint { lag, .. } = **expr {
            assert!(lag);
        }
    }

    println!("EXECUTE");
    let mut execution_input1 = ExecutionInputImpl::new();
    execution_input1.add(
        "Vehicle.Cabin.Seat.Row1.Pos1.Position".to_string(),
        ExecutionInputImplData {
            value: DataValue::Int32(230),
            lag_value: DataValue::Int32(231),
        },
    );
    let res = compiled_query.execute(&execution_input1).unwrap();
    assert!(res.is_some());
    let expected = vec![
        (
            "Vehicle.Cabin.Seat.Row1.Pos1.Position".to_owned(),
            DataValue::Int32(230),
        ),
        (
            "previousCabinSeatRow1PosPosition".to_owned(),
            DataValue::Int32(231),
        ),
    ];
    assert_expected(res, &expected);
}

#[test]
fn executor_lag_subquery_test() {
    let sql = "
    SELECT
        (SELECT Vehicle.Cabin.Seat.Row1.Pos1.Position),
        (SELECT LAG(Vehicle.Cabin.Seat.Row1.Pos1.Position) as previousCabinSeatRow1PosPosition)
    ";
    let test_compilation_input = TestCompilationInput {};
    let compiled_query = compiler::compile(sql, &test_compilation_input).unwrap();
    assert_eq!(compiled_query.subquery.len(), 2);
    if let Some(subquery) = compiled_query.subquery.first() {
        assert!(subquery
            .input_spec
            .contains("Vehicle.Cabin.Seat.Row1.Pos1.Position"));
    }
    let mut execution_input1 = ExecutionInputImpl::new();
    execution_input1.add(
        "Vehicle.Cabin.Seat.Row1.Pos1.Position".to_string(),
        ExecutionInputImplData {
            value: DataValue::Int32(230),
            lag_value: DataValue::Int32(231),
        },
    );
    let res = compiled_query.execute(&execution_input1).unwrap();
    assert!(res.is_some());
    let expected = vec![
        (
            "Vehicle.Cabin.Seat.Row1.Pos1.Position".to_owned(),
            DataValue::Int32(230),
        ),
        (
            "previousCabinSeatRow1PosPosition".to_owned(),
            DataValue::Int32(231),
        ),
    ];
    assert_expected(res, &expected);
}

#[test]
fn executor_where_lag_subquery_test() {
    let sql = "
    SELECT
        (SELECT Vehicle.Cabin.Seat.Row1.Pos1.Position
            WHERE
                LAG(Vehicle.Cabin.Seat.Row1.Pos1.Position) <> Vehicle.Cabin.Seat.Row1.Pos1.Position
        )
    ";
    let test_compilation_input = TestCompilationInput {};
    let compiled_query = compiler::compile(sql, &test_compilation_input).unwrap();
    let mut execution_input1 = ExecutionInputImpl::new();
    execution_input1.add(
        "Vehicle.Cabin.Seat.Row1.Pos1.Position".to_string(),
        ExecutionInputImplData {
            value: DataValue::Int32(230),
            lag_value: DataValue::NotAvailable,
        },
    );
    let res = compiled_query.execute(&execution_input1).unwrap();
    assert!(res.is_some());
    let expected = vec![(
        "Vehicle.Cabin.Seat.Row1.Pos1.Position".to_owned(),
        DataValue::Int32(230),
    )];
    assert_expected(res, &expected);

    let mut execution_input1 = ExecutionInputImpl::new();
    execution_input1.add(
        "Vehicle.Cabin.Seat.Row1.Pos1.Position".to_string(),
        ExecutionInputImplData {
            value: DataValue::Int32(230),
            lag_value: DataValue::Int32(230),
        },
    );
    let res = compiled_query.execute(&execution_input1).unwrap();
    assert!(res.is_none());
}
