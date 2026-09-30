#include "../src/mod_pcmesh_source.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>

using modmesh::pcmeshsource::Mesh;
using modmesh::pcmeshsource::Source;

static void require(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}

static Mesh mesh(const char *name, uint32_t offset, uint32_t triangles)
{
    Mesh value;
    value.name = name;
    value.offset = offset;
    modmesh::pcmeshsource::Section section;
    section.primitiveType = 4;
    section.indexCount = triangles * 3;
    value.sections.push_back(section);
    return value;
}

static bool selected(const std::vector<std::string> &names, const char *name)
{
    return std::find(names.begin(), names.end(), name) != names.end();
}

static void scopeTests()
{
    Source source;
    source.meshes = {mesh("col_kg_bldgne2000", 1, 10), mesh("col_kg_bldgne2001", 2, 5),
                     mesh("col_kg_bldgne3000", 3, 100)};
    auto names = source.replacementMeshNames("COL_KG_BLDGNE2.PCMESH");
    require(names.size() == 2 && selected(names, "col_kg_bldgne2000")
                && selected(names, "col_kg_bldgne2001"), "numeric target suffix was lost");
    source.meshes = {mesh("foo01", 1, 100), mesh("foo02", 2, 30), mesh("fx_foo", 3, 10)};
    names = source.replacementMeshNames("foo");
    require(names.size() == 1 && names.front() == "foo01", "two-digit native names became a broad family");
    source.meshes = {mesh("auxiliary", 1, 1), mesh("box01", 2, 100)};
    names = source.replacementMeshNames("BLDG_BOX");
    require(names.size() == 1 && names.front() == "box01", "mismatched filename did not choose largest primary");
    source.meshes = {mesh("bdr000", 1, 10), mesh("bdr001", 2, 100), mesh("bdr002", 3, 5),
                     mesh("other000", 4, 1)};
    names = source.replacementMeshNames("BDC");
    require(names.size() == 3 && !selected(names, "other000"), "native primary family was not retained");
    source.meshes = {mesh("usm_blacksuit000", 1, 100), mesh("ultimate_spiderman001", 2, 40),
                     mesh("ultimate_spiderman002", 3, 5), mesh("ultimate_spiderman003", 4, 50),
                     mesh("venom_tentacles", 5, 500)};
    source.meshes[0].lods.push_back({0, 2, "ultimate_spiderman001", 20.f});
    source.meshes[1].lods.push_back({0, 3, "ultimate_spiderman002", 100.f});
    source.meshes[2].lods.push_back({0, 1, "usm_blacksuit000", 200.f});
    source.meshes[0].lods.push_back({0, 0, "external_mesh", 500.f});
    names = source.replacementMeshNames("usm_blacksuit");
    require(names.size() == 3 && selected(names, "ultimate_spiderman001")
                && selected(names, "ultimate_spiderman002") && !selected(names, "ultimate_spiderman003")
                && !selected(names, "venom_tentacles"), "LOD graph lost links or escaped its native scope");
    source.meshes = {mesh("venom000", 1, 100), mesh("venom001", 2, 50),
                     mesh("venom_tentacles", 3, 1000), mesh("venom_spider000", 4, 1000)};
    names = source.replacementMeshNames("VENOM");
    require(names.size() == 2 && !selected(names, "venom_tentacles")
                && !selected(names, "venom_spider000"), "matching file family replaced native attack meshes");
    std::puts("PASS native scope: numeric names, primary fallback, actual LOD graph, cycles and auxiliary isolation");
}

static void morphableMaterialTests()
{
    std::vector<uint8_t> bytes(0x170);
    auto put = [](std::vector<uint8_t> &b, size_t offset, uint32_t value) {
        std::memcpy(b.data()+offset, &value, sizeof(value));
    };
    auto name = [&](size_t offset, const char *text) {
        put(bytes, offset, 0x12345678u);
        std::memcpy(bytes.data()+offset+4, text, std::strlen(text));
    };
    std::memcpy(bytes.data(), "PCM ", 4);
    put(bytes,4,0x601); put(bytes,8,2); put(bytes,12,0x20);
    put(bytes,0x20,0x01000050); put(bytes,0x24,0x40);
    put(bytes,0x2c,0x02000040); put(bytes,0x30,0x90);
    put(bytes,0x40,0xd0); put(bytes,0x44,0xf0);
    put(bytes,0x58,0x110); put(bytes,0x60,0x130); put(bytes,0x90,0x150);
    name(0xd0,"face"); name(0xf0,"uspersonmorphable");
    name(0x110,"diffuse"); name(0x130,"detail"); name(0x150,"fixture000");
    Source source; std::string why;
    require(source.parse(bytes,&why),"valid morphable material fixture rejected");
    require(source.materials.size()==1 && source.materials[0].characterBindable
            && source.materials[0].diffuseName=="diffuse"
            && source.materials[0].detailName=="detail", "morphable texture layout differs from USPerson");
    for (int mutation=0;mutation<6;++mutation) {
        auto corrupt=bytes;
        switch (mutation) {
        case 0: put(corrupt,0x20,0x0100004f); break;
        case 1: put(corrupt,0x58,0xfffffff0u); break;
        case 2: put(corrupt,0x60,0xfffffff0u); break;
        case 3: put(corrupt,0x5c,1); break;
        case 4: put(corrupt,0x64,1); break;
        case 5: put(corrupt,0x68,0x7fc00000u); break;
        }
        require(!source.parse(std::move(corrupt),&why),"unsafe morphable material accepted");
        require(source.bytes.empty() && source.materials.empty(),"failed morphable parse retained data");
    }
    std::puts("PASS morphable material: native USPerson pointer layout and 6 malformed guards");
}

