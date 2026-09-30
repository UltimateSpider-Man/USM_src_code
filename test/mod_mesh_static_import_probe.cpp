// Public FBX parser/importer verification against the user's real static
// PCMESH vertex layouts. Every named object has a distinct source triangle.
#include "../src/mod_mesh_import.h"
#include "../src/mod_pcmesh_source.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}

std::string fixture(const modmesh::pcmeshsource::Source &source)
{
    std::ostringstream out, links;
    out << "; FBX 7.4.0 project file\nFBXHeaderExtension: {\n FBXVersion: 7400\n}\nObjects: {\n";
    for (size_t i = 0; i <= source.meshes.size(); ++i) {
        const std::string name = i < source.meshes.size() ? source.meshes[i].name : "ArtistReference";
        const size_t model = 100 + i * 10, geom = model + 1, material = model + 2;
        out << " Model: " << model << ", \"Model::" << name << "\", \"Mesh\" {\n }\n"
            << " Geometry: " << geom << ", \"Geometry::" << name << "\", \"Mesh\" {\n"
            << "  Vertices: *9 {\n   a: " << i * 10 << ",0,0," << i * 10 + 1 << ",0,0," << i * 10 << ",1,0\n  }\n"
            << "  PolygonVertexIndex: *3 {\n   a: 0,1,-3\n  }\n"
            << "  LayerElementUV: 0 {\n   MappingInformationType: \"ByPolygonVertex\"\n   ReferenceInformationType: \"Direct\"\n"
            << "   UV: *6 {\n    a: " << i * 3 << ",0," << i * 3 + 1 << ",0," << i * 3 + 2 << ",1\n   }\n  }\n"
            << "  LayerElementMaterial: 0 {\n   MappingInformationType: \"AllSame\"\n   ReferenceInformationType: \"IndexToDirect\"\n"
            << "   Materials: *1 {\n    a: 0\n   }\n  }\n }\n"
            << " Material: " << material << ", \"Material::Authored_" << name << "\", \"\" {\n }\n";
        links << " C: \"OO\"," << geom << "," << model << "\n"
              << " C: \"OO\"," << material << "," << model << "\n"
              << " C: \"OO\"," << model << ",0\n";
    }
    out << "}\nConnections: {\n" << links.str() << "}\n";
    return out.str();
}

std::vector<modmesh::OrigSectionView> views(const modmesh::pcmeshsource::Source &source,
                                         const modmesh::pcmeshsource::Mesh &mesh)
{
    std::vector<modmesh::OrigSectionView> result;
    for (const auto &section : mesh.sections) {
        modmesh::staticlayout::VertexLayout layout;
        require(modmesh::staticlayout::vertexLayout(section.vertexDefinitionName, section.stride, layout),
                "static native declaration unavailable");
        modmesh::OrigSectionView view;
        view.verts = reinterpret_cast<const float *>(source.bytes.data() + section.vertexOffset);
        view.nverts = section.vertexCount;
        view.strideBytes = section.stride;
        view.skinned = false;
        view.posOff = layout.position; view.nrmOff = layout.normal;
        view.uvOff = layout.uv; view.colOff = layout.color;
        result.push_back(view);
    }
    return result;
}

