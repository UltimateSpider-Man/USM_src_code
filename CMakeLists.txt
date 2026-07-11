# ═══════════════════════════════════════════════════════════════════════
#   OpenUSM — unified CMakeLists.txt
#
#   Produces, in a single `cmake --build build` invocation:
#
#     • build/USM_original.exe   — compiled from USM.c's rawData[5926912]
#                                  byte array (5,926,912 bytes = 5.65 MiB,
#                                  byte-identical to the retail USM.exe).
#     • build/binkw32.dll        — the OpenUSM mod-loader (release)
#     • build/binkw32_prelib.dll — the OpenUSM mod-loader (prerelease;
#                                  built from main_beta.cpp). Shipped under
#                                  the name USM_prelib.dll.
#     • build/USM.dll            — stripped, shippable copy of binkw32.dll
#     • build/USM.exe            — USM_original.exe with a stripped
#                                  binkw32-derived DLL injected via
#                                  tools/dll2exe.exe.
#     • build/USMITA_original.exe / USMITA.exe — Italian retail variant,
#                                  compiled from USMITA.c (optional).
#     • build/Ultimate_prerelease_original.exe — PRERELEASE variant,
#                                  compiled from Ultimate_prerelease.c's
#                                  rawData[5984512] byte array
#                                  (5,984,512 bytes = 5.71 MiB — note this
#                                  differs from the 5,926,912-byte retail
#                                  exes). Built only if the source is present.
#     • build/USM_prelib.dll     — binkw32_prelib.dll under its shipping name.
#     • build/Ultimate_prerelease.exe — Ultimate_prerelease_original.exe with
#                                  a stripped binkw32_prelib DLL
#                                  (USM_prelib_stripped.dll) injected via
#                                  dll2exe.exe (mirrors the USM.exe step).
#
#   No file literally named openusm.dll / openusm_stripped.dll is produced.
#   The only DLLs stripped by this build are binkw32.dll (for USM.exe) and
#   binkw32_prelib.dll (for Ultimate_prerelease.exe).
#
#   Auto-deploys every output to ../Ultimate Spider-Man/ if that directory
#   exists next to the source tree. Stock DLLs (binkw32, d3dx9_24, dsound,
#   MSVCP71, msvcr71, xerces-c_2_3_0) in that folder are left untouched.
#
#   Required layout:
#
#       OpenUSM/
#       ├── CMakeLists.txt                 ← this file
#       ├── main_final.cpp                 ← binkw32 entry point
#       ├── main_beta.cpp                  ← binkw32_prelib entry point
#       ├── USM.c                          ← rawData[5926912] blob (REQUIRED)
#       ├── USMITA.c                       ← rawData[5926912] blob — ITA variant (OPTIONAL)
#       ├── Ultimate_prerelease.c          ← rawData[5984512] blob — PRERELEASE variant (OPTIONAL)
#       ├── emit_usm_exe.c                 ← host-tool writer       (REQUIRED)
#       ├── src/                           ← parent-project libraries
#       ├── cmake/
#       │   ├── i686-pc-mingw32.cmake      ← MinGW cross toolchain
#       │   └── report_size.cmake          ← size reporter for UPX step
#       ├── tools/
#       │   └── dll2exe.exe                ← DLL→EXE injector
#       ├── third_party/{glm,googletest}/  ← (optional)
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
#     -DUSE_GLM=ON                   link glm::glm into binkw32/binkw32_prelib
#     -DUSE_FSTREAM=ON               -DUSE_FSTREAM in binkw32/binkw32_prelib
#     -DCHECK_STRUCT_SIZES=OFF       disable CHECK_STRUCT_SIZES define
#     -DTARGET_XBOX=ON               build the Xbox variant of mash
#     -DENABLE_TESTS=OFF             skip googletest/CTest
#     -DUSM_C_SOURCE=<path>          override USM.c location
#     -DUSM_C_WRITER_SOURCE=<path>   override emit_usm_exe.c location
#     -DUSMITA_C_SOURCE=<path>       override USMITA.c location (ITA variant)
#     -DUltimate_prerelease_C_SOURCE=<path>  override Ultimate_prerelease.c location
# ═══════════════════════════════════════════════════════════════════════

cmake_minimum_required(VERSION 3.12)

include(ExternalProject)
include(cmake/i686-pc-mingw32.cmake)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS}
    "-static -std=gnu11 -Os -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}
    "-fomit-frame-pointer -m32 -fno-asynchronous-unwind-tables -masm=intel -static -Os -ffunction-sections -fdata-sections")

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
    # binkw32.dll / binkw32_prelib.dll — not libbinkw32.dll etc.
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
#   binkw32.dll (release) + binkw32_prelib.dll (prerelease) — OpenUSM loaders
#
#   Both share the same ${SRC} glob (the in-process mod-loader code). They
#   differ only in entry point: main_final.cpp vs. main_beta.cpp.
# ─────────────────────────────────────────────────────────────────────────

file(GLOB SRC "./src/*.cpp" "./src/*.h")

add_library(binkw32 SHARED main_final.cpp ${SRC})
target_compile_options(binkw32 PRIVATE ${flags})

add_library(binkw32_prelib SHARED main_beta.cpp ${SRC})
target_compile_options(binkw32_prelib PRIVATE ${flags})

add_library(binkw32_build SHARED main_build.cpp ${SRC})
target_compile_options(binkw32_build PRIVATE ${flags})


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
#   Shared settings for binkw32 & binkw32_prelib (same code, different entry)
# ─────────────────────────────────────────────────────────────────────────

if(USE_FSTREAM)
    target_compile_definitions(binkw32         PUBLIC USE_FSTREAM)
    target_compile_definitions(binkw32_prelib  PUBLIC USE_FSTREAM)
	    target_compile_definitions(binkw32_build  PUBLIC USE_FSTREAM)
endif()

target_include_directories(binkw32        PUBLIC "include" "./src/nfl/src/")
target_include_directories(binkw32_prelib PUBLIC "include" "./src/nfl/src/")
target_include_directories(binkw32_build PUBLIC "include" "./src/nfl/src/")

