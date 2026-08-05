#include "entity_base.h"

#include "common.h"

#include "actor.h"
#include "box_trigger.h"
#include "conglom.h"
#include "convex_box.h"
#include "distance_fader.h"
#include "entity.h"
#include "entity_handle_manager.h"
#include "entity_mash.h"
#include "event.h"
#include "event_manager.h"
#include "event_recipient_entry.h"
#include "event_type.h"
#include "func_wrapper.h"
#include "limbo_entities.h"
#include "memory.h"
#include "motion_effect_struct.h"
#include "oldmath_po.h"
#include "os_developer_options.h"
#include "parse_generic_mash.h"
#include "scratchpad_stack.h"
#include "slab_allocator.h"
#include "sound_and_pfx_interface.h"
#include "stack_allocator.h"
#include "tl_system.h"
#include "trigger_manager.h"
#include "trace.h"
#include "utility.h"
#include "vtbl.h"
#include "wds.h"
#include "wds_render_manager.h"

#include <cassert>
#include <cmath>

VALIDATE_SIZE(entity_base, 0x44u);
VALIDATE_OFFSET(entity_base, my_abs_po, 0x14);

int DEBUG_foster_conglom_warning{};

entity_base::entity_base(bool a2)
{
    if constexpr (1) {
        this->field_10 = 0;
        this->my_sound_and_pfx_interface = nullptr;

        this->field_10 = ANONYMOUS;
        this->field_4 = 0;
        this->field_40 = -1;
        this->field_3C = -1;
        this->rel_po_idx = -1;
        this->my_conglom_root = nullptr;
        this->my_handle = 0;
        this->field_8 = (a2 ? 0x20000 : 0) | 0x80000000;
        this->my_rel_po = nullptr;
        if ((this->field_8 & 0x20000) == 0) {
            auto *mem = mem_alloc(sizeof(po));
            this->my_rel_po = new (mem) po{};
        }

        this->my_handle = entity_handle_manager::add_entity(this);
        this->field_41 = 0;
        if ((this->field_8 & 0x20000) == 0) {
            entity_handle_manager::register_entity(this);
        }

        this->field_8 |= 0x140u;
        this->m_parent = nullptr;
        this->m_child = nullptr;
        this->field_28 = nullptr;
        this->adopted_children = nullptr;
        this->field_18 = nullptr;
        this->my_sound_and_pfx_interface = nullptr;
        this->proximity_map_reference_count = 0;
        this->proximity_map_cell_reference_count = 0;
        this->field_3E = 0;
        this->m_timer = -1;
        this->my_abs_po = this->my_rel_po;
    } else {
        THISCALL(0x004F32C0, this, a2);
    }
}

entity_base::entity_base(const string_hash &a2, uint32_t a3, bool a4)
{
    auto *v5 = &this->field_10.source_hash_code;
    this->my_sound_and_pfx_interface = nullptr;
    *v5 = a2.source_hash_code;
    this->field_4 = {a3};
    this->field_40 = -1;
    this->field_3C = -1;
    this->rel_po_idx = -1;
    this->my_conglom_root = nullptr;
    this->field_8 = (a4 != 0 ? 0x20000 : 0) | 0x80000000;
    this->my_handle = 0;
    this->my_rel_po = nullptr;
    if ((this->field_8 & 0x20000) == 0) {
        auto *mem = mem_alloc(sizeof(po));
        this->my_rel_po = new (mem) po{};
    }

    this->my_handle = entity_handle_manager::add_entity(this);
    this->field_41 = 0;

    if ((this->field_8 & 0x20000) == 0) {
        entity_handle_manager::register_entity(this);
    }

    this->field_8 |= 0x140u;
    this->m_parent = nullptr;
    this->m_child = nullptr;
    this->field_28 = nullptr;
    this->adopted_children = nullptr;
    this->field_18 = nullptr;
    this->my_sound_and_pfx_interface = nullptr;
    this->my_abs_po = this->my_rel_po;
    this->proximity_map_reference_count = 0;
    this->proximity_map_cell_reference_count = 0;
    this->field_3E = 0;

    this->set_timer(0xFF);
}

entity_base * __fastcall entity_base_constructor(void *self, int, const string_hash *a2, unsigned int a3, bool a4)
{
    TRACE("entity_base::entity_base");

    return new (self) entity_base {*a2, a3, a4};
}

bool entity_base::has_region_idx() const
{
    return this->field_3C != 0xFFFF;
}

uint16_t entity_base::get_bone_idx() const
{
    uint16_t result = -1;
    if ( this->field_40 != 255 )
    {
        result = this->field_40;
    }

    return result;
}

float entity_base::sub_57CB80() {
    auto v3 = this->get_abs_po().m[2][1];
    if (v3 < -1.f) {
        return std::asin(-1.0f);
    }

    if (v3 > 1.f) {
        v3 = 1.0;
    }

    return std::asin(v3);
}

entity_base::~entity_base()
{
    if constexpr (0)
    {
        assert(is_dynamic());
        this->common_destruct();
        if ( !this->is_conglom_member() && this->my_rel_po != nullptr )
        {
            mem_dealloc(this->my_rel_po, sizeof(po));
            this->my_rel_po = nullptr;
        }
    }
    else
    {
        THISCALL(0x004F8FA0, this);
    }
}

int entity_base::get_entity_size() {
    return sizeof(entity_base);
}

bool entity_base::has_sound_and_pfx_ifc() {
    return this->my_sound_and_pfx_interface != nullptr;
}

sound_and_pfx_interface *entity_base::sound_and_pfx_ifc() {
    return this->my_sound_and_pfx_interface;
}

void entity_base::release_mem()
{
    if constexpr (1)
    {
        assert(!is_dynamic());

        auto v3 = this->is_ext_flagged(0x400);;
        auto v2 = this->is_mashed_member();
        this->common_destruct();
        if (!v2)
        {
            if (v3) {
                mem_freealign(this);
            } else {
                release_generic_mash(this);
            }
        }

    } else {
        THISCALL(0x004F9020, this);
    }
}

void entity_base::load(chunk_file &) {
    ;
}

bool entity_base::handle_load_chunk(chunk_file &, mString &) {
    return false;
}

void entity_base::read_enx(chunk_file &) {
    ;
}

