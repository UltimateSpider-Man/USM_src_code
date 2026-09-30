param(
    [string]$Compiler = 'C:/msys64/mingw32/bin/g++.exe',
    [string]$BuildDirectory = 'build/multiplayer-pack-check'
)

# Execute the production context acquisition paths against fake resource slots.
# The game stores the active hero outside the mission partition, so testing only
# candidate strings would miss the regression this probe protects.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$output = Join-Path $repo $BuildDirectory
[void](New-Item -ItemType Directory -Force -Path $output)
$env:PATH = (Split-Path -Parent $Compiler) + ';' + $env:PATH

function Extract-Function([string]$Source, [string]$Signature) {
    $start = $Source.IndexOf($Signature)
    if ($start -lt 0) { throw "Production function missing: $Signature" }
    $open = $Source.IndexOf('{', $start)
    $depth = 0
    for ($at = $open; $at -lt $Source.Length; ++$at) {
        if ($Source[$at] -eq '{') { ++$depth }
        if ($Source[$at] -eq '}') {
            --$depth
            if ($depth -eq 0) { return $Source.Substring($start, $at - $start + 1) }
        }
    }
    throw "Production function end missing: $Signature"
}

$assets = [IO.File]::ReadAllText((Join-Path $repo 'src/multiplayer_assets.cpp'))
$online = [IO.File]::ReadAllText((Join-Path $repo 'src/multiplayer_online_engine.cpp'))
$lookup = [IO.File]::ReadAllText((Join-Path $repo 'src/multiplayer_pack_lookup.h'))
$lookup = [regex]::Replace($lookup, '(?m)^\s*#(?:include|pragma)[^\r\n]*', '')
$arenaAcquire = Extract-Function $assets 'resource_pack_slot *context_for('
$onlineAcquire = Extract-Function $online 'resource_pack_slot* acquire('
$retainOwned = Extract-Function $assets 'void retain_owned('

$prefix = @'
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

