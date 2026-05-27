#pragma once

#include "mash.h"
#include "memory.h"
#include "trace.h"

#include <cassert>
#include <cstdint>

template<typename T>
struct simple_list {
    static_assert(std::is_pointer_v<T>, "");

    using value_type = typename std::remove_pointer_t<T>;

    value_type * _first_element;
    value_type * _last_element;
    uint32_t m_size;

    simple_list() {
        this->initialize(mash::ALLOCATED);
    }

    void initialize(mash::allocation_scope scope)
    {
        if (scope == mash::ALLOCATED) {
            this->clear();
        }
    }

    void clear()
    {
        this->_first_element = nullptr;
        this->_last_element = nullptr;
        this->m_size = 0;
    }


    struct vars_t {
        value_type * _sl_next_element;
        value_type * _sl_prev_element;
        simple_list<T> *_sl_list_owner;
        
        vars_t() : _sl_next_element(nullptr),
                    _sl_prev_element(nullptr),
                    _sl_list_owner(nullptr) {}
    };

    struct iterator {
        value_type *_ptr {nullptr};

        bool operator==(const iterator &it) const {
            return this->_ptr == it._ptr;
        }

        bool operator!=(const iterator &it) const {
            return this->_ptr != it._ptr;
        }

        void operator++() {
            if ( this->_ptr != nullptr ) {
                this->_ptr = this->_ptr->simple_list_vars._sl_next_element;
            }
        }

        value_type *& operator*() {
            return this->_ptr;
        }

        static void swap(iterator &a, iterator &b)
        {
            assert(a._ptr->simple_list_vars._sl_list_owner == b._ptr->simple_list_vars._sl_list_owner);

            auto v2 = a._ptr->simple_list_vars._sl_list_owner;
            if (v2->_first_element == a._ptr) {
                v2->_first_element = b._ptr;
            } else {
                auto *v3 = b._ptr->simple_list_vars._sl_list_owner;
                if (v3->_first_element == b._ptr) {
                    v3->_first_element = a._ptr;
                }
            }

            auto *v4 = a._ptr->simple_list_vars._sl_list_owner;
            if (v4->_last_element == a._ptr) {
                v4->_last_element = b._ptr;
            } else {
                auto *v5 = b._ptr->simple_list_vars._sl_list_owner;
                if (v5->_last_element == b._ptr) {
                    v5->_last_element = a._ptr;
                }
            }

            auto *v6 = a._ptr->simple_list_vars._sl_prev_element;
            a._ptr->simple_list_vars._sl_prev_element = b._ptr->simple_list_vars._sl_prev_element;
            b._ptr->simple_list_vars._sl_prev_element = v6;
            auto *v7 = a._ptr->simple_list_vars._sl_prev_element;
            if (v7 != nullptr) {
                v7->simple_list_vars._sl_next_element = a._ptr;
            }

            auto *v8 = b._ptr->simple_list_vars._sl_prev_element;
            if (v8 != nullptr) {
                v8->simple_list_vars._sl_next_element = b._ptr;
            }

            auto *v9 = a._ptr->simple_list_vars._sl_next_element;
            a._ptr->simple_list_vars._sl_next_element = b._ptr->simple_list_vars._sl_next_element;
            b._ptr->simple_list_vars._sl_next_element = v9;
            auto *v10 = a._ptr->simple_list_vars._sl_next_element;
            if (v10 != nullptr) {
                v10->simple_list_vars._sl_prev_element = a._ptr;
            }

            auto *v11 = b._ptr->simple_list_vars._sl_next_element;
            if (v11 != nullptr) {
                v11->simple_list_vars._sl_prev_element = b._ptr;
            }

            std::swap(a._ptr, b._ptr);
        }


    };

    uint32_t size() const {
        return this->m_size;
    }

    bool empty() const {
        return (this->m_size == 0);
    }

    iterator begin() {
        return iterator {this->_first_element};
    }
    
    iterator end() {
        return iterator {nullptr};
    }