bool entity_base::handle_enx_chunk(chunk_file &, mString &) {
    return false;
}

vector3d entity_base::get_last_position() {
    return this->get_abs_position();
}

float entity_base::get_visual_radius() {
    return 0.0;
}

vector3d entity_base::get_visual_center() {
    return ZEROVEC;
}

vector3d entity_base::get_velocity() {
    return ZEROVEC;
}

void entity_base::po_changed() {
    this->field_8 &= 0xF7FFFFFF;
}

void entity_base::set_flag_recursive(entity_flag_t a2, bool a3)
{
    if (a3) {
        this->field_4 |= a2;
    } else {
        this->field_4 &= ~a2;
    }
}

static constexpr auto EFLAG_EXT_SYSTEM_ONLY_FLAGS = 0x80020410;

void entity_base::set_ext_flag_recursive_internal(entity_ext_flag_t f, bool a3)
{
    assert((f & EFLAG_EXT_SYSTEM_ONLY_FLAGS) == 0);

    if ( a3 ) {
        this->field_8 |= f;
    } else {
        this->field_8 &= ~f;
    }
}

void entity_base::set_ext_flag_recursive(entity_ext_flag_t a2, bool a3) {
    this->set_ext_flag_recursive_internal(a2, a3);
}

void entity_base::set_active(bool a2)
{
    if (a2) {
        this->field_4 |= 0x2000;
    } else {
        this->field_4 &= ~0x2000;
    }
}

void entity_base::set_visible(bool a2, bool)
{
    if (a2) {
        this->field_4 |= 0x200;
    } else {
        this->field_4 &= ~0x200;
    }
}

ai::ai_core *entity_base::get_ai_core() {
    if constexpr (1)
    {
        ai::ai_core * (__fastcall *func)(void *) = CAST(func, get_vfunc(m_vtbl, 0x48));
        return func(this);
    }
    else
    {
        return nullptr;
    }
}

bool entity_base::is_hero() {
    return false;
}

bool entity_base::is_alive() {
    return true;
}

int entity_base::get_flavor() {
    if constexpr (1)
    {
        int (__fastcall *func)(entity_base *) = CAST(func, get_vfunc(m_vtbl, 0x54));
        return func(this);

    } else {
        return 3;
    }
}

bool entity_base::is_an_entity_base() {
    return true;
}

bool entity_base::is_a_signaller() {
    return false;
}

bool entity_base::is_an_entity() {
    if constexpr (1) {
        bool (__fastcall *func)(entity_base *) = CAST(func, get_vfunc(m_vtbl, 0x60));
        return func(this);
    } else {
        return false;
    }
}

bool entity_base::is_an_actor() const {
    if constexpr (1)
    {
        bool (__fastcall *func)(const entity_base *) = CAST(func, get_vfunc(m_vtbl, 0x64));
        return func(this);

    } else {
        return false;
    }
}

bool entity_base::is_a_conglomerate_clone() {
    return false;
}

bool entity_base::is_a_camera() {
    return false;
}

bool entity_base::is_a_station_camera() {
    return false;
}

bool entity_base::is_a_game_camera() {
    if constexpr (1) {
        bool (__fastcall *func)(entity_base *) = CAST(func, get_vfunc(m_vtbl, 0x74));

        return func(this);

    } else {
        return false;
    }
}

bool entity_base::is_a_glam_camera() {
    return false;
}

bool entity_base::is_a_marky_camera() {
    return false;
}

bool entity_base::is_a_mouselook_camera() {
    return false;
}

bool entity_base::is_a_sniper_camera() {
    return false;
}

bool entity_base::is_an_ai_camera() {
    return false;
}

bool entity_base::is_a_spiderman_camera() {
    return false;
}

bool entity_base::is_a_light_source()
{
    if constexpr (1)
    {
        bool (__fastcall *func)(void *) = CAST(func, get_vfunc(m_vtbl, 0x90));
        return func(this);
    }
    else
    {
        return false;
    }
}

bool entity_base::is_a_neolight() {
    return false;
}

bool entity_base::is_a_marker() {
    return false;
}

bool entity_base::is_a_parking_marker() {
    return false;
}

bool entity_base::is_a_water_exit_marker() {
    return false;
}

bool entity_base::is_a_rectangle_marker() {
    return false;
}

bool entity_base::is_a_cube_marker() {
    return false;
}

bool entity_base::is_a_crawl_marker() {
    return false;
}

bool entity_base::is_an_anchor_marker() {
    return false;
}

bool entity_base::is_a_line_marker_base() {
    return false;
}

bool entity_base::is_a_line_anchor() {
    return false;
}

bool entity_base::is_a_pole() {
    return false;
}

bool entity_base::is_an_ai_cover_marker() {
    return false;
}

bool entity_base::is_a_lensflare() {
    return false;
}

bool entity_base::is_a_manip_obj() {
    return false;
}

bool entity_base::is_a_switch_obj() {
    return false;
}

bool entity_base::is_a_mic() {
    return false;
}

bool entity_base::is_a_pfx_entity() const {
    if constexpr(1)
    {
        bool (__fastcall *func)(const entity_base *) = CAST(func, get_vfunc(m_vtbl, 0xD4));
        return func(this);
    }
    else
    {
        return false;
    }
}

bool entity_base::is_an_item() {
    return false;
}

bool entity_base::is_a_handheld_item() {
    return false;
}

bool entity_base::is_a_gun() {
    return false;
}

bool entity_base::is_a_thrown_item() {
    return false;
}

bool entity_base::is_a_melee_item() {
    return false;
}

bool entity_base::is_a_visual_item() {
    return false;
}

bool entity_base::is_a_grenade() {
    return false;
}

bool entity_base::is_a_rocket() {
    return false;
}

bool entity_base::is_a_sky() {
    return false;
}

bool entity_base::is_a_beam() {
    return false;
}

bool entity_base::is_a_polytube() {
    return false;
}

bool entity_base::is_a_trigger() {
    return false;
}

bool entity_base::is_material_switching() {
    if constexpr (1)
    {
        bool (__fastcall *func)(entity_base *) = CAST(func, get_vfunc(m_vtbl, 0x108));
        return func(this);
    } else {
        return false;
    }
}

bool entity_base::has_time_ifc() {
    return false;
}

time_interface *entity_base::time_ifc() {
    assert(0 && "Accessing an invalid interface");
    return nullptr;
}

