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

project(kuksa-val-server)
enable_testing()

######
# CMake configuration responsible for building core library and executable

###
# Setup target names
set(SERVER_EXE_NAME "${PROJECT_NAME}" )
set(SERVER_OBJ_LIB_NAME ${SERVER_EXE_NAME}-object )
set(SERVER_LIB_NAME ${SERVER_EXE_NAME}-core )
set(SERVER_LIB_STATIC_NAME ${SERVER_LIB_NAME}-static )


###
# Setup command-line options

# Set if executable should be built
set(BUILD_EXE ON CACHE BOOL "Build '${SERVER_EXE_NAME}' executable")
# Use address sanitizer when building with Clang
set(ADDRESS_SAN OFF CACHE BOOL "Build with AddressSanitizer")

###
# Define source list

set(LIB_SOURCES
   AccessChecker.cpp
   Authenticator.cpp
   BasicLogger.cpp
   JsonResponses.cpp
   permmclient.cpp
   RestV1ApiHandler.cpp
   SigningHandler.cpp
   SubscriptionHandler.cpp
   VSSRequestValidator.cpp
   VSSPath.cpp
   VssCommandProcessor.cpp
   VssCommandGet.cpp
   VssDatabase.cpp
   WebSockHttpFlexServer.cpp
   MQTTClient.cpp
)

###
# set main 'object library' as target to prevent multiple compilations of same sources
add_library(${SERVER_OBJ_LIB_NAME} OBJECT ${LIB_SOURCES})

target_compile_features(${SERVER_OBJ_LIB_NAME} PUBLIC cxx_std_14)

###
# Setup object library
target_include_directories(${SERVER_OBJ_LIB_NAME}
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/interface>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/interface>
)

