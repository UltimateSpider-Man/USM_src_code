#include "consolecmds.h"

#include "actor.h"
#include "base_ai_core.h"
#include "collide.h"
#include "console.h"
#include "consolevars.h"
#include "cut_scene.h"
#include "cut_scene_player.h"
#include "damage_interface.h"
#include "debug_render.h"
#include "dvar.h"
#include "entity_handle_manager.h"
#include "filespec.h"
#include "game.h"
#include "game_settings.h"
#include "local_collision.h"
#include "mstring.h"
#include "nal_anim.h"
#include "nal_system.h"
#include "oldmath_po.h"
#include "mission_table_container.h"
#include "region.h"
#include "resource_directory.h"
#include "resource_manager.h"
#include "script.h"
#include "script_object.h"
#include "script_library_class.h"
#include "script_manager.h"
#include "variables.h"
#include "wds.h"

std::list<ConsoleCommand *> *g_console_cmds{nullptr};

ConsoleCommand::ConsoleCommand()
{
    this->setName({});

    if (g_console_cmds == nullptr) {
        g_console_cmds = new std::list<ConsoleCommand *>{};
    }

    g_console_cmds->push_back(this);
}

static constexpr auto MAX_COMMAND_NAME_LEN = 32;

std::string ConsoleCommand::getName() const
{
    std::string out{this->field_4};
    return out;
}

void ConsoleCommand::setName(const std::string &pName) {
    assert(pName.size() < MAX_COMMAND_NAME_LEN);

    auto *v2 = pName.c_str();
    strcpy(this->field_4, v2);
    strlwr(this->field_4);
}

bool ConsoleCommand::process_cmd(const std::vector<std::string> &) {
    return false;
}

bool ConsoleCommand::match(const std::string &a2) const
{
    std::string v4{this->field_4};

    return (v4 == a2);
}

bool ExecCommand::process_cmd(const std::vector<std::string> &a1) {
    if (a1.size()) {
        auto &v1 = a1.front();
        g_console->exec(v1.c_str());
    }

    return true;
}

static HelpCommand g_HelpCommand{};

HelpCommand::HelpCommand() {
    setName(std::string{"help"});
}

bool HelpCommand::process_cmd(const std::vector<std::string> &a2)
{
    if (!a2.empty())
    {
        g_console->addToLog("");
        auto &v2 = a2[0];

        mString v9{v2.c_str()};
        v9.to_lower();

        auto *v8 = g_console->getCommand(v9.c_str());
        if (v8 != nullptr) {
            auto *v3 = v8->helpText();
            g_console->addToLog(v3);
        } else {
            auto *v7 = g_console->getVariable(v9.c_str());
            if (v7 != nullptr) {
                auto *v4 = v7->helpText();
                g_console->addToLog(v4);
            } else {
                auto *v5 = v9.c_str();
                g_console->addToLog("Command '%s' not found!", v5);
            }
        }

    } else {
        g_console->addToLog("");
        g_console->addToLog("<-- Keys -->");
        g_console->addToLog("Up Arrow    Scroll command buffer up");
        g_console->addToLog("Down Arrow  Scroll command buffer down");
        g_console->addToLog("Tab         Complete current command");
        g_console->addToLog("Page Up     Scroll log buffer up");
        g_console->addToLog("Page Down   Scroll log buffer down");
        g_console->addToLog("Home        Scroll log buffer top");
        g_console->addToLog("End         Scroll log buffer bottom");
        g_console->addToLog("ESC         Show / hide console");
        g_console->addToLog("");
        g_console->addToLog("<-- Commands -->");
        g_console->addToLog("cmdlist     List all commands");
        g_console->addToLog("varlist     List all variables");
        g_console->addToLog("help <?>    Get help for a command");
    }

    return true;
}

static ListEntsCommand g_ListEntsCommand{};

ListEntsCommand::ListEntsCommand() {
    setName(std::string{"list_ents"});
}

bool ListEntsCommand::process_cmd(const std::vector<std::string> &) {
    g_console->addToLog("visible entities:");
    auto *v10 = g_world_ptr->ent_mgr.get_entities();
    auto it = v10->begin();
    while (1) {
        auto end = v10->end();
        if (it == end) {
            break;
        }

        auto &v8 = (*it.field_8);

        if (v8 != nullptr && !v8->sub_48AE20()) {
            if (v8->is_still_visible()) {
                auto v4 = v8->get_id();
                auto *v5 = v4.to_string();
                g_console->addToLog("  %s", v5);
            }
        }

        ++it;
    }

    return true;
}

static LoadLevelCommand g_LoadLevelCommand{};

LoadLevelCommand::LoadLevelCommand() {
    setName("load_level");
}

bool LoadLevelCommand::process_cmd(const std::vector<std::string> &a1) {
    if (a1.size() != 1) {
        return false;
    }

    auto &v2 = a1[0];
    auto *v3 = v2.c_str();
    g_console->addToLog("Now loading level %s", v3);
    auto &v4 = a1[0];
    g_game_ptr->load_new_level(v4.c_str(), -1);
    return true;
}

static VariableList g_VariableList{};

VariableList::VariableList() {
    setName("varlist");
}

