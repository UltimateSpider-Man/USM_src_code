#include <gtest/gtest.h>

#include "../src/msimpletemplates.h"

struct A {
    simple_list<A *>::vars_t simple_list_vars {};
};

TEST(SimpleList, Construct)
{
    simple_list<A *> list {};
    EXPECT_EQ(list.size(), 0);
}

TEST(SimpleList, PushBack)
{
    simple_list<A *> list {};

    A a {};
    list.push_back(&a);

    A b {};
    list.push_back(&b);

    EXPECT_EQ(list.size(), 2);

    auto it = list.begin();
    EXPECT_EQ((*it), &a);

    ++it;
    EXPECT_EQ((*it), &b);
}

TEST(SimpleList, PushFront)
{
    simple_list<A *> list {};

    A a {};
    list.push_front(&a);

    A b {};
    list.push_front(&b);

    EXPECT_EQ(list.size(), 2);

    auto it = list.begin();
    EXPECT_EQ((*it), &b);

    ++it;
    EXPECT_EQ((*it), &a);
}

TEST(SimpleList, Erase)
{
    simple_list<A *> list {};

    auto begin = list.begin();

    A a {};
    list.push_back(&a);

    A b {};
    list.push_back(&b);

    EXPECT_EQ(list.erase(&b), begin);
    EXPECT_EQ(list.erase(&a), begin);
}


TEST(SimpleList, CheckedErase)
{
    simple_list<A *> list {};

    A a {};

    EXPECT_FALSE(list.checked_erase(&a));

    list.push_back(&a);

    EXPECT_TRUE(list.checked_erase(&a));
}

TEST(SimpleList, PopFront)
{
    simple_list<A *> list {};

    A a {};
    list.push_back(&a);

    A b {};
    list.push_back(&b);

    list.pop_front();

    EXPECT_TRUE(list.contains(&b));
    EXPECT_FALSE(list.contains(&a));
}

TEST(SimpleListIterator, Construct) 
{
    simple_list<A *> list {};

    A a {};
    list.push_back(&a);

    auto it = list.begin();

    EXPECT_EQ(it._ptr, &a);
    EXPECT_EQ(it._ptr->simple_list_vars._sl_list_owner, &list);
}

TEST(SimpleListIterator, Next) 
{
    A a {};
    simple_list<A *>::iterator it {&a};

    ++it;

    const simple_list<A *>::iterator end {nullptr};
    EXPECT_EQ(it, end);
}

TEST(SimpleListIterator, Swap) 
{
    simple_list<A *> list {};

    A a {};
    list.push_back(&a);

    auto iterA = list.begin();

    A b {};
    list.push_front(&b);

    auto iterB = list.begin();

    iterA.swap(iterA, iterB);

    EXPECT_EQ(iterA._ptr, &b);
    EXPECT_EQ(iterB._ptr, &a);
}
