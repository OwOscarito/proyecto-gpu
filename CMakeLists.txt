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
FetchContent_Declare(
  CImg
  GIT_REPOSITORY https://github.com/GreycLab/CImg.git
  GIT_TAG v.4.0.2
  #FIND_PACKAGE_ARGS NAMES CImg
)
FetchContent_GetProperties(CImg)
if(NOT CImg_POPULATED)
  FetchContent_MakeAvailable(CImg)
  add_library(CImg INTERFACE)
  target_include_directories(CImg INTERFACE ${CImg_SOURCE_DIR})
endif()

### ----- OpenCL -----
find_package(OpenCL REQUIRED)

### ----- Executable -----
add_executable(ascii
    src/main.cpp
    src/ascii/cl_runner.cpp
)

target_link_libraries(ascii
  PRIVATE
    argparse::argparse
    OpenCL::OpenCL
)
