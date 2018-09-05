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


# Set this variable to ON build an exe
set(BUILD_EXE ON)

#set this variable to ON to build the testclient
set(BUILD_TEST_CLIENT ON)



#Flag for csv implementation.
add_definitions(-DDEBUG)
add_compile_options(-std=c++11 -pthread)


add_library(simple-websocket-server INTERFACE)

target_include_directories(simple-websocket-server INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/Simple-WebSocket-Server ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jsoncons ${CMAKE_CURRENT_SOURCE_DIR}/../3rd-party-libs/jwt-cpp)


find_package(Threads REQUIRED)
target_link_libraries(simple-websocket-server INTERFACE ${CMAKE_THREAD_LIBS_INIT})

# Boost
find_package(Boost REQUIRED)
set(Boost_USE_STATIC_LIBS ON)
include_directories(${Boost_INCLUDE_DIRS})
message(STATUS " boost includes ${Boost_INCLUDE_DIRS} ")

find_package(Boost 1.54.0 COMPONENTS system thread REQUIRED)
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
endif(NOT EXE)
target_link_libraries(${PROJECT_NAME}  simple-websocket-server)

if(BUILD_TEST_CLIENT)
   add_executable(testclient ${CMAKE_CURRENT_SOURCE_DIR}/test/testclient.cpp)
   target_link_libraries(testclient  simple-websocket-server)
   target_include_directories(testclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/jsoncons)
endif(BUILD_TEST_CLIENT)

install( TARGETS ${PROJECT_NAME} DESTINATION /usr/bin/w3c-visserver)

