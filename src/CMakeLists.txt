add_executable(bench_insert_s
    bench_insert_s.cpp
)

add_executable(bench_insert
    bench_insert.cpp
)

add_executable(bench_match20_s
    bench_match20_s.cpp
)

target_include_directories(bench_insert PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(bench_insert_s PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(bench_match20_s PRIVATE ${PROJECT_SOURCE_DIR}/include)


