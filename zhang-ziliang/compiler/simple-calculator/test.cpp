#include"calculator.hpp"
#include<gtest/gtest.h>

// 测试 add 函数
TEST(calculatorTest,AddTest) {
    
    EXPECT_EQ(calculate("( 8 + 4 )"), 12);
    EXPECT_EQ(calculate("1 + 2 + 3"), 6);
    EXPECT_EQ(calculate("0 + 5 + 10"), 15);
}

// 测试 sub 函数
TEST(calculatorTest, SubTest) {
    EXPECT_EQ(calculate("( 8 - 4 )"), 4);
    EXPECT_EQ(calculate("1 - 2 - 3"), -4);
    EXPECT_EQ(calculate("10 - 5 - 1"), 4);
}

// 测试 divide 函数
TEST(calculatorTest, DivideTest) {
    EXPECT_EQ(calculate("( 8 / 4 )"), 2);
    EXPECT_EQ(calculate("8 / 2 / 2"), 2);
    EXPECT_EQ(calculate("10 / 3"), 3);
}

// 测试 mul 函数
TEST(calculatorTest, MulTest) {
    EXPECT_EQ(calculate("( 8 * 4 )"), 32);
    EXPECT_EQ(calculate("3 * 2 * 2"), 12);
    EXPECT_EQ(calculate("6 * 7"), 42);
}

// 测试 rem 函数
TEST(calculatorTest, RemTest) {
    EXPECT_EQ(calculate("( 8 % 4 )"), 0);
    EXPECT_EQ(calculate("10 % 3"), 1);
    EXPECT_EQ(calculate("23 % 7"), 2);
}

// 混合测试
TEST(calculatorTest, MixedTest) {
    EXPECT_EQ(calculate("( 2 + 3 * 4 ) - ( 6 / 2 ) * ( 2 + 3 )"), -1);
    EXPECT_EQ(calculate("( 8 + 4 ) * 2"), 24);
    EXPECT_EQ(calculate("( 5 - 3 ) / 2"), 1);
    EXPECT_EQ(calculate("( 10 + 5 ) / ( 3 - 1 )"), 7);
    EXPECT_EQ(calculate("( 2 + 3 ) * 4 / 2 - 1"), 9);
    EXPECT_EQ(calculate("( 10 - 6 ) * ( 2 + 3 ) / ( 4 + 2 )"), 3);
    EXPECT_EQ(calculate("25 % ( 3 * 4 ) + 2 * ( 7 - 2 )"), 11);
    EXPECT_EQ(calculate("( 10 + 5 ) * ( 6 - 3 ) - ( 8 % 3 )"), 43);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}