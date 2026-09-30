#include <gtest/gtest.h>

#include <set.hpp>

TEST(StdSet, Construct)
{
    _std::set<int> s {};
    EXPECT_EQ(s.size(), 0);
}