bool entity_base::has_damage_ifc() {
    return false;
}

damage_interface *entity_base::damage_ifc() {
    assert(0 && "Accessing an invalid interface");
    return nullptr;
}

bool entity_base::has_facial_expression_ifc() {
    return false;
}

facial_expression_interface *entity_base::facial_expression_ifc() {
    assert(0 && "Accessing an invalid interface");
    return nullptr;
}

bool entity_base::has_physical_ifc() {
    if constexpr (1) {
        bool (__fastcall *func)(entity_base *) = CAST(func, get_vfunc(m_vtbl, 0x124));
        return func(this);

    } else {
        return false;
    }
}

physical_interface *entity_base::physical_ifc() {
    //assert(0 && "Accessing an invalid interface");
    //return nullptr;

    physical_interface * (__fastcall *func)(entity_base *) = CAST(func, get_vfunc(m_vtbl, 0x128));
    return func(this);
}

bool entity_base::has_skeleton_ifc() {
    return false;
}

skeleton_interface *entity_base::skeleton_ifc() {
    assert(0 && "Accessing an invalid interface");
    return nullptr;
}

bool entity_base::has_animation_ifc() {
    return false;
}

animation_interface *entity_base::animation_ifc() {
    assert(0 && "Accessing an invalid interface");
    return nullptr;
}

bool entity_base::has_script_data_ifc() {
    return false;
}

script_data_interface *entity_base::script_data_ifc() {
    assert(0 && "Accessing an invalid interface");
    return nullptr;
}

bool entity_base::has_decal_data_ifc() {
    return false;
}

decal_data_interface *entity_base::decal_data_ifc() {
    assert(0 && "Accessing an invalid interface");
    return nullptr;
}

bool entity_base::get_ifc_num(const resource_key &, float &, bool) {
    return false;
}

bool entity_base::set_ifc_num(const resource_key &, float, bool) {
    return false;
}

bool entity_base::get_ifc_vec(const resource_key &, vector3d &, bool) {
    return false;
}

bool entity_base::set_ifc_vec(const resource_key &, const vector3d &, bool) {
    return false;
}

bool entity_base::get_ifc_str(const resource_key &, mString &, bool) {
    return false;
}

bool entity_base::set_ifc_str(const resource_key &, const mString &, bool) {
    return false;
}

void entity_base::_un_mash(generic_mash_header *a1, void *a2, generic_mash_data_ptrs *a3)
{
    TRACE("entity_base::un_mash");

    if constexpr (1)
    {
        std::memcpy(&this->field_4, a3->field_4, 4);
        a3->field_4 += 4;

        std::memcpy(&this->field_8, a3->field_4, 4);
        a3->field_4 += 4;
        if ( !this->is_conglom_member() )
        {
            auto v6 = 16 - ((int)a3->field_0 % 16);
            if ( v6 < 16 )
            {
                a3->field_0 += v6;
            }

            auto v7 = 4 - ((int)a3->field_0 % 4);
            if ( v7 < 4 )
            {
                a3->field_0 += v7;
            }

            this->my_rel_po = (po *) a3->field_0;
            a3->field_0 += sizeof(po);

            {
                auto *header = a1;
                assert(((int)header) % 4 == 0);
            }

            this->my_rel_po->un_mash(a1, this->my_rel_po, a3);
        }

        this->my_abs_po = this->my_rel_po;
        this->m_parent = nullptr;
        this->m_child = nullptr;
        this->field_28 = nullptr;
        this->adopted_children = nullptr;
        this->field_18 = nullptr;
        this->my_sound_and_pfx_interface = nullptr;
        this->proximity_map_reference_count = 0;
        this->proximity_map_cell_reference_count = 0;
        this->field_3E = 0;
        this->field_8 |= 0x140;

        assert(!is_ext_flagged(EFLAG_EXT_SIGNALLER_ONLY));

        if ( (a1->field_E & 0x880) != 0 )
        {
            auto v9 = 8 - ((int)a3->field_0 % 8);
            if ( v9 < 8 )
            {
                a3->field_0 += v9;
            }

#ifndef TARGET_XBOX
            if ( (a1->field_E & 0x880) != 0 )
            {
                auto v10 = 4 - ((int)a3->field_0 & 3);
                if ( v10 < 4 )
                {
                    a3->field_0 += v10;
                }

                this->my_sound_and_pfx_interface = (sound_and_pfx_interface *)a3->field_0;
                a3->field_0 += sizeof(sound_and_pfx_interface);
                this->my_sound_and_pfx_interface->m_vtbl = ifc_v_table_lookup()[12];
                this->my_sound_and_pfx_interface->un_mash(
                    a1,
                    this,
                    this->my_sound_and_pfx_interface,
                    a3);
            }
            else
            {
                this->my_sound_and_pfx_interface = nullptr;
            }
#endif
        }

        if ( this->is_flagged(0x400) )
        {
            if ( auto v11 = 4 - ((int) a3->field_4 % 4u);
                    v11 < 4 ) {
                a3->field_4 += v11;
            }

            auto *v12 = bit_cast<convex_box *>(a3->field_4);
            a3->field_4 += sizeof(convex_box);
            auto *v13 = (box_trigger *) trigger_manager::instance->new_box_trigger(this->field_10, this);
            v13->set_box_info(*v12);
        }
    }
    else
    {
        THISCALL(0x004CB2F0, this, a1, a2, a3);
    }
}

void entity_base::un_mash(generic_mash_header *a1, void *a2, generic_mash_data_ptrs *a3) {
    sp_log("0x%08X", m_vtbl);

    void (__fastcall *func)(void *, int, generic_mash_header *, void *, generic_mash_data_ptrs*) = CAST(func, get_vfunc(m_vtbl, 0x164));
    func(this, 0, a1, a2, a3);
}

entity_base_vhandle entity_base::get_my_vhandle() {
    return my_handle;
}
 
void entity_base::raise_event(string_hash a2)
{
    entity_base_vhandle v2 = this->get_my_vhandle();
    event_manager::raise_event(a2, v2);
}

