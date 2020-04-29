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

cmake_minimum_required(VERSION 3.12.0)
project(w3c-visserver-api VERSION 2.0.1 LANGUAGES CXX)

#####
# Create buildinfo
#
set(PRE_CONFIGURE_FILE "buildinfo.h.in")
set(POST_CONFIGURE_FILE "buildinfo.h")
include(git_watcher.cmake)

######
# Root CMake file responsible for setting up mandatory dependencies
# and any other global information for 'w3c-visserver-api'

###
# Default option setup

# If not provided otherwise, set build type as 'Release'
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Default build type" FORCE)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


###
# System library includes

# Prefer pthread as threading library
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Boost
set(Boost_USE_STATIC_LIBS OFF)
set(MIN_BOOST_VER 1.67.0)

find_package(Boost ${MIN_BOOST_VER} EXACT REQUIRED COMPONENTS atomic date_time chrono program_options system thread unit_test_framework)
message(STATUS " boost includes ${Boost_INCLUDE_DIRS} ")
message(STATUS " boost libs ${Boost_LIBRARIES} ")

# OpenSSL
find_package(OpenSSL REQUIRED)
message(STATUS " openssl libraries ${OPENSSL_LIBRARIES}")

# glib, gobject
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GLIB glib-2.0)
PKG_CHECK_MODULES(GObject gobject-2.0)
PKG_CHECK_MODULES(GIO gio-2.0)

###
# 3rd-party library includes already available in git repo

add_library(jsoncons INTERFACE)
target_include_directories(jsoncons SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/3rd-party-libs)

add_library(jsonpath INTERFACE)
target_include_directories(jsonpath SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/3rd-party-libs/jsoncons_ext)

add_library(jwt-cpp INTERFACE)
target_include_directories(jwt-cpp SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/3rd-party-libs/jwt-cpp)

add_library(turtle INTERFACE)
target_include_directories(turtle SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/3rd-party-libs/turtle)

###
# Add project subdirectories to build

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(unit-test)
