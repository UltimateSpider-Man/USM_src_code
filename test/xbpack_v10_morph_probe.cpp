// Offline native x86 morph regression; never starts the game or calls imports.
// g++ -std=c++17 -O2 -static test/xbpack_v10_morph_probe.cpp -o build/xbpack_v10_morph_probe.exe
// Run with original PE and fixtures emitted by xbpack_v10_morph_assets.py.
#include "../src/ngl_xbox_morph_remap.h"
#include <windows.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <algorithm>

static_assert(sizeof(void *) == 4, "Native x86 probe requires a 32-bit compiler");
namespace {
int failures = 0;
using apply_fn = void (__cdecl *)(void *, uint32_t, uint32_t, void *, uint32_t, uint32_t, float);
void check(bool value, const char *name) {
    if (!value) { ++failures; std::printf("FAIL: %s\n",name); }
}
std::vector<uint8_t> read(const char *path) {
    std::ifstream file(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
}
uint32_t u32(const uint8_t *p) {
    uint32_t value; std::memcpy(&value,p,4); return value;
}
apply_fn map_apply(const char *path) {
    const auto bytes=read(path);
    if(bytes.size()<64 || u32(bytes.data()+60)>bytes.size()-sizeof(IMAGE_NT_HEADERS32)) return nullptr;
    const auto *pe=reinterpret_cast<const IMAGE_NT_HEADERS32 *>(bytes.data()+u32(bytes.data()+60));
    if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->OptionalHeader.ImageBase!=0x400000) return nullptr;
    const auto *sections=IMAGE_FIRST_SECTION(pe);
    if(reinterpret_cast<const uint8_t *>(sections+pe->FileHeader.NumberOfSections)>bytes.data()+bytes.size()) return nullptr;
    constexpr uint32_t begin=0x140d0, size=0x282;
    for(unsigned i=0;i<pe->FileHeader.NumberOfSections;++i) {
        const auto &s=sections[i];
        if(begin<s.VirtualAddress || begin-s.VirtualAddress+size>s.SizeOfRawData) continue;
        const size_t offset=size_t(s.PointerToRawData)+begin-s.VirtualAddress;
        if(offset+size>bytes.size()) return nullptr;
        uint32_t fnv=2166136261u;
        for(size_t p=offset;p<offset+size;++p) fnv=(fnv^bytes[p])*16777619u;
        if(fnv!=0xb1e0b380u) return nullptr;
        auto *code=VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
        if(!code) return nullptr;
        std::memcpy(code,bytes.data()+offset,size);
        return reinterpret_cast<apply_fn>(code);
    }
    return nullptr;
}
struct vertex { float position[3]; uint32_t untouched[13]; };
static_assert(sizeof(vertex)==64);
void apply(apply_fn native, std::vector<vertex> &vertices, std::vector<uint8_t> &stream) {
    uint32_t section[0x60/4]{};
    section[0x38/4]=static_cast<uint32_t>(vertices.size()-2);
    section[0x4c/4]=reinterpret_cast<uint32_t>(vertices.data()+1);
    uint32_t vertex_def[2]{0,reinterpret_cast<uint32_t>(section)};
    xbox_morph::section morph{};
    morph.vertex_count=section[0x38/4]; morph.component_mask=1;
    morph.streams[0][0]=reinterpret_cast<uint32_t>(stream.data());
    // With non-null source and mask1, this slice only touches fixture data;
    // the absolute constant used by the separate scale-only path is skipped.
    native(&morph,1,0,vertex_def,0,0,0.375f);
}
vertex initial(uint32_t i) {
    vertex result{};
    result.position[0]=float(i)*0.125f+7;
    result.position[1]=float(i)*-0.25f-2;
    result.position[2]=float(i)*0.0625f+11;
    std::fill(std::begin(result.untouched),std::end(result.untouched),0xA5C39E71u);
    return result;
}
void guards() {
    std::vector<uint8_t> output{17,29,43}; const auto pristine=output;
    const uint8_t empty[4]{};
    check(!xbox_morph::remap_positions(nullptr,4,1,{0},output),"null input");
    check(!xbox_morph::remap_positions(empty,3,1,{0},output),"short run header");
    check(!xbox_morph::remap_positions(empty,4,1,{1},output),"invalid source vertex");
    const uint8_t count_overflow[4]{2,0,0,0};
    check(!xbox_morph::remap_positions(count_overflow,4,1,{0},output),"run exceeds source vertices");
    const uint8_t skip_overflow[4]{0,0,2,0};
    check(!xbox_morph::remap_positions(skip_overflow,4,1,{0},output),"skip exceeds source vertices");
    const uint8_t truncated_values[4]{1,0,0,0};
    check(!xbox_morph::remap_positions(truncated_values,4,1,{0},output),"truncated float3 payload");
    const uint8_t unterminated[4]{0,0,1,0};
    check(!xbox_morph::remap_positions(unterminated,4,1,{0},output),"unterminated run sequence");
    check(output==pristine,"all rejected inputs leave output unchanged");
    check(xbox_morph::remap_positions(empty,4,1,{0,0},output) && output==std::vector<uint8_t>(4),"empty deltas remain empty");
}
}
int main(int argc,char **argv) {
    if(argc!=3) { std::puts("usage: probe original.exe morph-fixtures.bin"); return 2; }
    auto native=map_apply(argv[1]);
    if(!native) { std::puts("Unsupported PE/native morph slice"); return 2; }
    guards();
    const auto fixture=read(argv[2]);
    if(fixture.size()<8 || u32(fixture.data())!=0x31464d58) return 2;
    size_t offset=8; uint32_t compared=0, old_mismatch=0;
    const auto count=u32(fixture.data()+4);
    for(uint32_t record=0;record<count;++record) {
        if(offset+8>fixture.size()) return 2;
        const auto n=u32(fixture.data()+offset), size=u32(fixture.data()+offset+4); offset+=8;
        if(n>65533 || offset+size>fixture.size()) return 2;
        std::vector<uint8_t> stream(fixture.begin()+offset,fixture.begin()+offset+size); offset+=size;
        std::vector<uint32_t> map;
        for(uint32_t i=n;i>0;--i) map.push_back(i-1);
        if(n) { map.push_back(n/2); map.push_back(0); }
        std::vector<uint8_t> remapped;
        check(xbox_morph::remap_positions(stream.data(),stream.size(),n,map,remapped),"real asset remaps");
        if(remapped.empty()) continue;
        std::vector<vertex> source(n+2,initial(0x10000)), target(map.size()+2,initial(0x10000));
        for(uint32_t i=0;i<n;++i) source[i+1]=initial(i);
        for(size_t i=0;i<map.size();++i) target[i+1]=initial(map[i]);
        auto old=target;
        apply(native,source,stream); apply(native,target,remapped); apply(native,old,stream);
        for(size_t i=0;i<map.size();++i) {
            check(std::memcmp(&target[i+1],&source[map[i]+1],sizeof(vertex))==0,"native weighted geometry and untouched channels match");
            compared++;
        }
        bool old_diff=false;
        for(size_t i=0;i<map.size();++i)
            old_diff |= std::memcmp(&old[i+1],&source[map[i]+1],12)!=0;
        old_mismatch+=old_diff;
        const auto sentinel=initial(0x10000);
        check(std::memcmp(&target.front(),&sentinel,sizeof(vertex))==0 &&
              std::memcmp(&target.back(),&sentinel,sizeof(vertex))==0,"native apply preserves vertex guards");
    }
    check(offset==fixture.size(),"fixture consumed exactly");
    check(count>0 && old_mismatch>0,"unchanged old stream fails vertex-order regression");
    std::printf("streams=%u vertices=%u old_stream_failures=%u new_failures=%d\n",count,compared,old_mismatch,failures);
    VirtualFree(reinterpret_cast<void *>(native),0,MEM_RELEASE);
    return failures?1:0;
}