void entity_base::set_parent(entity_base *parent) {
    if constexpr (1) {
        if (parent != nullptr) {
            auto *v3 = this->m_parent;
            if (parent != v3) {
                if (v3) {
                    this->clear_parent(true);
                } else {
                    auto *v4 = this->m_child;
                    for (this->field_8 |= 0x10000040u; v4; v4 = v4->field_28) {
                        if ((v4->field_8 & 0x10000000) == 0) {
                            v4->dirty_family(0);
                        }
                    }
                }

                if ((this->field_8 & 0x40000000) == 0 && this->my_abs_po == this->my_rel_po) {
                    po *v7 = static_cast<po *>(mem_alloc(sizeof(po)));
                    if (v7 != nullptr) {
                        auto &v6 = parent->get_abs_po();
                        *v7 = this->my_rel_po->sub_4BAB00(v6);
                    }

                    this->my_abs_po = v7;
                }

                auto *my_child = parent->m_child;
                if (my_child != nullptr) {
                    for (auto *i = my_child->field_28; i != nullptr; i = i->field_28) {
                        my_child = i;
                    }

                    my_child->field_28 = this;
                } else {
                    parent->m_child = this;
                }

                this->m_parent = parent;
                this->field_28 = nullptr;
                if (!this->is_conglom_member()) {
                    entity_base *conglom_root = nullptr;
                    if (parent->is_conglom_member()) {
                        conglom_root = parent->get_conglom_owner();
                    } else {
                        conglom_root = parent;
                    }

                    assert(conglom_root != nullptr);

                    conglom_root->add_adopted_child(this);
                }
            }
        } else {
            this->clear_parent(true);
        }

    } else {
        THISCALL(0x004E0E50, this, parent);
    }
}

bool entity_base::is_a_parent(entity_base *a2) {
    auto *v2 = this->m_parent;
    return v2 != nullptr && (v2 == a2 || v2->is_a_parent(a2));
}

po &entity_base::get_rel_po() {
    if (this->is_rel_po_dirty()) {
        this->compute_rel_po_from_model();
    }

    assert(my_rel_po->is_valid());

    return *this->my_rel_po;
}

void entity_base::set_abs_position(const vector3d &pos) {
    if constexpr (1) {
        auto &rel_po = this->get_rel_po();

        rel_po.set_position(pos);
        this->dirty_family(false);

        if (this->is_conglom_member() || this->is_a_conglomerate()) {
            this->dirty_model_po_family();
        }

        this->po_changed();
    } else {
        THISCALL(0x0048C600, this, &pos);
    }
}

void entity_base::look_at(const vector3d &a1) {
    THISCALL(0x004E09F0, this, &a1);
}

void entity_base::sub_4D3F60(entity_base *a2) {
    THISCALL(0x004D3F60, this, a2);
}

void entity_base::sub_4E0DD0() {
    THISCALL(0x004E0DD0, this);
}

void entity_base::clear_parent(bool a1)
{
    if constexpr (0)
    {
        if (this->m_parent != nullptr)
        {
            this->dirty_family(false);

            assert(m_parent->get_first_child() != nullptr);

            if ( a1 && !this->is_conglom_member() )
            {
                auto *v3 = this->m_parent;
                if ( v3->is_conglom_member() ) {
                    v3 = v3->get_conglom_owner();
                }

                if (v3) {
                    v3->sub_4D3F60(this);
                }
            }

            auto *v4 = this->m_parent;
            auto *v5 = v4->m_child;
            if (v5 == this) {
                v4->m_child = this->field_28;
                this->m_parent = nullptr;
                this->field_28 = nullptr;
            } else {
                auto *v6 = v5->field_28;
                if (v6 != nullptr) {
                    while (v6 != this) {
                        v5 = v6;
                        v6 = v6->field_28;
                        if (v6 == nullptr) {
                            return;
                        }
                    }

                    v5->field_28 = this->field_28;
                    this->m_parent = nullptr;
                    this->field_28 = nullptr;
                }
            }
        }
    } else {
        THISCALL(0x004D3FB0, this, a1);
    }
}

entity_base * entity_base::get_conglom_owner() const
{
    //TRACE("entity_base::get_conglom_owner");

    if (!this->is_conglom_member() && !this->is_a_conglomerate()) {
        return nullptr;
    }

    if (this->my_conglom_root != nullptr) {
        return this->my_conglom_root;
    } else if (this->is_a_conglomerate()) {
        return bit_cast<entity_base *>(this);
    } else if (this->is_conglom_member() && this->m_parent) {
        return this->m_parent->get_conglom_owner();
    } else {
        return nullptr;
    }

    return nullptr;

}

void entity_base::un_mash_start(generic_mash_header *a2,
                                void *a3,
                                generic_mash_data_ptrs *a4,
                                [[maybe_unused]] void *a5)
{
    TRACE("entity_base::un_mash_start");

    if (a5 != nullptr)
    {
        this->field_10.source_hash_code = *static_cast<uint32_t *>(a5);
    }

    assert(my_handle == INVALID_HANDLE && "re-un_mashing an already un_mashed entity!");

    this->my_handle = entity_handle_manager::add_entity(this);

    this->un_mash(a2, a3, a4);
    if (!this->is_conglom_member()) {
        entity_handle_manager::register_entity(this);
    }

    this->field_8 |= 0x10000000u;

    assert(!is_dynamic());
}

void entity_base::add_adopted_child(entity_base *child_arg) {
    if constexpr (1) {
        assert(!child_arg->is_conglom_member());

        assert(DEBUG_foster_conglom_warning == 0 &&
               "We can't add adopted children when we are deleting the conglom they are being "
               "adopted by.");

        if (this->adopted_children == nullptr) {
            auto *mem = mem_alloc(sizeof(adopted_children));

            this->adopted_children = new (mem) _std::vector<entity_base *>{};
        }

        auto end = this->adopted_children->end();

        auto it = std::find(this->adopted_children->begin(), end, child_arg);

        if (it == end)
		{
            void (__fastcall *push_back)(void *, void *, void *) = CAST(push_back, 0x005E7330);

            push_back(this->adopted_children, nullptr, &child_arg);
        }

    } else {
        THISCALL(0x004DB740, this, child_arg);
    }
}

_std::vector<entity_base *> *entity_base::get_adopted_children()
{
    assert(adopted_children != nullptr);
    return this->adopted_children;
}

bool entity_base::empty_adopted_children() const
{
    return this->adopted_children != nullptr && !this->adopted_children->empty();
}

