#include "conglom.h"

#include "app.h"
#include "als_res_data.h"
#include "animation_interface.h"
#include "base_ai_data.h"
#include "beam.h"
#include "camera.h"
#include "collision_geometry.h"
#include "common.h"
#include "custom_math.h"
#include "debug_render.h"
#include "decal_data_interface.h"
#include "entity_mash.h"
#include "femanager.h"
#include "func_wrapper.h"
#include "game.h"
#include "geometry_manager.h"
#include "light_manager.h"
#include "ngl.h"

#if defined(TARGET_XBOX) || defined(OPENUSM_XBPACK_MODE)
#include "mashable_interface.h"
#endif

#include "memory.h"
#include "ngl_mesh.h"
#include "oldmath_po.h"
#include "os_developer_options.h"
#include "parse_generic_mash.h"
#include "potential_shadow.h"
#include "script_data_interface.h"
#include "region.h"
#include "render_text.h"
#include "skeleton_interface.h"
#include "tentacle_interface.h"
#include "trace.h"
#include "utility.h"
#include "variables.h"
#include "variant_interface.h"
#include "vector2di.h"
#include "vtbl.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>

VALIDATE_SIZE(conglomerate, 0x130);

static constexpr auto MAX_BONES = 64u;
static constexpr auto MAX_MEMBERS = 16u;
static constexpr auto MAX_BONES_AND_MEMBERS = MAX_MEMBERS + MAX_BONES;

conglomerate::conglomerate(const string_hash &a2, unsigned int a3) : actor(a2, a3) {}

void conglomerate::init_member_data()
{
    this->skeleton_ifc = nullptr;
    this->field_11C = nullptr;
    this->script_data_ifc = nullptr;
    this->field_124 = nullptr;
    this->my_decal_data_interface = nullptr;
    this->m_variant_interface = nullptr;
    this->field_FC = nullptr;
    this->field_100 = nullptr;
    this->field_104 = 0;
    this->field_108 = -1.0;
    this->field_10C = -1.0;
    this->set_flag_recursive(static_cast<entity_flag_t>(4u), true);
    this->field_110 = 3;
}

void conglomerate::create_parentage_tree()
{
    TRACE("conglomerate::create_parentage_tree");

    THISCALL(0x004E55E0, this);
}

void conglomerate::create_variant_ifc()
{
    auto *mem = mem_alloc(sizeof(variant_interface));
    this->m_variant_interface = new (mem) variant_interface {this};
}

void conglomerate::remove_member_lights_from_region(region *a2)
{
	if constexpr (0) {
		for ( auto &v1 : (*this->field_100) ) {
			a2->remove(v1);
		}
	} else {
		THISCALL(0x004D27A0, this, a2);
	}
}

void conglomerate::sub_4D0E00()
{
    THISCALL(0x004D0E00, this);
}

als::animation_logic_system *conglomerate::get_my_als()
{
    auto *v1 = this->field_114;
    if ( v1 != nullptr ) {
        return v1->field_8;
    }

    return nullptr;
}

#ifdef OPENUSM_XBPACK_V10
constexpr auto _ENTM_TYPE_MAX = 29u;
#else
constexpr auto _ENTM_TYPE_MAX = 28u;
#endif

