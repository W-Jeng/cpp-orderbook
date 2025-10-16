cmake_minimum_required(VERSION 3.20)
project(cpp-orderbook VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")

add_library(orderbook_lib INTERFACE)
target_include_directories(orderbook_lib INTERFACE include)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(benchmark)
