cmake_minimum_required(VERSION 3.10)
project(RiscEncoder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)

add_library(encoder_lib src/encoder.cpp src/utilits.cpp)

target_include_directories(encoder_lib PUBLIC include)

add_executable(risc_encoder src/main.cpp)
target_link_libraries(risc_encoder encoder_lib nlohmann_json::nlohmann_json)

enable_testing()
add_executable(unit_tests tests/encoder_test.cpp)
target_link_libraries(unit_tests 
    encoder_lib 
    nlohmann_json::nlohmann_json 
    GTest::gtest 
    GTest::gtest_main
)

add_test(NAME AllTests COMMAND unit_tests)