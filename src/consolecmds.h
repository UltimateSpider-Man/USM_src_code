#pragma once

#include <list>
#include <vector>

#include <string>

struct ConsoleCommand {
    char field_4[32];

    ConsoleCommand();

    virtual ~ConsoleCommand() = default;

    virtual bool process_cmd(const std::vector<std::string> &);

    virtual const char *helpText() const {
        return "No help available.";
    }

    std::string getName() const;

    void setName(const std::string &pName);

    bool match(const std::string &a2) const;
};

extern std::list<ConsoleCommand *> *g_console_cmds;

struct ExecCommand : ConsoleCommand {
    virtual bool process_cmd(const std::vector<std::string> &a1) override;

    virtual const char *helpText() const override {
        return "exec [file] -> Executes a console script file";
    }
};

struct HelpCommand : ConsoleCommand {
    HelpCommand();

    virtual bool process_cmd(const std::vector<std::string> &a1) override;

    virtual const char *helpText() const override {
        return "Retrieves help for console / command";
    }
};

struct ListEntsCommand : ConsoleCommand {
    ListEntsCommand();

    bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override {
        return "List visible entities";
    }
};

struct LoadLevelCommand : ConsoleCommand {
    LoadLevelCommand();

    virtual bool process_cmd(const std::vector<std::string> &a1) override;

    virtual const char *helpText() const override {
        return "Loads a new level (nuking the current one)";
    }
};

struct VariableList : ConsoleCommand {
    VariableList();

    bool process_cmd(const std::vector<std::string> &) override;

    const char *helpText() const override {
        return "Lists all available variables";
    }
};

struct SetCommand : ConsoleCommand {
    SetCommand();

    bool process_cmd(const std::vector<std::string> &a2) override;

    const char *helpText() const override {
        return "set [name] [val] -> Sets the value of a variable";
    }
};

struct GetCommand : ConsoleCommand {
    GetCommand();

    bool process_cmd(const std::vector<std::string> &a2) override;

    const char *helpText() const override {
        return "get [name] -> Gets the value of a variable";
    }
};

struct GameStateCommand : ConsoleCommand {
    GameStateCommand();

    bool process_cmd(const std::vector<std::string> &cmds) override;

    const char *helpText() const override {
        return "get / set game_state variables";
    }
};

struct GameInfoCommand : ConsoleCommand {
    GameInfoCommand() {
        setName("game_info");
    }

    bool process_cmd(const std::vector<std::string> &cmds) override;

    const char *helpText() const override {
        return "get / set game_info nums";
    }
};

struct QuitCommand : ConsoleCommand {
    QuitCommand();

    bool process_cmd(const std::vector<std::string> &) override;

    const char *helpText() const override {
        return "quit -> exit program.  good for test scripts.";
    }
};

struct CommandList : ConsoleCommand {
    CommandList();

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override {
        return "Lists all available commands";
    }
};

struct ForceMissionCommand : ConsoleCommand {
    ForceMissionCommand();

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override
    {
        return "force_mission of given district, name, and instance";
    }
};

// Developer-only playback seam for the loose CITY_CAR_COMBAT comic panels.
// It exercises the same resource lookup and stock cut-scene player used by
// gameplay scripts without changing mission state or save data.
struct PanelTestCommand : ConsoleCommand {
    PanelTestCommand();

    bool process_cmd(const std::vector<std::string> &args) override;

    const char *helpText() const override {
        return "panel_test <left|right> -> plays the validated gun comic panel";
    }
};

struct ListDebugVariablesCommand : ConsoleCommand
{
    ListDebugVariablesCommand();

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override
    {
        return "Lists all available debug variables";
    } 
};

struct DebugVarCommand : ConsoleCommand
{
    DebugVarCommand();

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override
    {
        return "dvar <var> <value> -> Set a debug variable\n dvar <var> -> View a debug variable";
    } 
};

struct ListMissionsCommand : ConsoleCommand
{
    ListMissionsCommand();

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override
    {
        return "list_missions currently available";
    }
        
};

struct DebugRenderCommand : ConsoleCommand
{
    DebugRenderCommand();

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override {
        return "render <flag> <value>";
    }
};

struct PlayAnimCommand : ConsoleCommand
{
    PlayAnimCommand();

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override {
        return "play_anim <anim_name> [<entity_id>]";
    }
};

struct ListNearbyEntsCommand : ConsoleCommand
{
    ListNearbyEntsCommand();

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override {
        return "List nearby entities <radius=10>";
    }
};

struct DumpThreadsCommand : ConsoleCommand {
    DumpThreadsCommand();

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override {
        return "Dumps script threads to the console (pass a '1' to dump to a file)";
    }
};

struct SpawnCommand : ConsoleCommand {
    SpawnCommand() {
        this->setName("spawn");
    }

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override {
        return "Spawns an object at hero position: spawn <object name> <entity ID> [distance]";
    }
};

struct SpawnXCommand : ConsoleCommand {
    SpawnXCommand() {
        this->setName("spawnx");
    }

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override {
        return "Spawns an object at hero position (with pickup abilities): spawnx <object name> [distance]";
    }
};

struct SetPBFloatCommand : ConsoleCommand {
    SetPBFloatCommand();

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override {
        return "set_ai <EntityName> <ParamName> [<ParamValue>]";
    }
};

struct SCCommand : ConsoleCommand {

    SCCommand() {
        this->setName("sc");
    }

    virtual bool process_cmd(const std::vector<std::string> &) override;

    virtual const char *helpText() const override {
        return "sc <function> [args] ...";
    }
};