target_include_directories(chuck   PUBLIC "include" "src")
target_include_directories(tx      PUBLIC "include" "src")
target_include_directories(ngl_dx  PUBLIC "include" "src")
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
        -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=OFF
        -DASSIMP_BUILD_OBJ_IMPORTER=ON
        -DASSIMP_BUILD_FBX_IMPORTER=ON
        -DASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT=OFF
        -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
        -DASSIMP_BUILD_TESTS=OFF
        -DASSIMP_WARNINGS_AS_ERRORS=OFF
        -DASSIMP_NO_EXPORT=ON
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

target_include_directories(binkw32_prelib PUBLIC "./assimp/include/")
target_link_directories   (binkw32_prelib PUBLIC ${CMAKE_BINARY_DIR}/assimp-build/lib)
add_dependencies          (binkw32_prelib AssimpProject)

target_include_directories(binkw32_build PUBLIC "./assimp/include/")
target_link_directories   (binkw32_build PUBLIC ${CMAKE_BINARY_DIR}/assimp-build/lib)
add_dependencies          (binkw32_build AssimpProject)


# ─────────────────────────────────────────────────────────────────────────
#   Final link lines for binkw32 and binkw32_prelib
#
#   (Top level — NOT inside any if(TARGET USM). The loaders must link
#    regardless of whether USM.c / the exe blobs are present.)
# ─────────────────────────────────────────────────────────────────────────

target_link_libraries(ngl_dx PUBLIC d3dx9)

# --- binkw32 (release loader) ---
target_link_directories(binkw32 PUBLIC ${CMAKE_FIND_ROOT_PATH}/include)
target_link_directories(binkw32 PUBLIC ${CMAKE_FIND_ROOT_PATH}/lib)
target_link_libraries  (binkw32 PUBLIC physics
                                       charanimtype mash chuck nfl ngl nal nsl utility
                                       winmm psapi d3d9 dinput8 dxguid dsound assimp z)
if(USE_GLM)
    target_link_libraries(binkw32 PUBLIC glm::glm)
endif()

# --- binkw32_prelib (prerelease loader) — identical link set to binkw32 ---
target_link_directories(binkw32_prelib PUBLIC ${CMAKE_FIND_ROOT_PATH}/include)
target_link_directories(binkw32_prelib PUBLIC ${CMAKE_FIND_ROOT_PATH}/lib)
target_link_libraries  (binkw32_prelib PUBLIC physics
                                       charanimtype mash chuck nfl ngl nal nsl utility
                                       winmm psapi d3d9 dinput8 dxguid dsound assimp z)
									   
									   
target_link_directories(binkw32_build PUBLIC ${CMAKE_FIND_ROOT_PATH}/include)
target_link_directories(binkw32_build PUBLIC ${CMAKE_FIND_ROOT_PATH}/lib)
target_link_libraries  (binkw32_build PUBLIC physics
                                       charanimtype mash chuck nfl ngl nal nsl utility
                                       winmm psapi d3d9 dinput8 dxguid dsound assimp z)
if(USE_GLM)
    target_link_libraries(binkw32_prelib PUBLIC glm::glm)
	target_link_libraries(binkw32_build PUBLIC glm::glm)
endif()

