"""Compile the real V10 gun clone and check the native weapon layout.

Usage: python test/verify_v10_gun_layout.py --compiler <g++> --pc-exe <original-exe>
Use --source and --actor-source baseline files to demonstrate regression failure.
No game assets are copied into this test.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

# Native gun::un_mash cache calls, in serialized shared-stream order.
UNMASH = ((0x4FF88C, 0x1AC), (0x4FF89A, 0x1EC), (0x4FF8A8, 0x22C),
          (0x4FF8B6, 0x26C), (0x4FF8C4, 0x2AC))


def verify_pc_operands(path):
    data = path.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    image_base = struct.unpack_from('<I', data, pe + 52)[0]
    table = pe + 24 + struct.unpack_from('<H', data, pe + 20)[0]
    sections = [struct.unpack_from('<8s8I', data, table + i * 40)
                for i in range(struct.unpack_from('<H', data, pe + 6)[0])]

    def read(address, size):
        rva = address - image_base
        position = next((raw + rva - start for _, _, start, length, raw, *_ in sections
                         if start <= rva and rva + size <= start + length), None)
        if position is None:
            raise ValueError(f'Native PC address unavailable: {address:#x}')
        return data[position:position + size]

    # Xbox fire/impact consumers select +1A4/+1E4/+264 at C6CF9/C6757/C6779.
    for address, prefix, offset in ((0x500971, b'\x8d\x8f', 0x1EC),
                                    (0x50035D, b'\x8d\x8e', 0x22C),
                                    (0x5003BC, b'\x8d\x8e', 0x2AC)):
        if read(address, 6) != prefix + struct.pack('<I', offset):
            raise ValueError(f'Unexpected native PC effect selection at {address:#x}')
    for call, offset in UNMASH:
        expected = b'\x8d\x8f' + struct.pack('<I', offset) + b'\x51\x53\xe8'
        expected += struct.pack('<i', 0x4D3650 - call - 5)
        if read(call - 8, len(expected)) != expected:
            raise ValueError(f'Unexpected native PC cache unmash call at {call:#x}')
    print('PASS: original PC fire/impact operands and five native unmash calls', flush=True)


def verify_shared_order(source):
    hooks = re.findall(r'REDIRECT\(\s*(0x[0-9A-Fa-f]+)\s*,\s*init_v10_gun_effect\s*\)', source)
    if len(hooks) != 1:
        raise ValueError('Expected one V10 extra-cache unmash redirect')
    skip = int(hooks[0], 16)
    # Distinct serialized payloads must be consumed by their native effect
    # slots. The inserted PC cache must not consume any shared-stream token.
    tokens = iter(('muzzle', 'impact_bl', 'impact_other', 'impact_non_bl'))
    actual = {offset: (None if call == skip else next(tokens, 'OVERREAD'))
              for call, offset in UNMASH}
    expected = {0x1AC: None, 0x1EC: 'muzzle', 0x22C: 'impact_bl',
                0x26C: 'impact_other', 0x2AC: 'impact_non_bl'}
    if actual != expected or next(tokens, None) is not None:
        print(f'FAIL: native shared-cache order with redirected call {skip:#x}: {actual}', flush=True)
        return False
    print('PASS: inserted first PC cache skips shared data; four payloads retain order', flush=True)
    return True


PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
struct generic_mash_header { uint8_t bytes[0x10]; };
static uint8_t* allocation;
static size_t allocated_size;
void* arch_memalign(size_t alignment, size_t size) {
    assert(alignment == 0x10);
    allocated_size = size;
    allocation = static_cast<uint8_t*>(std::malloc(size + 32));
    assert(allocation);
    std::memset(allocation, 0xCD, size + 32);
    return allocation + 16;
}
'''

CHECK = r'''
int failures = 0;
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
bool equal(const uint8_t* a, const uint8_t* b, size_t size) {
    return std::memcmp(a, b, size) == 0;
}
bool filled(const uint8_t* a, size_t size, uint8_t value) {
    for (size_t i = 0; i < size; ++i) if (a[i] != value) return false;
    return true;
}
void run(size_t trailing) {
    // Independent native sizes: Xbox gun 0x328, PC gun 0x374.
    std::vector<uint8_t> source(0x328 + trailing);
    for (size_t i = 0; i < source.size(); ++i)
        source[i] = static_cast<uint8_t>((i * 37 + i / 251) % 255 + 1);
    constexpr size_t xbox_caches[] = {0x1A4, 0x1E4, 0x224, 0x264};
    constexpr size_t pc_caches[] = {0x1EC, 0x22C, 0x26C, 0x2AC};
    for (size_t cache = 0; cache < 4; ++cache)
        for (size_t byte = 0; byte < 0x40; ++byte)
            source[xbox_caches[cache] + byte] = static_cast<uint8_t>(cache * 64 + byte);
    const auto before = source;
    const auto clone = clone_v10_gun(source.data(), source.size());
    const auto* p = clone.object;
    check(allocated_size == 0x374 + trailing, "converted allocation size");
    check(clone.normal == p + 0x374, "normal mash stream starts after PC gun");
    check(source == before, "source buffer remains unchanged");
    check(equal(p, source.data(), 0xBC), "actor prefix preserved");
    check(filled(p + 0xBC, 4, 0), "PC actor word initialized");
    check(equal(p + 0xC0, source.data() + 0xBC, 0x6C), "gun prefix preserved");
    check(filled(p + 0x12C, 4, 0), "PC gun prefix word initialized");
    check(equal(p + 0x130, source.data() + 0x128, 0x7C), "firing data before caches preserved");
    for (size_t cache = 0; cache < 4; ++cache) {
        if (!equal(p + pc_caches[cache], source.data() + xbox_caches[cache], 0x40)) {
            std::cerr << "FAIL: Xbox cache +0x" << std::hex << xbox_caches[cache]
                      << " must reach native PC +0x" << pc_caches[cache] << std::dec << '\n';
            ++failures;
        }
    }
    check(filled(p + 0x1AC, 0x40, 0), "extra PC effect is the FIRST cache");
    check(equal(p + 0x2EC, source.data() + 0x2A4, 0x84), "weapon suffix preserved");
    check(filled(p + 0x370, 4, 0), "PC trailing weapon word initialized");
    check(equal(clone.normal, source.data() + 0x328, trailing), "trailing mash stream preserved");
    check(filled(allocation, 16, 0xCD) && filled(p + allocated_size, 16, 0xCD),
          "conversion respects allocation boundaries");
    std::free(allocation);
}
int main() {
    run(0); run(1); run(0x137);
    if (failures) return 1;
    std::cout << "PASS: four native caches, extra slot, prefixes, suffix, normal stream "
                 "and allocation guards (three lengths)\n";
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parents[1]
    parser.add_argument('--source', type=Path, default=root / 'src/parse_generic_mash.cpp')
    parser.add_argument('--actor-source', type=Path, default=root / 'src/actor_xbpack.cpp')
    parser.add_argument('--pc-exe', type=Path)
    parser.add_argument('--compiler', default='g++')
    args = parser.parse_args()
    if args.pc_exe:
        verify_pc_operands(args.pc_exe)
    shared_ok = verify_shared_order(args.actor_source.read_text())
    source = args.source.read_text()
    # Use the real constants, result type and production clone.
    definitions = source[source.index('constexpr uint16_t V10_CONGLOM_TYPE'):
                         source.index('v10_mash_clone clone_v10_conglom(')]
    begin = source.index('v10_mash_clone clone_v10_gun(')
    end = source.index('v10_mash_clone clone_v10_thrown_item(', begin)
    with tempfile.TemporaryDirectory(prefix='usm-gun-layout-') as temporary:
        directory = Path(temporary)
        cpp = directory / 'gun-layout.cpp'
        cpp.write_text(PRELUDE + definitions + source[begin:end] + CHECK)
        executable = directory / ('gun-layout.exe' if os.name == 'nt' else 'gun-layout')
        compiler = shutil.which(args.compiler) or str(Path(args.compiler).resolve())
        env = dict(os.environ)
        env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
        subprocess.run([compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', '-O2',
                        str(cpp), '-o', str(executable)], check=True, env=env)
        result = subprocess.run([str(executable)], env=env).returncode
        return result if result else (0 if shared_ok else 1)


if __name__ == '__main__':
    raise SystemExit(main())
