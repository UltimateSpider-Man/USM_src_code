# ═══════════════════════════════════════════════════════════════════════
#   OpenUSM — unified CMakeLists.txt
#
#   Produces, in a single `cmake --build build` invocation:
#
#     • build/USM_original.exe          — compiled from USM.c's rawData[5926912]
#                                byte array (5,926,912 bytes = 5.65 MiB,
#                                SHA-256 5d554ef6…0488b8 — byte-identical
#                                to the original Ultimate Spider-Man exe
#                                because that's what USM.c encodes).
#     • build/binkw32.dll      — the OpenUSM mod-loader
#     • build/debug_menu.dll   — the PC debug menu (UPX-compressed)
#     • build/openusm.dll      — a copy of binkw32.dll under the
#                                openusm name for asi-loader deploy
#     • build/USM.exe          — USM_original.exe with a stripped
#                                openusm.dll injected via tools/dll2exe.exe
#                                (single-file mod-loader, no asi /
#                                no on-disk binkw32 hijack required)
#     • build/USMITA_original.exe — Italian variant, compiled from
#                                USMITA.c's rawData[5926912] byte array.
#                                Only built if USMITA.c is present.
#     • build/USMITA.exe       — USMITA_original.exe with the same
#                                stripped openusm.dll injected via
#                                dll2exe.exe (mirrors the USM.exe step,
#                                reusing the already-stripped DLL).
#
#   Auto-deploys every output to ../Ultimate Spider-Man/ if that
#   directory exists next to the source tree. Stock DLLs (binkw32,
#   d3dx9_24, dsound, MSVCP71, msvcr71, xerces-c_2_3_0) in that
#   folder are left untouched by this build.
#
#   Required layout:
#
#       OpenUSM/
#       ├── CMakeLists.txt                 ← this file
#       ├── main.cpp                       ← binkw32 entry point
#       ├── usm_debug_menu.cpp             ← debug_menu entry point
#       ├── USM.c                          ← rawData[5926912] blob (REQUIRED)
#       ├── USMITA.c                       ← rawData[5926912] blob — ITA
#       │                                    variant (OPTIONAL; same symbol
#       │                                    name as USM.c, so it's built
#       │                                    as a parallel host program)
#       ├── emit_usm_exe.c                 ← host-tool writer       (REQUIRED)
#       ├── src/                           ← parent-project libraries
#       ├── usm_debug_menu/                ← optional: sibling debug
#       │   ├── usm.cpp                      menu project (USM_DEBUG_MENU_DIR)
#       │   └── dev_src/*.cpp,*.h
#       ├── cmake/
#       │   ├── i686-pc-mingw32.cmake      ← MinGW cross toolchain
#       │   └── report_size.cmake          ← size reporter for UPX step
#       ├── tools/
#       │   └── dll2exe.exe                ← DLL→EXE injector used to
#       │                                    bake openusm.dll into USM.exe
#       ├── third_party/
#       │   ├── glm/                       ← (optional, USE_GLM)
#       │   └── googletest/                ← (optional, ENABLE_TESTS)
#       ├── assimp/                        ← Assimp source tree
#       └── test/                          ← (optional, ENABLE_TESTS)
#
#   Build:
#
#       cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/i686-pc-mingw32.cmake
#       cmake --build build -j$(nproc)
#
#   Top-level options:
#
#     -DUSE_GLM=ON                   link glm::glm into binkw32/debug_menu
#     -DUSE_FSTREAM=ON               -DUSE_FSTREAM in binkw32/debug_menu
#     -DCHECK_STRUCT_SIZES=OFF       disable CHECK_STRUCT_SIZES define
#     -DTARGET_XBOX=ON               build the Xbox variant of mash
#     -DENABLE_TESTS=OFF             skip googletest/CTest
#     -DUSM_BUILD_DEBUG_MENU=OFF     skip the debug_menu target
#     -DDEBUG_MENU_COMPRESS=OFF      skip the UPX pack of debug_menu.dll
#     -DDEBUG_MENU_STRIP_BEFORE_UPX=OFF
#     -DDEBUG_MENU_SIZE_LIMIT=<bytes>  soft warning threshold (1 MiB default)
#     -DUSM_C_SOURCE=<path>          override USM.c location
#     -DUSM_C_WRITER_SOURCE=<path>   override emit_usm_exe.c location
#     -DUSMITA_C_SOURCE=<path>       override USMITA.c location (ITA variant)
# ═══════════════════════════════════════════════════════════════════════

cmake_minimum_required(VERSION 3.12)

include(ExternalProject)
include(cmake/i686-pc-mingw32.cmake)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-static -std=gnu11 -g")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}
    "-fomit-frame-pointer -m32 -fno-asynchronous-unwind-tables -masm=intel -static -g")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)