bool VariableList::process_cmd(const std::vector<std::string> &)
{
    g_console->addToLog("");
    g_console->addToLog("<-- Console Variables -->");

    if (g_console_vars != nullptr && g_console_vars->size())
    {
        auto &vars = *g_console_vars;
        for (auto &v3 : vars)
        {
            if (v3 != nullptr)
            {
                auto v9 = v3->getValue();
                auto v8 = v3->getName();

                auto *v7 = v9.c_str();
                auto *v5 = v8.c_str();
                g_console->addToLog("%s %s", v5, v7);
            }
        }
    }

    return 1;
}

static SetCommand g_SetCommand{};

SetCommand::SetCommand() {
    setName("set");
}

bool SetCommand::process_cmd(const std::vector<std::string> &a2)
{
    if (a2.size() > 1) {
        auto &v2 = a2[0];
        auto *v12 = g_console->getVariable(v2);
        if (v12 != nullptr) {
            auto v9 = v12->match(std::string{"health"});
            if (v9 && a2.size() == 3) {
                v12->setValue(a2[1], a2[2]);
            } else {
                v12->setValue(a2[1]);
            }

        } else {
            auto &v5 = a2[0];
            auto *v6 = v5.c_str();
            g_console->addToLog("??? %s", v6);
        }
    }

    return true;
}

static GetCommand g_GetCommand{};

GetCommand::GetCommand() {
    setName("get");
}

bool GetCommand::process_cmd(const std::vector<std::string> &a2) {
    if (a2.size()) {
        auto &v2 = a2[0];
        auto *v11 = g_console->getVariable(v2);
        if (v11 != nullptr) {
            auto v10 = v11->getValue();

            auto v8 = v11->getName();

            auto *v7 = v10.c_str();
            auto *v3 = v8.c_str();
            g_console->addToLog("%s is %s", v3, v7);

        } else {
            auto &v4 = a2[0];
            auto *v5 = v4.c_str();
            g_console->addToLog("??? %s", v5);
        }
    }

    return true;
}

static GameStateCommand g_GameStateCommand{};

GameStateCommand::GameStateCommand() {
    setName("game_state");
}

bool GameStateCommand::process_cmd(const std::vector<std::string> &cmds) {
    if (cmds.size() && cmds.size() <= 2) {
        auto &v3 = cmds[0];
        mString a1 {v3.c_str()};

        bool a2 = false;
        script_library_class *var_type = nullptr;
        auto *v18 = static_cast<float *>(script_manager::get_game_var_address(a1, &a2, &var_type));
        if (v18 != nullptr) {
            if (var_type != nullptr) {
                assert(strcmp(var_type->get_name(), "num") == 0);
            }

            if (cmds.size() <= 1) {
                auto *v15 = (a2 ? "GameVar" : "SharedVar");

                auto v13 = *v18;
                auto *v10 = a1.c_str();
                g_console->addToLog("(%s) %s %s = %4.2f", v15, "num", v10, v13);
            } else {
                auto &v6 = cmds[1];
                auto v17 = v6;
                auto v16 = *v18;

                auto *v7 = v17.c_str();
                auto v8 = atof(v7);
                *v18 = v8;

                auto *v14 = (a2 ? "GameVar" : "SharedVar");

                auto v12 = *v18;
                auto *v9 = a1.c_str();
                g_console->addToLog("(%s) %s %s = %4.2f (was %4.2f)", v14, "num", v9, v12, v16);
            }
        } else {
            auto *v4 = a1.c_str();
            g_console->addToLog("Game state variable %s not found.", v4);
        }

    } else {
        auto *v2 = this->helpText();
        g_console->addToLog(v2);
    }

    return true;
}

static GameInfoCommand g_GameInfoCommand {};