# Pin both loaders' output directory so the inject/deploy steps below know
# exactly where to find them.
set_target_properties(binkw32 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

foreach(_USM_LOADER_TARGET binkw32 binkw32_prelib binkw32_build)
    target_compile_options(${_USM_LOADER_TARGET} PRIVATE
        -Os -ffunction-sections -fdata-sections)
    set_property(TARGET ${_USM_LOADER_TARGET} APPEND_STRING PROPERTY
        LINK_FLAGS " -Wl,--gc-sections -Wl,--strip-all")
endforeach()
unset(_USM_LOADER_TARGET)
set_target_properties(binkw32_prelib PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(binkw32_build PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")


# ═══════════════════════════════════════════════════════════════════════
#   USM_original.exe — compiled from USM.c   (retail English, 5,926,912 B)
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
    # NOTE: emit_usm_exe.c should size its write with sizeof(rawData),
    # so the same writer works for every blob regardless of length.
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

    add_custom_command(
        OUTPUT  "${_OUT_USM}"
        COMMAND "${CMAKE_BINARY_DIR}/emit_usm_exe_build/emit_usm_exe"
                "${_OUT_USM}"
        DEPENDS emit_usm_exe_host "${USM_C_SOURCE}" "${USM_C_WRITER_SOURCE}"
        COMMENT "Compiling USM_original.exe from USM.c rawData[] (5,926,912 bytes / 5.65 MiB)"
        VERBATIM
    )

    add_custom_target(USM ALL DEPENDS "${_OUT_USM}")

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
#   USMITA_original.exe — Italian retail variant, compiled from USMITA.c
#
#   Same rawData[5926912] symbol as USM.c, so it's built as a parallel
#   host writer (emit_usmita_exe). Skipped silently if USMITA.c absent.
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
    # Missing-writer warning already emitted by the USM block. Silent here.
else()
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



# ═══════════════════════════════════════════════════════════════════════
#   Ultimate_Build_original.exe — Italian retail variant, compiled from ultimate_build.c
#
#   Same rawData[5926912] symbol as USM.c, so it's built as a parallel
#   host writer (emit_ultimate_build_exe). Skipped silently if ultimate_build.c absent.
# ═══════════════════════════════════════════════════════════════════════

set(ultimate_build_C_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/ultimate_build.c"
    CACHE FILEPATH "Path to ultimate_build.c with rawData[5926912] byte array (Italian variant)")

set(_OUT_ultimate_build "${CMAKE_BINARY_DIR}/Ultimate_Build_original.exe")

if(NOT EXISTS "${ultimate_build_C_SOURCE}")
    message(STATUS
        "ultimate_build.c not found at ${ultimate_build_C_SOURCE} — ultimate_build target "
        "skipped. (Optional Italian variant; set -Dultimate_build_C_SOURCE=<path> "
        "to enable.)")
elseif(NOT EXISTS "${USM_C_WRITER_SOURCE}")
    # Missing-writer warning already emitted by the USM block. Silent here.
else()
    ExternalProject_Add(emit_ultimate_build_exe_host
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/emit_ultimate_build_exe_src"
        BINARY_DIR        "${CMAKE_BINARY_DIR}/emit_ultimate_build_exe_build"
        DOWNLOAD_COMMAND  ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND
            ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_BINARY_DIR}/emit_ultimate_build_exe_build"
            COMMAND
            cc -O0
               "${ultimate_build_C_SOURCE}"
               "${USM_C_WRITER_SOURCE}"
               -o "${CMAKE_BINARY_DIR}/emit_ultimate_build_exe_build/emit_ultimate_build_exe"
        INSTALL_COMMAND   ""
        BUILD_BYPRODUCTS  "${CMAKE_BINARY_DIR}/emit_ultimate_build_exe_build/emit_ultimate_build_exe"
        BUILD_ALWAYS      FALSE
    )

    add_custom_command(
        OUTPUT  "${_OUT_ultimate_build}"
        COMMAND "${CMAKE_BINARY_DIR}/emit_ultimate_build_exe_build/emit_ultimate_build_exe"
                "${_OUT_ultimate_build}"
        DEPENDS emit_ultimate_build_exe_host "${ultimate_build_C_SOURCE}" "${USM_C_WRITER_SOURCE}"
        COMMENT "Compiling Ultimate_Build_original.exe from ultimate_build.c rawData[] (5,926,912 bytes / 5.65 MiB)"
        VERBATIM
    )

    add_custom_target(ultimate_build ALL DEPENDS "${_OUT_ultimate_build}")

    file(SIZE   "${ultimate_build_C_SOURCE}" _ultimate_build_C_SIZE)
    file(SHA256 "${ultimate_build_C_SOURCE}" _ultimate_build_C_SHA)
    message(STATUS "Ultimate_Build_original.exe: compiling from ultimate_build.c rawData[]")
    message(STATUS "  ultimate_build.c       : ${ultimate_build_C_SOURCE}")
    message(STATUS "  ultimate_build.c size  : ${_ultimate_build_C_SIZE} bytes")
    message(STATUS "  ultimate_build.c sha256: ${_ultimate_build_C_SHA}")
    message(STATUS "  writer         : ${USM_C_WRITER_SOURCE}")
    message(STATUS "  output         : ${_OUT_ultimate_build} (expected: 5926912 bytes / 5.65 MiB)")
endif()


# ═══════════════════════════════════════════════════════════════════════
#   Ultimate_prerelease_original.exe — PRERELEASE variant
#
#   Ultimate_prerelease.c is the same kind of ASCII-hex blob as USM.c, but
#   declares  unsigned char rawData[5984512]  — the bytes of the Ultimate
#   Spider-Man PRERELEASE build (5,984,512 bytes / 5.71 MiB).
#
#   ⚠ SIZE DIFFERS from retail: 5,984,512 (prerelease) vs 5,926,912
#     (USM/USMITA). emit_usm_exe.c MUST write sizeof(rawData) bytes (not a
#     hard-coded 5926912) or this exe will be truncated. If your writer
#     hard-codes the retail length, parameterise it before building this.
#
#   Built as its own host writer (emit_Ultimate_prerelease_exe) so its
#   rawData[] cannot collide with USM.c / USMITA.c at link time. Skipped
#   silently if Ultimate_prerelease.c is absent.
# ═══════════════════════════════════════════════════════════════════════

set(Ultimate_prerelease_C_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/Ultimate_prerelease.c"
    CACHE FILEPATH "Path to Ultimate_prerelease.c with rawData[5984512] byte array (prerelease variant)")

set(_OUT_Ultimate_prerelease "${CMAKE_BINARY_DIR}/Ultimate_prerelease_original.exe")

if(NOT EXISTS "${Ultimate_prerelease_C_SOURCE}")
    message(STATUS
        "Ultimate_prerelease.c not found at ${Ultimate_prerelease_C_SOURCE} — "
        "Ultimate_prerelease target skipped. (Optional prerelease variant; "
        "set -DUltimate_prerelease_C_SOURCE=<path> to enable.)")
elseif(NOT EXISTS "${USM_C_WRITER_SOURCE}")
    # Missing-writer warning already emitted by the USM block. Silent here.
else()
    # ── Host-native compile of Ultimate_prerelease.c + emit_usm_exe.c ──
    # Mirrors the USM_original.exe ExternalProject. The host program is
    # named emit_Ultimate_prerelease_exe so it cannot collide with the
    # English / Italian writers on disk or at link time.
    ExternalProject_Add(emit_Ultimate_prerelease_exe_host
        SOURCE_DIR        "${CMAKE_BINARY_DIR}/emit_Ultimate_prerelease_exe_src"
        BINARY_DIR        "${CMAKE_BINARY_DIR}/emit_Ultimate_prerelease_exe_build"
        DOWNLOAD_COMMAND  ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND
            ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_BINARY_DIR}/emit_Ultimate_prerelease_exe_build"
            COMMAND
            cc -O0
               "${Ultimate_prerelease_C_SOURCE}"
               "${USM_C_WRITER_SOURCE}"
               -o "${CMAKE_BINARY_DIR}/emit_Ultimate_prerelease_exe_build/emit_Ultimate_prerelease_exe"
        INSTALL_COMMAND   ""
        BUILD_BYPRODUCTS  "${CMAKE_BINARY_DIR}/emit_Ultimate_prerelease_exe_build/emit_Ultimate_prerelease_exe"
        BUILD_ALWAYS      FALSE
    )

    add_custom_command(
        OUTPUT  "${_OUT_Ultimate_prerelease}"
        COMMAND "${CMAKE_BINARY_DIR}/emit_Ultimate_prerelease_exe_build/emit_Ultimate_prerelease_exe"
                "${_OUT_Ultimate_prerelease}"
        DEPENDS emit_Ultimate_prerelease_exe_host
                "${Ultimate_prerelease_C_SOURCE}" "${USM_C_WRITER_SOURCE}"
        COMMENT "Compiling Ultimate_prerelease_original.exe from Ultimate_prerelease.c rawData[] (5,984,512 bytes / 5.71 MiB)"
        VERBATIM
    )

    add_custom_target(Ultimate_prerelease ALL DEPENDS "${_OUT_Ultimate_prerelease}")

    file(SIZE   "${Ultimate_prerelease_C_SOURCE}" _PRE_C_SIZE)
    file(SHA256 "${Ultimate_prerelease_C_SOURCE}" _PRE_C_SHA)
    message(STATUS "Ultimate_prerelease_original.exe: compiling from Ultimate_prerelease.c rawData[]")
    message(STATUS "  Ultimate_prerelease.c       : ${Ultimate_prerelease_C_SOURCE}")
    message(STATUS "  Ultimate_prerelease.c size  : ${_PRE_C_SIZE} bytes")
    message(STATUS "  Ultimate_prerelease.c sha256: ${_PRE_C_SHA}")
    message(STATUS "  writer         : ${USM_C_WRITER_SOURCE}")
    message(STATUS "  output         : ${_OUT_Ultimate_prerelease} (expected: 5984512 bytes / 5.71 MiB)")
endif()


# ─────────────────────────────────────────────────────────────────────────
#   Shared dll2exe / strip / WSL setup
#
#   Used by BOTH the USM/USMITA inject pipeline and the
#   Ultimate_prerelease inject pipeline below, so it lives at top level
#   (not inside any if(TARGET ...)).
# ─────────────────────────────────────────────────────────────────────────

set(DLL2EXE "${CMAKE_CURRENT_SOURCE_DIR}/tools/dll2exe.exe")

# Locate strip from the active toolchain (MinGW i686 cross-compile, or
# native MSYS2 MinGW). The toolchain file usually sets CMAKE_STRIP.
if(NOT CMAKE_STRIP)
    find_program(CMAKE_STRIP
        NAMES i686-pc-mingw32-strip
              i686-w64-mingw32-strip
              strip)
endif()

# ─────────────────────────────────────────────────────────────────────────
#   Should the loader DLL keep its debug symbols when injected into the exe?
#
#   The final *.exe is NEVER `strip`ped directly. What the inject pipelines
#   below strip is the loader DLL copy (USM_stripped.dll / *_prelib_stripped /
#   *_build_stripped) that dll2exe folds into the retail blob. Turning this
#   OFF injects the UNstripped loader, so its symbols survive inside the exe.
#
#   Tradeoff: symbols are real bytes. With Assimp statically linked, an
#   unstripped loader is much larger, so the injected exe grows and — even
#   after UPX --lzma — the packed *.exe will NOT be byte-identical to (and may
#   exceed the budget of) the stripped build. The size REPORTED by the pack
#   step is the source of truth. Note that stripping only ever changed the
#   on-disk size; the loaded image size (PE SizeOfImage) is identical either
#   way, since symbol/debug sections are not mapped at runtime.
#
#   Default ON preserves today's shipping-exe size exactly. Build with
#   -DUSM_STRIP_INJECTED_DLL=OFF to retain symbols inside the exe.
# ─────────────────────────────────────────────────────────────────────────
option(USM_STRIP_INJECTED_DLL
    "strip --strip-all the loader DLL before dll2exe injects it into the exe"
    ON)

# Copy-then-maybe-strip helper, generated next to the build outputs (mirrors
# usm_pack.cmake). Centralises the strip decision for every inject pipeline
# and stays 3.12-compatible (no file(COPY_FILE)). Never hard-fails on a strip
# hiccup — it downgrades to a warning so dll2exe still runs.
set(_USM_MAKE_LOADER_SCRIPT "${CMAKE_BINARY_DIR}/usm_make_loader.cmake")
file(WRITE "${_USM_MAKE_LOADER_SCRIPT}" [=[
# usm_make_loader.cmake (generated by CMakeLists.txt)
#   cmake -DSRC=<built.dll> -DOUT=<loader.dll> \
#         -DSTRIP=<strip-or-empty> -DDOSTRIP=<ON/OFF> \
#         -P usm_make_loader.cmake
# Copy SRC -> OUT, then strip OUT in place ONLY when DOSTRIP is true.
if(NOT DEFINED SRC OR NOT EXISTS "${SRC}")
    message(FATAL_ERROR "usm_make_loader: SRC not found: ${SRC}")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy "${SRC}" "${OUT}"
    RESULT_VARIABLE _cp)
if(NOT _cp EQUAL 0)
    message(FATAL_ERROR "usm_make_loader: copy failed: ${SRC} -> ${OUT}")
endif()
if(DOSTRIP AND STRIP)
    execute_process(
        COMMAND "${STRIP}" --strip-all "${OUT}"
        RESULT_VARIABLE _st)
    if(NOT _st EQUAL 0)
        message(WARNING
            "usm_make_loader: strip exit ${_st} on ${OUT}; left unstripped.")
    endif()
else()
    message(STATUS
        "usm_make_loader: ${OUT} left UNstripped (debug symbols retained).")
endif()
]=])