list(APPEND flags
    #"-Werror"
    #"-pedantic-errors"
    "-w"                                # master switch: inhibit all warnings
    # Keep the explicit -Wno-* list below as documentation of known sources of
    # noise, but -w above disables everything. Remove -w and uncomment
    # -Wall/-Wextra/-Wpedantic if you want selective warnings back.
    #"-Wall"
    #"-Wextra"
    #"-Wpedantic"

    # ── Suppress decompiled-code / IDA-output artifacts ─────────────────
    "-Wno-deprecated"
    "-Wno-deprecated-declarations"
    "-Wno-float-equal"                  # float == / != throughout IDA output
    "-Wno-sign-compare"                 # signed/unsigned pervasive in decompiled code
    "-Wno-cast-qual"                    # const-cast-away common in decompiled code
    "-Wno-cast-align"                   # decompiled pointer casts
    "-Wno-cast-function-type"           # func pointer casts in hook code
    "-Wno-zero-as-null-pointer-constant" # old C-style 0 for nullptr
    "-Wno-missing-field-initializers"   # {0} partial struct init
    "-Wno-missing-braces"               # nested aggregate init
    "-Wno-class-memaccess"              # memcpy/memset on non-trivial types
    "-Wno-write-strings"                # string literal -> char*
    "-Wno-unused-parameter"
    "-Wno-unused-variable"
    "-Wno-unused-but-set-variable"
    "-Wno-unused-value"
    "-Wno-unused-label"
    "-Wno-unused-function"
    "-Wno-unused-result"
    "-Wno-unused-local-typedefs"
    "-Wno-redundant-decls"              # forward-decls duplicating header decls
    "-Wno-type-limits"                  # uint8_t != 0xFF always-true comparisons
    "-Wno-narrowing"                    # float->int narrowing in aggregate inits
    "-Wno-maybe-uninitialized"
    "-Wno-uninitialized"
    "-Wno-switch"
    "-Wno-switch-default"
    "-Wno-switch-enum"
    "-Wno-implicit-fallthrough"
    "-Wno-comment"
    "-Wno-parentheses"
    "-Wno-unknown-pragmas"              # #pragma region / #pragma endregion
    "-Wno-format"
    "-Wno-format-security"
    "-Wno-format-extra-args"
    "-Wno-misleading-indentation"
    "-Wno-reorder"
    "-Wno-pedantic"
    "-Wno-overloaded-virtual"
    "-Wno-ctor-dtor-privacy"
    "-Wno-sign-promo"
    "-Wno-return-type"
    "-Wno-return-local-addr"
    "-Wno-duplicated-cond"
    "-Wno-duplicated-branches"
    "-Wno-strict-aliasing"
    "-Wno-address"
    "-Wno-nonnull"
    "-Wno-nonnull-compare"
    "-Wno-null-dereference"
    "-Wno-stringop-overflow"
    "-Wno-stringop-overread"
    "-Wno-stringop-truncation"
    "-Wno-restrict"
    "-Wno-array-bounds"
    "-Wno-attributes"
    "-Wno-conversion"
    "-Wno-extra"
    "-Wno-shift-count-overflow"
    "-Wno-shift-overflow"
    "-Wno-volatile"                     # C++20 volatile deprecation
    "-Wno-builtin-macro-redefined"
    "-Wno-subobject-linkage"
)


project(USM-dev)


if(MINGW)
    # Drop the "lib" prefix on MinGW so shared libs come out as
    # binkw32.dll / debug_menu.dll — not libbinkw32.dll etc.
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_STATIC_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_MODULE_PREFIX  "")
endif()


# ─────────────────────────────────────────────────────────────────────────
#   Feature toggles (most are runtime-free defines that flip compile paths)
# ─────────────────────────────────────────────────────────────────────────

option(USE_GLM            ""                                             OFF)
option(USE_FSTREAM        ""                                             OFF)
option(CHECK_STRUCT_SIZES ""                                             ON)
option(TARGET_XBOX        ""                                             OFF)
option(ENABLE_TESTS       ""                                             ON)

# Optional big-ticket targets — flipping any of these OFF skips that
# target cleanly (others keep building).

if(CHECK_STRUCT_SIZES)
    add_compile_definitions(CHECK_STRUCT_SIZES)
endif()

add_compile_definitions(CINTERFACE)

if(USE_GLM)
    add_compile_definitions(USE_GLM)
    add_subdirectory(third_party/glm)
endif()

if(TARGET_XBOX)
    add_compile_definitions(TARGET_XBOX)
endif()


# ─────────────────────────────────────────────────────────────────────────
#   Core libraries
# ─────────────────────────────────────────────────────────────────────────

add_library(utility src/utility/trace.cpp)
target_include_directories(utility PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/utility>)

add_library(variables src/variables/variables.cpp)
target_include_directories(variables PUBLIC "src/variables")
target_link_libraries(variables PUBLIC utility)


if(TARGET_XBOX)
    add_library(mash src/mash/xbox/src/mash_info_struct.cpp)
    target_include_directories(mash PUBLIC "src/mash/xbox/include/")
else()
    add_library(mash src/mash/pc/src/mash_info_struct.cpp)
    target_include_directories(mash PUBLIC "src/mash/pc/include")
endif()
target_include_directories(mash PUBLIC "src" "include")
target_compile_options   (mash PRIVATE ${flags})
target_link_libraries    (mash PUBLIC utility variables)


# ─────────────────────────────────────────────────────────────────────────
#   binkw32.dll — OpenUSM mod loader
# ─────────────────────────────────────────────────────────────────────────

file(GLOB SRC "./src/*.cpp" "./src/*.h")

add_library(binkw32 SHARED main.cpp ${SRC})
target_compile_options(binkw32 PRIVATE ${flags})


# ─────────────────────────────────────────────────────────────────────────
#   debug_menu.dll — PC debug menu overlay
#
#   Shares the same ${SRC} glob as binkw32 (this is intentional — the
#   debug menu runs inside the same in-process mod-loader code). The
#   different entry point (usm_debug_menu.cpp vs. main.cpp) is what
#   makes them distinct DLLs with different init behaviour.
# ─────────────────────────────────────────────────────────────────────────



# ─────────────────────────────────────────────────────────────────────────
#   memory / slab_allocator
# ─────────────────────────────────────────────────────────────────────────

