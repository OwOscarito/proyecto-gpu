cmake_minimum_required(VERSION 3.24)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(proyecto-gpu
  LANGUAGES CXX
)

include(FetchContent)
FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
  GIT_TAG v3.2
)
FetchContent_MakeAvailable(argparse)

### ----- OpenCL -----
find_package(OpenCL REQUIRED)

### ----- Executable -----
add_executable(ascii
    src/main.cpp
    src/ascii/opencl.cpp
)

target_link_libraries(ascii
  PRIVATE
    argparse::argparse
    OpenCL::OpenCL
)
