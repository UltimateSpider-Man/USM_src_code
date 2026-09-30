#pragma once
#include <d3d9.h>

#include <mmreg.h>
#include <dsound.h>

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>

#include <map>
#include <set>
#include <array>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <new>

#include "variable.h"

struct Mod {
    std::filesystem::path Path;
    int Type;       // tlresource_type: 1 = texture, 2 = raw mesh file (.PCMESH), 3 = (custom) mesh
    std::vector<uint8_t> Data;
};

// Mod::Type value for .ENT entity-template mash images (entity_base.cpp).
// tlresource_type occupies 0..12 and resource_key_type reuses the same small
// integers (ENTITY == 4 collides with TLRESOURCE_TYPE_MORPH_FILE), so .ent
// mods key their registry entries with a constant safely outside both enums.
inline constexpr int MOD_TYPE_ENT_FILE = 0x100;

// Mod::Type value for .PCSX script-executable mash images (script_object.cpp).
// Same reasoning as MOD_TYPE_ENT_FILE: kept safely clear of the small
// tlresource_type / resource_key_type integer ranges.
inline constexpr int MOD_TYPE_PCSX_FILE = 0x101;

// Mod::Type value for .PCSX files in the CHUNK (text) format the CHUCK
// compiler emits -- the form an actual file on disk has, as opposed to the
// mash image a pack serves. These carry no Data: the engine's own
// script_executable::load() reads them, plus their .pcsst/.pcpst/.pcsxl
// siblings, straight from Mod::Path's directory.
inline constexpr int MOD_TYPE_PCSX_CHUNK = 0x102;

// Mod::Type value for PlayStation 2 beta script-executable images (.PS2SX).
// These use a wider serialized VM record and beta script-library indices, so
// registration translates them to a validated PC mash image before the
// normal script loader is allowed to see the bytes.
inline constexpr int MOD_TYPE_PS2SX_FILE = 0x103;

// Mod::Type value for loose .ALS animation-logic-system mash streams.
// ALS resources are not generic-mash images and are unmashed in place before
// entity instances consume them, so they need their own typed registry entry.
inline constexpr int MOD_TYPE_ALS_FILE = 0x104;

// ---------------------------------------------------------------------------
// Skeletal-animation carriers
//
// The native FBX importer (mod_mesh_import.h) bakes every take in the file
// into channels keyed by *bone name*. The engine, on the other hand,
// addresses bones by *slot index* into the mesh bone array, so each channel
// also carries skelIndex: modmesh::buildSectionsForMesh resolves it against
// the TARGET mesh with the same bind-pose cluster table the skinning path
// trusts (Bone_N name digits as fallback), which is what lets an arbitrary
// FBX joint order land on the retail skeleton layout. skelIndex -1 means the
// bone does not exist on this mesh - do not play that channel.
//
// Clips converted by modCollectFbxAnimations (ngl.cpp) use SECONDS
// (ticksPerSecond = 1.0), positions in game units, and quaternions with the
// FBX RotationOrder + Pre/PostRotation already folded in: a channel is the
// bone's complete local transform in parent space, composed T*R*S.
// ---------------------------------------------------------------------------
struct modVecKey {                 // position / scale key
    double  time = 0.0;            // in ticks
    float   x = 0.0f, y = 0.0f, z = 0.0f;
};

struct modQuatKey {                // rotation key
    double  time = 0.0;            // in ticks
    float   x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
};

struct modBoneChannel {
    std::string             boneName;       // Assimp node/bone name
    int                     skelIndex = -1; // resolved engine slot (-1 = unmapped)
    std::vector<modVecKey>  positions;
    std::vector<modQuatKey> rotations;
    std::vector<modVecKey>  scales;
};

struct modAnimClip {
    std::string                 name;
    double                      duration = 0.0;       // in ticks
    double                      ticksPerSecond = 25.0; // 0 in FBX -> default
    std::vector<modBoneChannel> channels;
};

struct modGenericMesh {
    Mod* mod;
    std::vector<float> vertices;
    std::vector<uint16_t> indices;
    IDirect3DVertexBuffer9* vertexBuffer = nullptr;
    IDirect3DIndexBuffer9* indexBuffer = nullptr;
    UINT stride = 16;
    UINT numVertices = 0;
    UINT numIndices = 0;

