#pragma once

// Known .fbx mod names for files dropped in mods/. Characters go through
// the full skinned pipeline; objects are the hero-gallery / trophy display
// meshes. The list is ADVISORY: any other stem is bound as a custom mesh
// target too (the replacement only triggers when the engine actually
// requests a mesh file with that name, so unknown stems are harmless), the
// list just drives the "recognized" log line and documents the stock names.

#include <cctype>
#include <string>

namespace modmesh {

inline const char *const kCharacterFbxNames[] = {
    "venom", "ultimate_spiderman", "peter_parker", "peter_hooded", "carnage",
    "usm_blacksuit", "usm_blacksuit_costume", "usm_wrestling_costume",
    "venom_2018", "venarge", "batman", "hulk", "electro_nosuit",
    "mary_jane", "gang1", "gang_skin_boss_fem", "gang_skin_boss",
    "gang_ftb_boss", "gang_srk_boss", "gang_mercs_boss", "silver_sable",
    "venom_spider", "venom_eddie",
};

inline const char *const kObjectFbxNames[] = {
    "skins_flamethrower",
    "hg_boss_wolverine", "hg_boss_venom", "hg_boss_spiderman",
    "hg_boss_shocker", "hg_boss_sable", "hg_boss_rhino",
    "hg_boss_mystique", "hg_boss_goblin", "hg_boss_electro_suit",
    "hg_boss_electro_nosuit", "hg_boss_electro", "hg_boss_carnage", "hg_boss_beetle",
    "hg_tp_wolverine", "hg_tp_spiderman", "hg_tp_sable",
    "hg_hero_venom", "hg_hero_spiderman_ex_03", "hg_hero_spiderman_ex_02",
    "hg_hero_spiderman_ex_01", "hg_hero_spiderman", "hg_hero_peter",
};

// case-insensitive stem lookup; isObject reports which list matched
inline bool isKnownFbxTarget(std::string stem, bool *isObject = nullptr)
{
    for (auto &c : stem) c = char(std::tolower((unsigned char) c));
    for (const char *n : kCharacterFbxNames)
        if (stem == n) { if (isObject) *isObject = false; return true; }
    for (const char *n : kObjectFbxNames)
        if (stem == n) { if (isObject) *isObject = true;  return true; }
    return false;
}

} // namespace modmesh
