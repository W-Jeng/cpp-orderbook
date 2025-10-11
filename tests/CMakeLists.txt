# GoogleTest setup
find_package(GTest CONFIG REQUIRED)

add_executable(all_tests 
    run_tests.cpp
    test_price_level.cpp
    test_orderbook.cpp
)
target_link_libraries(all_tests 
    PRIVATE 
    GTest::gtest 
    GTest::gtest_main 
    orderbook_lib
)

add_test(NAME SampleTest COMMAND sample)
