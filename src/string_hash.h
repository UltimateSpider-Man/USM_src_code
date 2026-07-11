#pragma once

#include <variable.h>

struct entity_base;
struct mic;
struct mash_info_struct;

struct string_hash {
    std::uint32_t source_hash_code;

    string_hash();

    string_hash(const char *a1);

    string_hash(int a4);

    //0x00547A00
    void initialize(int a2, const char *a3, int hash_code);

    void unmash(mash_info_struct *, void *);

    //0x005374B0
    const char *to_string() const;

    //0x00542710
    void set(const char *str);

    void destruct_mashed_class();

    bool operator==(const string_hash &hash) const {
        return this->source_hash_code == hash.source_hash_code;
    }

    bool operator<(const string_hash &rhs) const {
        return this->source_hash_code < rhs.source_hash_code;
    }

    bool operator>(const string_hash &rhs) const {
        return this->source_hash_code > rhs.source_hash_code;
    }
	
	    uint32_t get_hash() const {
        return source_hash_code;
    }

    operator size_t() const {
        return static_cast<size_t>(this->source_hash_code);
    }

    static Var<char[12]> ghetto_string;

    static string_hash INVALID_STRING_HASH;

    [[nodiscard]] string_hash sub_501E80();
	
	
static inline constexpr bool is_alpha2(unsigned char c) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            return true;
        }

        return false;
    }

    static inline constexpr unsigned char to_lower2(unsigned char c) {
        constexpr auto delta = 'a' - 'A';

        if (c >= 'A' && c <= 'Z') {
            return (c + delta);
        }

        return c;
    }

    //0x00501BE0
    static constexpr std::uint32_t to_hash2(const char *str) {
        std::uint32_t res = 0;

        for (int c = *str; c != '\0'; ++str, c = *str) {
            int ch_lower = [](auto ch) -> int {
                if (is_alpha2(ch)) {
                    return to_lower2(ch);
                }

                return ch;
            }(c);

            res = ch_lower + 33 * res;
        }

        return res;
    }
	

};


namespace ai {
extern Var<string_hash> cat_id_idle_walk_run;

extern Var<string_hash> loco_allow_web_tie_id;
}

extern string_hash ANONYMOUS;

//0x004BFD50
extern string_hash make_unique_entity_id();

inline constexpr bool is_alpha(unsigned char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        return true;
    }

    return false;
}

inline constexpr unsigned char to_lower(unsigned char c) {
    constexpr auto delta = 'a' - 'A';

    if (c >= 'A' && c <= 'Z') {
        return (c + delta);
    }

    return c;
}

//0x00501BE0
extern constexpr inline std::uint32_t to_hash(const char *str) {
    std::uint32_t res = 0;

    for (int c = *str; c != '\0'; ++str, c = *str) {
        int ch_lower = [](auto c) -> int {
            if (is_alpha(c)) {
                return to_lower(c);
            }

            return c;
        }(c);

        res = ch_lower + 33 * res;
    }

    return res;
}

extern void string_hash_patch();