#ifdef OPENUSM_XBPACK_MODE
namespace
{
struct xb_ifc_t
{
    uint8_t *image;
    uint32_t size;
};

struct pc_tentacle_hash_entry
{
    uint32_t hash;
    uint32_t aux;
};

struct pc_tentacle_record
{
    uint32_t field_0;
    uint32_t field_4;
    uint32_t field_8;
    float radius;
    char texture[0x20];
    uint32_t source_hash;
    uint32_t source_hash_aux;
    entity_base *resolved_entity;
    mashable_vector<pc_tentacle_hash_entry> control_hashes;
    mashable_vector<entity_base *> control_entities;
};

static_assert(sizeof(pc_tentacle_record) == 0x4C);
static_assert(offsetof(pc_tentacle_record, texture) == 0x10);
static_assert(offsetof(pc_tentacle_record, source_hash) == 0x30);
static_assert(offsetof(pc_tentacle_record, control_hashes) == 0x3C);
static_assert(offsetof(pc_tentacle_record, control_entities) == 0x44);

#ifdef OPENUSM_XBPACK_V10
struct xb_v10_tentacle_record
{
    pc_tentacle_record record;
    uint32_t field_4C;
};

static_assert(sizeof(xb_v10_tentacle_record) == 0x50);
#endif

bool read_ifc(
    generic_mash_data_ptrs *data,
    xb_ifc_t &ifc)
{
    ifc = {};
    const auto exists = *data->get<uint32_t>();
    if ( exists == 0 )
        return false;

    rebase(data->field_0, 4);
    ifc.size = *data->get<uint32_t>();
    rebase(data->field_0, 16);
    rebase(data->field_0, 4);
    ifc.image = data->field_0;
    data->field_0 += ifc.size;
    return true;
}

bool take_bytes(
    uint8_t *&cursor,
    uint8_t *limit,
    size_t byte_count,
    size_t alignment,
    uint8_t *&result)
{
    if ( alignment == 0 || (alignment & (alignment - 1)) != 0 )
        return false;

    const auto value = reinterpret_cast<uintptr_t>(cursor);
    const auto end = reinterpret_cast<uintptr_t>(limit);
    if ( value > UINTPTR_MAX - (alignment - 1) )
        return false;

    const auto aligned = (value + alignment - 1) & ~(alignment - 1);
    if ( aligned > end || byte_count > end - aligned )
        return false;

    result = reinterpret_cast<uint8_t *>(aligned);
    cursor = result + byte_count;
    return true;
}

bool take_array(
    uint8_t *&cursor,
    uint8_t *limit,
    uint32_t count,
    size_t element_size,
    size_t alignment,
    uint8_t *&result)
{
    const auto byte_count = static_cast<uint64_t>(count) * element_size;
    if ( byte_count > UINT32_MAX )
        return false;

    return take_bytes(
        cursor,
        limit,
        static_cast<size_t>(byte_count),
        alignment,
        result);
}

uint32_t read_u32(const uint8_t *source)
{
    uint32_t value;
    std::memcpy(&value, source, sizeof(value));
    return value;
}

bool take_u32(uint8_t *&cursor, uint8_t *limit, uint32_t &value)
{
    uint8_t *source = nullptr;
    if ( !take_bytes(cursor, limit, sizeof(value), 4, source) )
        return false;

    value = read_u32(source);
    return true;
}

bool read_tentacle_records(
    const xb_ifc_t &ifc,
    pc_tentacle_record *records,
    uint16_t &record_count)
{
    constexpr uint32_t XB_TENTACLE_VTABLE_HASH = 0x170BCC20;
    constexpr size_t XB_TENTACLE_RECORDS_OFFSET = 0x50;
    constexpr size_t XB_TENTACLE_RECORD_SIZE = 0x4C;

    if ( ifc.image == nullptr || ifc.size < XB_TENTACLE_RECORDS_OFFSET )
        return false;

    const auto image_address = reinterpret_cast<uintptr_t>(ifc.image);
    if ( ifc.size > UINTPTR_MAX - image_address )
        return false;

    auto *image_end = ifc.image + ifc.size;
    if ( read_u32(ifc.image) != 0x6D617368
        || read_u32(ifc.image + 0x10) != XB_TENTACLE_VTABLE_HASH )
        return false;

    const auto shared_offset = read_u32(ifc.image + 8);
    if ( shared_offset < XB_TENTACLE_RECORDS_OFFSET || shared_offset > ifc.size )
        return false;

    auto *normal_cursor = ifc.image + XB_TENTACLE_RECORDS_OFFSET;
    auto *shared_cursor = ifc.image + shared_offset;
    auto *const normal_limit = shared_cursor;
    auto *const shared_limit = image_end;

    uint32_t count = 0;
    if ( !take_u32(shared_cursor, shared_limit, count)
        || count > UINT16_MAX )
        return false;

    record_count = static_cast<uint16_t>(count);

    uint8_t *record_ptrs = nullptr;
    if ( !take_array(
            normal_cursor,
            normal_limit,
            count,
            sizeof(uint32_t),
            4,
            record_ptrs) )
        return false;
    (void) record_ptrs;

    for ( uint32_t i = 0; i < count; ++i )
    {
        uint8_t *xb_record = nullptr;
        if ( !take_bytes(
                normal_cursor,
                normal_limit,
                XB_TENTACLE_RECORD_SIZE,
                4,
                xb_record) )
            return false;

        int32_t texture_length;
        std::memcpy(
            &texture_length,
            xb_record + 0x10,
            sizeof(texture_length));
        if ( texture_length >= static_cast<int32_t>(sizeof(pc_tentacle_record::texture)) )
            return false;

        uint8_t *texture = nullptr;
        if ( texture_length > 0 )
        {
            if ( !take_bytes(
                    normal_cursor,
                    normal_limit,
                    static_cast<size_t>(texture_length) + 1,
                    1,
                    texture)
                || texture[texture_length] != '\0' )
                return false;
        }

        uint32_t hash_count = 0;
        uint32_t entity_count = 0;
        uint8_t *hashes = nullptr;
        uint8_t *entities = nullptr;
        if ( !take_u32(shared_cursor, shared_limit, hash_count)
            || hash_count > UINT16_MAX
            || !take_array(
                normal_cursor,
                normal_limit,
                hash_count,
                sizeof(pc_tentacle_hash_entry),
                4,
                hashes)
            || !take_u32(shared_cursor, shared_limit, entity_count)
            || entity_count != hash_count
            || !take_array(
                normal_cursor,
                normal_limit,
                entity_count,
                sizeof(entity_base *),
                4,
                entities) )
            return false;

        if ( records != nullptr )
        {
            auto &record = records[i];
            std::memcpy(&record, xb_record, 0x10);
            if ( texture != nullptr )
                std::memcpy(record.texture, texture, texture_length + 1);
            std::memcpy(&record.source_hash, xb_record + 0x1C, 8);
            record.resolved_entity = nullptr;
            record.control_hashes = {
                reinterpret_cast<pc_tentacle_hash_entry *>(hashes),
                static_cast<uint16_t>(hash_count),
                false,
                true};
            record.control_entities = {
                reinterpret_cast<entity_base **>(entities),
                static_cast<uint16_t>(entity_count),
                false,
                true};
        }
    }

    return true;
}

tentacle_interface *tentacle_error(const char *reason)
{
    sp_log("Invalid xb tentacle interface: %s", reason);
    assert(0 && "Invalid xb tentacle interface");
    return nullptr;
}

#ifdef OPENUSM_XBPACK_V10
tentacle_interface *unmash_v10_tentacle_ifc(
    generic_mash_header *header,
    generic_mash_data_ptrs *data,
    tentacle_interface *ifc,
    conglomerate *owner)
{
    auto *xb_records =
        reinterpret_cast<mashable_vector<xb_v10_tentacle_record> *>(
            &ifc->field_1C);
    const auto count = xb_records->m_size;
    const auto records_size =
        static_cast<size_t>(count) * sizeof(xb_v10_tentacle_record);
    xb_v10_tentacle_record *source = nullptr;

    const auto unmash_records = [&]()
    {
        for ( uint16_t i = 0; i < count; ++i )
        {
            auto &record = source[i].record;
            const auto *hashes = data->get<uint32_t>(2);
            record.source_hash = hashes[0];
            record.source_hash_aux = hashes[1];

            THISCALL(
                0x004C56B0,
                &record.control_hashes,
                header,
                &record.control_hashes,
                data,
                nullptr);
            THISCALL(
                0x004C6120,
                &record.control_entities,
                header,
                &record.control_entities,
                data,
                nullptr);
        }
    };

    if ( xb_records->m_shared )
    {
        rebase(data->field_4, 4);
        auto *meta = reinterpret_cast<uint32_t *>(data->field_4);
        const auto normal_advance = meta[0];
        const auto shared_advance = meta[1];
        data->field_4 += 12;

        rebase(data->field_4, 4);
        source = reinterpret_cast<xb_v10_tentacle_record *>(data->field_4);
        data->field_4 += records_size;

        if ( meta[2] != 0 )
        {
            if ( shared_advance < records_size )
                return tentacle_error("invalid v10 shared record span");

            data->field_0 += normal_advance;
            data->field_4 += shared_advance - records_size;
        }
        else
        {
            unmash_records();
        }

        ++meta[2];
        rebase(data->field_4, 4);
    }
    else
    {
        rebase(data->field_0, 4);
        source = reinterpret_cast<xb_v10_tentacle_record *>(data->field_0);
        data->field_0 += records_size;
        unmash_records();
        rebase(data->field_0, 4);
    }

    const auto pc_records_size =
        static_cast<size_t>(count) * sizeof(pc_tentacle_record);
    auto *records = count == 0
        ? nullptr
        : static_cast<pc_tentacle_record *>(mem_alloc(pc_records_size));
    if ( count != 0 && records == nullptr )
        return tentacle_error("v10 record allocation failed");

    for ( uint16_t i = 0; i < count; ++i )
        std::memcpy(&records[i], &source[i].record, sizeof(records[i]));

    const mashable_vector<pc_tentacle_record> record_vector {
        records,
        count,
        false,
        true};
    std::memcpy(&ifc->field_1C, &record_vector, sizeof(record_vector));

    rebase(data->field_0, 4);
    auto *polytubes = data->get<void *>(count);
    if ( count != 0 )
        std::memset(polytubes, 0, static_cast<size_t>(count) * sizeof(void *));

    ifc->field_4 = owner;
    ifc->field_8 = false;
    ifc->field_24 = static_cast<int>(
        reinterpret_cast<uintptr_t>(polytubes));
    ifc->field_28 = 3;
    ifc->field_34 = 0;
    return ifc;
}
#endif

tentacle_interface *unmash_tentacle_ifc(
    generic_mash_data_ptrs *data,
    conglomerate *owner)
{
    xb_ifc_t source;
    if ( !read_ifc(data, source) )
        return nullptr;

    uint16_t record_count = 0;
    if ( !read_tentacle_records(source, nullptr, record_count) )
        return tentacle_error("layout validation failed");

    const auto records_size =
        static_cast<size_t>(record_count) * sizeof(pc_tentacle_record);
    auto *records = record_count == 0
        ? nullptr
        : static_cast<pc_tentacle_record *>(mem_alloc(records_size));
    if ( record_count != 0 && records == nullptr )
        return tentacle_error("record allocation failed");
    if ( records != nullptr )
        std::memset(records, 0, records_size);

    uint16_t parsed_count = 0;
    if ( !read_tentacle_records(
            source,
            records,
            parsed_count)
        || parsed_count != record_count )
    {
        if ( records != nullptr )
            mem_dealloc(records, records_size);
        return tentacle_error("record conversion failed");
    }

    auto *polytubes = record_count == 0
        ? nullptr
        : mem_alloc(static_cast<size_t>(record_count) * sizeof(void *));
    if ( record_count != 0 && polytubes == nullptr )
    {
        mem_dealloc(records, records_size);
        return tentacle_error("runtime allocation failed");
    }
    if ( polytubes != nullptr )
        std::memset(
            polytubes,
            0,
            static_cast<size_t>(record_count) * sizeof(void *));

    auto *ifc = static_cast<tentacle_interface *>(
        mem_alloc(sizeof(tentacle_interface)));
    if ( ifc == nullptr )
    {
        if ( polytubes != nullptr )
            mem_dealloc(
                polytubes,
                static_cast<size_t>(record_count) * sizeof(void *));
        if ( records != nullptr )
            mem_dealloc(records, records_size);
        return tentacle_error("interface allocation failed");
    }
    std::memset(ifc, 0, sizeof(*ifc));
    ifc->m_vtbl = ifc_v_table_lookup()[8];
    ifc->field_4 = owner;

    const mashable_vector<pc_tentacle_record> record_vector {
        records,
        record_count,
        false,
        true};
    std::memcpy(&ifc->field_1C, &record_vector, sizeof(record_vector));
    ifc->field_24 = static_cast<int>(
        reinterpret_cast<uintptr_t>(polytubes));
    ifc->field_28 = 3;
    ifc->field_34 = 0;

    sp_log(
        "Converted xb tentacle interface with %u records",
        static_cast<unsigned int>(record_count));
    return ifc;
}

static bool skip_ifc(
    generic_mash_data_ptrs *data,
    const char *name)
{
    xb_ifc_t ifc;
    if ( !read_ifc(data, ifc) )
        return false;

    // The nested Xbox image is not a PC interface object.  It is still a
    // valid optional part of the conglomerate, so consume the complete image
    // and leave the corresponding PC pointer null.  Asserting here made every
    // character carrying script-data or variant metadata unspawnable even
    // though the remaining conglomerate stream was parsed correctly.
    sp_log("Discarded unsupported xb %s mashable interface", name);
    return true;
}
}
#endif