    value_type *& front() {
        return this->_first_element;
    }

    iterator push_front(value_type *tmp)
    {
        TRACE("simple_list::push_front");

        assert(tmp != nullptr);

        assert(tmp->simple_list_vars._sl_next_element == nullptr);

        assert(tmp->simple_list_vars._sl_prev_element == nullptr);

        assert(tmp->simple_list_vars._sl_list_owner == nullptr);

        tmp->simple_list_vars._sl_next_element = this->_first_element;
        tmp->simple_list_vars._sl_prev_element = nullptr;

        if ( this->_first_element != nullptr ) {
            this->_first_element->simple_list_vars._sl_prev_element = tmp;
        }

        this->_first_element = tmp;
        if ( tmp->simple_list_vars._sl_next_element == nullptr ) {
            this->_last_element = tmp;
        }

        tmp->simple_list_vars._sl_list_owner = this;

        ++this->m_size;

        iterator a2{tmp};
        return a2;
    }

    iterator push_back(value_type *tmp)
    {
        TRACE("simple_list::push_back");

        assert(tmp != nullptr);

        assert(tmp->simple_list_vars._sl_next_element == nullptr);
        
        assert(tmp->simple_list_vars._sl_prev_element == nullptr);

        assert(tmp->simple_list_vars._sl_list_owner == nullptr);

        if ( this->_last_element != nullptr )
        {
            assert(this->_last_element->simple_list_vars._sl_next_element == nullptr);

            this->_last_element->simple_list_vars._sl_next_element = tmp;
            tmp->simple_list_vars._sl_prev_element = this->_last_element;
            tmp->simple_list_vars._sl_next_element = nullptr;
            this->_last_element = tmp;
            tmp->simple_list_vars._sl_list_owner = this;
            ++this->m_size;
            return iterator {tmp};
        }
        else
        {
            return this->push_front(tmp);
        }
    }

    bool contains(value_type *iter) const {
        return (iter != nullptr)
                && (iter->simple_list_vars._sl_list_owner == this);
    }

    value_type * common_erase(value_type *iter, bool a3)
    {
        value_type *result = nullptr;
        if ( iter != nullptr )
        {
            assert(this->contains(iter));

            result = (a3
                        ? iter->simple_list_vars._sl_prev_element
                        : iter->simple_list_vars._sl_next_element
                        );

            auto *sl_prev_element = iter->simple_list_vars._sl_prev_element;
            if ( sl_prev_element != nullptr ) {
                sl_prev_element->simple_list_vars._sl_next_element = iter->simple_list_vars._sl_next_element;
            } else {
                assert(iter->simple_list_vars._sl_list_owner->_first_element == iter);

                iter->simple_list_vars._sl_list_owner->_first_element = iter->simple_list_vars._sl_next_element;
            }

            if ( iter->simple_list_vars._sl_next_element != nullptr ) {
                iter->simple_list_vars._sl_next_element->simple_list_vars._sl_prev_element = iter->simple_list_vars._sl_prev_element;
            } else {
                assert(iter->simple_list_vars._sl_list_owner->_last_element == iter);

                iter->simple_list_vars._sl_list_owner->_last_element = iter->simple_list_vars._sl_prev_element;
            }

            assert(iter->simple_list_vars._sl_list_owner->m_size >= 0);

            --iter->simple_list_vars._sl_list_owner->m_size;
            iter->simple_list_vars._sl_next_element = nullptr;
            iter->simple_list_vars._sl_prev_element = nullptr;
            iter->simple_list_vars._sl_list_owner = nullptr;
        }

        return result;
    }

    bool checked_erase(value_type *a2)
    {
        if ( !this->contains(a2) ) {
            return false;
        }

        this->common_erase(a2, false);
        return true;
    }

    iterator erase(value_type *a3)
    {
        auto *v3 = this->common_erase(a3, false);
        return {v3};
    }

    void pop_front()
    {
        auto it = this->begin();
        this->erase(it._ptr);
    }

};
