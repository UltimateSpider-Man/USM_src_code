#include <gtest/gtest.h>

#include <entity_class_entry.h>
#include <mash_info_struct.h>
#include <mvector.h>

struct from_mash_in_place_constructor;

struct A {
    A(from_mash_in_place_constructor *) {}
};

TEST(MVector, Construct)
{
    mVector<A> v {};
    EXPECT_EQ(v.size(), 0);
}


TEST(MVector, ConstructFromMash)
{
    mVector<A> v {};
    EXPECT_EQ(v.size(), 0);

    constexpr auto size = 10u;

    A * buffer[size] {new A {nullptr}};
    v.m_data = buffer;
    v.m_size = size;

    auto *v1 = new (&v) mVector<A> {static_cast<from_mash_in_place_constructor *>(nullptr)};
    EXPECT_EQ(v1->size(), 10);

}
