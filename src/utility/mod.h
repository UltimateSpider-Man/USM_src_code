#pragma once
#include <d3d9.h>

#include <cstdint>
#include <cstring>

#include <map>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <new>

struct Mod {
    std::filesystem::path Path;
    int Type;       // 1 = tex, 2 = mesh, 3 = (custom) mesh
    std::vector<uint8_t> Data;
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
};


extern std::map<uint32_t, Mod> Mods;
extern Mod* dbgReplaceMesh;


[[maybe_unused]] static bool hasMod(uint32_t hash) {
    return Mods.find(hash) != Mods.end();
}

[[maybe_unused]] static Mod* getMod(uint32_t hash, int type = -1) {
    if (type == -1) {
        auto it = Mods.find(hash);
        if (it != Mods.end())
            return &it->second;
    }
    else {
        for (auto& [ihash, mod] : Mods)
            if (hash == ihash && mod.Type == type)
                return &mod;
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
