#include "mAvlTree.h"

#include "common.h"
#include "mash_info_struct.h"
#include "utility.h"

VALIDATE_SIZE(mAvlTree<string_hash_entry>, 16u);

template<>
void mAvlTree<string_hash_entry>::sub_564370(mAvlNode<string_hash_entry> **a1) {
    auto v2 = a1;
    auto v3 = *a1;
    auto v4 = (*a1)->m_left;
    auto a1a = (*a1)->m_parent;
    v3->m_left = v4->m_right;
    auto v5 = (*v2)->m_left;
    if (v5) {
        v5->m_parent = *v2;
    }

    auto v6 = *v2;
    auto v7 = (*v2 == nullptr);
    v4->m_right = *v2;
    if (!v7) {
        v6->m_parent = v4;
    }

    auto v8 = *v2;
    auto *v9 = (*v2)->m_left;
    char v10 = -1;
    if (v9) {
        v10 = v9->field_10;
    }

    auto *v11 = v8->m_right;
    char v12 = -1;
    if (v11) {
        v12 = v11->field_10;
    }

    char v13;
    [&]() {
        if (v10 <= v12) {
            if (v11) {
                v13 = v11->field_10;
                return;
            }
        } else if (v9) {
            v13 = v9->field_10;
            return;
        }
        v13 = -1;
    }();

    v8->field_10 = v13 + 1;
    auto *v14 = v4->m_left;
    char v15 = -1;
    if (v14) {
        v15 = v14->field_10;
    }

    auto v16 = (*v2)->field_10;
    if (v15 > v16) {
        if (v14) {
            v4->field_10 = v14->field_10 + 1;
            *v2 = v4;
            v4->m_parent = a1a;
            return;
        }
        v16 = -1;
    }
    v4->field_10 = v16 + 1;
    *v2 = v4;
    v4->m_parent = a1a;
}

template<>
void mAvlTree<string_hash_entry>::sub_43BF70(mAvlNode<string_hash_entry> **a1) {
    auto v2 = a1;
    auto v3 = *a1;
    auto v4 = (*a1)->m_right;
    auto a1a = (*a1)->m_parent;
    v3->m_right = v4->m_left;
    auto *v5 = v4->m_left;
    if (v5) {
        v5->m_parent = *v2;
    }

    v4->m_left = *v2;
    if (*v2) {
        (*v2)->m_parent = v4;
    }

    auto v6 = *v2;
    auto v7 = (*v2)->m_left;
    char v8 = -1;
    if (v7) {
        v8 = v7->field_10;
    }

    auto *v9 = v6->m_right;
    char v10 = -1;
    if (v9) {
        v10 = v9->field_10;
    }

    char v11;
    [&]() {
        if (v8 <= v10) {
            if (v9) {
                v11 = v9->field_10;
                return;
            }
        } else if (v7) {
            v11 = v7->field_10;
            return;
        }

        v11 = -1;
    }();

    v6->field_10 = v11 + 1;
    auto *v12 = v4->m_right;
    char v13 = -1;
    if (v12) {
        v13 = v12->field_10;
    }

    auto v14 = (*v2)->field_10;
    if (v13 > v14) {
        if (v12) {
            v4->field_10 = v12->field_10 + 1;
            *v2 = v4;
            v4->m_parent = a1a;
            return;
        }
        v14 = -1;
    }

    v4->field_10 = v14 + 1;
    *v2 = v4;
    v4->m_parent = a1a;
}

template<>
void mAvlTree<string_hash_entry>::sub_744960(mAvlNode<string_hash_entry> **a1) {
    this->sub_564370(&(*a1)->m_right);
    this->sub_43BF70(a1);
}

template<>
void mAvlTree<string_hash_entry>::sub_745DF0(mAvlNode<string_hash_entry> **a2) {
    this->sub_43BF70(&(*a2)->m_left);
    this->sub_564370(a2);
}

template<>
int *mAvlTree<string_hash_entry>::sub_64A090(int *a2) {
    return (int *) THISCALL(0x0064A090, this, a2);
}

template<>
void mAvlTree<string_hash_entry>::sub_64A2B0() {
    THISCALL(0x0064A2B0, this);
}