void entity_base::update_abs_po(bool a2) {
    if constexpr (1) {
        constexpr auto n_bytes = 512;
        entity_base **ents = nullptr;

        if constexpr (0) {
            ents = CAST(ents, scratchpad_stack::stk().current);
            if (scratchpad_stack::stk().current == scratchpad_stack::stk().segment) {
                scratchpad_stack::lock();
            }

            scratchpad_stack::stk().current += (scratchpad_stack::stk().alignment + n_bytes - 1) &
                ~(scratchpad_stack::stk().alignment - 1);
        } else {
            static entity_base *entities[n_bytes / 4]{};
            ents = entities;
        }

        auto **const ents_end = ents + (n_bytes / 4);
        auto **ents_cur = ents + (n_bytes / 4);

        entity_base *This = nullptr;
        for (This = this; This->my_abs_po != This->my_rel_po; This = This->m_parent) {
            auto *parent = This->m_parent;
            if (parent == nullptr || (parent->field_8 & 0x10000000) == 0) {
                break;
            }

            if (a2) {
                *--ents_cur = This;
            }
        }

        *--ents_cur = This;
        while (ents_cur != ents_end) {
            This = *ents_cur++;

            if (This->my_abs_po != This->my_rel_po) {
                if (This->m_parent != nullptr) {
                    po *rel_po = nullptr;
                    po *abs_po = nullptr;

                    if (!This->is_conglom_member() || (!This->is_rel_po_dirty()) ||
                        (This->field_8 & 0x100) != 0 ||
                        (This->rel_po_idx >= This->my_conglom_root->all_model_po.m_size)) {
                        rel_po = &This->get_rel_po();
                        abs_po = This->m_parent->my_abs_po;
                    } else {
                        assert(This->is_conglom_member());

                        assert((conglomerate *) This != This->my_conglom_root);

                        assert(This->rel_po_idx - 1 < This->my_conglom_root->all_model_po.size());

                        rel_po = &This->my_conglom_root->all_model_po.at(This->rel_po_idx - 1);
                        abs_po = This->my_conglom_root->my_abs_po;
                    }

                    void * a2a[2] {
                        rel_po,
                        abs_po
                    };

                    This->my_abs_po->m.sub_415A30(a2a);
                } else {
                    *This->my_abs_po = This->get_rel_po();
                }
            }

            assert(This->my_abs_po->is_valid());

            This->field_8 &= 0xEFFFFFFF;
            if (!a2) {
                for (auto *ent = This->m_child; ent != nullptr; ent = ent->field_28) {
                    *--ents_cur = ent;

                    assert(ents_cur > ents);
                }
            }
        }

        if constexpr (0) {
            scratchpad_stack::stk().current -= (scratchpad_stack::stk().alignment + n_bytes - 1) &
                ~(scratchpad_stack::stk().alignment - 1);
            if (scratchpad_stack::stk().current == scratchpad_stack::stk().segment) {
                scratchpad_stack::unlock();
            }
        }

    } else {
        THISCALL(0x004DB590, this, a2);
    }
}

void entity_set_abs_parent(entity_base *me, entity_base *parent) {
    if constexpr (1) {
        assert(me != nullptr);

        if (parent != nullptr) {
            assert(!parent->is_a_parent(me->get_my_handle().get_volatile_ptr()) &&
                   "An entity circular parenting was requested!");

            auto *v2 = me->m_parent;
            if (v2 == nullptr || v2 != parent) {
                auto my_po = me->get_abs_po();
                me->set_parent(parent);

                auto v3 = parent->get_abs_po();

                void * v4[2] {
                    &my_po,
                    v3.inverse()
                };
                my_po.m.sub_415A30(v4);
                me->set_abs_po(my_po);
                assert(my_po.is_valid());
            }

        } else if (me->m_parent != nullptr) {
            auto &my_po = me->get_abs_po();

            me->clear_parent(true);
            me->set_abs_po(my_po);
            assert(my_po.is_valid());
        }

    } else {
        CDECL_CALL(0x004E1290, me, parent);
    }
}

void entity_set_abs_po(entity_base *ent, const po &the_po)
{
    if constexpr (1)
    {
        assert(ent != nullptr && "No Entity passed in");

        assert(Abs(the_po.get_x_facing()) > EPSILON);

        assert(Abs(the_po.get_y_facing()) > EPSILON);

        assert(Abs(the_po.get_z_facing()) > EPSILON);

        assert(the_po.is_valid());

        assert(the_po.get_position().length2() < MAX_ALLOWED_POSITION_LENGTH_SQUARED);

        auto ENABLE_LONG_MALOR_ASSERTS = os_developer_options::instance->get_int(
            mString{"ENABLE_LONG_MALOR_ASSERTS"});

        if (ENABLE_LONG_MALOR_ASSERTS > 0 && !ent->is_flagged(0x800) && ent->is_an_actor() &&
            !bit_cast<actor *>(ent)->get_allow_tunnelling_into_next_frame()) {
            auto v24 = (float) (ENABLE_LONG_MALOR_ASSERTS * ENABLE_LONG_MALOR_ASSERTS);
            auto a2a = ent->get_abs_position();

            auto v22 = the_po.get_position();

            auto v8 = a2a - v22;
            auto v9 = v8.length2();
            if (v9 > v24) {
                auto v10 = ent->get_id();
                auto *a3 = v10.to_string();
                if (strstr(a3, "PED_") == nullptr && strstr(a3, "ENTID_") == nullptr) {
                    auto v11 = a2a - v22;
                    auto v12 = v11.length();

                    char a1[256];
                    sprintf(a1, "Entity %s moved %f meters", a3, v12);

                    sp_log(a1);
                    assert(0);
                }
            }
        }

        if (ent->m_parent != nullptr) {
            static po new_po{};

            auto *parent = ent->get_parent();

            po &parent_abs_po = parent->get_abs_po();

            const void * v4[2] {
                &the_po,
                parent_abs_po.inverse()
            };

            new_po.m.sub_415A30(v4);
            ent->set_abs_po(new_po);

            assert(new_po.is_valid());

        } else {
            ent->set_abs_po(the_po);
        }
    } else {
        CDECL_CALL(0x004E1100, ent, &the_po);
    }
}

