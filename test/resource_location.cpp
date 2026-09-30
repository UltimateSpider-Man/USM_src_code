#include <gtest/gtest.h>

#include <resource_location.h>

TEST(ResourceLocation, Construct)
{
    resource_location loc {};
    EXPECT_EQ(loc.m_size, 0);
}