void conglomerate::_un_mash(generic_mash_header *a2, void *a3, generic_mash_data_ptrs *a4)
{
    TRACE("conglomerate::un_mash", this->field_10.to_string());

    if constexpr (1)
    {
        this->field_110 = *a4->get_from_shared<int>();
        if ( this != (conglomerate *)-248 )
        {
            this->field_F8 = nullptr;
        }


#if !defined(TARGET_XBOX) && (!defined(OPENUSM_XBPACK_MODE) || defined(OPENUSM_XBPACK_V10))
        if ( (a2->field_E & 0x40) != 0 )
        {
            rebase(a4->field_0, 4);

            this->skeleton_ifc = a4->get<skeleton_interface>();
            fix_ifc_v_table((char *) this->skeleton_ifc, (eEntityMashIFCTypeEnum) 6);

            this->skeleton_ifc->un_mash(
                a2,
                this,
                this->skeleton_ifc,
                a4);
        }
        else
        {
            this->skeleton_ifc = nullptr;
        }
#endif

        this->member_abs_po.custom_un_mash(a2, &this->member_abs_po, a4, nullptr);
        this->all_rel_po.custom_un_mash(a2, &this->all_rel_po, a4, nullptr);
        this->all_model_po.custom_un_mash(a2, &this->all_model_po, a4, nullptr);

        {
            auto *header = a2;
            assert(((int)header) % 4 == 0);
        }

#ifdef TARGET_XBOX
        mashable_interface<skeleton_interface> tmp_skeleton_ifc;
        tmp_skeleton_ifc.custom_un_mash(a2, &tmp_skeleton_ifc, a4);

        if ( tmp_skeleton_ifc.exists() )
        {
            auto *skel_ifc = tmp_skeleton_ifc.get_interface();
            skel_ifc->field_4 = this;
        }

        skeleton_ifc = tmp_skeleton_ifc.get_interface();
#endif

#if defined(OPENUSM_XBPACK_MODE) && !defined(OPENUSM_XBPACK_V10)
        mashable_interface<skeleton_interface> tmp_skeleton_ifc;
        tmp_skeleton_ifc.custom_un_mash(a2, &tmp_skeleton_ifc, a4);
        skeleton_ifc = tmp_skeleton_ifc.get_interface();
        if ( skeleton_ifc != nullptr )
            skeleton_ifc->field_4 = this;

        // no anim_interface...
        field_11C = nullptr;
        script_data_ifc = nullptr;
        field_124 = nullptr;
        m_variant_interface = nullptr;
        skip_ifc(a4, "script_data_interface");
        field_124 = unmash_tentacle_ifc(a4, this);
        skip_ifc(a4, "variant_interface");

        if ( (a2->field_E & 0x1000) != 0 )
        {
            rebase(a4->field_0, 4);
            my_decal_data_interface = a4->get<decal_data_interface>();
            fix_ifc_v_table(
                reinterpret_cast<char *>(my_decal_data_interface),
                static_cast<eEntityMashIFCTypeEnum>(9));
            my_decal_data_interface->un_mash(
                a2,
                this,
                my_decal_data_interface,
                a4);
            assert(!my_decal_data_interface->is_dynamic());
        }
        else
        {
            my_decal_data_interface = nullptr;
        }
#endif

#if !defined(TARGET_XBOX) && (!defined(OPENUSM_XBPACK_MODE) || defined(OPENUSM_XBPACK_V10))
        if ( (a2->field_E & 4) != 0 )
        {
            rebase(a4->field_0, 4);

            this->field_11C = a4->get<animation_interface>();

            fix_ifc_v_table((char *)this->field_11C, (eEntityMashIFCTypeEnum) 0);

            this->field_11C->un_mash(a2, this,
                this->field_11C,
                a4);
        }
        else
        {
            this->field_11C = nullptr;
        }

        if ( (a2->field_E & 0x20) != 0 )
        {
            rebase(a4->field_0, 4);

            this->script_data_ifc = a4->get<script_data_interface>();

            fix_ifc_v_table((char *)this->script_data_ifc, (eEntityMashIFCTypeEnum) 3);

            this->script_data_ifc->un_mash(
                a2,
                this,
                this->script_data_ifc,
                a4);
        }
        else
        {
            this->script_data_ifc = nullptr;
        }

        if ( (a2->field_E & 0x2000) != 0 )
        {
            rebase(a4->field_0, 4);

            this->field_124 = a4->get<tentacle_interface>();

            fix_ifc_v_table((char *) this->field_124, (eEntityMashIFCTypeEnum) 8);
#ifdef OPENUSM_XBPACK_V10
            this->field_124 = unmash_v10_tentacle_ifc(
                a2,
                a4,
                this->field_124,
                this);
#else
            this->field_124->un_mash(
                a2,
                this,
                this->field_124,
                a4);
#endif
        }
        else
        {
            this->field_124 = nullptr;
        }

        if ( (a2->field_E & 0x1000) != 0 )
        {
            rebase(a4->field_0, 4);

            this->my_decal_data_interface = a4->get<decal_data_interface>();

            fix_ifc_v_table((char *) this->my_decal_data_interface, (eEntityMashIFCTypeEnum) 9);
            this->my_decal_data_interface->un_mash(
                a2,
                this,
                this->my_decal_data_interface,
                a4);

            assert(!my_decal_data_interface->is_dynamic());
        }
        else
        {
            this->my_decal_data_interface = nullptr;
        }

        assert(((int)a2) % 4 == 0);

        if ( (a2->field_E & 0x4000) != 0 )
        {
            rebase(a4->field_0, 4);

            this->m_variant_interface = a4->get<variant_interface>();

            fix_ifc_v_table((char *) this->m_variant_interface, static_cast<eEntityMashIFCTypeEnum>(10));

            this->m_variant_interface->un_mash(
                a2,
                this,
                this->m_variant_interface,
                a4);
        }
        else
        {
            this->m_variant_interface = nullptr;
        }
#endif

        assert(((int)a2) % 4 == 0);

        this->field_E8.custom_un_mash(a2, &this->field_E8, a4, nullptr);
        this->field_F0.custom_un_mash(a2, &this->field_F0, a4, nullptr);
        this->members.custom_un_mash(a2, &this->members, a4, nullptr);

        assert(members.size() == member_abs_po.size());

        auto *v87 = this->member_abs_po.m_data;
        this->field_110 &= 0xFFFFFFF3;
        this->field_FC = nullptr;
        this->field_100 = nullptr;
        this->set_flag_recursive(static_cast<entity_flag_t>(4), true);

        this->set_flag_recursive(static_cast<entity_flag_t>(2), false);
        this->set_flag_recursive(static_cast<entity_flag_t>(1), false);

        this->my_conglom_root = this;

        sp_log("members.size() = %d", this->members.size());
        for (auto &member : this->members)
        {
            uint16_t __ENT_TYPE = *a4->get_from_shared<uint16_t>();

            assert(__ENT_TYPE <= _ENTM_TYPE_MAX);

            actor *tmp_ptr = nullptr;

            if ( __ENT_TYPE != _ENTM_TYPE_MAX )
            {
                rebase(a4->field_4, 4);
                
                auto *header = a4->get_from_shared<generic_mash_header>();
                assert(((int)header) % 4 == 0);

                assert(__ENT_TYPE == header->get_class_id());

                const auto ent_type = pc_entity_mash_type(__ENT_TYPE);

                rebase(a4->field_0, sizeof(generic_mash_header));

                entity_base *__ENT_ptr = CAST(__ENT_ptr, a4->field_0);
                a4->field_0 += entity_mash_size(__ENT_TYPE);

                {
                    eEntityMashTypeEnum v11 = (eEntityMashTypeEnum) ent_type;
                    fix_entity_v_table((char *)__ENT_ptr, v11);
                }

                assert(__ENT_ptr->rel_po_idx != 0xFF);
                assert(__ENT_ptr->rel_po_idx < all_rel_po.size());

                __ENT_ptr->my_rel_po = &this->all_rel_po.m_data[__ENT_ptr->rel_po_idx];
                __ENT_ptr->my_conglom_root = this;
                __ENT_ptr->un_mash_start(header, __ENT_ptr, a4, nullptr);

                __ENT_ptr->field_8 |= 0x10u;
                __ENT_ptr->field_3E = 0;

                tmp_ptr = CAST(tmp_ptr, __ENT_ptr);
            }

            assert(tmp_ptr->is_conglom_member());

            assert(!tmp_ptr->manage_abs_po());

            tmp_ptr->my_abs_po = v87++;
            if ( tmp_ptr->is_an_actor() )
            {
                if ( tmp_ptr->get_colgeom() != nullptr )
                {
                    this->field_110 |= 4u;
                    this->set_flag_recursive(static_cast<entity_flag_t>(2), true);
                    if ( tmp_ptr->possibly_collide() )
                    {
                        if ( this->field_FC == nullptr )
                        {
                            auto *v31 = mem_alloc(sizeof(*this->field_FC));
                            this->field_FC = new (v31) actor_list_t {};
                        }

                        this->field_FC->push_back(tmp_ptr);
                    }
                }

                if ( tmp_ptr->get_ai_core() )
                {
                    this->field_110 |= 8u;
                    this->set_flag_recursive(static_cast<entity_flag_t>(1), true);
                }

                if ( this->is_material_switching() ) {
                    for (auto i = 0; i < 4; ++i) {
                        auto v20 = this->field_90.field_C[i];
                        tmp_ptr->field_90.field_C[i] = v20;
                    }
                }
            }

            if ( tmp_ptr->is_a_light_source() ) {
                if ( this->field_100 == nullptr ) {
                    auto *v39 = mem_alloc(sizeof(*this->field_100));
                    this->field_100 = new (v39) light_list_t {};
                }

                this->field_100->push_back(bit_cast<light_source *>(tmp_ptr));
            }

            member = tmp_ptr;
        }

        this->skin_bones.custom_un_mash(a2, &this->skin_bones, a4, nullptr);

        sp_log("skin_bones_size = %d", this->skin_bones.size());

#ifdef TARGET_XBOX
        assert(skin_bones.empty() || tmp_skeleton_ifc.exists());
#endif

        for (auto &bone : this->skin_bones)
        {
            uint16_t __ENT_TYPE = *a4->get_from_shared<uint16_t>();

            assert(__ENT_TYPE <= _ENTM_TYPE_MAX);

            actor *tmp_ptr = nullptr;
            if ( __ENT_TYPE != _ENTM_TYPE_MAX )
            {
                rebase(a4->field_4, 4);

                auto *header = a4->get_from_shared<generic_mash_header>();
                assert(((int)header) % 4 == 0);

                assert(__ENT_TYPE == header->get_class_id());

                const auto ent_type = pc_entity_mash_type(__ENT_TYPE);

                rebase(a4->field_0, sizeof(generic_mash_header));
                
                auto *__ENT_ptr = (entity_base *) a4->field_0;
                a4->field_0 += entity_mash_size(__ENT_TYPE);

                {
                    auto v26 = (eEntityMashTypeEnum) ent_type;
                    fix_entity_v_table((char *) __ENT_ptr, v26);
                }

                assert(__ENT_ptr->rel_po_idx != 0xFF);
                assert(__ENT_ptr->rel_po_idx < all_rel_po.size());

                __ENT_ptr->my_rel_po = &this->all_rel_po.m_data[__ENT_ptr->rel_po_idx];
                __ENT_ptr->my_conglom_root = this;

                __ENT_ptr->un_mash_start(header, __ENT_ptr, a4, nullptr);

                __ENT_ptr->field_8 |= 0x10u;
                __ENT_ptr->field_3E = 0;

                tmp_ptr = (actor *) __ENT_ptr;
            }

            {
                auto *tmp_handle = tmp_ptr;
                assert(tmp_handle->is_conglom_member());
            }

            this->skeleton_ifc->connect_bone_abs_po(tmp_ptr->get_bone_idx(), tmp_ptr);

            bone = tmp_ptr;
        }

        int v59 = *a4->get<int>();
        if ( v59 != 0 ) {
            int v61 = *a4->get<int>();

            rebase(a4->field_0, 16);

            rebase(a4->field_0, 4);

            global_transfer_variable_the_conglom() = this;

            auto *v65 = a4->get<uint8_t>(v61);

#ifdef TARGET_XBOX
            mash_info_struct v92 {mash::UNMASH_MODE, v65, v61, true};
#else
            mash_info_struct v92 {v65, v61};
#endif

            this->field_114 = (als_res_data *) v92.read_from_buffer(
#ifdef TARGET_XBOX
                mash::NORMAL_BUFFER,
#endif
                    sizeof(als_res_data), 8);
            this->field_114->unmash(&v92, nullptr);
            mash_info_struct::construct_class(this->field_114);
        }

        actor::_un_mash(a2, a3, a4);

        if ( this->has_skeleton_ifc() ) {
            this->ifl_lock(0);
        }

        nglMesh **v70 = ( this->field_90.field_5 <= 1u
                            ? this->field_90.field_0
                            : (nglMesh **)this->field_90.field_0[this->field_90.field_4]
                        );

        if ( v70 != nullptr )
        {
            if ( v70[4] != nullptr )
            {
                auto *v72 = mem_alloc(sizeof(light_manager));
                auto *v74 = new (v72) light_manager {0};

                auto *v75 = this->field_F8;
                if ( v75 != v74 ) {
                    if ( v75 != nullptr ) {
                        auto v17 = (v75->field_0-- == 1);
                        if ( v17 ) {
                            auto *v76 = this->field_F8;
                            if ( v76 != nullptr )
                            {
                                this->field_F8->remove_from_list();
                                mem_dealloc(v76, sizeof(*v76));
                            }
                        }
                    }

                    this->field_F8 = v74;

                    if ( v74 != nullptr ) {
                        ++v74->field_0;
                    }
                }

                this->field_F8->field_C = this->my_handle.field_0;
                this->set_flag_recursive(static_cast<entity_flag_t>(0x2000u), true);
            }
        }

        this->field_110 |= 3u;

        this->set_flag_recursive(static_cast<entity_flag_t>(2), this->possibly_collide());

        this->field_108 = -1.0;
        this->field_10C = -1.0;
        if ( this->field_7C != nullptr )
        {
            this->field_7C->post_entity_mash();
        }

        if ( this->has_tentacle_ifc() )
        {
            auto *v81 = this->tentacle_ifc();
            v81->initialize_polytubes();
        }

        if ( this->has_variant_ifc() )
        {
            auto *v82 = this->variant_ifc();
            v82->field_28 = v82->field_4->get_mesh()->File;
            auto *v83 = v82->get_random_variant();
            v82->apply_variant(v83);
        }
    }
    else
    {
        THISCALL(0x004FC830, this, a2, a3, a4);
    }
}

