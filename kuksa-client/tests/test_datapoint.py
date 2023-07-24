# /********************************************************************************
# * Copyright (c) 2023 Contributors to the Eclipse Foundation
# *
# * See the NOTICE file(s) distributed with this work for additional
# * information regarding copyright ownership.
# *
# * This program and the accompanying materials are made available under the
# * terms of the Apache License 2.0 which is available at
# * http://www.apache.org/licenses/LICENSE-2.0
# *
# * SPDX-License-Identifier: Apache-2.0
# ********************************************************************************/

import pytest
from kuksa_client.grpc import Datapoint

#
# Client rules:
# For simple strings like abd it is optional to quote them ("abc") or not (abc)
# Quotes are needed if you have commas ("ab, c")
# If you have duoble quotes in strings you must escape them
#
# Note that KUKSA Server has different rules, there the payload must be valid JSON,
# so not all tests shown below are recommended as they cannot be used for KUKSA Server


def test_array_parse_no_quote():
    """
    No need for quotes just because you have a blank
    """
    test_str = r'[say hello, abc]'
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 2
    assert my_array[0] == "say hello"
    assert my_array[1] == "abc"


def test_array_parse_no_inside_quote():
    """Quotes are OK"""
    test_str = r'["say hello","abc"]'
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 2
    assert my_array[0] == "say hello"
    assert my_array[1] == "abc"


def test_array_parse_no_inside_quote_single():
    """Quotes are OK"""
    test_str = "['say hello','abc']"
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 2
    assert my_array[0] == "say hello"
    assert my_array[1] == "abc"


def test_array_parse_double_quote():
    test_str = r'["say \"hello\"","abc"]'
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 2
    assert my_array[0] == "say \"hello\""
    assert my_array[1] == "abc"


def test_array_parse_single_quote():
    test_str = r'[say \'hello\',abc]'
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 2
    assert my_array[0] == "say 'hello'"
    assert my_array[1] == "abc"


def test_array_parse_comma():
    test_str = r'["say, hello","abc"]'
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 2
    assert my_array[0] == r'say, hello'
    assert my_array[1] == "abc"


def test_array_square():
    """No problem having square brackets as part of strings"""
    test_str = r'[say hello[], abc]'
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 2
    assert my_array[0] == "say hello[]"
    assert my_array[1] == "abc"


def test_array_empty_string_quoted():
    test_str = r'["", abc]'
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 2
    assert my_array[0] == ""
    assert my_array[1] == "abc"


def test_array_empty_string_not_quoted():
    """In this case the first item is ignored"""
    test_str = r'[, abc]'
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 1
    assert my_array[0] == "abc"


def test_double_comma():
    """In this case the middle item is ignored"""
    test_str = r'[def,, abc]'
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 2
    assert my_array[0] == "def"
    assert my_array[1] == "abc"


def test_quotes_in_string_values():
    """Escaped double quotes, so in total 4 items"""
    test_str = r'["dtc1, dtc2", dtc3, \" dtc4, dtc4\"]'
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 4
    assert my_array[0] == "dtc1, dtc2"
    assert my_array[1] == "dtc3"
    assert my_array[2] == "\" dtc4"
    assert my_array[3] == "dtc4\""


def test_quotes_in_string_values_2():
    """Doubee quotes in double quotes so in total three values"""
    test_str = "['dtc1, dtc2', dtc3, \" dtc4, dtc4\"]"
    my_array = list(Datapoint.cast_array_values(Datapoint.cast_str, test_str))
    assert len(my_array) == 3
    assert my_array[0] == 'dtc1, dtc2'
    assert my_array[1] == "dtc3"
    assert my_array[2] == " dtc4, dtc4"


def test_int_no_quote():
    test_str = r'[123,456]'
    my_array = list(Datapoint.cast_array_values(int, test_str))
    assert len(my_array) == 2
    assert my_array[0] == 123
    assert my_array[1] == 456


def test_int_quote():
    """Quoting individual int values is not allowed"""
    test_str = r'["123","456"]'
    with pytest.raises(ValueError):
        list(Datapoint.cast_array_values(int, test_str))


def test_float_no_quote():
    test_str = r'[123,456.23]'
    my_array = list(Datapoint.cast_array_values(float, test_str))
    assert len(my_array) == 2
    assert my_array[0] == 123
    assert my_array[1] == 456.23


def test_cast_str():
    """Unquoted quotation marks shall be removed, quoted kept without quotes"""
    test_str = r'"say hello"'
    assert Datapoint.cast_str(test_str) == r'say hello'
    test_str = r'"say \"hello\""'
    assert Datapoint.cast_str(test_str) == r'say "hello"'
    test_str = r'say "hello"'
    assert Datapoint.cast_str(test_str) == r'say "hello"'


def test_cast_bool():
    assert Datapoint.cast_bool("true") is True
    assert Datapoint.cast_bool("True") is True
    assert Datapoint.cast_bool("T") is True
    assert Datapoint.cast_bool("t") is True
    assert Datapoint.cast_bool("false") is False
    assert Datapoint.cast_bool("False") is False
    assert Datapoint.cast_bool("F") is False
    assert Datapoint.cast_bool("f") is False

    # And then some other, treated as true for now
    assert Datapoint.cast_bool("Ja") is True
    assert Datapoint.cast_bool("Nein") is True
    assert Datapoint.cast_bool("Doch") is True
