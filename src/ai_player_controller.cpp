#include "ai_player_controller.h"

#include "actor.h"
#include "base_ai_core.h"
#include "camera.h"
#include "common.h"
#include "conglom.h"
#include "debugutil.h"
#include "func_wrapper.h"
#include "game.h"
#include "hero_type_lookup.h"
#include "osassert.h"
#include "pole_swing_inode.h"
#include "spiderman_camera.h"
#include "trace.h"

#include <vtbl.h>

VALIDATE_SIZE(ai_player_controller, 0x424u);

ai_player_controller::ai_player_controller(actor *a2)
{
    if constexpr (0)
    {
        this->m_vtbl = 0x0087EAD8;

        this->gb_jump = {};

        this->gb_swing_raw = {};

        this->gb_attack = {};

        this->gb_attack_secondary = {};

        this->gb_grab = {};

        this->gb_range = {};

        this->field_150 = {};

        this->field_254 = {};
        this->field_288 = {};

        this->gb_camera_center = {};

        this->field_1B8 = {};

        this->gb_swing = {};

        this->field_220 = {};

        for ( auto &axis : this->field_2BC ) {
            axis = {};
        }

        for ( auto &axis : this->field_2BC ) {
            axis.field_8 = 0.1;
        }


        this->field_3E0 = ZEROVEC;
        this->field_4[1] = bit_cast<conglomerate *>(a2);
        this->field_14 = 0;
        this->field_4[0] = nullptr;

        this->m_spidey_loco_mode = static_cast<eHeroLocoMode>(0);
        this->m_prev_spidey_loco_mode = static_cast<eHeroLocoMode>(0);

        this->field_3F8 = 0.0;
        this->field_3FC = 0.0;

        this->field_400 = YVEC;
        this->m_hero_type = this->find_native_hero_type();

        this->unlock_controls(false);

        this->force_always_camera_relative(false);
        this->remap_controls();
    }
    else
    {
        THISCALL(0x004728D0, this, a2);
    }
}

void ai_player_controller::lock_controls(bool a2)
{
    TRACE("ai_player_controller::lock_controls");

    return;

    if ( a2 && !this->field_3DC ) {
        error("Someone (likely script or UI code) tried to lock controls when they were already locked.");
    }

    if ( this->field_3DC ) {
        debug_print_va("-- controls locked (previous was %s)", "locked");
    } else {
        debug_print_va("-- controls locked (previous was %s)", "unlocked");
    }

    this->field_3DC = false;
    g_spiderman_camera_ptr()->sub_4B3260(true);
}

void ai_player_controller::unlock_controls(bool a2)
{
    if ( a2 && this->field_3DC ) {
        error("Someone (likely script or UI code) tried to unlock controls when they were already unlocked.");
    }

    if ( this->field_3DC ) {
        debug_print_va("-- controls unlocked (previous was %s)", "locked");
    } else {
        debug_print_va("-- controls unlocked (previous was %s)", "unlocked");
    }

    this->field_3DC = true;
    g_spiderman_camera_ptr()->sub_4B3260(false);
}

void ai_player_controller::set_spidey_loco_mode(eHeroLocoMode a2)
{
    if ( a2 != this->m_spidey_loco_mode ) {
        this->m_prev_spidey_loco_mode = this->m_spidey_loco_mode;
    }

    this->m_spidey_loco_mode = a2;
}

hero_type_enum ai_player_controller::find_hero_type() const
{
    auto *controller = this->field_4[1];
    if (controller == nullptr) {
        return hero_type_enum::UNDEFINED;
    }

    auto *ai_core = controller->get_ai_core();
    if (ai_core == nullptr) {
        return hero_type_enum::UNDEFINED;
    }

    return find_hero_type_from_graphs([ai_core](const char *name) {
        const resource_key key{
            string_hash{name},
            RESOURCE_KEY_TYPE_AI_STATE_GRAPH
        };
        return ai_core->find_machine(key) != nullptr;
    });
}