void conglomerate::release_mem() {
    THISCALL(0x004F9F10, this);
}

void conglomerate::_render(Float a2)
{
    TRACE("conglomerate::render");

    if (debug_render_get_ival(SKELETONS) <= 0)
    {
        THISCALL(0x004F9930, this, a2);
    }
    else
    {
        this->debug_render();
    }
}

void conglomerate::debug_render()
{
    TRACE("conglomerate::debug_render");
    
    if (g_camera_link() == nullptr) {
        return;
    }

    auto &v1 = this->get_abs_po();
    auto v139 = this->get_abs_position() + v1.get_z_facing() + YVEC;
    [[maybe_unused]] auto v5 = this->get_abs_position() + YVEC;
    render_beam(v5, v139, color32 {33, 134, 216, 128}, 0.1, false);

    auto &v6 = this->get_abs_po();
    auto v9 = this->get_abs_position() + v6.get_z_facing() + YVEC;
    render_debug_hemisphere(v9, 0.12, color32 {242, 124, 6, 128});

    auto v12 = g_camera_link()->get_abs_position() - this->get_abs_position();
    auto v189 = v12.length2();
    auto v176 = this->get_visual_radius();
    auto v245 = (sqr(v176) > v189 && debug_render_get_ival(SKELETONS) == 1);
    if ( v245 )
    {
        auto v13 = this->get_id();
        auto *v177 = v13.to_string();
        color32 v141 {255, 0, 0, 255};
        auto &v14 = this->get_abs_position();
        print_3d_text(v14, v141, 0.5, "%s", v177);
    }

    for ( auto i = 0; i < this->members.size(); ++i )
    {
        auto &member = this->members.at(i);
        if ( this->field_E8.at(i) < 0
            || this->field_E8.at(i) >= this->skin_bones.size() + this->members.size() )
        {
            color32 v165 {33, 134, 216, 128};
            auto &v143 = this->get_abs_position();
            auto &v29 = member->get_abs_position();
            render_beam(v29, v143, v165, 0.02, 0);
        }
        else
        {
            entity_base *v21;
            auto v18 = this->field_E8.at(i);
            if ( v18 >= this->members.size() )
            {
                auto v22 = this->field_E8.at(i);
                auto v23 = this->members.size();
                v21 = this->skin_bones.at(v22 - v23);
            }
            else
            {
                auto v19 = this->field_E8.at(i);
                v21 = this->members.at(v19);
            }

            auto &a2 = v21->get_abs_position();
            color32 v164 {33, 134, 216, 128};
            auto &v26 = member->get_abs_position();
            render_beam(v26, a2, v164, 0.02, 0);
        }

        color32 v178 {242, 124, 6, 128};
        auto &v32 = member->get_abs_position();
        render_debug_hemisphere(v32, 0.02, v178);

        if ( v245 )
        {
            auto v35 = member->get_id();
            auto *v179 = v35.to_string();
            color32 v144 {255, 0, 0, 255};
            auto &v38 = member->get_abs_position();
            print_3d_text(v38, v144, 0.5, "%s", v179);

            color32 v166 {255, 0, 0, 128};
            auto &v41 = member->get_rel_po();
            auto &v42 = v41.get_x_facing();
            auto v145 = v42 * 0.050000001;
            auto &v45 = member->get_abs_position();
            auto v146 = v45 + v145;
            render_beam(v45, v146, v166, 0.0099999998, 0);

            color32 v167 {0, 255, 0, 128};
            auto &v51 = member->get_rel_po();
            auto &v52 = v51.get_y_facing();
            auto v147 = v52 * 0.050000001;
            auto &v55 = member->get_abs_position();
            auto v148 = v55 + v147;
            render_beam(v55, v148, v167, 0.0099999998, 0);

            color32 v168 {0, 0, 255, 128};
            auto &v61 = member->get_rel_po();
            auto &v62 = v61.get_z_facing();
            auto v149 = v62 * 0.050000001;
            auto &v65 = member->get_abs_position();
            auto v150 = v65 + v149;

            render_beam(v65, v150, v168, 0.0099999998, 0);
        }
    }

    for ( auto i = 0; i < this->skin_bones.size(); ++i )
    {
        {
            color32 v170 {33, 134, 216, 128};
            if ( this->field_F0.at(i) < 0
                || (this->field_F0.at(i) >= this->skin_bones.size() + this->members.size()) )
            {
                auto &v152 = this->get_abs_position();
                auto &v82 = this->skin_bones.at(i);
                auto &v83 = v82->get_abs_position();
                render_beam(v83, v152, v170, 0.02, false);
            }
            else
            {
                entity_base *v75 = nullptr;
                auto v72 = this->field_F0.at(i);
                if ( v72 >= this->members.size() )
                {
                    auto v76 = this->field_F0.at(i);
                    auto v77 = this->members.size();
                    v75 = this->skin_bones.at(v76 - v77);
                }
                else
                {
                    auto v73 = this->field_F0.at(i);
                    v75 = this->members.at(v73);
                }

                auto v151 = v75->get_abs_position();
                auto &v79 = this->skin_bones.at(i);
                auto &v80 = v79->get_abs_position();
                render_beam(v80, v151, v170, 0.02, false);
            }
        }

        color32 v180 {242, 124, 6, 128};
        auto &v85 = this->skin_bones.at(i);
        auto &v86 = v85->get_abs_position();
        render_debug_hemisphere(v86, 0.02, v180);

        if ( v245 )
        {
            auto &v88 = this->skin_bones.at(i);
            auto v89 = v88->get_id();
            auto *v181 = v89.to_string();
            color32 v153 {255, 0, 0, 255};

            auto &v91 = this->skin_bones.at(i);
            auto &v92 = v91->get_abs_position();
            print_3d_text(v92, v153, 0.5, "%s", v181);

            color32 v171 {255, 0, 0, 128};
            auto &v94 = this->skin_bones.at(i);
            auto &v95 = v94->get_abs_po();
            auto v154 = v95.get_x_facing() * 0.050000001;

            auto &v98 = this->skin_bones.at(i);
            auto &v99 = v98->get_abs_position();
            auto v155 = v99 + v154;
            auto &v101 = this->skin_bones.at(i);
            auto &v102 = v101->get_abs_position();
            render_beam(v102, v155, v171, 0.0099999998, false);

            color32 v172 {0, 255, 0, 128};
            auto &v104 = this->skin_bones.at(i);
            auto &v105 = v104->get_abs_po();
            auto &v106 = v105.get_y_facing();
            auto v156 = v106 * 0.050000001;

            auto &v108 = this->skin_bones.at(i);
            auto &v109 = v108->get_abs_position();
            auto v157 = v109 + v156;

            auto &v111 = this->skin_bones.at(i);
            auto &v112 = v111->get_abs_position();
            render_beam(v112, v157, v172, 0.0099999998, false);

            color32 v173 {0, 0, 255, 128};
            auto &v114 = this->skin_bones.at(i);
            auto &v115 = v114->get_abs_po();
            auto &v116 = v115.get_z_facing();
            auto v158 = v116 * 0.050000001;

            auto &v118 = this->skin_bones.at(i);
            auto &v119 = v118->get_abs_position();
            auto v159 = v119 + v158;

            auto &v121 = this->skin_bones.at(i);
            auto &v122 = v121->get_abs_position();
            render_beam(v122, v159, v173, 0.0099999998, false);
        }
    }

    if ( debug_render_get_ival(SKELETONS) == 2 )
    {
        static string_hash stru_1564240 {"CAMERA_ROOT"};

        auto *v123 = this->get_member(stru_1564240, true);
        if ( v123 != nullptr )
        {
            auto *v244 = this->get_member(stru_1564240, true);
            color32 v182 {0, 255, 0, 255};
            auto &v125 = v244->get_abs_position();
            render_debug_hemisphere(v125, 0.1, v182);
            auto v126 = v244->get_id();
            auto *v183 = v126.to_string();
            
            color32 v160 {255, 0, 0, 255};
            auto &v127 = v244->get_abs_position();
            print_3d_text(v127, v160, 0.5, "%s", v183);
        }
    }

    if ( debug_render_get_ival(SKELETONS) == 3 )
    {
        static string_hash stru_1564218 {"BIP01 PROP"};

        auto *v128 = this->get_member(stru_1564218, true);

        if ( v128 != nullptr )
        {
            auto *v243 = this->get_member(stru_1564218, true);
            color32 v184 {0, 255, 0, 255};
            auto &v130 = v243->get_abs_position();
            render_debug_hemisphere(v130, 0.1, v184);

            auto v131 = v243->get_id();
            auto *v185 = v131.to_string();
            color32 v161 {255, 0, 0, 255};
            auto &v132 = v243->get_abs_position();
            print_3d_text(v132, v161, 0.5, "%s", v185);
        }

        static string_hash stru_156410C {"BIP01 PROP2"};

        auto *v133 = this->get_member(stru_156410C, true);
        if ( v133 != nullptr )
        {
            auto *v242 = this->get_member(stru_156410C, true);
            auto v186 = color32(0, 255, 0, 255);
            auto &v135 = v242->get_abs_position();
            render_debug_hemisphere(v135, 0.1, v186);

            auto v136 = v242->get_id();
            auto *v187 = v136.to_string();
            auto v162 = color32(255, 0, 0, 255);
            auto &v137 = v242->get_abs_position();
            print_3d_text(v137, v162, 0.5, "%s", v187);
        }
    }
}

