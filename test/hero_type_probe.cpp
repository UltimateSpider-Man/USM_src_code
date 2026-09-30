#include "hero_type_lookup.h"

#include <cassert>
#include <cstring>
#include <initializer_list>

// Engine headers initialize hash sentinels even though this probe never loads
// the game executable. Integer hash construction has no engine dependency.
string_hash::string_hash(int value) : source_hash_code(value) {}

static hero_type_enum identify(std::initializer_list<const char *> graphs)
{
    return find_hero_type_from_graphs([&](const char *name) {
        for (const char *loaded : graphs) {
            if (std::strcmp(name, loaded) == 0) {
                return true;
            }
        }
        return false;
    });
}

int main()
{
    static_assert(CARNAGE == 4, "Carnage's public identity must be 4");
    static_assert(sizeof(hero_type_enum) == 4, "Native enum ABI must stay 32-bit");
    assert(identify({}) == UNDEFINED);
    assert(identify({"UNRELATED"}) == UNDEFINED);
    assert(identify({"SPIDEY"}) == SPIDEY);
    assert(identify({"VENOM"}) == VENOM);
    assert(identify({"PARKER"}) == PARKER);
    assert(identify({"USM_BLACKSUIT"}) == VENOM);
    assert(identify({"CARNAGE"}) == CARNAGE);
    assert(identify({"SPIDEY", "VENOM", "CARNAGE"}) == CARNAGE);

    // The native mission condition checks 2 for its 0x4000 Venom flag.
    // Public identity must remain distinct while that native gate still passes.
    assert(native_hero_type(identify({"CARNAGE"})) == 2);
    assert(native_hero_type(identify({"USM_BLACKSUIT"})) == 2);
    assert(native_hero_type(identify({"VENOM"})) == 2);
    assert(native_hero_type(identify({"SPIDEY"})) == 1);
    assert(native_hero_type(identify({"PARKER"})) == 3);
    assert(native_hero_type(identify({})) == 0);
}
