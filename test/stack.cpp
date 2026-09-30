#include <gtest/gtest.h>

#include <stack.hpp>

TEST(StdStack, Construct)
{
    _std::stack<int> stack {};
    EXPECT_EQ(stack.size(), 0);
}

TEST(StdStack, Push)
{
    _std::stack<int> stack {};

    stack.push(10);
    EXPECT_EQ(stack.size(), 1);
}

TEST(StdStack, Pop)
{
    _std::stack<int> stack {};
    stack.push(10);

    stack.pop();
    EXPECT_EQ(stack.size(), 0);
}