float conglomerate::get_colgeom_radius() {
    if constexpr (1) {
        if ((this->field_110 & 2) != 0) {
            float v1 = ((this->colgeom != nullptr) ? this->colgeom->get_bounding_sphere_radius()
                                                   : 0.0f);

            auto *v4 = this->field_FC;
            this->field_10C = v1;
            if (v4 != nullptr && v4->size()) {
                for (auto &act : (*v4)) {
                    vector3d v8 = this->get_colgeom_center();

                    vector3d v9 = act->get_colgeom_center();

                    auto tmp = v9 - v8;

                    auto v10 = act->get_colgeom_radius() + tmp.length();

                    if (v10 > this->field_10C) {
                        this->field_10C = v10;
                    }
                }

                this->field_10C *= 1.1f;
            }

            this->field_110 &= 0xFFFFFFFD;
        }

        return this->field_10C;

    } else {
        float __fastcall (*func)(void *) = CAST(func, 0x004D2670);

        return func(this);
    }
}

vector3d conglomerate::get_colgeom_center() {
    if (this->colgeom != nullptr) {
        auto &v3 = this->get_abs_po();
        auto v4 = this->colgeom->get_local_space_bounding_sphere_center();

        auto v7 = v3.m * v4;
        return v7;
    }

    return this->get_abs_position();
}