# WSL detection — on WSL the host is "Linux" but dll2exe.exe runs as a
# native Windows process via interop, so Linux-style argv paths must be
# translated to a Windows-resolvable form.
set(_IS_WSL FALSE)
if(NOT CMAKE_HOST_WIN32 AND EXISTS "/proc/sys/kernel/osrelease")
    file(READ "/proc/sys/kernel/osrelease" _OSRELEASE)
    if(_OSRELEASE MATCHES "[Mm]icrosoft|WSL")
        set(_IS_WSL TRUE)
        find_program(WSLPATH wslpath)
    endif()
endif()

# Translate a CMake (Linux) path into the form dll2exe.exe expects in argv.
#   • /mnt/<drive>/...   →   <DRIVE>:/...        (pure string rewrite)
#   • /home/..., /tmp/.. →   \\wsl$\<distro>\... (via wslpath)
# The /mnt/ branch deliberately avoids wslpath (it returns empty for paths
# that don't yet exist on the Windows side, e.g. the dll2exe output at
# configure time).
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

# dll2exe.exe is a Windows PE binary. Pick a launcher:
#   • Native Windows / WSL: run directly.
#   • Native Linux/macOS: drive it through wine (wine maps Linux paths
#     via Z:/ so no translation is needed there).
if(CMAKE_HOST_WIN32 OR _IS_WSL)
    set(_DLL2EXE_RUN "${DLL2EXE}")