std::string normalize(std::string value) {
    for (auto& c : value) c = char(std::tolower(static_cast<unsigned char>(c)));
    return value;
}
struct string_hash {
    std::string text;
    string_hash(const char* value) : text(normalize(value)) {}
};
enum { RESOURCE_KEY_TYPE_ENTITY = 4, RESOURCE_KEY_TYPE_PACK = 25 };
enum resource_partition_enum {
    RESOURCE_PARTITION_START, RESOURCE_PARTITION_HERO, RESOURCE_PARTITION_LANG,
    RESOURCE_PARTITION_MISSION, RESOURCE_PARTITION_COMMON, RESOURCE_PARTITION_STRIP,
    RESOURCE_PARTITION_DISTRICT, RESOURCE_PARTITION_STANDALONE, RESOURCE_PARTITION_END
};
struct resource_key {
    string_hash m_hash;
    int m_type;
    bool operator==(const resource_key& rhs) const {
        return m_hash.text == rhs.m_hash.text && m_type == rhs.m_type;
    }
    bool operator!=(const resource_key& rhs) const { return !(*this == rhs); }
};
struct mString {
    std::string value;
    mString(const char* text) : value(text) {}
    const char* c_str() const { return value.c_str(); }
};
struct resource_partition;
struct resource_pack_slot {
    resource_key key{{""}, RESOURCE_KEY_TYPE_PACK};
    std::string entity;
    resource_pack_slot* resource_owner = nullptr;
    resource_partition* partition = nullptr;
    bool ready = true;
    int bytes = 512;
    std::uint8_t data[16]{};
    const resource_key& get_name_key() const { return key; }
    bool is_pack_ready() const { return ready; }
    resource_partition* get_partition() const { return partition; }
    std::uint8_t* get_resource(const resource_key& wanted, int* size, resource_pack_slot** owner) {
        if (wanted.m_type != RESOURCE_KEY_TYPE_ENTITY || wanted.m_hash.text != normalize(entity)) return nullptr;
        if (size) *size = bytes;
        if (owner) *owner = resource_owner ? resource_owner : this;
        return data;
    }
};
struct resource_partition {
    resource_partition_enum type = RESOURCE_PARTITION_MISSION;
    std::vector<resource_pack_slot*> slots;
    int partition_buffer_used = 0, partition_buffer_size = 4096;
    auto& get_pack_slots() { return slots; }
    auto get_type() const { return type; }
    bool has_room_for_slot(int bytes) const {
        return partition_buffer_used + bytes <= partition_buffer_size;
    }
};
struct resource_pack_location { struct { int m_size = 0; } loc; };
struct InstalledPack { std::string entity; int size = 1024; };
std::map<std::string, InstalledPack> installed;
std::array<resource_partition, RESOURCE_PARTITION_END> partition_storage;
std::vector<resource_partition*> partition_list;
std::vector<std::unique_ptr<resource_pack_slot>> slot_storage;
unsigned pushes = 0, pops = 0, stats = 0;
namespace resource_manager {
std::vector<resource_partition*>* partitions = &partition_list;
int table = 1;
int* amalgapak_pack_location_table = &table;
int amalgapak_base_offset = 0;
resource_partition* get_partition_pointer(resource_partition_enum which) {
    if (!partitions || unsigned(which) >= partitions->size()) return nullptr;
    return (*partitions)[which];
}
bool get_pack_file_stats(const resource_key& key, resource_pack_location* location, void*, void*) {
    ++stats;
    auto found = installed.find(key.m_hash.text);
    if (found == installed.end()) return false;
    if (location) location->loc.m_size = found->second.size;
    return true;
}
}
resource_pack_slot* add_slot(resource_partition_enum where, const char* pack, const char* entity) {
    auto slot = std::make_unique<resource_pack_slot>();
    slot->key = resource_key{string_hash{pack}, RESOURCE_KEY_TYPE_PACK};
    slot->entity = entity;
    slot->partition = &partition_storage[where];
    auto* result = slot.get();
    slot_storage.push_back(std::move(slot));
    partition_storage[where].slots.push_back(result);
    return result;
}
struct mission_stack_manager {
    static mission_stack_manager* s_inst;
    bool busy = false;
    bool waiting_for_push_or_pop() const { return busy; }
    bool is_pack_pushed(const mString& pack) const {
        for (auto* slot : partition_storage[RESOURCE_PARTITION_MISSION].slots)
            if (slot && slot->get_name_key().m_hash.text == normalize(pack.value)) return true;
        return false;
    }
    void push_mission_pack_immediate(const mString&, const mString& pack) {
        ++pushes;
        auto found = installed.find(normalize(pack.value));
        if (found == installed.end()) throw std::runtime_error("pushed nonexistent pack");
        add_slot(RESOURCE_PARTITION_MISSION, pack.c_str(), found->second.entity.c_str());
        partition_storage[RESOURCE_PARTITION_MISSION].partition_buffer_used += found->second.size;
    }
    void pop_mission_pack_immediate(const mString&, const mString& pack) {
        auto& slots = partition_storage[RESOURCE_PARTITION_MISSION].slots;
        if (slots.empty() || slots.back()->key.m_hash.text != normalize(pack.value))
            throw std::runtime_error("non-LIFO pack release");
        ++pops;
        auto found = installed.find(normalize(pack.value));
        if (found != installed.end())
            partition_storage[RESOURCE_PARTITION_MISSION].partition_buffer_used -= found->second.size;
        slots.pop_back();
    }
} stack;
mission_stack_manager* mission_stack_manager::s_inst = &stack;
std::vector<std::string> messages;
void log(const std::string& text) { messages.push_back(text); }
void release_owned(std::vector<std::string>& owned) {
    auto& slots = partition_storage[RESOURCE_PARTITION_MISSION].slots;
    while (!slots.empty() && !owned.empty()) {
        auto found = std::find_if(owned.begin(), owned.end(), [&](const auto& pack) {
            return normalize(pack) == slots.back()->get_name_key().m_hash.text;
        });
        if (found == owned.end()) return;
        mString name{found->c_str()};
        stack.pop_mission_pack_immediate(name, name);
        owned.erase(found);
    }
}
'@

