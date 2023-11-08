## 编译原理练习：实现一个简易的计算器

本次练习使用了词法分析+语法分析制作了一个简易的算数解释器  
通过词法分析将输入代码分成多个token，再生成构建抽象语法树  
没有实现抽象语法树到IR的转换，而是直接转换为计算结果  
支持加+、减-、乘*、除/、取模% 运算  

使用方法：数字和符号之间用空格分隔，按q程序退出  

构建运行程序和测试程序：  
```bash
mkdir build
cd build
cmake ..
make
```

运行和测试结果：  
``` bash
$ ./calculator 
( 2 + 3 * 4 ) - ( 6 / 2 ) * ( 2 + 3 )
-1
( 8 + 4 ) * 2
24
( 5 - 3 ) / 2
1
( 10 + 5 ) / ( 3 - 1 )
7
( 2 + 3 ) * 4 / 2 - 1
9
( 10 - 6 ) * ( 2 + 3 ) / ( 4 + 2 )
3
25 % ( 3 * 4 ) + 2 * ( 7 - 2 )
11
( 10 + 5 ) * ( 6 - 3 ) - ( 8 % 3 )
43
q
```

更新： 使用gtest进行测试  
gtest测试结果：  
```bash
./calculatorTest 
[==========] Running 6 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 6 tests from calculatorTest
[ RUN      ] calculatorTest.AddTest
[       OK ] calculatorTest.AddTest (0 ms)
[ RUN      ] calculatorTest.SubTest
[       OK ] calculatorTest.SubTest (0 ms)
[ RUN      ] calculatorTest.DivideTest
[       OK ] calculatorTest.DivideTest (0 ms)
[ RUN      ] calculatorTest.MulTest
[       OK ] calculatorTest.MulTest (0 ms)
[ RUN      ] calculatorTest.RemTest
[       OK ] calculatorTest.RemTest (0 ms)
[ RUN      ] calculatorTest.MixedTest
[       OK ] calculatorTest.MixedTest (0 ms)
[----------] 6 tests from calculatorTest (0 ms total)

[----------] Global test environment tear-down
[==========] 6 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 6 tests.
```

如果在gtest使用时希望临时禁用某个测试，可以给测试加disabled前缀  
```cpp
TEST(calculatorTest, DISABLED_AddTest)
```
同样，可以使用ctest执行测试：  
```bash
$ ctest
Test project /home/zzl/Documents/intern-practice/zhang-ziliang/compiler/simple-calculator/build
    Start 1: google_test
1/1 Test #1: google_test ......................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.00 sec
```