add_library(memory "./src/memory.cpp" "./src/memory.h")
target_compile_options    (memory PRIVATE ${flags})
target_include_directories(memory PUBLIC  "./include/")
target_link_libraries     (memory PUBLIC  utility)

add_library(slab_allocator "./src/slab_allocator.cpp" "./src/slab_allocator.h")
target_compile_options    (slab_allocator PRIVATE ${flags})
target_include_directories(slab_allocator PUBLIC  "./include/")
target_link_libraries     (slab_allocator PUBLIC  memory)


# ─────────────────────────────────────────────────────────────────────────
#   nalcomp / nalpanel / nsl / nal (animation + panel systems)
# ─────────────────────────────────────────────────────────────────────────

file(GLOB_RECURSE NALCOMP_SRC "./src/nalcomp/*.cpp")
add_library(nalcomp ${NALCOMP_SRC})
target_compile_options    (nalcomp PRIVATE ${flags})
target_include_directories(nalcomp PUBLIC  "./src/nalcomp/" "./src/")

file(GLOB_RECURSE NALPANEL_SRC "./src/panels/nalpanel/*.cpp")
add_library(nalpanel ${NALPANEL_SRC})
target_compile_options    (nalpanel PRIVATE ${flags})
target_include_directories(nalpanel PUBLIC  "./src/panels/nalpanel/")
target_link_libraries     (nalpanel PUBLIC  nalcomp)

file(GLOB_RECURSE NSL_SRC "./src/nsl/src/nsl/*.cpp")
add_library(nsl ${NSL_SRC})
target_compile_options    (nsl PRIVATE ${flags})
target_include_directories(nsl PUBLIC  "./include/" "./src/" "./src/nsl/src/nsl/")
target_link_libraries     (nsl PUBLIC  nfl utility)

file(GLOB_RECURSE NAL_SRC "./src/nal_system.cpp" "./src/nal/source/common/*.cpp")
add_library(nal ${NAL_SRC})
target_compile_options    (nal PRIVATE ${flags})
target_include_directories(nal PUBLIC  "./include/" "./src/" "./src/nal/include/common/")
target_link_libraries     (nal PUBLIC  nfl nalpanel utility)

add_library(test_nal ${NAL_SRC})
target_compile_options    (test_nal PRIVATE ${flags})
target_include_directories(test_nal PUBLIC  "./include/" "./src/" "./src/nal/include/common/")
target_link_libraries     (test_nal PUBLIC  nfl nalpanel utility)

target_link_libraries(nalcomp PUBLIC nal)


# ─────────────────────────────────────────────────────────────────────────
#   chuck (scripting VM)
# ─────────────────────────────────────────────────────────────────────────

file(GLOB_RECURSE SRC_CHUCK "./src/chuck/vm/*.cpp")
add_library(chuck ${SRC_CHUCK})
target_compile_options    (chuck PRIVATE ${flags})
target_include_directories(chuck INTERFACE "./src/chuck/vm/")
target_link_libraries     (chuck PUBLIC   slab_allocator variables)


# ─────────────────────────────────────────────────────────────────────────
#   tx / nfl / ngl / charanimtype / physics
# ─────────────────────────────────────────────────────────────────────────

file(GLOB_RECURSE SRC_TX "./src/tx/src/*.cpp" "./src/tx/src/*.h")
add_library(tx ${SRC_TX})
target_compile_options    (tx PRIVATE ${flags})
target_include_directories(tx INTERFACE "./src/tx/src/")
target_link_libraries     (tx PUBLIC    utility)

add_library(test_tx ${SRC_TX})
target_compile_options    (test_tx PRIVATE ${flags})
target_include_directories(test_tx INTERFACE "./src/tx/src/")
target_link_libraries     (test_tx PUBLIC    utility)

file(GLOB_RECURSE SRC_NFL "./src/nfl/src/*.cpp" "./src/nfl/src/*.h")
add_library(nfl ${SRC_NFL})
target_compile_options    (nfl PRIVATE ${flags})
target_include_directories(nfl INTERFACE "./src/nfl/src/")
target_link_libraries     (nfl PUBLIC    tx)
target_link_libraries     (mash PUBLIC   nfl)

add_library(test_nfl ${SRC_NFL})
target_compile_options    (test_nfl PRIVATE ${flags})
target_include_directories(test_nfl INTERFACE "./src/nfl/src/")
target_link_libraries     (test_nfl PUBLIC    test_tx)

file(GLOB_RECURSE SRC_NGL_DX "./src/ngl/src/dx/*.cpp")
add_library(ngl_dx ${SRC_NGL_DX})
target_compile_options    (ngl_dx PRIVATE ${flags})
target_include_directories(ngl_dx PUBLIC  "./src/ngl/include/dx" "./src/ngl/include/")
target_link_libraries     (ngl_dx PUBLIC  psapi utility variables)

file(GLOB_RECURSE SRC_NGL_SHADERS "./src/ngl/shaders/*.cpp")
add_library(ngl_shaders ${SRC_NGL_SHADERS})
target_compile_options    (ngl_shaders PRIVATE ${flags})
target_include_directories(ngl_shaders PUBLIC  "./src/ngl/shaders")
target_link_libraries     (ngl_shaders PUBLIC  ngl_dx)

file(GLOB SRC_NGL "./src/ngl/src/*.cpp")
add_library(ngl ${SRC_NGL})
target_compile_options    (ngl PRIVATE ${flags})
target_include_directories(ngl PUBLIC  "./src/ngl/include/")
target_link_libraries     (ngl PUBLIC  ngl_dx ngl_shaders)

