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

cmake_minimum_required(VERSION 3.1.3)
project(w3c-visserver VERSION 2.0.0 LANGUAGES CXX)
enable_testing()

# Setup executable names
set(SERVER_EXE_NAME "w3c-visserver")
set(UNITTEST_EXE_NAME "w3c-unit-test")
set(TESTCLIENT_EXE_NAME "testclient")

set(UNIT_TEST ON CACHE STRING "Build unit tests")
# Set this variable to ON build an exe
set(BUILD_EXE ON)

#set this variable to ON to build the ${TESTCLIENT_EXE_NAME}
set(BUILD_TEST_CLIENT ON CACHE STRING "Build test client")

set(ADDRESS_SAN OFF CACHE STRING "Build with AddressSanitizer")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(UNIT_TEST)
   add_definitions(-DUNIT_TEST)
endif(UNIT_TEST)

# global
add_compile_options(-Wall -Wextra -Werror -g)

if ("${ADDRESS_SAN}" STREQUAL "ON" AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-g -fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-g -fsanitize=address -ldl)
endif()

# Get all standard source files
file(GLOB_RECURSE STANDARD_SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
# But exclude default main.cpp so we can re-use source list with other artifacts
list(REMOVE_ITEM STANDARD_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

######
# 3rd-party library includes already available in git

add_library(jsoncons INTERFACE)
target_include_directories(jsoncons SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs)

add_library(jsonpath INTERFACE)
target_include_directories(jsonpath SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jsoncons_ext)

add_library(jwt-cpp INTERFACE)
target_include_directories(jwt-cpp SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jwt-cpp)

######
# System library includes

# Prefer pthread as threading library
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED)
message(STATUS " boost includes ${Boost_INCLUDE_DIRS} ")

find_package(Boost 1.67.0 REQUIRED COMPONENTS system thread program_options filesystem system unit_test_framework)
message(STATUS " boost libs ${Boost_LIBRARIES} ")

#openssl
find_package(OpenSSL REQUIRED)
message(STATUS " openssl libraries ${OPENSSL_LIBRARIES}")

#glib, gobject
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GLIB glib-2.0)
PKG_CHECK_MODULES(GObject gobject-2.0)
PKG_CHECK_MODULES(GIO gio-2.0)

######
# Build unit test support

if(UNIT_TEST)
  set(TEST_FILES
      ${STANDARD_SRC_FILES}
      # ${CMAKE_CURRENT_SOURCE_DIR}/unit-test/VssDatabase_test.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/unit-test/w3cunittest.cpp
   )

   ######
   # Setup ${UNITTEST_EXE_NAME} with necessary links and includes

   add_executable(${UNITTEST_EXE_NAME} ${TEST_FILES})

   target_compile_features(${UNITTEST_EXE_NAME} PUBLIC cxx_std_14)

   target_include_directories(${UNITTEST_EXE_NAME} PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/interface>
   )

   target_link_libraries(${UNITTEST_EXE_NAME} LINK_PRIVATE jwt-cpp jsonpath jsoncons)

   target_link_libraries(${UNITTEST_EXE_NAME} LINK_PRIVATE Threads::Threads)

   target_link_libraries(${UNITTEST_EXE_NAME} LINK_PRIVATE ${Boost_LIBRARIES})
   target_include_directories(${UNITTEST_EXE_NAME} INTERFACE PUBLIC ${Boost_INCLUDE_DIR})

   target_link_libraries(${UNITTEST_EXE_NAME} LINK_PRIVATE ${OPENSSL_LIBRARIES})
   target_include_directories(${UNITTEST_EXE_NAME} INTERFACE PUBLIC ${OPENSSL_INCLUDE_DIR})

   target_include_directories(${UNITTEST_EXE_NAME} INTERFACE PUBLIC ${GLIB_INCLUDE_DIRS})
   add_definitions(${GLIB_CFLAGS_OTHER})

   target_include_directories(${UNITTEST_EXE_NAME} INTERFACE PUBLIC ${GObject_INCLUDE_DIR})
   target_link_libraries(${UNITTEST_EXE_NAME} LINK_PRIVATE ${GObject_LIBRARY_DIRS})

   target_include_directories(${UNITTEST_EXE_NAME} INTERFACE PUBLIC ${GIO_INCLUDE_DIR})
   target_link_libraries(${UNITTEST_EXE_NAME} LINK_PRIVATE ${GIO_LIBRARY_DIRS})

   target_link_libraries(${UNITTEST_EXE_NAME} LINK_PRIVATE gobject-2.0 glib-2.0 gio-2.0)
