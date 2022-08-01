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

project(w3c-testclient)

######
# CMake configuration responsible for building kuksa-val optional test client

###
# Setup target names
set(TESTCLIENT_EXE_NAME "testclient" )

###
# Setup command-line options


###
# Setup targets

if(BUILD_TEST_CLIENT)
  add_executable(${TESTCLIENT_EXE_NAME} testclient.cpp)

  target_compile_features(${TESTCLIENT_EXE_NAME} PUBLIC cxx_std_14)

  target_link_libraries(${TESTCLIENT_EXE_NAME} PRIVATE Threads::Threads)
  target_link_libraries(${TESTCLIENT_EXE_NAME} PRIVATE jwt-cpp jsonpath jsoncons)
  target_link_libraries(${TESTCLIENT_EXE_NAME} PRIVATE ${Boost_LIBRARIES})
  target_link_libraries(${TESTCLIENT_EXE_NAME} PRIVATE ${OPENSSL_LIBRARIES})

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../kuksa_certificates/Client.pem  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../kuksa_certificates/Client.key  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../../kuksa_certificates/CA.pem  ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

endif(BUILD_TEST_CLIENT)
