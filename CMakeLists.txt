cmake_minimum_required(VERSION 3.10)

project(VideoReaderLib)

# Specify the required C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find FFmpeg libraries using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
    libswscale)

# Add pybind11 module
add_subdirectory(external/pybind11)

# Create a static library target
add_library(videoreader_static STATIC src/videoreader.cpp)
target_link_libraries(videoreader_static PRIVATE PkgConfig::FFMPEG)

# Python bindings as a shared library (module) linked against the static library
pybind11_add_module(videoreader_py MODULE src/videoreader_bindings.cpp)
target_link_libraries(videoreader_py PRIVATE videoreader_static)