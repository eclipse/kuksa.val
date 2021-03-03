#
# ******************************************************************************
# Copyright (c) 2018 Robert Bosch GmbH and others.
#
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v2.0
# which accompanies this distribution, and is available at
# https://www.eclipse.org/org/documents/epl-2.0/index.php
#
#  Contributors:
#      Robert Bosch GmbH - initial API and functionality
# *****************************************************************************

project(kuksa-val-unit-test)
enable_testing()
include(CTest)

######
# CMake configuration responsible for building kuksa-val optional unit tests based on core library

###
# Setup target names
set(UNITTEST_EXE_NAME "kuksaval-unit-test" )

set(BUILD_UNIT_TEST OFF CACHE BOOL "Build unit tests")

###
# Setup targets

if(BUILD_UNIT_TEST)
  add_executable(${UNITTEST_EXE_NAME}
    AccessCheckerTests.cpp
    AuthenticatorTests.cpp
    RestV1ApiHandlerTests.cpp
    SubscriptionHandlerTests.cpp
    VssCommandProcessorTests.cpp
    Gen2GetTests.cpp
    Gen2SetTests.cpp
    VssDatabaseTests.cpp
    VSSPathTests.cpp
    KuksavalUnitTest.cpp
  )

  # declares a test with our executable
  add_test(NAME ${UNITTEST_EXE_NAME}
    COMMAND $<TARGET_FILE:${UNITTEST_EXE_NAME}> --log_format=XML --log_sink=results.xml --log_level=all --report_level=no --result_code=no
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  target_compile_features(${UNITTEST_EXE_NAME} PRIVATE cxx_std_14)

  # disable warning, as we are not going to link unit-test library with anyone
  target_compile_options(${UNITTEST_EXE_NAME} PRIVATE -Wno-noexcept-type)

  # using shared library as it allows us to re-use test runner to verify different library versions, if needed
  target_link_libraries(${UNITTEST_EXE_NAME} PRIVATE "kuksa-val-server-core-static")

  target_include_directories(${UNITTEST_EXE_NAME} PRIVATE mock)
  target_include_directories(${UNITTEST_EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
  target_include_directories(${UNITTEST_EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include/interface)

  target_link_libraries(${UNITTEST_EXE_NAME} PRIVATE turtle)
  target_link_libraries(${UNITTEST_EXE_NAME} PRIVATE ${Boost_LIBRARIES})
  target_link_libraries(${UNITTEST_EXE_NAME} PRIVATE ${OPENSSL_LIBRARIES})
  target_link_libraries(${UNITTEST_EXE_NAME} PRIVATE jsoncons jwt-cpp)

  configure_file(test_vss_rel_2.0.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../certificates/jwt/jwt.key.pub ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endif(BUILD_UNIT_TEST)
