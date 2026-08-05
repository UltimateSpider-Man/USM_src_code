#include "resource_directory.h"

#include "binary_search_array_cmp.h"
#include "common.h"
#include "debugutil.h"
#include "error.h"
#include "variables.h"
#include "func_wrapper.h"
#include "hashstring.h"
#include "log.h"
#include "mstring.h"
#include "osassert.h"
#include "os_developer_options.h"
#include "parse_generic_mash.h"
#include "resource_pack_slot.h"
#include "resource_partition.h"
#include "return_address.h"
#include "utility.h"

#ifdef TARGET_XBOX
VALIDATE_SIZE(resource_directory, 0x2C4);
#else
VALIDATE_SIZE(resource_directory, 0x2BC);
#endif

VALIDATE_OFFSET(resource_directory, pack_slot, 0x78);

 
void resource_directory::un_mash_start(generic_mash_header *header,
                                       [[maybe_unused]] void *a3,
                                       generic_mash_data_ptrs *a4,
                                       [[maybe_unused]] void *a5)
{
    TRACE("resource_directory::un_mash_start");
 
    if constexpr (1)
    {
        //sp_log("0x%08X", a4->field_0);
        if (uint32_t v6 = 8 - ((uint32_t) a4->field_0 % 8u); v6 < 8) {
            a4->field_0 += v6;
        }
 
        this->parents.custom_un_mash(header, &this->parents, a4, nullptr);
        this->resource_locations.custom_un_mash(header, &this->resource_locations, a4, nullptr);
 
        this->texture_locations.custom_un_mash(header, &this->texture_locations, a4, nullptr);
 
        this->mesh_file_locations.custom_un_mash(header, &this->mesh_file_locations, a4, nullptr);
        this->mesh_locations.custom_un_mash(header, &this->mesh_locations, a4, nullptr);
        this->morph_file_locations.custom_un_mash(header, &this->morph_file_locations, a4, nullptr);
        this->morph_locations.custom_un_mash(header, &this->morph_locations, a4, nullptr);
        this->material_file_locations.custom_un_mash(header,
                                                     &this->material_file_locations,
                                                     a4,
                                                     nullptr);
        this->material_locations.custom_un_mash(header, &this->material_locations, a4, nullptr);
        this->anim_file_locations.custom_un_mash(header, &this->anim_file_locations, a4, nullptr);
        this->anim_locations.custom_un_mash(header, &this->anim_locations, a4, nullptr);
        this->scene_anim_locations.custom_un_mash(header, &this->scene_anim_locations, a4, nullptr);
 
        this->skeleton_locations.custom_un_mash(header, &this->skeleton_locations, a4, nullptr);
 
        this->field_68.custom_un_mash(header, &this->field_68, a4, nullptr);
        this->field_70.custom_un_mash(header, &this->field_70, a4, nullptr);
 
        // The retail PC build (0x0051F6E0) ends here. The per-vector
        // get_type() == TLRESOURCE_TYPE_* checks only exist in debug builds
        // (e.g. the Xbox debug listing) and are stripped from retail, so they
        // are omitted here to match the binary.
    } else {
        THISCALL(0x0051F6E0, this, header, a3, a4, a5);
    }
}
 
int resource_directory::get_resource_count(resource_key_type type) {
    assert(type > RESOURCE_KEY_TYPE_NONE && type < RESOURCE_KEY_TYPE_Z);
 
    return this->type_end_idxs[type];
}
 
