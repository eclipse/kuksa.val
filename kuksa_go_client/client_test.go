
//********************************************************************************
// Copyright (c) 2022 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License 2.0 which is available at
// http://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// ********************************************************************************/

package main

import (
	"testing"
	"github.com/eclipse/kuksa.val/kuksa_go_client/kuksa_client"
)

// Note: Go support two methods to write a string
//
// Using double quotes
// var myvar := "hello \"there\""
//
// Using back quotes (raw strings)
// var myvar := `hello "there"`
//
// Single quotes only allowed for literals/runes, like 'w'

func TestArrayParseNoQuote(t *testing.T) {

	array, _ := kuksa_client.GetArrayFromInput[string](`[say hello, abc]`)
	if len(array) != 2 { t.Fail()}
	if array[0] != "say hello" { t.Fail()}
	if array[1] != "abc" { t.Fail()}
}

func TestArrayParseNoInsideQuote(t *testing.T) {

	array, _ := kuksa_client.GetArrayFromInput[string](`["say hello","abc"]`)
	if len(array) != 2 { t.Fail()}
	if array[0] != "say hello" { t.Fail()}
	if array[1] != "abc" { t.Fail()}
}

func TestArrayParseNoInsideQuoteSingle(t *testing.T) {

	array, _ := kuksa_client.GetArrayFromInput[string](`['say hello','abc']`)
	if len(array) != 2 { t.Fail()}
	if array[0] != "say hello" { t.Fail()}
	if array[1] != "abc" { t.Fail()}
}

func TestArrayParseDoubleQuote(t *testing.T) {

	array, _ := kuksa_client.GetArrayFromInput[string](`["say \"hello\"","abc"]`)
	if len(array) != 2 { t.Fail()}
	if array[0] != `say "hello"` { t.Fail()}
	if array[1] != "abc" { t.Fail()}
}

func TestArrayParseSingleQuote(t *testing.T) {

	array, _ := kuksa_client.GetArrayFromInput[string](`[say \'hello\',"abc"]`)
	if len(array) != 2 { t.Fail()}
	if array[0] != `say 'hello'` { t.Fail()}
	if array[1] != "abc" { t.Fail()}
}
	
func TestArrayParseComma(t *testing.T) {

	array, _ := kuksa_client.GetArrayFromInput[string](`["say, hello","abc"]`)
	if len(array) != 2 { t.Fail()}
	if array[0] != `say, hello` { t.Fail()}
	if array[1] != "abc" { t.Fail()}
}

func TestArraySquare(t *testing.T) {

	array, _ := kuksa_client.GetArrayFromInput[string](`[say hello[], abc]`)
	if len(array) != 2 { t.Fail()}
	if array[0] != `say hello[]` { t.Fail()}
	if array[1] != "abc" { t.Fail()}
}

func TestArrayEmptyStringQuoted(t *testing.T) {

	array, _ := kuksa_client.GetArrayFromInput[string](`["", abc]`)
	if len(array) != 2 { t.Fail()}
	if array[0] != `` { t.Fail()}
	if array[1] != "abc" { t.Fail()}
}

func TestArrayEmptyStringNotQuoted(t *testing.T) {
	// First item shall be ignored
	array, _ := kuksa_client.GetArrayFromInput[string](`[, abc]`)
	if len(array) != 1 { t.Fail()}
	if array[0] != "abc" { t.Fail()}
}

func TestDoubleComma(t *testing.T) {
	// In this case the middle item is ignored
	array, _ := kuksa_client.GetArrayFromInput[string](`[def,, abc]`)
	if len(array) != 2 { t.Fail()}
	if array[0] != "def" { t.Fail()}
	if array[1] != "abc" { t.Fail()}
}

func TestQuotesInStringValues(t *testing.T) {
	array, _ := kuksa_client.GetArrayFromInput[string](`["dtc1, dtc2", dtc3, \" dtc4, dtc4\"]`)
	if len(array) != 4 { t.Fail()}
	if array[0] != "dtc1, dtc2" { t.Fail()}
	if array[1] != "dtc3" { t.Fail()}
	if array[2] != "\" dtc4" { t.Fail()}
	if array[3] != "dtc4\"" { t.Fail()}
}

func TestQuotesInStringValues2(t *testing.T) {
	array, _ := kuksa_client.GetArrayFromInput[string]("['dtc1, dtc2', dtc3, \" dtc4, dtc4\"]")
	if len(array) != 3 { t.Fail()}
	if array[0] != "dtc1, dtc2" { t.Fail()}
	if array[1] != "dtc3" { t.Fail()}
	if array[2] != " dtc4, dtc4" { t.Fail()}
}