    // Bone ordering actually emitted into the vertex stream, in palette-slot
    // order (slot i -> boneNames[i]). Used to rebuild Section->BonesIdx and to
    // line animation channels up against the section palette.
    std::vector<std::string> boneNames;

    // Parsed FBX animation clips (empty for OBJ / static meshes).
    std::vector<modAnimClip> animations;
};


// Several mods may legitimately share one name hash as long as their types
// differ (VENOM.PCMESH and VENOM.FBX both key "venom"), so the container is
// a multimap. Typed lookups disambiguate; untyped ones return the first
// registered entry for the hash.
extern std::multimap<uint32_t, Mod> Mods;
extern Mod* dbgReplaceMesh;


[[maybe_unused]] static bool hasMod(uint32_t hash) {
    return Mods.find(hash) != Mods.end();
}

[[maybe_unused]] static Mod* getMod(uint32_t hash, int type = -1) {
    auto range = Mods.equal_range(hash);
    for (auto it = range.first; it != range.second; ++it) {
        if (type == -1 || it->second.Type == type)
            return &it->second;
    }
    return nullptr;
}
[[maybe_unused]] static uint8_t* getModDataByHash(uint32_t hash) {
    if (hasMod(hash))
        if (auto mod = getMod(hash))
            return &mod->Data.data()[0];
    return nullptr;
}

[[maybe_unused]] static std::string transformToLower(const std::string& name)
{
    std::string res = name;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) { return std::tolower(c); });
    return res;
}