mashable_vector<tlresource_location> *resource_directory::tlresource_type_to_vector(
    tlresource_type a2)
{
    mashable_vector<tlresource_location> *result = nullptr;
 
    if constexpr (1)
    {
        switch (a2) {
        case TLRESOURCE_TYPE_TEXTURE:
        case 13:
        case 14:
        case 15:
            result = &this->texture_locations;
            break;
        case TLRESOURCE_TYPE_MESH_FILE:
            result = &this->mesh_file_locations;
            break;
        case TLRESOURCE_TYPE_MESH:
            result = &this->mesh_locations;
            break;
        case TLRESOURCE_TYPE_MORPH_FILE:
            result = &this->morph_file_locations;
            break;
        case TLRESOURCE_TYPE_MORPH:
            result = &this->morph_locations;
            break;
        case TLRESOURCE_TYPE_MATERIAL_FILE:
            result = &this->material_file_locations;
            break;
        case TLRESOURCE_TYPE_MATERIAL:
            result = &this->material_locations;
            break;
        case TLRESOURCE_TYPE_ANIM_FILE:
            result = &this->anim_file_locations;
            break;
        case TLRESOURCE_TYPE_ANIM:
            result = &this->anim_locations;
            break;
        case TLRESOURCE_TYPE_SCENE_ANIM: {
            result = &this->scene_anim_locations;
            break;
        }
        case TLRESOURCE_TYPE_SKELETON: {
            result = &this->skeleton_locations;
            break;
        }
        default:
            assert(0 && "invalid tlresource type");
 
            result = nullptr;
            break;
        }
    } else {
        result = CAST(result, THISCALL(0x0050E380, this, a2));
    }
 
    return result;
}
 
uint8_t *resource_directory::get_mash_data(uint32_t offset)
{
    assert(base != 0);
    return bit_cast<uint8_t *>(base + offset);
}
 
void resource_directory::constructor_common(resource_pack_slot *a2, uint8_t *a3, uint8_t *a4, uint32_t a5, uint32_t a6)
{
    TRACE("resource_directory::constructor_common");
 
    if constexpr (1)
    {
        this->field_80 = a4;
        this->field_84 = a5;
        this->pack_slot = a2;
        this->base = (int) a3;
        this->field_88 = a6;
        if (this->base != 0)
        {
            if (this->field_70.size())
            {
                for (int i = 0; i < this->field_70.size(); ++i) {
                    this->field_70.m_data[i].field_8 += (int) a2->get_header_mem_addr();
                }
            }
 
            if (this->texture_locations.size())
            {
                for (int i = 0; i < this->texture_locations.size(); ++i)
                {
                    this->texture_locations.m_data[i].field_8 += this->base;
                }
            }
 
            if (this->mesh_file_locations.size())
            {
                for (int i = 0; i < this->mesh_file_locations.size(); ++i)
                {
                    auto &mesh_file_loc = this->mesh_file_locations.m_data[i];
                    mesh_file_loc.field_8 += this->base;
                }
            }
 
            if (this->mesh_locations.size())
            {
                for (int i = 0; i < this->mesh_locations.size(); ++i) {
                    this->mesh_locations.m_data[i].field_8 += this->base;
                }
            }
 
            if (this->morph_file_locations.size()) {
                for (int i = 0; i < this->morph_file_locations.size(); ++i) {
                    this->morph_file_locations.m_data[i].field_8 += this->base;
                }
            }
 
            if (this->morph_locations.size()) {
                for (int i = 0; i < this->morph_locations.size(); ++i) {
                    this->morph_locations.m_data[i].field_8 += this->base;
                }
            }
 
            if (this->material_file_locations.size()) {
                for (int i = 0; i < this->material_file_locations.size(); ++i) {
                    this->material_file_locations.m_data[i].field_8 += this->base;
                }
            }
 
            if (this->material_locations.size()) {
                for (int i = 0; i < this->material_locations.size(); ++i) {
                    this->material_locations.m_data[i].field_8 += this->base;
                }
            }
 
            if (this->anim_file_locations.size()) {
                for (int i = 0; i < this->anim_file_locations.size(); ++i) {
                    this->anim_file_locations.m_data[i].field_8 += this->base;
                }
            }
 
            if (this->anim_locations.size()) {
                for (int i = 0; i < this->anim_locations.size(); ++i) {
                    this->anim_locations.m_data[i].field_8 += this->base;
                }
            }
 
            if (this->scene_anim_locations.size()) {
                for (int i = 0; i < this->scene_anim_locations.size(); ++i) {
                    this->scene_anim_locations.m_data[i].field_8 += this->base;
                }
            }
 
            if (this->skeleton_locations.size() != 0)
            {
                for (int i = 0; i < this->skeleton_locations.size(); ++i) {
                    this->skeleton_locations.m_data[i].field_8 += this->base;
                }
            }
        }
 
        if constexpr (0) {
            sp_log("resource_locations: size = %d", this->resource_locations.size());
            for (auto i = 0u; i < this->resource_locations.size(); ++i) {
                auto &loc = this->resource_locations.at(i);
                sp_log("%d %s %d", i,
                        loc.field_0.get_platform_string(g_platform).c_str(),
                        loc.m_size);
            }
        }
 
    } else {
        THISCALL(0x0051F0D0, this, a2, a3, a4, a5, a6);
    }
}
 
