#include <gtest/gtest.h>

#include <hash_map.hpp>

TEST(StdHashMmap, Construct)
{
    stdext::hash_map<int, int> map {};
    EXPECT_EQ(map.size(), 0);
}

TEST(StdHashMap, Insert)
{
    stdext::hash_map<int, int> map {};
    auto it = map.insert(std::pair {0, 0});
    EXPECT_EQ(it.second, true);
    EXPECT_EQ(it.first, map.begin());

    EXPECT_EQ(map.size(), 1);
}

TEST(StdHashMap, Erase)
{
    stdext::hash_map<int, int> map {};
    map.insert(std::pair {0, 0});
    
    auto it = map.erase(map.begin());
    EXPECT_EQ(it, map.begin());
    EXPECT_EQ(map.size(), 0);
}