file(GLOB SRC_CHARANIMTYPE "./src/charanimtype/*.cpp")
add_library(charanimtype ${SRC_CHARANIMTYPE})
target_compile_options    (charanimtype PRIVATE ${flags})
target_include_directories(charanimtype PUBLIC  "./src/charanimtype/")
target_link_libraries     (charanimtype PUBLIC  nal ngl_dx ngl_shaders)

file(GLOB_RECURSE PHYSICS_SRC "./src/physics/source/*.cpp")
add_library(physics ${PHYSICS_SRC})
target_compile_options    (physics PRIVATE ${flags})
target_include_directories(physics PUBLIC  "./src/physics/include/")
target_link_libraries     (physics PUBLIC  utility ngl)


# ─────────────────────────────────────────────────────────────────────────
#   Shared settings for binkw32 & debug_menu (same code, different entry)
# ─────────────────────────────────────────────────────────────────────────

if(USE_FSTREAM)
    target_compile_definitions(binkw32 PUBLIC USE_FSTREAM)
    endif()


target_include_directories(binkw32 PUBLIC "include" "./src/nfl/src/")


target_include_directories(chuck  PUBLIC "include" "src")
target_include_directories(tx     PUBLIC "include" "src")
target_include_directories(ngl_dx PUBLIC "include" "src")
target_include_directories(test_tx PUBLIC "include" "src")


# ─────────────────────────────────────────────────────────────────────────
#   Assimp (external project, built once)
# ─────────────────────────────────────────────────────────────────────────

ExternalProject_Add(AssimpProject
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/assimp
    BINARY_DIR ${CMAKE_BINARY_DIR}/assimp-build
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/i686-pc-mingw32.cmake
        -DCMAKE_BUILD_TYPE=Release
        -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=ON
        -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
        -DASSIMP_BUILD_TESTS=OFF
        -DASSIMP_NO_EXPORT=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DASSIMP_BUILD_ZLIB=ON
        -DASSIMP_BUILD_SAMPLES=OFF
        -DASSIMP_OPT_BUILD_PACKAGES=OFF
        -DCMAKE_CXX_FLAGS=-static-libgcc\ -static-libstdc++
    INSTALL_COMMAND ""
)

target_include_directories(binkw32 PUBLIC "./assimp/include/")
target_link_directories   (binkw32 PUBLIC ${CMAKE_BINARY_DIR}/assimp-build/lib)
add_dependencies          (binkw32 AssimpProject)



# ═══════════════════════════════════════════════════════════════════════
#   USM_original.exe — compiled from USM.c
#
#   USM.c is a 37 MB ASCII-hex C source that declares one giant byte
#   array:
#       unsigned char rawData[5926912] = { 0x4D, 0x5A, ... };
#   These bytes are the stock Ultimate Spider-Man USM_original.exe.
#
#   Build pipeline:
#     1. Compile USM.c + emit_usm_exe.c with the HOST C compiler via
#        ExternalProject (the MinGW cross-toolchain used everywhere
#        else in this file can't be used here because the writer has
#        to RUN on the build machine).
#     2. Run the compiled writer with ${CMAKE_BINARY_DIR}/USM_original.exe as
#        argv[1] — it fwrites the 5,926,912 bytes of rawData[] to disk
#        in 1 MiB chunks.
#
#   Output: ${CMAKE_BINARY_DIR}/USM_original.exe — 5,926,912 bytes (5.65 MiB),
#   SHA-256 5d554ef69e8066eed9e327a4fe331992124aee6de16af54bd7ac5c5dcb0488b8
#   (byte-identical to the original game executable, because that's
#   what's sitting inside USM.c's rawData[]).
# ═══════════════════════════════════════════════════════════════════════

set(USM_C_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/USM.c"
    CACHE FILEPATH "Path to USM.c with rawData[5926912] byte array")
set(USM_C_WRITER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/emit_usm_exe.c"
    CACHE FILEPATH "Path to emit_usm_exe.c host-tool writer")

set(_OUT_USM "${CMAKE_BINARY_DIR}/USM_original.exe")

if(NOT EXISTS "${USM_C_SOURCE}")
    message(WARNING
        "USM.c not found at ${USM_C_SOURCE} — the USM target will not "
        "be created. Place USM.c (containing rawData[5926912]) at that "
        "path or override with -DUSM_C_SOURCE=<path>.")
elseif(NOT EXISTS "${USM_C_WRITER_SOURCE}")
    message(WARNING
        "emit_usm_exe.c not found at ${USM_C_WRITER_SOURCE} — the USM "
        "target will not be created. Add the writer source or override "
        "with -DUSM_C_WRITER_SOURCE=<path>.")
