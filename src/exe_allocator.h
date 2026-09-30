#pragma once

#include <config.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <utility>

template<typename T>
struct exe_allocator
{
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    struct rebind
    {
        using other = exe_allocator<U>;
    };

    exe_allocator() noexcept = default;

    template<typename U>
    exe_allocator(const exe_allocator<U> &) noexcept
    {
    }

    pointer address(reference value) const noexcept
    {
        return &value;
    }

    const_pointer address(const_reference value) const noexcept
    {
        return &value;
    }

    pointer allocate(size_type count)
    {
        if (count > max_size())
            throw std::bad_alloc {};

        using allocate_t = void *(__cdecl *)(size_type);
        auto *result = bit_cast<allocate_t>(std::uintptr_t {0x00822046})(count * sizeof(T));
        if (result == nullptr)
            throw std::bad_alloc {};

        return static_cast<pointer>(result);
    }

    void deallocate(pointer ptr, size_type) noexcept
    {
        using deallocate_t = void (__cdecl *)(void *);
        bit_cast<deallocate_t>(std::uintptr_t {0x0082207C})(ptr);
    }

    template<typename U, typename... Args>
    void construct(U *ptr, Args &&...args)
    {
        ::new (static_cast<void *>(ptr)) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U *ptr)
    {
        ptr->~U();
    }

    size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }
};

template<typename T, typename U>
bool operator==(const exe_allocator<T> &, const exe_allocator<U> &) noexcept
{
    return true;
}

template<typename T, typename U>
bool operator!=(const exe_allocator<T> &, const exe_allocator<U> &) noexcept
{
    return false;
}
