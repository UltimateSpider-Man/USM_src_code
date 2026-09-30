"""Verify the V10 ALS parameter remap against matching original native methods.

Run under WSL with g++:
  python3 test/xbpack_v10_als_params_probe.py build/Ultimate_prerelease_original.exe default.xbe
Optionally pass --before <old xbpack.h> to reproduce the mismatched filters.
"""
from pathlib import Path
import argparse
import os
import struct
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("original_exe", type=Path)
parser.add_argument("original_xbe", type=Path)
parser.add_argument("--header", type=Path,
                    default=Path(__file__).resolve().parents[1] / "src/xbpack.h")
parser.add_argument("--before", type=Path)
parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
args = parser.parse_args()


def image(path, xbox=False):
    data = path.read_bytes()
    u32 = lambda offset: struct.unpack_from("<I", data, offset)[0]
    sections = []
    if xbox:
        base = u32(0x104)
        for i in range(u32(0x11C)):
            _, va, _, raw, size = struct.unpack_from("<5I", data, u32(0x120) - base + i * 56)
            sections.append((va, size, raw))
    else:
        pe = u32(0x3C)
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional = struct.unpack_from("<H", data, pe + 20)[0]
        base = u32(pe + 52)
        for i in range(count):
            _, va, size, raw = struct.unpack_from("<4I", data, pe + 24 + optional + i * 40 + 8)
            sections.append((base + va, size, raw))

    def read(va, size):
        for start, length, raw in sections:
            if start <= va and va + size <= start + length:
                return data[raw + va - start:raw + va - start + size]
        raise ValueError(f"unmapped native address {va:#x}")
    return read


pc = image(args.original_exe)
xb = image(args.original_xbe, True)
# Each row names the same semantic setter/consumer in the two native games.
# Opcode c7 00 is mov dword ptr [eax], id; 6a is push id.
# The altitude consumer (72 -> 74) subtracts the actor's Y coordinate;
# the new PC 75 branch handles horizontal speed before that calculation.
anchors = [
    (18, 18, 0x26A057, 0x6A68BC, "c700"),
    (19, 21, 0x26F045, 0x6CF4DD, "c700"),
    (20, 22, 0x26F061, 0x6CF4F4, "c700"),
    (21, 23, 0x26F082, 0x6CF50B, "c700"),
    (30, 32, 0x2B071A, 0x71FA49, "c700"),
    (49, 51, 0x250276, 0x69424F, "c700"),
    (52, 54, 0x2749DE, 0x6AFCA1, "c700"),
    (59, 61, 0x26CFB5, 0x6A9671, "c700"),
    (67, 69, 0x2F3600, 0x458B20, "c700"),
    (68, 70, 0x26B1CE, 0x6A7B73, "c700"),
    (70, 72, 0x2E8F11, 0x44B986, "c700"),
    (71, 73, 0x598D8, 0x4A587B, "6a"),
    (72, 74, 0x5C74D, 0x4A1F2B, "6a"),
    (73, 76, 0x5C818, 0x4A1FEE, "6a"),
    (74, 77, 0x5C80C, 0x4A1FE2, "6a"),
    (75, 78, 0x5C7FE, 0x4A1FD4, "6a"),
    (76, 79, 0x5C7C3, 0x4A1F9A, "6a"),
    (77, 80, 0x5C7B5, 0x4A1F8C, "6a"),
    (78, 81, 0x5C7A7, 0x4A1F7E, "6a"),
    (79, 82, 0x5C829, 0x4A1FFF, "6a"),
    (80, 83, 0x5DDFB, 0x4A4B18, "6a"),
    (81, 84, 0x5DDED, 0x4A4B0A, "6a"),
    (82, 85, 0x5DDDF, 0x4A4AFC, "6a"),
    (83, 86, 0x5DD18, 0x4A4A31, "6a"),
    (87, 90, 0x5DEF3, 0x4A4BF7, "6a"),
]
for source, destination, xb_address, pc_address, opcode in anchors:
    prefix = bytes.fromhex(opcode)
    width = 1 if opcode == "6a" else 4
    assert xb(xb_address, len(prefix) + width) == prefix + source.to_bytes(width, "little"), hex(xb_address)
    assert pc(pc_address, len(prefix) + width) == prefix + destination.to_bytes(width, "little"), hex(pc_address)

sources = [("before", args.before)] if args.before else []
sources.append(("after", args.header))
with tempfile.TemporaryDirectory(prefix="xbpack-v10-als-") as temporary:
    out = Path(temporary)
    for label, header in sources:
        (out / "xbpack.h").write_text(header.read_text())
        cpp = out / "probe.cpp"
        cpp.write_text('#define OPENUSM_XBPACK_V10\n#include "xbpack.h"\n#include <iostream>\n'
                       'int main() { for(unsigned i=0;i<=125;++i) std::cout << xbpack::pc_als_param(i) << "\\n"; }\n')
        exe = out / ("probe.exe" if os.name == "nt" else "probe")
        subprocess.run([args.cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        str(cpp), "-o", str(exe)], check=True)
        values = list(map(int, subprocess.check_output([str(exe)], text=True).splitlines()))
        mismatches = [(source, values[source], destination)
                      for source, destination, *_ in anchors if values[source] != destination]
        if label == "before":
            assert mismatches, "old remap unexpectedly satisfies native parameter consumers"
            print(f"before: {len(mismatches)} native parameter mismatches reproduced")
        else:
            assert not mismatches, mismatches
            assert values[:19] == list(range(19)), "low external parameters changed"
            # Preserve existing internal remapping and unknown-value handling.
            for source, destination in [(88,91),(90,93),(91,123),(92,95),(101,104),(102,108),(123,129),(124,124),(125,125)]:
                assert values[source] == destination, (source, values[source], destination)
            print(f"PASS: {len(anchors)} original native enum anchors, external boundaries, preserved internal parameters")
