#include "string_hash.h"

#include "config.h"
#include "entity_handle_manager.h"
#include "func_wrapper.h"
#include "log.h"
#include "return_address.h"
#include "string_hash_dictionary.h"
#include "trace.h"
#include "utility.h"

#include <stdio.h>

// script_object.cpp owns the embedded-name table for converted beta PS2SX
// images. Keep this dependency narrow so hashing does not need script types.
extern const char *modPS2SXHashName(uint32_t nameHash);

namespace ai {
Var<string_hash> cat_id_idle_walk_run{0x0096C1E8};

Var<string_hash> loco_allow_web_tie_id{0x00958538};



} // namespace ai

string_hash ANONYMOUS{};

string_hash string_hash::INVALID_STRING_HASH = {0};

#ifndef TEST_CASE
Var<char[12]> string_hash::ghetto_string = {0x0095C7D0};
#else

static char g_ghetto_string[12]{};
Var<char[12]> string_hash::ghetto_string{&g_ghetto_string};
#endif

string_hash::string_hash() {
    this->initialize(0, nullptr, 0);
}

string_hash::string_hash(const char *a1) {
    this->initialize(0, a1, 0);
}

string_hash::string_hash(int a4) {
    this->initialize(0, nullptr, a4);
}

void string_hash::set(const char *str) {
    if constexpr (1) {
        if (str == nullptr || str[0] != '\0') {
            *this = string_hash_dictionary::register_string(str);

        } else {
            this->source_hash_code = 0;
        }
    } else {
        THISCALL(0x00542710, this, str);
    }
}

void string_hash::destruct_mashed_class() {
    ;
}

string_hash string_hash::sub_501E80() {
    return *this;
}

void string_hash::initialize(int a2, const char *a3, int hash_code) {
    if (a2 == 0) {
        if (hash_code) {
            if (a3 != nullptr) {
                this->source_hash_code = to_hash(a3);

                assert(source_hash_code == hash_code && "sanity check");
            }

            this->source_hash_code = hash_code;
        } else if (a3) {
            this->set(a3);
        } else {
            this->source_hash_code = 0;
        }
    }
}

void string_hash::unmash(mash_info_struct *, void *)
{}

const char *string_hash::to_string() const {
    if constexpr (1) {
        if (string_hash_dictionary::is_loaded()) {
            const char *str;

            if ((str = string_hash_dictionary::lookup_string(*this)) != nullptr) {
                return str;
            }
        }

        if (const char *externalName =
                    modPS2SXHashName(this->source_hash_code)) {
            return externalName;
        }

        sprintf(string_hash::ghetto_string(), "0x%08x", this->source_hash_code);
        return string_hash::ghetto_string();
    } else {
        return (char *) THISCALL(0x005374B0, this);
    }
}

bool sub_54C220(uint32_t a1) {
    return (bool) CDECL_CALL(0x0054C220, a1);
}

string_hash make_unique_entity_id()
{
    TRACE("make_unique_entity_id");

    if constexpr (0)
    {
        static Var<int> s_unique_entity_id_idx{0x0095A6C8};

        char Dest[32]; // [esp+4h] [ebp-20h]

        uint32_t hash;
        do {
            int v1 = s_unique_entity_id_idx()++;
            std::snprintf(Dest, 0x20u, "%s%u", "_ENTID_", v1);
            hash = to_hash(Dest);
        } while (sub_54C220(hash));

        string_hash result{Dest};

        return result;
    }
    else
    {
        void (*func)(string_hash *out) = CAST(func, 0x004BFD50);

        string_hash result;
        func(&result);
        return result;
    }
}

static_assert(is_alpha('a'));
static_assert(is_alpha('B'));
static_assert(!is_alpha('?'));
static_assert(!is_alpha('-'));

static_assert(to_lower('C') == 'c');
static_assert(to_lower('D') != 'f');
static_assert(to_lower('Z') == 'z');
static_assert(to_lower('g') == 'g');

static_assert(to_hash("combat_state") == 0x5DC44F76);

void string_hash_patch() {
    {
        FUNC_ADDRESS(address, &string_hash::initialize);
        SET_JUMP(0x00547A00, address);
    }
    {
        FUNC_ADDRESS(address, &string_hash::to_string);
        REDIRECT(0x0056BD90, address);
    }

    {
        FUNC_ADDRESS(address, &string_hash::set);
        REDIRECT(0x00550FAC, address);
    }
}
