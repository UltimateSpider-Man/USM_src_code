#include "ltd_edition.h"

#include "common.h"
#include "func_wrapper.h"

#include "mission_stack_manager.h"


#include "comic_panels.h"


#include "game.h"


#include "movie_manager.h"


#include "sound_instance_id.h"

VALIDATE_SIZE(ltd_edition, 0x14cu);

ltd_edition::ltd_edition(FEMenuSystem *a2, int a3, int a4) : FEMenu(a2, 0, a3, a4, 8, 0) {
    THISCALL(0x00614570, this, a2, a3, a4);
}


constexpr auto NUM_MOVIES = 6u;

const char* movies_list[NUM_MOVIES] = {
    "G4",
	"VENOM_BIO",
	"USM_CARNAGE_BIO_V2",
	"USM_BEETLE_BIO_V2",
	"SPIDEY_BIO",
	"TIPSTRICKS"
};

void ltd_edition::Deactivate(int a2)
{
    mString pack_name{"unlockables_ltd"};

    auto *msm = mission_stack_manager::s_inst;
    if (msm->is_pack_pushed(pack_name)) {
        msm->pop_mission_pack_immediate(pack_name, pack_name);
    }

    this->field_28 &= ~0x80u;
	
	
	THISCALL(0x00614690, this, a2);
}


void ltd_edition::scroll(int a1)
{
      THISCALL(0x00614730,this, a1);
}


void ltd_edition::scroll_left(int a2)
{
    if (--this->field_110 < 0) {
        this->field_110 = 6;
    }

    this->scroll(1);

static string_hash sfx_id_hash{"fe_ps_udscroll"};
[[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);

THISCALL(0x006258B0,this, a2);
}


void ltd_edition::scroll_right(int a2)
{
    if (++this->field_110 > 6) {
        this->field_110 = 0;
    }

    this->scroll(-1);

static string_hash sfx_id_hash{"fe_ps_udscroll"};
[[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);

THISCALL(0x006259E0,this, a2);
} 

void ltd_edition::on_back(int a2)
{
    if (this->field_11C)
    {
        this->field_11C = 0;
        return;
    }

    mString pack_name{"unlockables_ltd"};
    mission_stack_manager::s_inst->pop_mission_pack_immediate(pack_name, pack_name);

    this->field_2C->MakeActive(8);

    comic_panels::game_play_panel()->field_67 = 1;

	
	static string_hash sfx_id_hash{"fe_ul_back"};
[[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);

THISCALL(0x00625A90,this, a2);
}


void ltd_edition::on_resume_game(int a2)
{
    if (this->field_11C)
    {
        this->field_11C = 0;
        return;
    }

    mString pack_name{"unlockables_ltd"};
    mission_stack_manager::s_inst->pop_mission_pack_immediate(pack_name, pack_name);

    auto *menu_system = this->field_2C;
    if (menu_system->m_index >= 0)
    {
        menu_system->MakeActive(-1);
        g_game_ptr->unpause();
        comic_panels::game_play_panel()->field_67 = this->field_30;
    }
	
	static string_hash sfx_id_hash{"fe_ul_back"};
[[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);

THISCALL(0x00625C50,this, a2);
}


void ltd_edition::on_select(int a2)
{
	static string_hash sfx_id_hash{"fe_ps_accept"};
[[maybe_unused]] sound_instance_id id = sub_60B960(sfx_id_hash, 1.0, 1.0);

    movie_manager::load_and_play_movie(movies_list[this->field_110], 0, 0);

    this->field_11C = 1;
    this->field_2C->UpdateButtonPresses();
	
	THISCALL(0x0062DD40,this, a2);
}



