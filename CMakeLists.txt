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

cmake_minimum_required(VERSION 2.8)
project(w3c-visserver)

set(UNIT_TEST OFF CACHE STRING "Build unit tests")
# Set this variable to ON build an exe
set(BUILD_EXE ON)

#set this variable to ON to build the testclient
set(BUILD_TEST_CLIENT ON CACHE STRING "Build test client")

set(ADDRESS_SAN OFF CACHE STRING "Build with AddressSanitizer")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(UNIT_TEST)
   add_definitions(-DUNIT_TEST)
endif(UNIT_TEST)

add_definitions(-DDEBUG)
add_compile_options(-std=c++11 -pthread -Wall -Wextra -Werror)

if ("${ADDRESS_SAN}" STREQUAL "ON" AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-g -fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-g -fsanitize=address)
endif()


add_library(simple-websocket-server INTERFACE)

target_include_directories(simple-websocket-server
  SYSTEM INTERFACE # Set 3rd party libs as "system" in order to ignore warnings in them
  ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs
  ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/Simple-WebSocket-Server
  ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jsoncons
  ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jwt-cpp
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  # ${CMAKE_CURRENT_SOURCE_DIR}/unit-test
  )


find_package(Threads REQUIRED)
target_link_libraries(simple-websocket-server INTERFACE ${CMAKE_THREAD_LIBS_INIT})

# Boost
find_package(Boost REQUIRED)
set(Boost_USE_STATIC_LIBS ON)
include_directories(${Boost_INCLUDE_DIRS})
message(STATUS " boost includes ${Boost_INCLUDE_DIRS} ")

find_package(Boost 1.58.0 COMPONENTS system thread program_options REQUIRED)

target_link_libraries(simple-websocket-server INTERFACE ${Boost_LIBRARIES})
target_include_directories(simple-websocket-server INTERFACE ${Boost_INCLUDE_DIR})
message(STATUS " boost libs ${Boost_LIBRARIES} ")

#openssl
find_package(OpenSSL REQUIRED)
target_link_libraries(simple-websocket-server INTERFACE ${OPENSSL_LIBRARIES})
target_include_directories(simple-websocket-server INTERFACE ${OPENSSL_INCLUDE_DIR})

#glib, gobject
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GLIB glib-2.0)
PKG_CHECK_MODULES(GObject gobject-2.0)
PKG_CHECK_MODULES(GIO gio-2.0)

# Add to include paths
include_directories(${GLIB_INCLUDE_DIRS})
add_definitions(${GLIB_CFLAGS_OTHER})

include_directories(${GObject_INCLUDE_DIR})
link_directories(${GObject_LIBRARY_DIRS})

include_directories(${GIO_INCLUDE_DIR})
link_directories(${GIO_LIBRARY_DIRS})


message(STATUS " openssl libraries ${OPENSSL_LIBRARIES}")

file(GLOB_RECURSE SRC_FILES
${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

if(NOT BUILD_EXE)
   add_library(${PROJECT_NAME} ${SRC_FILES})
else()
   add_executable(${PROJECT_NAME} ${SRC_FILES})
endif(NOT BUILD_EXE)
target_link_libraries(${PROJECT_NAME}  simple-websocket-server)
#add glib libraries
target_link_libraries(${PROJECT_NAME} -lgobject-2.0 -lglib-2.0 -lgio-2.0)

if(UNIT_TEST)

   set(TEST_FILES
         ${CMAKE_CURRENT_SOURCE_DIR}/src/vssdatabase.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/vsscommandprocessor.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/accesschecker.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/authenticator.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/subscriptionhandler.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/signing.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/wsserver.cpp
         # ${CMAKE_CURRENT_SOURCE_DIR}/unit-test/vssdatabase_test.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/unit-test/w3cunittest.cpp
       )
   add_executable(w3c-unit-test ${TEST_FILES})
   target_include_directories(simple-websocket-server INTERFACE
     ${CMAKE_CURRENT_SOURCE_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs
     ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/Simple-WebSocket-Server
     ${CMAKE_CURRENT_SOURCE_DIR}/include
     ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jsoncons
     ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jwt-cpp)
   target_link_libraries(w3c-unit-test simple-websocket-server)
endif(UNIT_TEST)

if(UNIT_TEST)
find_package(Boost COMPONENTS filesystem system unit_test_framework REQUIRED)
target_include_directories(w3c-unit-test INTERFACE ${Boost_INCLUDE_DIR})
target_link_libraries(w3c-unit-test ${Boost_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
endif(UNIT_TEST)

if(UNIT_TEST)
target_include_directories(w3c-unit-test INTERFACE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(w3c-unit-test INTERFACE ${OPENSSL_LIBRARIES})
endif(UNIT_TEST)



if(BUILD_TEST_CLIENT)
   add_executable(testclient ${CMAKE_CURRENT_SOURCE_DIR}/test/testclient.cpp)
   target_link_libraries(testclient  simple-websocket-server)
   target_include_directories(testclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/jsoncons)
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Client.pem  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Client.key  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/CA.pem  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
endif(BUILD_TEST_CLIENT)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/jwt.pub.key  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Server.pem  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Server.key  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/unit-test/vss_rel_2.0.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

install( TARGETS ${PROJECT_NAME} DESTINATION bin/w3c-visserver)
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/jwt.pub.key DESTINATION bin/w3c-visserver)
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Server.pem DESTINATION bin/w3c-visserver)
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Server.key DESTINATION bin/w3c-visserver)
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/unit-test/vss_rel_2.0.json DESTINATION bin/w3c-visserver)

# CPack
INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "w3c-visserver")
SET(CPACK_PACKAGE_VENDOR "Robert Bosch GmbH")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

SET(CPACK_GENERATOR "DEB;TGZ")
SET(CPACK_STRIP_FILES "bin/w3c-visserver")
SET(CPACK_SOURCE_STRIP_FILES "")

SET(CPACK_PACKAGE_EXECUTABLES "w3c-visserver" "W3C VSS server")

# debian
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.14), libgcc1 (>= 1:3.0), libssl1.0.0 (>= 1.0.1), libstdc++6 (>= 5.2)")
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "John Argerus")

INCLUDE(CPack)
