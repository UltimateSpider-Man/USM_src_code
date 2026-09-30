#include "xbpack.h"

#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)

#include "xbpack_v10_scene_pack.h"
#include "log.h"
#include "memory.h"
#include "parse_generic_mash.h"
#include "resource_directory.h"
#include "resource_pack_standalone.h"

#include <windows.h>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

namespace
{
namespace scene_pack = xbpack::v10_scene_pack;

// These handles must belong to the native NFL pool. The local NFL patch is
// below the xbpack boot early return and is not active in this configuration.
using native_open_fn = int (__cdecl *)(int, const char *);
using native_close_fn = void (__cdecl *)(int);
using native_unload_fn = void (__fastcall *)(resource_pack_standalone *, void *);
constexpr std::uintptr_t native_open = 0x0079E490u;
constexpr std::uintptr_t native_close = 0x0079F3F0u;

// POD storage survives both the scene unload and the native atexit path.
char owned_temporary_path[scene_pack::native_path_capacity] {};
resource_pack_standalone *temporary_owner = nullptr;

struct file_handle {
    HANDLE value = INVALID_HANDLE_VALUE;
    explicit file_handle(HANDLE file) : value(file) {}
    ~file_handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    file_handle(const file_handle &) = delete;
    file_handle &operator=(const file_handle &) = delete;
};

struct native_resources {
    int file = -1;
    void *mash = nullptr;
    ~native_resources()
    {
        if (file != -1) reinterpret_cast<native_close_fn>(native_close)(file);
        if (mash != nullptr) mem_freealign(mash);
    }
};

bool read_at(HANDLE file, std::uint32_t offset, void *destination, DWORD size)
{
    LARGE_INTEGER position {};
    position.QuadPart = offset;
    DWORD count = 0;
    return SetFilePointerEx(file, position, nullptr, FILE_BEGIN) != 0 &&
           ReadFile(file, destination, size, &count, nullptr) != 0 && count == size;
}

void remove_owned_temporary(resource_pack_standalone *pack)
{
    if (temporary_owner != pack || owned_temporary_path[0] == '\0') return;
    if (!DeleteFileA(owned_temporary_path)) {
        sp_log("[xbpack] V10 SCNANIMS temporary cleanup failed: Win32 %lu", GetLastError());
    }
    owned_temporary_path[0] = '\0';
    temporary_owner = nullptr;
}

void __fastcall unload_scene_pack(resource_pack_standalone *pack, void *)
{
    reinterpret_cast<native_unload_fn>(scene_pack::native_unload)(pack, nullptr);
    remove_owned_temporary(pack); // Native unload closes NFL before deletion.
}

void __fastcall destroy_scene_pack(resource_pack_standalone *pack, void *)
{
    reinterpret_cast<native_unload_fn>(scene_pack::native_destroy)(pack, nullptr);
    remove_owned_temporary(pack);
}

bool load_path(resource_pack_standalone *pack, const char *candidate)
{
    char absolute[scene_pack::native_path_capacity] {};
    const DWORD length = GetFullPathNameA(candidate, sizeof(absolute), absolute, nullptr);
    if (length == 0 || length >= sizeof(absolute)) return false;

    // Keep this read handle open until NFL opens the same file. Share only
    // reads, so a writer cannot replace the validated bytes in between.
    file_handle file(CreateFileA(absolute, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (file.value == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER file_size {};
    std::uint8_t header[0x2C] {};
    if (!GetFileSizeEx(file.value, &file_size) || file_size.QuadPart < sizeof(header) ||
        file_size.QuadPart > 0x7FFFFFFFLL || !read_at(file.value, 0, header, sizeof(header))) {
        sp_log("[xbpack] V10 SCNANIMS rejected: cannot read header: %s", absolute);
        return false;
    }
    const std::uint64_t prefix_size = std::uint64_t(scene_pack::u32(header + 0x18)) +
                                     scene_pack::u32(header + 0x1C);
    if (prefix_size < sizeof(header) || prefix_size > scene_pack::maximum_directory_prefix ||
        prefix_size > static_cast<std::uint64_t>(file_size.QuadPart)) {
        sp_log("[xbpack] V10 SCNANIMS rejected: directory range: %s", absolute);
        return false;
    }
    std::vector<std::uint8_t> prefix(static_cast<std::size_t>(prefix_size));
    scene_pack::directory_info directory;
    if (!read_at(file.value, 0, prefix.data(), static_cast<DWORD>(prefix.size())) ||
        !scene_pack::inspect_directory({prefix.data(), prefix.size()}, file_size.QuadPart, directory)) {
        sp_log("[xbpack] V10 SCNANIMS rejected: incompatible directory: %s", absolute);
        return false;
    }
    for (const auto &scene : directory.scenes) {
        std::uint8_t scene_header[0x50] {};
        if (!read_at(file.value, scene.offset, scene_header, sizeof(scene_header)) ||
            !scene_pack::inspect_scene_nodes({scene_header, sizeof(scene_header)}, scene,
                [&](std::uint32_t offset, void *data, std::size_t size) {
                    return read_at(file.value, scene.offset + offset, data, static_cast<DWORD>(size));
                })) {
            sp_log("[xbpack] V10 SCNANIMS rejected: scene 0x%08X: %s", scene.hash, absolute);
            return false;
        }
    }

    native_resources pending;
    pending.file = reinterpret_cast<native_open_fn>(native_open)(1, absolute);
    if (pending.file == -1) {
        sp_log("[xbpack] V10 SCNANIMS native streaming open failed: %s", absolute);
        return false;
    }
    pending.mash = arch_memalign(16, directory.directory_size);
    if (pending.mash == nullptr) return false;
    std::memcpy(pending.mash, prefix.data() + directory.directory_offset, directory.directory_size);

    resource_directory *parsed = nullptr;
    // Validation requires a pristine, non-polymorphic mash: no clone, class
    // table, or shared vector allocation is permitted on this path.
    const bool allocated = parse_generic_object_mash(parsed, pending.mash, nullptr,
                                                     nullptr, nullptr, 0, 0, nullptr);
    if (allocated || reinterpret_cast<std::uint8_t *>(parsed) !=
                     static_cast<std::uint8_t *>(pending.mash) + 0x10) {
        sp_log("[xbpack] V10 SCNANIMS unexpected directory parser result");
        return false;
    }
    parsed->constructor_common(nullptr, nullptr, nullptr, 0, 0);

    // Publish only after both readers and the parsed directory are ready.
    static_assert(sizeof(pack->m_header) == sizeof(header));
    std::memcpy(&pack->m_header, header, sizeof(header));
    pack->res_dir_mash = pending.mash;
    pack->res_dir = parsed;
    pack->base_offset = directory.directory_size; // Not offset + directory size.
    pack->m_filedID.field_0 = pending.file;
    pending.mash = nullptr;
    pending.file = -1;
    sp_log("[xbpack] V10 SCNANIMS ready: %u scenes, base=0x%X, path=%s",
           static_cast<unsigned>(directory.scenes.size()), directory.directory_size, absolute);
    return true;
}

bool load_embedded(resource_pack_standalone *pack)
{
    const auto original = scene_pack::embedded_pack();
    if (original.data == nullptr || original.size == 0 || original.size > MAXDWORD) return false;
    char directory[MAX_PATH] {};
    const DWORD length = GetTempPathA(sizeof(directory), directory);
    if (length == 0 || length >= sizeof(directory)) return false;
    char path[MAX_PATH] {};
    if (GetTempFileNameA(directory, "usm", 0, path) == 0) return false;

    // This path was created exclusively for this load attempt. It is never
    // an external game asset and may be deleted after failure or unload.
    struct temporary_file {
        const char *path;
        bool keep = false;
        ~temporary_file() { if (!keep) DeleteFileA(path); }
    } temporary {path};
    if (std::strlen(path) >= scene_pack::native_path_capacity) return false;
    {
        file_handle output(CreateFileA(path, GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING,
                                       FILE_ATTRIBUTE_TEMPORARY, nullptr));
        if (output.value == INVALID_HANDLE_VALUE) return false;
        DWORD written = 0;
        if (!WriteFile(output.value, original.data, static_cast<DWORD>(original.size), &written, nullptr) ||
            written != original.size) return false;
    }
    if (!load_path(pack, path)) return false;
    std::memcpy(owned_temporary_path, path, std::strlen(path) + 1);
    temporary_owner = pack;
    temporary.keep = true;
    sp_log("[xbpack] V10 SCNANIMS using the embedded original Xbox animation pack");
    return true;
}

bool __fastcall load_scene_pack(resource_pack_standalone *pack, void *, const mString *)
{
    if (pack->m_filedID.field_0 != -1 && pack->res_dir != nullptr &&
        pack->res_dir_mash != nullptr && pack->base_offset > 0) return true;
    if (pack->m_filedID.field_0 != -1 || pack->res_dir != nullptr ||
        pack->res_dir_mash != nullptr || pack->base_offset != 0) {
        unload_scene_pack(pack, nullptr);
    }

    try {
        // Native init requests packs\\scnanims_XB.PAK, while its two I/O
        // systems apply different roots. Resolve once for both readers.
        constexpr const char *relative_paths[] {
            "data\\packs\\SCNANIMS_XB.PAK", "packs\\SCNANIMS_XB.PAK",
            "extra\\SCNANIMS_XB.PAK", "SCNANIMS_XB.PAK"
        };
        for (const char *relative : relative_paths) {
            if (load_path(pack, relative)) return true;
        }
        char executable[MAX_PATH] {};
        const DWORD length = GetModuleFileNameA(nullptr, executable, sizeof(executable));
        if (length != 0 && length < sizeof(executable)) {
            if (char *slash = std::strrchr(executable, '\\')) {
                slash[1] = '\0';
                for (const char *relative : relative_paths) {
                    const auto path = std::string(executable) + relative;
                    if (load_path(pack, path.c_str())) return true;
                }
            }
        }
        if (load_embedded(pack)) return true;
    } catch (const std::exception &error) {
        sp_log("[xbpack] V10 SCNANIMS load exception: %s", error.what());
    }

    // Startup ignores native load's false return. Do not allow it to enter
    // gameplay with a null directory and fail later inside IGC2 streaming.
    sp_log("[xbpack] V10 SCNANIMS initialization failed (Win32 %lu)", GetLastError());
    MessageBoxA(nullptr,
        "OpenUSM could not open the V10 scene animations, including the embedded fallback.\n"
        "Check free space and access to the Windows temporary folder, or place SCNANIMS_XB.PAK\n"
        "in data\\packs beside the game executable, then restart. See the OpenUSM log for details.",
        "OpenUSM Xbox V10 - scene animation initialization failed", MB_OK | MB_ICONERROR);
    ExitProcess(ERROR_FILE_NOT_FOUND);
    return false;
}

struct hook {
    std::uintptr_t address;
    std::uint8_t opcode;
    std::uintptr_t original;
    std::uintptr_t replacement;
};
}

bool xbpack_v10_scene_pack_patch()
{
    const hook hooks[] {
        {scene_pack::load_call, 0xE8, scene_pack::native_load,
         reinterpret_cast<std::uintptr_t>(&load_scene_pack)},
        {scene_pack::unload_jump, 0xE9, scene_pack::native_unload,
         reinterpret_cast<std::uintptr_t>(&unload_scene_pack)},
        {scene_pack::destroy_jump, 0xE9, scene_pack::native_destroy,
         reinterpret_cast<std::uintptr_t>(&destroy_scene_pack)}
    };
    // Validate every site before changing any of the three instructions.
    for (const auto &entry : hooks) {
        const auto *instruction = reinterpret_cast<const std::uint8_t *>(entry.address);
        std::int32_t displacement = 0;
        std::memcpy(&displacement, instruction + 1, sizeof(displacement));
        const auto target = entry.address + 5u + displacement;
        if (instruction[0] != entry.opcode ||
            (target != entry.original && target != entry.replacement)) {
            sp_log("[xbpack] V10 SCNANIMS patch rejected: instruction changed at 0x%08X",
                   static_cast<unsigned>(entry.address));
            return false;
        }
    }
    for (const auto &entry : hooks) {
        const auto displacement = static_cast<std::uint32_t>(entry.replacement - entry.address - 5u);
        // Preserve the byte after each five-byte tail jump; SET_JUMP also
        // writes a sixth byte, which is unnecessary at these existing jumps.
        std::memcpy(reinterpret_cast<void *>(entry.address + 1), &displacement, sizeof(displacement));
    }
    sp_log("[xbpack] V10 standalone scene-animation loading and fallback enabled");
    return true;
}

#endif