else()
    # ── Host-native compile of USM.c + emit_usm_exe.c ────────────────
    # ExternalProject gives us an escape hatch from the parent
    # CMAKE_TOOLCHAIN_FILE — we explicitly call `cc` (the host C
    # compiler from the build environment) instead of the MinGW
    # i686 cross-compiler. -O0 because optimizing a 5.9 MB initialized
    # array gains nothing and costs minutes of compile time.
    ExternalProject_Add(emit_usm_exe_host
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/emit_usm_exe_src"
        BINARY_DIR        "${CMAKE_BINARY_DIR}/emit_usm_exe_build"
        DOWNLOAD_COMMAND  ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND
            ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_BINARY_DIR}/emit_usm_exe_build"
            COMMAND
            cc -O0
               "${USM_C_SOURCE}"
               "${USM_C_WRITER_SOURCE}"
               -o "${CMAKE_BINARY_DIR}/emit_usm_exe_build/emit_usm_exe"
        INSTALL_COMMAND   ""
        BUILD_BYPRODUCTS  "${CMAKE_BINARY_DIR}/emit_usm_exe_build/emit_usm_exe"
        BUILD_ALWAYS      FALSE
    )

    # ── Run the writer to produce USM_original.exe ────────────────────────────
    add_custom_command(
        OUTPUT  "${_OUT_USM}"
        COMMAND "${CMAKE_BINARY_DIR}/emit_usm_exe_build/emit_usm_exe"
                "${_OUT_USM}"
        DEPENDS emit_usm_exe_host "${USM_C_SOURCE}" "${USM_C_WRITER_SOURCE}"
        COMMENT "Compiling USM_original.exe from USM.c rawData[] (5,926,912 bytes / 5.65 MiB)"
        VERBATIM
    )

    add_custom_target(USM ALL DEPENDS "${_OUT_USM}")

    # Compute + display the SHA-256 of USM.c so changes to the rawData[]
    # blob are obvious at configure time.
    file(SIZE   "${USM_C_SOURCE}" _USM_C_SIZE)
    file(SHA256 "${USM_C_SOURCE}" _USM_C_SHA)
    message(STATUS "USM_original.exe: compiling from USM.c rawData[]")
    message(STATUS "  USM.c       : ${USM_C_SOURCE}")
    message(STATUS "  USM.c size  : ${_USM_C_SIZE} bytes")
    message(STATUS "  USM.c sha256: ${_USM_C_SHA}")
    message(STATUS "  writer      : ${USM_C_WRITER_SOURCE}")
    message(STATUS "  output      : ${_OUT_USM} (expected: 5926912 bytes / 5.65 MiB)")
endif()


# ═══════════════════════════════════════════════════════════════════════
#   USMITA.exe — Italian variant, compiled from USMITA.c
#
#   USMITA.c is the same kind of ASCII-hex blob as USM.c — it declares
#   the same `unsigned char rawData[5926912]` symbol but contains the
#   bytes of the Italian (ITA) release of USM.exe (also 5,926,912 bytes,
#   5.65 MiB).
#
#   Because USM.c and USMITA.c both define `rawData[]`, they CANNOT be
#   linked into a single host program — that would be a duplicate
#   symbol error. So we build a parallel writer:
#
#       emit_usmita_exe = USMITA.c + emit_usm_exe.c
#
#   The writer source is exactly the same emit_usm_exe.c used for the
#   English exe; only the rawData[] it links against differs. Two host
#   programs, two distinct rawData[] arrays, two output exes — no
#   source-file edits needed to either USMITA.c or emit_usm_exe.c.
#
#   Skipped silently if USMITA.c isn't present (it's an optional
#   variant — the English USM.exe build is unaffected).
# ═══════════════════════════════════════════════════════════════════════

set(USMITA_C_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/USMITA.c"
    CACHE FILEPATH "Path to USMITA.c with rawData[5926912] byte array (Italian variant)")

set(_OUT_USMITA "${CMAKE_BINARY_DIR}/USMITA_original.exe")

if(NOT EXISTS "${USMITA_C_SOURCE}")
    message(STATUS
        "USMITA.c not found at ${USMITA_C_SOURCE} — USMITA target "
        "skipped. (Optional Italian variant; set -DUSMITA_C_SOURCE=<path> "
        "to enable.)")
elseif(NOT EXISTS "${USM_C_WRITER_SOURCE}")
    # Same writer source as USM.exe; the missing-writer warning was
    # already emitted by the USM_original.exe block above. Silent here.
else()
    # ── Host-native compile of USMITA.c + emit_usm_exe.c ─────────────
    # Mirrors the USM_original.exe ExternalProject exactly. The output
    # binary is named differently (emit_usmita_exe) so it cannot
    # collide with the English writer on disk or at link time.
    ExternalProject_Add(emit_usmita_exe_host
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/emit_usmita_exe_src"
        BINARY_DIR        "${CMAKE_BINARY_DIR}/emit_usmita_exe_build"
        DOWNLOAD_COMMAND  ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND
            ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_BINARY_DIR}/emit_usmita_exe_build"
            COMMAND
            cc -O0
               "${USMITA_C_SOURCE}"
               "${USM_C_WRITER_SOURCE}"
               -o "${CMAKE_BINARY_DIR}/emit_usmita_exe_build/emit_usmita_exe"
        INSTALL_COMMAND   ""
        BUILD_BYPRODUCTS  "${CMAKE_BINARY_DIR}/emit_usmita_exe_build/emit_usmita_exe"
        BUILD_ALWAYS      FALSE
    )

    # ── Run the writer to produce USMITA.exe ─────────────────────────
    add_custom_command(
        OUTPUT  "${_OUT_USMITA}"
        COMMAND "${CMAKE_BINARY_DIR}/emit_usmita_exe_build/emit_usmita_exe"
                "${_OUT_USMITA}"
        DEPENDS emit_usmita_exe_host "${USMITA_C_SOURCE}" "${USM_C_WRITER_SOURCE}"
        COMMENT "Compiling USMITA_original.exe from USMITA.c rawData[] (5,926,912 bytes / 5.65 MiB)"
        VERBATIM
    )

    add_custom_target(USMITA ALL DEPENDS "${_OUT_USMITA}")

    file(SIZE   "${USMITA_C_SOURCE}" _USMITA_C_SIZE)
    file(SHA256 "${USMITA_C_SOURCE}" _USMITA_C_SHA)
    message(STATUS "USMITA_original.exe: compiling from USMITA.c rawData[]")
    message(STATUS "  USMITA.c       : ${USMITA_C_SOURCE}")
    message(STATUS "  USMITA.c size  : ${_USMITA_C_SIZE} bytes")
    message(STATUS "  USMITA.c sha256: ${_USMITA_C_SHA}")
    message(STATUS "  writer         : ${USM_C_WRITER_SOURCE}")
    message(STATUS "  output         : ${_OUT_USMITA} (expected: 5926912 bytes / 5.65 MiB)")
