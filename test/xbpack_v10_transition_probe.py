"""Check v10 transition permissions against native PC constructor and consumers.

Run with a host C++ compiler, e.g. under WSL:
  python3 test/xbpack_v10_transition_probe.py build/Ultimate_prerelease_original.exe
Optionally pass --before <source snapshot> to demonstrate the old failure.
The converter is extracted unchanged to avoid linking the game runtime.
"""
from pathlib import Path
import argparse
import os
import struct
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("original_exe", type=Path)
parser.add_argument("--source", type=Path,
                    default=Path(__file__).resolve().parents[1] / "src/core_ai_resource.cpp")
parser.add_argument("--before", type=Path)
parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
args = parser.parse_args()
pe = args.original_exe.read_bytes()
h=struct.unpack_from('<I',pe,0x3c)[0]; ns=struct.unpack_from('<H',pe,h+6)[0]; opt=struct.unpack_from('<H',pe,h+20)[0]; base=struct.unpack_from('<I',pe,h+52)[0]
def native(va,n):
 for i in range(ns):
  vs,rva,sz,off=struct.unpack_from('<4I',pe,h+24+opt+i*40+8)
  if base+rva<=va<base+rva+sz:return pe[off+va-base-rva:off+va-base-rva+n]
 raise ValueError(hex(va))
for va,offset in [(0x6a1740,0x24),(0x6a1778,0x25),(0x6a177c,0x26)]:
 assert native(va,4)==bytes([0xc6,0x46,offset,1]),(hex(va),native(va,4).hex())
assert native(0x478e0b,3)==bytes.fromhex('385826') # hero death gate
assert native(0x6904c8,3)==bytes.fromhex('8a4825') # subdued gate
assert native(0x688c68,3)==bytes.fromhex('8a4826') # generic death gate
prefix='''#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>
uint32_t read_u32(const uint8_t *p) { uint32_t x; std::memcpy(&x,p,4); return x; }
'''
main='''
int main() {
  // Two serialized records exercise conversion and the following boundary.
  std::vector<uint8_t> data(0x1C + 0x20, 0xA1);
  const uint32_t header[] = {0x163,0x94B51E64,0};
  const uint32_t next[] = {0x148,0xD552BA6D,0};
  std::memcpy(data.data(),header,sizeof(header));
  std::memcpy(data.data()+0x1C,next,sizeof(next));
  data[0x18]=1; data[0x19]=0; data[0x1A]=1;
  auto raw=data;
  if(!convert_v10_nodes(data)||data.size()!=0x34+0x24) return 2;
  if(std::memcmp(data.data(),raw.data(),0x0C)||read_u32(data.data()+0xC)!=0
     ||std::memcmp(data.data()+0x10,raw.data()+0xC,0x10)) return 3;
  if(data[0x24]!=1||data[0x25]!=1||data[0x26]!=1) {
    std::cerr << "FAIL: native death/subdued/recovery permissions lost in conversion\\n";
    return 4;
  }
  if(read_u32(data.data()+0x28)||read_u32(data.data()+0x2C)||read_u32(data.data()+0x30)) return 5;
  if(read_u32(data.data()+0x34)!=0x148||read_u32(data.data()+0x38)!=0xD552BA6D) return 6;
  std::cout << "PASS: native transition permissions, legacy fields, zero callbacks, following-node boundary\\n";
}
'''
sources = [("before", args.before)] if args.before else []
sources.append(("after", args.source))
with tempfile.TemporaryDirectory(prefix="xbpack-v10-transition-") as temporary:
    out = Path(temporary)
    for label, source in sources:
        text = source.read_text()
        start = text.index("enum class node_layout")
        end = text.index("\n#else\n", start)
        cpp = out / (label + ".cpp")
        exe = out / (label + (".exe" if os.name == "nt" else ""))
        cpp.write_text(prefix + text[start:end] + main)
        subprocess.run([args.cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        "-O0", str(cpp), "-o", str(exe)], check=True)
        result = subprocess.run([str(exe)])
        expected = 4 if label == "before" else 0
        if result.returncode != expected:
            raise SystemExit(f"{label}: expected exit {expected}, got {result.returncode}")
        print(f"{label}: expected exit {expected} verified", flush=True)
print("PASS: original PC constructor/consumer signatures and converted transition permissions")
