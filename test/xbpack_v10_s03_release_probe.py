"""Verify the V10 carried-release fix against native PC/Xbox and S03 ALS assets.

Uses a Windows 32-bit MinGW compiler to execute the production installer and
both native load instructions in isolated mapped memory; does not launch game.
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


def hash_name(name):
    value = 0
    for char in name.lower().encode():
        value = (value * 33 + char) & 0xFFFFFFFF
    return value


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('original_exe', type=Path)
parser.add_argument('xbox_exe', type=Path)
parser.add_argument('packs', type=Path)
parser.add_argument('--cxx', default='g++')
args = parser.parse_args()
pc = args.original_exe.read_bytes()
xb = args.xbox_exe.read_bytes()
pe = u32(pc, 0x3C)
sections = pe + 24 + struct.unpack_from('<H', pc, pe + 20)[0]


def native(address, size):
    for i in range(struct.unpack_from('<H', pc, pe + 6)[0]):
        _, rva, raw_size, raw = struct.unpack_from('<4I', pc, sections + i * 40 + 8)
        va = u32(pc, pe + 52) + rva
        if va <= address and address + size <= va + raw_size:
            return pc[raw + address - va:raw + address - va + size]
    raise AssertionError(f'unmapped PC address {address:#x}')


def xbox(address, size):
    table = u32(xb, 0x120) - u32(xb, 0x104)
    for i in range(u32(xb, 0x11C)):
        va, _, raw, raw_size = struct.unpack_from('<4I', xb, table + i * 56 + 4)
        if va <= address and address + size <= va + raw_size:
            return xb[raw + address - va:raw + address - va + size]
    raise AssertionError(f'unmapped Xbox address {address:#x}')


# These are two native versions of the same default-release branch. The
# registered strings independently determine the correct instruction operand.
assert native(0x463FB7, 11) == bytes.fromhex('8B1558C4960068EFBEADDE')
assert native(0x855620, 14) == bytes.fromhex('6A0068584E8A006A00B958C49600')
assert native(0x8A4E58, 14) == b'Idle_No_Blend\0'
assert xbox(0x310DB8, 11) == bytes.fromhex('8B159C56570068EFBEADDE')
assert xbox(0x3DFD20, 14) == bytes.fromhex('6A006818884B006A00B99C565700')
assert xbox(0x4B8818, 5) == b'Idle\0'
assert native(0x463FB5, 2) == bytes.fromhex('EB3A')  # configured release bypass
assert native(0x463FF1, 14) == bytes.fromhex('8BCEE8A8DE05008BC8E8014D0300')
idle = hash_name('Idle')
no_blend = hash_name('Idle_No_Blend')
assert idle == 0x003B4B7E and no_blend == 0x8C0F433E

# The serialized base-layer scripted state starts with mash type, state ID,
# and category ID. Check every S03 ALS, including all rescue NPC variants.
idle_state = struct.pack('<3I', 0x1FC, idle, hash_name('Idle_Walk_Run'))
als = []
for path in sorted(args.packs.glob('S03*.XBPACK')):
    data = path.read_bytes()
    assert u32(data, 0) == 10
    directory = u32(data, 0x18) + 16
    payload = u32(data, 0x1C)
    parents = u32(data, directory + 4) & 0xFFFFFF
    count = u32(data, directory + 12) & 0xFFFFFF
    locations = (directory + 0x2B0 + parents * 4 + 7) & ~7
    assert locations + count * 16 <= payload
    for i in range(count):
        key, kind, offset, size = struct.unpack_from('<4I', data, locations + i * 16)
        if kind != 3:
            continue
        raw = data[payload + offset:payload + offset + size]
        assert len(raw) == size
        assert raw.count(idle_state) == 1, (path.name, hex(key))
        assert struct.pack('<I', no_blend) not in raw, (path.name, hex(key))
        state_at = payload + offset + raw.index(idle_state)
        als.append((path.name, key, payload + offset, state_at))
assert len(als) == 9, als
assert ('S03_TAM4_PACK.XBPACK', 0x8AB911CA, 0x7728, 0x7A28) in als
print('BASELINE FAIL reproduced: all 9 S03 ALS resources lack PC Idle_No_Blend; '
      'all contain the Xbox Idle state.', flush=True)

root = Path(__file__).resolve().parents[1]
source = (root / 'src/xbpack_v10_s03.cpp').read_text()
# Compile the actual installer body; only project headers/logging are stubbed.
source = re.sub(r'^#include "[^"\n]+"\n', '', source, flags=re.M)
caller = native(0x463E50, 0x1C3)
array = ','.join(str(b) for b in caller)
prologue = r'''
#define OPENUSM_XBPACK_V10
#include <windows.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
void sp_log(const char *) {}
#define CHECK(v) do { if (!(v)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #v); std::exit(1); } } while(0)
'''
test = r'''
// Keep the two native addresses inside this isolated test image's own BSS.
// VirtualAlloc at those low addresses can collide with a randomized CRT heap.
static uint8_t probe_memory[0xA00000];
int main() {
    CHECK(reinterpret_cast<uintptr_t>(probe_memory) <= 0x460000);
    CHECK(reinterpret_cast<uintptr_t>(probe_memory) + sizeof(probe_memory) >= 0x970000);
    auto *region = reinterpret_cast<uint8_t *>(0x460000);
    DWORD protection;
    CHECK(VirtualProtect(region, 0x10000, PAGE_EXECUTE_READWRITE, &protection));
    *reinterpret_cast<uint32_t *>(0x96C458) = 0x8C0F433E;
    const uint8_t original[] = {CALLER_BYTES};
    auto *caller = reinterpret_cast<uint8_t *>(0x463E50);
    auto *site = reinterpret_cast<uint8_t *>(0x463FB7);
    std::memcpy(caller, original, sizeof(original));
    auto execute_load = [&]() {
        auto *stub = region + 0x5000;
        std::memcpy(stub, site, 6);
        stub[6] = 0x89; stub[7] = 0xD0; stub[8] = 0xC3; // mov eax,edx;ret
        FlushInstructionCache(GetCurrentProcess(), stub, 9);
        return reinterpret_cast<uint32_t (*)()>(stub)();
    };
    CHECK(execute_load() == 0x8C0F433E);
    CHECK(xbpack_v10_s03_patch());
    const uint8_t fixed[] = {0xBA,0x7E,0x4B,0x3B,0,0x90};
    CHECK(std::memcmp(site, fixed, sizeof(fixed)) == 0);
    CHECK(execute_load() == 0x003B4B7E);
    for (size_t i = 0; i < sizeof(original); ++i) {
        if (i < 0x167 || i >= 0x16D) CHECK(caller[i] == original[i]);
    }
    CHECK(*reinterpret_cast<uint32_t *>(0x96C458) == 0x8C0F433E);
    std::array<uint8_t, sizeof(original)> snapshot;
    std::memcpy(snapshot.data(), caller, snapshot.size());
    CHECK(xbpack_v10_s03_patch());
    CHECK(std::memcmp(caller, snapshot.data(), snapshot.size()) == 0);
    std::memcpy(caller, original, sizeof(original));
    site[2] ^= 1;
    std::memcpy(snapshot.data(), caller, snapshot.size());
    CHECK(!xbpack_v10_s03_patch());
    CHECK(std::memcmp(caller, snapshot.data(), snapshot.size()) == 0);
    std::puts("PASS: native load now selects Xbox Idle; installer preserves caller/global, is idempotent, and rejects mismatched code without writes.");
}
'''.replace('CALLER_BYTES', array)
with tempfile.TemporaryDirectory(prefix='xbpack-s03-release-') as temp:
    cpp = Path(temp) / 'probe.cpp'
    exe = Path(temp) / 'probe.exe'
    cpp.write_text(prologue + source + test)
    compiler = shutil.which(args.cxx) or str(Path(args.cxx).resolve())
    env = dict(os.environ)
    env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
    subprocess.run([compiler, '-std=c++17', '-O2', '-static', '-m32',
                    '-Wl,--image-base,0x400000', '-Wl,--disable-dynamicbase',
                    str(cpp), '-o', str(exe)],
                   env=env, check=True)
    subprocess.run([str(exe)], env=env, check=True)
print('PASS: PC/Xbox instruction and string provenance; all 9 real S03 ALS states.')