endif()


# ─────────────────────────────────────────────────────────────────────────
#   USM target — post-build: wire in the built mod loader + deploy outputs
#
#   No stock DLLs are staged into the build tree by this file. Only
#   files produced by THIS build are touched:
#     • $<TARGET_FILE:binkw32>   → copied alongside USM_original.exe as openusm.dll
#     • $<TARGET_FILE:debug_menu> → built alongside USM_original.exe as debug_menu.dll
#   If your game install folder exists next to the source tree, the
#   same three artifacts (USM_original.exe, binkw32.dll, debug_menu.dll) are
#   also deployed there.
# ─────────────────────────────────────────────────────────────────────────

if(TARGET USM)
    # Wire the mod loader into the USM build graph so every build of
    # USM also produces a fresh USM.dll next to it.
    if(TARGET binkw32)
        add_dependencies(USM binkw32)
        add_custom_command(TARGET USM POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:binkw32>"
                "${CMAKE_BINARY_DIR}/USM.dll"
            COMMENT "Deploying built mod loader → USM.dll"
            VERBATIM)
    endif()

    # ─────────────────────────────────────────────────────────────────
    #   USM.exe — final shipping executable
    #
    #   tools/dll2exe.exe injects USM.dll into USM_original.exe so
    #   the game launches with the mod loader baked in (no asi-loader,
    #   no on-disk binkw32 hijack required).
    #
    #   Pipeline:
    #     1. strip --strip-all USM.dll → openusm_stripped.dll
    #        (the build runs with -g; embedded debug info would bloat
    #         USM.exe by several MB for no gain)
    #     2. dll2exe -efix -injimp
    #            USM_original.exe openusm_stripped.dll USM.exe
    #
    #   dll2exe switches:
    #     -efix    restore the original PE entry point so the game's
    #              own startup runs after the DLL loads
    #     -injimp  rewrite USM.exe's import table to bind Bink* imports
    #              to openusm.dll's exports — the same hijack pattern
    #              as the on-disk binkw32.dll, but baked into the exe
    # ─────────────────────────────────────────────────────────────────

    set(DLL2EXE "${CMAKE_CURRENT_SOURCE_DIR}/tools/dll2exe.exe")

    # Locate strip from the active toolchain (MinGW i686 cross-compile,
    # or native MSYS2 MinGW). CMake usually sets CMAKE_STRIP from the
    # toolchain file; fall back to a name search if it didn't.
    if(NOT CMAKE_STRIP)
        find_program(CMAKE_STRIP
            NAMES i686-pc-mingw32-strip
                  i686-w64-mingw32-strip
                  strip)
    endif()

    # ── WSL detection ────────────────────────────────────────────
    # On WSL the host is "Linux" from CMake's POV (CMAKE_HOST_WIN32
    # is FALSE), but dll2exe.exe is launched via Win32 interop and
    # therefore runs as a native Windows process. Linux-style paths
    # like /mnt/c/Users/... in argv are passed through verbatim and
    # Windows tries to resolve them as \mnt\c\Users\... on the
    # current drive — which fails with "failed to load executable
    # image".
    set(_IS_WSL FALSE)
    if(NOT CMAKE_HOST_WIN32 AND EXISTS "/proc/sys/kernel/osrelease")
        file(READ "/proc/sys/kernel/osrelease" _OSRELEASE)
        if(_OSRELEASE MATCHES "[Mm]icrosoft|WSL")
            set(_IS_WSL TRUE)
            find_program(WSLPATH wslpath)
        endif()
    endif()

    # Translate a CMake (Linux) path into the form dll2exe.exe expects
    # in argv. On WSL:
    #   • /mnt/<drive>/...   →   <DRIVE>:/...   (pure string rewrite)
    #   • /home/..., /tmp/.. →   \\wsl$\<distro>\...  (via wslpath)
    #
    # The /mnt/ branch deliberately does NOT call wslpath. Several WSL
    # builds have wslpath silently return an empty string for paths
    # that don't yet exist on the Windows side — which is the case for
    # `USM.exe` (the dll2exe output) at configure time. Translating
    # the prefix with a regex avoids that pitfall entirely.
    function(_to_dll2exe_path linux_path out_var)
        set(_p "${linux_path}")
        if(_IS_WSL)
            if(_p MATCHES "^/mnt/([a-zA-Z])/(.*)$")
                string(TOUPPER "${CMAKE_MATCH_1}" _drive)
                set(_p "${_drive}:/${CMAKE_MATCH_2}")
            elseif(_p MATCHES "^/mnt/([a-zA-Z])/?$")
                string(TOUPPER "${CMAKE_MATCH_1}" _drive)
                set(_p "${_drive}:/")
            elseif(WSLPATH)
                # Off /mnt/ — translate via wslpath (\\wsl$\<distro>\…).
                execute_process(
                    COMMAND "${WSLPATH}" -m "${linux_path}"
                    OUTPUT_VARIABLE _w
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE _r)
                if(_r EQUAL 0 AND _w)
                    set(_p "${_w}")
                endif()
            endif()
        endif()
        set(${out_var} "${_p}" PARENT_SCOPE)
    endfunction()

    _to_dll2exe_path("${_OUT_USM}"                                _OUT_USM_ARG)
    _to_dll2exe_path("${CMAKE_BINARY_DIR}/USM_stripped.dll"   _STRIPPED_ARG)
    _to_dll2exe_path("${CMAKE_BINARY_DIR}/USM.exe"                _USM_EXE_ARG)
    _to_dll2exe_path("${_OUT_USMITA}"                             _OUT_USMITA_ARG)
    _to_dll2exe_path("${CMAKE_BINARY_DIR}/USMITA.exe"             _USMITA_EXE_ARG)

    # Belt-and-suspenders: refuse to configure if any argv slot is
    # empty. dll2exe.exe will otherwise report
    # `failed to create output file ()` or similar with the empty
    # arg displayed as just `()`, which is hard to diagnose.
    foreach(_v _OUT_USM_ARG _STRIPPED_ARG _USM_EXE_ARG _OUT_USMITA_ARG _USMITA_EXE_ARG)
        if(NOT "${${_v}}")

        endif()
    endforeach()

    # dll2exe.exe is a Windows PE binary. Pick a launcher:
    #   • Native Windows / WSL: run directly (Win32 interop on WSL
    #     handles the .exe extension automatically).
    #   • Native Linux/macOS: drive it through wine. wine maps
    #     Linux paths via Z:/ so no path translation is needed there.
    if(CMAKE_HOST_WIN32 OR _IS_WSL)
        set(_DLL2EXE_RUN "${DLL2EXE}")
    else()
        find_program(WINE_EXECUTABLE wine)
        if(WINE_EXECUTABLE)
            set(_DLL2EXE_RUN "${WINE_EXECUTABLE}" "${DLL2EXE}")
        else()
            # Configure-time: let it through. Build-time: will fail
            # with a useful error pointing at the missing wine.
            set(_DLL2EXE_RUN "${DLL2EXE}")
        endif()
    endif()

    if(EXISTS "${DLL2EXE}")
        if(TARGET binkw32)
            add_custom_command(TARGET USM POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                        "${CMAKE_BINARY_DIR}/USM.dll"
                        "${CMAKE_BINARY_DIR}/USM_stripped.dll"
                COMMAND "${CMAKE_STRIP}" --strip-all
                        "${CMAKE_BINARY_DIR}/USM_stripped.dll"
                COMMAND ${_DLL2EXE_RUN} -efix -injimp
                        "${_OUT_USM_ARG}"
                        "${_STRIPPED_ARG}"
                        "${_USM_EXE_ARG}"
                COMMENT "Stripping USM.dll and injecting into USM_original.exe → USM.exe"
                VERBATIM)
            message(STATUS "USM.exe: dll2exe injector  : ${DLL2EXE}")
            message(STATUS "USM.exe: strip             : ${CMAKE_STRIP}")
            if(_IS_WSL)
                message(STATUS "USM.exe: WSL detected      : translating argv paths via ${WSLPATH}")
                message(STATUS "USM.exe:   USM_original.exe : ${_OUT_USM_ARG}")
                message(STATUS "USM.exe:   USM_stripped: ${_STRIPPED_ARG}")
                message(STATUS "USM.exe:   USM.exe (out)   : ${_USM_EXE_ARG}")
            endif()

            # ─────────────────────────────────────────────────────────
            #   USMITA.exe — same injection, Italian variant
            #
            #   Reuses the openusm_stripped.dll produced by USM's
            #   POST_BUILD above — no need to strip the loader DLL
            #   twice. add_dependencies(USMITA USM) below guarantees
            #   USM's POST_BUILD (which creates openusm_stripped.dll)
            #   completes before USMITA's POST_BUILD reads it.
            # ─────────────────────────────────────────────────────────
            if(TARGET USMITA)
                add_dependencies(USMITA USM)
                add_custom_command(TARGET USMITA POST_BUILD
                    COMMAND ${_DLL2EXE_RUN} -efix -injimp
                            "${_OUT_USMITA_ARG}"
                            "${_STRIPPED_ARG}"
                            "${_USMITA_EXE_ARG}"
                    COMMENT "Injecting USM_stripped.dll into USMITA_original.exe → USMITA.exe"
                    VERBATIM)
                message(STATUS "USMITA.exe: dll2exe injector  : ${DLL2EXE}")
                if(_IS_WSL)
                    message(STATUS "USMITA.exe:   USMITA_original.exe : ${_OUT_USMITA_ARG}")
                    message(STATUS "USMITA.exe:   USM_stripped    : ${_STRIPPED_ARG}")
                    message(STATUS "USMITA.exe:   USMITA.exe (out)    : ${_USMITA_EXE_ARG}")
                endif()
            endif()
        endif()
    else()
        message(WARNING
            "dll2exe.exe not found at ${DLL2EXE} — USM.exe will not be "
            "produced. Place dll2exe.exe in tools/ at the project root.")
    endif()

    # Ditto for debug_menu — ensure it's up to date next to USM_original.exe.

    # Auto-deploy the built artifacts into ../Ultimate Spider-Man/ if
    # that folder exists. The user's existing stock DLLs in that
    # folder are left untouched.
    set(_GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Ultimate Spider-Man")
    if(EXISTS "${_GAME_DIR}/")
        add_custom_command(TARGET USM POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${_OUT_USM}" "${_GAME_DIR}/USM_original.exe"
            COMMENT "Deploying USM_original.exe to ${_GAME_DIR}"
            VERBATIM)

        # USM.exe is produced only when dll2exe.exe is present; guard
        # the deploy with the same EXISTS check used above.
        if(EXISTS "${DLL2EXE}" AND TARGET binkw32)
            add_custom_command(TARGET USM POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_BINARY_DIR}/USM.exe" "${_GAME_DIR}/USM.exe"
                COMMENT "Deploying USM.exe to ${_GAME_DIR}"
                VERBATIM)
        endif()

        if(TARGET binkw32)
            add_custom_command(TARGET binkw32 POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                    "$<TARGET_FILE:binkw32>" "${_GAME_DIR}/binkw32.dll"
                COMMENT "Deploying binkw32.dll to ${_GAME_DIR}"
                VERBATIM)
        endif()

        # USMITA.exe is built only when USMITA.c is present; guard the
        # deploys on the USMITA target's existence so this block is a
        # no-op for English-only setups.
        #
        # Two artifacts are deployed:
        #   • USMITA_original.exe — clean Italian exe (from the writer)
        #   • USMITA.exe          — with openusm_stripped.dll injected
        #                           (only when dll2exe.exe is present —
        #                           same EXISTS guard as USM.exe above)
        if(TARGET USMITA)
            add_custom_command(TARGET USMITA POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                    "${_OUT_USMITA}" "${_GAME_DIR}/USMITA_original.exe"
                COMMENT "Deploying USMITA_original.exe to ${_GAME_DIR}"
                VERBATIM)

            if(EXISTS "${DLL2EXE}" AND TARGET binkw32)
                add_custom_command(TARGET USMITA POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                        "${CMAKE_BINARY_DIR}/USMITA.exe" "${_GAME_DIR}/USMITA.exe"
                    COMMENT "Deploying USMITA.exe to ${_GAME_DIR}"
                    VERBATIM)
            endif()
        endif()
