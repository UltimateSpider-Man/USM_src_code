"""Test the V10 mission frame hook and orphaned black-screen recovery.

Compiles the actual installer, guard, active frame branch, and fade-off method
with a deterministic clock and small typed game/resource substitutes. Does not
launch or attach to the game.
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


def function(source, signature):
    start = source.index(signature)
    at = source.index('{', start)
    depth = 1
    end = at + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('original_exe', type=Path)
parser.add_argument('--cxx', default='g++')
args = parser.parse_args()
pc = args.original_exe.read_bytes()
pe = u32(pc, 0x3C)
sections = pe + 24 + struct.unpack_from('<H', pc, pe + 20)[0]


def native(address, size):
    for i in range(struct.unpack_from('<H', pc, pe + 6)[0]):
        _, rva, count, raw = struct.unpack_from('<4I', pc, sections + i * 40 + 8)
        va = u32(pc, pe + 52) + rva
        if va <= address and address + size <= va + count:
            return pc[raw + address - va:raw + address - va + size]
    raise AssertionError(f'unmapped address {address:#x}')


# The world frame loads the manager into ECX, pushes its frame increment, and
# calls the native implementation. The standalone recovery wrapper was absent.
assert native(0x55D754, 12) == bytes.fromhex('8B0D1885960057E8503F0800')
assert native(0x5DEF4C, 5) == bytes.fromhex('E85FB5FFFF')
assert native(0x5E1B1F, 5) == bytes.fromhex('E81CD3FFFF')
assert 0x55D760 + struct.unpack('<i', native(0x55D75C, 4))[0] == 0x5E16B0
print('BASELINE FAIL confirmed: native 55D75B calls 5E16B0 directly, bypassing '
      'orphaned black-screen recovery.', flush=True)

root = Path(__file__).resolve().parents[1]
source = (root / 'src/mission_manager.cpp').read_text()
guard = function(source, 'void release_orphaned_blackscreen(')
constants = source[source.index('constexpr auto ORPHANED_BLACK_GRACE'):source.index(guard)]
# Only the clock dependency is substituted; the conditions and transitions are
# taken unchanged from production and tested against behavioral expectations.
guard_source = (constants + guard).replace('std::chrono::steady_clock', 'test_clock')
fade = function(source, 'void mission_manager::blackscreen_off(')
frame = function(source, 'void mission_manager::frame_advance(')
active_frame = frame[frame.rindex('\n    else') + len('\n    else'):]
assert active_frame.rstrip().endswith('}\n}')
active_frame = active_frame.rstrip()[:-1]
assert active_frame.index('THISCALL(0x005E16B0') < active_frame.index('release_orphaned_blackscreen(this)')
frame_source = 'void mission_manager::frame_advance(Float a2)' + active_frame
installer = function(source, 'bool mission_manager_v10_patch(')
call_constants = '\n'.join(re.findall(
    r'constexpr uintptr_t (?:LAUNCH_TRANSITION_CALL|LOAD_SCRIPT_CALL) = [^;]+;', source))
assert len(call_constants.splitlines()) == 2

prologue = r'''
#define OPENUSM_XBPACK_V10
#include <array>
#include <chrono>
#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define CHECK(v) do { if (!(v)) { std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#v); std::exit(1); } } while(0)
#define TRACE(...) ((void)0)
using Float = float;
void sp_log(const char *) {}
struct test_clock {
    using time_point = std::chrono::steady_clock::time_point;
    static inline time_point current{};
    static time_point now() { return current; }
};
struct game {
    struct { bool load_completed = true; } level;
    struct { bool level_is_loaded = true; } flag;
    bool field_166 = true;
};
game live_game;
game *g_game_ptr = &live_game;
namespace resource_manager {
bool idle = true;
bool is_idle() { return idle; }
}
struct mission_manager {
    void *m_script = nullptr;
    void *m_script_to_load = nullptr;
    bool m_unload_script = false;
    bool field_80 = false;
    int field_FC = 3;
    float field_F4 = 1.0f;
    float field_F8 = 0.0f;
    unsigned unfreezes = 0;
    void sub_5BAC00() { ++unfreezes; }
    void blackscreen_off(Float duration);
    void frame_advance(Float increment);
};
unsigned native_frames = 0;
bool load_during_native_frame = false;
Float last_increment = -1;
void native_call(uintptr_t address,mission_manager *self,Float increment) {
    CHECK(address == 0x5E16B0);
    ++native_frames;
    last_increment = increment;
    if (load_during_native_frame) self->m_script = reinterpret_cast<void *>(1);
}
#define THISCALL(address,self,arg) native_call(address,self,arg)
template<class T> void *function_address(T pointer) {
    void *result;
    std::memcpy(&result,&pointer,sizeof(result));
    return result;
}
#define FUNC_ADDRESS(name,member) auto *name = function_address(member)
template<class T> void redirect(uintptr_t site,T target) {
    auto *bytes = reinterpret_cast<uint8_t *>(site);
    const auto displacement = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(function_address(target))-site-5);
    bytes[0] = 0xE8;
    std::memcpy(bytes+1,&displacement,4);
}
#define REDIRECT(site,target) redirect(site,target)
// Existing title/load helpers are only address dependencies of the installer.
void launch_transition() {}
void load_script() {}
'''
test = r'''
static uint8_t probe_memory[0xA00000];
int main() {
    mission_manager manager;
    auto reset = [&]() {
        manager = mission_manager{};
        live_game = game{};
        g_game_ptr = &live_game;
        resource_manager::idle = true;
        orphaned_black = false;
        test_clock::current = test_clock::time_point{};
        load_during_native_frame = false;
        native_frames = 0;
    };
    auto frame_at = [&](int milliseconds) {
        test_clock::current = test_clock::time_point{} + std::chrono::milliseconds(milliseconds);
        const auto before = native_frames;
        manager.frame_advance(0.0f); // held black freezes game time
        CHECK(native_frames == before+1);
        CHECK(last_increment == 0.0f);
    };
    auto block = [&](int condition) {
        switch (condition) {
        case 0: manager.m_script = reinterpret_cast<void *>(1); break;
        case 1: manager.m_script_to_load = reinterpret_cast<void *>(1); break;
        case 2: manager.m_unload_script = true; break;
        case 3: manager.field_80 = true; break;
        case 4: g_game_ptr = nullptr; break;
        case 5: live_game.level.load_completed = false; break;
        case 6: live_game.flag.level_is_loaded = false; break;
        case 7: resource_manager::idle = false; break;
        case 8: manager.field_FC = 0; break;
        case 9: manager.field_FC = 1; break;
        case 10: manager.field_FC = 2; break;
        }
    };
    auto unblock = [&]() {
        manager.m_script = manager.m_script_to_load = nullptr;
        manager.m_unload_script = manager.field_80 = false;
        manager.field_FC = 3;
        g_game_ptr = &live_game;
        live_game.level.load_completed = live_game.flag.level_is_loaded = true;
        resource_manager::idle = true;
    };
    for (int condition=0;condition<11;++condition) {
        reset();
        block(condition);
        frame_at(0); frame_at(5000);
        CHECK(manager.unfreezes == 0);
        CHECK(!orphaned_black);
        unblock();
        frame_at(5001); frame_at(6000);
        CHECK(manager.unfreezes == 0);
        frame_at(6001);
        CHECK(manager.unfreezes == 1);
        CHECK(manager.field_FC == 2 && manager.field_F8 == -2.0f);
        CHECK(manager.field_F4 == 1.0f && !live_game.field_166);
        frame_at(20000);
        CHECK(manager.unfreezes == 1);
        // Every interruption invalidates elapsed grace from a prior candidate.
        reset();
        frame_at(0); frame_at(999);
        block(condition); frame_at(1000);
        CHECK(manager.unfreezes == 0 && !orphaned_black);
        unblock();
        frame_at(1001); frame_at(2000);
        CHECK(manager.unfreezes == 0);
        frame_at(2001);
        CHECK(manager.unfreezes == 1);
    }
    for (int held : {3,4}) {
        reset(); manager.field_FC=held;
        frame_at(0); frame_at(999);
        CHECK(manager.unfreezes==0);
        frame_at(1000);
        CHECK(manager.unfreezes==1 && manager.field_FC==2);
    }
    reset(); frame_at(0);
    load_during_native_frame=true;
    frame_at(1000);
    CHECK(manager.unfreezes==0 && !orphaned_black);
    // Source installer writes real x86 calls into isolated fixed-base BSS.
    CHECK(reinterpret_cast<uintptr_t>(probe_memory)<=0x550000);
    CHECK(reinterpret_cast<uintptr_t>(probe_memory)+sizeof(probe_memory)>=0x5F0000);
    constexpr uintptr_t sites[]={0x55D75B,0x5DEF4C,0x5E1B1F};
    const uint8_t originals[][5]={{0xE8,0x50,0x3F,8,0},
        {0xE8,0x5F,0xB5,0xFF,0xFF},{0xE8,0x1C,0xD3,0xFF,0xFF}};
    auto initialize_code = [&]() {
        for (int i=0;i<3;++i) {
            std::memset(reinterpret_cast<void *>(sites[i]-8),0xCC,21);
            std::memcpy(reinterpret_cast<void *>(sites[i]),originals[i],5);
        }
    };
    auto call_target = [&](uintptr_t site) {
        int32_t displacement;
        CHECK(*reinterpret_cast<uint8_t *>(site)==0xE8);
        std::memcpy(&displacement,reinterpret_cast<void *>(site+1),4);
        return reinterpret_cast<void *>(site+5+displacement);
    };
    initialize_code();
    CHECK(call_target(sites[0])==reinterpret_cast<void *>(0x5E16B0));
    CHECK(mission_manager_v10_patch());
    CHECK(call_target(sites[0])==function_address(&mission_manager::frame_advance));
    CHECK(call_target(sites[1])==function_address(&launch_transition));
    CHECK(call_target(sites[2])==function_address(&load_script));
    std::array<std::array<uint8_t,21>,3> snapshot;
    for (int i=0;i<3;++i) {
        auto *bytes=reinterpret_cast<uint8_t *>(sites[i]-8);
        for (int j=0;j<21;++j) if (j<8 || j>=13) CHECK(bytes[j]==0xCC);
        std::memcpy(snapshot[i].data(),bytes,21);
    }
    CHECK(mission_manager_v10_patch());
    for (int i=0;i<3;++i) CHECK(std::memcmp(snapshot[i].data(),reinterpret_cast<void *>(sites[i]-8),21)==0);
    initialize_code();
    *reinterpret_cast<uint8_t *>(sites[0]+1)^=1;
    for (int i=0;i<3;++i) std::memcpy(snapshot[i].data(),reinterpret_cast<void *>(sites[i]-8),21);
    CHECK(!mission_manager_v10_patch());
    for (int i=0;i<3;++i) CHECK(std::memcmp(snapshot[i].data(),reinterpret_cast<void *>(sites[i]-8),21)==0);
    std::puts("PASS: active/pending/unloading/requested missions and unready/busy world stay black; every interruption resets grace; held states 3/4 release after 1s with 0.5s fade at zero game time; native update runs first.");
    std::puts("PASS: all 3 installed call targets, preserved surrounding bytes, idempotence, and atomic frame-signature rejection.");
}
'''
compiler = shutil.which(args.cxx) or str(Path(args.cxx).resolve())
env = dict(os.environ)
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='xbpack-v10-mission-') as temp:
    cpp, exe = Path(temp) / 'probe.cpp', Path(temp) / 'probe.exe'
    cpp.write_text(prologue + guard_source + '\n' + fade + '\n' + frame_source
                   + '\n' + call_constants + '\n' + installer + test)
    subprocess.run([compiler,'-std=c++17','-O2','-static','-m32',
                    '-Wl,--image-base,0x400000','-Wl,--disable-dynamicbase',
                    str(cpp),'-o',str(exe)],env=env,check=True)
    subprocess.run([str(exe)],env=env,check=True)