void verifySource(const modmesh::pcmeshsource::Source &source, const std::string &filename)
{
    const auto fbx = fixture(source);
    auto scene = modmesh::loadScene(filename + ".generated.fbx", fbx.data(), fbx.size());
    require(bool(scene), "generated named ASCII FBX rejected");
    require(scene->meshModelOrder.size() == source.meshes.size() + 1, "ASCII FBX lost named objects");
    scene->cfg.custom = true; scene->cfg.fit = false; scene->cfg.weld = false;
    scene->cfg.anim = false; scene->cfg.skin = 3; scene->cfg.autoTex = false;
    scene->cfg.roundtripEps = 0;
    modmesh::OrigMeshRef ref;
    ref.customSource = true;
    ref.targetFileName = filename;
    ref.targetMeshNames = source.replacementMeshNames(filename);
    for (const auto &mesh : source.meshes) ref.targetFileMeshNames.push_back(mesh.name);
    require(modmesh::hasNamedStaticTargets(*scene, ref), "native named static mode not selected");
    size_t triangles = 0;
    for (size_t mi = 0; mi < source.meshes.size(); ++mi) {
        const auto &mesh = source.meshes[mi];
        const auto original = views(source, mesh);
        const auto built = modmesh::buildSectionsForMesh(*scene, mesh.name, original, ref);
        size_t visible = 0;
        std::set<int> identities;
        for (const auto &section : built) {
            require(bool(section), "native geometry remained after named static replacement");
            if (section->hide) continue;
            ++visible;
            require(section->rigid && section->targetStride == 24 && section->tPosOff == 0
                && section->tNrmOff == -1 && section->tUvOff == 12 && section->tColOff == 20,
                "static replacement lost native declaration");
            require(section->palette.empty(), "static replacement acquired a bone palette");
            require(section->customMaterial && section->indices.size() == 3 && section->vertices.size() == 48,
                "sibling FBX geometry was copied into a static object");
            require(section->templateSection < original.size(), "invalid native template section");
            const auto &donor = original[section->templateSection];
            std::set<uint32_t> nativeColors;
            for (uint32_t i = 0; i < donor.nverts; ++i) nativeColors.insert(donor.getCol(i));
            require(section->colors.size() == 3, "static baked colors omitted");
            for (uint32_t color : section->colors)
                require(nativeColors.count(color) != 0, "static baked alpha/color changed");
            for (uint32_t index : section->indices) {
                require(index < 3, "static triangle index invalid");
                identities.insert(int(std::lround(section->vertices[size_t(index) * 16 + 6])));
            }
            ++triangles;
        }
        require(visible == 1 && identities == std::set<int>{int(mi * 3), int(mi * 3 + 1), int(mi * 3 + 2)},
                "named static object imported another object's triangle");
        require(scene->meshModelOrder.size() == source.meshes.size() + 1, "cached FBX scene was mutated");
    }
    const auto firstViews = views(source, source.meshes.front());
    require(modmesh::buildSectionsForMesh(*scene, "ArtistReference", firstViews, ref).empty(),
            "FBX-only object escaped native file inventory");
    require(modmesh::buildSectionsForMesh(*scene, "outside_file", firstViews, ref).empty(),
            "replacement escaped native file");
    if (source.meshes.size() > 1) {
        auto partial = *scene;
        partial.meshModelOrder.erase(partial.meshModelOrder.begin() + 1);
        require(modmesh::buildSectionsForMesh(partial, source.meshes[1].name,
                    views(source, source.meshes[1]), ref).empty(), "omitted native object was overwritten");
    }
    // Renaming every FBX model disables exact-name mode. The normal filename
    // scope then still restricts HUD substitutions to the private *_back.
    if (filename.compare(0, 3, "hg_") == 0) {
        auto arbitrary = *scene;
        arbitrary.meshModelOrder.resize(1);
        arbitrary.models.at(arbitrary.meshModelOrder.front()).name = "MySketchfabModel";
        require(!modmesh::hasNamedStaticTargets(arbitrary, ref), "unnamed static source entered named mode");
        for (const auto &mesh : source.meshes) {
            const bool expected = mesh.name == std::filesystem::path(filename).stem().string() + "_back";
            require(modmesh::targetMeshAllowed(arbitrary, mesh.name, ref) == expected,
                    "unnamed HUD replacement changed shared gauge parts");
        }
    }
    std::printf("PASS named static import %s: %zu independent objects/triangles, native colors and alpha preserved\n",
                filename.c_str(), triangles);
}

modmesh::pcmeshsource::Source syntheticSource()
{
    modmesh::pcmeshsource::Source source;
    source.bytes.resize(144);
    for (size_t i = 0; i < 2; ++i) {
        modmesh::pcmeshsource::Mesh mesh;
        mesh.name = i ? "hg_hero_gauge_tick_01b" : "hg_hero_fixture_back";
        modmesh::pcmeshsource::Section section;
        section.vertexOffset = uint32_t(i * 72); section.vertexCount = 3; section.vertexBytes = 72;
        section.stride = 24; section.vertexDefinitionName = "us_frontend";
        for (size_t v = 0; v < 3; ++v) {
            float row[6] = {float(v == 1), float(v == 2), 0, float(v == 1), float(v == 2), 0};
            const uint32_t color = i ? 0xb2ffab6c : 0xffffffff;
            std::memcpy(&row[5], &color, 4);
            std::memcpy(source.bytes.data() + i * 72 + v * 24, row, 24);
        }
        mesh.sections.push_back(section); source.meshes.push_back(mesh);
    }
    return source;
}
}

int main(int argc, char **argv)
{
    try {
        require(argc <= 2, "usage: mod_mesh_static_import_probe [uploaded-pcmesh-directory]");
        verifySource(syntheticSource(), "hg_hero_fixture");
        if (argc == 2) {
            const char *paths[] = {"hg_hero_blacksuit.PCMESH", "ZG_INT_G/ZG_INT_GC.PCMESH",
                "ZG_INT_F/ZG_INT_FC.PCMESH", "FX_DECAL_LAND.PCMESH", "FX_VENOMCLAW.PCMESH",
                "hg_hero_spiderman.PCMESH", "hg_hero_venom.PCMESH", "hg_boss_carnage.PCMESH",
                "hg_boss_venom.PCMESH", "hg_hero_peter.PCMESH"};
            for (const char *path : paths) {
                std::ifstream input(std::filesystem::path(argv[1]) / path, std::ios::binary);
                std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(input), {});
                modmesh::pcmeshsource::Source source;
                std::string why;
                if (!source.parse(std::move(bytes), &why)) throw std::runtime_error(std::string(path) + ": " + why);
                verifySource(source, std::filesystem::path(path).filename().string());
            }
        }
        return 0;
    } catch (const std::exception &error) { std::fprintf(stderr, "FAIL static import: %s\n", error.what()); return 1; }
}