void resource_directory::remove_parent(resource_directory *bye)
{
    assert(bye != nullptr);
 
    auto size = this->parents.size();
    for (auto i = 0; i < size; ++i)
    {
        if (this->parents.m_data[i] == bye)
        {
            assert((i == parents.size() - 1 || parents[i + 1] == nullptr) && "must remove the LAST parent from directory");
 
            auto v5 = bye->pack_slot->get_name_key().m_hash;
            auto v6 = this->pack_slot->get_name_key().m_hash;
            printf("Removed parent %s from %s", v5.to_string(), v6.to_string());
            this->parents.m_data[i] = nullptr;
            return;
        }
    }
 
}
 
void resource_directory::add_parent(resource_directory *new_dir)
{
    TRACE("resource_directory::add_parent");
 
    assert(new_dir != nullptr);
 
    auto size = this->parents.m_size;
    for (auto i = 0; i < size; ++i)
    {
        if (this->parents.m_data[i] == nullptr)
        {
            auto v9 = new_dir->pack_slot->get_name_key().m_hash;
            auto v8 = this->pack_slot->get_name_key().m_hash;
            printf("Added parent %s to %s", v9.to_string(), v8.to_string());
            this->parents.m_data[i] = new_dir;
            
            return;
        }
    }
 
    error("too many parents added to resource_directory");
}
 
int compare_resource_key_resource_location_just_hash(const resource_key &a1, resource_location &a2) {
    //sp_log("%s", a2->field_0.get_platform_string(g_platform).c_str());
    return CDECL_CALL(0x0055F7E0, &a1, &a2);
}
 
int resource_directory::get_type_start_idxs(resource_key_type type) {
    assert(type > RESOURCE_KEY_TYPE_NONE && type < RESOURCE_KEY_TYPE_Z);
 
    return this->type_start_idxs[type];
}
 
 
bool resource_directory::find_resource(const resource_key &a2,
                                       resource_directory **out_dir,
                                       resource_location **out_loc)
{
    TRACE("resource_directory::find_resource", a2.get_platform_string(g_platform).c_str());
    if constexpr (1)
    {
        assert(out_dir != nullptr && out_loc != nullptr);
 
        bool result;
 
        auto **v5 = out_dir;
        *out_dir = nullptr;
        *out_loc = nullptr;
        auto type = a2.get_type();
        auto begin_idx = this->type_start_idxs[type];
        auto end_idx = begin_idx + this->type_end_idxs[type];
        assert(begin_idx >= 0 && end_idx <= this->resource_locations.size());
 
        auto *v14 = this->resource_locations.m_data;
 
        auto index = 0;
        if (binary_search_array_cmp<const resource_key, resource_location>(
                &a2,
                v14,
                begin_idx,
                end_idx,
                &index,
                compare_resource_key_resource_location_just_hash))
        {
            if ( this->pack_slot != nullptr )
            {
                if ( os_developer_options::instance->get_flag(mString{"SHOW_RESOURCE_SPAM"}) )
                {
                    auto &v6 = this->pack_slot->get_name_key();
                    auto v27 = v6.m_hash;
                    auto v24 = a2.get_platform_string(g_platform);
                    auto *v20 = v27.to_string();
                    auto *v7 = v24.c_str();
                    debug_print_va("found resource %s in %s", v7, v20);
                }
            }
 
            uint16_t idx = index;
 
            *v5 = this;
            *out_loc = &this->resource_locations.m_data[idx];
            result = true;
        }
        else
        {
            if (this->parents.size() != 0)
            {
                for (auto i = 0u; i < this->parents.size(); ++i)
                {
                    auto *the_parent = this->parents.at(i);
                    if (the_parent == nullptr) {
                        break;
                    }
 
                    assert(the_parent->pack_slot != nullptr);
 
                    if (the_parent->pack_slot->get_partition()->get_type() == RESOURCE_PARTITION_STRIP) {
                        assert(the_parent->parents.size() == 1);
                        the_parent = the_parent->parents.at(0);
                    }
 
                    if ( this->pack_slot != nullptr
                            && os_developer_options::instance->get_flag(mString{"SHOW_RESOURCE_SPAM"}))
                    {
                        auto v28 = the_parent->pack_slot->get_name_key().m_hash;
                        auto v25 = this->pack_slot->get_name_key().m_hash;
 
                        auto v23 = a2.get_platform_string(g_platform);
                        auto *v21 = v28.to_string();
                        auto *v19 = v25.to_string();
                        auto *v13 = v23.c_str();
                        debug_print_va("didn't find resource %s in %s, checking parent %s", v13, v19, v21);
                    }
 
                    result = the_parent->find_resource(a2, v5, out_loc);
                    if (result) {
                        return result;
                    }
                }
            }
 
            result = false;
        }
 
        return result;
    } else {
        return (bool) THISCALL(0x0051F550, this, &a2, out_dir, out_loc);
    }
}
 
