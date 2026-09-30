#pragma once

namespace mod_hero_pack {

inline bool is_female_boss(const char *name)
{
    if (name == nullptr) return false;
    const char *expected = "gang_skin_boss_fem";
    for (; *name && *expected; ++name, ++expected) {
        const char lower = (*name >= 'A' && *name <= 'Z')
            ? static_cast<char>(*name + ('a' - 'A')) : *name;
        if (lower != *expected) return false;
    }
    return *name == *expected;
}

inline bool is_usm_peterhead(const char *name)
{
    if (name == nullptr) return false;
    const char *expected = "usm_peterhead";
    for (; *name && *expected; ++name, ++expected) {
        const char lower = (*name >= 'A' && *name <= 'Z')
            ? static_cast<char>(*name + ('a' - 'A')) : *name;
        if (lower != *expected) return false;
    }
    return *name == *expected;
}

// Keep the actor/save/sound name independent from the package containing it.
// Prefer a user's dedicated hero pack. The installed PC female boss otherwise
// lives in this exact viewer pack; other characters retain their existing path.
template <class PackAvailable>
inline const char *resolve(const char *hero_name, PackAvailable available)
{
#if !defined(OPENUSM_XBPACK_MODE)
    if (is_female_boss(hero_name) && !available(hero_name)) {
        constexpr const char *viewer = "ch_vwr_gang_skin_boss_fem";
        if (available(viewer)) return viewer;
    }
    if (is_usm_peterhead(hero_name) && !available(hero_name)) {
        constexpr const char *viewer2 = "usm_peterhead";
        if (available(viewer2)) return viewer2;
    }
#else
    (void)available;
#endif
    return hero_name;
}

} // namespace mod_hero_pack
