#include <gtest/gtest.h>// 引入被测试的模板函数
#include "calculation.hpp"

// 测试 add 函数
TEST(TemplateFunctionsTest, AddTest) {
    EXPECT_EQ(add(2, 3), 5);
    EXPECT_EQ(add(0, 0), 0);
    EXPECT_EQ(add(-5, 5), 0);
}

// 测试 sub 函数
TEST(TemplateFunctionsTest, SubTest) {
    EXPECT_EQ(sub(5, 3), 2);
    EXPECT_EQ(sub(0, 0), 0);
    EXPECT_EQ(sub(-5, -5), 0);
}

// 测试 divide 函数
TEST(TemplateFunctionsTest, DivideTest) {
    EXPECT_EQ(divide(10, 2), 5);
    EXPECT_EQ(divide(-12, 4), -3);
    EXPECT_EQ(divide(0, 5), 0);
}

// 测试 rem 函数
TEST(TemplateFunctionsTest, RemTest) {
    EXPECT_EQ(rem(10, 3), 1);
    EXPECT_EQ(rem(8, 4), 0);
    EXPECT_EQ(rem(15, 4), 3);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}