void resource_directory::debug_print() const {
    TRACE("resource_directory::debug_print");
 
    if ( this->pack_slot != nullptr )
    {
        auto &v1 = this->pack_slot->get_name_key();
        auto &v40 = v1.m_hash;
        auto *v2 = v40.to_string();
        debug_print_va("%s contains:", v2);
    }
 
    int i = 0;
    for ( i = 0; i < this->resource_locations.size(); ++i )
    {
        auto v39 = g_platform;
        auto &v4 = this->resource_locations.at(i);
        auto str = v4.field_0.get_platform_string(v39);
        auto *v5 = str.c_str();
        debug_print_va("  resource %s", v5);
    }
 
    for ( i = 0; i < this->texture_locations.size(); ++i )
    {
        auto &v7 = this->texture_locations.at(i);
        auto *v8 = v7.name.to_string();
        debug_print_va("  tlresource texture %s", v8);
    }
 
    for ( i = 0; i < this->mesh_file_locations.size(); ++i )
    {
        auto &v10 = this->mesh_file_locations.at(i);
        auto *v11 = v10.name.to_string();
        debug_print_va("  tlresource mesh file %s", v11);
    }
 
    for ( i = 0; i < this->mesh_locations.size(); ++i )
    {
        auto &v13 = this->mesh_locations.at(i);
        auto *v14 = v13.name.to_string();
        debug_print_va("  tlresource mesh %s", v14);
    }
 
    for ( i = 0; i < this->morph_file_locations.size(); ++i )
    {
        auto &v16 = this->morph_file_locations.at(i);
        auto *v17 = v16.name.to_string();
        debug_print_va("  tlresource morph file %s", v17);
    }
 
    for ( i = 0; i < this->morph_locations.size(); ++i )
    {
        auto &v19 = this->morph_locations.at(i);
        auto *v20 = v19.name.to_string();
        debug_print_va("  tlresource morph %s", v20);
    }
 
    for ( i = 0; i < this->material_file_locations.size(); ++i )
    {
        auto &v22 = this->material_file_locations.at(i);
        auto *v23 = v22.name.to_string();
        debug_print_va("  tlresource material file %s", v23);
    }
 
    for ( i = 0; i < this->material_locations.size(); ++i )
    {
        auto &v25 = this->material_locations.at(i);
        auto *v26 = v25.name.to_string();
        debug_print_va("  tlresource material %s", v26);
    }
 
    for ( i = 0; i < this->anim_file_locations.size(); ++i )
    {
        auto &v28 = this->anim_file_locations.at(i);
        auto *v29 = v28.name.to_string();
        debug_print_va("  tlresource anim file %s", v29);
    }
 
    for ( i = 0; i < this->anim_locations.size(); ++i )
    {
        auto &v31 = this->anim_locations.at(i);
        auto *v32 = v31.name.to_string();
        debug_print_va("  tlresource anim %s", v32);
    }
 
    for ( auto i = 0; i < this->scene_anim_locations.size(); ++i )
    {
        auto &v34 = this->scene_anim_locations.at(i);
        auto *v35 = v34.name.to_string();
        debug_print_va("  tlresource scene anim %s", v35);
    }
 
    for ( auto i = 0; i < this->skeleton_locations.size(); ++i )
    {
        auto &v37 = this->skeleton_locations.at(i);
        auto *v38 = v37.name.to_string();
        debug_print_va("  tlresource skeleton %s", v38);
    }
}
 
