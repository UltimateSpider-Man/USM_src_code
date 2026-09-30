// Execute the original 32-bit PC lane return instructions with a fixture inode.
// Build: i686-w64-mingw32-g++ -std=c++17 -Wall -Wextra -Werror -O2
//   test/xbpack_v10_lane_probe.cpp -o build/xbpack_v10_lane_probe.exe
// Run with the untouched Ultimate_prerelease_original.exe as its only argument.
#include "../src/xbpack_v10_ped_patch.h"
#include <windows.h>
#include <array>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

static_assert(sizeof(void *) == 4, "native instruction probe requires x86");
namespace {
std::uint32_t word(const std::vector<std::uint8_t> &b, std::size_t p) {
    if (p + 4 > b.size()) throw "truncated PE";
    std::uint32_t v; std::memcpy(&v, b.data() + p, 4); return v;
}
std::uint16_t half(const std::vector<std::uint8_t> &b, std::size_t p) {
    if (p + 2 > b.size()) throw "truncated PE";
    std::uint16_t v; std::memcpy(&v, b.data() + p, 2); return v;
}
std::vector<std::uint8_t> native_code(const char *path) {
    std::ifstream in(path, std::ios::binary);
    const std::vector<std::uint8_t> pe((std::istreambuf_iterator<char>(in)), {});
    const auto h = word(pe, 0x3c), base = word(pe, h + 52);
    const auto sh = h + 24 + half(pe, h + 20);
    for (unsigned i = 0; i < half(pe, h + 6); ++i) {
        const auto s = sh + i * 40, va = base + word(pe, s + 12);
        const auto bytes = word(pe, s + 16), off = word(pe, s + 20);
        const auto begin = xbpack::v10_ped::lane_code_begin;
        const auto size = xbpack::v10_ped::lane_code_size;
        if (va <= begin && begin - va <= bytes && size <= bytes - (begin - va)) {
            const auto pos = off + begin - va;
            if (pos + size > pe.size()) throw "truncated code section";
            return {pe.begin() + pos, pe.begin() + pos + size};
        }
    }
    throw "lane code section absent";
}
int evaluate(const std::vector<std::uint8_t> &code, std::uint32_t flags) {
    constexpr auto start = 0x0070D792u - xbpack::v10_ped::lane_code_begin;
    constexpr auto count = 0x0070D7A6u - 0x0070D792u;
    auto *fn = static_cast<std::uint8_t *>(VirtualAlloc(nullptr, count + 1,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!fn) throw "executable allocation failed";
    std::memcpy(fn, code.data() + start, count); fn[count] = 0xc3;
    FlushInstructionCache(GetCurrentProcess(), fn, count + 1);
    std::array<std::uint8_t, 0x20> inode {};
    std::memcpy(inode.data() + 0x1c, &flags, sizeof(flags));
    int result;
    asm volatile("call *%2" : "=a"(result) : "S"(inode.data()), "r"(fn)
        : "ecx", "edx", "cc", "memory");
    VirtualFree(fn, 0, MEM_RELEASE);
    return result;
}
void require(bool ok, const char *message) { if (!ok) throw message; }
}
int main(int argc, char **argv) {
    try {
        require(argc == 2, "expected original PE path");
        auto code = native_code(argv[1]);
        require(evaluate(code, 0) == 5 && evaluate(code, 0x40) == 1,
                "baseline does not distinguish full/lite pedestrians");
        require(evaluate(code, 0x83) == 5 && evaluate(code, 0xc3) == 1,
                "baseline does not reproduce observed live flags");
        require(xbpack::v10_ped::apply_lanes(code.data(), code.size()), "production patch rejected baseline");
        for (auto flags : {0u, 0x40u, 0x83u, 0xc3u})
            require(evaluate(code, flags) == 1, "patched native return is not beta SUCCESS");
        const auto installed = code;
        require(xbpack::v10_ped::apply_lanes(code.data(), code.size()) && code == installed,
                "patch is not idempotent");
        code[0x7a3] ^= 0xff;
        const auto unexpected = code;
        require(!xbpack::v10_ped::apply_lanes(code.data(), code.size()) && code == unexpected,
                "unexpected binary was modified");
        std::puts("PASS: native baseline 5/1; production lane patch 1/1; idempotence and signature rejection");
        return 0;
    } catch (const char *message) { std::fprintf(stderr, "FAIL: %s\n", message); return 1; }
}
