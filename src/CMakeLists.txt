add_executable(bench_insert_s
    bench_insert_s.cpp
)

add_executable(bench_insert_m
    bench_insert_m.cpp
)

add_executable(bench_match20_s
    bench_match20_s.cpp
)

add_executable(bench_match20_m
    bench_match20_m.cpp
)

target_include_directories(bench_insert_m PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(bench_insert_s PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(bench_match20_s PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(bench_match20_m PRIVATE ${PROJECT_SOURCE_DIR}/include)


