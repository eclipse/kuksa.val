# Advanced kuksa.val cmake information

For a simple compilation howto see the [main readme](../README.md). Here we have collected some additional details regarding our cmake setup, that are not needed if you "just want to compile".


## CMake Project Structure

Structure of CMake project is organized as a tree. This keeps different modules simple and logically separated, but allows for easy re-use and extension.

Root [CMakeLists.txt](../CMakeLists.txt) CMake file defines root project and its common properties, like dependency verification and common library definition.
Adding any new dependency or new module to the project is done in this file.
Each project module is responsible for its own build configuration options and it should not be handled in root CMakeLists.txt file.

Each project module can have different user-configurable build configuration options. To list currently available build options go to [ List build configuration options](#List-build-configuration-options) chapter.

Current project has three existing modules which can serve as an example and are defined in:
 - [src/CMakeLists.txt](../src/CMakeLists.txt) - Main module which defines W3C-Server library and executable.
- [test/CMakeLists.txt](../test/CMakeLists.txt) - Module defining testclient executable as a helper tool for exercising W3C-Server.
- [unit-test/CMakeLists.txt](../unit-test/CMakeLists.txt) - Module defining unit-test executable for testing build W3C-Server library.


## Configure CMake project

Build configuration options of W3C-Server are defined in different CMakeLists.txt files, depending on each included module in build.

### List build configuration options

To list all available build options, invoke below CMake command from build directory:
```
cmake -LH ..
```
Provided command shall list cached CMake variables that are available for configuration.

Alternatively, one can use [cmake-gui](https://cmake.org/cmake/help/v3.1/manual/cmake-gui.1.html) to query and|or configure project through UI.




### Main build configuration options

By changing values of different configuration options, user can control
build of W3C-Server.  
Available build options with optional parameters, if available, are presented below. Default parameters are shown in **bold**:
 - **CMAKE_BUILD_TYPE** [**Release**/Debug/Coverage] - Build type. Available options:
   * **_Release_** - Default build type. Enabled optimizations and no debug information is available.
   * **_Debug_** - Debug information is available and optimization is disabled.
   * **_Coverage_** - Coverage information is generated with debug information and reduced optimization levels for better reporting. Check [_Coverage_](#Coverage) chapter for more details.
 - **BUILD_EXE** [**ON**/OFF] - Default build shall produce W3C-Server executable.
   If set to **OFF** W3C-Server shall be built as a library which could be used in another application.
   Eg: could be found in the _vehicle2cloud_ app.
 - **BUILD_TEST_CLIENT** [**ON**/OFF] - Build separate _testclient_ executable. Test client is a utility to test
   Web-Socket request interface of W3C-Server and retrieve responses.
 - **BUILD_UNIT_TEST** [ON/**OFF**] - If enabled, build shall produce separate _w3c-unit-test_ executable which
   will run existing tests for server implementation.
 - **ADDRESS_SAN** [ON/**OFF**] - If enabled and _Clang_ is used as compiler, _AddressSanitizer_ will be used to build
   W3C-Server for verifying run-time execution.

An example of W3C-Server CMake build configuration invocation with enabled unit test is shown below (e.g. used when debugging unit-tests):
```
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_UNIT_TEST=ON ..
```


