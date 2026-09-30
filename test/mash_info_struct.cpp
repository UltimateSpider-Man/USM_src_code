#include <gtest/gtest.h>

#include <mash_info_struct.h>

TEST(MashInfoStruct, Construct)
{
    constexpr auto size = 8;
    uint8_t buffer [size] {};
    mash_info_struct s {buffer, size};
    EXPECT_EQ(s.m_size, size);
}
