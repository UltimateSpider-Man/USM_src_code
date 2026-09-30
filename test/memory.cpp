#include <gtest/gtest.h>

#include <memory.h>

TEST(Memory, Malloc)
{
    auto *mem = arch_malloc(1);
    EXPECT_TRUE(mem != nullptr);
}
