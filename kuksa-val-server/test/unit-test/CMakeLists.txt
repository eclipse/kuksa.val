#
# ******************************************************************************
# Copyright (c) 2018-2022 Robert Bosch GmbH and others.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#
#  Contributors:
#      Robert Bosch GmbH - initial API and functionality
# *****************************************************************************

project(kuksa-val-unit-test)
enable_testing()
include(CTest)
include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.11.0
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

######
# CMake configuration responsible for building kuksa-val optional unit tests based on core library

###
# Setup target names
set(UNITTEST_EXE_NAME "kuksaval-unit-test" )

set(BUILD_UNIT_TEST OFF CACHE BOOL "Build unit tests")

###
# Setup targets

set(proto_gen_dir "${CMAKE_BINARY_DIR}/proto")
include_directories(${proto_gen_dir})
include_directories(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

if(BUILD_UNIT_TEST)
  add_executable(${UNITTEST_EXE_NAME}
    AccessCheckerTests.cpp
    AuthenticatorTests.cpp
    SubscriptionHandlerTests.cpp
    VssCommandProcessorTests.cpp
    Gen2GetTests.cpp
    Gen2SetTests.cpp
    VSSTypeSanitizerTests.cpp
    VssDatabaseTests.cpp
    VSSPathTests.cpp
    KuksavalUnitTest.cpp
    UpdateVSSTreeTest.cpp
    UpdateMetadataTest.cpp
  )

  # declares a test with our executable
  set(CTEST_RESULT_CODE yes CACHE BOOL "Build unit tests")
  add_test(NAME ${UNITTEST_EXE_NAME}
    COMMAND $<TARGET_FILE:${UNITTEST_EXE_NAME}> --log_format=XML --log_sink=results.xml --log_level=all --report_level=short --result_code=${CTEST_RESULT_CODE}
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

  target_link_libraries(${UNITTEST_EXE_NAME} PRIVATE turtle gmock_main gtest)
  target_link_libraries(${UNITTEST_EXE_NAME} PRIVATE ${Boost_LIBRARIES})
  target_link_libraries(${UNITTEST_EXE_NAME} PRIVATE ${OPENSSL_LIBRARIES})
  target_link_libraries(${UNITTEST_EXE_NAME} PRIVATE jsoncons jwt-cpp gtest_main gmock)

# Copy all files needed for tests, in general no problem to use older versions as long as KUKSA.val supports that version
# Unless required for other reasons tests shall use "test_vss_release_latest.json" so that we always test against latest released version
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../../data/vss-core/vss_release_3.0.json ${CMAKE_CURRENT_BINARY_DIR}/test_vss_release_latest.json COPYONLY)
# If older versions needs to be used in test then include them as "test_vss_release_X.Y.json"

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../../kuksa_certificates/jwt/jwt.key.pub ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
  if (ENABLE_COVERAGE)
    add_coverage(${UNITTEST_EXE_NAME})
  endif()
endif(BUILD_UNIT_TEST)