void entity_base::common_destruct()
{
    if constexpr (1) {
        this->sub_4E0DD0();
        while (this->m_child != nullptr) {
            auto *my_child = this->m_child;

            if (!my_child->is_conglom_member()) {
                if (my_child->m_parent != nullptr) {
                    auto abs_pos = my_child->get_abs_po();
                    my_child->clear_parent(true);
                    my_child->set_abs_po(abs_pos);
                }

            } else {
                my_child->clear_parent(true);
            }
        }

        this->clear_parent(true);
        auto *v3 = this->field_18;
        if (v3 != nullptr) {
            this->field_18->~motion_effect_struct();
            operator delete(v3);
            this->field_18 = nullptr;
        }

        if ( this->is_flagged(0x400) )
        {
            auto *v4 = trigger_manager::instance->find_instance(this);
            if (v4 != nullptr) {
                trigger_manager::instance->delete_trigger(v4);
            }
        }

        if ( !this->is_ext_flagged(0x20000) )
        {
            if ( this->manage_abs_po() )
            {
                auto *v5 = this->my_abs_po;
                if (v5 != this->my_rel_po) {
                    mem_dealloc(v5, sizeof(*v5));

                    this->my_abs_po = nullptr;
                }
            }

            if (!this->is_conglom_member()) {
                entity_handle_manager::deregister_entity(this);
            }
        }

        this->has_sound_and_pfx_ifc();
        if (this->has_sound_and_pfx_ifc())
        {
            auto *v6 = this->my_sound_and_pfx_interface;
            if (v6->field_8) {
                if (v6 != nullptr) {
                    void (__fastcall *finalize)(void *, void *, bool) = CAST(finalize, get_vfunc(v6->m_vtbl, 0x0));
                    finalize(v6, nullptr, true);
                }

            } else {
                void (__fastcall *func24)(void *) = CAST(func24, get_vfunc(v6->m_vtbl, 0x24));
                func24(v6);
            }

            this->my_sound_and_pfx_interface = nullptr;
        }

        entity_handle_manager::remove_entity(this->my_handle);

    } else {
        THISCALL(0x004F3550, this);
    }
}

bool entity_base::is_ext_flagged(uint32_t a2) const {
    return (a2 & this->field_8) != 0;
}

bool entity_base::is_flagged_in_the_moved_list() const {
    return this->is_ext_flagged(0x800000u);
}

bool entity_base::is_dynamic() const {
    return this->is_ext_flagged(0x80000000);
}

bool entity_base::playing_scene_anim() const {
    return this->is_ext_flagged(0x40000);
}

bool entity_base::manage_abs_po() {
    return !this->is_ext_flagged(EFLAG_EXT_DOES_NOT_MANAGE_ABS_PO);
}

bool entity_base::is_mashed_member() {
    return this->is_ext_flagged(0x10u);
}

po &entity_base::get_abs_po() {
    assert(my_abs_po != nullptr);

    if (this->is_ext_flagged(0x10000000u)) {
        this->update_abs_po(true);
    }

    return *this->my_abs_po;
}

bool entity_base::is_rel_po_dirty() const {
    return (this->field_8 & 0x8000000) != 0;
}

void entity_base::set_timer(int new_timer) {
    assert(new_timer >= 0 && new_timer <= 0xFF);
    this->m_timer = new_timer;
}

unsigned int entity_base::compute_rel_po_from_model() {
    return static_cast<uint32_t>(THISCALL(0x004D6050, this));
}

int entity_base::add_callback(string_hash a2,
                              void (*callback)(event *, entity_base_vhandle, void *),
                              void *a4,
                              bool a5) {
    entity_base_vhandle v5 = this->my_handle;

    int result = 0;

    event_recipient_entry *v7;

    event_type *v6 = event_manager::register_event_type(a2, false);
    if (v6 != nullptr && (v7 = v6->create_recipient_entry(v5)) != nullptr) {
        result = v7->add_callback(callback, a4, a5);
    }

    return result;
}

bool entity_base::event_raised_last_frame(string_hash a2) {
    return (bool) THISCALL(0x004F3800, this, a2);
}

bool entity_base::has_model_po() const
{
    if ( !this->is_conglom_member() ) {
        return false;
    }

    if ( this == this->my_conglom_root ) {
        return false;
    }

    auto v2 = this->rel_po_idx - 1;
    return v2 < this->my_conglom_root->all_model_po.size();
}

void entity_base::set_abs_po(const po &the_po)
{
    TRACE("entity_base::set_abs_po");

    if constexpr (1) {
        assert(the_po.is_valid());

        *this->my_rel_po = the_po;
        this->dirty_family(false);

        if (this->is_conglom_member() || this->is_a_conglomerate()) {
            this->dirty_model_po_family();
        }

        this->po_changed();

    } else {
        THISCALL(0x0048C5C0, this, &the_po);
    }
}

const vector3d &entity_base::get_rel_position()
{
    if (this->is_rel_po_dirty()) {
        this->compute_rel_po_from_model();
    }

    assert(my_rel_po->is_valid());

    return this->my_rel_po->get_position();
}

const vector3d &entity_base::get_abs_position()
{
    assert(this->my_abs_po != nullptr);

    if (this->is_ext_flagged(0x10000000u)) {
        this->update_abs_po(true);
    }

    return this->my_abs_po->get_position();
}

entity_base *entity_base::get_first_child() {
    return this->m_child;
}

po *entity_base::get_model_po() const
{
    assert(is_conglom_member());

    assert((conglomerate*)this != this->my_conglom_root);

    assert(this->rel_po_idx - 1 < this->my_conglom_root->all_model_po.size());

    return &this->my_conglom_root->all_model_po.m_data[uint16_t(this->rel_po_idx - 1)];
}

void entity_base::dirty_family(bool a2)
{
    if constexpr (1)
    {
        this->set_ext_flag_recursive_internal(static_cast<entity_ext_flag_t>(EXTFLAG_DIRTY_ABS_PO), true);
        this->set_ext_flag_recursive_internal(static_cast<entity_ext_flag_t>(EXTFLAG_DIRTY), true);

        for (auto *v2 = this->get_first_child(); v2 != nullptr; v2 = v2->field_28)
        {
            if (!v2->is_ext_flagged(EXTFLAG_DIRTY_ABS_PO) || a2) {
                v2->dirty_family(a2);
            }
        }
    } else {
        THISCALL(0x004BFED0, this, a2);
    }
}

