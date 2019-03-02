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

set(UNIT_TEST OFF)
# Set this variable to ON build an exe
set(BUILD_EXE ON)

#set this variable to ON to build the testclient
set(BUILD_TEST_CLIENT ON)

if(UNIT_TEST)
   add_definitions(-DUNIT_TEST)
endif(UNIT_TEST)

add_definitions(-DDEBUG)
add_compile_options(-std=c++11 -pthread)


add_library(simple-websocket-server INTERFACE)

target_include_directories(simple-websocket-server INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/Simple-WebSocket-Server ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/unit-test ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jsoncons ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jwt-cpp)


find_package(Threads REQUIRED)
target_link_libraries(simple-websocket-server INTERFACE ${CMAKE_THREAD_LIBS_INIT})

# Boost
find_package(Boost REQUIRED)
set(Boost_USE_STATIC_LIBS ON)
include_directories(${Boost_INCLUDE_DIRS})
message(STATUS " boost includes ${Boost_INCLUDE_DIRS} ")

find_package(Boost 1.58.0 COMPONENTS system thread REQUIRED)

target_link_libraries(simple-websocket-server INTERFACE ${Boost_LIBRARIES})
target_include_directories(simple-websocket-server INTERFACE ${Boost_INCLUDE_DIR})
message(STATUS " boost libs ${Boost_LIBRARIES} ")

#openssl
find_package(OpenSSL REQUIRED)
target_link_libraries(simple-websocket-server INTERFACE ${OPENSSL_LIBRARIES})
target_include_directories(simple-websocket-server INTERFACE ${OPENSSL_INCLUDE_DIR})



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

if(UNIT_TEST)

   set(TEST_FILES
         ${CMAKE_CURRENT_SOURCE_DIR}/src/vssdatabase.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/vsscommandprocessor.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/accesschecker.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/authenticator.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/subscriptionhandler.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/src/wsserver.cpp
       )
   add_executable(w3c-unit-test ${TEST_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/unit-test/w3c-viss-test.cpp)
   target_include_directories(simple-websocket-server INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/Simple-WebSocket-Server ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jsoncons ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jwt-cpp)
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

install( TARGETS ${PROJECT_NAME} DESTINATION /usr/bin/w3c-visserver)
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/jwt.pub.key DESTINATION /usr/bin/w3c-visserver)
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Server.pem DESTINATION /usr/bin/w3c-visserver)
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../examples/demo-certificates/Server.key DESTINATION /usr/bin/w3c-visserver)