else()
    find_program(WINE_EXECUTABLE wine)
    if(WINE_EXECUTABLE)
        set(_DLL2EXE_RUN "${WINE_EXECUTABLE}" "${DLL2EXE}")
    else()
        set(_DLL2EXE_RUN "${DLL2EXE}")
    endif()
endif()


# ─────────────────────────────────────────────────────────────────────────
#   UPX packing of the FINAL injected exes
#   (USM.exe / Ultimate_prerelease.exe / Ultimate_Build.exe → ~8 MiB)
#
#   Each of those exes is the ~5.7 MiB retail/prerelease blob PLUS the
#   statically-linked OpenUSM loader that dll2exe injects, so on disk they
#   land well above 8 MiB. `strip` cannot help here — the blob is already a
#   final retail PE, and the injected loader DLL was --strip-all'd before
#   injection (unless -DUSM_STRIP_INJECTED_DLL=OFF), so the remaining weight
#   is genuine code+data. UPX --lzma packs
#   the whole PE in place; the UPX stub self-extracts at launch and transfers
#   to the dll2exe-fixed original entry point (-efix).
#
#   ORDER MATTERS: packing must run AFTER dll2exe has produced the exe, so
#   each usm_pack_exe() call below is attached to the SAME target as — and
#   added strictly after — that target's dll2exe POST_BUILD step. Because it
#   is also added before the ../Ultimate Spider-Man/ deploy steps, the copy
#   that lands in the game folder is the packed one.
#
#   Tunables:
#     -DUSM_PACK_EXES=ON             re-enable UPX compression (default OFF)
#     -DUSM_EXE_SIZE_BUDGET=<bytes>  size ceiling for the report
#                                    (default 8388608 = 8 MiB)
#
#   NOTE: default is now OFF — the final exes are injected but NOT compressed,
#   so on disk they keep their full ~injected size (retail blob + loader,
#   which lands well above the 8 MiB budget). UPX is the ONLY thing that
#   shrinks these on disk; with it off the size is still reported and the
#   budget check still warns. Set -DUSM_PACK_EXES=ON to compress again.
#   (Changing this default only affects FRESH build dirs; an already-cached
#   USM_PACK_EXES keeps its old value until you pass -DUSM_PACK_EXES=... or
#   delete the build dir.)
# ─────────────────────────────────────────────────────────────────────────

option(USM_PACK_EXES "UPX-compress the final injected exes" OFF)
set(USM_EXE_SIZE_BUDGET "8388608" CACHE STRING
    "Size ceiling in bytes for the packed exes (default 8 MiB = 8388608).")
option(USM_PACK_DLLS "UPX-compress the produced loader DLLs" ON)
set(USM_DLL_SIZE_BUDGET "2097152" CACHE STRING
    "Size ceiling in bytes for packed loader DLLs (default 2 MiB = 2097152).")

find_program(UPX_EXECUTABLE NAMES upx upx-ucl)
if(UPX_EXECUTABLE)
    message(STATUS "Final-exe packing: upx = ${UPX_EXECUTABLE} "
                   "(budget ${USM_EXE_SIZE_BUDGET} bytes)")
elseif(USM_PACK_EXES)
    message(WARNING
        "USM_PACK_EXES is ON but upx was not found — the final exes will be "
        "size-reported but NOT compressed. Install UPX "
        "(https://upx.github.io) or configure with -DUSM_PACK_EXES=OFF.")
endif()

# In-place pack-and-report helper, emitted next to the build outputs so the
# whole step is self-contained, cross-platform, and never hard-fails the
# build (any UPX trouble downgrades to a warning).
set(_USM_PACK_SCRIPT "${CMAKE_BINARY_DIR}/usm_pack.cmake")
file(WRITE "${_USM_PACK_SCRIPT}" [=[
# usm_pack.cmake (generated by CMakeLists.txt)
#   cmake -DFILE=<exe> -DUPX=<upx-or-empty> -DDOPACK=<ON/OFF> \
#         -DBUDGET=<bytes> -P usm_pack.cmake
# Always reports FILE's size against BUDGET. Compresses FILE in place with
# UPX only when DOPACK is ON and a usable UPX is supplied. Any UPX trouble
# downgrades to a warning so deploy/cleanup still run.
if(NOT DEFINED FILE OR NOT EXISTS "${FILE}")
    message(WARNING "usm_pack: target file not found, skipping: ${FILE}")
    return()
endif()
get_filename_component(_name "${FILE}" NAME)
file(SIZE "${FILE}" _before)
if(DOPACK AND UPX AND EXISTS "${UPX}")
    execute_process(
        COMMAND "${UPX}" --best --lzma --force -q "${FILE}"
        RESULT_VARIABLE _rc
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE  _err)
    if(NOT _rc EQUAL 0)
        message(WARNING
            "usm_pack: upx exit ${_rc} on ${_name}; left uncompressed. "
            "${_out}${_err}")
    endif()
elseif(DOPACK)
    message(WARNING "usm_pack: no usable UPX; ${_name} left uncompressed.")
else()
    message(STATUS "usm_pack: UPX disabled; ${_name} left uncompressed.")
endif()
file(SIZE "${FILE}" _after)
math(EXPR _mib "${_after} / 1048576")
math(EXPR _kib "(${_after} % 1048576) / 1024")
if(DEFINED BUDGET AND _after GREATER BUDGET)
    math(EXPR _bmib "${BUDGET} / 1048576")
    message(WARNING
        "${_name}: ${_before} -> ${_after} bytes (~${_mib} MiB ${_kib} KiB) "
        "OVER budget ${BUDGET} bytes (~${_bmib} MiB).")
else()
    message(STATUS
        "${_name}: ${_before} -> ${_after} bytes "
        "(~${_mib} MiB ${_kib} KiB) within budget.")
endif()
]=])

