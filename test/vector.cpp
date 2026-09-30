#include <gtest/gtest.h>

#include <vector.hpp>

TEST(StdVector, Construct)
{
    _std::vector<int> vec {};
    EXPECT_EQ(vec.size(), 0);
}

TEST(StdVector, PushBack)
{
    _std::vector<int> vec {};

    vec.push_back(10);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.front(), 10);
    EXPECT_EQ(vec.back(), 10);
}

TEST(StdVector, PopBack)
{
    _std::vector<int> vec(1);

    vec.pop_back();
    EXPECT_EQ(vec.size(), 0);
}
