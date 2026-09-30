#include "binary_search_array_cmp.h"

#include "entity_base_vhandle.h"
#include "enum_anim_key.h"
#include "event_recipient_entry.h"
#include "event_type.h"
#include "func_wrapper.h"
#include "log.h"
#include "resource_pack_location.h"

int compare_resource_key(const resource_key &a1, const resource_key &a2)
{
    if constexpr (1)
    {
        if (a1 == a2) {
            return 0;
        }

        if (a1 >= a2) {
            return 1;
        }

        return -1;
    }
    else
    {
        return CDECL_CALL(0x004201F0, a1, a2);
    }
}

int compare_resource_key_resource_pack_location(const resource_key &a1,
                                                const resource_pack_location &a2)
{
#ifdef OPENUSM_XBPACK_MODE
    const auto requested_hash = a1.m_hash.source_hash_code;
    const auto location_hash = a2.loc.field_0.m_hash.source_hash_code;

    if (requested_hash < location_hash) {
        return -1;
    }

    if (requested_hash > location_hash) {
        return 1;
    }

    return 0;
#else
    return compare_resource_key(a1, a2.loc.field_0);
#endif
}

template<>
int compare_deref(
        entity_base_vhandle &a1,
        event_recipient_entry *&a2)
{
    if ( a1 < a2->field_0 )
        return -1;
    else
        return (a1 > a2->field_0);
}

template<>
int compare_deref(string_hash &a1, event_type *&a2)
{
    auto source_hash_code = a2->field_0.source_hash_code;
    if ( source_hash_code <= a1.source_hash_code ) {
        return source_hash_code < a1.source_hash_code;
    } else {
        return -1;
    }
}

#if 0
template<>
bool binary_search_array_cmp(resource_key *a1,
                             resource_pack_location *a2,
                             int start_index,
                             int count,
                             int *p_index,
                             int (*fn_compare)(resource_key *a1, resource_pack_location *)) {
#if 1

    auto v6 = count;
    auto v7 = start_index;
    if (start_index < count) {
        do {
            int mid_index = (v7 + v6) / 2;
            int res_comp = fn_compare(a1, &a2[mid_index]);
            if (res_comp > -1) {
                if (res_comp < 1) {
                    if (p_index != nullptr) {
                        *p_index = mid_index;
                    }

                    return true;
                }

                v7 = mid_index + 1;
                continue;
            }

            v6 = (v7 + v6) / 2;

        } while (v7 < v6);
    }

    if (p_index != nullptr) {
        *p_index = v6;
    }

    return false;
#else
    return (bool) CDECL_CALL(0x00560E40, a1, a2, start_index, count, a5, fn_compare);
#endif
}


template<>
bool binary_search_array_cmp(const resource_key *a1,
                             resource_location *a2,
                             int start_index,
                             int count,
                             int *p_index,
                             int (*fn_compare)(const resource_key *a1, resource_location *)) {
#if 1

    auto v6 = count;
    auto v7 = start_index;
    if (start_index < count) {
        do {
            int mid_index = (v7 + v6) / 2;
            int res_comp = fn_compare(a1, &a2[mid_index]);
            if (res_comp > -1) {
                if (res_comp < 1) {
                    if (p_index != nullptr) {
                        *p_index = mid_index;
                    }

                    return true;
                }

                v7 = mid_index + 1;
                continue;
            }

            v6 = (v7 + v6) / 2;

        } while (v7 < v6);
    }

    if (p_index != nullptr) {
        *p_index = v6;
    }

    return false;
#else
    return (bool) CDECL_CALL(0x00560F20, a1, a2, start_index, count, a5, fn_compare);
#endif
}

template<>
bool binary_search_array_cmp(
    anim_key **a1, int *a2, int a3, int a4, int *a5, int (*fn_compare)(anim_key **, int *)) {
    return (bool) CDECL_CALL(0x005E2870, a1, a2, a3, a4, a5, fn_compare);
}
#endif
