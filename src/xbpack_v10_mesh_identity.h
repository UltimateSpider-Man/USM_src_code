#pragma once

#include <cstdint>

// The retail packed loader builds its NGL argument by calling
// tlFixedString(loc->name.to_string()). Without a dictionary entry this hashes
// the *text* "0x43366c2f", not the original resource ID 0x43366C2F.
// Carry the real ID across that native call without changing its ABI, the
// global hash dictionary, or any entity/mission state.
namespace xbpack::v10_mesh_identity {

class scoped_source {
    inline static thread_local scoped_source *current_ = nullptr;
    scoped_source *previous_;
    const void *image_;
    std::uint32_t hash_;
    bool consumed_ = false;

public:
    scoped_source(const void *image, std::uint32_t hash) noexcept
        : previous_(current_), image_(image), hash_(hash)
    {
        current_ = this;
    }

    ~scoped_source() noexcept { current_ = previous_; }
    scoped_source(const scoped_source &) = delete;
    scoped_source &operator=(const scoped_source &) = delete;
    scoped_source(scoped_source &&) = delete;
    scoped_source &operator=(scoped_source &&) = delete;

    // Match the original image *before* any private override copy is made.
    // A different/nested loose load must not inherit this resource's ID.
    // Consume at most once; the scope also restores state on an early return
    // or C++ exception. Thread-local state prevents cross-thread attribution.
    static std::uint32_t take(const void *image) noexcept
    {
        if (image == nullptr) return 0;
        for (auto *source = current_; source != nullptr;
             source = source->previous_) {
            if (source->image_ != image) continue;
            if (source->consumed_) return 0;
            source->consumed_ = true;
            return source->hash_;
        }
        return 0;
    }
};

// A packed nglMeshFile also carries its serialized FileName hash. Prefer that
// authoritative value on direct/non-handler calls. The argument is only a
// fallback for a loose file with no stored name.
inline constexpr std::uint32_t resource_hash(std::uint32_t stored_hash,
                                             std::uint32_t argument_hash) noexcept
{
    return stored_hash != 0 ? stored_hash : argument_hash;
}

} // namespace xbpack::v10_mesh_identity
