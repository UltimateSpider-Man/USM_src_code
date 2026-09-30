"""Check real V10 S02 cores against original PC class sizes.

Requires the user's S02_SHOCKER_PACK.XBPACK and original PC executable;
no game assets are stored in this test. Example:
  python test/verify_v10_s02_core_layouts.py --pack <pack> --pc-exe <exe> --compiler <g++>
The compiled code is extracted from core_ai_resource.cpp, including its real
conversion table and functions. Expected Xbox record sizes/identities are
from the serialized S02 cores; expected PC sizes are read independently from
the original executable's vtable size callbacks. --source accepts a baseline
source file to demonstrate that the regression rejects the previous code.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

# type, identity, Xbox serialized size, original PC vtable
CORES = {
    'police': [
        (0x13D, 0x0001AB00, 0x028, 0x0087CEC0),
        (0x187, 0x085DE71B, 0x028, 0x0087DD04),
        (0x12B, 0x15897C0C, 0x018, 0x0087DB04),
        (0x14E, 0x1754B0DC, 0x05C, 0x0087C410),
        (0x189, 0x1CF0A9AA, 0x034, 0x0087DD34),
        (0x0E9, 0x39338F28, 0x05C, 0x0087D838),
        (0x093, 0x4C90D9E0, 0x054, 0x0087CFB4),
        (0x14A, 0x5D0C49A4, 0x30C, 0x0087BE38),
        (0x10D, 0x6D4B8BFF, 0x020, 0x0087DA74),
        (0x167, 0x74556656, 0x02C, 0x0087CA18),
        (0x096, 0x76DDDD6F, 0x0A8, 0x0087CF80),
        (0x0F2, 0x7CED570D, 0x038, 0x0087D904),
        (0x185, 0x8AB41E64, 0x01C, 0x0087DCD4),
        (0x157, 0x9317E156, 0x034, 0x0087DB94),
        (0x163, 0x94B51E64, 0x01C, 0x0087CC7C),
        (0x17C, 0x973950CC, 0x028, 0x0087C090),
        (0x08D, 0x9EE13B40, 0x044, 0x0087CE10),
        (0x19D, 0xA1F4712A, 0x068, 0x0087DE08),
        (0x1B4, 0xA2D277FE, 0x040, 0x0087DDD8),
        (0x08F, 0xC8553C6E, 0x04C, 0x0087CE40),
        (0x155, 0xCC62C392, 0x0FC, 0x0087D3D0),
        (0x148, 0xD552BA6D, 0x020, 0x0087BB9C),
        (0x142, 0xD970BD20, 0x054, 0x0087BFDC),
        (0x109, 0xEAED554A, 0x05C, 0x0087D938),
        (0x145, 0xF6E3EBA5, 0x01C, 0x0087C018),
        (0x094, 0xFAD58E58, 0x038, 0x0087CF38),
    ],
    'shocker': [
        (0x13D, 0x0001AB00, 0x028, 0x0087CEC0),
        (0x0E7, 0x0087893D, 0x028, 0x0087D6D0),
        (0x12B, 0x15897C0C, 0x018, 0x0087DB04),
        (0x14E, 0x1754B0DC, 0x05C, 0x0087C410),
        (0x189, 0x1CF0A9AA, 0x034, 0x0087DD34),
        (0x146, 0x5D0C49A4, 0x2F8, 0x0087BBD0),
        (0x163, 0x94B51E64, 0x01C, 0x0087CC7C),
        (0x1B4, 0xA2D277FE, 0x040, 0x0087DDD8),
        (0x155, 0xCC62C392, 0x0FC, 0x0087D3D0),
        (0x148, 0xD552BA6D, 0x020, 0x0087BB9C),
        (0x142, 0xD970BD20, 0x054, 0x0087BFDC),
        (0x145, 0xF6E3EBA5, 0x01C, 0x0087C018),
        (0x140, 0xFAD58E58, 0x030, 0x0087C048),
    ],
}


def word(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def native_sizes(data):
    pe = word(data, 0x3C)
    image_base = word(data, pe + 24 + 28)
    table = pe + 24 + struct.unpack_from('<H', data, pe + 20)[0]
    sections = [struct.unpack_from('<8s8I', data, table + 40 * i)
                for i in range(struct.unpack_from('<H', data, pe + 6)[0])]

    def offset(address):
        rva = address - image_base
        for _, _, start, length, raw, *_ in sections:
            if start <= rva < start + length:
                return raw + rva - start
        raise ValueError(f'PC address outside sections: {address:#x}')

    def size_function(address, depth=0):
        if depth > 4:
            raise ValueError('Unexpected PC size-function jump chain')
        at = offset(address)
        if data[at] == 0xB8 and data[at + 5] == 0xC3:
            return word(data, at + 1)
        if data[at] == 0xE9:
            target = address + 5 + struct.unpack_from('<i', data, at + 1)[0]
            return size_function(target, depth + 1)
        raise ValueError(f'Unsupported PC size function at {address:#x}')

    return {vtable: size_function(word(data, offset(vtable + 0x2C)))
            for records in CORES.values() for _, _, _, vtable in records}


def fixture(pack, records):
    # Locate by the complete expected record sequence, independent of pack offsets.
    signature = struct.pack('<II', *records[0][:2])
    start = 0
    while True:
        node = pack.find(signature, start)
        if node < 0:
            raise ValueError('Expected complete S02 core was not found in supplied pack')
        start = node + 4
        base = node - 0x14 - 4 * len(records)
        if base < 0 or word(pack, base + 4) != len(records):
            continue
        cursor = node
        try:
            for kind, name, size, _ in records:
                if (word(pack, cursor), word(pack, cursor + 4)) != (kind, name):
                    raise ValueError('Record mismatch')
                has_parameters = word(pack, cursor + 0x10)
                cursor += size
                if has_parameters:
                    count = word(pack, cursor + 4)
                    if count > 100:
                        raise ValueError('Invalid serialized parameter count')
                    cursor += 24 + count * 4
                    for _ in range(count):
                        kind = word(pack, cursor + 4)
                        cursor += 12 + {3: 32, 4: 12, 5: 8}.get(kind, 0)
            return pack[base:cursor]
        except (ValueError, struct.error):
            continue


CHECK = r'''
struct Expected { unsigned type, name, size; };
bool check(const char* path, const std::vector<Expected>& records) {
    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), {});
    if (data.empty() || !convert_v10_nodes(data)) return false;
    size_t cursor = 0x14 + records.size() * 4;
    for (size_t n = 0; n < records.size(); ++n) {
        const size_t start = cursor;
        const auto& expected = records[n];
        if (start + expected.size > data.size()
            || read_u32(data.data() + start) != expected.type
            || read_u32(data.data() + start + 4) != expected.name) {
            std::cerr << "Node boundary mismatch at index " << n << '\n';
            return false;
        }
        cursor += expected.size;
        if (read_u32(data.data() + start + 0x14)) {
            if (cursor + 24 > data.size()) return false;
            const unsigned count = read_u32(data.data() + cursor + 4);
            if (count > 100) {
                std::cerr << "Invalid parameter count at node " << n << ": 0x"
                          << std::hex << count << std::dec << '\n';
                return false;
            }
            cursor += 24 + count * 4;
            for (unsigned j = 0; j < count; ++j) {
                if (cursor + 12 > data.size()) return false;
                const unsigned type = read_u32(data.data() + cursor + 4);
                cursor += 12;
                if (type == 3) cursor += 32;
                else if (type == 4) cursor += 12;
                else if (type == 5) cursor += 8;
            }
        }
    }
    if (cursor != data.size()) {
        std::cerr << "Final cursor does not match buffer size\n";
        return false;
    }
    std::cout << "PASS: " << records.size() << " native node boundaries and parameter arrays\n";
    return true;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pack', type=Path, required=True)
    parser.add_argument('--pc-exe', type=Path, required=True)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--source', type=Path,
                        default=Path(__file__).resolve().parents[1] / 'src/core_ai_resource.cpp')
    args = parser.parse_args()
    sizes = native_sizes(args.pc_exe.read_bytes())
    pack = args.pack.read_bytes()
    source = args.source.read_text()
    begin = source.index('enum class node_layout')
    end = source.index('\n#else\n\nbool convert_web_zip_inode', begin)
    code = '\n'.join('#include <' + name + '>' for name in
                     ('cstdint', 'cstring', 'array', 'vector', 'fstream', 'iostream', 'iterator'))
    code += '\nuint32_t read_u32(const uint8_t* p) { uint32_t x; std::memcpy(&x,p,4); return x; }\n'
    code += source[begin:end] + CHECK + '\nint main() { bool ok = true;\n'
    with tempfile.TemporaryDirectory(prefix='usm-s02-layout-') as temporary:
        directory = Path(temporary)
        for name, records in CORES.items():
            path = directory / (name + '.bin')
            path.write_bytes(fixture(pack, records))
            expected = ','.join('{' + ','.join(hex(v) for v in (kind, ident, sizes[vtable])) + '}'
                                for kind, ident, _, vtable in records)
            code += 'ok = check(' + json.dumps(str(path)) + ', {' + expected + '}) && ok;\n'
        code += 'return ok ? 0 : 1; }\n'
        cpp = directory / 's02-core-layouts.cpp'
        cpp.write_text(code)
        executable = directory / ('s02-core-layouts.exe' if os.name == 'nt' else 's02-core-layouts')
        compiler = shutil.which(args.compiler) or str(Path(args.compiler).resolve())
        env = dict(os.environ)
        # MinGW's cc1plus needs the compiler runtime DLLs visible in PATH.
        env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
        subprocess.run([compiler, '-std=c++20', '-O2', str(cpp), '-o', str(executable)],
                       check=True, env=env)
        return subprocess.run([str(executable)], env=env).returncode


if __name__ == '__main__':
    raise SystemExit(main())