// this is O(n) (don't use this unless necessary!)
[[maybe_unused]] static Mod* getModByFilemame(const std::string& name) {
    std::string search = transformToLower(name);
    for (auto& [hash, mod] : Mods) {
        std::string filename = transformToLower(mod.Path.filename().string());
        if (filename == search)
            return &mod;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Where mods live on disk.
//
// enumerate_mods() walks these RECURSIVELY at startup, so users organize
// drops in subfolders ("extra/anims/HERO.PCANIM"). Every other scanner has
// to agree on both points or files silently go unseen: the base is resolved
// against the process working directory once (a bare relative "mods" breaks
// if the game ever chdir's, and the lazy NAL scan runs long after startup),
// and traversal is recursive.
// ---------------------------------------------------------------------------
[[maybe_unused]] static const std::vector<std::filesystem::path> &modRootDirs()
{
    static const std::vector<std::filesystem::path> dirs = [] {
        std::error_code ec;
        std::filesystem::path base = std::filesystem::current_path(ec);
        if (ec)
            base = std::filesystem::path();
        return std::vector<std::filesystem::path> { base / "mods", base / "extra" };
    }();
    return dirs;
}

// Resolve a file named in a sidecar ("donor=ULTIMATE_SPIDERMAN.PCANIM") to a
// real path: an explicit relative/absolute path first, then a recursive
// filename match under the mod roots (case-insensitively, since the retail
// naming is upper case and drops rarely are). `hint` is searched first, for
// files that live next to the one that referenced them.
[[maybe_unused]] static bool modResolveFile(const std::string &name,
                                            std::filesystem::path &out,
                                            const std::filesystem::path &hint = {})
{
    if (name.empty())
        return false;

    std::error_code ec;
    auto tryPath = [&](const std::filesystem::path &p) {
        if (p.empty() || !std::filesystem::is_regular_file(p, ec))
            return false;
        out = p;
        return true;
    };

    if (!hint.empty() && tryPath(hint / name))
        return true;
    for (const auto &root : modRootDirs())
        if (tryPath(root / name))
            return true;
    if (tryPath(std::filesystem::path(name)))
        return true;

    // recursive filename match
    const std::string want =
        transformToLower(std::filesystem::path(name).filename().string());
    for (const auto &root : modRootDirs())
    {
        std::filesystem::recursive_directory_iterator it(
            root, std::filesystem::directory_options::skip_permission_denied, ec);
        if (ec)
            continue;
        for (const auto &e : it)
        {
            std::error_code fec;
            if (!e.is_regular_file(fec))
                continue;
            if (transformToLower(e.path().filename().string()) == want)
            {
                out = e.path();
                return true;
            }
        }
    }
    return false;
}

[[maybe_unused]] static bool readModFile(const Mod* mod, std::vector<uint8_t>& out) {
    if (mod == nullptr)
        return false;
    if (!mod->Data.empty()) {            // eagerly loaded types (meshes, ...)
        out = mod->Data;
        return true;
    }
    std::ifstream f(mod->Path, std::ios::binary);
    if (!f)
        return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !out.empty();
}


// ---------------------------------------------------------------------------
// WAV sound mods
//
// Any *.wav dropped under the mod root (extra/ — see enumerate_mods) is
// decoded at enumerate_mods() time into an in-memory PCM16 image and keyed
// by the engine hash of its file stem, i.e.
// extra/sounds/gang_skin_boss_fem_dead.wav registers under
// to_hash("gang_skin_boss_fem_dead") — the same hash space the engine uses
// for sound / alias names, so a wav named after a sound resource is a
// drop-in override candidate for it.
//
// Voice lines are the reason two more bindings exist on top of that: VO is
// requested by hash only and the original strings never shipped, so a stem
// that parses as a literal hash ("extra/0x1189AB87.wav", "1189ab87.wav",
// decimal "294628231.wav") binds under that exact value, and an external
// hash->name sidecar (extra/*hashes*.txt, see modWavLoadHashNames) lets a
// wav use a human-readable stem that is routed to the hash the sidecar
// pairs it with. Together every voice line is overridable — worst case by
// its raw hash, best case by a community-named sidecar entry.
//
// Playback goes through the game's own IDirectSound8 device (0x00987518,
// created by create_sound_ifc() @ 0x0081E2D0 during startup). Because
// enumerate_mods() runs inside DllMain — where the device doesn't exist yet
// and creating COM objects under the loader lock is unsafe — buffer upload
// is deferred: sound_bank_slot::load() calls modWav_onSoundBankLoad(), which
// runs on the main thread long after audio init and uploads every decoded
// wav into a DirectSound secondary buffer exactly once.
//
// NOTE: the whole project is compiled with CINTERFACE (see CMakeLists), so
// every COM call below goes through lpVtbl-> explicitly.
// ---------------------------------------------------------------------------

struct modWavSound {
    std::filesystem::path path;
    uint16_t channels   = 0;
    uint32_t sampleRate = 0;
    std::vector<int16_t> pcm;                  // interleaved, always 16-bit
    IDirectSoundBuffer *dsBuffer = nullptr;    // uploaded lazily, owned here
};

// Registry: engine hash of the file stem -> decoded sound.
// C++17 inline variable: one shared instance across all TUs of the DLL.
inline std::unordered_map<uint32_t, modWavSound> ModWavSounds;

// Duplicated buffers currently (or recently) playing. Duplication is what
// lets the same sound overlap itself; entries are pruned opportunistically.
inline std::vector<IDirectSoundBuffer *> ModWavVoices;

// Mirrors to_hash() @ 0x00501BE0 (string_hash.h). Re-implemented here since
// mod.h is pulled in through common.h ahead of string_hash.h; keep the two
// in sync (h = lower(c) + 33 * h).
[[nodiscard]] inline uint32_t modSoundHash(const char *str) {
    uint32_t res = 0;
    for (int c = *str; c != '\0'; ++str, c = *str) {
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        res = (uint32_t)c + 33u * res;
    }
    return res;
}

// ---------------------------------------------------------------------------
// External hash -> name sidecars (extra/*hashes*.txt)
//
// Loaded by enumerate_mods() BEFORE the wav walk. Any .txt under the mod
// root whose filename contains "hash" is parsed line by line:
//
//     # voice_line_hashes.txt — '#', ';' and '//' start comments
//     0x1189AB87 = spidey_m01_l001       hex hash = name
//     294628231  = venom_taunt_03        decimal hash works too
//     spidey_m01_l001 = 0x1189AB87       either order is accepted
//     boss_intro_line                    bare name -> modSoundHash(name)
//
// With the first mapping loaded, BOTH of these override hash 0x1189AB87:
//     extra/0x1189AB87.wav               (literal-hash stem, no sidecar needed)
//     extra/spidey_m01_l001.wav          (friendly stem routed via the sidecar)
// The hash->name direction also feeds the override log, so a firing voice
// line prints its name instead of a bare hex value.
// ---------------------------------------------------------------------------
inline std::unordered_map<uint32_t, std::string> ModWavHashNames;   // hash -> name
inline std::unordered_map<std::string, uint32_t> ModWavNameHashes;  // lower(name) -> hash
inline std::unordered_map<uint32_t, uint32_t>    ModWavHashAliases; // alias hash -> registered hash

// "0x1189AB87" / bare 8-hex-digit "1189ab87" / decimal "294628231" -> value.
// Same acceptance rules as the mesh literal-hash stems in enumerate_mods().
[[nodiscard]] inline bool modParseLiteralHash(const std::string &s, uint32_t *out) {
    if (s.empty())
        return false;
    size_t i = 0;
    bool hexMarked = false;
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        i = 2;
        hexMarked = true;
    }
    const size_t n = s.size() - i;
    if (n == 0 || n > (hexMarked ? 8u : 10u))
        return false;
    bool allDec = true, allHex = true;
    for (size_t k = i; k < s.size(); ++k) {
        const char c = s[k];
        if (c < '0' || c > '9') allDec = false;
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
              || (c >= 'A' && c <= 'F')))
            allHex = false;
    }
    uint64_t v = 0;
    if (hexMarked || (!allDec && allHex && n == 8)) {
        if (!allHex || n > 8) return false;
        v = std::strtoull(s.c_str() + i, nullptr, 16);
    } else if (allDec) {
        v = std::strtoull(s.c_str() + i, nullptr, 10);
    } else {
        return false;
    }
    if (v > 0xFFFFFFFFull)
        return false;
    *out = (uint32_t)v;
    return true;
}