bool GameInfoCommand::process_cmd(const std::vector<std::string> &a2)
{
    if ( a2.size() != 0 )
    {
        auto &v2 = a2.at(0);
        resource_key a2a {string_hash {v2.c_str()}, RESOURCE_KEY_TYPE_IFC_ATTRIBUTE};
        if ( a2.size() <= 1 )
        {
            float a3 = 0.0;
            auto *v11 = g_game_ptr->get_game_settings();
            v11->get_num(a2a, a3, true);

            auto &v8 = a2.at(0);
            auto *v9 = v8.c_str();
            g_console->addToLog("%s = %.2f", v9, a3);
        }
        else
        {
            auto &v4 = a2.at(1);
            auto *v5 = v4.c_str();
            auto num = atof(v5);
            auto *v6 = g_game_ptr->get_game_settings();
            v6->set_num(a2a, num);
        }
    }
    else
    {
        g_console->addToLog("");

        auto func = [](const char *str) -> void {
            resource_key key {string_hash {str}, RESOURCE_KEY_TYPE_IFC_ATTRIBUTE};
            float num = 0.0;
            auto *v11 = g_game_ptr->get_game_settings();
            if ( v11->get_num(key, num, true) ) {
                g_console->addToLog("%s = %.2f", str, num);
            }
        };

        func("HERO_POINTS");

        func("UPG_MINIMAP_PTS");

        func("UPG_IMPACT_WEB_PTS");

        func("UPG_HERO_METER_PTS");

        func("UPG_2ND_CHANCE_PTS");

        func("UPG_SPEED_PTS");

        func("UPG_UNDERDOG_PTS");

        func("HERO_METER_LEVEL_1");

        func("HERO_METER_LEVEL_2");

        func("HERO_METER_LEVEL_3");

        func("CUR_HERO_METER_LEVEL");

        func("CUR_HERO_METER_POINTS");

        func("OPT_SCORE_DISPLAY");

        func("OPT_CONT_1_RUMBLE");

        func("OPT_AUDIO_MODE");

        func("OPT_LEVELS_GAME");

        func("OPT_LEVELS_MUSIC");

        func("OPT_CONT_CONFIG");

        func("MINI_MAP_ENABLED");

        func("ENABLE_WEB_SHOT");

        func("ENABLE_WEB_DOME");

        func("ENABLE_ADV_WEB_DOME");

        func("ENABLE_WEB_GLOVES");

        func("ENABLE_ADV_WEB_GLOVES");

        func("ENABLE_IMPACT_WEB");

        func("ENABLE_ADV_IMPACT_WEB");

        func("ENABLE_WEB_COWBOY");

        func("ENABLE_WEB_YANK");

        func("ENABLE_YANK_MANIP");

        func("ENABLE_ZIP_WEB_ATTACK");

        func("ENABLE_SWING_CANNONBALL");

        func("ENABLE_DIRECTIONAL_ATTACK");

        func("ENABLE_DIRECTIONAL_DODGE");

        func("DIFFICULTY");

        func("RUN_SENSITIVITY");

        func("CHAR_REL_MOVEMENT");

        func("CRAWL_CHAR_REL_MOVEMENT");

        func("SHOW_STYLE_SCORE");

        func("HERO_HEALTH");

        func("HERO_TYPE");

        func("SWING_SPEED");

        func("CAM_INVERTED_X");

        func("CAM_INVERTED_Y");

        func("SPEED_DEMON");

        func("YOURE_AMAZING");

        func("ERRAND_BOY");

        func("BETTER_TO_DO");

        func("FANBOY");

        func("ANGSTY");

        func("SECRET_IDENTITY");

        func("STYLE");

        func("FASHION");

        func("PICTURES");

        func("BUNNY");

        func("CLOBBER");

        func("SCRAP_HEAP");

        func("SILVER_SPOON");

        func("KUNG_FU_FIGHTING");

        func("BIG_TIME_SUPER_HERO");

        func("ENJOY_THE_SUNSHINE");

        func("TIME_PLAYED");

        func("STORY_PERCENT_COMPLETE");

        func("STORY_MISSION_FAILURES");

        func("STORY_MISSIONS_COMPLETED");

        func("SPIDEY_RACES_COMPLETED");

        func("MILES_RUN_SPIDEY");

        func("MILES_CRAWLED_SPIDEY");

        func("MILES_WEB_SWINGING");

        func("MILES_WEB_ZIPPING");

        func("WEB_FLUID_USED");

        func("YANCY_DEFEATED");

        func("DIE_CASTEDEFEATED");

        func("HIGH_ROLLERSDEFEATED");

        func("FOU_TOU_BANGDEFEATED");

        func("VENOM_RACES_COMPLETED");

        func("MILES_RUN_VENOM");

        func("MILES_CRAWLED_VENOM");

        func("MILES_LOCOMOTION_JUMPED");

        func("PEOPLE_EATEN");

        func("CARS_THROWN");

        func("VENOM_HOT_PERSUIT");

        func("TOKENS_COLLECTED_COUNT");

        func("RACE_POINTS_EARNED_COUNT");

        func("COMBAT_TOURS_COMPLETED_COUNT");

        func("CITY_EVENTS_COMPLETED_COUNT");

        func("TOKENS_COLLECTED_MAX");

        func("RACE_POINTS_EARNED_MAX");

        func("COMBAT_TOURS_COMPLETED_MAX");

        func("CITY_EVENTS_COMPLETED_MAX");

        func("TOKENS_COLLECTED_TOT_COUNT");

        func("RACE_POINTS_EARNED_TOT_COUNT");

        func("COMBAT_TOURS_COMPLETED_TOT_COUNT");

        func("CITY_EVENTS_COMPLETED_TOT_COUNT");

        func("TOKENS_COLLECTED_TOT_MAX");

        func("RACE_POINTS_EARNED_TOT_MAX");

        func("COMBAT_TOURS_COMPLETED_TOT_MAX");

        func("CITY_EVENTS_COMPLETED_TOT_MAX");
    }

    return true;
}

static QuitCommand g_QuitCommand{};

QuitCommand::QuitCommand() {
    setName("quit");
}

bool QuitCommand::process_cmd(const std::vector<std::string> &)
{
    g_game_ptr->field_164 = true;
    return true;
}

static CommandList g_CommandList{};

CommandList::CommandList() {
    this->setName("cmdlist");
}

bool CommandList::process_cmd(const std::vector<std::string> &)
{
    g_console->addToLog("");
    g_console->addToLog("<-- Console Commands -->");
    if (g_console_cmds != nullptr && g_console_cmds->size())
    {
        auto &cmds = *g_console_cmds;
        for (auto &cmd : cmds)
        {
            if (cmd != nullptr)
            {
                auto v6 = cmd->getName();

                auto *v4 = v6.c_str();
                g_console->addToLog("%s", v4);
            }
        }
    }

    return true;
}

bool is_int_format(const mString &a1)
{
    if ( a1.size() == 0 )
    {
        return false;
    }

    for ( auto i = 0; i < a1.size(); ++i )
    {
        auto v2 = a1.at(i);
        if ( isdigit(v2) == 0 && (i || a1.at(0) != '-') && (i || a1.at(0) != '+') )
        {
            return false;
        }
    }

    return true;
}

