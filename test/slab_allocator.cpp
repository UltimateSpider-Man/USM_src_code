#include <gtest/gtest.h>

#include <slab_allocator.h>

TEST(SlabAllocator, Initialize)
{
    slab_allocator::initialize();

    EXPECT_TRUE(slab_allocator::initialized);
    EXPECT_TRUE(slab_allocator::slab_partial_list != nullptr);
    EXPECT_TRUE(slab_allocator::slab_full_list != nullptr);
    EXPECT_TRUE(slab_allocator::slab_free_list != nullptr);
    EXPECT_TRUE(slab_allocator::static_slab_arena != nullptr);
    EXPECT_TRUE(slab_allocator::static_slab_headers != nullptr);

    for (auto count : slab_allocator::free_object_count) {
        EXPECT_EQ(count, 0);
    }
}

TEST(SlabAllocator, CreateSlab)
{
    auto *slab = slab_allocator::create_slab(10);

    auto index = slab_allocator::get_index(10);
    EXPECT_EQ(slab_allocator::free_slab_count, -1);
    EXPECT_EQ(slab_allocator::total_slab_count, 0);

    EXPECT_EQ(slab_allocator::free_object_count[index], slab->get_total_object_count());
    EXPECT_EQ(slab_allocator::partial_slab_count[index], 1);
}

struct A {
    char field_0[10];
};

void *mem = nullptr;

TEST(SlabAllocator, Allocate)
{
    mem = slab_allocator::allocate(sizeof(A), nullptr);
    EXPECT_TRUE(mem != nullptr);

    auto index = slab_allocator::get_index(sizeof(A));
    EXPECT_TRUE(index >= 0);

    auto *slab = slab_allocator::find_slab_for_object(mem);
    EXPECT_TRUE(slab != nullptr);

    EXPECT_EQ(slab->get_alloc_object_count(), 1);

    EXPECT_EQ(slab_allocator::allocated_object_count[index], 1);
    EXPECT_EQ(slab_allocator::free_object_count[index], slab->get_total_object_count() - 1);

    EXPECT_EQ(slab_allocator::partial_slab_count[index], 1);
    EXPECT_EQ(slab_allocator::full_slab_count[index], 0);
}

TEST(SlabAllocator, Deallocate)
{
    slab_allocator::deallocate(mem, nullptr);

    auto index = slab_allocator::get_index(sizeof(A));

    auto *slab = slab_allocator::find_slab_for_object(mem);
    EXPECT_TRUE(slab != nullptr);

    EXPECT_EQ(slab->get_alloc_object_count(), 0);

    EXPECT_EQ(slab_allocator::allocated_object_count[index], 0);
    EXPECT_EQ(slab_allocator::free_object_count[index], 0);

    EXPECT_EQ(slab_allocator::partial_slab_count[index], 0);
    EXPECT_EQ(slab_allocator::full_slab_count[index], 0);
}

TEST(SlabAllocator, CreateObject)
{
    auto *slab = slab_allocator::find_slab_for_object(mem);
    EXPECT_TRUE(slab != nullptr);

    auto *obj = slab->create_object();
    EXPECT_TRUE(obj != nullptr);

    EXPECT_EQ(slab->get_alloc_object_count(), 1);
}

TEST(SlabAllocator, FreeObject)
{
    auto *slab = slab_allocator::find_slab_for_object(mem);
    EXPECT_TRUE(slab != nullptr);

    slab->free_object(mem);
    EXPECT_EQ(slab->get_alloc_object_count(), 0);
}