// Name a hash maps to, or nullptr when no sidecar entry covers it.
[[nodiscard]] inline const char *modWavHashName(uint32_t hash) {
    auto it = ModWavHashNames.find(hash);
    return it != ModWavHashNames.end() ? it->second.c_str() : nullptr;
}

// Parse one sidecar file into the maps above. Returns entries added.
[[maybe_unused]] static int modWavLoadHashNames(const std::filesystem::path &path) {
    std::ifstream f(path);
    if (!f)
        return 0;

    auto trim = [](std::string &s) {
        const size_t b = s.find_first_not_of(" \t\r\n");
        const size_t e = s.find_last_not_of(" \t\r\n");
        s = (b == std::string::npos) ? std::string() : s.substr(b, e - b + 1);
    };

    int added = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (const size_t cut = line.find_first_of("#;"); cut != std::string::npos)
            line.erase(cut);
        if (const size_t cut = line.find("//"); cut != std::string::npos)
            line.erase(cut);
        trim(line);
        if (line.empty())
            continue;

        // "<hash> [= ,\t] <name>", "<name> [= ,\t] <hash>", or "<name>" alone.
        std::string left = line, right;
        if (const size_t sep = line.find_first_of("=,\t "); sep != std::string::npos) {
            left  = line.substr(0, sep);
            right = line.substr(sep + 1);
            trim(left);
            trim(right);
            if (!right.empty() && (right[0] == '=' || right[0] == ',')) {
                right.erase(0, 1);              // "name = hash" leaves the '='
                trim(right);
            }
        }

        uint32_t hash = 0;
        std::string name;
        if (!right.empty() && modParseLiteralHash(left, &hash)) {
            name = right;
        } else if (!right.empty() && modParseLiteralHash(right, &hash)) {
            name = left;
        } else if (right.empty()) {
            name = left;                        // bare name: self-hashing entry
            hash = modSoundHash(transformToLower(name).c_str());
        } else {
            continue;                           // two names, no hash — skip
        }

        const std::string key = transformToLower(name);
        ModWavHashNames[hash] = key;
        ModWavNameHashes[key] = hash;
        ++added;
    }

    if (added)
        printf("mod: hash-name sidecar %s -> %d entr%s\n",
               path.filename().string().c_str(), added, added == 1 ? "y" : "ies");
    return added;
}

// The game's DirectSound8 device. Null until create_sound_ifc() has run.
[[nodiscard]] inline IDirectSound8 *modWavDevice() {
    static Var<IDirectSound8 *> s_gameDirectSound {0x00987518};
    return s_gameDirectSound();
}

// Linear [0..1] gain -> DirectSound attenuation (hundredths of dB).
[[nodiscard]] inline LONG modWavLinearToDb(float v) {
    if (v <= 0.001f) return DSBVOLUME_MIN;      // -100 dB
    if (v >= 1.0f)   return DSBVOLUME_MAX;      //    0 dB
    return (LONG)(2000.0f * std::log10(v));
}

