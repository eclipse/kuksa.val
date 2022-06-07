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

use crate::types::DataValue;

#[derive(Debug)]
pub enum ExecutionError {
    GeneralError(String),
    TypeError(String),
    CastError(String),
}

#[derive(Debug)]
pub struct ExecutionInputImpl {
    fields: HashMap<String, DataValue>,
}

pub trait ExecutionInput {
    fn lookup(&self, field: &str) -> DataValue;
}

impl CompiledQuery {
    pub fn execute(
        &self,
        input: &impl ExecutionInput,
    ) -> Result<Option<Vec<(String, DataValue)>>, ExecutionError> {
        // Check condition
        let condition_fulfilled = match &self.selection {
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
            let mut fields = Vec::new();
            for (index, e) in self.projection.iter().enumerate() {
                let name = match e {
                    Expr::Datapoint { name, data_type: _ } => name.clone(),
                    Expr::Alias { alias, .. } => alias.clone(),
                    _ => format!("field_{}", index),
                };
                match e.execute(input) {
                    Ok(value) => fields.push((name, value)),
                    Err(e) => return Err(e),
                }
            }
            Ok(Some(fields))
        } else {
            // Successful execution, but condition wasn't met
            Ok(None)
        }
    }
}

impl Expr {
    pub fn execute(&self, input: &impl ExecutionInput) -> Result<DataValue, ExecutionError> {
        match &self {
            Expr::Datapoint { name, data_type: _ } => Ok(input.lookup(name)),
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
                "OR is only possible with boolean expressions, tried \"{:?} OR {:?}\"",
                left_value, right_value
            ))),
        },
        Operator::And => match (&left_value, &right_value) {
            (DataValue::Bool(left), DataValue::Bool(right)) => Ok(DataValue::Bool(*left && *right)),
            _ => Err(ExecutionError::TypeError(format!(
                "AND is only possible with boolean expressions, tried \"{:?} AND {:?}\"",
                left_value, right_value
            ))),
        },
        Operator::Eq => match left_value.equals(&right_value) {
            Ok(equals) => Ok(DataValue::Bool(equals)),
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {:?} = {:?} isn't supported",
                left_value, right_value
            ))),
        },
        Operator::NotEq => match left_value.equals(&right_value) {
            Ok(equals) => Ok(DataValue::Bool(!equals)), // Negate equals
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {:?} != {:?} isn't supported",
                left_value, right_value
            ))),
        },
        Operator::Gt => match left_value.greater_than(&right_value) {
            Ok(greater_than) => Ok(DataValue::Bool(greater_than)),
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {:?} > {:?} isn't supported",
                left_value, right_value
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
                            "comparison {:?} >= {:?} isn't supported",
                            left_value, right_value
                        ))),
                    }
                }
            }
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {:?} >= {:?} isn't supported",
                left_value, right_value
            ))),
        },
        Operator::Lt => match left_value.less_than(&right_value) {
            Ok(less_than) => Ok(DataValue::Bool(less_than)),
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {:?} < {:?} isn't supported",
                left_value, right_value
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
                            "comparison {:?} <= {:?} isn't supported",
                            left_value, right_value
                        ))),
                    }
                }
            }
            Err(_) => Err(ExecutionError::CastError(format!(
                "comparison {:?} <= {:?} isn't supported",
                left_value, right_value
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
                "comparison BETWEEN {:?} AND ... not supported",
                data_value
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
                "comparison BETWEEN ... AND {:?} not supported",
                data_value
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

    pub fn add(&mut self, name: String, value: DataValue) {
        self.fields.insert(name, value);
    }
}

impl Default for ExecutionInputImpl {
    fn default() -> Self {
        Self::new()
    }
}

impl ExecutionInput for ExecutionInputImpl {
    fn lookup(&self, field: &str) -> DataValue {
        match self.fields.get(field) {
            Some(value) => value.to_owned(),
            None => DataValue::NotAvailable,
        }
    }
}

#[cfg(test)]
use super::{CompilationError, CompilationInput};
#[cfg(test)]
use crate::query::compiler;
#[cfg(test)]
use crate::types::DataType;

#[cfg(test)]
struct TestExecutionInput {
    seat_pos: i32,
    datapoint1: i32,
    datapoint2: bool,
}

#[cfg(test)]
struct TestCompilationInput {}

#[cfg(test)]
impl ExecutionInput for TestExecutionInput {
    fn lookup(&self, field: &str) -> DataValue {
        match field {
            "Vehicle.Cabin.Seat.Row1.Pos1.Position" => DataValue::Int32(self.seat_pos),
            "Vehicle.Datapoint1" => DataValue::Int32(self.datapoint1),
            "Vehicle.Datapoint2" => DataValue::Bool(self.datapoint2),
            _ => DataValue::NotAvailable,
        }
    }
}

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
    let execution_input1 = TestExecutionInput {
        seat_pos: 230,
        datapoint1: 61,
        datapoint2: true,
    };
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
    assert!(matches!(res, Some(_)));
    if let Some(fields) = &res {
        assert_eq!(fields.len(), 2);
        for (i, (name, value)) in fields.iter().enumerate() {
            assert_eq!(name, &expected[i].0);
            assert_eq!(value, &expected[i].1);
            println!("{}: {:?}", name, value)
        }
    }

    println!("EXECUTE");
    let execution_input1 = TestExecutionInput {
        seat_pos: 230,
        datapoint1: 40, // Condition not met
        datapoint2: true,
    };
    let res = compiled_query.execute(&execution_input1).unwrap();

    assert!(matches!(res, None));
}
