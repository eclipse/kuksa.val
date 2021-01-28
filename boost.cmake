#
# ******************************************************************************
# Copyright (c) 2020 Robert Bosch GmbH and others.
#
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v2.0
# which accompanies this distribution, and is available at
# https://www.eclipse.org/org/documents/epl-2.0/index.php
#
#  Contributors:
#      Robert Bosch GmbH 
# *****************************************************************************

set(Boost_USE_STATIC_LIBS OFF)
set(BOOST_VER 1.75.0)
set(Boost_NO_BOOST_CMAKE ON)
set(BOOST_COMPONENTS filesystem program_options system)

# Workaround function to allow cmake call `find_package` twice. Avoide side effects from local variables, which are produced be `find_package`
function(findBoost Required)
    find_package(Boost ${BOOST_VER} EXACT ${Required} 
        COMPONENTS ${BOOST_COMPONENTS}
        OPTIONAL_COMPONENTS unit_test_framework
    )
    set(Boost_FOUND ${Boost_FOUND} PARENT_SCOPE)
    set(Boost_LIBRARIES ${Boost_LIBRARIES} PARENT_SCOPE)
    set(Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} PARENT_SCOPE)
endfunction()

findBoost("")

if(NOT Boost_FOUND)
  string(REPLACE "." "_" BOOST_VER_ ${BOOST_VER}) 
  set(BOOST_URL "https://dl.bintray.com/boostorg/release/${BOOST_VER}/source/boost_${BOOST_VER_}.tar.bz2" CACHE STRING "Boost download URL")
  set(BOOST_URL_SHA256 "953db31e016db7bb207f11432bef7df100516eeb746843fa0486a222e3fd49cb" CACHE STRING "Boost download URL SHA256 checksum")
  option(BOOST_DISABLE_TESTS "Do not build test targets" OFF)
  include(FetchContent)
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_Declare(
    Boost
    URL ${BOOST_URL}
    URL_HASH SHA256=${BOOST_URL_SHA256}
  )
  FetchContent_GetProperties(Boost)
  set(FETCHCONTENT_QUIET OFF)
  if(NOT Boost_POPULATED)
    message(STATUS "Fetching Boost")
    FetchContent_Populate(Boost)
    message(STATUS "Fetching Boost - done")
    message(STATUS " boost source dir is ${boost_SOURCE_DIR} ")
    message(STATUS " boost source dir is ${boost_BINARY_DIR} ")
    string(JOIN "," BOOST_WITH_LIBRARIES ${BOOST_COMPONENTS}) 
    execute_process(
      COMMAND ./bootstrap.sh --prefix=${boost_BINARY_DIR} --with-libraries=${BOOST_WITH_LIBRARIES}
      WORKING_DIRECTORY ${boost_SOURCE_DIR}
      RESULT_VARIABLE result
    )
    if(NOT result EQUAL "0")
      message( FATAL_ERROR "Bad exit status of bootstrap")
    endif()
    execute_process(
      COMMAND ./b2 install -j8
      WORKING_DIRECTORY ${boost_SOURCE_DIR}
      RESULT_VARIABLE result
    )
    if(NOT result EQUAL "0")
      message( FATAL_ERROR "Bad exit status of b2")
    endif()
    set(BOOST_ROOT ${boost_BINARY_DIR} CACHE PATH "Root folder to find boost" FORCE)
    set(Boost_DIR ${boost_BINARY_DIR} CACHE PATH "Root folder to find boost" FORCE)
  endif()
message("ROOT ${BOOST_ROOT}")
    #find_package(Boost ${BOOST_VER} EXACT REQUIRED COMPONENTS ${BOOST_COMPONENTS} OPTIONAL_COMPONENTS unit_test_framework PATHS ${BOOST_ROOT} NO_SYSTEM_PATH)
    findBoost(REQUIRED)
endif()

message(STATUS " boost libs ${Boost_LIBRARIES}")
message(STATUS " boost includes ${Boost_INCLUDE_DIRS}")
include_directories(${Boost_INCLUDE_DIRS})
