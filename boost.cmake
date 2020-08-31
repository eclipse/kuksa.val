# Copyright (c) 2019 Robert Bosch GmbH.

set(Boost_USE_STATIC_LIBS OFF)
set(BOOST_VER 1.67.0)
set(BOOST_COMPONENTS atomic date_time chrono filesystem program_options system thread)
find_package(Boost ${BOOST_VER} EXACT COMPONENTS ${BOOST_COMPONENTS} unit_test_framework)

if(NOT Boost_FOUND)
  string(REPLACE "." "_" BOOST_VER_ ${BOOST_VER}) 
  set(BOOST_URL "https://dl.bintray.com/boostorg/release/${BOOST_VER}/source/boost_${BOOST_VER_}.tar.bz2" CACHE STRING "Boost download URL")
  set(BOOST_URL_SHA256 "2684c972994ee57fc5632e03bf044746f6eb45d4920c343937a465fd67a5adba" CACHE STRING "Boost download URL SHA256 checksum")
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
  endif()
  find_package(Boost ${BOOST_VER} EXACT COMPONENTS ${BOOST_COMPONENTS})
endif()

message(STATUS " boost libs ${Boost_LIBRARIES} ")
message(STATUS " boost includes ${Boost_INCLUDE_DIRS} ")
include_directories(${Boost_INCLUDE_DIRS})
