#include "../src/mod_pcmesh_source.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>

static void require(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}

static void layoutTests()
{
    using namespace modmesh::staticlayout;
    const char *shaders[] = {"us_frontend", "usfloor", "ussimpleinterior",
        "ustranslucentinterior", "usmsimplemorphable", "us_decal3d", "usperson",
        "uspersonsolid", "uspersonmorphable"};
    for (const char *shader : shaders) {
        MaterialLayout layout;
        require(materialLayout(shader, layout), "known material layout missing");
        std::vector<uint8_t> original(layout.minimumSize + 32, 0xa5), changed = original;
        const float tint[4] = {.1f, .2f, .3f, .4f};
        require(retargetMaterial(changed.data(), layout.minimumSize, layout,
                    0x11223344, 0x55667788, tint), "valid material retarget rejected");
        for (size_t i = 0; i < changed.size(); ++i) {
            const bool slot = (i >= layout.textureName && i < layout.textureName + 4)
                || (i >= layout.texture && i < layout.texture + 4)
                || (layout.color >= 0 && i >= size_t(layout.color) && i < size_t(layout.color) + 16);
            if (!slot) require(changed[i] == original[i], "material retarget damaged shader fields or canary");
        }
        for (size_t size = 0; size < layout.minimumSize; ++size) {
            changed = original;
            require(!retargetMaterial(changed.data(), size, layout, 1, 2, tint), "truncated material accepted");
            require(changed == original, "rejected retarget wrote partial material");
        }
        changed = original;
        require(retargetMaterial(changed.data(), layout.minimumSize, layout,
                    0x11223344, 0x55667788, tint, true), "external albedo retarget rejected");
        for (size_t i = 0; i < changed.size(); ++i) {
            const bool slot = (i >= layout.textureName && i < layout.textureName + 4)
                || (i >= layout.texture && i < layout.texture + 4)
                || (layout.color >= 0 && i >= size_t(layout.color) && i < size_t(layout.color) + 16);
            const bool personDetail = layout.color == 0x28 && i >= 0x3c && i < 0x44;
            if (personDetail) require(changed[i] == 0, "external albedo retained native sphere-map shader");
            else if (!slot) require(changed[i] == original[i], "external albedo changed lighting, outline or static flags");
        }
    }
    MaterialLayout missing;
    require(!materialLayout("unknownshader", missing), "unknown material guessed");
    VertexLayout v;
    require(vertexLayout("US_FRONTEND", 24, v) && v.position == 0 && v.normal == -1
                && v.uv == 12 && v.color == 20, "HUD declaration offsets wrong");
    require(!vertexLayout("us_frontend", 32, v), "incompatible declaration stride accepted");
    require(!vertexLayout("unknown", 24, v), "unknown vertex declaration guessed");
    modmesh::pcmeshsource::Source source;
    modmesh::pcmeshsource::Mesh back, ticks;
    back.name = "hg_hero_venom_back"; back.offset = 1;
    ticks.name = "hg_hero_gauge_tick_01b"; ticks.offset = 2;
    modmesh::pcmeshsource::Section section;
    section.primitiveType = 4; section.indexCount = 300;
    ticks.sections.push_back(section);
    source.meshes = {ticks, back};
    const auto selected = source.replacementMeshNames("HG_HERO_VENOM.PCMESH");
    require(selected.size() == 1 && selected.front() == back.name, "HUD private back lost to larger shared ticks");
    std::puts("PASS static layouts: 9 material ABIs, bounded writes, truncation guards, PC PUV-color declaration and HUD scope");
}

static void uploadedTests(const std::filesystem::path &root)
{
    const char *paths[] = {"hg_hero_blacksuit.PCMESH", "ZG_INT_G/ZG_INT_GC.PCMESH",
        "ZG_INT_F/ZG_INT_FC.PCMESH", "FX_DECAL_LAND.PCMESH", "FX_VENOMCLAW.PCMESH",
        "hg_hero_spiderman.PCMESH", "hg_hero_venom.PCMESH", "hg_boss_carnage.PCMESH",
        "hg_boss_venom.PCMESH", "hg_hero_peter.PCMESH"};
    size_t meshes = 0, sections = 0, materials = 0, transparent = 0;
    std::set<std::string> declarations;
    for (const char *path : paths) {
        std::ifstream input(root / path, std::ios::binary);
        std::vector<uint8_t> image(std::istreambuf_iterator<char>(input), {});
        modmesh::pcmeshsource::Source source;
        std::string why;
        if (!source.parse(std::move(image), &why)) throw std::runtime_error(std::string(path) + ": " + why);
        const auto selected = source.replacementMeshNames(path);
        if (std::string(path).compare(0, 3, "hg_") == 0) {
            require(source.meshes.size() == 5, "HUD gauge mesh count changed");
            require(selected.size() == 1 && selected[0] == std::filesystem::path(path).stem().string() + "_back",
                    "HUD filename does not select its private panel");
        } else if (std::string(path).compare(0, 3, "FX_") == 0) {
            require(source.meshes.size() == 1 && selected.size() == 1, "effect scope changed");
        }
        for (const auto &material : source.materials) {
            require(material.textureBindable && !material.characterBindable, "static material metadata missing");
            require(!material.diffuseName.empty(), "static diffuse texture name was not read");
            require(material.size == material.layout.minimumSize, "uploaded material allocation size differs");
            auto corrupt = source.bytes;
            const uint32_t invalid = 0xfffffff0;
            std::memcpy(corrupt.data() + material.offset + material.layout.textureName, &invalid, 4);
            modmesh::pcmeshsource::Source bad;
            require(!bad.parse(corrupt, &why), "out-of-range static texture reference accepted");
            ++materials;
        }
        for (const auto &mesh : source.meshes) {
            require(mesh.nbones == 0, "static source has bones");
            ++meshes;
            for (const auto &section : mesh.sections) {
                modmesh::staticlayout::VertexLayout layout;
                require(modmesh::staticlayout::vertexLayout(section.vertexDefinitionName, section.stride, layout),
                        "uploaded vertex declaration unsupported");
                require(layout.uv == 12 && layout.color == 20 && layout.normal == -1,
                        "actual PUV-color declaration changed");
                declarations.insert(section.vertexDefinitionName);
                for (uint32_t i = 0; i < section.vertexCount; ++i) {
                    uint32_t color;
                    std::memcpy(&color, source.bytes.data() + section.vertexOffset + size_t(i) * section.stride + 20, 4);
                    if ((color >> 24) != 255) ++transparent;
                }
                ++sections;
            }
        }
        std::printf("PASS %s: %zu meshes, %zu materials; selected", path, source.meshes.size(), source.materials.size());
        for (const auto &name : selected) std::printf(" %s", name.c_str());
        std::puts("");
    }
    require(meshes == 55 && sections == 79 && materials == 59 && declarations.size() == 6,
            "uploaded fixture counts changed");
    require(transparent > 0, "transparent packed-color fixture missing");
    std::printf("PASS uploads: 10 PCMESH files, %zu meshes, %zu sections, %zu materials, %zu transparent vertices\n",
                meshes, sections, materials, transparent);
}

int main(int argc, char **argv)
{
    try {
        layoutTests();
        if (argc == 2) uploadedTests(argv[1]);
        else if (argc > 2) throw std::runtime_error("usage: mod_mesh_static_probe [uploaded-pcmesh-directory]");
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "FAIL static: %s\n", error.what());
        return 1;
    }
}