void conglomerate::radius_changed(bool a2)
{
    if (a2)
    {
        this->field_110 |= 3u;
        actor::radius_changed(a2);
    }
}

bool render_drop_shadow(math::MatClass<4, 3> &a1, Float a2, Float a3, bool a4)
{
    TRACE("render_drop_shadow");

    return (bool) CDECL_CALL(0x0059F910, &a1, a2, a3, a4);
}

bool conglomerate::render_complex_shadow(Float camera_distance)
{
    assert(this->is_flagged(EFLAG_MISC_CAST_SHADOW) && this->is_flagged(EFLAG_MISC_HIRES_SHADOW));

    assert(camera_distance <= 50.0f);

    if constexpr (0)
    {}
    else
    {
        bool (__fastcall *func)(void *, void *edx, Float camera_distance) = CAST(func, 0x004E5300);
        return func(this, nullptr, camera_distance);
    }
}

void conglomerate::render_simple_shadow(Float arg0, Float arg4)
{
    assert(this->is_flagged(EFLAG_MISC_CAST_SHADOW)
            && this->is_flagged(EFLAG_GRAPHICS_VISIBLE));

    if constexpr (0)
    {
    }
    else
    {
        THISCALL(0x004E4D80, this, arg0, arg4);
    }
}

bool sub_4C19C0(const string_hash &a1, entity_base **a2, int size, int *index) {
    bool v14 = false;
    int v13 = 0;
    auto v12 = size;
    while (v13 < v12) {
        auto v11 = (v12 + v13) >> 1;
        auto *v4 = a2[v11];
        auto v5 = v4->get_id();
        if (a1 < v5) {
            v12 = (v12 + v13) >> 1;
        } else {
            auto *v6 = a2[v11];
            auto v7 = v6->get_id();
            if (!(a1 > v7)) {
                v14 = true;
                if (index) {
                    *index = v11;
                }

                break;
            }

            v13 = v11 + 1;
        }
    }

    if (!v14 && index != nullptr) {
        if (v12 == size - 1) {
            auto *v8 = a2[v12];
            auto v9 = v8->get_id();
            if (a1 > v9) {
                ++v12;
            }
        }

        *index = v12;
    }

    return v14;
}

