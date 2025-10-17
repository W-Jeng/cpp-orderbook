find_package(benchmark CONFIG REQUIRED)

add_executable(orderbook_bench main.cpp)
target_link_libraries(orderbook_bench PRIVATE benchmark::benchmark benchmark::benchmark_main)
target_include_directories(orderbook_bench PRIVATE ${CMAKE_SOURCE_DIR}/include)