static ForceMissionCommand g_ForceMissionCommand{};

ForceMissionCommand::ForceMissionCommand()
{
    setName("force_mission");
}

bool ForceMissionCommand::process_cmd(const std::vector<std::string> &a1)
{
    if ( a1.size() == 3 )
    {
        auto &v2 = a1.at(2);
        if ( is_int_format(v2.c_str()) )
        {
            mString v3 {a1.at(2).c_str()};
            auto v19 = v3.to_int();
            mString v4 {a1.at(1).c_str()};
            auto *v16 = v4.c_str();
            mString v5 {a1.at(0).c_str()};
            auto v14 = v5.to_int();
            auto *v6 = mission_manager::s_inst;
            v6->force_mission(v14, v16, v19, nullptr);
        }
        else
        {
            auto &v7 = a1.at(2);
            auto *v20 = v7.c_str();
            auto &v8 = a1.at(1);
            auto *v17 = v8.c_str();
            mString v9 {a1.at(0).c_str()};
            auto v15 = v9.to_int();
            auto *v10 = mission_manager::s_inst;
            v10->force_mission(v15, v17, 0, v20);
        }
    }
    else
    {
        if ( a1.size() != 1 )
        {
            g_console->addToLog("force_mission <district name or global> <mission> <instance>");
            g_console->addToLog("force_mission <mission>");
            return false;
        }

        auto &v11 = a1.at(0);
        auto *v18 = v11.c_str();
        auto *v12 = mission_manager::s_inst;
        v12->force_mission(0, v18, 0, nullptr);
    }

    return true;
}

static PanelTestCommand g_PanelTestCommand{};

PanelTestCommand::PanelTestCommand()
{
    setName("panel_test");
}

bool PanelTestCommand::process_cmd(const std::vector<std::string> &args)
{
    if (args.size() != 1 || (args[0] != "left" && args[0] != "right"))
    {
        g_console->addToLog(helpText());
        return false;
    }

    constexpr uint32_t leftHash = 0x70EADB23u;
    constexpr uint32_t rightHash = 0x8EB50736u;
    resource_key key {};
    key.m_hash.source_hash_code = args[0] == "left" ? leftHash : rightHash;
    key.m_type = RESOURCE_KEY_TYPE_CUT_SCENE;

    auto *scene = reinterpret_cast<cut_scene *>(
        resource_manager::get_resource(key, nullptr, nullptr));
    if (scene == nullptr)
    {
        g_console->addToLog("panel_test: CUT resource is unavailable");
        return false;
    }

    cut_scene_player *player = g_cut_scene_player();
    if (player == nullptr || player->is_playing())
    {
        g_console->addToLog("panel_test: cut-scene player is unavailable or busy");
        return false;
    }

    player->play(scene);
    g_console->addToLog("panel_test: playing %s panel (0x%08X)",
                        args[0].c_str(), key.m_hash.source_hash_code);
    return true;
}

static ListDebugVariablesCommand g_ListDebugVariablesCommand{};

ListDebugVariablesCommand::ListDebugVariablesCommand()
{
    setName("dvarlist");
}

bool ListDebugVariablesCommand::process_cmd(const std::vector<std::string> &)
{
    g_console->addToLog("<-- DVars -->");
    for ( auto &v3 : g_dvars )
    {
        auto *v4 = v3.first.c_str();
        g_console->addToLog("  %s", v4);
    }

    return true;
}

static DebugVarCommand g_DebugVarCommand {};

DebugVarCommand::DebugVarCommand()
{
    setName("dvar");
}

bool DebugVarCommand::process_cmd(const std::vector<std::string> &a2)
{
    if ( a2.size() == 0 ) {
        return false;
    }

    auto &v3 = a2.at(0);
    auto a1 = g_dvars.find(mString {v3.c_str()});
    if ( a2.size() == 1 )
    {
        auto v4 = g_dvars.end();
        if ( a1 == v4 )
        {
            auto &v5 = a2.at(0);
            auto *v6 = v5.c_str();
            g_console->addToLog("Couldn't find debug variable %s.", v6);
        }
        else
        {
            auto &v7 = (*a1);
            auto *v16 = v7.second.c_str();
            auto &v8 = a2.at(0);
            auto *v9 = v8.c_str();
            g_console->addToLog("%s -> %s", v9, v16);
        }

        return true;
    }
    else if ( a2.size() == 2 )
    {
        auto v10 = g_dvars.end();
        if ( a1 == v10 )
        {
            auto &v17 = a2.at(1);
            auto &v11 = a2.at(0);
            std::pair<mString, mString> v20 {mString {v11.c_str()}, mString {v17.c_str()} };
            g_dvars.insert(v20);
        }
        else
        {
            auto &v18 = a2.at(1);
            auto &v12 = (*a1);
            v12.second = mString {v18.c_str()};
        }

        auto &v13 = a2.at(1);
        auto *v19 = v13.c_str();
        auto &v14 = a2.at(0);
        auto *v15 = v14.c_str();
        g_console->addToLog("%s -> %s", v15, v19);
        return true;
    }
    else
    {
        return false;
    }
}

static ListMissionsCommand g_ListMissionsCommand{};

ListMissionsCommand::ListMissionsCommand()
{
    setName("list_missions");
}

