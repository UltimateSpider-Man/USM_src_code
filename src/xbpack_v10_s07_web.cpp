#include "xbpack_v10_s07_web.h"

#if defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
#include "script_executable.h"
#include "script_object.h"
#include "vm_executable.h"
#include "func_wrapper.h"
#include "log.h"
#include "memory.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>

namespace {
// S07's IGC1 Web_Shoot_Left marker can disappear without ATTACK_END.
// Its script then keeps a radius-0.3 web attached to the hero during play.
// Finish the existing helper when IGC1 ends; retain its fade/destruction.
constexpr std::uint16_t original_finish[] = {
    0x1D01, 0x3F80, 0, 0x1C08, 1, 0x005C, 0x1E04, 0
};
constexpr std::uint16_t original_cleanup[] = {
    0x1D01, 0, 0, 0x1D06, 0xFFF8, 0x1C07, 0x000C,
    0x1D06, 0xFFFC, 0x1D07, 0, 0x040A, 0x0017, 3,
    0x1D06, 0xFFFC, 0x1D06, 0xFFF8, 0x0709, 4, 5,
    0x2104, 0xFFFC, 0x1E04, 4
};
constexpr std::uint16_t finish_code[] = {
    0x1D01, 0x3F80, 0, 0x1C08, 1, 0x005C, // IGC1 finished = true.
    0x0F09, 1, 21,                         // Cancel suspended web initialization.
    0x1D08, 1, 0x007C,                     // Push its web helper.
    0x0509, 4, 4,                          // Call helper.cleanup(this).
    0x1E04, 0                             // Callee consumes this argument.
};
constexpr std::uint16_t cleanup_guards[] = {
    0x1D06, 0xFFFC, 0x0205, 66,                  // No helper.
    0x1D06, 0xFFFC, 0x1D07, 0x000C, 0x0205, 54  // Already inactive.
}; // Each branch reaches the original SPA -4 / RET 4, balancing the argument.
constexpr std::uint16_t tube_guard[] = {
    0x1D06, 0xFFFC, 0x1D07, 0, 0x0205, 28 // No tube: stop after clearing active.
};

struct function_backup {
    vm_executable *function;
    std::uint16_t *buffer;
    int length;
    std::uint32_t flags;

    explicit function_backup(vm_executable *fn)
        : function(fn), buffer(fn->buffer), length(fn->buffer_len), flags(fn->flags) {}
    void restore() const {
        function->buffer = buffer;
        function->buffer_len = length;
        function->flags = flags;
    }
};

struct replacement {
    function_backup finish;
    function_backup cleanup;
    std::array<std::uint16_t, std::size(finish_code)> finish_buffer;
    std::array<std::uint16_t, 41> cleanup_buffer;

    replacement(vm_executable *end, vm_executable *stop) : finish(end), cleanup(stop) {
        std::memcpy(finish_buffer.data(), finish_code, sizeof(finish_code));
        std::memcpy(cleanup_buffer.data(), cleanup_guards, sizeof(cleanup_guards));
        std::memcpy(cleanup_buffer.data() + std::size(cleanup_guards),
                    original_cleanup, 7 * sizeof(std::uint16_t));
        std::memcpy(cleanup_buffer.data() + std::size(cleanup_guards) + 7,
                    tube_guard, sizeof(tube_guard));
        std::memcpy(cleanup_buffer.data() + std::size(cleanup_guards) + 7 + std::size(tube_guard),
                    original_cleanup + 7, sizeof(original_cleanup) - 7 * sizeof(std::uint16_t));
    }
};

std::map<script_executable *, std::unique_ptr<replacement>> replacements;

void __cdecl release_script_mash(script_executable *script)
{
    // Both native callsites have already destroyed every instance/thread.
    // The mash may remain cached, so restore raw code AND its unlinked flags.
    auto found = replacements.find(script);
    if (found != replacements.end()) {
        found->second->finish.restore();
        found->second->cleanup.restore();
        replacements.erase(found);
        sp_log("[xbpack] V10 S07 web bytecode released");
    }
    CDECL_CALL(0x004C2060, script);
}

template<std::size_t N>
bool matches(vm_executable *fn, const std::uint16_t (&code)[N])
{
    return fn != nullptr && fn->buffer != nullptr && fn->buffer_len == int(N) &&
           !fn->is_linked() && std::memcmp(fn->buffer, code, sizeof(code)) == 0;
}
} // namespace

void xbpack_v10_s07_web_unmash(script_executable *script)
{
    auto *global = script->global_script_object;
    if (global == nullptr || global->name.source_hash_code != 0x1986401Bu ||
        replacements.count(script) != 0)
        return;

    auto **sorted = script->script_objects_by_name;
    auto *helper = sorted != nullptr && script->total_script_objects > 4 ? sorted[4] : nullptr;
    if (helper == nullptr || sorted[1] != global || helper->name.source_hash_code != 0x2AF7D36Du ||
        global->total_funcs <= 51 || helper->total_funcs <= 5 ||
        global->funcs == nullptr || helper->funcs == nullptr) {
        sp_log("[xbpack] V10 S07 web cleanup rejected: script layout mismatch");
        return;
    }
    auto *finish = global->funcs[51];
    auto *cleanup = helper->funcs[4];
    if (!matches(finish, original_finish) || !matches(cleanup, original_cleanup) ||
        finish->name.source_hash_code != 0xAEFFC549u ||
        finish->parms_stacksize != 0 || cleanup->parms_stacksize != 4) {
        sp_log("[xbpack] V10 S07 web cleanup rejected: bytecode signature mismatch");
        return;
    }

    auto owner = std::make_unique<replacement>(finish, cleanup);
    auto *code = owner.get();
    replacements.emplace(script, std::move(owner));
    finish->buffer = code->finish_buffer.data();
    finish->buffer_len = int(code->finish_buffer.size());
    cleanup->buffer = code->cleanup_buffer.data();
    cleanup->buffer_len = int(code->cleanup_buffer.size());

    // Fresh loads use the normal linker. A cached executable skips that pass;
    // link only these two new buffers, leaving all other cached code intact.
    if (script->is_linked()) {
        THISCALL(0x0059F000, finish, script);
        THISCALL(0x0059F000, cleanup, script);
    }
    sp_log("[xbpack] V10 S07 intro web cleanup installed (cached=%u)", script->is_linked());
}

bool xbpack_v10_s07_web_patch()
{
    constexpr std::uintptr_t sites[] = {0x005B04B4u, 0x005B06DFu};
    for (auto site : sites) {
        auto *call = reinterpret_cast<const std::uint8_t *>(site);
        std::int32_t displacement;
        std::memcpy(&displacement, call + 1, sizeof(displacement));
        const auto target = site + 5u + displacement;
        if (call[0] != 0xE8 || (target != 0x004C2060u &&
            target != reinterpret_cast<std::uintptr_t>(&release_script_mash))) {
            sp_log("[xbpack] V10 S07 web release hook rejected at 0x%08X", unsigned(site));
            return false;
        }
    }
    for (auto site : sites)
        REDIRECT(site, release_script_mash);
    return true;
}
#endif