resource_location *resource_directory::get_resource_location(int idx) {
    assert(idx >= 0 && idx < resource_locations.size());
    return &this->resource_locations.at(idx);
}
 
uint8_t *resource_directory::get_resource(const resource_location *loc, resource_pack_slot **a3)
{
    TRACE("resource_directory::get_resource");
 
    assert(!this->resource_locations.empty());
    assert(loc != nullptr);
 
    assert(loc >= &(resource_locations.at(0)) &&
           loc <= &(resource_locations.at(resource_locations.size() - 1)));
 
    auto func = [this](uint32_t a2) -> uint8_t * {
        assert(this->base);
 
        return bit_cast<uint8_t *>(this->base + a2);
    };
 
    auto *result = func(loc->m_offset);
    if (a3 != nullptr) {
        *a3 = this->pack_slot;
    }
 
    return result;
}
 
uint8_t *resource_directory::get_resource(const resource_key &resource_id,
                                          int *mash_data_size,
                                          resource_pack_slot **a4)
{
    TRACE("resource_directory::get_resource");
 
    if constexpr (0)
    {
        assert(resource_id.is_set());
        assert(resource_id.get_type() != RESOURCE_KEY_TYPE_NONE);
        assert(pack_slot != nullptr);
 
        resource_location *found_loc = nullptr;
        resource_directory *found_dir = nullptr;
        if (!this->find_resource(resource_id, &found_dir, &found_loc)) {
            return nullptr;
        }
 
        assert(found_dir != nullptr && found_loc != nullptr);
 
        if (mash_data_size != nullptr) {
            *mash_data_size = found_loc->m_size;
        }
 
        auto *result = found_dir->get_resource(found_loc, a4);
        return result;
    } else {
        return (uint8_t *) THISCALL(0x0052AA70, this, &resource_id, mash_data_size, a4);
    }
}
 
int resource_directory::get_tlresource_count(tlresource_type a2) {
    auto *locations = this->tlresource_type_to_vector(a2);
    assert(locations != nullptr);
 
    return locations->size();
}
 
char *resource_directory::get_tlresource(const tlHashString &a1, tlresource_type a2) {
    auto v3 = a1.GetHash();
    auto *res = this->get_tlresource(v3, a2);
    if (res == nullptr && 1) {
        auto *v4 = a1.c_str();
        debug_print_va("Failed to find resource %s.", v4);
    }
 
    return res;
}
 
char *resource_directory::get_tlresource(const tlFixedString &a1, tlresource_type a2)
{
    TRACE("resource_directory::get_tlresource", a1.to_string());
 
    auto v3 = a1.m_hash;
    auto *v6 = this->get_tlresource(v3, a2);
 
    bool SHOW_RESOURCE_SPAM = os_developer_options::instance->get_flag(mString {"SHOW_RESOURCE_SPAM"});
    if ( v6 == nullptr && SHOW_RESOURCE_SPAM )
    {
        auto *v4 = a1.to_string();
        debug_print_va("Failed to find resource %s.", v4);
    }
 
    return v6;
}
 
void *resource_directory::allocate_from_pool(int a2, int a3) {
    return (void *) THISCALL(0x0051F620, this, a2, a3);
}
 