bool ListMissionsCommand::process_cmd(const std::vector<std::string> &)
{
    auto *v2 = mission_manager::s_inst;
    auto v25 = v2->get_district_table_count();
    for ( auto i = -1; i < v25; ++i )
    {
        auto [v20, v21, v19] = [&v2](int i)
            -> std::tuple<fixedstring<8>, mission_table_container *, int>
        {
            if ( i != -1 )
            {
                auto *v21 = v2->get_district_table(i);
                auto *reg = v21->get_region();
                auto &v6 = reg->get_name();
                auto v19 = reg->get_district_id();

                return std::make_tuple(v6, v21, v19);
            }

            auto *v21 = v2->get_global_table();
            return std::make_tuple(fixedstring<8>{"GLOBAL"}, v21, 0);
        }(i);

        _std::vector<mission_table_container::script_info> v23;
        v21->append_script_info(&v23);
        for ( auto &info : v23 )
        {
            auto v14 = info.field_8;
            auto v12 = v19;
            auto *v11 = v20.to_string();
            g_console->addToLog("Name: '%s' District: '%s'(%d) Instance: %d",
                    info.field_0,
                    v11,
                    v12,
                    v14);
        }
    }

    return true;
}

static DebugRenderCommand g_DebugRenderCommand{};

DebugRenderCommand::DebugRenderCommand()
{
    this->setName("render");
}

bool DebugRenderCommand::process_cmd(const std::vector<std::string> &a2)
{
    int result;
    auto v17 = a2.size();
    if ( v17 != 0 )
    {
        auto &v4 = a2.at(0);
        mString v15 {v4.c_str()};
        v15.to_upper();
        int v14 = -1;
        for ( auto i = 0; i < 51; ++i )
        {
            if ( v15 == debug_render_items_names()[i] )
            {
                v14 = i;
            }
        }

        if ( v14 == -1 )
        {
            goto LABEL_20;
        }

        if ( v17 == 1 )
        {
            auto &v9 = debug_render_items()[v14];
            auto *v5 = debug_render_items_names()[v14].c_str();
            g_console->addToLog("%s %d", v5, v9);
            return true;
        }

        if ( v17 != 2 )
        {
            LABEL_20:
            g_console->addToLog("Bad syntax or unknown render flag!");
            result = true;
        }
        else
        {
            auto &v6 = a2.at(1);
            auto *v7 = v6.c_str();
            auto v12 = std::atoi(v7);
            auto v11 = debug_render_get_min((debug_render_items_e) v14);
            auto v10 = debug_render_get_max((debug_render_items_e) v14);
            if ( v12 >= v11 && v12 <= v10 )
                debug_render_items()[v14] = v12;
            else
                g_console->addToLog("Value out of range. Should be in [%d, %d]", v11, v10);

            result = true;
        }
    }
    else
    {
        for ( auto j = 0; j < 51; ++j )
        {
            auto &v8 = debug_render_items()[j];
            auto *v2 = debug_render_items_names()[j].c_str();
            g_console->addToLog("%s %d", v2, v8);
        }

        result = true;
    }

    return result;
}

static PlayAnimCommand g_PlayAnimCommand{};

PlayAnimCommand::PlayAnimCommand()
{
    this->setName("play_anim");
}

bool PlayAnimCommand::process_cmd(const std::vector<std::string> &a2)
{
    if (a2.size() == 1 || a2.size() == 2 )
    {
        auto &v3 = a2.at(0);
        mString a1 {v3.c_str()};
        filespec v35 {a1};
        v35.m_ext = resource_key_type_ext[g_platform][RESOURCE_KEY_TYPE_ANIMATION];
        actor *v33 = nullptr;
        if ( a2.size() == 2 )
        {
            auto &v5 = a2.at(1);
            mString v32 {v5.c_str()};
            v32.to_upper();

            auto *v6 = v32.c_str();
            string_hash v31 {v6};
            auto *ent = entity_handle_manager::find_entity(v31, IGNORE_FLAVOR, true);
            v33 = (actor *) ent;
            if ( v33 == nullptr )
            {
                g_console->addToLog("Entity not found");
                return 1;
            }
        }
        else
        {
#if 0
            j_selection_manager::get(g_selection_mgr, (int)&v30, 0);
            if ( sub_6784B6(&v30) != 5 )
            {
                g_console->addToLog("No entity selected");
                return 1;
            }

            v33 = (actor *)sub_676BA2(&v30);
#else
            assert(0);
#endif
        }

        auto *v29 = v33;
        if ( v33 != nullptr && !v33->is_an_actor() )
        {
            g_console->addToLog("The selected entity is not an actor.");
            return 1;
        }

#if 0
        bool v28 = false;
        auto v15 = v35.m_name + v35.m_ext;
        auto *v9 = v15.c_str();
        auto v27 = create_resource_key_from_path(v9, RESOURCE_KEY_TYPE_NONE);
        auto *resource = resource_manager::get_resource(v27, nullptr, nullptr);
        v28 = resource != nullptr;
        if ( resource == nullptr )
        {
            filespec v26 {a1};
            auto *v11 = v26.m_name.c_str();
            auto v25 = create_resource_key_from_path(v11, RESOURCE_KEY_TYPE_ANIMATION);
            auto *v12 = v29->field_BC;
            resource_manager::push_resource_context(v12);
            v28 = resource_manager::get_resource(v25, nullptr, nullptr) != nullptr;
            resource_manager::pop_resource_context();
            if ( !v28 )
            {
                g_console->addToLog("Entity animation not in packfile.");
                return 1;
            }
        }

        if ( !v33->is_an_actor() )
        {
            g_console->addToLog("Invalid entity (must be an actor).");
            return 1;
        }
#else
        {
            auto *pack_slot = v33->get_resource_context();
            if (pack_slot != nullptr)
            {
                auto &res_dir = pack_slot->get_resource_directory();
                auto tlresource_count = res_dir.get_tlresource_count(TLRESOURCE_TYPE_ANIM_FILE);
                assert(tlresource_count == 1);

                const auto idx = 0;
                auto *tlres_loc = res_dir.get_tlresource_location(idx, TLRESOURCE_TYPE_ANIM_FILE);
                nalAnimFile *anim_file = CAST(anim_file, tlres_loc->field_8);
                assert(anim_file->field_0 == 0x10101);

                nalAnimClass<nalAnyPose> *found_anim = nullptr;
                for (auto *anim = bit_cast<nalAnimClass<nalAnyPose> *>(anim_file->field_34);
                        anim != nullptr;
                        anim = anim->field_4) {

                    if (a1 == mString {anim->field_8.to_string()}) {
                        found_anim = anim;
                    }

                    sp_log("%s", anim->field_8.to_string());
                }

                if (found_anim == nullptr)
                {
                    g_console->addToLog("Entity animation not in packfile.");
                    return 1;
                }

                a1 = mString {found_anim->field_8.to_string()};
            }
        }

#endif

        auto *v13 = v29->get_resource_context();
        resource_manager::push_resource_context(v13);
        string_hash v16 {a1.c_str()};

        auto v24 = v29->play_anim(v16);
        resource_manager::pop_resource_context();

        v24.set_anim_speed(1.0);
    }
    else
    {
        auto *v2 = this->helpText();
        g_console->addToLog(v2);
    }

    return 1;
}