// ---------------------------------------------------------------------------
// RIFF/WAVE -> interleaved PCM16 decoder.
//
// Accepts:  fmt 1 (PCM) at 8/16/24/32 bits, fmt 3 (IEEE float32) and
//           WAVE_FORMAT_EXTENSIBLE (0xFFFE) wrapping either of those.
// Emits:    16-bit signed samples whatever the source depth, which keeps
//           the DirectSound path to a single WAVEFORMATEX shape.
// ---------------------------------------------------------------------------
[[nodiscard]] inline bool modWavParse(const uint8_t *bytes, size_t size, modWavSound &out) {
    auto rd_u32 = [&](size_t off) -> uint32_t {
        uint32_t v; std::memcpy(&v, bytes + off, 4); return v;
    };
    auto rd_u16 = [&](size_t off) -> uint16_t {
        uint16_t v; std::memcpy(&v, bytes + off, 2); return v;
    };
    constexpr auto fourcc = [](char a, char b, char c, char d) -> uint32_t {
        return (uint32_t)(uint8_t)a | ((uint32_t)(uint8_t)b << 8) |
               ((uint32_t)(uint8_t)c << 16) | ((uint32_t)(uint8_t)d << 24);
    };

    if (size < 12 ||
        rd_u32(0) != fourcc('R','I','F','F') ||
        rd_u32(8) != fourcc('W','A','V','E'))
        return false;

    uint16_t fmtTag = 0, numChannels = 0, bitsPerSample = 0;
    uint32_t sampleRate = 0;
    const uint8_t *data = nullptr;
    size_t dataSize = 0;

    // Chunk walk (chunks are word-aligned).
    for (size_t off = 12; off + 8 <= size; ) {
        const uint32_t id = rd_u32(off);
        const uint32_t sz = rd_u32(off + 4);
        const size_t body = off + 8;
        if (body + sz > size)
            break;                                  // truncated file

        if (id == fourcc('f','m','t',' ') && sz >= 16) {
            fmtTag        = rd_u16(body + 0);
            numChannels   = rd_u16(body + 2);
            sampleRate    = rd_u32(body + 4);
            bitsPerSample = rd_u16(body + 14);

            // WAVE_FORMAT_EXTENSIBLE: real tag lives in SubFormat.Data1.
            if (fmtTag == 0xFFFE && sz >= 40)
                fmtTag = (uint16_t)rd_u32(body + 24);
        } else if (id == fourcc('d','a','t','a')) {
            data = bytes + body;
            dataSize = sz;
        }

        off = body + sz + (sz & 1);
    }

    if (!data || !numChannels || !sampleRate)
        return false;

    const uint32_t bytesPer = bitsPerSample / 8;
    if (!bytesPer)
        return false;
    const size_t sampleCount = dataSize / bytesPer;

    out.channels   = numChannels;
    out.sampleRate = sampleRate;
    out.pcm.resize(sampleCount);

    if (fmtTag == 1 && bitsPerSample == 16) {          // the common case
        std::memcpy(out.pcm.data(), data, sampleCount * 2);
    } else if (fmtTag == 1 && bitsPerSample == 8) {    // unsigned 8 -> s16
        for (size_t i = 0; i < sampleCount; ++i)
            out.pcm[i] = (int16_t)(((int)data[i] - 128) << 8);
    } else if (fmtTag == 1 && bitsPerSample == 24) {   // s24 -> s16
        for (size_t i = 0; i < sampleCount; ++i)
            out.pcm[i] = (int16_t)((int16_t)data[i * 3 + 2] << 8 | data[i * 3 + 1]);
    } else if (fmtTag == 1 && bitsPerSample == 32) {   // s32 -> s16
        for (size_t i = 0; i < sampleCount; ++i) {
            int32_t s; std::memcpy(&s, data + i * 4, 4);
            out.pcm[i] = (int16_t)(s >> 16);
        }
    } else if (fmtTag == 3 && bitsPerSample == 32) {   // f32 -> s16
        for (size_t i = 0; i < sampleCount; ++i) {
            float f; std::memcpy(&f, data + i * 4, 4);
            f = f < -1.0f ? -1.0f : (f > 1.0f ? 1.0f : f);
            out.pcm[i] = (int16_t)(f * 32767.0f);
        }
    } else {
        out.pcm.clear();
        return false;                                  // ADPCM/mp3-in-wav/etc.
    }

    return true;
}