void entity_base::enter_limbo()
{
    if ( !this->is_ext_flagged(EXTFLAG_UPDATE_VIA_REGIONLINK) && this->is_ext_flagged(0x200u) )
    {
        vhandle_type<entity> v3 {this->get_my_handle()};
        add_to_limbo_list(v3);
        this->set_ext_flag_recursive_internal(static_cast<entity_ext_flag_t>(EXTFLAG_UPDATE_VIA_REGIONLINK), true);
        this->raise_event(event::ENTER_LIMBO);
    }
}

void entity_base::exit_limbo()
{
    if constexpr (0) {
        if ( this->is_ext_flagged(EXTFLAG_UPDATE_VIA_REGIONLINK) && this->is_ext_flagged(0x200u) )
        {
            vhandle_type<entity> v3 {this->get_my_handle()};
            remove_from_limbo_list(v3);
            this->set_ext_flag_recursive_internal(static_cast<entity_ext_flag_t>(EXTFLAG_UPDATE_VIA_REGIONLINK), false);
            this->raise_event(event::EXIT_LIMBO);
        }
    } else {
        THISCALL(0x004F3B80, this);
    }
}

void entity_base::on_fade_distance_changed_internal(int a2) {
    assert(is_an_entity() && ((entity *) this)->is_renderable());

    if (this->is_visible() && distance_fader::fade_distances()[a2] > 140.0f) {
        g_world_ptr->field_A0.add_far_away_entity({this->my_handle});
    }
}

bool entity_base::is_visible() const {
    auto result = this->is_flagged(FLAG_IS_VISIBLE);
    return result;
}

void entity_base::set_fade_distance(Float a1) {
    auto idx = distance_fader::get_fade_index_for_fade_distance(a1);
    this->on_fade_distance_changed(idx);
}

void entity_base::on_fade_distance_changed(int value) {
    assert((value & ~FADE_DISTANCE_MASK) == 0);

    this->field_8 &= 0xFFFFFFF0;
    this->field_8 |= value;

    this->on_fade_distance_changed_internal(value);
}

bool entity_base::is_flagged(uint32_t a2) const
{
    return (a2 & this->field_4) != 0;
}

bool entity_base::is_walkable() const
{
    return this->is_flagged(FLAG_CAN_WALK_ON);
}

bool entity_base::are_collisions_active() const {
    return this->is_flagged(FLAG_COLLISION_ACTIVE);
}

void entity_base::dirty_model_po_family()
{
    if constexpr (1)
    {
        if ( !this->is_a_conglomerate() )
        {
            this->field_8 |= 0x100u;

            for (entity_base *v2 = this->get_first_child(); v2 != nullptr; v2 = v2->field_28) {
                if (v2->is_conglom_member() && v2->my_conglom_root == this->my_conglom_root) {
                    v2->dirty_model_po_family();
                }
            }
        }
    } else {
        THISCALL(0x004BFF20, this);
    }
}

void entity_report(entity_base *a1, const mString &a2, bool a3) {
    CDECL_CALL(0x004E1490, a1, &a2, a3);
}

void entity_set_abs_position(entity_base *ent, const vector3d &pos)
{
    assert(ent != nullptr && "No Entity passed in");
    assert(pos.length2() < MAX_ALLOWED_POSITION_LENGTH_SQUARED);

    if constexpr (1)
    {
        auto ENABLE_LONG_MALOR_ASSERTS = os_developer_options::instance->get_int(
            mString{"ENABLE_LONG_MALOR_ASSERTS"});
        if (ENABLE_LONG_MALOR_ASSERTS > 0 && !ent->is_flagged(0x800) && ent->is_an_actor() &&
            !bit_cast<actor *>(ent)->get_allow_tunnelling_into_next_frame()) {
            auto v19 = (float) (ENABLE_LONG_MALOR_ASSERTS * ENABLE_LONG_MALOR_ASSERTS);
            auto a2 = ent->get_abs_position();

            auto v17 = pos;
            auto v3 = a2 - v17;
            auto v4 = v3.length2();
            if (v4 > v19) {
                auto id = ent->get_id();
                auto *a3 = id.to_string();
                if (strstr(a3, "PED_") == nullptr && strstr(a3, "ENTID_") == nullptr) {
                    auto v6 = a2 - v17;
                    auto v7 = v6.length();

                    char a1[256];
                    sprintf(a1, "Entity %s moved %f meters", a3, v7);

                    sp_log(a1);
                    assert(0);
                }
            }
        }

        auto *parent = ent->m_parent;
        if (parent != nullptr) {
            auto posn = parent->get_abs_po().inverse_xform(pos);
            ent->set_abs_position(posn);

            assert(posn.length2() < MAX_ALLOWED_POSITION_LENGTH_SQUARED);

        } else {
            ent->set_abs_position(pos);
        }

    } else {
        CDECL_CALL(0x004E1230, ent, &pos);
    }
}

void entity_teleport_abs_po(entity_base *a1, const po &a2, bool a3)
{
    CDECL_CALL(0x004F3890, a1, &a2, a3);
}

void entity_teleport_abs_position(entity_base *a2, const vector3d &a3, bool a4)
{
    auto &v5= a2->get_abs_po();
    v5.set_position(a3);
    entity_teleport_abs_po(a2, v5, a4);
}

void check_po(entity_base *e)
{
    TRACE("check_po");

    if constexpr (1)
    {
        auto &abs_po = e->get_abs_po();
        if ( abs_po.has_nonuniform_scaling() )
        {
            auto id = e->get_id();
            auto *v3 = id.to_string();
            mString v9 {v3};
            auto v7 = v9 + ": non-uniform scaling is not supported";
            sp_log(v7.c_str());
        }
        else if ( e->is_an_actor() )
        {
            if ( bit_cast<actor *>(e)->get_colgeom() != nullptr )
            {
                auto &v4 = e->get_abs_po();
                auto v15 = vector3d {v4[0]}.length();
                if ( v15 < 0.99000001 || v15 > 1.01 )
                {
                    auto v5 = e->get_id();
                    auto *v6 = v5.to_string();
                    mString v10 {v6};
                    auto v8 = v10 + ": scaling is not supported on entities with collision geometry";
                    sp_log(v8.c_str());
                }
            }
        }
    } else {
        CDECL_CALL(0x0053CD00, e);
    }
}