char *resource_directory::get_tlresource(tlresource_location *loc, resource_pack_slot **a3)
{
    assert(loc != nullptr);
 
    auto v3 = loc->get_type();
 
    auto *vec = this->tlresource_type_to_vector(v3);
    assert(vec != nullptr);
 
    assert(loc >= &(vec->at(0)) && loc <= &(vec->at(vec->size() - 1)));
 
    auto *v6 = loc->field_8;
    if (a3 != nullptr)
    {
        *a3 = this->pack_slot;
    }
 
    //sp_log("0x%08X", v6);
 
    return v6;
}
 
tlresource_location *resource_directory::get_tlresource_location(int idx, tlresource_type a2)
{
    auto *locations = tlresource_type_to_vector(a2);
    assert(locations != nullptr);
 
    assert(idx >= 0 && idx < locations->size());
    return &locations->at(idx);
}
 
char *resource_directory::get_tlresource(uint32_t a2, tlresource_type tlres_type)
{
    TRACE("resource_directory::get_tlresource");
 
    assert(tlres_type > TLRESOURCE_TYPE_NONE && tlres_type < TLRESOURCE_TYPE_Z);
    assert(pack_slot != nullptr);
 
    char *result = nullptr;
    resource_directory *found_dir = nullptr;
    tlresource_location *found_loc = nullptr;
    if (this->find_tlresource(a2, tlres_type, &found_dir, &found_loc))
    {
        assert(found_dir != nullptr && found_loc != nullptr);
 
        result = found_dir->get_tlresource(found_loc, nullptr);
    }
 
    return result;
}
 
static const char *tlresource_type_str[10] {
    "(none)",
    "texture",
    "mesh file",
    "mesh",
    "morph file",
    "morph",
    "anim file",
    "anim",
    "scene anim",
    "skeleton"
};
 