static ListNearbyEntsCommand g_ListNearbyEntsCommand{};

ListNearbyEntsCommand::ListNearbyEntsCommand() {
    this->setName("list_nearby_ents");
}

bool ListNearbyEntsCommand::process_cmd(const std::vector<std::string> &a2)
{
    auto *v3 = g_world_ptr->get_hero_ptr(0);
    auto &abs_position = v3->get_abs_position();
    auto a3 = abs_position;
    float v25 = 10.0;
    if ( a2.size() == 1 )
    {
        mString v5 {a2.at(0).c_str()};
        v25 = v5.to_float();
    }


    using map = std::decay_t<decltype(entity_handle_manager::the_map())>;
    auto &the_map = entity_handle_manager::the_map();
    using iterator = map::iterator;

    iterator v24 = the_map.begin();
    iterator end = the_map.end();
    for ( ; v24 != end; ++v24 )
    {
        auto &v7 = (*v24);
        auto *v23 = v7.second;
        auto &v8 = v23->get_abs_position();
        auto a2a = v8;
        auto v9 = a2a - a3;
        auto v21 = v9.length();
        if ( v25 >= v21 )
        {
            auto v16 = a2a.z;
            auto v15 = a2a.y;
            auto v14 = a2a.x;
            auto v13 = v21;
            auto id = v23->get_id();
            auto *v11 = id.to_string();
            g_console->addToLog("%s at %.2f meters (%.1f, %.1f, %.1f)", v11, v13, v14, v15, v16);
        }
    }

    //g_console->addToLog("ENTRY BLOCKS = %d, HASH BLOCKS = %d", dword_1585078, stru_158AC0C.field_8);
    return true;
}

static DumpThreadsCommand g_DumpThreadsCommand{};

DumpThreadsCommand::DumpThreadsCommand() {
    this->setName("dump_threads");
}

bool DumpThreadsCommand::process_cmd(const std::vector<std::string> &a2)
{
    if ( a2.size() != 0 )
    {
        auto &v2 = a2.at(0);
        if ( v2 != "1" ) {
            return false;
        }

        g_console->addToLog("Threads dumped to file.");
        script_manager::dump_threads_to_file();
    }
    else
    {
        g_console->addToLog(mString::null());
        script_manager::dump_threads_to_console();
    }
    
    return true; 
}

entity_base * sub_65F164(std::string arg0, string_hash a1, po &a3)
{
    filespec v25 {arg0.c_str()};
    if ( v25.m_ext.length() <= 0 ) {
        v25.m_ext = ".ent";
    }

    if ( v25.m_dir.length() <= 0 ) {
        v25.m_dir = "characters\\" + v25.m_name + "\\";
    }

    mString a7 {};
    string_hash v11 = a1;
    string_hash v10 {v25.m_name.c_str()};
    auto *v26 = g_world_ptr->ent_mgr.create_and_add_entity_or_subclass(
            v10,
            v11,
            a3,
            a7,
            129,
            nullptr);

    if ( v26 != nullptr )
    {
        auto v17 = v26->get_flavor();
        if ( v17 == ENTITY_ITEM )
        {
            a3 = {identity_matrix};
            auto &abs_pos = v26->get_abs_position();
            a3.set_position(abs_pos);

            v26->set_abs_po(a3);
        }

        v26->set_visible(true, false);
        //sub_6959DA(&v26);
        //sub_65A632(g_selection_mgr, v26, (selection_slot)0);
        auto *the_terrain = g_world_ptr->get_the_terrain();
        v26->compute_sector(the_terrain, 0, v26);

        if ( v26->has_damage_ifc() )
        {
            auto *v7 = v26->damage_ifc();
            auto v13 = v7->field_1FC.field_0[2];
            auto *v8 = v26->damage_ifc();
            v8->field_1FC.sub_48BFB0(v13);
        }
    }

    return v26;
}

