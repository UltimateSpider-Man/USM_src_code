#include <gtest/gtest.h>

#include <list.hpp>

TEST(StdList, Construct)
{
    _std::list<int> list {};
    EXPECT_EQ(list.size(), 0);
}

TEST(StdList, PushBack)
{
    _std::list<int> list {};

    list.push_back(10);
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list.front(), 10);
    EXPECT_EQ(list.back(), 10);
}

TEST(StdList, PopBack)
{
    _std::list<int> list {};
    list.push_back(1);

    list.pop_back();
    EXPECT_EQ(list.size(), 0);
}

TEST(StdList, PopFront)
{
    _std::list<int> list {};
    list.push_back(1);

    list.pop_front();
    EXPECT_EQ(list.size(), 0);
}

TEST(StdList, Clear)
{
    _std::list<int> list {};
    list.push_back(1);

    list.clear();
    EXPECT_EQ(list.size(), 0);
}
