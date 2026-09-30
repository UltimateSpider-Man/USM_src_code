#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace xbpack::v10_directory_parents {

struct hook_contract {
    std::uintptr_t call;
    std::uintptr_t native_target;
};

inline constexpr std::array<hook_contract, 3> hooks{{
    {0x005D1FA2u, 0x00537D30u}, // Incoming pack: remove the base-pack edge.
    {0x005D1FEEu, 0x00537D30u}, // Outgoing pack: remove the temporary edge.
    {0x005D1FB3u, 0x00537CC0u}, // Base pack: add the incoming pack instead.
}};

template<class Directory>
struct parent_range {
    Directory *const *data;
    std::size_t size;
};

// Native mission-stack remapping deliberately turns some prerequisite slots
// into nulls. Those nulls are distinct from prerequisites not loaded yet.
// Keep that distinction outside the packed directory's unchanged layout.
template<class Directory>
class lifecycle {
public:
    void reset(const Directory *directory)
    {
        removed_.erase(directory);
    }

    bool is_removed(const Directory *directory, std::size_t index) const
    {
        const auto found = removed_.find(directory);
        return found != removed_.end() && found->second.count(index) != 0;
    }

    template<class ParentAt, class DeclaredMatch, class NativeRemove>
    void remove(Directory *directory, Directory *bye, std::size_t count,
                ParentAt parent_at, DeclaredMatch declared_match,
                NativeRemove native_remove)
    {
        // Also remember an intentionally removed prerequisite whose pointer
        // has not been resolved yet. Native remove_parent leaves it null.
        for (std::size_t i = 0; i < count; ++i) {
            auto *parent = parent_at(i);
            if (parent == bye || (parent == nullptr && declared_match(i))) {
                removed_[directory].insert(i);
            }
        }
        native_remove();
    }

    template<class ParentAt, class NativeAdd>
    void add(Directory *directory, Directory *parent, std::size_t count,
             ParentAt parent_at, NativeAdd native_add)
    {
        std::size_t inserted = 0;
        while (inserted < count && parent_at(inserted) != nullptr) {
            ++inserted;
        }
        native_add();
        // Native add_parent fills the first free slot. Clear only that slot,
        // after verifying that the native operation actually filled it.
        if (inserted == count || parent_at(inserted) != parent) {
            return;
        }
        const auto found = removed_.find(directory);
        if (found != removed_.end()) {
            found->second.erase(inserted);
            if (found->second.empty()) {
                removed_.erase(found);
            }
        }
    }

private:
    std::unordered_map<const Directory *, std::unordered_set<std::size_t>> removed_;
};

// Inspect only existing edges. Resolving missing prerequisites during this
// check would itself mutate the graph being checked and could recurse forever.
template<class Directory, class ParentsOf>
bool can_link(Directory *directory, Directory *candidate, bool candidate_unloading,
              ParentsOf parents_of)
{
    if (candidate == nullptr || candidate == directory || candidate_unloading) {
        return false;
    }

    std::vector<Directory *> pending{candidate};
    std::unordered_set<Directory *> visited;
    while (!pending.empty()) {
        auto *current = pending.back();
        pending.pop_back();
        if (current == directory) {
            return false;
        }
        if (!visited.insert(current).second) {
            continue;
        }
        const auto parents = parents_of(current);
        for (std::size_t i = 0; i < parents.size; ++i) {
            if (parents.data[i] != nullptr) {
                pending.push_back(parents.data[i]);
            }
        }
    }
    return true;
}

} // namespace xbpack::v10_directory_parents

bool xbpack_v10_directory_parents_patch();