entity_base *conglomerate::get_member(const string_hash &a2, bool a3) {
    auto members_size = this->members.size();
    auto i = -1;
    if (members_size != 0) {
        auto **data = this->members.m_data;
        if (sub_4C19C0(a2, data, members_size, &i)) {
            return data[i];
        }
    }

    if (a3) {
        auto skin_bones_size = this->skin_bones.size();
        auto i = -1;
        if (skin_bones_size != 0) {
            auto **data = this->skin_bones.m_data;
            if (sub_4C19C0(a2, data, skin_bones_size, &i)) {
                return data[i];
            }
        }
    }

    return nullptr;
}

bool conglomerate::has_tentacle_ifc()
{
    bool (__fastcall *func)(void *) = CAST(func, get_vfunc(m_vtbl, 0x294));
    return func(this);
}

tentacle_interface *conglomerate::tentacle_ifc()
{
    tentacle_interface * (__fastcall *func)(void *) = CAST(func, get_vfunc(m_vtbl, 0x298));
    return func(this);
}

bool conglomerate::has_variant_ifc()
{
    bool (__fastcall *func)(void *) = CAST(func, get_vfunc(m_vtbl, 0x29C));
    return func(this);
}

variant_interface *conglomerate::variant_ifc()
{
    variant_interface* (__fastcall *func)(void *) = CAST(func, get_vfunc(m_vtbl, 0x2A0));
    return func(this);
}

