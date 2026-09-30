#include <gtest/gtest.h>

#include <deque.hpp>

TEST(StdDeque, Construct)
{
    _std::deque<int> d {};
    EXPECT_EQ(d.size(), 0);
}