static SpawnCommand g_SpawnCommand{};

bool SpawnCommand::process_cmd(const std::vector<std::string> &a2)
{
    if ( a2.size() != 0 )
    {
        auto &v5 = a2.at(0);
        mString v70 {v5.c_str()};
        v70.to_lower();

        mString v69 = v70;
        if ( a2.size() <= 1 )
        {
            g_console->addToLog("Need to pass an entity ID");
        }
        else
        {
            auto &v6 = a2.at(1);
            mString entity_id {v6.c_str()};
            entity_id.to_upper();

            auto *v7 = entity_id.c_str();
            auto *ent = g_world_ptr->ent_mgr.get_entity(string_hash {v7});
            if ( ent == nullptr )
            {
                auto *v9 = entity_id.c_str();
                string_hash id {v9};
                vector3d v67 = ZEROVEC;

                if ( a2.size() <= 4 )
                {
                    if ( a2.size() > 2 )
                    {
                        auto &v16 = a2.at(2);
                        auto *v17 = v16.c_str();
                        v67[2] = atof(v17);
                    }
                }
                else
                {
                    auto &v10 = a2.at(2);
                    auto *v11 = v10.c_str();
                    v67[0] = atof(v11);

                    auto &v12 = a2.at(3);
                    auto *v13 = v12.c_str();
                    v67[1] = atof(v13);

                    auto &v14 = a2.at(4);
                    auto *v15 = v14.c_str();
                    v67[2] = atof(v15);
                }

                po v66 {identity_matrix};
                auto *v19 = g_world_ptr->get_hero_ptr(0);
                v66 = v19->get_abs_po();

                auto *v22 = g_world_ptr->get_hero_ptr(0);
                auto &v23 = v22->get_abs_po();
                vector3d v43 = v23.non_affine_slow_xform(v67);
                auto *v25 = g_world_ptr->get_hero_ptr(0);
                auto abs_pos = v25->get_abs_position();
                vector3d v27 = abs_pos + v43;
                v66.set_position(v27);

                vector3d v65 {};
                vector3d v64 {};

                vector3d pos = v66.get_position();
                bool v30 = find_sphere_intersection(
                            pos,
                            0.5f,
                            *local_collision::entfilter_accept_all,
                            *local_collision::obbfilter_sphere_test,
                            &v65,
                            &v64,
                            nullptr,
                            nullptr);

                if ( v30 )
                {
                    vector3d v43 {0.0, 0.1, 0.0};
                    auto *v32 = g_world_ptr->get_hero_ptr(0);
                    vector3d v33 = v32->get_abs_position();
                    vector3d v34 = v33 + v43;
                    v66.set_position(v34);
                }

                std::string v41 {v69.c_str()};
                auto *v46 = sub_65F164(v41, id, v66);
                if ( v46 != nullptr )
                {
                    auto *v35 = id.to_string();
                    g_console->addToLog("Spawned entity '%s'", v35);
                }
                else
                {
                    auto *v36 = id.to_string();
                    g_console->addToLog("Failed to spawn entity '%s'. Try a different entity file/type", v36);
                }
            }
            else
            {
                auto &v37 = a2.at(1);
                auto *v38 = v37.c_str();
                g_console->addToLog("Entity ID '%s' is already in use", v38);
            }
        }
    }
    else
    {
        g_console->addToLog("Need to pass an entity type and entity ID or a command");
    }

    return true;
}

static SpawnXCommand g_SpawnXCommand{};

