param(
    [string]$Compiler = 'C:/msys64/mingw32/bin/g++.exe',
    [string]$BuildDirectory = 'build/extra-pcmesh-check'
)

# Compile the actual loader traversal/registration and native image detector,
# isolated from the game's fixed-address entry points. No installed files are used.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$output = Join-Path $repo $BuildDirectory
[void](New-Item -ItemType Directory -Force -Path $output)
$env:PATH = (Split-Path -Parent $Compiler) + ';' + $env:PATH
$ngl = [IO.File]::ReadAllText((Join-Path $repo 'src/ngl.cpp'))
$detectorStart = $ngl.IndexOf('#pragma pack(push, 1)', $ngl.IndexOf('// Raw PCMESH detection / registry access.'))
$detectorEnd = $ngl.IndexOf('bool modPCMESHImageUsable(', $detectorStart)
if ($detectorStart -lt 0 -or $detectorEnd -lt 0) { throw 'Native PCMESH detector was not found' }
$detector = $ngl.Substring($detectorStart, $detectorEnd - $detectorStart)
$hashSource = [IO.File]::ReadAllText((Join-Path $repo 'src/string_hash.h'))
$hashStart = $hashSource.IndexOf('inline constexpr bool is_alpha(')
if ($hashStart -lt 0) { $hashStart = $hashSource.IndexOf('constexpr bool is_alpha(') }
if ($hashStart -lt 0) { throw 'Engine hash helpers were not found' }
$hashCode = $hashSource.Substring($hashStart, $hashSource.IndexOf('extern void string_hash_patch();') - $hashStart)

