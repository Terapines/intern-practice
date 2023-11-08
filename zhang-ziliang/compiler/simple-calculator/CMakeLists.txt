cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(gtest-practice LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加库 
add_library(lib_calculate calculator.cpp)

# 添加可执行文件
add_executable(calculator main.cpp)

target_link_libraries(calculator lib_calculate)

# 添加可执行文件
add_executable(calculatorTest test.cpp)

# 添加gtest库的链接
find_package(GTest REQUIRED)
target_link_libraries(
    calculatorTest
PRIVATE
lib_calculate 
GTest::GTest 
GTest::Main
)

# 启用CTest
enable_testing()

# 将测试添加到CTest
add_test(
    NAME google_test
    COMMAND calculatorTest
)