bool resource_directory::find_tlresource(uint32_t a1,
                                         tlresource_type tlres_type,
                                         resource_directory **out_dir,
                                         tlresource_location **out_loc)
{
    TRACE("resource_directory::find_tlresource", std::to_string(a1).c_str());
 
    if constexpr (0)
    {
        assert(tlres_type >= TLRESOURCE_TYPE_NONE && tlres_type < TLRESOURCE_TYPE_Z);
 
        if (out_dir != nullptr) {
            *out_dir = nullptr;
        }
 
        if (out_loc != nullptr) {
            *out_loc = nullptr;
        }
 
        tlresource_location *array = nullptr;
        auto array_size = 0;
        switch (tlres_type) {
        case 0: {
            assert(0);
            break;
        }
        case TLRESOURCE_TYPE_TEXTURE:
            array = this->texture_locations.m_data;
            array_size = this->texture_locations.m_size;
            break;
        case TLRESOURCE_TYPE_MESH_FILE:
            array = this->mesh_file_locations.m_data;
            array_size = this->mesh_file_locations.m_size;
            break;
        case TLRESOURCE_TYPE_MESH:
            array = this->mesh_locations.m_data;
            array_size = this->mesh_locations.m_size;
            break;
        case TLRESOURCE_TYPE_MORPH_FILE:
            array = this->morph_file_locations.m_data;
            array_size = this->morph_file_locations.m_size;
            break;
        case TLRESOURCE_TYPE_MORPH:
            array = this->morph_locations.m_data;
            array_size = this->morph_locations.m_size;
            break;
        case TLRESOURCE_TYPE_MATERIAL_FILE:
            array = this->material_file_locations.m_data;
            array_size = this->material_file_locations.m_size;
            break;
        case TLRESOURCE_TYPE_MATERIAL:
            array = this->material_locations.m_data;
            array_size = this->material_locations.m_size;
            break;
        case TLRESOURCE_TYPE_ANIM_FILE:
            array = this->anim_file_locations.m_data;
            array_size = this->anim_file_locations.m_size;
            break;
        case TLRESOURCE_TYPE_ANIM:
            array = this->anim_locations.m_data;
            array_size = this->anim_locations.m_size;
            break;
        case TLRESOURCE_TYPE_SCENE_ANIM:
            array = this->scene_anim_locations.m_data;
            array_size = this->scene_anim_locations.m_size;
            break;
        case TLRESOURCE_TYPE_SKELETON:
            array = this->skeleton_locations.m_data;
            array_size = this->skeleton_locations.m_size;
            break;
        default:
            assert(0 && "Unknown tlresource type");
            return false;
        }
 
        assert(array != nullptr);
 
        auto SHOW_RESOURCE_SPAM = os_developer_options::instance->get_flag(mString {"SHOW_RESOURCE_SPAM"});
 
        bool result = false;
 
        auto idx = 0;
        if (binary_search_array_cmp(&a1,
                                    array,
                                    0,
                                    array_size,
                                    &idx,
                                    compare_tlFixedString_tlresource_location))
        {
            if (out_loc != nullptr) {
                *out_loc = &array[idx];
            }
 
            result = true;
        }
 
        if (result)
        {
            if (SHOW_RESOURCE_SPAM)
            {
                //sp_log("idx = %d", idx);
                auto &v5 = this->pack_slot->get_name_key();
                auto v6 = v5.m_hash.to_string();
                debug_print_va("found tlresource %s 0x%08x in %s",
                       tlresource_type_str[tlres_type], a1, v6);
            }
 
            if (out_dir != nullptr) {
                *out_dir = this;
            }
 
            return result;
        }
        else
        {
            for (auto &the_parent : this->parents)
            {
                if (the_parent == nullptr) {
                    break;
                }
 
                assert(the_parent->pack_slot != nullptr);
 
                if (the_parent->pack_slot->get_partition()->get_type() == RESOURCE_PARTITION_STRIP)
                {
                    assert(the_parent->parents.size() == 1);
 
                    the_parent = the_parent->parents.at(0);
                }
 
                if (SHOW_RESOURCE_SPAM)
                {
                    auto v18 = the_parent->pack_slot->get_name_key().m_hash;
 
                    auto v16 = this->pack_slot->get_name_key().m_hash;
 
                    auto *v15 = v18.to_string();
                    auto *v12 = v16.to_string();
                    debug_print_va("didn't find tlresource %s 0x%08x in %s, checking parent %s",
                           tlresource_type_str[tlres_type],
                           a1,
                           v12,
                           v15);
                }
 
                result = the_parent->find_tlresource(a1, tlres_type, out_dir, out_loc);
                if (result) {
                    return result;
                }
            }
 
            if (SHOW_RESOURCE_SPAM)
            {
                auto v19 = this->pack_slot->get_name_key().m_hash;
 
                auto *v14 = v19.to_string();
                debug_print_va("didn't find tlresource %s 0x%08x in %s",
                       tlresource_type_str[tlres_type],
                       a1,
                       v14);
            }
        }
 
        return result;
    }
    else
    {
        bool (__fastcall *func)(resource_directory *, void *,
                                uint32_t,
                                tlresource_type,
                                resource_directory **,
                                tlresource_location **) = CAST(func, 0x0051F350);
 
        bool res = func(this, nullptr, a1, tlres_type, out_dir, out_loc);
        if (res) {
            // Typed lookup, and never for the types whose field_8 is a LIVE
            // STRUCT rather than file bytes: MESH_FILE holds the nglMeshFile
            // itself (raw .PCMESH mods ride nglLoadMeshFileInternal through
            // FileBuf instead - see mesh_file_resource_handler), and the NAL
            // types hold parsed nalAnimFile / nalSceneAnim / nalBaseSkeleton
            // images whose overrides are owned by nal_system.cpp
            // (nalFlushPendingAnimOverrides / nalLoadSceneAnimInternal) and
            // nalConstructSkeleton. Stuffing raw, un-parsed mod bytes into
            // field_8 for any of those hands the consumer an image whose
            // offsets were never fixed up into pointers.
            if (tlres_type != TLRESOURCE_TYPE_MESH_FILE &&
                tlres_type != TLRESOURCE_TYPE_ANIM_FILE &&
                tlres_type != TLRESOURCE_TYPE_SCENE_ANIM &&
                tlres_type != TLRESOURCE_TYPE_SKELETON) {
                if (Mod* mod = getMod(a1, (int)tlres_type))
                    (*out_loc)->field_8 = reinterpret_cast<char*>(mod->Data.data());
            }
        }
        return res;
    }
}
 