// Decode + register one wav under to_hash(stem). Called by enumerate_mods().
[[maybe_unused]] static bool modWavRegister(const std::filesystem::path &path,
                                            const std::vector<uint8_t> &fileData) {
    modWavSound snd;
    if (!modWavParse(fileData.data(), fileData.size(), snd)) {
        printf("mod: wav %s SKIPPED (unsupported format - use PCM/float wav)\n",
               path.filename().string().c_str());
        return false;
    }

    snd.path = path;
    const std::string stem = transformToLower(path.stem().string());
    const uint32_t hash = modSoundHash(stem.c_str());

    printf("mod: wav %s -> 0x%08X (%u ch, %u Hz, %u samples)\n",
           stem.c_str(), hash, (uint32_t)snd.channels,
           snd.sampleRate, (uint32_t)snd.pcm.size());

    // Re-registration (enumerate_mods() reruns) frees the old buffer first.
    if (auto it = ModWavSounds.find(hash); it != ModWavSounds.end()) {
        if (it->second.dsBuffer)
            it->second.dsBuffer->lpVtbl->Release(it->second.dsBuffer);
        ModWavSounds.erase(it);
    }

    ModWavSounds.emplace(hash, std::move(snd));

    // Voice-line bindings on top of to_hash(stem): a literal-hash stem
    // ("0x1189AB87.wav") and a stem named in a *hashes*.txt sidecar both
    // alias this registration to the hash they designate, so lines known
    // only by hash stay overridable (see the sidecar block above).
    if (uint32_t literal = 0;
        modParseLiteralHash(stem, &literal) && literal != hash) {
        ModWavHashAliases[literal] = hash;
        printf("mod: wav %s also bound as literal hash 0x%08X\n",
               stem.c_str(), literal);
    }
    if (auto it = ModWavNameHashes.find(stem);
        it != ModWavNameHashes.end() && it->second != hash) {
        ModWavHashAliases[it->second] = hash;
        printf("mod: wav %s also bound to 0x%08X via hash-name sidecar\n",
               stem.c_str(), it->second);
    }

    return true;
}

[[nodiscard]] inline modWavSound *getWavMod(uint32_t hash) {
    auto it = ModWavSounds.find(hash);
    if (it == ModWavSounds.end()) {
        if (auto al = ModWavHashAliases.find(hash); al != ModWavHashAliases.end())
            it = ModWavSounds.find(al->second);
    }
    return it != ModWavSounds.end() ? &it->second : nullptr;
}

[[nodiscard]] inline modWavSound *getWavModByName(const char *name) {
    const std::string key = transformToLower(name);
    if (auto it = ModWavNameHashes.find(key); it != ModWavNameHashes.end())
        if (auto *snd = getWavMod(it->second))
            return snd;
    return getWavMod(modSoundHash(name));
}

[[maybe_unused]] inline bool hasWavMod(uint32_t hash) {
    return getWavMod(hash) != nullptr;
}

// Upload the decoded PCM into a static DirectSound secondary buffer (once).
// Fails soft (returns false) if the game's device isn't up yet.
inline bool modWavEnsureBuffer(modWavSound &snd) {
    if (snd.dsBuffer)
        return true;
    if (snd.pcm.empty())
        return false;

    IDirectSound8 *ds = modWavDevice();
    if (ds == nullptr)
        return false;                           // create_sound_ifc not run yet

    WAVEFORMATEX wfx {};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = snd.channels;
    wfx.nSamplesPerSec  = snd.sampleRate;
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = (WORD)(wfx.nChannels * 2);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    DSBUFFERDESC desc {};
    desc.dwSize        = sizeof(DSBUFFERDESC);
    desc.dwFlags       = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN |
                         DSBCAPS_CTRLFREQUENCY | DSBCAPS_GLOBALFOCUS |
                         DSBCAPS_STATIC;
    desc.dwBufferBytes = (DWORD)(snd.pcm.size() * 2);
    desc.lpwfxFormat   = &wfx;

    IDirectSoundBuffer *buf = nullptr;
    if (FAILED(ds->lpVtbl->CreateSoundBuffer(ds, &desc, &buf, nullptr)) || !buf) {
        printf("wav mod: CreateSoundBuffer failed for %s\n",
               snd.path.filename().string().c_str());
        return false;
    }

    void *p1 = nullptr, *p2 = nullptr;
    DWORD n1 = 0, n2 = 0;
    if (FAILED(buf->lpVtbl->Lock(buf, 0, desc.dwBufferBytes, &p1, &n1, &p2, &n2, 0))) {
        buf->lpVtbl->Release(buf);
        return false;
    }
    std::memcpy(p1, snd.pcm.data(), n1);
    if (p2 && n2)
        std::memcpy(p2, (const uint8_t *)snd.pcm.data() + n1, n2);
    buf->lpVtbl->Unlock(buf, p1, n1, p2, n2);

    snd.dsBuffer = buf;
    return true;
}