# usm_pack_exe(<target> <exe-path-on-disk>)
#   Append a POST_BUILD step to <target> that reports <exe-path>'s size vs
#   USM_EXE_SIZE_BUDGET and, when USM_PACK_EXES is ON, UPX-packs it in place.
#   The size is reported either way, so the no-UPX size is always visible.
#   MUST be called after the target's dll2exe injection command so it runs
#   later in the POST_BUILD chain (and before the deploy steps below).
function(usm_pack_exe tgt exe_path)
    if(USM_PACK_EXES AND UPX_EXECUTABLE)
        set(_upx_arg "${UPX_EXECUTABLE}")
    else()
        set(_upx_arg "")
    endif()
    if(USM_PACK_EXES)
        set(_dopack ON)
    else()
        set(_dopack OFF)
    endif()
    add_custom_command(TARGET ${tgt} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                "-DFILE=${exe_path}"
                "-DUPX=${_upx_arg}"
                "-DDOPACK=${_dopack}"
                "-DBUDGET=${USM_EXE_SIZE_BUDGET}"
                -P "${_USM_PACK_SCRIPT}"
        COMMENT "Checking ${exe_path} size (UPX pack=${_dopack}, budget ${USM_EXE_SIZE_BUDGET} bytes)"
        VERBATIM)
endfunction()

function(usm_pack_dll tgt dll_path)
    if(USM_PACK_DLLS AND UPX_EXECUTABLE)
        set(_upx_arg "${UPX_EXECUTABLE}")
    else()
        set(_upx_arg "")
    endif()
    if(USM_PACK_DLLS)
        set(_dopack ON)
    else()
        set(_dopack OFF)
    endif()
    add_custom_command(TARGET ${tgt} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                "-DFILE=${dll_path}"
                "-DUPX=${_upx_arg}"
                "-DDOPACK=${_dopack}"
                "-DBUDGET=${USM_DLL_SIZE_BUDGET}"
                -P "${_USM_PACK_SCRIPT}"
        COMMENT "Checking ${dll_path} size (UPX pack=${_dopack}, budget ${USM_DLL_SIZE_BUDGET} bytes)"
        VERBATIM)
endfunction()


# ─────────────────────────────────────────────────────────────────────────
#   USM.exe / USMITA.exe — inject the release loader (binkw32)
#
#     1. binkw32.dll → USM.dll → USM_stripped.dll, then strip USM_stripped.dll
#     2. dll2exe -efix -injimp USM_original.exe USM_stripped.dll USM.exe
#        (USMITA.exe reuses the same stripped DLL)
#
#   -efix    restore the original PE entry point after the DLL loads
#   -injimp  rewrite the import table to bind Bink* imports to the loader
# ─────────────────────────────────────────────────────────────────────────

if(TARGET USM)
    if(TARGET binkw32)
        add_dependencies(USM binkw32)
        add_custom_command(TARGET USM POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:binkw32>"
                "${CMAKE_BINARY_DIR}/USM.dll"
            COMMENT "Deploying built mod loader → USM.dll"
            VERBATIM)
    endif()

    _to_dll2exe_path("${_OUT_USM}"                          _OUT_USM_ARG)
    _to_dll2exe_path("${CMAKE_BINARY_DIR}/USM_stripped.dll" _STRIPPED_ARG)
    _to_dll2exe_path("${CMAKE_BINARY_DIR}/USM.exe"          _USM_EXE_ARG)
    _to_dll2exe_path("${_OUT_USMITA}"                       _OUT_USMITA_ARG)
    _to_dll2exe_path("${CMAKE_BINARY_DIR}/USMITA.exe"       _USMITA_EXE_ARG)

    if(EXISTS "${DLL2EXE}")
        if(TARGET binkw32)
            add_custom_command(TARGET USM POST_BUILD
                COMMAND ${CMAKE_COMMAND}
                        "-DSRC=${CMAKE_BINARY_DIR}/USM.dll"
                        "-DOUT=${CMAKE_BINARY_DIR}/USM_stripped.dll"
                        "-DSTRIP=${CMAKE_STRIP}"
                        "-DDOSTRIP=${USM_STRIP_INJECTED_DLL}"
                        -P "${_USM_MAKE_LOADER_SCRIPT}"
                COMMAND ${_DLL2EXE_RUN} -efix -injimp
                        "${_OUT_USM_ARG}"
                        "${_STRIPPED_ARG}"
                        "${_USM_EXE_ARG}"
                COMMENT "Preparing USM loader (strip=${USM_STRIP_INJECTED_DLL}) and injecting into USM_original.exe → USM.exe"
                VERBATIM)
            usm_pack_exe(USM "${CMAKE_BINARY_DIR}/USM.exe")
            message(STATUS "USM.exe: dll2exe injector  : ${DLL2EXE}")
            message(STATUS "USM.exe: inject strip      : ${USM_STRIP_INJECTED_DLL} (${CMAKE_STRIP})")
            if(_IS_WSL)
                message(STATUS "USM.exe: WSL detected      : translating argv paths via ${WSLPATH}")
            endif()

            # USMITA.exe — same injection, reusing USM_stripped.dll.
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
            endif()
        endif()
    else()
        message(WARNING
            "dll2exe.exe not found at ${DLL2EXE} — USM.exe will not be "
            "produced. Place dll2exe.exe in tools/ at the project root.")
    endif()
endif()


# ─────────────────────────────────────────────────────────────────────────
#   Ultimate_prerelease.exe — inject the PRERELEASE loader (binkw32_prelib)
#
#   Mirrors the USM.exe pipeline, but for the prerelease and using its own
#   binkw32_prelib-derived DLLs (no openusm.dll / openusm_stripped.dll):
#
#     1. binkw32_prelib.dll → USM_prelib.dll          (deployable loader —
#                                                       the requested rename)
#     2. binkw32_prelib.dll → USM_prelib_stripped.dll, then
#        strip --strip-all USM_prelib_stripped.dll     (the dll2exe input)
#     3. dll2exe -efix -injimp
#            Ultimate_prerelease_original.exe
#            USM_prelib_stripped.dll
#            Ultimate_prerelease.exe
# ─────────────────────────────────────────────────────────────────────────

