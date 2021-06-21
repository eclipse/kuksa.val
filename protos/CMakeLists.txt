file(GLOB PROTOBUF_DEFINITION_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")
set(PROTOBUF_INPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/protos")
set(PROTOBUF_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/src")
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
foreach(file ${PROTOBUF_DEFINITION_FILES})
    execute_process (
        COMMAND bash -c "${Protobuf_PROTOC_EXECUTABLE} --proto_path=\"${PROTOBUF_INPUT_DIRECTORY}\" --cpp_out=\"${PROTOBUF_OUTPUT_DIRECTORY}\" \"${file}\""
    )
    execute_process (
        COMMAND bash -c "${Protobuf_PROTOC_EXECUTABLE} --plugin=protoc-gen-grpc=\"${_GRPC_CPP_PLUGIN_EXECUTABLE}\" --proto_path=\"${PROTOBUF_INPUT_DIRECTORY}\" --grpc_out=\"${PROTOBUF_OUTPUT_DIRECTORY}\" \"${file}\""
    )
    message(STATUS "Generated *.pb.c *.pb.h for ${file}")
    message(STATUS "Files have been written to: \"${PROTOBUF_OUTPUT_DIRECTORY}\"")
endforeach()