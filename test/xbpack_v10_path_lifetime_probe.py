"""Verify V10 path removal preserves the other registered mission paths.

Compiles the production removal method with the repository's real legacy
vector. The S03 path keys come from its four serialized PATH resources.
Pass --before <source snapshot> to verify the regression rejects the old code.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
PRELUDE = r'''
#define OPENUSM_XBPACK_V10
#include <vector.hpp>
#include <cassert>
#include <cstdint>
#include <iostream>
#define THISCALL(...) ((void)0)
struct path_graph { uint32_t hash; };
struct wds_ai_manager {
    _std::vector<path_graph *> path_graph_list;
    void add_path_graph(path_graph *);
};
'''
CHECK = r'''
int main() {
    path_graph paths[] = {{0x8A4B2C2B}, {0x8A54426D},
                          {0x8A544270}, {0xD4E08FA1}};
    path_graph absent {0xFFFFFFFF};
    const unsigned orders[][4] = {{0,1,2,3}, {2,0,3,1}, {3,2,1,0}};
    for (const auto &order : orders) {
        wds_ai_manager manager;
        auto &list = manager.path_graph_list;
        // Native push_back populates this same storage in the game. Seed it
        // directly so this probe isolates removal from legacy insertion code.
        list._Buy(4);
        for (auto &path : paths) *list.m_last++ = &path;
        bool live[] = {true, true, true, true};
        for (unsigned step = 0; step < 4; ++step) {
            manager.add_path_graph(&paths[order[step]]);
            live[order[step]] = false;
            if (list.size() != 3 - step) {
                std::cerr << "FAIL: removing S03 path " << order[step]
                          << " left " << list.size() << " paths; expected "
                          << 3 - step << '\n';
                return 1;
            }
            unsigned position = 0;
            for (unsigned i = 0; i < 4; ++i) {
                if (live[i] && list[position++] != &paths[i]) {
                    std::cerr << "FAIL: an unrelated path was lost or reordered\n";
                    return 2;
                }
            }
            // Removing an absent/already removed resource must preserve all
            // survivors; this also covers the empty registry after teardown.
            manager.add_path_graph(&absent);
            manager.add_path_graph(&paths[order[step]]);
            if (list.size() != 3 - step) return 3;
        }
    }
    {
        wds_ai_manager manager;
        auto &list = manager.path_graph_list;
        path_graph *registered[] = {&paths[0], nullptr, &paths[1], &paths[0],
                                    &paths[2], &paths[3], &paths[0]};
        path_graph *survivors[] = {nullptr, &paths[1], &paths[2], &paths[3]};
        list._Buy(7);
        for (auto *path : registered) *list.m_last++ = path;
        manager.add_path_graph(&paths[0]);
        if (list.size() != 4) return 4;
        for (unsigned i = 0; i < 4; ++i) {
            if (list[i] != survivors[i]) return 5;
        }
    }
    std::cout << "PASS: S03 path registry survives first/middle/last removal "
                 "and repeated teardown; duplicate removal preserves null entries\n";
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'src/wds_ai_manager.cpp')
    parser.add_argument('--before', type=Path)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'g++'))
    args = parser.parse_args()
    compiler = shutil.which(args.cxx) or str(Path(args.cxx).resolve())
    env = dict(os.environ)
    env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
    sources = [('before', args.before)] if args.before else []
    sources.append(('after', args.source))
    with tempfile.TemporaryDirectory(prefix='usm-v10-path-') as temporary:
        directory = Path(temporary)
        for label, path in sources:
            source = path.read_text()
            start = source.index('void wds_ai_manager::add_path_graph(')
            begin = source.index('{', start)
            depth = 1
            end = begin + 1
            while depth:
                depth += (source[end] == '{') - (source[end] == '}')
                end += 1
            cpp = directory / (label + '.cpp')
            executable = directory / (label + ('.exe' if os.name == 'nt' else ''))
            cpp.write_text(PRELUDE + source[start:end] + CHECK)
            subprocess.run([compiler, '-std=c++17', '-O2', '-I', str(ROOT / 'include'),
                            str(cpp), '-o', str(executable)], env=env, check=True)
            result = subprocess.run([str(executable)], env=env)
            expected = 1 if label == 'before' else 0
            if result.returncode != expected:
                return result.returncode or 99
            print(f'{label}: expected exit {expected} verified', flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
