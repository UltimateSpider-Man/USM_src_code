#pragma once

// Pointer publication for a single packed resource directory. This helper has
// no engine dependencies so matching, isolation and unload restoration can be
// checked without running the game.
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace modmesh::resourcebinding {

class Snapshot;

// Shared only by the snapshots in one resource slot. Keep the original packed
// pointer once, followed by every live file that published a replacement.
class Claims {
    friend class Snapshot;
    struct Layer {
        const Snapshot *owner;
        char *replacement;
    };
    struct Location {
        char *original;
        std::vector<Layer> layers;
    };
    std::map<char **, Location> locations_;
};

class Snapshot {
    std::vector<char **> locations_;
    Claims *claims_ = nullptr;

    std::size_t remove(bool restorePointers)
    {
        std::size_t restored = 0;
        if (claims_ != nullptr) {
            for (auto *target : locations_) {
                const auto found = claims_->locations_.find(target);
                if (found == claims_->locations_.end()) continue;
                auto &location = found->second;
                auto &layers = location.layers;
                char *oldTop = layers.back().replacement;
                for (auto it = layers.begin(); it != layers.end(); ++it) {
                    if (it->owner == this) {
                        layers.erase(it);
                        break;
                    }
                }
                char *next = layers.empty() ? location.original : layers.back().replacement;
                // An external rebind owns its value even when one of our
                // layers unloads. Never replace it with a historical pointer.
                if (restorePointers && *target == oldTop && *target != next) {
                    *target = next;
                    ++restored;
                }
                if (layers.empty()) claims_->locations_.erase(found);
            }
        }
        locations_.clear();
        claims_ = nullptr;
        return restored;
    }

public:
    Snapshot() = default;
    Snapshot(const Snapshot &) = delete;
    Snapshot &operator=(const Snapshot &) = delete;
    ~Snapshot() { abandon(); }

    // Location supplies name.source_hash_code and field_8, as the packed
    // tlresource_location does. Only this explicit range can be changed;
    // parents and another slot with the same hash are never searched.
    template<class Location>
    std::size_t rebindMatching(Location *locations, std::size_t count,
                               std::uint32_t hash, char *replacement,
                               Claims &claims)
    {
        if (locations == nullptr || replacement == nullptr) return 0;
        if (claims_ != nullptr && claims_ != &claims) return 0;
        claims_ = &claims;
        std::size_t changed = 0;
        for (std::size_t i = 0; i < count; ++i) {
            auto &location = locations[i];
            if (location.name.source_hash_code != hash) continue;
            auto *target = &location.field_8;
            auto found = claims.locations_.find(target);
            if (found != claims.locations_.end()) {
                auto &layers = found->second.layers;
                if (*target != layers.back().replacement) continue;
                Claims::Layer *prior = nullptr;
                for (auto &layer : layers) {
                    if (layer.owner == this) {
                        prior = &layer;
                        break;
                    }
                }
                if (prior != nullptr) {
                    // Reentry updates this file's layer without moving it
                    // above another file that was published subsequently.
                    prior->replacement = replacement;
                    if (prior != &layers.back()) continue;
                } else {
                    if (*target == replacement) continue;
                    layers.push_back({this, replacement});
                    locations_.push_back(target);
                }
            } else {
                if (*target == replacement) continue;
                claims.locations_.emplace(target,
                    Claims::Location{*target, {{this, replacement}}});
                locations_.push_back(target);
            }
            if (*target == replacement) continue;
            *target = replacement;
            ++changed;
        }
        return changed;
    }

    std::size_t restore()
    {
        return remove(true);
    }

    // Metadata-only cleanup for a discarded/reused slot. Never dereference a
    // location after its packed table has gone away.
    void abandon()
    {
        remove(false);
    }

    bool empty() const { return locations_.empty(); }
};

} // namespace modmesh::resourcebinding