static void directoryTests(const std::filesystem::path &directory)
{
    size_t files = 0, selectedCount = 0, externalLODs = 0;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (!entry.is_regular_file()
            || modmesh::pcmeshsource::normalized(entry.path().extension().string()) != ".pcmesh") continue;
        std::ifstream file(entry.path(), std::ios::binary);
        std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(file), {});
        Source source;
        std::string why;
        if (!source.parse(std::move(bytes), &why))
            throw std::runtime_error(entry.path().filename().string() + ": " + why);
        const auto names = source.replacementMeshNames(entry.path().filename().string());
        require(!names.empty(), "native file had no replacement scope");
        std::set<std::string> unique;
        for (const auto &name : names) {
            require(source.findMesh(name), "replacement escaped the actual file");
            require(unique.insert(name).second, "LOD graph duplicated a selected mesh");
        }
        for (const auto &mesh : source.meshes)
            for (const auto &lod : mesh.lods) if (!lod.meshOffset) ++externalLODs;
        ++files;
        selectedCount += names.size();
    }
    require(files > 0, "directory contains no native PCMESH files");
    std::printf("PASS native directory: %zu files, %zu selected meshes, %zu external LOD references kept outside scope\n",
                files, selectedCount, externalLODs);
}

int main(int argc, char **argv)
{
    try { scopeTests(); morphableMaterialTests(); }
    catch (const std::exception &error) { std::fprintf(stderr, "FAIL scope: %s\n", error.what()); return 1; }
    if (argc == 1) return 0;
    if (argc != 2) {
        std::fprintf(stderr, "usage: mod_pcmesh_source_probe [character.PCMESH|native-directory]\n");
        return 2;
    }
    if (std::filesystem::is_directory(argv[1])) {
        try { directoryTests(argv[1]); return 0; }
        catch (const std::exception &error) { std::fprintf(stderr, "FAIL directory: %s\n", error.what()); return 1; }
    }
    std::ifstream file(argv[1], std::ios::binary);
    const std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(file), {});
    modmesh::pcmeshsource::Source source;
    std::string why;
    if (!source.parse(bytes, &why)) {
        std::fprintf(stderr, "FAIL parse: %s\n", why.c_str());
        return 1;
    }
    size_t sectionCount = 0;
    for (const auto &mesh : source.meshes) {
        sectionCount += mesh.sections.size();
        std::printf("mesh=%s bones=%u sections=%zu\n", mesh.name.c_str(),
                    mesh.nbones, mesh.sections.size());
    }
    for (const auto &material : source.materials)
        std::printf("material=%s shader=%s diffuse=%s bytes=%u bindable=%d\n",
                    material.name.c_str(), material.shaderName.c_str(),
                    material.diffuseName.c_str(), material.size,
                    int(material.characterBindable));

    if (const auto *black = source.findMesh("USM_BLACKSUIT000")) {
        if (black->nbones != 66 || black->sections.size() != 10) return 1;
        const auto *body = source.findMaterial(black->sections[0].materialOffset);
        const auto *emblem = source.findMaterial(black->sections[9].materialOffset);
        if (!body || !emblem || !body->characterBindable || !emblem->characterBindable
            || body->name != "usm_blacksuit body" || emblem->name != "usm_blacksuit white"
            || body->diffuseName != "usm_blacksuit" || body == emblem)
            return 1;
    }
    if (const auto *venom = source.findMesh("venom000"))
        if (venom->nbones != 60 || venom->sections.size() != 17) return 1;

    size_t rejected = 0;
    auto requireRejection = [&](std::vector<uint8_t> corrupt) {
        modmesh::pcmeshsource::Source bad;
        if (bad.parse(std::move(corrupt), &why) || !bad.bytes.empty()
            || !bad.meshes.empty() || !bad.materials.empty()) return false;
        ++rejected;
        return true;
    };
    if (!requireRejection(std::vector<uint8_t>(bytes.begin(), bytes.begin() + 12))) return 1;
    auto corrupt = bytes;
    const uint32_t invalidOffset = 0xfffffff0u;
    std::memcpy(corrupt.data() + 12, &invalidOffset, 4);
    if (!requireRejection(std::move(corrupt))) return 1;
    for (const auto &material : source.materials) {
        if (!material.characterBindable) continue;
        corrupt = bytes;
        std::memcpy(corrupt.data() + material.offset + 0x18, &invalidOffset, 4);
        if (!requireRejection(std::move(corrupt))) return 1;
        break;
    }
    for (const auto &mesh : source.meshes) {
        if (mesh.lods.empty()) continue;
        uint32_t lodOffset = 0;
        std::memcpy(&lodOffset, bytes.data() + mesh.offset + 0x1c, 4);
        for (size_t badField : {size_t(mesh.offset) + 0x18, size_t(mesh.offset) + 0x1c, size_t(lodOffset)}) {
            corrupt = bytes;
            std::memcpy(corrupt.data() + badField, &invalidOffset, 4);
            if (!requireRejection(std::move(corrupt))) return 1;
        }
        corrupt = bytes;
        const uint32_t notFinite = 0x7fc00000;
        std::memcpy(corrupt.data() + lodOffset + 4, &notFinite, 4);
        if (!requireRejection(std::move(corrupt))) return 1;
        break;
    }
    std::printf("PASS meshes=%zu sections=%zu materials=%zu malformed_rejected=%zu\n",
                source.meshes.size(), sectionCount, source.materials.size(), rejected);
    return 0;
}