bool SpawnXCommand::process_cmd(const std::vector<std::string> &a2)
{
    TRACE("SpawnXCommand::process_cmd");

    if ( a2.size() != 0 )
    {
        auto &v5 = a2.at(0);
        mString v75 {v5.c_str()};
        v75.to_lower();

        const mString v74 = v75;

        vector3d v73 = ZEROVEC;
        if ( a2.size() <= 3 )
        {
            if ( a2.size() > 1 )
            {
                auto &v12 = a2.at(1);
                auto *v13 = v12.c_str();
                v73[2] = atof(v13);
            }
        }
        else
        {
            auto &v6 = a2.at(1);
            auto *v7 = v6.c_str();
            v73[0] = atof(v7);

            auto &v8 = a2.at(2);
            auto *v9 = v8.c_str();
            v73[1] = atof(v9);

            auto &v10 = a2.at(3);
            auto *v11 = v10.c_str();
            v73[2] = atof(v11);
        }

        po v72 {identity_matrix};
        auto *hero_ptr = g_world_ptr->get_hero_ptr(0);

        v72 = hero_ptr->get_abs_po();
        auto &v19 = hero_ptr->get_abs_po();
        auto v46 = v19.non_affine_slow_xform(v73);
        auto &abs_pos = hero_ptr->get_abs_position();
        auto v23 = abs_pos + v46;
        v72.set_position(v23);

        vector3d impact_pos {};
        vector3d impact_normal {};
        vector3d pos = v72.get_position();
        if ( find_sphere_intersection(
                pos,
                0.5,
                *local_collision::entfilter_accept_all,
                *local_collision::obbfilter_sphere_test,
                &impact_pos,
                &impact_normal,
                nullptr,
                nullptr) )
        {
            vector3d v46 {0.0, 0.1, 0.0};
            auto *v26 = g_world_ptr->get_hero_ptr(0);
            vector3d v27 = v26->get_abs_position();
            auto v28 = v27 + v46;
            v72.set_position(v28);
        }

        filespec v68 {v74};
        if ( v68.m_ext.length() <= 0 ) {
            v68.m_ext = ".ent";
        }

        if ( v68.m_dir.length() <= 0 ) {
            v68.m_dir = "characters\\" + v68.m_name + "\\";
        }

        sp_log("%s", v68.fullname().c_str());

        mString v63 {};
        auto v42 = make_unique_entity_id();
        string_hash v41 {v68.m_name.c_str()};
        auto *new_ent = g_world_ptr->ent_mgr.create_and_add_entity_or_subclass(
                v41,
                v42,
                v72,
                v63,
                1,
                nullptr);
        if ( new_ent != nullptr )
        {
            if ( auto v51 = new_ent->get_flavor();
                    v51 == ENTITY_ITEM )
            {
                v72 = {identity_matrix};
                auto &v31 = new_ent->get_abs_position();
                v72.set_position(v31);
                new_ent->set_abs_po(v72);
            }

            new_ent->set_visible(true, false);
            if ( new_ent->has_damage_ifc() )
            {
                auto *v33 = new_ent->damage_ifc();

                auto v13 = v33->field_1FC.field_0[2];
                v33->field_1FC.sub_48BFB0(v13);
            }

            //v79 = 0;
            //v35 = sub_694A7B(&v69);
            //sub_65A632(g_selection_mgr, &v35->base.base, (selection_slot)v79);

            auto id = new_ent->get_id();
            auto *v79 = id.to_string();
            auto *v38 = v74.c_str();
            g_console->addToLog("Spawned entity '%s' (ent_id = '%s')", v38, v79);
        }
        else
        {
            auto *v39 = v74.c_str();
            g_console->addToLog("Failed to spawn entity '%s'. Try a different entity file/type", v39);
        }
    }
    else
    {
        g_console->addToLog("Need to pass an entity type");
    }

    return true;
}

static SetPBFloatCommand g_SetPBFloatCommand {};

SetPBFloatCommand::SetPBFloatCommand() {
    this->setName("set_ai");
}

bool SetPBFloatCommand::process_cmd(const std::vector<std::string> &a2)
{
    auto v46 = a2.size();
    if ( v46 == 2 || v46 == 3 )
    {
        auto &v2 = a2.at(0);
        auto *v3 = v2.c_str();
        string_hash a2a {v3};
        auto *ent = entity_handle_manager::find_entity(a2a, IGNORE_FLAVOR, true);
        if ( ent != nullptr )
        {
            if ( ent->is_an_actor() )
            {
                auto *the_actor = bit_cast<actor *>(ent);
                auto *v42 = the_actor->get_ai_core();
                if ( v42 != nullptr )
                {
                    auto &v8 = a2.at(1);
                    auto *v9 = v8.c_str();
                    string_hash v39 {v9};

                    auto *the_pblock = v42->get_param_block();
                    if ( the_pblock->does_parameter_exist(v39) )
                    {
                        auto pb_float = the_pblock->get_pb_float(v39);
                        if ( v46 == 2 )
                        {
                            auto &v14 = a2.at(1);
                            auto *v15 = v14.c_str();
                            g_console->addToLog("%s = %.2f", v15, pb_float);
                            return true;
                        }
                        else
                        {
                            auto &v16 = a2.at(2);
                            auto *v17 = v16.c_str();
                            auto v38 = atof(v17);
                            the_pblock->set_pb_float(v39, v38, false);

                            auto &v19 = a2.at(1);
                            auto *v20 = v19.c_str();
                            g_console->addToLog("%s was %.2f, now set to %.2f", v20, pb_float);
                            return true;
                        }
                    }
                    else
                    {
                        auto &v12 = a2.at(1);
                        auto *v13 = v12.c_str();
                        g_console->addToLog("Parameter %s does not exist in the param block", v13);
                        return true;
                    }
                }
                else
                {
                    g_console->addToLog("This actor doesn't have an AI core!");
                    return true;
                }
            }
            else
            {
                g_console->addToLog("This entity is not an actor!");
                return true;
            }
        }
        else
        {
            auto &v5 = a2.at(0);
            auto *v6 = v5.c_str();
            g_console->addToLog("Entity %s not found!", v6);
            return true;
        }
    }
    else
    {
        auto *v21 = this->helpText();
        g_console->addToLog(v21);
        return true;
    }
}

static SCCommand g_SCCommand {};

bool SCCommand::process_cmd(const std::vector<std::string> &a2)
{
    auto *gso = script::get_gso();
    auto *gsoi = script::get_gsoi();
    if ( a2.size() != 1 ) {
        return false;
    }

    auto &v3 = a2.at(0);
    auto *v4 = v3.c_str();
    string_hash v7 {v4};
    int func_idx = gso->find_func_short(v7);
    if ( func_idx != -1 )
    {
        auto func = gso->get_func(func_idx);
        auto *v11 = gsoi->add_thread(func);
        if ( v11 == nullptr ) {
            return false;
        }

        gsoi->run_single_thread(v11, true);
    }
    return true;
}
