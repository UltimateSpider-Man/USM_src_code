param(
    [string]$Compiler = 'C:/msys64/mingw32/bin/g++.exe',
    [string]$BuildDirectory = 'build/custom-material-check'
)

# Execute the real material retarget function and serialized-layout writer.
# Fake only the fixed-address engine allocator and image/white-texture providers.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$output = Join-Path $repo $BuildDirectory
[void](New-Item -ItemType Directory -Force -Path $output)
$env:PATH = (Split-Path -Parent $Compiler) + ';' + $env:PATH
$source = [IO.File]::ReadAllText((Join-Path $repo 'src/mod_mesh_custom_materials.inc'))
$start = $source.IndexOf('static void modRetargetCustomMaterial(')
if ($start -lt 0) { throw 'Production custom-material retarget function missing' }
$function = $source.Substring($start)

$prefix = @'
#include "mod_mesh_static_layout.h"
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

struct tlFixedString {
    std::string text;
    tlFixedString() = default;
    tlFixedString(const char* value) : text(value) {}
    const char* to_string() const { return text.c_str(); }
};
struct Shader {
    tlFixedString name;
    tlFixedString GetName() const { return name; }
};
struct nglMeshFile { struct { char* Buf = nullptr; } FileBuf; };
struct nglMaterialBase {
    tlFixedString* Name;
    Shader* m_shader;
    nglMeshFile* File;
    nglMaterialBase* NextMaterial;
    uint32_t field_10, field_14;
};
static_assert(sizeof(void*) == 4, "Probe must use retail 32-bit pointer ABI");
static_assert(sizeof(nglMaterialBase) == 0x18, "Native material header layout changed");
struct nglMeshSection { nglMaterialBase* Material; };
struct nglMesh { nglMeshFile* File; };
struct nglTexture { uint32_t identity; };
namespace modmesh {
struct BuiltSection {
    bool forceWhite = false, customMaterial = false;
    std::string sourceMaterialName;
    std::array<float, 4> customDiffuse{{1.f, 1.f, 1.f, 1.f}};
};
}
struct ModCustomMaterial {
    nglMaterialBase* material = nullptr;
    size_t size = 0;
    tlFixedString name, diffuseName;
};
struct ModCustomMaterialFile {
    std::map<std::string, std::unique_ptr<ModCustomMaterial>> materials;
};
static std::map<nglMeshFile*, ModCustomMaterialFile> modCustomMaterialFiles;
static std::map<char*, nglMeshFile*> modMeshBufferOwners;
static nglTexture white{1}, image{2};
static nglTexture* availableImage = nullptr;
static size_t nativeSize = 0;
static unsigned allocations = 0;
void* tlMemAlloc(uint32_t size, unsigned, unsigned) { ++allocations; return std::malloc(size); }
void tlMemFree(void* pointer) { std::free(pointer); }
void sp_log(const char*, ...) {}
static size_t modCustomMaterialSize(const nglMaterialBase*) { return nativeSize; }
static std::string modCustomImageKey(const std::filesystem::path& path) { return path.generic_string(); }
static nglTexture* modWhiteTexture() { return &white; }
static nglTexture* modCustomDiffuse(ModCustomMaterialFile&, const modmesh::BuiltSection&,
                                    const std::filesystem::path&, std::string& selected) {
    if (availableImage) selected = "authored-image";
    return availableImage;
}
'@

