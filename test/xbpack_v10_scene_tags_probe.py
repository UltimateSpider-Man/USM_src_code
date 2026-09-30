"""Verify the V10 empty-audio scene fix against the original PC and FL_INT_A.

Run under WSL with a host C++ compiler:
  python3 test/xbpack_v10_scene_tags_probe.py \
    build/Ultimate_prerelease_original.exe /path/to/v10/FL_INT_A.XBPACK

The native instruction operands and the actual resource directory provide the
oracle. The production installer is compiled unchanged into a mapped-memory
probe; game loading and the unrelated entity-loader redirect are not executed.
"""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("original_exe", type=Path)
parser.add_argument("pack", type=Path)
parser.add_argument("--cxx", default="g++")
args = parser.parse_args()

pe = args.original_exe.read_bytes()
header = u32(pe, 0x3C)
sections = struct.unpack_from("<H", pe, header + 6)[0]
optional_size = struct.unpack_from("<H", pe, header + 20)[0]
image_base = u32(pe, header + 52)


def native(address, size):
    for index in range(sections):
        _, rva, raw_size, offset = struct.unpack_from(
            "<4I", pe, header + 24 + optional_size + index * 40 + 8)
        if image_base + rva <= address and address + size <= image_base + rva + raw_size:
            start = offset + address - image_base - rva
            return pe[start:start + size]
    raise AssertionError(f"unmapped native address {address:#x}")


# Native audio loader tests END, then advances into the payload and calls the
# rtree unmash. These fingerprints establish that changing this operand stops
# the bad parse, rather than merely hiding the later traversal exception.
assert native(0x53CBAF, 5) == bytes.fromhex("83 3f 12 74 34")
assert native(0x53CBD8, 15) == bytes.fromhex(
    "83 c7 04 57 8d 8e e0 00 00 00 e8 19 3a fe ff")
assert native(0x52061D, 18) == bytes.fromhex(
    "8b 7c 24 10 8d 77 3f 8b c6 83 e0 c0 89 41 04 8b 58 5c")
assert native(0x5213D9, 6) == bytes.fromhex("8b 57 2c 8b 5f 20")
assert native(0x521408, 5) == bytes.fromhex("66 3b 54 19 02")
assert native(0x55AD64, 5) == bytes.fromhex("83 f9 12 0f 84")

pack = args.pack.read_bytes()
assert struct.unpack_from("<5I", pack) == (10, 486, 265, 487, 246)
directory = u32(pack, 0x18) + 16
payload = u32(pack, 0x1C)
parents = u32(pack, directory + 4) & 0xFFFFFF
count = u32(pack, directory + 12) & 0xFFFFFF
locations = (directory + 0x2B0 + parents * 4 + 7) & ~7
assert locations + count * 16 <= payload
audio = []
entities = []
for index in range(count):
    name, kind, offset, size = struct.unpack_from("<4I", pack, locations + index * 16)
    assert payload + offset + size <= len(pack)
    if kind == 11:  # V10 SCN_AUDIO_BOX, translated to PC resource type 12.
        audio.append((payload + offset, size))
    if kind == 9:
        entities.append(payload + offset)
assert len(audio) == 1
audio_start, audio_size = audio[0]
assert audio_size == 4 and u32(pack, audio_start) == 13
misparsed_root = (audio_start + 4 + 63) & ~63
assert misparsed_root in entities
assert u32(pack, misparsed_root + 0x20) == 0x7BADBB9B
assert u32(pack, misparsed_root + 0x5C) == 0xCD000000
assert 13 != native(0x53CBAF, 3)[2]
print(f"BASELINE FAIL reproduced: END13 audio at {audio_start:#x}, size=4; "
      f"PC continues into entity at {misparsed_root:#x}", flush=True)

source = (Path(__file__).resolve().parents[1] / "src/wds.cpp").read_text()
start = source.index("bool wds_xbpack_patch()")
end = source.index("\n}", start) + 2
installer = source[start:end]
prologue = r'''
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include <sys/mman.h>
template<typename T> T bit_cast(std::uintptr_t value) { return reinterpret_cast<T>(value); }
void sp_log(const char *) {}
#define FUNC_ADDRESS(name, member) const unsigned name = 0
#define REDIRECT(site, target) ((void)(site), (void)(target))
'''
initializers = "\n".join(
    f"    std::memcpy(reinterpret_cast<void *>({address}), "
    f"std::vector<uint8_t>{{{','.join(map(str, native(address, size)))}}}.data(), {size});"
    for address, size in [(0x53CBAF, 5), (0x55AD64, 9), (0x558793, 1)])
main = r'''
int main() {
    constexpr uintptr_t base = 0x520000;
    constexpr size_t size = 0x40000;
    auto *memory = static_cast<uint8_t *>(mmap(reinterpret_cast<void *>(base), size,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (memory == MAP_FAILED) { std::perror("mmap probe pages"); return 1; }
    if (reinterpret_cast<uintptr_t>(memory) != base) { munmap(memory, size); return 1; }
    std::memset(memory, 0xA7, size);
INITIALIZERS
    const std::vector<uint8_t> before(memory, memory + size);
#ifdef OPENUSM_XBPACK_V10
    if (!wds_xbpack_patch()) return 2;
    for (size_t i = 0; i != size; ++i) {
        const auto address = base + i;
        const auto expected = address == 0x53CBB1 || address == 0x55AD66 ? 13u
            : address == 0x558793 ? 0x3Cu : before[i];
        if (memory[i] != expected) return 3;
    }
    // A native cmp [edi],imm8 / je now recognizes the real four-byte END13
    // resource. Normal AUDIO8 still enters its original decoding path.
    const auto native_empty_operand = memory[0x53CBB1 - base];
    if (native_empty_operand != 13 || native_empty_operand == 8) return 4;
    const std::vector<uint8_t> installed(memory, memory + size);
    if (!wds_xbpack_patch() || std::memcmp(memory, installed.data(), size)) return 5;
    for (auto bad_site : {0x53CBAFu, 0x55AD64u}) {
        std::memcpy(memory, before.data(), size);
        memory[bad_site - base] ^= 1;
        const std::vector<uint8_t> rejected(memory, memory + size);
        if (wds_xbpack_patch() || std::memcmp(memory, rejected.data(), size)) return 6;
    }
    std::cout << "PASS V10: empty AUDIO skipped; payload unchanged; installer idempotent; "
                 "mismatched code rejected without partial writes\n";
#else
    if (!wds_xbpack_patch() || std::memcmp(memory, before.data(), size)) return 7;
    std::cout << "PASS retail: no code writes\n";
#endif
    munmap(memory, size);
}
'''.replace("INITIALIZERS", initializers)
with tempfile.TemporaryDirectory(prefix="v10-scene-tags-") as temporary:
    temporary = Path(temporary)
    cpp = temporary / "probe.cpp"
    cpp.write_text(prologue + installer + main)
    for configuration, definitions in [("v10", ["-DOPENUSM_XBPACK_V10"]), ("retail", [])]:
        executable = temporary / configuration
        subprocess.run([args.cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-fPIE", "-pie",
                        *definitions, str(cpp), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