if(TARGET Ultimate_prerelease AND TARGET binkw32_prelib)
    add_dependencies(Ultimate_prerelease binkw32_prelib)

    # (1) deployable loader: binkw32_prelib.dll → USM_prelib.dll
    add_custom_command(TARGET binkw32_prelib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:binkw32_prelib>"
            "${CMAKE_BINARY_DIR}/USM_prelib.dll"
        COMMENT "Renaming prerelease loader → USM_prelib.dll"
        VERBATIM)

    usm_pack_dll(binkw32_prelib "${CMAKE_BINARY_DIR}/USM_prelib.dll")

    if(EXISTS "${DLL2EXE}")
        _to_dll2exe_path("${_OUT_Ultimate_prerelease}"                 _OUT_PRE_ARG)
        _to_dll2exe_path("${CMAKE_BINARY_DIR}/USM_prelib_stripped.dll" _PRE_STRIPPED_ARG)
        _to_dll2exe_path("${CMAKE_BINARY_DIR}/Ultimate_prerelease.exe" _PRE_EXE_ARG)

        # (2)+(3) strip a copy of binkw32_prelib.dll, then inject it.
        add_custom_command(TARGET Ultimate_prerelease POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                    "-DSRC=$<TARGET_FILE:binkw32_prelib>"
                    "-DOUT=${CMAKE_BINARY_DIR}/USM_prelib_stripped.dll"
                    "-DSTRIP=${CMAKE_STRIP}"
                    "-DDOSTRIP=${USM_STRIP_INJECTED_DLL}"
                    -P "${_USM_MAKE_LOADER_SCRIPT}"
            COMMAND ${_DLL2EXE_RUN} -efix -injimp
                    "${_OUT_PRE_ARG}"
                    "${_PRE_STRIPPED_ARG}"
                    "${_PRE_EXE_ARG}"
            COMMENT "Preparing prerelease loader (strip=${USM_STRIP_INJECTED_DLL}) and injecting into Ultimate_prerelease_original.exe → Ultimate_prerelease.exe"
            VERBATIM)
        usm_pack_exe(Ultimate_prerelease "${CMAKE_BINARY_DIR}/Ultimate_prerelease.exe")
        usm_pack_dll(Ultimate_prerelease "${CMAKE_BINARY_DIR}/USM_prelib_stripped.dll")
        usm_pack_dll(Ultimate_prerelease "$<TARGET_FILE:binkw32_prelib>")
        message(STATUS "Ultimate_prerelease.exe: dll2exe injector : ${DLL2EXE}")
        message(STATUS "Ultimate_prerelease.exe: inject strip     : ${USM_STRIP_INJECTED_DLL} (${CMAKE_STRIP})")
        if(_IS_WSL)
            message(STATUS "Ultimate_prerelease.exe: WSL argv paths translated via ${WSLPATH}")
        endif()
    else()
        message(WARNING
            "dll2exe.exe not found at ${DLL2EXE} — Ultimate_prerelease.exe "
            "will not be produced (USM_prelib.dll is still built).")
    endif()
endif()


# ─────────────────────────────────────────────────────────────────────────
#   Ultimate_Build.exe — inject the BUILD loader (binkw32_build)
#
#   Mirrors the Ultimate_prerelease.exe pipeline, but for the ultimate_build
#   variant and using its own binkw32_build-derived DLLs:
#
#     1. binkw32_build.dll → USM_build.dll                 (deployable loader)
#     2. binkw32_build.dll → USM_build_stripped.dll, then
#        strip --strip-all USM_build_stripped.dll          (the dll2exe input)
#     3. dll2exe -efix -injimp
#            Ultimate_Build_original.exe
#            USM_build_stripped.dll
#            Ultimate_Build.exe
# ─────────────────────────────────────────────────────────────────────────

if(TARGET ultimate_build AND TARGET binkw32_build)
    add_dependencies(ultimate_build binkw32_build)

    # (1) deployable loader: binkw32_build.dll → USM_build.dll
    add_custom_command(TARGET binkw32_build POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:binkw32_build>"
            "${CMAKE_BINARY_DIR}/USM_build.dll"
        COMMENT "Renaming build loader → USM_build.dll"
        VERBATIM)

    usm_pack_dll(binkw32_build "${CMAKE_BINARY_DIR}/USM_build.dll")

    if(EXISTS "${DLL2EXE}")
        _to_dll2exe_path("${_OUT_ultimate_build}"                     _OUT_BUILD_ARG)
        _to_dll2exe_path("${CMAKE_BINARY_DIR}/USM_build_stripped.dll" _BUILD_STRIPPED_ARG)
        _to_dll2exe_path("${CMAKE_BINARY_DIR}/Ultimate_Build.exe"     _BUILD_EXE_ARG)

        # (2)+(3) strip a copy of binkw32_build.dll, then inject it.
        add_custom_command(TARGET ultimate_build POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                    "-DSRC=$<TARGET_FILE:binkw32_build>"
                    "-DOUT=${CMAKE_BINARY_DIR}/USM_build_stripped.dll"
                    "-DSTRIP=${CMAKE_STRIP}"
                    "-DDOSTRIP=${USM_STRIP_INJECTED_DLL}"
                    -P "${_USM_MAKE_LOADER_SCRIPT}"
            COMMAND ${_DLL2EXE_RUN} -efix -injimp
                    "${_OUT_BUILD_ARG}"
                    "${_BUILD_STRIPPED_ARG}"
                    "${_BUILD_EXE_ARG}"
            COMMENT "Preparing build loader (strip=${USM_STRIP_INJECTED_DLL}) and injecting into Ultimate_Build_original.exe → Ultimate_Build.exe"
            VERBATIM)
        usm_pack_exe(ultimate_build "${CMAKE_BINARY_DIR}/Ultimate_Build.exe")
        usm_pack_dll(ultimate_build "${CMAKE_BINARY_DIR}/USM_build_stripped.dll")
        usm_pack_dll(ultimate_build "$<TARGET_FILE:binkw32_build>")
        message(STATUS "Ultimate_Build.exe: dll2exe injector : ${DLL2EXE}")
        message(STATUS "Ultimate_Build.exe: inject strip    : ${USM_STRIP_INJECTED_DLL} (${CMAKE_STRIP})")
        if(_IS_WSL)
            message(STATUS "Ultimate_Build.exe: WSL argv paths translated via ${WSLPATH}")
        endif()
    else()
        message(WARNING
            "dll2exe.exe not found at ${DLL2EXE} — Ultimate_Build.exe "
            "will not be produced (USM_build.dll is still built).")
    endif()
