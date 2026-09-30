#include <gtest/gtest.h>

#include <resource_partition.h>

TEST(ResourcePartition, Construct)
{
    resource_partition part {RESOURCE_PARTITION_HERO};
#if 0
    EXPECT_EQ(part.get_type(), RESOURCE_PARTITION_HERO);
    EXPECT_EQ(part.get_buffer_used(), 0);
    EXPECT_EQ(part.get_buffer_size(), 0);
    EXPECT_TRUE(part.get_pack_slots().empty());
    EXPECT_TRUE(part.get_buffer() == nullptr);
#endif
}

TEST(ResourcePartition, PushPackSlot)
{
#if 0
    resource_partition part {RESOURCE_PARTITION_HERO};

    static constexpr auto buffer_size = 1572864u;
    uint8_t buffer[buffer_size] {};
    part.set_buffer(buffer);
    part.set_buffer_size(buffer_size);
    part.push_pack_slot(5000u, nullptr);
#endif
}