$prefix = @'
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs = std::filesystem;
constexpr int TLRESOURCE_TYPE_NONE = 0, TLRESOURCE_TYPE_MESH_FILE = 2;
constexpr uint32_t MOD_PCMESH_VERSION = 0x601;
enum class TypeDirectoryEntry { MATERIAL = 1, MESH = 2, MORPH = 3 };
struct Mod { fs::path Path; int Type; std::vector<uint8_t> Data; };
std::multimap<uint32_t, Mod> Mods;
std::vector<fs::path> roots;
const std::vector<fs::path>& modRootDirs() { return roots; }
std::string transformToLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}
std::vector<uint8_t> read_file(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
}
'@
$suffix = @'
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void put(std::vector<uint8_t>& bytes, size_t at, uint32_t value) {
    std::memcpy(bytes.data() + at, &value, 4);
}
void save(const fs::path& path, const std::vector<uint8_t>& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    require(bool(output), "fixture write failed");
}
const Mod* find(const std::string& key) {
    auto range = Mods.equal_range(to_hash(key.c_str()));
    for (auto it = range.first; it != range.second; ++it)
        if (it->second.Type == TLRESOURCE_TYPE_MESH_FILE) return &it->second;
    return nullptr;
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected fixture directory");
        const fs::path base = fs::absolute(argv[1]);
        roots = {base / "mods", base / "extra"};
        // One pristine PC 0x601 mesh, a directory entry and an inline name.
        std::vector<uint8_t> valid(0x80);
        std::memcpy(valid.data(), "PCM ", 4);
        put(valid, 4, 0x601); put(valid, 8, 1); put(valid, 12, 0x14);
        put(valid, 0x14, 0x02000040); put(valid, 0x18, 0x20);
        put(valid, 0x1c, 0x60); put(valid, 0x20, 0x60);
        std::memcpy(valid.data() + 0x64, "fixture000", 11);
        require(modPCMESHDetectTLType(valid.data(), valid.size(), 2) == 2,
                "fixture is not a pristine native image");
        save(base / "extra" / "Nested" / "Hero.PcMeSh", valid);
        save(base / "extra" / "Hash" / "0x12345678.PCMESH", valid);
        save(base / "mods" / "Legacy.PCMESH", valid);
        save(base / "mods" / "Priority.PCMESH", valid);
        save(base / "extra" / "Priority.PCMESH", valid);
        save(base / "extra" / "Ignored.fbx", valid);
        save(base / "extra" / "Truncated.PCMESH", std::vector<uint8_t>(12));
        auto invalid = valid; put(invalid, 4, 0x600);
        save(base / "extra" / "WrongVersion.PCMESH", invalid);
        invalid = valid; put(invalid, 16, 0x12340000);
        save(base / "extra" / "AlreadyBound.PCMESH", invalid);
        invalid = valid; put(invalid, 12, 0xfffffffc);
        save(base / "extra" / "BadDirectory.PCMESH", invalid);
        // Same-stem textures/FBX must continue coexisting in the typed registry.
        Mods.emplace(to_hash("hero"), Mod{base / "extra" / "Hero.fbx", 3, {1}});
        Mods.emplace(to_hash("hero"), Mod{base / "extra" / "Hero.dds", 1, {2}});
        scan();
        for (const char* key : {"hero", "hero.pcmesh", "nested/hero",
                               "nested/hero.pcmesh", "nested\\hero",
                               "nested\\hero.pcmesh"}) {
            const Mod* mod = find(key);
            require(mod && mod->Data == valid && mod->Path.filename() == "Hero.PcMeSh",
                    "nested/case-insensitive mesh alias missing or source bytes changed");
        }
        require(Mods.count(0x12345678) == 1, "literal mesh hash missing");
        require(find("legacy") != nullptr, "legacy mods root was lost");
        require(find("priority") && find("priority")->Path.parent_path() == roots[0],
                "existing mods-before-extra priority changed");
        for (const char* key : {"ignored", "truncated", "wrongversion", "alreadybound", "baddirectory"})
            require(find(key) == nullptr, "invalid/unsupported image was registered");
        require(Mods.count(to_hash("hero")) == 3, "typed resources no longer coexist");
        const size_t count = Mods.size();
        scan();
        require(Mods.size() == count, "duplicate registration changed registry size");
        std::puts("PASS nested/case-insensitive PCMESH, 6 aliases, literal hash, mods root/priority, typed coexistence, pristine bytes, 4 invalid images, deduplication");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
'@

foreach ($loader in @('Ultimate_final.cpp', 'Ultimate_release.cpp')) {
    $source = [IO.File]::ReadAllText((Join-Path $repo $loader))
    $scanStart = $source.IndexOf('    // Enumerate the same roots used by texture, PCMESH and animation lookup.')
    $scanEnd = $source.IndexOf('    auto normalize_rel = ', $scanStart)
    $registerStart = $source.IndexOf('        if (ext == ".pcmesh") {', $scanEnd)
    if ($scanStart -lt 0 -or $scanEnd -lt 0 -or $registerStart -lt 0) {
        throw "$loader lacks validated native PCMESH registration"
    }
    $registerEnd = $source.IndexOf('        // PCANIM must not use the generic nfl path override above:', $registerStart)
    if ($registerEnd -lt 0) { throw "$loader registration boundary missing" }
    $walk = $source.Substring($scanStart, $scanEnd - $scanStart)
    $register = $source.Substring($registerStart, $registerEnd - $registerStart)
    $body = "void scan() {`n" + $walk + @'
    for (const auto& file : modFiles) {
        const auto& path = file.first;
        const auto& modsDir = file.second;
        const auto ext = transformToLower(path.extension().string());
        const auto stem = transformToLower(path.stem().string());
        const uint32_t hash = to_hash(stem.c_str());
'@ + $register + "`n    }`n}`n"
    $name = [IO.Path]::GetFileNameWithoutExtension($loader)
    $cpp = Join-Path $output ($name + '_registration.cpp')
    $exe = Join-Path $output ($name + '_registration.exe')
    [IO.File]::WriteAllText($cpp, $prefix + $hashCode + $detector + $body + $suffix)
    & $Compiler '-std=c++17' '-O0' '-static' $cpp '-o' $exe
    if ($LASTEXITCODE -ne 0) { throw "$loader probe compilation failed: $LASTEXITCODE" }
    & $exe (Join-Path $output ($name + '_fixtures'))
    if ($LASTEXITCODE -ne 0) { throw "$loader registration check failed: $LASTEXITCODE" }
}
