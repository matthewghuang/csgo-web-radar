cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

project(csgo_web_radar LANGUAGES C CXX)

set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_FLAGS_RELEASE "-Oz")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(provider src/main.cpp src/process.cpp src/easywsclient/easywsclient.cpp)