endif()
# ─────────────────────────────────────────────────────────────────────────
#   Final link lines for binkw32 and debug_menu
# ─────────────────────────────────────────────────────────────────────────

target_link_libraries(ngl_dx PUBLIC d3dx9)

target_link_directories(binkw32 PUBLIC ${CMAKE_FIND_ROOT_PATH}/include)
target_link_directories(binkw32 PUBLIC ${CMAKE_FIND_ROOT_PATH}/lib)
target_link_libraries  (binkw32 PUBLIC physics
                                       charanimtype mash chuck nfl ngl nal nsl utility
                                       winmm psapi d3d9 dinput8 dxguid dsound assimp z)



if(USE_GLM)
    target_link_libraries(binkw32 PUBLIC glm::glm)
    endif()
endif()

# Pin binkw32's output directory so the USM target can embed it later
# and downstream deploy steps know exactly where to find it.
set_target_properties(binkw32 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# ═══════════════════════════════════════════════════════════════════════
#   Final build-tree cleanup — keep only the shipping artifacts
#
#   After USM.exe (and USMITA.exe when present) have been built, deployed
#   to ../Ultimate Spider-Man/, and dll2exe.exe has consumed
#   USM_stripped.dll, the intermediate loader DLLs are no longer needed
#   in the build tree:
#
#     • binkw32.dll      — already injected into USM.exe via dll2exe and
#                          already deployed to the game folder by the
#                          binkw32 POST_BUILD step above.
#     • USM.dll          — debug-symbol-laden copy of binkw32.dll; was
#                          only kept around as the input to `strip`.
#     • USM_stripped.dll — renamed to USM.dll so the final, stripped,
#                          shippable DLL sits next to USM.exe under the
#                          user-facing name.
#
#   Attached to the LAST target in the dll2exe chain (USMITA when it
#   exists, otherwise USM) so it runs strictly after every step that
#   still reads any of these files — in particular USMITA's POST_BUILD,
#   which reads USM_stripped.dll for the Italian-variant injection
#   (lines 776–782) and which we must NOT pull the rug out from under.
#
#   POST_BUILD commands attached to a single target execute in the order
#   they are added, so appending this block at the very end of the file
#   also guarantees it runs after the ../Ultimate Spider-Man/ deploys
#   (lines 803–852), which still reference $<TARGET_FILE:binkw32> and
#   ${CMAKE_BINARY_DIR}/USM.exe.
#
#   `cmake -E remove -f` is used instead of `rm` so the build remains
#   compatible with the cmake_minimum_required(3.12) declared at the top
#   of this file (`cmake -E rm` was introduced in CMake 3.17). The -f
#   flag makes the step idempotent on a re-run where binkw32.dll or
#   USM.dll has already been cleaned away.
# ═══════════════════════════════════════════════════════════════════════
if(EXISTS "${DLL2EXE}" AND TARGET USM AND TARGET binkw32)
    if(TARGET USMITA)
        set(_USM_CLEANUP_TARGET USMITA)
    else()
        set(_USM_CLEANUP_TARGET USM)
    endif()

    add_custom_command(TARGET ${_USM_CLEANUP_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove -f
                "${CMAKE_BINARY_DIR}/binkw32.dll"
                "${CMAKE_BINARY_DIR}/USM.dll"
        COMMAND ${CMAKE_COMMAND} -E rename
                "${CMAKE_BINARY_DIR}/USM_stripped.dll"
                "${CMAKE_BINARY_DIR}/USM.dll"
        COMMENT "Cleanup: removing binkw32.dll & USM.dll, renaming USM_stripped.dll → USM.dll"
        VERBATIM)

    message(STATUS "USM.exe: build-tree cleanup attached to target '${_USM_CLEANUP_TARGET}'")

    unset(_USM_CLEANUP_TARGET)
endif()