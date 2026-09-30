#include <gtest/gtest.h>

#include <mashable_vector.h>
#include <parse_generic_mash.h>

TEST(MashableVector, Construct)
{
    mashable_vector<int> vec {};
    EXPECT_EQ(vec.size(), 0);
}

TEST(MashableVector, UnMash)
{
    mashable_vector<int> vec {};

    constexpr auto size = 2u;

    vec.m_size = size;
    vec.field_7 = true;

    int buffer[32] {};
    new (buffer) int[size] {10, 11};

    generic_mash_data_ptrs ptrs {bit_cast<uint8_t *>(&buffer), bit_cast<uint8_t *>(&buffer)};

    vec.un_mash(nullptr, nullptr, &ptrs, nullptr);

    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 11);

    vec.at(0) = 12;
    vec.at(1) = 13;
    EXPECT_EQ(buffer[0], 12);
    EXPECT_EQ(buffer[1], 13);
}