entity_base *conglomerate::get_bone(const string_hash &a2, bool a3) {
    if constexpr (1) {
        auto skin_bones_size = this->skin_bones.size();
        auto i = -1;
        if (skin_bones_size != 0) {
            auto **data = this->skin_bones.m_data;
            if (sub_4C19C0(a2, data, skin_bones_size, &i)) {
                return data[i];
            }
        }

        if (a3) {
            return this->get_member(a2, 0);
        }

        return nullptr;
    } else {
        return (entity_base *) THISCALL(0x004CCC90, this, &a2, a3);
    }
}

light_manager *conglomerate::get_light_set() const
{
    return this->field_F8;
}

void conglomerate_patch()
{

    REDIRECT(0x004E52D4, render_drop_shadow);

    {
        FUNC_ADDRESS(address, &conglomerate::_un_mash);
        set_vfunc(0x00884834, address);
    }

    {
        FUNC_ADDRESS(address, &conglomerate::_render);
        set_vfunc(0x0088487C, address);
    }

    {
        FUNC_ADDRESS(address, &mashable_vector<entity_base *>::custom_un_mash);
        REDIRECT(0x004FCAC8, address);
        REDIRECT(0x004FCE5A, address);
    }

    {
        FUNC_ADDRESS(address, &mashable_vector<int8_t>::custom_un_mash);
        REDIRECT(0x004FCAA6, address);
        REDIRECT(0x004FCAB6, address);
    }

    {
        FUNC_ADDRESS(address, &mashable_vector<po>::custom_un_mash);
        REDIRECT(0x004FC8D4, address);
        REDIRECT(0x004FC8E3, address);
        REDIRECT(0x004FC8F2, address);
    }

    return;

    FUNC_ADDRESS(address, &conglomerate::get_colgeom_radius);
    set_vfunc(0x00884924, address);

    {
        FUNC_ADDRESS(address, &conglomerate::get_colgeom_center);
        //set_vfunc(0x00884928, address);
    }
}

void conglomerate_xbpack_patch()
{
#ifdef OPENUSM_XBPACK_MODE
    FUNC_ADDRESS(address, &conglomerate::_un_mash);
    set_vfunc(0x00884834, address);
#endif
}