// Drop voices that have finished playing (keeps ModWavVoices bounded).
inline void modWavPruneVoices() {
    for (size_t i = ModWavVoices.size(); i-- > 0; ) {
        IDirectSoundBuffer *v = ModWavVoices[i];
        DWORD status = 0;
        if (v == nullptr ||
            FAILED(v->lpVtbl->GetStatus(v, &status)) ||
            !(status & DSBSTATUS_PLAYING)) {
            if (v) v->lpVtbl->Release(v);
            ModWavVoices.erase(ModWavVoices.begin() + i);
        }
    }
}

// Fire-and-forget playback. Each play duplicates the master buffer so the
// same sound can overlap itself. Returns the live voice (engine keeps
// ownership; do NOT Release it yourself) or nullptr.
inline IDirectSoundBuffer *modWavPlay(modWavSound &snd,
                                      float volume = 1.0f,
                                      bool loop = false) {
    modWavPruneVoices();

    if (!modWavEnsureBuffer(snd))
        return nullptr;

    IDirectSound8 *ds = modWavDevice();
    IDirectSoundBuffer *voice = nullptr;
    if (ds == nullptr ||
        FAILED(ds->lpVtbl->DuplicateSoundBuffer(ds, snd.dsBuffer, &voice)) ||
        voice == nullptr) {
        voice = snd.dsBuffer;                   // fall back: restart master
    }

    const bool duplicated = voice != snd.dsBuffer;
    if (FAILED(voice->lpVtbl->SetCurrentPosition(voice, 0)) ||
        FAILED(voice->lpVtbl->SetVolume(voice, modWavLinearToDb(volume))) ||
        FAILED(voice->lpVtbl->Play(voice, 0, 0, loop ? DSBPLAY_LOOPING : 0))) {
        voice->lpVtbl->Stop(voice);
        if (duplicated)
            voice->lpVtbl->Release(voice);
        return nullptr;
    }

    if (duplicated)
        ModWavVoices.push_back(voice);
    return voice;
}

[[maybe_unused]] inline IDirectSoundBuffer *modWavPlayByHash(uint32_t hash,
                                                             float volume = 1.0f,
                                                             bool loop = false) {
    if (auto *snd = getWavMod(hash))
        return modWavPlay(*snd, volume, loop);
    return nullptr;
}

[[maybe_unused]] inline IDirectSoundBuffer *modWavPlayByName(const char *name,
                                                             float volume = 1.0f,
                                                             bool loop = false) {
    return modWavPlayByHash(modSoundHash(name), volume, loop);
}

[[maybe_unused]] inline void modWavStopAll() {
    for (auto *v : ModWavVoices) {
        if (v) {
            v->lpVtbl->Stop(v);
            v->lpVtbl->Release(v);
        }
    }
    ModWavVoices.clear();
    for (auto &[hash, snd] : ModWavSounds) {
        if (snd.dsBuffer)
            snd.dsBuffer->lpVtbl->Stop(snd.dsBuffer);
    }
}

// Hook target: called from sound_bank_slot::load() right after the engine
// kicks off WBK streaming for (scene, bank). The game's DirectSound device
// is guaranteed to exist by now, so this is where the deferred buffer
// upload for every registered wav happens. As an audible smoke test /
// bank-load jingle, a wav named exactly like the bank (mods/<bank>.wav)
// is played once here.
[[maybe_unused]] inline void modWav_onSoundBankLoad(const char *scene, const char *bank) {
    if (ModWavSounds.empty())
        return;

    int uploaded = 0;
    for (auto &[hash, snd] : ModWavSounds) {
        if (snd.dsBuffer == nullptr && modWavEnsureBuffer(snd))
            ++uploaded;
    }

    if (uploaded)
        printf("wav mod: uploaded %d sound buffer(s) on bank load \"%s\" (scene \"%s\")\n",
               uploaded, bank ? bank : "", scene ? scene : "");

    if (bank && *bank) {
        if (auto *snd = getWavModByName(bank)) {
            printf("wav mod: playing bank-load override \"%s\"\n", bank);
            modWavPlay(*snd);
        }
    }
}
