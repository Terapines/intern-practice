## 4.1 创建一个简单的单元测试
核心代码：  
- enable_testing()
- add_test(
NAME cpp_test
COMMAND \$<TARGET_FILE:cpp_test>
)  
使用方法：  
- cmake ..
- cmake --build .
- ctest  
在该小节中，通过测试程序的返回值来告诉ctest测试结果，返回0即为通过测试  
## 4.2 使用Catch2库进行单元测试  
Catch2的特点是只有单个头文件  
代码：  
- add_test(
  NAME catch_test
  COMMAND \$<TARGET_FILE:cpp_test> --success
  )  
在使用ctest时可以添加-V选项查看详细输出信息：  
- ctest -V  
上述代码在执行ctest后生效，相当于执行以下指令：  
- ./cpp_test --success  

## 4.3 使用Google Test库进行单元测试  
在这里使用了一种新方法导入googletest库，使用FetchContent模块来从github下载：  
- include(FetchContent)

- FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.8.0
  )  

## 4.4 使用Boost Test进行单元测试  
使用find_package导入boost库，其他同上：    
-find_package(Boost 1.54 REQUIRED COMPONENTS unit_test_framework)  

## 4.5 使用动态分析来检测内存缺陷  
使用find_program模块来查找valgrind程序，再配合ctest的选项就可以通过cmake使用valgrind  
代码：  
 - find_program(MEMORYCHECK_COMMAND NAMES valgrind)
 - set(MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full")

 使用方法：  
 - ctest -T memcheck  

## 4.6 预期测试失败  
用来设置预期结果即为失败的测试，若测试程序失败，即通过测试：  
- set_tests_properties(example PROPERTIES WILL_FAIL true)  

## 4.7 使用超时测试运行时间过长的测试  
即设置超时时间，超时即测试不通过：  
 - set_tests_properties(example PROPERTIES TIMEOUT 10)

## 4.8 并行测试  
使用方法：  
- ctest --parallel 4  
或使用环境变量：  
- CTEST_PARALLEL_LEVEL  

## 4.9 运行测试子集  
核心代码：  
set_tests_properties(
  feature-a
  feature-b
  feature-c
  PROPERTIES
    LABELS "quick"
  )

set_tests_properties(
  feature-d
  benchmark-a
  benchmark-b
  PROPERTIES
    LABELS "long"
  )

相当于有一个分组的功能，可以给每个组打一个标签，在运行ctest后会按标签进行统计  

## 4.10 使用测试固件  
1.给setup这个测试添加一个FIXTURES_SETUP属性：  
set_tests_properties(
  setup
  PROPERTIES
    FIXTURES_SETUP my-fixture
  )  

2.给两个feature测试添加属性：  
set_tests_properties(
  feature-a
  feature-b
  PROPERTIES
    FIXTURES_REQUIRED my-fixture
  )  

3.给cleanup添加属性：  
set_tests_properties(
  cleanup
  PROPERTIES
    FIXTURES_CLEANUP my-fixture
  )  