$arenaPrefix = @'
namespace arena_probe {
using usm::mp::find_loaded_actor_pack_context;
struct Candidate { std::string pack, entity; };
std::vector<std::string> owned_packs;
void assets_retry_release() { release_owned(owned_packs); }
'@
$onlinePrefix = @'
namespace online_probe {
namespace mp = usm::mp;
struct Candidate { std::string pack, entity; };
std::vector<std::string> owned_packs;
void release_unused_packs() { release_owned(owned_packs); }
'@
$suffix = @'
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void reset() {
    arena_probe::owned_packs.clear(); online_probe::owned_packs.clear();
    installed.clear(); slot_storage.clear(); partition_list.clear(); messages.clear();
    for (unsigned n = 0; n < partition_storage.size(); ++n) {
        partition_storage[n] = resource_partition{};
        partition_storage[n].type = resource_partition_enum(n);
        partition_list.push_back(&partition_storage[n]);
    }
    resource_manager::partitions = &partition_list;
    resource_manager::amalgapak_pack_location_table = &resource_manager::table;
    resource_manager::amalgapak_base_offset = 0;
    mission_stack_manager::s_inst = &stack;
    stack.busy = false; pushes = pops = stats = 0;
}
template<class Acquire>
void run(const char* label, Acquire acquire, std::vector<std::string>& owned) {
    reset();
    auto* hero = add_slot(RESOURCE_PARTITION_HERO, "ultimate_spiderman", "ultimate_spiderman");
    partition_storage[RESOURCE_PARTITION_MISSION].partition_buffer_size = 0;
    require(acquire("ultimate_spiderman", "ultimate_spiderman") == hero,
            "loaded HERO pack is invisible when mission memory is full");
    require(pushes == 0 && pops == 0 && stats == 0 && owned.empty(),
            "borrowing loaded HERO pack performs mission IO or takes ownership");

    reset();
    auto* common = add_slot(RESOURCE_PARTITION_COMMON, "ultimate_spiderman", "ultimate_spiderman");
    resource_manager::amalgapak_pack_location_table = nullptr;
    resource_manager::amalgapak_base_offset = -1;
    require(acquire("ultimate_spiderman", "ultimate_spiderman") == common,
            "ready pack in a non-mission partition incorrectly requires archive metadata");
    require(pushes == 0 && pops == 0 && owned.empty(), "borrowed COMMON pack was owned");

    for (auto transient : {RESOURCE_PARTITION_DISTRICT, RESOURCE_PARTITION_STRIP}) {
        reset();
        add_slot(transient, "ultimate_spiderman", "ultimate_spiderman");
        require(acquire("ultimate_spiderman", "ultimate_spiderman") == nullptr,
                "fighter borrowed a transient streaming partition");
        require(pushes == 0 && pops == 0 && owned.empty(), "transient pack altered mission ownership");
    }

    for(auto transient : {RESOURCE_PARTITION_STRIP, RESOURCE_PARTITION_DISTRICT}) {
        reset();
        add_slot(transient, "ultimate_spiderman", "ultimate_spiderman");
        require(acquire("ultimate_spiderman", "ultimate_spiderman") == nullptr,
                "transient streamed pack was borrowed for a persistent actor");
        require(owned.empty(), "transient pack changed ownership");
    }

    reset();
    auto* parent = add_slot(RESOURCE_PARTITION_HERO, "other_pack", "ultimate_spiderman");
    auto* inherited = add_slot(RESOURCE_PARTITION_MISSION, "ultimate_spiderman", "ultimate_spiderman");
    inherited->resource_owner = parent;
    require(acquire("ultimate_spiderman", "ultimate_spiderman") == nullptr,
            "same-named entity from a different pack was accepted");
    require(pushes == 0 && pops == 0 && owned.empty(), "failed borrowed pack altered stack");

    reset();
    auto* unloaded = add_slot(RESOURCE_PARTITION_HERO, "ultimate_spiderman", "ultimate_spiderman");
    unloaded->ready = false;
    require(acquire("ultimate_spiderman", "ultimate_spiderman") == nullptr,
            "unready hero resource was accepted");

    reset();
    auto* small = add_slot(RESOURCE_PARTITION_HERO, "ultimate_spiderman", "ultimate_spiderman");
    small->bytes = 8;
    require(acquire("ultimate_spiderman", "ultimate_spiderman") == nullptr,
            "truncated entity resource was accepted");

    reset();
    auto* mission = add_slot(RESOURCE_PARTITION_MISSION, "venom_spider", "venom_spider");
    require(acquire("venom_spider", "venom_spider") == mission,
            "borrowed mission actor resource was rejected");
    require(pushes == 0 && pops == 0 && owned.empty(), "borrowed mission pack became owned");

    reset();
    installed["venom_spider"] = {"venom_spider", 1024};
    auto* first = acquire("venom_spider", "venom_spider");
    require(first && pushes == 1 && pops == 0 && owned.size() == 1,
            "newly loaded mission pack was not retained exactly once");
    require(acquire("venom_spider", "venom_spider") == first,
            "second fighter did not share the already loaded exact pack");
    require(pushes == 1 && pops == 0 && owned.size() == 1,
            "second fighter duplicated the pack or its ownership");

    reset();
    require(acquire("missing_pack", "missing_entity") == nullptr,
            "absent archive pack was accepted");
    require(pushes == 0 && pops == 0 && owned.empty(), "missing pack changed mission stack");

    reset();
    installed["venom_spider"] = {"wrong_entity", 1024};
    require(acquire("venom_spider", "venom_spider") == nullptr,
            "missing exact entity was accepted");
    require(pushes == 1, "missing entity fixture never reached pack validation");

    reset();
    installed["venom_spider"] = {"venom_spider", 1024};
    partition_storage[RESOURCE_PARTITION_MISSION].partition_buffer_size = 512;
    require(acquire("venom_spider", "venom_spider") == nullptr,
            "new pack overfilled the mission partition");
    require(pushes == 0 && pops == 0 && owned.empty(), "capacity rejection evicted native packs");
    std::printf("PASS %s: loaded HERO/COMMON, transient partitions rejected, exact owner, ready/size guards, borrowed MISSION, shared ownership, missing pack/entity, capacity\n", label);
}
int main() {
    try {
        run("arena context_for", [](const char* p, const char* e) {
            return arena_probe::context_for({p, e});
        }, arena_probe::owned_packs);
        run("online acquire", [](const char* p, const char* e) {
            return online_probe::acquire({p, e});
        }, online_probe::owned_packs);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what()); return 1;
    }
}
'@

$cpp = Join-Path $output 'multiplayer_pack_loading.cpp'
$exe = Join-Path $output 'multiplayer_pack_loading.exe'
$code = $prefix + "`n" + $lookup + "`n" + $arenaPrefix + "`n" + $retainOwned + "`n" + $arenaAcquire + "`n}`n" + $onlinePrefix + "`n" + $onlineAcquire + "`n}`n" + $suffix
[IO.File]::WriteAllText($cpp, $code)
& $Compiler '-std=c++17' '-O0' '-static' $cpp '-o' $exe
if ($LASTEXITCODE -ne 0) { throw "Multiplayer pack probe compilation failed: $LASTEXITCODE" }
& $exe
if ($LASTEXITCODE -ne 0) { throw "Multiplayer pack check failed: $LASTEXITCODE" }
