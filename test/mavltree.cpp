#include <gtest/gtest.h>

#include <mAvlTree.h>
#include <string_hash_entry.h>

TEST(MAvlTree, Construct)
{
    mAvlTree<string_hash_entry> tree {};

    EXPECT_EQ(tree.root(), nullptr);
    EXPECT_EQ(tree.size(), 0);
    EXPECT_TRUE(tree.get_destruct_contents());
}

TEST(MAvlTree, AddHelper)
{
    mAvlTree<string_hash_entry> tree {};

    string_hash h0 {2};

    string_hash_entry entries[3] =
    {
        string_hash_entry {nullptr, string_hash {1}},
        string_hash_entry {nullptr, string_hash {2}},
        string_hash_entry {nullptr, string_hash {3}}
    };

    using node_t = mAvlNode<string_hash_entry>;
    node_t nodes[3] =
    {
        node_t {&entries[0]},
        node_t {&entries[1]},
        node_t {&entries[2]}
    };

    auto res = tree.addHelper(&nodes[1], tree.root(), nullptr);
    EXPECT_EQ(res, 1);

    res = tree.addHelper(&nodes[2], tree.root(), nullptr);
    EXPECT_EQ(res, 1);

    res = tree.addHelper(&nodes[0], tree.root(), nullptr);
    EXPECT_EQ(res, 1);

    EXPECT_EQ(tree.size(), 3);

    EXPECT_TRUE(tree.root() != nullptr);
    EXPECT_TRUE(tree.root()->m_right != nullptr);
    EXPECT_TRUE(tree.root()->m_left != nullptr);

    EXPECT_EQ(tree.root()->m_key->field_0, string_hash {2});
    EXPECT_EQ(tree.root()->m_left->m_key->field_0, string_hash {1});
    EXPECT_EQ(tree.root()->m_right->m_key->field_0, string_hash {3});

    auto it = tree.begin();
    EXPECT_TRUE(it.field_0 != nullptr);

    EXPECT_EQ(it.field_0->m_key->field_0, string_hash {1});

    ++it;
    EXPECT_EQ(it.field_0->m_key->field_0, string_hash {2});

    ++it;
    EXPECT_EQ(it.field_0->m_key->field_0, string_hash {3});
}
