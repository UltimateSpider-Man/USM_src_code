#pragma once
#include <d3d9.h>

#include <mmreg.h>
#include <dsound.h>

#include <cstdint>
#include <cstring>
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

// ---------------------------------------------------------------------------
// Skeletal-animation carriers
//
// Assimp gives us animation channels keyed by *bone name*. The engine, on the
// other hand, addresses bones by *slot index* into nglMeshParams::field_8[].
// We therefore keep the name on every keyframe track and resolve it to an
// engine slot at bind time (see modResolveSkeletonRemap in ngl.cpp). This is
// also what lets us reorder an arbitrary FBX joint order onto the retail
// skeleton layout instead of trusting the exporter to emit them in order.
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
// WAV sound mods
//
// Any *.wav dropped under mods/ is decoded at enumerate_mods() time into an
// in-memory PCM16 image and keyed by the engine hash of its file stem, i.e.
// mods/sounds/gang_skin_boss_fem_dead.wav registers under
// to_hash("gang_skin_boss_fem_dead") — the same hash space the engine uses
// for sound / alias names, so a wav named after a sound resource is a
// drop-in override candidate for it.
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
    return true;
}

[[nodiscard]] inline modWavSound *getWavMod(uint32_t hash) {
    auto it = ModWavSounds.find(hash);
    return it != ModWavSounds.end() ? &it->second : nullptr;
}

[[nodiscard]] inline modWavSound *getWavModByName(const char *name) {
    return getWavMod(modSoundHash(name));
}

[[maybe_unused]] inline bool hasWavMod(uint32_t hash) {
    return ModWavSounds.find(hash) != ModWavSounds.end();
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
    } else {
        ModWavVoices.push_back(voice);
    }

    voice->lpVtbl->SetCurrentPosition(voice, 0);
    voice->lpVtbl->SetVolume(voice, modWavLinearToDb(volume));
    voice->lpVtbl->Play(voice, 0, 0, loop ? DSBPLAY_LOOPING : 0);
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