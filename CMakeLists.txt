cmake_minimum_required(VERSION 3.24)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
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

### ----- stb -----
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

### ----- OpenCL -----
find_package(OpenCL 3.0 REQUIRED)

### ----- Executable -----
add_executable(ascii
    src/main.cpp
    src/ascii/ascii_art.cpp
    src/ascii/cl_runner.cpp
    src/ascii/cpu_runner.cpp
)
target_link_libraries(ascii
  PRIVATE
    argparse::argparse
    OpenCL::OpenCL
)
target_include_directories(ascii
  SYSTEM PRIVATE
  ${stb_SOURCE_DIR})

target_compile_definitions(ascii
      PRIVATE
          CL_TARGET_OPENCL_VERSION=300
  )
