// Native x86 ABI regression probe. No game startup or imported PE calls.
// Only the decoder code slice is mapped, at an arbitrary address.
// g++ -std=c++17 -O2 -static \
//     test/xbpack_v10_po_probe.cpp -o build/xbpack_v10_po_probe.exe
// Run: build/xbpack_v10_po_probe.exe build/Ultimate_prerelease_original.exe
#include "../src/xbpack_v10_po_patch.h"
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

static_assert(sizeof(void *) == 4, "This probe executes 32-bit PC instructions");

namespace {
int failures = 0;
std::uint8_t *mapped_code = nullptr;
template<class T> T native(std::uintptr_t address)
{
    return reinterpret_cast<T>(mapped_code+address-xbpack::v10_po::code_begin);
}
void check(bool ok, const char *message)
{
    if (!ok) { std::printf("FAIL: %s\n", message); ++failures; }
}

std::uint8_t *map_game(const char *path)
{
    std::ifstream file(path, std::ios::binary);
    std::vector<char> bytes{std::istreambuf_iterator<char>(file), {}};
    if (bytes.size() < sizeof(IMAGE_DOS_HEADER)) return nullptr;
    const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(bytes.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 ||
        std::size_t(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS32) > bytes.size())
        return nullptr;
    const auto *pe = reinterpret_cast<const IMAGE_NT_HEADERS32 *>(bytes.data() + dos->e_lfanew);
    if (pe->Signature != IMAGE_NT_SIGNATURE ||
        pe->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        pe->OptionalHeader.ImageBase != 0x400000) return nullptr;
    // These selected paths use stack/fixture data and relative local code,
    // without imports or absolute game data, so the interval can relocate.
    constexpr auto target_begin=xbpack::v10_po::code_begin;
    constexpr auto target_size=xbpack::v10_po::code_size;
    auto *image = static_cast<std::uint8_t *>(VirtualAlloc(
        nullptr, target_size,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!image) { std::printf("VirtualAlloc failed: %lu\n",GetLastError()); return nullptr; }
    const auto *section = IMAGE_FIRST_SECTION(pe);
    const auto table_end = reinterpret_cast<const char *>(section + pe->FileHeader.NumberOfSections);
    if (table_end > bytes.data() + bytes.size()) return nullptr;
    for (unsigned i = 0; i < pe->FileHeader.NumberOfSections; ++i) {
        const auto &s = section[i];
        if (std::uint64_t(s.PointerToRawData) + s.SizeOfRawData > bytes.size() ||
            std::uint64_t(s.VirtualAddress) + s.SizeOfRawData > pe->OptionalHeader.SizeOfImage)
            { std::printf("Invalid section %u raw=%lu size=%lu virtual=%lu image=%lu\n",i,s.PointerToRawData,s.SizeOfRawData,s.VirtualAddress,pe->OptionalHeader.SizeOfImage); return nullptr; }
        const auto begin=pe->OptionalHeader.ImageBase+s.VirtualAddress;
        const auto end=begin+s.SizeOfRawData;
        const auto copy_begin=begin>target_begin?begin:target_begin;
        const auto copy_end=end<target_begin+target_size?end:target_begin+target_size;
        if(copy_begin<copy_end)
            std::memcpy(image+copy_begin-target_begin,bytes.data()+s.PointerToRawData+copy_begin-begin,copy_end-copy_begin);
    }
    return image;
}

struct fixture {
    alignas(16) std::uint8_t source[512]{};
    alignas(16) std::uint8_t base[256]{};
    alignas(16) std::uint8_t metadata[256]{};
    alignas(16) std::uint8_t state[256]{};
    alignas(16) std::uint8_t cache[256]{};
    std::uintptr_t animation[0x64 / 4]{};
    std::uint32_t mask[2]{1, 0};
    std::uint32_t info[0x30 / 4]{};
    void *metadata_cursor = metadata;
    void *base_cursor = base + 4;
    std::uintptr_t context[4]{};
    fixture() {
        animation[0x60 / 4] = reinterpret_cast<std::uintptr_t>(mask);
        context[0] = reinterpret_cast<std::uintptr_t>(animation);
        context[1] = reinterpret_cast<std::uintptr_t>(info);
        context[2] = reinterpret_cast<std::uintptr_t>(&metadata_cursor);
        context[3] = reinterpret_cast<std::uintptr_t>(&base_cursor);
    }
};

using control_fn = int (__thiscall *)(void *, std::uintptr_t *, void **, void **, unsigned);
using update_fn = int (__thiscall *)(void *, std::uintptr_t *, void **, void **, void *, int, unsigned, int);
using apply_fn = int (__thiscall *)(void *, std::uintptr_t *, void *, void **, void *, void *);
using extract_fn = int (__thiscall *)(void *, std::uintptr_t *, void *, void *, void *);
using blend_fn = int (__thiscall *)(void *, std::uintptr_t *, void *, void *, void *, void *, void *);

void raw_source_probe(std::uintptr_t address, bool trajectory)
{
    // Xbox raw PO control at 0x36A8D0 / trajectory 0x36AE50 aligns each
    // active raw source to 16, stores that pointer, then advances frames*32.
    // Sparse tracks straddle a mask-word boundary and must consume no data
    // for inactive track 32. The private PC pointer cache remains 4-byte.
    fixture f;
    f.info[0x24 / 4] = 31;
    f.info[0x28 / 4] = 3;
    f.mask[0] = 0x80000000;
    f.mask[1] = 2;
    void *state = f.state;
    void *source = f.source + 4;
    native<control_fn>(address)(nullptr, f.context, &state, &source, 2);
    const auto *pointers = reinterpret_cast<const std::uintptr_t *>(f.state);
    check(pointers[0] == reinterpret_cast<std::uintptr_t>(f.source + 16), "raw first active source starts at +16");
    check(pointers[1] == reinterpret_cast<std::uintptr_t>(f.source + 80), "raw second active source starts at +80");
    check(source == f.source + 144, "raw frame source advances 2 tracks * 2 frames * 32");
    check(state == f.state + 8, "raw pointer state retains PC 4-byte stride");
    if (trajectory)
        check(f.base_cursor == f.base + 80, "raw trajectory base aligns to16 and advances32 per active track");

    fixture inactive;
    inactive.info[0x28 / 4] = 1;
    inactive.mask[0] = 0;
    state = inactive.state;
    source = inactive.source + 4;
    native<control_fn>(address)(nullptr, inactive.context, &state, &source, 2);
    check(source == inactive.source + 4 && state == inactive.state, "inactive raw source consumes no bytes");
}

void trajectory_boundaries()
{
    // Zero tracks isolate serialized base-cursor alignment from decoding.
    // Xbox's entropy trajectory consumes a 4-byte scalar header then aligns
    // the base PO; raw trajectory has no scalar header. Both finish at+16
    // when given a base stream at+4. Metadata/private caches remain native.
    for (auto address : {0x788790u, 0x788CB0u}) {
        fixture f; void *state=f.state; void *source=f.source;
        native<control_fn>(address)(nullptr,f.context,&state,&source,1);
        check(f.base_cursor == f.base + 16,"trajectory control aligns serialized base");
    }
    for (auto address : {0x788830u, 0x788D90u}) {
        fixture f; void *state=f.state; void *cache=f.cache+4;
        native<update_fn>(address)(nullptr,f.context,&cache,&state,nullptr,0,0,0);
        check(f.base_cursor == f.base+16,"trajectory update aligns serialized base");
        check(cache == f.cache+4,"trajectory update retains PC private-cache alignment");
    }
    for (auto address : {0x788990u,0x788F80u}) {
        fixture f; void *cache=f.cache+4;
        native<apply_fn>(address)(nullptr,f.context,nullptr,&cache,nullptr,nullptr);
        check(f.base_cursor == f.base+16,"trajectory apply aligns serialized base");
        check(cache == f.cache+4,"trajectory apply retains PC private-cache alignment");
    }
    for (auto address : {0x788A90u,0x7890A0u}) {
        fixture f;
        native<extract_fn>(address)(nullptr,f.context,nullptr,nullptr,nullptr);
        check(f.base_cursor == f.base+16,"trajectory extract aligns serialized base");
    }
    for (auto address : {0x788B50u,0x789190u}) {
        fixture f;
        native<blend_fn>(address)(nullptr,f.context,nullptr,nullptr,nullptr,nullptr,nullptr);
        check(f.base_cursor == f.base+16,"trajectory blend aligns serialized base");
    }
}

void raw_decode_probe()
{
    fixture f;
    f.info[0x28/4]=1;
    const float frames[16]={0,0,0,1,11,22,33,999, 0,0,1,0,44,55,66,999};
    std::memcpy(f.source+16,frames,sizeof(frames));
    *reinterpret_cast<void **>(f.state)=f.source+16;
    void *state=f.state; void *cache=f.cache+4;
    native<update_fn>(0x788070)(nullptr,f.context,&cache,&state,nullptr,0,2,28);
    check(std::memcmp(f.cache+4,frames,28)==0,"raw first frame decodes seven payload floats");
    check(std::memcmp(f.cache+32,frames+8,28)==0,"raw second frame skips serialized padding");
    check(cache==f.cache+32,"raw decoded cache retains 28-byte PC stride");
    check(state==f.state+4,"raw decode retains 4-byte PC pointer state");
}

void patch_guards(std::uint8_t *code)
{
    using namespace xbpack::v10_po;
    const std::vector<std::uint8_t> original(code,code+code_size);
    const instruction_patch *failed=nullptr;
    check(!apply(nullptr,code_size,&failed),"null code buffer is rejected");
    check(failed==&patches[0],"null buffer reports the first rejected instruction");
    check(!apply(code,0,&failed),"empty code buffer is rejected");
    const auto &last=patches[patch_count-1];
    const auto last_offset=std::size_t(last.address-code_begin);
    check(!apply(code,last_offset+last.size-1,&failed),"buffer truncated inside final instruction is rejected");
    check(failed==&last,"short buffer reports the incomplete instruction");
    check(std::memcmp(code,original.data(),code_size)==0,"short-buffer rejection performs no writes anywhere in code slice");

    // Corrupt the LAST signature while the early signatures are still stock.
    // An implementation that patches during validation would leave a partial
    // installation. Compare the entire slice, including unpatched gaps.
    code[last_offset]^=1;
    const std::vector<std::uint8_t> damaged(code,code+code_size);
    check(!apply(code,code_size,&failed),"unexpected instruction is rejected");
    check(failed==&last,"signature rejection reports the changed instruction");
    check(std::memcmp(code,damaged.data(),code_size)==0,"signature rejection performs no writes anywhere in code slice");
    std::memcpy(code,original.data(),code_size);
}
}

int main(int argc,char **argv)
{
    if(argc!=2) { std::puts("Usage: xbpack_v10_po_probe.exe Ultimate_prerelease_original.exe"); return 2; }
    auto *image=map_game(argv[1]);
    if(!image) { std::puts("Cannot load/map the original PC decoder code"); return 2; }
    mapped_code=image;
    auto *code=mapped_code;
    patch_guards(code);
    const xbpack::v10_po::instruction_patch *failed=nullptr;
    if(!xbpack::v10_po::apply(code,xbpack::v10_po::code_size,&failed)) {
        std::printf("PE signature mismatch: %s\n",failed?failed->purpose:"unknown"); return 2;
    }
    check(xbpack::v10_po::apply(code,xbpack::v10_po::code_size),"patch application is idempotent");
    FlushInstructionCache(GetCurrentProcess(),code,xbpack::v10_po::code_size);
    raw_source_probe(0x787FE0,false);
    raw_source_probe(0x788790,true);
    trajectory_boundaries();
    raw_decode_probe();
    VirtualFree(image,0,MEM_RELEASE);
    std::printf("Native PO regression: %d failures (%u guarded instructions).\n",failures,unsigned(xbpack::v10_po::patch_count));
    return failures?1:0;
}
