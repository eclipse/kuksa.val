#
# ******************************************************************************
# Copyright (c) 2019 Robert Bosch GmbH and others.
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
project(w3c-unit-test)

add_definitions(-DUNIT_TEST)

add_compile_options(-std=c++11 -pthread)

add_library(simple-websocket-server INTERFACE)

target_include_directories(simple-websocket-server INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd-party-libs ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd-party-libs/Simple-WebSocket-Server ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd-party-libs/jsoncons ${CMAKE_CURRENT_SOURCE_DIR}/../../3rd-party-libs/jwt-cpp)


find_package(Threads REQUIRED)
target_link_libraries(simple-websocket-server INTERFACE ${CMAKE_THREAD_LIBS_INIT})


set(TEST_SRC_FILES
         ${CMAKE_CURRENT_SOURCE_DIR}/../src/vssdatabase.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/../src/vsscommandprocessor.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/../src/accesschecker.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/../src/authenticator.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/../src/subscriptionhandler.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/../src/wsserver.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/w3cunittest.cpp
)
add_executable(${PROJECT_NAME} ${TEST_SRC_FILES})
target_include_directories(simple-websocket-server INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/Simple-WebSocket-Server ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jsoncons ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jwt-cpp)
target_link_libraries(${PROJECT_NAME}  simple-websocket-server)


# Boost
find_package(Boost 1.62.0 COMPONENTS filesystem system unit_test_framework system thread REQUIRED)
set(Boost_USE_STATIC_LIBS ON)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
message(STATUS " boost includes ${Boost_INCLUDE_DIRS} ")
target_link_libraries(simple-websocket-server INTERFACE ${Boost_LIBRARIES})
target_include_directories(simple-websocket-server INTERFACE ${Boost_INCLUDE_DIR})
message(STATUS " boost libs ${Boost_LIBRARIES} ")

#openssl
find_package(OpenSSL REQUIRED)
target_link_libraries(simple-websocket-server INTERFACE ${OPENSSL_LIBRARIES})
target_include_directories(simple-websocket-server INTERFACE ${OPENSSL_INCLUDE_DIR})
message(STATUS " openssl libraries ${OPENSSL_LIBRARIES}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../examples/demo-certificates/jwt.pub.key  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