hero_type_enum ai_player_controller::find_native_hero_type() const
{
    // The original constructor stores this result at +0x420. Native mission
    // scripts, map/token filters and movement read that field directly, so a
    // new value of 4 must not escape into the retail gameplay code.
    return native_hero_type(this->find_hero_type());
}

anchor_storage_class ai_player_controller::get_poleswing_anchor() const
{
    auto *v2 = this->field_4[1]->get_ai_core();
    auto *v3 = (ai::pole_swing_inode *) v2->get_info_node(ai::pole_swing_inode::default_id(), true);

    auto a2 = v3->field_1C;
    return a2;
}

int ai_player_controller::get_spidey_loco_mode() const
{
    return this->m_spidey_loco_mode;
}

void ai_player_controller::set_player_num(int a2)
{
    this->field_14 = a2;
    this->remap_controls();
}

void ai_player_controller::clear_controls()
{
    debug_print_va("-- controls cleared");
    this->remap_controls();
    this->gb_jump.clear_flags();
    this->gb_swing_raw.clear_flags();
    this->gb_attack.clear_flags();
    this->gb_attack_secondary.clear_flags();
    this->gb_grab.clear_flags();
    this->gb_range.clear_flags();
    this->field_150.clear_flags();
    this->field_254.clear_flags();
    this->field_288.clear_flags();
    this->gb_camera_center.clear_flags();
    this->field_1B8.clear_flags();
    this->gb_swing.clear_flags();
}

void ai_player_controller::remap_controls()
{
    if constexpr (0)
    {
        auto v2 = input_mgr::instance->field_58;
        this->gb_jump.set_id(v2);

        this->gb_swing_raw.set_id(v2);
        this->gb_attack.set_id(v2);
        this->gb_attack_secondary.set_id(v2);
        this->gb_grab.set_id(v2);
        this->gb_range.set_id(v2);
        this->field_150.set_id(v2);
        this->field_254.set_id(v2);
        this->field_288.set_id(v2);
        this->gb_camera_center.set_id(v2);
        this->field_1B8.set_id(v2);
        this->gb_swing.set_id(v2);
        this->field_220.set_id(v2);

        this->field_2BC[0].set_id(v2);
        this->field_2BC[1].set_id(v2);
        this->field_2BC[2].set_id(v2);
        this->field_2BC[3].set_id(v2);
        this->field_2BC[4].set_id(v2);
        this->field_2BC[5].set_id(v2);

        this->gb_jump.set_control(static_cast<game_control_t>(96));
        this->gb_swing_raw.set_control(static_cast<game_control_t>(101));

        this->gb_swing_raw.set_modifier(game_button {static_cast<game_control_t>(104)});

        this->gb_swing_raw.set_trigger_type(0);

        this->gb_attack.set_control((game_control_t)97);
        this->gb_attack_secondary.set_control((game_control_t)99);
        this->gb_grab.set_control((game_control_t)98);
        this->gb_range.set_control((game_control_t)104);

        this->gb_range.field_2C = 0.1f;

        this->field_150.set_control((game_control_t)100);
        this->field_254.set_control((game_control_t)104);
        this->field_288.set_control((game_control_t)101);

        this->gb_camera_center.set_primary(game_button {(game_control_t)100});
        this->gb_camera_center.set_modifier(game_button {(game_control_t)103});

        this->gb_camera_center.set_trigger_type(1);

        this->field_1B8.set_control((game_control_t)101);
        this->field_1B8.field_2C = 0.1f;

        this->gb_swing.set_control((game_control_t)101);
        this->field_220.set_control((game_control_t)114);

        this->field_2BC[0].set_control(106);
        this->field_2BC[1].set_control(107);
        this->field_2BC[2].set_control(110);
        this->field_2BC[3].set_control(111);
        this->field_2BC[4].set_control(108);
        this->field_2BC[5].set_control(109);
    }
    else
    {
        THISCALL(0x00468FE0, this);
    }
}

game_button *ai_player_controller::get_gb_jump() {
    return &this->gb_jump;
}