$suffix = @'
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
template<class T> T read(const void* pointer, size_t offset) {
    T value;
    std::memcpy(&value, static_cast<const uint8_t*>(pointer) + offset, sizeof(T));
    return value;
}
template<class T> void write(void* pointer, size_t offset, const T& value) {
    std::memcpy(static_cast<uint8_t*>(pointer) + offset, &value, sizeof(T));
}
void run(const char* label, const char* shaderName, bool custom, bool withImage,
         bool forceWhite = false) {
    modmesh::staticlayout::MaterialLayout layout;
    require(modmesh::staticlayout::materialLayout(shaderName, layout), "fixture shader unsupported");
    nativeSize = layout.minimumSize;
    auto* original = static_cast<nglMaterialBase*>(std::malloc(nativeSize));
    std::memset(original, 0xa5, nativeSize);
    Shader shader{tlFixedString{shaderName}};
    nglMeshFile owner;
    owner.FileBuf.Buf = reinterpret_cast<char*>(original);
    original->m_shader = &shader;
    original->File = &owner;
    original->NextMaterial = nullptr;
    tlFixedString name{"native-material"}; original->Name = &name;
    const bool person = layout.color == 0x28;
    if (person) {
        // Both native highlight/ink effects are active. These would alter
        // the FBX color when the target hero's sphere map is inherited.
        write(original, 0x3c, uint32_t(1)); write(original, 0x40, uint32_t(1));
        write(original, 0x44, uint32_t(1)); write(original, 0x48, uint32_t(1));
        write(original, 0x4c, uint32_t(2));
    }
    const auto* bytes = reinterpret_cast<const uint8_t*>(original);
    const std::vector<uint8_t> before(bytes, bytes + nativeSize);
    modmesh::BuiltSection built;
    built.sourceMaterialName = label; built.customMaterial = custom; built.forceWhite = forceWhite;
    built.customDiffuse = {{.7f, .51f, .39f, 1.f}};
    availableImage = withImage ? &image : nullptr;
    nglMeshSection section{original}; nglMesh mesh{&owner};
    const unsigned allocationStart = allocations;
    modRetargetCustomMaterial(&section, built, "fixture.fbx", 0, &mesh);
    require(std::memcmp(original, before.data(), nativeSize) == 0, "shared native material changed");
    if (!custom && !withImage && !forceWhite) {
        require(section.Material == original && allocations == allocationStart,
                "missing image replaced native material with white");
    } else {
        require(section.Material != original && allocations == allocationStart + 1,
                "private material clone missing");
        const auto* changed = section.Material;
        require(changed->File == &owner && changed->m_shader == &shader,
                "clone owner or shader changed");
        require(changed->NextMaterial == nullptr, "private clone entered native material chain");
        require(changed->Name->text == label, "authored material identity missing");
        const auto* expectedTexture = forceWhite || !withImage ? &white : &image;
        require(read<nglTexture*>(changed, layout.texture) == expectedTexture,
                "wrong diffuse texture binding");
        const auto* diffuseName = read<tlFixedString*>(changed, layout.textureName);
        require(diffuseName && diffuseName->text == (forceWhite || !withImage ? "nglwhite" : "authored-image"),
                "diffuse texture name does not describe binding");
        if (person) {
            for (size_t c = 0; c < 4; ++c) {
                const float expected = withImage || forceWhite ? 1.f : built.customDiffuse[c];
                require(read<float>(changed, 0x28 + c * 4) == expected,
                        "custom flat color or textured white tint is incorrect");
            }
            require(read<uint32_t>(changed, 0x3c) == (custom ? 0u : 1u)
                    && read<uint32_t>(changed, 0x40) == (custom ? 0u : 1u),
                    "custom albedo inherited native sphere-map effects");
        }
        // Check every shader byte outside the documented retarget slots:
        // lighting, outlines, blending, detail binding, and static flags.
        for (size_t i = 0x18; i < nativeSize; ++i) {
            const bool textureSlot = (i >= layout.textureName && i < layout.textureName + 4)
                || (i >= layout.texture && i < layout.texture + 4);
            const bool tintSlot = person && i >= 0x28 && i < 0x38;
            const bool sphereSlot = person && custom && i >= 0x3c && i < 0x44;
            if (!textureSlot && !tintSlot && !sphereSlot)
                require(reinterpret_cast<const uint8_t*>(changed)[i] == before[i],
                        "retarget changed native lighting/outline/blend/detail/static fields");
        }
        require(modMeshBufferOwners[owner.FileBuf.Buf] == &owner, "private material owner untracked");
        section.Material = original;
        modRetargetCustomMaterial(&section, built, "fixture.fbx", 0, &mesh);
        require(section.Material == changed && allocations == allocationStart + 1,
                "same source material did not reuse its private clone");
    }
    for (auto& file : modCustomMaterialFiles)
        for (auto& record : file.second.materials) tlMemFree(record.second->material);
    modCustomMaterialFiles.clear(); modMeshBufferOwners.clear(); std::free(original);
    std::printf("PASS %s\n", label);
}
int main() {
    try {
        run("flat-skin", "usperson", true, false);
        run("textured-custom", "usperson", true, true);
        run("flat-solid", "uspersonsolid", true, false);
        run("flat-morphable", "uspersonmorphable", true, false);
        run("explicit-white-custom", "usperson", true, false, true);
        run("native-person-image-pin", "usperson", false, true);
        run("native-static-image-pin", "us_decal3d", false, true);
        run("native-static-missing-image", "us_decal3d", false, false);
        std::puts("PASS production retarget: isolated custom albedos, preserved native fields, private cloning and cache reuse");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what()); return 1;
    }
}
'@

$cpp = Join-Path $output 'custom_material_retarget.cpp'
$exe = Join-Path $output 'custom_material_retarget.exe'
[IO.File]::WriteAllText($cpp, $prefix + "`n" + $function + "`n" + $suffix)
& $Compiler '-std=c++17' '-O0' '-static' '-I' (Join-Path $repo 'src') $cpp '-o' $exe
if ($LASTEXITCODE -ne 0) { throw "Material retarget probe compilation failed: $LASTEXITCODE" }
& $exe
if ($LASTEXITCODE -ne 0) { throw "Material retarget check failed: $LASTEXITCODE" }
