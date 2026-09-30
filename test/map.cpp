#include <gtest/gtest.h>

#include <map.hpp>

TEST(StdMap, Construct)
{
    _std::map<int, int> map {};
    EXPECT_EQ(map.size(), 0);
}

TEST(StdMap, Insert)
{
    _std::map<int, int> map {};
    auto it = map.insert(std::pair {0, 0});
    EXPECT_EQ(it.second, true);
    EXPECT_EQ(it.first, map.begin());

    EXPECT_EQ(map.size(), 1);
}