game_button *ai_player_controller::get_gb_attack() {
    return &this->gb_attack;
}

game_button *ai_player_controller::get_gb_attack_secondary() {
    return &this->gb_attack_secondary;
}

game_button *ai_player_controller::get_gb_grab() {
    return &this->gb_grab;
}

game_button *ai_player_controller::get_gb_range() {
    return &this->gb_range;
}

game_button *ai_player_controller::get_gb_camera_center() {
    return &this->gb_camera_center;
}

game_button & ai_player_controller::get_gb_swing_raw() {
    return this->gb_swing;
}

vector3d ai_player_controller::convert_left_stick_from_camera_space_to_world_space(bool a3)
{
    if constexpr (0)
    {}
    else
    {
        vector3d result;
        THISCALL(0x00457C20, this, &result, a3);

        return result;
    }
}

vector3d ai_player_controller::compute_left_stick_from_camera()
{
    vector3d result;

    if constexpr (1)
    {
        auto *cam = g_game_ptr->get_current_view_camera(0);
        if (cam != nullptr)
        {
            auto v8 = this->field_2BC[1].field_10;
            auto v7 = this->field_2BC[0].field_10;

            vector3d look, up;
            cam->get_look_and_up(&look, &up);

            vector3d v11 = vector3d::cross(up, look);

            look[1] = 0.0;
            look.normalize();

            v11[1] = 0.0;
            v11.normalize();
            if (std::abs(v8) < 0.1f) {
                v8 = 0.0;
            }

            if (std::abs(v7) < 0.1f) {
                v7 = 0.0;
            }

            v11 = v11 * v8 - look * v7;
            v11.normalize();

            result = v11;
        } else {
            result = ZEROVEC;
        }

        return result;

    } else {
        THISCALL(0x004495D0, this, &result);

        return result;
    }
}

void ai_player_controller::update_controls(Float a2, bool a3)
{
    TRACE("ai_player_controller::update_controls");

    if (a3 || this->field_3DC && is_a_controllable_mode(this->m_spidey_loco_mode))
    {
        this->gb_jump.update(a2);
        this->gb_swing_raw.update(a2);
        this->gb_attack.update(a2);

        this->gb_attack_secondary.update(a2);
        this->gb_grab.update(a2);
        this->gb_range.update(a2);
        this->field_150.update(a2);
        this->field_254.update(a2);
        this->field_288.update(a2);
        this->gb_camera_center.update(a2);
        this->field_1B8.update(a2);
        this->gb_swing.update(a2);

        this->field_2BC[0].update(a2);
        this->field_2BC[1].update(a2);
        this->field_2BC[2].update(a2);
        this->field_2BC[3].update(a2);
        this->field_2BC[4].update(a2);
        this->field_2BC[5].update(a2);
    }

    this->field_220.update(a2);
}

void ai_player_controller::frame_advance(Float a2)
{
    TRACE("ai_player_controller::frame_advance");

    if (this->field_4[0] == nullptr) {
        this->field_4[0] = CAST(this->field_4[0], g_game_ptr->current_game_camera);

        auto *hero_camera = this->field_4[0];
        assert(hero_camera != nullptr);
    }

    this->update_controls(a2, false);

    this->field_3E0 = this->convert_left_stick_from_camera_space_to_world_space(false);

    this->field_3EC = this->compute_left_stick_from_camera();
}

float ai_player_controller::get_motion_force() {
    float __fastcall (*func)(void *self) = CAST(func, 0x00449880);

    return func(this);
}

void ai_player_controller_patch()
{
    {
        FUNC_ADDRESS(address, &ai_player_controller::frame_advance);
        SET_JUMP(0x00468E80, address);
    }

    {
        FUNC_ADDRESS(address, &ai_player_controller::lock_controls);
        REDIRECT(0x00741839, address);
    }
	
	{
        FUNC_ADDRESS(address, &ai_player_controller::find_native_hero_type);
        REDIRECT(0x0047357E, address);
    }
	
	
}