if(CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(${SERVER_OBJ_LIB_NAME} PUBLIC -Wall -Wextra -Werror -Wno-error=maybe-uninitialized -Wno-error=parentheses)
else(CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(${SERVER_OBJ_LIB_NAME} PUBLIC -Wall -Wextra -Werror -Wno-error=parentheses)
endif(CMAKE_COMPILER_IS_GNUCXX)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "this should be deeeeeebuuuuuuuuuuuuuuuuugggggggggggggggggggggggg")
  target_compile_options(${SERVER_OBJ_LIB_NAME} PUBLIC -g -O0)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Coverage)
  if("${CMAKE_C_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    target_compile_options(${SERVER_OBJ_LIB_NAME} PUBLIC -g -O0 -fprofile-instr-generate -fcoverage-mapping)
    target_link_libraries(${SERVER_OBJ_LIB_NAME} PUBLIC -fprofile-instr-generate -fcoverage-mapping)
  elseif(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${SERVER_OBJ_LIB_NAME} PUBLIC -g -O0 --coverage -fprofile-arcs -ftest-coverage)
    target_link_libraries(${SERVER_OBJ_LIB_NAME} PUBLIC gcov)
  else()
    message(FATAL_ERROR "Code coverage requires Clang or GCC. Aborting.")
  endif()
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

if(BUILD_UNIT_TEST)
  target_compile_definitions(${SERVER_OBJ_LIB_NAME} PUBLIC UNIT_TEST)
endif(BUILD_UNIT_TEST)

# link library dependencies
target_link_libraries(${SERVER_OBJ_LIB_NAME} PUBLIC jwt-cpp jsonpath jsoncons)

if ("${ADDRESS_SAN}" STREQUAL "ON" AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_options(${SERVER_OBJ_LIB_NAME} PUBLIC -g -fsanitize=address -fno-omit-frame-pointer)
  target_link_libraries(${SERVER_OBJ_LIB_NAME} PUBLIC "-g -fsanitize=address ")
endif()

###
# Define common libraries

# shared and static libraries built from the same object files
add_library(${SERVER_LIB_NAME}        SHARED $<TARGET_OBJECTS:${SERVER_OBJ_LIB_NAME}>)
add_library(${SERVER_LIB_STATIC_NAME} STATIC $<TARGET_OBJECTS:${SERVER_OBJ_LIB_NAME}>)
# re-use include directories from object library
target_include_directories(${SERVER_LIB_NAME}        PUBLIC $<TARGET_PROPERTY:${SERVER_OBJ_LIB_NAME},INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(${SERVER_LIB_STATIC_NAME} PUBLIC $<TARGET_PROPERTY:${SERVER_OBJ_LIB_NAME},INTERFACE_INCLUDE_DIRECTORIES>)
# re-use link libraries from object library
target_link_libraries(${SERVER_LIB_NAME}        PUBLIC $<TARGET_PROPERTY:${SERVER_OBJ_LIB_NAME},INTERFACE_LINK_LIBRARIES>)
target_link_libraries(${SERVER_LIB_STATIC_NAME} PUBLIC $<TARGET_PROPERTY:${SERVER_OBJ_LIB_NAME},INTERFACE_LINK_LIBRARIES>)


if(BUILD_EXE)
  add_executable(${SERVER_EXE_NAME} main.cpp)
  # using static library to simplify existing packaging and maintainance
  target_link_libraries(${SERVER_EXE_NAME} PRIVATE ${SERVER_LIB_STATIC_NAME})

  # link executable dependencies
  target_link_libraries(${SERVER_EXE_NAME} PRIVATE Threads::Threads)
  target_include_directories(${SERVER_EXE_NAME} PRIVATE ${GLIB_INCLUDE_DIRS})
  target_link_libraries(${SERVER_EXE_NAME} PRIVATE ${GObject_LIBRARY_DIRS})
  target_include_directories(${SERVER_EXE_NAME} PRIVATE ${GIO_INCLUDE_DIR})
  target_link_libraries(${SERVER_EXE_NAME} PRIVATE ${GIO_LIBRARY_DIRS})
  target_link_libraries(${SERVER_EXE_NAME} PRIVATE gobject-2.0 glib-2.0 gio-2.0)
  target_link_libraries(${SERVER_EXE_NAME} PRIVATE ${Boost_LIBRARIES})
  target_link_libraries(${SERVER_EXE_NAME} PRIVATE ${OPENSSL_LIBRARIES})
  target_link_libraries(${SERVER_EXE_NAME} PRIVATE "-ldl")
  target_include_directories(${SERVER_EXE_NAME} PRIVATE ${MOSQUITTO_INCLUDE_DIR})
  target_link_libraries(${SERVER_EXE_NAME} PRIVATE ${MOSQUITTO_LIBRARY})

  ######
  # Setup server install and packaging

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../certificates/jwt/jwt.key.pub  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../certificates/Server.pem  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../certificates/Server.key  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../test/unit-test/vss_rel_2.0.json ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../test/unit-test/config.ini ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

  install( TARGETS ${SERVER_EXE_NAME} DESTINATION bin/${SERVER_EXE_NAME})
  install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../certificates/jwt/jwt.key.pub DESTINATION bin/${SERVER_EXE_NAME})
  install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../certificates/Server.pem DESTINATION bin/${SERVER_EXE_NAME})
  install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../certificates/Server.key DESTINATION bin/${SERVER_EXE_NAME})
  install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/../test/unit-test/vss_rel_2.0.json DESTINATION bin/${SERVER_EXE_NAME})

  # CPack
  INCLUDE(InstallRequiredSystemLibraries)

  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${SERVER_EXE_NAME}")
  SET(CPACK_PACKAGE_VENDOR "Robert Bosch GmbH")
  # Updating project version defined at the beggining of root CMakeLists.txt file shall be used in package build
  set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
  SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
  SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

  SET(CPACK_GENERATOR "DEB;TGZ")
  SET(CPACK_STRIP_FILES "bin/${SERVER_EXE_NAME}")
  SET(CPACK_SOURCE_STRIP_FILES "")

  SET(CPACK_PACKAGE_EXECUTABLES "${SERVER_EXE_NAME}" "W3C VSS server")

  # Debian specifics
  SET(DEPENDENCY_LIBRARY_LIST
    "libc6 (>= 2.14)"
    "libgcc1 (>= 1:3.0)"
    "libssl1.0.0 (>= 1.0.1)"
    "libstdc++6 (>= 5.2)"
  )

  STRING(REPLACE ";" ", " DEPENDENCIES "${DEPENDENCY_LIBRARY_LIST}")
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${DEPENDENCIES}")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "John Argerus")

  INCLUDE(CPack)
endif(BUILD_EXE)