// ---------------------------------------------------------------------------
// .ENT mod overrides (see entity_base.h for the contract)
//
// A .ent image is the same generic-mash blob a retail pack serves for
// RESOURCE_KEY_TYPE_ENTITY: 16-byte generic_mash_header, then the entity
// object image whose vtable word holds the 0x7ACE5BAD placeholder until
// parse_generic_mash_init patches it from ent_v_table_lookup[class_id].
// The override rides resource_manager::get_resource (the one funnel every
// named-entity fetch passes through - dynamic spawns, fx caches, console
// "spawn"), swapping the byte pointer + size while the pack slot the retail
// lookup produced stays in place, so instance tracking and pack-unload
// teardown keep working. Scene-embedded entities (.ENS blobs) never do a
// per-entity fetch and are out of scope on purpose.
// ---------------------------------------------------------------------------

bool modEntImageUsable(const uint8_t *bytes, size_t size)
{
    if (bytes == nullptr || size < sizeof(generic_mash_header) + 4)
        return false;

    const auto *header = bit_cast<const generic_mash_header *>(bytes);

    // the header authenticates itself; IN_USE (0x80000000) and the
    // vtable flag (0x40000000) sit outside the checksummed low 28 bits
    if (header->safety_key != header->generate_safety_key())
        return false;

    // entity images are polymorphic mashes: class_id indexes the 28-entry
    // ent_v_table_lookup/ent_size_lookup tables (parse_entity_mash passes
    // num_table_entries = 0x1C)
    if (!header->is_flagged(0x40000000) || header->class_id >= 0x1C)
        return false;

    // shared mash data lives at header + field_8, inside the image
    if (header->field_8 < (int)sizeof(generic_mash_header) ||
        (size_t)header->field_8 > size)
        return false;

    return true;
}

bool modEntRegister(const std::filesystem::path &path,
                    std::vector<uint8_t> &&fileData)
{
    const std::string stem = transformToLower(path.stem().string());

    if (!modEntImageUsable(fileData.data(), fileData.size()))
    {
        sp_log("[mod] ent \"%s\": not a packer-produced entity mash image, ignored",
               path.filename().string().c_str());
        return false;
    }

    // A dumped-from-memory image may carry a live IN_USE flag; the flag is
    // outside the checksummed bits, so clearing it keeps the header valid.
    auto *header = bit_cast<generic_mash_header *>(fileData.data());
    header->field_4 &= ~_MASH_FLAG_IN_USE;

    const uint32_t hash = to_hash(stem.c_str());

    // Re-registration (enumerate_mods() reruns): replace, don't stack.
    {
        auto range = Mods.equal_range(hash);
        for (auto it = range.first; it != range.second; )
            it = (it->second.Type == MOD_TYPE_ENT_FILE) ? Mods.erase(it) : std::next(it);
    }

    sp_log("[mod] registered ent override \"%s\" -> \"%s\" (%u bytes, key 0x%08X, class %u)",
           path.filename().string().c_str(), stem.c_str(),
           (unsigned)fileData.size(), hash, (unsigned)header->class_id);

    Mods.emplace(hash, Mod{path, MOD_TYPE_ENT_FILE, std::move(fileData)});

    // Hash-named drops ("extra/0x1189AB87.ent") also bind under the literal
    // value, mirroring the mesh/texture/wav stem convention.
    if (uint32_t literal = 0;
        modParseLiteralHash(stem, &literal) && literal != hash)
    {
        const Mod *just = getMod(hash, MOD_TYPE_ENT_FILE);
        if (just != nullptr)
        {
            Mods.emplace(literal, Mod{just->Path, MOD_TYPE_ENT_FILE, just->Data});
            sp_log("[mod] ent \"%s\" also bound as literal hash 0x%08X",
                   stem.c_str(), literal);
        }
    }

    return true;
}

uint8_t *modEntGetOverride(uint32_t classHash, int *sizeOut)
{
    Mod *mod = getMod(classHash, MOD_TYPE_ENT_FILE);
    if (mod == nullptr || mod->Data.empty())
        return nullptr;

    // One immortal, 16-aligned, writable copy per mod image, reused for
    // every spawn: the first parse un-mashes it in place and flags it
    // IN_USE; every later spawn takes parse_generic_mash_init's clone
    // branch - exactly a retail pack image's lifecycle, except this one
    // never unloads. The bytes in Mods stay pristine as the master copy.
    struct entImage { const Mod *source; void *copy; };
    static std::unordered_map<uint32_t, entImage> s_images;

    auto &slot = s_images[classHash];
    if (slot.copy == nullptr || slot.source != mod)
    {
        void *copy = tlMemAlloc((uint32_t)mod->Data.size(), 16u, 0x2000000u);
        if (copy == nullptr)
            return nullptr;
        std::memcpy(copy, mod->Data.data(), mod->Data.size());
        slot.source = mod;
        slot.copy = copy;   // the previous copy (if any) stays alive: live
                            // entities keep pointers into it
    }

    if (sizeOut != nullptr)
        *sizeOut = (int)mod->Data.size();
    return static_cast<uint8_t *>(slot.copy);
}

void entity_base_patch() {

    {
        //SET_JUMP(0x004F3400, entity_base_constructor);
    }

    {
        FUNC_ADDRESS(address, &entity_base::get_conglom_owner);
        SET_JUMP(0x00502B50, address);
    }

    {
        FUNC_ADDRESS(address, &entity_base::_un_mash);
        set_vfunc(0x00882CC4, address);

    }
    return;
    
    {
        FUNC_ADDRESS(address, &entity_base::update_abs_po);
        SET_JUMP(0x004DB590, address);
    }

    SET_JUMP(0x004E1100, entity_set_abs_po);

    SET_JUMP(0x004E1230, entity_set_abs_position);

    SET_JUMP(0x004E1290, entity_set_abs_parent);

#if 0

    {
        FUNC_ADDRESS(address, &entity_base::is_a_line_anchor);
        set_vfunc(0x00882C18, address);
    }

    {
        FUNC_ADDRESS(address, &entity_base::on_fade_distance_changed_internal);
        //SET_JUMP(0x004CB450, address);
    }
#endif
}
