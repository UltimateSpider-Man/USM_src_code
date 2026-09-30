#pragma once

#include "ai_std_hero.h"

// Separate logical character identity from the native gameplay family.
// HasGraph receives an ASG resource name and reports whether it is loaded.
template <typename HasGraph>
hero_type_enum find_hero_type_from_graphs(HasGraph has_graph)
{
    struct mapping {
        const char *name;
        hero_type_enum type;
    };
    static constexpr mapping graphs[] = {
        // A playable Carnage pack can also contain a shared base graph.
        {"CARNAGE", hero_type_enum::CARNAGE},
        {"SPIDEY", hero_type_enum::SPIDEY},
        {"VENOM", hero_type_enum::VENOM},
        {"PARKER", hero_type_enum::PARKER},
        {"USM_BLACKSUIT", hero_type_enum::VENOM},
    };
    for (const auto &graph : graphs) {
        if (has_graph(graph.name)) {
            return graph.type;
        }
    }
    return hero_type_enum::UNDEFINED;
}
