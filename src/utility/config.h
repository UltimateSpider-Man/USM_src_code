#pragma once

#include <cstddef>

#include <cstring>

#define PROGRESS_LOG

#ifndef __has_feature
#define __has_feature(x) 0 // Compatibility with non-clang compilers.
#endif

// Any compiler claiming C++11 supports, Visual C++ 2015 and Clang version supporting constexpr
#if ((__cplusplus >= 201103L) || (_MSC_VER >= 1900) || \
     (__has_feature(cxx_constexpr))) // C++ 11 implementation
#define _STDEX_NATIVE_CPP11_SUPPORT
#define _STDEX_NATIVE_CPP11_TYPES_SUPPORT
#endif

#if !defined(_STDEX_NATIVE_CPP11_TYPES_SUPPORT)
#if ((__cplusplus > 199711L) || defined(__CODEGEARC__))
#define _STDEX_NATIVE_CPP11_TYPES_SUPPORT
#endif
#endif

#if ((!defined(_MSC_VER) || _MSC_VER < 1600) && !defined(_STDEX_NATIVE_CPP11_SUPPORT))
#define _STDEX_IMPLEMENTS_nullptr_SUPPORT
#else
#define _STDEX_NATIVE_nullptr_SUPPORT
#endif

#if (_MSC_VER >= 1600)
#ifndef _STDEX_NATIVE_CPP11_TYPES_SUPPORT
#define _STDEX_NATIVE_CPP11_TYPES_SUPPORT
#endif
#endif

#if _MSC_VER // Visual C++ fallback
#define _STDEX_NATIVE_MICROSOFT_COMPILER_EXTENSIONS_SUPPORT
#define _STDEX_CDECL __cdecl

#if (__cplusplus >= 199711L)
#define _STDEX_NATIVE_CPP_98_SUPPORT
#endif
#endif

// C++ 98 check:
#if ((__cplusplus >= 199711L) &&                          \
     ((defined(__INTEL_COMPILER) || defined(__clang__) || \
       (defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4))))))
#ifndef _STDEX_NATIVE_CPP_98_SUPPORT
#define _STDEX_NATIVE_CPP_98_SUPPORT
#endif
#endif

#ifdef _STDEX_NATIVE_CPP11_SUPPORT
#include <cstdint>
#else
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int size_t;
typedef unsigned uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef int ptrdiff_t;
#endif

using ushort = uint16_t;
using uint = uint32_t;

#ifndef _STDEX_NATIVE_CPP11_SUPPORT
#define nullptr 0
#endif

#ifdef _STDEX_NATIVE_CPP11_SUPPORT
#include <type_traits>
#endif

#ifdef _STDEX_NATIVE_CPP11_SUPPORT
#include <functional>
#endif

using rational_t = float;

template<typename T>
bool equal(T a1, T a2) {
    std::equal_to<T> q{};
    return q(a1, a2);
}

template<typename T>
bool not_equal(T a1, T a2) {
    std::not_equal_to<T> q{};
    return q(a1, a2);
}

template<class To, class From>
constexpr
    typename std::enable_if_t<sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<From> &&
                                  std::is_trivially_copyable_v<To>,
                              To>
    // constexpr support needs compiler magic
    bit_cast(const From &src) noexcept {
    static_assert(
        std::is_trivially_constructible_v<To>,
        "This implementation additionally requires destination type to be trivially constructible");

    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

template<typename T0, typename T1>
decltype(auto) CAST([[maybe_unused]] const T0 &var, T1 address) {
    return bit_cast<T0>(address);
}
