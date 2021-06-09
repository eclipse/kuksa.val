file(GLOB PROTOBUF_DEFINITION_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
set(PROTOBUF_INPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/protos")
set(PROTOBUF_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/src")
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
foreach(file ${PROTOBUF_DEFINITION_FILES})
    execute_process (
        COMMAND bash -c "protoc --proto_path=\"${PROTOBUF_INPUT_DIRECTORY}\" --cpp_out=\"${PROTOBUF_OUTPUT_DIRECTORY}\" \"${file}\""
    )
    execute_process (
        COMMAND bash -c "protoc --plugin=protoc-gen-grpc=\"${_GRPC_CPP_PLUGIN_EXECUTABLE}\" --proto_path=\"${PROTOBUF_INPUT_DIRECTORY}\" --grpc_out=\"${PROTOBUF_OUTPUT_DIRECTORY}\" \"${file}\""
    )
endforeach()
message(STATUS "Generated helloworld.pb.h and helloworld.pb.c written to: \"${PROTOBUF_OUTPUT_DIRECTORY}\"")