endif(UNIT_TEST)

######
# Build test client

if(BUILD_TEST_CLIENT)
   add_executable(${TESTCLIENT_EXE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/test/testclient.cpp)

   target_compile_features(${TESTCLIENT_EXE_NAME} PUBLIC cxx_std_14)

   target_link_libraries(${TESTCLIENT_EXE_NAME} LINK_PRIVATE jwt-cpp jsonpath jsoncons)
   target_include_directories(${TESTCLIENT_EXE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/jsoncons)
   target_link_libraries(${TESTCLIENT_EXE_NAME} LINK_PRIVATE ${Boost_LIBRARIES})
   target_include_directories(${TESTCLIENT_EXE_NAME} INTERFACE PUBLIC ${Boost_INCLUDE_DIR})
   target_link_libraries(${TESTCLIENT_EXE_NAME} LINK_PRIVATE ${OPENSSL_LIBRARIES})
   target_include_directories(${TESTCLIENT_EXE_NAME} INTERFACE PUBLIC ${OPENSSL_INCLUDE_DIR})

   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Client.pem  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Client.key  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/CA.pem  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endif(BUILD_TEST_CLIENT)

######
# Setup server build and artifacts

if(NOT BUILD_EXE)
   add_library(${SERVER_EXE_NAME} ${STANDARD_SRC_FILES})
else()
   add_executable(${SERVER_EXE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ${STANDARD_SRC_FILES})
endif(NOT BUILD_EXE)

target_compile_features(${SERVER_EXE_NAME} PUBLIC cxx_std_14)

target_include_directories(${SERVER_EXE_NAME}
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/interface>
)

target_link_libraries(${SERVER_EXE_NAME} LINK_PRIVATE jwt-cpp jsonpath jsoncons)

target_link_libraries(${SERVER_EXE_NAME} LINK_PRIVATE Threads::Threads)

target_link_libraries(${SERVER_EXE_NAME} LINK_PRIVATE ${Boost_LIBRARIES})
target_include_directories(${SERVER_EXE_NAME} INTERFACE PUBLIC ${Boost_INCLUDE_DIR})

target_link_libraries(${SERVER_EXE_NAME} LINK_PRIVATE ${OPENSSL_LIBRARIES})
target_include_directories(${SERVER_EXE_NAME} INTERFACE PUBLIC ${OPENSSL_INCLUDE_DIR})

target_include_directories(${SERVER_EXE_NAME} INTERFACE PUBLIC ${GLIB_INCLUDE_DIRS})
add_definitions(${GLIB_CFLAGS_OTHER})

target_include_directories(${SERVER_EXE_NAME} INTERFACE PUBLIC ${GObject_INCLUDE_DIR})
target_link_libraries(${SERVER_EXE_NAME} LINK_PRIVATE ${GObject_LIBRARY_DIRS})

target_include_directories(${SERVER_EXE_NAME} INTERFACE PUBLIC ${GIO_INCLUDE_DIR})
target_link_libraries(${SERVER_EXE_NAME} LINK_PRIVATE ${GIO_LIBRARY_DIRS})

target_link_libraries(${SERVER_EXE_NAME} LINK_PRIVATE gobject-2.0 glib-2.0 gio-2.0)

######
# Setup server install and packaging

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/jwt.pub.key  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Server.pem  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Server.key  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/unit-test/vss_rel_2.0.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

install( TARGETS ${SERVER_EXE_NAME} DESTINATION bin/${SERVER_EXE_NAME})
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/jwt.pub.key DESTINATION bin/${SERVER_EXE_NAME})
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Server.pem DESTINATION bin/${SERVER_EXE_NAME})
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Server.key DESTINATION bin/${SERVER_EXE_NAME})
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/unit-test/vss_rel_2.0.json DESTINATION bin/${SERVER_EXE_NAME})

# CPack
INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${SERVER_EXE_NAME}")
SET(CPACK_PACKAGE_VENDOR "Robert Bosch GmbH")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

SET(CPACK_GENERATOR "DEB;TGZ")
SET(CPACK_STRIP_FILES "bin/${SERVER_EXE_NAME}")
SET(CPACK_SOURCE_STRIP_FILES "")

SET(CPACK_PACKAGE_EXECUTABLES "${SERVER_EXE_NAME}" "W3C VSS server")

# debian
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.14), libgcc1 (>= 1:3.0), libssl1.0.0 (>= 1.0.1), libstdc++6 (>= 5.2)")
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "John Argerus")

INCLUDE(CPack)