char *resource_directory::add_tlresource(tlFixedString *arg0,
                                         tlresource_type tlres_type,
                                         char *data) {
    char *result = nullptr;
 
    auto v20 = 0;
    assert(pack_slot != nullptr);
    assert(tlres_type >= 0 && tlres_type < TLRESOURCE_TYPE_Z);
    assert(data != nullptr);
 
    mashable_vector<tlresource_location> *vec = nullptr;
    tlresource_location *loc = nullptr;
    auto index = 0;
    if (tlres_type == TLRESOURCE_TYPE_TEXTURE) {
        vec = &this->texture_locations;
    } else if (tlres_type == TLRESOURCE_TYPE_ANIM_FILE) {
        vec = &this->anim_file_locations;
    } else if constexpr (0) {
        assert(0 && "Invalid tlresource type");
    }
 
    assert(vec != nullptr);
 
    int v11 = vec->size();
    auto *v4 = vec->data();
    if (binary_search_array_cmp(&arg0->m_hash,
                                v4,
                                0,
                                v11,
                                &index,
                                compare_tlFixedString_tlresource_location))
        loc = &vec->at(index);
    if (loc != nullptr) {
        int v5 = arg0->m_hash;
 
        string_hash key{v5};
 
        v20 |= 1u;
 
        if ((v20 & 1) != 0) {
            v20 &= 0xFFFFFFFE;
        }
 
        assert(loc->name == key && "Didn't find what we were looking for");
        loc->field_8 = data;
        if constexpr (1) {
            auto v14 = this->pack_slot->get_name_key().m_hash;
 
            auto *v12 = v14.to_string();
            auto *v7 = arg0->to_string();
            debug_print_va("added tlresource %s %s to %s",
                    tlresource_type_str[tlres_type], v7, v12);
        }
 
        result = data;
    } else {
        if constexpr (1) {
            auto &v9 = this->pack_slot->get_name_key();
            auto v15 = v9.m_hash;
 
            auto *v13 = v15.to_string();
            auto *v10 = arg0->to_string();
            debug_print_va("couldn't add tlresource %s %s to %s",
                   tlresource_type_str[tlres_type],
                   v10,
                   v13);
        }
 
        assert(0 && "Couldn't find place to add tlresource");
 
        result = nullptr;
    }
 
    return result;
}
 
bool resource_directory::sub_9C2EE0() {
    for (int i = 0; i < this->field_70.size(); ++i) {
        resource_allocation_pool *v1 = &this->field_70.at(i);
        if (!(v1->count == 0)) {
            return false;
        }
    }
 
    return true;
}
 
void resource_directory::release_mem() {
    THISCALL(0x0051F7D0, this);
}
void resource_directory_patch()
{
    {
        FUNC_ADDRESS(address, &resource_directory::add_parent);
        REDIRECT(0x005D1FB3, address);
    }

    {
        FUNC_ADDRESS(address, &resource_directory::find_resource);
        SET_JUMP(0x0051F550, address);
    }

    {
        uint8_t * (resource_directory::*func)(
            const resource_key &resource_id,
            int *a3,
            resource_pack_slot **a4) = &resource_directory::get_resource;

        FUNC_ADDRESS(address, func);
        REDIRECT(0x0062F022, address);
    }

    {
        FUNC_ADDRESS(address, &resource_directory::find_tlresource);
        REDIRECT(0x00566128, address);
        REDIRECT(0x00565F48, address);
        REDIRECT(0x00569250, address);
    }
	    {
        FUNC_ADDRESS(address, &resource_directory::un_mash_start);
        REDIRECT(0x0053E21C, address);
		REDIRECT(0x00563F9E, address);

    }
    return;

    {
        FUNC_ADDRESS(address, &resource_directory::un_mash_start);
        REDIRECT(0x0053E21C, address);
    }

    {
        FUNC_ADDRESS(address, &resource_directory::tlresource_type_to_vector);
        REDIRECT(0x00562F0A, address);
        REDIRECT(0x00562F2A, address);
    }
	

	
	

    REDIRECT(0x0053E204, parse_generic_mash_init);
}
