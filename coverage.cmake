
macro(setup_coverage)
	if(NOT (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
		AND (NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
		AND (NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang"))
		message(FATAL_ERROR "Compiler ${CMAKE_C_COMPILER_ID} is not supporting coverage! Aborting...")
	endif()

	if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
		message(FATAL_ERROR "Code coverage results with an optimised (non-Debug) build may be misleading! Add -DCMAKE_BUILD_TYPE=Debug")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -coverage")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -coverage")
endmacro()

