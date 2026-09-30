"""Exercise V10 LEGO visibility against original Xbox machine code and JM assets.

The isolated Windows 32-bit probe compiles the production helper/installer,
executes Xbox's bounds instructions as its independent oracle, and verifies
the installed native operands. No game process is launched or attached.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile


def u32(data, at):
    return struct.unpack_from('<I', data, at)[0]


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('original_exe', type=Path)
parser.add_argument('xbox_exe', type=Path)
parser.add_argument('pack', type=Path)
parser.add_argument('--cxx', default='g++')
args = parser.parse_args()
pc, xb, pack = (p.read_bytes() for p in (args.original_exe, args.xbox_exe, args.pack))
pe = u32(pc, 0x3C)
sections = pe + 24 + struct.unpack_from('<H', pc, pe + 20)[0]


def native(address, size):
    for i in range(struct.unpack_from('<H', pc, pe + 6)[0]):
        _, rva, count, raw = struct.unpack_from('<4I', pc, sections + 40 * i + 8)
        va = u32(pc, pe + 52) + rva
        if va <= address and address + size <= va + count:
            return pc[raw + address - va:raw + address - va + size]
    raise AssertionError(f'unmapped PC address {address:#x}')


def xbox(address, size):
    table = u32(xb, 0x120) - u32(xb, 0x104)
    for i in range(u32(xb, 0x11C)):
        va, _, raw, count = struct.unpack_from('<4I', xb, table + 56 * i + 4)
        if va <= address and address + size <= va + count:
            return xb[raw + address - va:raw + address - va + size]
    raise AssertionError(f'unmapped Xbox address {address:#x}')


# PC treats +11 as a fade-group index into a region-owned table. Xbox uses
# the radius nibble directly and reads bit 11 of the same word for height.
assert native(0x53A2A5, 5) == bytes.fromhex('8A5D1184DB')
assert native(0x53A2C9, 17) == bytes.fromhex('74118B969C0000008B42440FB6CBD90488')
assert native(0x53A30A, 5) == bytes.fromhex('E8315D0200')
assert native(0x560060, 8) == bytes.fromhex('8A4113C0E8062401')
assert xbox(0x1210E8, 4) == bytes.fromhex('668B4210')
assert xbox(0x1210F8, 14) == bytes.fromhex('8BC883E10FF30F10048DA8475100')
assert xbox(0x121116, 10) == bytes.fromhex('0FB64A1FC1E80B83E001')
assert native(0x921E88, 64) == xbox(0x5147A8, 64)
assert tuple(struct.unpack('<f', xbox(at, 4))[0]
             for at in (0x5122B0, 0x48F860, 0x5122BC)) == (5.0, 0.5, 1.0)

# Prove the failing record belongs to JM's scene directory and LEGO root.
assert args.pack.stem.upper() == 'JM' and u32(pack, 0) == 10
directory, payload = u32(pack, 0x18) + 16, u32(pack, 0x1C)
count = u32(pack, directory + 12) & 0xFFFFFF
parents = u32(pack, directory + 4) & 0xFFFFFF
locations = (directory + 0x2B0 + 4 * parents + 7) & ~7
scene = [struct.unpack_from('<4I', pack, locations + 16 * i)
         for i in range(count) if u32(pack, locations + 16 * i + 4) == 9]
assert scene == [(0xE17, 9, 0x3D00, 0x1ACC4)]
assert payload + scene[0][2] == 0x4A80
assert u32(pack, 0xF7D0) == 11
lego_root = 0xF7D8
meshes, materials, objects, size = struct.unpack_from('<4H', pack, lego_root + 0x10)
assert (meshes, materials, objects, size) == (68, 82, 493, 0x4FC0)
first = ((lego_root + 0x1F) & ~7) + 4 * (meshes + materials)
assert first == 0xFA48 and first + 16 * 32 == 0xFC48
records = pack[first:first + objects * 32]
assert records[16 * 32:17 * 32] == bytes.fromhex(
    '09005A0000308BC49A9995411E009BC29B0E0200181B040506071C0D0A030305')
assert sum(records[i * 32 + 0x11] != 0 for i in range(objects)) == 49
assert sum(bool(records[i * 32 + 0x11] & 8) for i in range(objects)) == 30
print('BASELINE FAIL reproduced: JM record 16 has fade index 14 under PC; '
      '49/493 records have Xbox flags in that byte.', flush=True)

# Execute the original Xbox height calculations with its absolute constants.
# Only ABI setup/output stores are added around this unchanged instruction slice.
oracle = (bytes.fromhex('8B5424040FB74210F30F104A08') + xbox(0x121116, 0x39)
          + bytes.fromhex('8B4C2408F30F11198B4C240CF30F1111C3'))
visitor = native(0x53A290, 0x306)
root = Path(__file__).resolve().parents[1]
source = (root / 'src/lego_map.cpp').read_text()
begin = source.index('#ifdef OPENUSM_XBPACK_V10\nnamespace')
source = source[begin:]
prologue = r'''
#define OPENUSM_XBPACK_V10
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
void sp_log(const char *) {}
#define CHECK(v) do { if (!(v)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#v); std::exit(1); } } while(0)
'''
test = r'''
static uint8_t probe_memory[0xA00000];
static const uint8_t records[] = {RECORD_BYTES};
int main() {
    CHECK(reinterpret_cast<uintptr_t>(probe_memory) <= 0x460000);
    CHECK(reinterpret_cast<uintptr_t>(probe_memory)+sizeof(probe_memory) >= 0x540000);
    DWORD protection;
    CHECK(VirtualProtect(reinterpret_cast<void *>(0x460000),0xE0000,PAGE_EXECUTE_READWRITE,&protection));
    auto *visitor = reinterpret_cast<uint8_t *>(0x53A290);
    auto *fade = reinterpret_cast<uint8_t *>(0x53A2A5);
    auto *bounds = reinterpret_cast<uint8_t *>(0x53A30A);
    const uint8_t original[] = {VISITOR_BYTES};
    const uint8_t oracle_code[] = {ORACLE_BYTES};
    auto *oracle_memory = reinterpret_cast<uint8_t *>(0x465000);
    std::memcpy(oracle_memory,oracle_code,sizeof(oracle_code));
    *reinterpret_cast<float *>(0x5122B0)=5.0f;
    *reinterpret_cast<float *>(0x48F860)=0.5f;
    *reinterpret_cast<float *>(0x5122BC)=1.0f;
    FlushInstructionCache(GetCurrentProcess(),oracle_memory,sizeof(oracle_code));
    auto oracle = reinterpret_cast<void (*)(const uint8_t *,float *,float *)>(oracle_memory);
    auto check_bounds = [&](const uint8_t *record) {
        std::array<uint8_t,32> before;
        std::memcpy(before.data(),record,32);
        float expected_lower,expected_upper,lower,upper;
        oracle(record,&expected_lower,&expected_upper);
        packed_vertical_bounds_v10(record,nullptr,&lower,&upper);
        CHECK(std::memcmp(&lower,&expected_lower,4)==0);
        CHECK(std::memcmp(&upper,&expected_upper,4)==0);
        CHECK(std::memcmp(before.data(),record,32)==0);
    };
    for (size_t i=0;i<sizeof(records);i+=32) check_bounds(records+i);
    for (unsigned flags : {0u,0x7FFu,0x800u,0xFFFu,0xFFFFu}) {
        for (unsigned height : {0u,1u,127u,255u}) {
            for (float y : {-1000.25f,0.0f,1000.25f}) {
                std::array<uint8_t,32> record{};
                std::memcpy(record.data()+8,&y,4);
                record[0x10]=static_cast<uint8_t>(flags);
                record[0x11]=static_cast<uint8_t>(flags>>8);
                record[0x1F]=static_cast<uint8_t>(height);
                check_bounds(record.data());
            }
        }
    }
    std::memcpy(visitor,original,sizeof(original));
    auto follows_fade_path = [&](const uint8_t *record) {
        // Preserve callee-saved EBP/EBX, then execute the actual producer and
        // native TEST BL,BL to determine whether the null table is accessed.
        uint8_t stub[]={0x55,0x53,0x8B,0x6C,0x24,0x0C,
            0,0,0,0x84,0xDB,0x0F,0x95,0xC0,0x0F,0xB6,0xC0,0x5B,0x5D,0xC3};
        std::memcpy(stub+6,fade,3);
        auto *code=reinterpret_cast<uint8_t *>(0x466000);
        std::memcpy(code,stub,sizeof(stub));
        FlushInstructionCache(GetCurrentProcess(),code,sizeof(stub));
        return reinterpret_cast<int (*)(const uint8_t *)>(code)(record);
    };
    CHECK(follows_fade_path(records+16*32)==1);
    CHECK(lego_map_xbpack_v10_patch());
    const uint8_t fixed_fade[]={0x30,0xDB,0x90};
    CHECK(std::memcmp(fade,fixed_fade,3)==0);
    CHECK(bounds[0]==0xE8);
    int32_t displacement;
    std::memcpy(&displacement,bounds+1,4);
    CHECK(bounds+5+displacement==reinterpret_cast<uint8_t *>(&packed_vertical_bounds_v10));
    for (size_t i=0;i<sizeof(records);i+=32) CHECK(follows_fade_path(records+i)==0);
    for (size_t i=0;i<sizeof(original);++i) {
        if ((i<0x15 || i>=0x18) && (i<0x7A || i>=0x7F)) CHECK(visitor[i]==original[i]);
    }
    std::array<uint8_t,sizeof(original)> patched;
    std::memcpy(patched.data(),visitor,patched.size());
    CHECK(lego_map_xbpack_v10_patch());
    CHECK(std::memcmp(visitor,patched.data(),patched.size())==0);
    for (auto *site : {fade,bounds}) {
        std::memcpy(visitor,original,sizeof(original));
        site[0]^=1;
        std::array<uint8_t,sizeof(original)> corrupt;
        std::memcpy(corrupt.data(),visitor,corrupt.size());
        CHECK(!lego_map_xbpack_v10_patch());
        CHECK(std::memcmp(visitor,corrupt.data(),corrupt.size())==0);
    }
    // An individually applied, valid site can be completed safely.
    std::memcpy(visitor,original,sizeof(original));
    std::memcpy(fade,fixed_fade,3);
    CHECK(lego_map_xbpack_v10_patch());
    CHECK(std::memcmp(visitor,patched.data(),patched.size())==0);
    std::puts("PASS: 493 real JM records and 60 boundary cases match executed Xbox bounds; no descriptor mutation; all fade branches disabled; installer idempotence, atomic rejection, and unchanged surrounding code.");
}
'''
for token, data in (('RECORD_BYTES', records), ('VISITOR_BYTES', visitor), ('ORACLE_BYTES', oracle)):
    test = test.replace(token, ','.join(str(v) for v in data))
compiler = shutil.which(args.cxx) or str(Path(args.cxx).resolve())
env = dict(os.environ)
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='xbpack-v10-lego-') as temp:
    cpp, exe = Path(temp) / 'probe.cpp', Path(temp) / 'probe.exe'
    cpp.write_text(prologue + source + test)
    subprocess.run([compiler,'-std=c++17','-O2','-static','-m32',
                    '-Wl,--image-base,0x400000','-Wl,--disable-dynamicbase',
                    str(cpp),'-o',str(exe)],env=env,check=True)
    subprocess.run([str(exe)],env=env,check=True)
print('PASS: original native PC/Xbox provenance and JM scene/LEGO fixture.')
