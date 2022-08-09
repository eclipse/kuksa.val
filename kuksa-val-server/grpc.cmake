# ******************************************************************************
# Copyright (c) 2021 Robert Bosch GmbH and others.
#
# All rights reserved. This configuration file is provided to you under the
# terms and conditions of the Eclipse Distribution License v1.0 which
# accompanies this distribution, and is available at
# http://www.eclipse.org/org/documents/edl-v10.php
#
#  Contributors:
#      Robert Bosch GmbH 
# *****************************************************************************

set(gRPC_SSL_PROVIDER "package" CACHE STRING "Provider of ssl library")
set(GRPC_VER 1.44.0)

find_package(Protobuf)
find_package(gRPC ${GRPC_VER} EXACT CONFIG)

if(Protobuf_FOUND AND gRPC_FOUND)
    message(STATUS "Using protobuf ${protobuf_VERSION}")
    message(STATUS "Using gRPC ${gRPC_VERSION}")

    set(_GRPC_GRPCPP gRPC::grpc++)
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
    set(_REFLECTION gRPC::grpc++_reflection)
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

    set(_PROTO_INC "${PROTOBUF_INCLUDE_DIR}" )

else()
    ### grpc repository and submodule fetch
    include(FetchContent)

    ## GRPC ##
    FetchContent_Declare(
      gRPC
      GIT_REPOSITORY https://github.com/grpc/grpc
      GIT_TAG        v${GRPC_VER}
    )
    ## END GRPC ##

    set(FETCHCONTENT_QUIET OFF)
    ## Disable tests in GRPC                                                        
    set(BUILD_TESTING OFF) 
    FetchContent_MakeAvailable(gRPC)

    # gRPC and Protobuf variables
    set(_GRPC_GRPCPP grpc++)
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
    set(_REFLECTION grpc++_reflection)
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
    file(COPY "${CMAKE_BINARY_DIR}/_deps/grpc-src/third_party/protobuf/src/google" DESTINATION "${CMAKE_BINARY_DIR}/_deps/grpc-build/third_party/protobuf/")
    set(_PROTO_INC "./" )
endif()


#
# Generate Protobuf/Grpc source files
#
function(protobuf_gen)
    set(options)
    set(single_value OUTPUT PROTO PROTO_PATH PROTO_INC)
    set(multi_value)
    cmake_parse_arguments(ARG "${options}" "${single_value}" "${multi_value}" "${ARGN}")
    get_filename_component(filename ${ARG_PROTO} NAME_WE)
    set(PROTO_SRCS "${ARG_OUTPUT}/${filename}.pb.cc")
    set(PROTO_HDRS "${ARG_OUTPUT}/${filename}.pb.h")
    set(GRPC_SRCS "${ARG_OUTPUT}/${filename}.grpc.pb.cc")
    set(GRPC_HDRS "${ARG_OUTPUT}/${filename}.grpc.pb.h")
    set(GRPC_MOCK_HDRS "${ARG_OUTPUT}/${filename}_mock.grpc.pb.h")
    add_custom_command(
        OUTPUT "${PROTO_SRCS}" "${PROTO_HDRS}" "${GRPC_SRCS}" "${GRPC_HDRS}" "${GRPC_MOCK_HDRS}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "generate_mock_code=true:${ARG_OUTPUT}"
          --cpp_out "${ARG_OUTPUT}"
          -I "${ARG_PROTO_PATH}" 
          -I "${ARG_PROTO_INC}" 
          --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
          "${ARG_PROTO}"
        DEPENDS "${ARG_PROTO}"
    )
    set(PROTO_SRCS ${PROTO_SRCS} PARENT_SCOPE)
    set(PROTO_HDRS ${PROTO_HDRS} PARENT_SCOPE)
    set(GRPC_SRCS ${GRPC_SRCS} PARENT_SCOPE)
    set(GRPC_HDRS ${GRPC_HDRS} PARENT_SCOPE) 

    message(STATUS "generated *.pb.cc and *.pb.h files for ${ARG_PROTO}")
endfunction()
