#include <gtest/gtest.h>

#include <resource_pack_slot.h>

TEST(ResourcePackDirectory, Construct)
{
    resource_pack_directory dir {};
    EXPECT_EQ(dir.field_0, nullptr);
}
