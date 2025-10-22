add_executable(main
    main.cpp
)

add_executable(main_s
    main_single.cpp
)

target_include_directories(main PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(main_s PRIVATE ${PROJECT_SOURCE_DIR}/include)


