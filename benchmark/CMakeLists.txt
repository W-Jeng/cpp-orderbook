cmake_minimum_required(VERSION 3.15)

project(orderbook_bench)

find_package(benchmark CONFIG REQUIRED)

add_executable(orderbook_bench main.cpp)
target_link_libraries(orderbook_bench PRIVATE benchmark::benchmark benchmark::benchmark_main)