template<>
void mAvlTree<string_hash_entry>::sub_439AD0(mAvlNode<string_hash_entry> *a1) {
    auto *v2 = a1->m_left;
    char v3 = -1;
    if (v2 != nullptr) {
        v3 = v2->field_10;
    }

    auto *v4 = a1->m_right;
    char v5 = -1;
    if (v4 != nullptr) {
        v5 = v4->field_10;
    }

    if (v3 <= v5) {
        if (v4) {
            a1->field_10 = v4->field_10 + 1;
            return;
        }

    } else if (v2) {
        a1->field_10 = v2->field_10 + 1;
        return;
    }

    a1->field_10 = 0;
}

int sub_561350(mAvlNode<string_hash_entry> *a1, mAvlNode<string_hash_entry> *a2) {
    auto v2 = a2->m_key->field_0.source_hash_code;
    auto v3 = a1->m_key->field_0.source_hash_code;
    if (v3 <= v2) {
        return -(v3 < v2);
    }

    return 1;
}

//0x0056AE30
template<>
int mAvlTree<string_hash_entry>::addHelper(mAvlNode<string_hash_entry> *a2,
                                           mAvlNode<string_hash_entry> *&a3,
                                           mAvlNode<string_hash_entry> *a4) {
    if constexpr (1) {
        if (a3 == nullptr) {
            a2->m_parent = a4;
            a3 = a2;
            ++this->m_size;
            return 1;
        }

        auto current_hash = a3->m_key->field_0.source_hash_code;
        auto new_hash = a2->m_key->field_0.source_hash_code;
        if (new_hash > current_hash) {
            auto result = this->addHelper(a2, a3->m_right, a3);
            auto *node = a3;
            auto *right = node->m_right;
            auto right_height = right != nullptr ? right->field_10 : -1;
            auto *left = node->m_left;
            auto left_height = left != nullptr ? left->field_10 : -1;

            if (right_height - left_height == 2) {
                if (sub_561350(a2, right) <= 0) {
                    this->sub_744960(&a3);
                } else {
                    this->sub_43BF70(&a3);
                }
            } else {
                this->sub_439AD0(node);
            }

            return result;
        }

        if (new_hash < current_hash) {
            auto result = this->addHelper(a2, a3->m_left, a3);
            auto *node = a3;
            auto *left = node->m_left;
            auto left_height = left != nullptr ? left->field_10 : -1;
            auto *right = node->m_right;
            auto right_height = right != nullptr ? right->field_10 : -1;

            if (left_height - right_height == 2) {
                if (new_hash >= left->m_key->field_0.source_hash_code) {
                    this->sub_745DF0(&a3);
                } else {
                    this->sub_564370(&a3);
                }
            } else {
                this->sub_439AD0(node);
            }

            return result;
        }

        return 0;
    } else {
        return THISCALL(0x0056AE30, this, a2, a3, a4);
    }
}

template<>
void mAvlTree<string_hash_entry>::unmash(mash_info_struct *a2, [[maybe_unused]] void *a3)
{
    if constexpr (1)
    {
        this->m_head = nullptr;
        auto real_size = this->m_size;
        this->m_size = 0;

        for (auto i = 0u; i < real_size; ++i)
        {
            mAvlNode<string_hash_entry> *v7 = nullptr;
            a2->unmash_class(v7, this);
            this->addHelper(v7, this->m_head, nullptr);
        }

        assert(real_size == this->m_size);

        this->field_0 = (int) &a2->mash_image_ptr[a2->buffer_size_used[0] - (DWORD) this];
    }
    else
    {
        THISCALL(0x00570900, this, a2, a3);
    }
}

template<>
void mAvlNode<string_hash_entry>::unmash(
                            mash_info_struct *a3,
                            void *)
{
    a3->unmash_class(this->m_key, this);
}

void mAvlTree_patch() {
    {
        FUNC_ADDRESS(address, &mAvlTree<string_hash_entry>::findHelper);
        REDIRECT(0x00531A07, address);
    }

    {
        FUNC_ADDRESS(address, &mAvlTree<string_hash_entry>::iterator::iterate);
        //REDIRECT(0x0052A7C2, address);
    }

    {
        FUNC_ADDRESS(address, &mAvlTree<string_hash_entry>::unmash);
        REDIRECT(0x00550F44, address);
    }
}