endif()


# ─────────────────────────────────────────────────────────────────────────
#   Auto-deploy built artifacts into ../Ultimate Spider-Man/ if present.
#   Stock DLLs already in that folder are left untouched.
# ─────────────────────────────────────────────────────────────────────────

option(USM_AUTO_DEPLOY "Copy built artifacts into ../Ultimate Spider-Man when that folder exists" ON)

set(_GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Ultimate Spider-Man")
if(USM_AUTO_DEPLOY AND EXISTS "${_GAME_DIR}/")

    # --- USM ---
    if(TARGET USM)
        add_custom_command(TARGET USM POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${_OUT_USM}" "${_GAME_DIR}/USM_original.exe"
            COMMENT "Deploying USM_original.exe to ${_GAME_DIR}"
            VERBATIM)
        if(EXISTS "${DLL2EXE}" AND TARGET binkw32)
            add_custom_command(TARGET USM POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_BINARY_DIR}/USM.exe" "${_GAME_DIR}/USM.exe"
                COMMENT "Deploying USM.exe to ${_GAME_DIR}"
                VERBATIM)
        endif()
    endif()

    # --- release loader binkw32.dll ---
    if(TARGET binkw32)
        add_custom_command(TARGET binkw32 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:binkw32>" "${_GAME_DIR}/binkw32.dll"
            COMMENT "Deploying binkw32.dll to ${_GAME_DIR}"
            VERBATIM)
    endif()

    # --- prerelease loader, deployed under the renamed USM_prelib.dll ---
    if(TARGET binkw32_prelib)
        add_custom_command(TARGET binkw32_prelib POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:binkw32_prelib>" "${_GAME_DIR}/USM_prelib.dll"
            COMMENT "Deploying USM_prelib.dll to ${_GAME_DIR}"
            VERBATIM)
    endif()

    # --- USMITA ---
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

    # --- Ultimate_prerelease ---
    if(TARGET Ultimate_prerelease)
        add_custom_command(TARGET Ultimate_prerelease POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${_OUT_Ultimate_prerelease}" "${_GAME_DIR}/Ultimate_prerelease_original.exe"
            COMMENT "Deploying Ultimate_prerelease_original.exe to ${_GAME_DIR}"
            VERBATIM)
        if(EXISTS "${DLL2EXE}" AND TARGET binkw32_prelib)
            add_custom_command(TARGET Ultimate_prerelease POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_BINARY_DIR}/Ultimate_prerelease.exe" "${_GAME_DIR}/Ultimate_prerelease.exe"
                COMMENT "Deploying Ultimate_prerelease.exe to ${_GAME_DIR}"
                VERBATIM)
        endif()
    endif()

    # --- ultimate_build ---
    if(TARGET ultimate_build)
        add_custom_command(TARGET ultimate_build POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${_OUT_ultimate_build}" "${_GAME_DIR}/Ultimate_Build_original.exe"
            COMMENT "Deploying Ultimate_Build_original.exe to ${_GAME_DIR}"
            VERBATIM)
        if(EXISTS "${DLL2EXE}" AND TARGET binkw32_build)
            add_custom_command(TARGET ultimate_build POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_BINARY_DIR}/Ultimate_Build.exe" "${_GAME_DIR}/Ultimate_Build.exe"
                COMMENT "Deploying Ultimate_Build.exe to ${_GAME_DIR}"
                VERBATIM)
        endif()
    endif()

    # --- build loader, deployed under the renamed USM_build.dll ---
    if(TARGET binkw32_build)
        add_custom_command(TARGET binkw32_build POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:binkw32_build>" "${_GAME_DIR}/USM_build.dll"
            COMMENT "Deploying USM_build.dll to ${_GAME_DIR}"
            VERBATIM)
    endif()

endif()


# ═══════════════════════════════════════════════════════════════════════
#   Final build-tree cleanup — keep only the shipping artifacts
#
#   After USM.exe (and USMITA.exe when present) are built and dll2exe has
#   consumed USM_stripped.dll, the intermediate loader DLLs are no longer
#   needed in the build tree:
#
#     • binkw32.dll      — already injected into USM.exe and already
#                          deployed to the game folder.
#     • USM.dll          — debug-laden copy of binkw32.dll, kept only as
#                          the input to strip.
#     • USM_stripped.dll — renamed to USM.dll so the final stripped,
#                          shippable DLL sits next to USM.exe.
#
#   Attached to the LAST target in the dll2exe chain (USMITA when it
#   exists, else USM) so it runs strictly after every step that reads
#   these files (in particular USMITA's POST_BUILD, which reads
#   USM_stripped.dll). The prerelease's USM_prelib*.dll files are left
#   untouched.
#
#   `cmake -E remove -f` (not `rm`) keeps this compatible with
#   cmake_minimum_required(3.12); `-f` makes the step idempotent on re-runs.
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

    usm_pack_dll(${_USM_CLEANUP_TARGET} "${CMAKE_BINARY_DIR}/USM.dll")

    message(STATUS "USM.exe: build-tree cleanup attached to target '${_USM_CLEANUP_TARGET}'")

    unset(_USM_CLEANUP_TARGET)
endif()
