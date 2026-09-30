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
#                                  built from Ultimate_prerelease.cpp). Shipped under
#                                  the name USM_prelib.dll.
#     • build/USM.dll            — stripped, shippable copy of binkw32.dll
#     • build/USM.exe            — USM_original.exe with a stripped
#                                  binkw32-derived DLL injected via
#                                  tools/dll2exe.exe.
#     • build/USMITA_original.exe / USMITA.exe — Italian retail variant,
#                                  compiled from USMITA.c (optional).
#     • build/Ultimate_prerelease_original.exe — PRERELEASE variant,
#                                  compiled from Ultimate_prerelease.c's
#                                  rawData[5947392] byte array
#                                  (5,947,392 bytes = 5.67 MiB — note this
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
#   Optional auto-deployment is controlled by USM_AUTO_DEPLOY and defaults to
#   OFF. When enabled, eligible outputs are copied to ../Ultimate Spider-Man/
#   if that directory exists next to the source tree. Stock DLLs (binkw32,
#   d3dx9_24, dsound, MSVCP71, msvcr71, xerces-c_2_3_0) are left untouched.
#
#   Required layout:
#
#       OpenUSM/
#       ├── CMakeLists.txt                 ← this file
#       ├── Ultimate_final.cpp                 ← binkw32 entry point
#       ├── Ultimate_prerelease.cpp                  ← binkw32_prelib entry point
#       ├── USM.c                          ← rawData[5926912] blob (REQUIRED)
#       ├── USMITA.c                       ← rawData[5926912] blob — ITA variant (OPTIONAL)
#       ├── Ultimate_prerelease.c          ← rawData[5947392] blob — PRERELEASE variant (OPTIONAL)
#       ├── emit_usm_exe.c                 ← host-tool writer       (REQUIRED)
#       ├── src/                           ← parent-project libraries
#       ├── cmake/
#       │   ├── i686-pc-mingw32.cmake      ← MinGW cross toolchain
#       │   └── report_size.cmake          ← size reporter for UPX step
#       ├── tools/
#       │   └── dll2exe.exe                ← DLL→EXE injector
#       ├── third_party/{glm,googletest}/  ← googletest retained for reference
#       ├── assimp/                        ← Assimp source tree
#       └── test/                          ← retained legacy test sources
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
#     -DENABLE_TESTS=ON              register ctest runtime tests (tools/runtime)
#                                    against -DUSM_TEST_GAME_DIR=<install>;
#                                    legacy unit tests are not wired to this build
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



# The rawData[] writers are build-time tools and must run on the host.  In a
# WSL cross build CMAKE_C_COMPILER is i686-w64-mingw32-gcc, which produces a
# Windows .exe that cannot be treated as a native Linux build tool.  Allow an
# explicit override, otherwise use the target compiler only on Windows hosts
# and locate a native C compiler everywhere else.
set(OPENUSM_HOST_C_COMPILER "" CACHE FILEPATH
    "Host C compiler used for rawData[] emitter tools")
if(OPENUSM_HOST_C_COMPILER)
    set(_OPENUSM_HOST_C_COMPILER "${OPENUSM_HOST_C_COMPILER}")
elseif(CMAKE_HOST_WIN32)
    set(_OPENUSM_HOST_C_COMPILER "${CMAKE_C_COMPILER}")
else()
    find_program(_OPENUSM_HOST_C_COMPILER
        NAMES cc gcc clang
        NO_CMAKE_FIND_ROOT_PATH)
    if(NOT _OPENUSM_HOST_C_COMPILER)
        message(FATAL_ERROR
            "A native host C compiler is required for the rawData[] emitter "
            "tools. Set -DOPENUSM_HOST_C_COMPILER=<path>.")
    endif()
endif()

if(CMAKE_HOST_WIN32)
    set(_OPENUSM_HOST_EXECUTABLE_SUFFIX ".exe")
else()
    set(_OPENUSM_HOST_EXECUTABLE_SUFFIX "")
endif()

function(openusm_add_host_emitter target_name output_path raw_source writer_source)
    get_filename_component(_emitter_dir "${output_path}" DIRECTORY)
    add_custom_command(
        OUTPUT "${output_path}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_emitter_dir}"
        COMMAND "${_OPENUSM_HOST_C_COMPILER}" -O0 ${ARGN}
                "${raw_source}" "${writer_source}" -o "${output_path}"
        DEPENDS "${raw_source}" "${writer_source}"
        COMMENT "Building host-native emitter ${target_name}"
        VERBATIM
        COMMAND_EXPAND_LISTS)
    add_custom_target(${target_name} DEPENDS "${output_path}")
endfunction()

option(ULTIMATE_RELEASE_XBPACK_MODE
    "Build ultimate_release for Xbox XBPACK data instead of retail PC PCPACK data"
    OFF)


if(MINGW)
    # Drop the "lib" prefix on MinGW so shared libs come out as
    # binkw32.dll / binkw32_prelib.dll — not libbinkw32.dll etc.
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_STATIC_LIBRARY_PREFIX "")
    set(CMAKE_SHARED_MODULE_PREFIX  "")
endif()


# ═══════════════════════════════════════════════════════════════════════
#   ultimate_release_original.exe — standalone release variant
# ═══════════════════════════════════════════════════════════════════════
set(ultimate_release_C_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/Ultimate_release.c"
    CACHE FILEPATH "Path to Ultimate_release.c rawData[] blob")
set(_OUT_ultimate_release
    "${CMAKE_BINARY_DIR}/ultimate_release_original.exe")
set(_ULTIMATE_RELEASE_WRITER "${CMAKE_CURRENT_SOURCE_DIR}/emit_usm_exe.c")
if(EXISTS "${ultimate_release_C_SOURCE}" AND EXISTS "${_ULTIMATE_RELEASE_WRITER}")
    set(_ULTIMATE_RELEASE_EMITTER
        "${CMAKE_BINARY_DIR}/emit_ultimate_release_exe_build/emit_ultimate_release_exe${_OPENUSM_HOST_EXECUTABLE_SUFFIX}")
    openusm_add_host_emitter(emit_ultimate_release_exe_host
        "${_ULTIMATE_RELEASE_EMITTER}"
        "${ultimate_release_C_SOURCE}"
        "${_ULTIMATE_RELEASE_WRITER}")
    add_custom_command(OUTPUT "${_OUT_ultimate_release}"
        COMMAND "${_ULTIMATE_RELEASE_EMITTER}" "${_OUT_ultimate_release}"
        DEPENDS emit_ultimate_release_exe_host
                "${ultimate_release_C_SOURCE}" "${_ULTIMATE_RELEASE_WRITER}"
        COMMENT "Compiling ultimate_release_original.exe from Ultimate_release.c"
        VERBATIM)
    add_custom_target(ultimate_release_image ALL DEPENDS "${_OUT_ultimate_release}")
endif()


# ─────────────────────────────────────────────────────────────────────────
#   Feature toggles (most are runtime-free defines that flip compile paths)
# ─────────────────────────────────────────────────────────────────────────

option(USE_GLM            ""                                             OFF)
option(USE_FSTREAM        ""                                             OFF)
option(CHECK_STRUCT_SIZES ""                                             ON)
option(TARGET_XBOX        ""                                             OFF)
option(ENABLE_TESTS "Enable the retained legacy unit tests (currently unavailable)" OFF)
option(OPENUSM_BUILD_MESH_PROBES "Build standalone FBX and PCMESH validation tools" OFF)

if(OPENUSM_BUILD_MESH_PROBES)
    foreach(probe mod_mesh_import_probe mod_mesh_import_build_probe mod_mesh_swap_probe mod_pcmesh_archive_probe mod_pcmesh_source_probe mod_mesh_aliases_probe mod_mesh_retarget_probe mod_pcskel_source_probe mod_polytube_attachment_probe mod_mesh_attachment_geometry_probe mod_nal_config_probe mod_pcanim_source_probe mod_mesh_custom_probe mod_mesh_texture_source_probe mod_mesh_donor_pose_probe
        mod_mesh_donor_surface_probe mod_mesh_axes_probe mod_mesh_axes_import_probe
        mod_mesh_static_probe mod_mesh_static_import_probe mod_mesh_foreign_rig_probe mod_mesh_foreign_import_probe mod_miles_skin_probe)
        add_executable(${probe} test/${probe}.cpp)
        target_link_options(${probe} PRIVATE -static-libgcc -static-libstdc++)
    endforeach()
    add_executable(build_miles_skin tools/build_miles_skin.cpp)
    add_executable(export_miles_donor_weights tools/export_miles_donor_weights.cpp)
    target_link_options(build_miles_skin PRIVATE -static-libgcc -static-libstdc++)
    target_link_options(export_miles_donor_weights PRIVATE -static-libgcc -static-libstdc++)
    enable_testing()
    add_test(NAME mesh_aliases_validation COMMAND mod_mesh_aliases_probe)
    add_test(NAME mesh_retarget_validation COMMAND mod_mesh_retarget_probe)
    add_test(NAME pcskel_source_validation COMMAND mod_pcskel_source_probe)
    add_test(NAME pcmesh_scope_validation COMMAND mod_pcmesh_source_probe)
    add_test(NAME polytube_attachment_validation COMMAND mod_polytube_attachment_probe)
    add_test(NAME mesh_attachment_geometry_validation COMMAND mod_mesh_attachment_geometry_probe)
    add_test(NAME nal_config_validation COMMAND mod_nal_config_probe)
    add_test(NAME pcanim_source_validation COMMAND mod_pcanim_source_probe)
    add_test(NAME custom_fbx_geometry_validation COMMAND mod_mesh_custom_probe)
    add_test(NAME custom_fbx_texture_validation COMMAND mod_mesh_texture_source_probe)
    add_test(NAME custom_fbx_donor_pose_validation COMMAND mod_mesh_donor_pose_probe)
    add_test(NAME custom_fbx_donor_surface_validation COMMAND mod_mesh_donor_surface_probe)
    add_test(NAME fbx_axes_validation COMMAND mod_mesh_axes_probe)
    add_test(NAME fbx_axes_import_validation COMMAND mod_mesh_axes_import_probe)
    add_test(NAME static_mesh_layout_validation COMMAND mod_mesh_static_probe)
    add_test(NAME static_mesh_import_validation COMMAND mod_mesh_static_import_probe)
    add_test(NAME foreign_rig_validation COMMAND mod_mesh_foreign_rig_probe)
    add_test(NAME foreign_fbx_import_validation COMMAND mod_mesh_foreign_import_probe)
    add_test(NAME miles_skin_validation COMMAND mod_miles_skin_probe)
    if(CMAKE_HOST_WIN32)
        find_package(Python3 QUIET COMPONENTS Interpreter)
        if(Python3_Interpreter_FOUND)
            enable_testing()
            add_test(NAME pcmesh_archive_validation
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/test/verify_pcmesh_archive.py"
                    "$<TARGET_FILE:mod_pcmesh_archive_probe>")
        endif()
    endif()
endif()

# ENABLE_TESTS registers bounded runtime tests (tools/runtime/runtime_test.ps1)
# that deploy a built image into a real game install, run it windowed and
# restore the install afterwards. The retained legacy unit-test sources still
# link the removed monolithic 'usm' target and are not part of this build.
set(USM_TEST_GAME_DIR "C:/Users/devusm1000/Desktop/Ultimate Spider-Man (20 june 2005) PC"
    CACHE PATH "XBPACK v10 prerelease install used by the ENABLE_TESTS runtime tests (Windows path)")
set(USM_TEST_RELEASE_GAME_DIR ""
    CACHE PATH "Optional retail install for an ultimate_release.exe runtime test (empty = skip)")
set(USM_TEST_SECONDS "90" CACHE STRING "Duration of each runtime test, in seconds")
if(ENABLE_TESTS)
    find_program(USM_POWERSHELL
        NAMES powershell.exe powershell
        PATHS "/mnt/c/Windows/System32/WindowsPowerShell/v1.0"
              "C:/Windows/System32/WindowsPowerShell/v1.0"
        NO_CMAKE_FIND_ROOT_PATH)
    if(NOT USM_POWERSHELL)
        message(FATAL_ERROR "ENABLE_TESTS needs Windows PowerShell (powershell.exe) to drive the game")
    endif()
    enable_testing()
    message(STATUS "Runtime tests enabled against ${USM_TEST_GAME_DIR} (${USM_TEST_SECONDS}s each)")
endif()


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


if (TARGET_XBOX)
    message(FATAL_ERROR "TARGET_XBOX and the PC XBPACK loader variants are mutually exclusive")
endif()

add_library(utility src/utility/trace.cpp)
target_include_directories(utility PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/utility>)

add_library(variables src/variables/variables.cpp)
target_include_directories(variables PUBLIC "src/variables")
target_link_libraries(variables PUBLIC utility)


# The retail PC loader must use the game's native one-buffer mash ABI.  Do not
# share this target with either XBPACK loader: the Xbox mash_info_struct is a
# different size and interprets the original PC object's fields differently.
add_library(mash src/mash/pc/src/mash_info_struct.cpp)
target_include_directories(mash PUBLIC "src/mash/pc/include/")
target_include_directories(mash PUBLIC "src" "include")
target_compile_options   (mash PRIVATE ${flags})
target_link_libraries    (mash PUBLIC utility variables)

# The later Xbox build uses the v14 two-buffer mash layout.
add_library(mash_xbpack src/mash/xbox/src/mash_info_struct.cpp)
target_include_directories(mash_xbpack PUBLIC "src/mash/xbox/include/")
target_include_directories(mash_xbpack PUBLIC "src" "include")
target_compile_options   (mash_xbpack PRIVATE ${flags})
target_compile_definitions(mash_xbpack PRIVATE OPENUSM_XBPACK_MODE)
target_link_libraries    (mash_xbpack PUBLIC utility variables)

# The June 2005 prerelease uses the older XBPACK v10 mash header layout.  Keep
# that ABI isolated from both the retail PC and later Xbox build loaders.
add_library(mash_prelib src/mash/xbox/src/mash_info_struct.cpp)
target_include_directories(mash_prelib PUBLIC "src/mash/xbox/include/")
target_include_directories(mash_prelib PUBLIC "src" "include")
target_compile_options   (mash_prelib PRIVATE ${flags})
target_compile_definitions(mash_prelib PRIVATE
    OPENUSM_XBPACK_MODE
    OPENUSM_XBPACK_V10)
target_link_libraries    (mash_prelib PUBLIC utility variables)


# ─────────────────────────────────────────────────────────────────────────
#   binkw32.dll (release) + binkw32_prelib.dll (prerelease) — OpenUSM loaders
#
#   Both share the same ${SRC} glob (the in-process mod-loader code). They
#   differ only in entry point: Ultimate_final.cpp vs. Ultimate_prerelease.cpp.
# ─────────────────────────────────────────────────────────────────────────

# The original openusm fork contributed a number of incomplete translation
# units that are useful as reference source, but were never part of the merged
# xbpack-mode build.  Keep them in the unified tree without letting broad
# source globs silently add them to production targets.
set(OPENUSM_RETAINED_LEGACY_CPP
    "src/actor_interface.cpp"
    "src/ai_state_car.cpp"
    "src/ai_std_avoidance.cpp"
    "src/ai_voice_box_inode.cpp"
    "src/als_adv_mocomps.cpp"
    "src/anim_event.cpp"
    "src/camera_setup_entry.cpp"
    "src/charanimtype/arbitrarypocharcomp.cpp"
    "src/charanimtype/armikentcompdecomp.cpp"
    "src/charanimtype/armikposedesc.cpp"
    "src/charanimtype/armstdposedesc.cpp"
    "src/charanimtype/character_anim_inst.cpp"
    "src/charanimtype/charcompressor.cpp"
    "src/charanimtype/fakerootentcompdecomp.cpp"
    "src/charanimtype/fing52knuckcurlentcompdecomp.cpp"
    "src/charanimtype/fing52knuckcurlposedesc.cpp"
    "src/charanimtype/fing5curlentcompdecomp.cpp"
    "src/charanimtype/fing5curlposedesc.cpp"
    "src/charanimtype/fing5redentcompdecomp.cpp"
    "src/charanimtype/fing5reducedposedesc.cpp"
    "src/charanimtype/finger5stdposedesc.cpp"
    "src/charanimtype/flexiblecharcomp.cpp"
    "src/charanimtype/floatsentcompdecomp.cpp"
    "src/charanimtype/genericcharcomp.cpp"
    "src/charanimtype/legsikposedesc.cpp"
    "src/charanimtype/legsstdposedesc.cpp"
    "src/charanimtype/quatsentcompdecomp.cpp"
    "src/charanimtype/tentaclesposedesc.cpp"
    "src/charanimtype/torsoheadentcompdecomp.cpp"
    "src/charanimtype/torsoheadstdposedesc.cpp"
    "src/chuck/vm/script_var_container.cpp"
    "src/component.cpp"
    "src/conglomerate_interface.cpp"
    "src/entity_base_interface.cpp"
    "src/entity_class_entry.cpp"
    "src/entity_interface.cpp"
    "src/entity_viseme_entry.cpp"
    "src/guidance_sys.cpp"
    "src/mVectorBasic.cpp"
    "src/nalcomp/component_id.cpp"
    "src/nsl/src/nsl/nslinit.cpp"
    "src/nsl/src/nsl/nslsource.cpp"
    "src/nugget.cpp"
    "src/parking_marker.cpp"
    "src/script_lib_beam.cpp"
    "src/terrain_decal.cpp"
    "src/tracking_panel.cpp"
    "src/traffic_light_interface.cpp"
)

function(openusm_filter_retained_legacy list_name)
    set(filtered_sources)
    foreach(source_path IN LISTS ${list_name})
        file(RELATIVE_PATH relative_source "${CMAKE_CURRENT_SOURCE_DIR}" "${source_path}")
        string(REPLACE "\\" "/" relative_source "${relative_source}")
        list(FIND OPENUSM_RETAINED_LEGACY_CPP "${relative_source}" legacy_index)
        if(legacy_index EQUAL -1)
            list(APPEND filtered_sources "${source_path}")
        endif()
    endforeach()
    set(${list_name} "${filtered_sources}" PARENT_SCOPE)
endfunction()

file(GLOB SRC CONFIGURE_DEPENDS "./src/*.cpp" "./src/*.h")
openusm_filter_retained_legacy(SRC)

add_library(USM SHARED Ultimate_final.cpp ${SRC})
target_compile_options(USM PRIVATE ${flags})


add_library(ultimate_prerelease SHARED Ultimate_prerelease.cpp ${SRC})
target_compile_options(ultimate_prerelease PRIVATE ${flags})
target_compile_definitions(ultimate_prerelease PRIVATE
    OPENUSM_XBPACK_MODE
    OPENUSM_XBPACK_V10)

add_library(ultimate_build SHARED Ultimate_build.cpp ${SRC})
target_compile_options(ultimate_build PRIVATE ${flags})
target_compile_definitions(ultimate_build PRIVATE OPENUSM_XBPACK_MODE)

# Standalone release variant.  This is intentionally a real target (rather
# than an ALIAS) so it can use its own entry point and produce its own DLL.
add_library(ultimate_release SHARED Ultimate_release.cpp ${SRC})
target_compile_options(ultimate_release PRIVATE ${flags})
target_compile_definitions(ultimate_release PRIVATE OPENUSM_ULTIMATE_RELEASE)
if(ULTIMATE_RELEASE_XBPACK_MODE)
    target_compile_definitions(ultimate_release PRIVATE OPENUSM_XBPACK_MODE)
endif()



# ─────────────────────────────────────────────────────────────────────────
#   memory / slab_allocator
# ─────────────────────────────────────────────────────────────────────────

add_library(memory "./src/memory.cpp" "./src/memory.h")
target_compile_options    (memory PRIVATE ${flags})
target_include_directories(memory PUBLIC  "./include/" "./src/")
target_link_libraries     (memory PUBLIC  utility)

add_library(slab_allocator "./src/slab_allocator.cpp" "./src/slab_allocator.h")
target_compile_options    (slab_allocator PRIVATE ${flags})
target_include_directories(slab_allocator PUBLIC  "./include/" "./src/")
target_link_libraries     (slab_allocator PUBLIC  memory)


# ─────────────────────────────────────────────────────────────────────────
#   nalcomp / nalpanel / nsl / nal (animation + panel systems)
# ─────────────────────────────────────────────────────────────────────────

file(GLOB_RECURSE NALCOMP_SRC "./src/nalcomp/*.cpp")
openusm_filter_retained_legacy(NALCOMP_SRC)
add_library(nalcomp ${NALCOMP_SRC})
target_compile_options    (nalcomp PRIVATE ${flags})
target_include_directories(nalcomp PUBLIC  "./src/nalcomp/" "./src/")

file(GLOB_RECURSE NALPANEL_SRC "./src/panels/nalpanel/*.cpp")
openusm_filter_retained_legacy(NALPANEL_SRC)
add_library(nalpanel ${NALPANEL_SRC})
target_compile_options    (nalpanel PRIVATE ${flags})
target_include_directories(nalpanel PUBLIC  "./src/panels/nalpanel/")
target_link_libraries     (nalpanel PUBLIC  nalcomp)

file(GLOB_RECURSE NSL_SRC "./src/nsl/src/nsl/*.cpp")
openusm_filter_retained_legacy(NSL_SRC)
add_library(nsl ${NSL_SRC})
target_compile_options    (nsl PRIVATE ${flags})
target_include_directories(nsl PUBLIC  "./include/" "./src/" "./src/nsl/src/nsl/")
target_link_libraries     (nsl PUBLIC  nfl utility)

file(GLOB_RECURSE NAL_SRC "./src/nal_system.cpp" "./src/nal/source/common/*.cpp")
openusm_filter_retained_legacy(NAL_SRC)
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
openusm_filter_retained_legacy(SRC_CHUCK)
add_library(chuck ${SRC_CHUCK})
target_compile_options    (chuck PRIVATE ${flags})
target_include_directories(chuck INTERFACE "./src/chuck/vm/")
target_link_libraries     (chuck PUBLIC   slab_allocator variables)

# Chuck's mashed executable records also changed in XBPACK v10.  Compile a
# prerelease-only copy so the other loader targets keep their native layout.
add_library(chuck_prelib ${SRC_CHUCK})
target_compile_options    (chuck_prelib PRIVATE ${flags})
target_compile_definitions(chuck_prelib PRIVATE
    OPENUSM_XBPACK_MODE
    OPENUSM_XBPACK_V10)
target_include_directories(chuck_prelib INTERFACE "./src/chuck/vm/")
target_link_libraries     (chuck_prelib PUBLIC slab_allocator variables)


# ─────────────────────────────────────────────────────────────────────────
#   tx / nfl / ngl / charanimtype / physics
# ─────────────────────────────────────────────────────────────────────────

file(GLOB_RECURSE SRC_TX "./src/tx/src/*.cpp" "./src/tx/src/*.h")
openusm_filter_retained_legacy(SRC_TX)
add_library(tx ${SRC_TX})
target_compile_options    (tx PRIVATE ${flags})
target_include_directories(tx INTERFACE "./src/tx/src/")
target_link_libraries     (tx PUBLIC    utility)

add_library(test_tx ${SRC_TX})
target_compile_options    (test_tx PRIVATE ${flags})
target_include_directories(test_tx INTERFACE "./src/tx/src/")
target_link_libraries     (test_tx PUBLIC    utility)

file(GLOB_RECURSE SRC_NFL "./src/nfl/src/*.cpp" "./src/nfl/src/*.h")
openusm_filter_retained_legacy(SRC_NFL)
add_library(nfl ${SRC_NFL})
target_compile_options    (nfl PRIVATE ${flags})
target_include_directories(nfl INTERFACE "./src/nfl/src/")
target_link_libraries     (nfl PUBLIC    tx)
target_link_libraries     (mash PUBLIC   nfl)
target_link_libraries     (mash_xbpack PUBLIC nfl)
target_link_libraries     (mash_prelib PUBLIC nfl)

add_library(test_nfl ${SRC_NFL})
target_compile_options    (test_nfl PRIVATE ${flags})
target_include_directories(test_nfl INTERFACE "./src/nfl/src/")
target_link_libraries     (test_nfl PUBLIC    test_tx)

file(GLOB_RECURSE SRC_NGL_DX "./src/ngl/src/dx/*.cpp")
openusm_filter_retained_legacy(SRC_NGL_DX)
add_library(ngl_dx ${SRC_NGL_DX})
target_compile_options    (ngl_dx PRIVATE ${flags})
target_include_directories(ngl_dx PUBLIC  "./src/ngl/include/dx" "./src/ngl/include/")
target_link_libraries     (ngl_dx PUBLIC  psapi utility variables)

file(GLOB_RECURSE SRC_NGL_SHADERS "./src/ngl/shaders/*.cpp")
openusm_filter_retained_legacy(SRC_NGL_SHADERS)
add_library(ngl_shaders ${SRC_NGL_SHADERS})
target_compile_options    (ngl_shaders PRIVATE ${flags})
target_include_directories(ngl_shaders PUBLIC  "./src/ngl/shaders")
target_link_libraries     (ngl_shaders PUBLIC  ngl_dx)

file(GLOB SRC_NGL "./src/ngl/src/*.cpp")
openusm_filter_retained_legacy(SRC_NGL)
add_library(ngl ${SRC_NGL})
target_compile_options    (ngl PRIVATE ${flags})
target_include_directories(ngl PUBLIC  "./src/ngl/include/")
target_link_libraries     (ngl PUBLIC  ngl_dx ngl_shaders)

file(GLOB SRC_CHARANIMTYPE "./src/charanimtype/*.cpp")
openusm_filter_retained_legacy(SRC_CHARANIMTYPE)
add_library(charanimtype ${SRC_CHARANIMTYPE})
target_compile_options    (charanimtype PRIVATE ${flags})
target_include_directories(charanimtype PUBLIC  "./src/charanimtype/")
target_link_libraries     (charanimtype PUBLIC  nal ngl_dx ngl_shaders)

file(GLOB_RECURSE PHYSICS_SRC "./src/physics/source/*.cpp")
openusm_filter_retained_legacy(PHYSICS_SRC)
add_library(physics ${PHYSICS_SRC})
target_compile_options    (physics PRIVATE ${flags})
target_include_directories(physics PUBLIC  "./src/physics/include/")
target_link_libraries     (physics PUBLIC  utility ngl)


# ─────────────────────────────────────────────────────────────────────────
#   Shared settings for binkw32 & binkw32_prelib (same code, different entry)
# ─────────────────────────────────────────────────────────────────────────

if(USE_FSTREAM)
    target_compile_definitions(USM               PUBLIC USE_FSTREAM)
    target_compile_definitions(ultimate_release    PUBLIC USE_FSTREAM)
    target_compile_definitions(ultimate_prerelease PUBLIC USE_FSTREAM)
    target_compile_definitions(ultimate_build      PUBLIC USE_FSTREAM)
endif()

target_include_directories(USM               PUBLIC "include" "./src/nfl/src/")
target_include_directories(ultimate_release    PUBLIC "include" "./src/nfl/src/")
target_include_directories(ultimate_prerelease PUBLIC "include" "./src/nfl/src/")
target_include_directories(ultimate_build      PUBLIC "include" "./src/nfl/src/")

target_include_directories(chuck        PUBLIC "include" "src")
target_include_directories(chuck_prelib PUBLIC "include" "src")
target_include_directories(tx           PUBLIC "include" "src")
target_include_directories(ngl_dx       PUBLIC "include" "src")
target_include_directories(test_tx      PUBLIC "include" "src")


# ─────────────────────────────────────────────────────────────────────────
#   Assimp (external project, built once)
# ─────────────────────────────────────────────────────────────────────────

ExternalProject_Add(AssimpProject
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/assimp
    BINARY_DIR ${CMAKE_BINARY_DIR}/assimp-build
    BUILD_BYPRODUCTS
        ${CMAKE_BINARY_DIR}/assimp-build/lib/libassimp.a
        ${CMAKE_BINARY_DIR}/assimp-build/contrib/zlib/libzlibstatic.a
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

target_include_directories(USM PUBLIC "./assimp/include/")
target_link_directories   (USM PUBLIC ${CMAKE_BINARY_DIR}/assimp-build/lib)
add_dependencies          (USM AssimpProject)
target_include_directories(ultimate_release PUBLIC "./assimp/include/")
target_link_directories   (ultimate_release PUBLIC ${CMAKE_BINARY_DIR}/assimp-build/lib)
add_dependencies          (ultimate_release AssimpProject)

target_include_directories(ultimate_prerelease PUBLIC "./assimp/include/")
target_link_directories   (ultimate_prerelease PUBLIC ${CMAKE_BINARY_DIR}/assimp-build/lib)
add_dependencies          (ultimate_prerelease AssimpProject)

target_include_directories(ultimate_build PUBLIC "./assimp/include/")
target_link_directories   (ultimate_build PUBLIC ${CMAKE_BINARY_DIR}/assimp-build/lib)
add_dependencies          (ultimate_build AssimpProject)


# ─────────────────────────────────────────────────────────────────────────
#   Final link lines for binkw32 and binkw32_prelib
#
#   (Top level — NOT inside any if(TARGET USM). The loaders must link
#    regardless of whether USM.c / the exe blobs are present.)
# ─────────────────────────────────────────────────────────────────────────

# Ultimate Spider-Man ships the June 2005 D3DX runtime.  MinGW's unversioned
# import library currently resolves to d3dx9_43.dll, which is not present in
# the game directory and prevents an injected executable from starting.
target_link_libraries(ngl_dx PUBLIC d3dx9_24)

# --- binkw32 (release loader) ---
target_link_directories(USM PUBLIC ${CMAKE_FIND_ROOT_PATH}/include)
target_link_directories(USM PUBLIC ${CMAKE_FIND_ROOT_PATH}/lib)
target_link_libraries  (USM PUBLIC physics
                                       charanimtype mash chuck nfl ngl nal nsl utility
                                       winmm psapi d3d9 dinput8 dxguid dsound assimp
                                       "${CMAKE_BINARY_DIR}/assimp-build/contrib/zlib/libzlibstatic.a")
target_link_directories(ultimate_release PUBLIC ${CMAKE_FIND_ROOT_PATH}/include)
target_link_directories(ultimate_release PUBLIC ${CMAKE_FIND_ROOT_PATH}/lib)
if(ULTIMATE_RELEASE_XBPACK_MODE)
    target_link_libraries(ultimate_release PUBLIC physics
        charanimtype mash_xbpack chuck nfl ngl nal nsl utility
        winmm psapi d3d9 dinput8 dxguid dsound assimp
        "${CMAKE_BINARY_DIR}/assimp-build/contrib/zlib/libzlibstatic.a")
else()
    target_link_libraries(ultimate_release PUBLIC physics
        charanimtype mash chuck nfl ngl nal nsl utility
        winmm psapi d3d9 dinput8 dxguid dsound assimp
        "${CMAKE_BINARY_DIR}/assimp-build/contrib/zlib/libzlibstatic.a")
endif()
# Private-LAN multiplayer transport (retail PC entrypoints only).
target_link_libraries(USM PUBLIC ws2_32 bcrypt iphlpapi)
target_link_libraries(ultimate_release PUBLIC ws2_32 bcrypt iphlpapi)
if(USE_GLM)
    target_link_libraries(USM PUBLIC glm::glm)
    target_link_libraries(ultimate_release PUBLIC glm::glm)
endif()

# --- binkw32_prelib (June 2005 XBPACK v10 prerelease loader) ---
target_link_directories(ultimate_prerelease PUBLIC ${CMAKE_FIND_ROOT_PATH}/include)
target_link_directories(ultimate_prerelease PUBLIC ${CMAKE_FIND_ROOT_PATH}/lib)
target_link_libraries  (ultimate_prerelease PUBLIC physics
                                       charanimtype mash_prelib chuck_prelib nfl ngl nal nsl utility
                                       winmm psapi d3d9 dinput8 dxguid dsound assimp
                                       "${CMAKE_BINARY_DIR}/assimp-build/contrib/zlib/libzlibstatic.a")

target_link_directories(ultimate_build PUBLIC ${CMAKE_FIND_ROOT_PATH}/include)
target_link_directories(ultimate_build PUBLIC ${CMAKE_FIND_ROOT_PATH}/lib)
target_link_libraries  (ultimate_build PUBLIC physics
                                       charanimtype mash_xbpack chuck nfl ngl nal nsl utility
                                       winmm psapi d3d9 dinput8 dxguid dsound assimp
                                       "${CMAKE_BINARY_DIR}/assimp-build/contrib/zlib/libzlibstatic.a")
if(USE_GLM)
    target_link_libraries(ultimate_prerelease PUBLIC glm::glm)
    target_link_libraries(ultimate_build PUBLIC glm::glm)
endif()

# Pin both loaders' output directory so the inject/deploy steps below know
# exactly where to find them.
set_target_properties(USM PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(ultimate_release PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

foreach(_USM_LOADER_TARGET USM ultimate_release ultimate_prerelease ultimate_build)
    target_compile_options(${_USM_LOADER_TARGET} PRIVATE
        -Os -ffunction-sections -fdata-sections)
    set_property(TARGET ${_USM_LOADER_TARGET} APPEND_STRING PROPERTY
        LINK_FLAGS " -Wl,--gc-sections -Wl,--strip-all")
endforeach()
unset(_USM_LOADER_TARGET)
set_target_properties(ultimate_prerelease PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(ultimate_build PROPERTIES
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
    set(_USM_EMITTER
        "${CMAKE_BINARY_DIR}/emit_usm_exe_build/emit_usm_exe${_OPENUSM_HOST_EXECUTABLE_SUFFIX}")
    openusm_add_host_emitter(emit_usm_exe_host
        "${_USM_EMITTER}"
        "${USM_C_SOURCE}"
        "${USM_C_WRITER_SOURCE}")

    add_custom_command(
        OUTPUT  "${_OUT_USM}"
        COMMAND "${_USM_EMITTER}" "${_OUT_USM}"
        DEPENDS emit_usm_exe_host "${USM_C_SOURCE}" "${USM_C_WRITER_SOURCE}"
        COMMENT "Compiling USM_original.exe from USM.c rawData[] (5,926,912 bytes / 5.65 MiB)"
        VERBATIM
    )

    add_custom_target(USM_original ALL DEPENDS "${_OUT_USM}")

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
    set(_USMITA_EMITTER
        "${CMAKE_BINARY_DIR}/emit_usmita_exe_build/emit_usmita_exe${_OPENUSM_HOST_EXECUTABLE_SUFFIX}")
    openusm_add_host_emitter(emit_usmita_exe_host
        "${_USMITA_EMITTER}"
        "${USMITA_C_SOURCE}"
        "${USM_C_WRITER_SOURCE}")

    add_custom_command(
        OUTPUT  "${_OUT_USMITA}"
        COMMAND "${_USMITA_EMITTER}" "${_OUT_USMITA}"
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
#   Ultimate_Build_original.exe — build variant, compiled from Ultimate_build.c
#
#   Same rawData[5926912] symbol as USM.c, so it's built as a parallel
#   host writer (emit_ultimate_build_exe). Skipped silently if Ultimate_build.c is absent.
# ═══════════════════════════════════════════════════════════════════════

set(ultimate_build_C_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/Ultimate_build.c"
    CACHE FILEPATH "Path to Ultimate_build.c with rawData[5926912] byte array (build variant)")

set(_OUT_ultimate_build "${CMAKE_BINARY_DIR}/Ultimate_Build_original.exe")

if(NOT EXISTS "${ultimate_build_C_SOURCE}")
    message(STATUS
        "Ultimate_build.c not found at ${ultimate_build_C_SOURCE} — ultimate_build target "
        "skipped. (Optional Italian variant; set -Dultimate_build_C_SOURCE=<path> "
        "to enable.)")
elseif(NOT EXISTS "${USM_C_WRITER_SOURCE}")
    # Missing-writer warning already emitted by the USM block. Silent here.
else()
    set(_ULTIMATE_BUILD_EMITTER
        "${CMAKE_BINARY_DIR}/emit_ultimate_build_exe_build/emit_ultimate_build_exe${_OPENUSM_HOST_EXECUTABLE_SUFFIX}")
    openusm_add_host_emitter(emit_ultimate_build_exe_host
        "${_ULTIMATE_BUILD_EMITTER}"
        "${ultimate_build_C_SOURCE}"
        "${USM_C_WRITER_SOURCE}")

    add_custom_command(
        OUTPUT  "${_OUT_ultimate_build}"
        COMMAND "${_ULTIMATE_BUILD_EMITTER}" "${_OUT_ultimate_build}"
        DEPENDS emit_ultimate_build_exe_host
                "${ultimate_build_C_SOURCE}" "${USM_C_WRITER_SOURCE}"
        COMMENT "Compiling Ultimate_Build_original.exe from Ultimate_build.c rawData[] (5,926,912 bytes / 5.65 MiB)"
        VERBATIM
    )

    add_custom_target(ultimate_build_original ALL DEPENDS "${_OUT_ultimate_build}")

    file(SIZE   "${ultimate_build_C_SOURCE}" _ultimate_build_C_SIZE)
    file(SHA256 "${ultimate_build_C_SOURCE}" _ultimate_build_C_SHA)
    message(STATUS "Ultimate_Build_original.exe: compiling from Ultimate_build.c rawData[]")
    message(STATUS "  Ultimate_build.c       : ${ultimate_build_C_SOURCE}")
    message(STATUS "  Ultimate_build.c size  : ${_ultimate_build_C_SIZE} bytes")
    message(STATUS "  Ultimate_build.c sha256: ${_ultimate_build_C_SHA}")
    message(STATUS "  writer         : ${USM_C_WRITER_SOURCE}")
    message(STATUS "  output         : ${_OUT_ultimate_build} (expected: 5926912 bytes / 5.65 MiB)")
endif()


# ═══════════════════════════════════════════════════════════════════════
#   Ultimate_prerelease_original.exe — PRERELEASE variant
#
#   Ultimate_prerelease.c is the same kind of ASCII-hex blob as USM.c, but
#   declares  unsigned char rawData[5947392]  — the bytes of the Ultimate
#   Spider-Man PRERELEASE build (5,947,392 bytes / 5.67 MiB).
#
#   ⚠ SIZE DIFFERS from retail: 5,947,392 (prerelease) vs 5,926,912
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
    CACHE FILEPATH "Path to Ultimate_prerelease.c with rawData[5947392] byte array (prerelease variant)")

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
    # Mirrors the USM_original.exe emitter. The host program is
    # named emit_Ultimate_prerelease_exe so it cannot collide with the
    # English / Italian writers on disk or at link time.
    set(_ULTIMATE_PRERELEASE_EMITTER
        "${CMAKE_BINARY_DIR}/emit_Ultimate_prerelease_exe_build/emit_Ultimate_prerelease_exe${_OPENUSM_HOST_EXECUTABLE_SUFFIX}")
    openusm_add_host_emitter(emit_Ultimate_prerelease_exe_host
        "${_ULTIMATE_PRERELEASE_EMITTER}"
        "${Ultimate_prerelease_C_SOURCE}"
        "${USM_C_WRITER_SOURCE}"
        -DRAWDATA_SIZE=5947392u)

    add_custom_command(
        OUTPUT  "${_OUT_Ultimate_prerelease}"
        COMMAND "${_ULTIMATE_PRERELEASE_EMITTER}" "${_OUT_Ultimate_prerelease}"
        DEPENDS emit_Ultimate_prerelease_exe_host
                "${Ultimate_prerelease_C_SOURCE}" "${USM_C_WRITER_SOURCE}"
        COMMENT "Compiling Ultimate_prerelease_original.exe from Ultimate_prerelease.c rawData[] (5,947,392 bytes / 5.67 MiB)"
        VERBATIM
    )

    add_custom_target(ultimate_prerelease_image ALL DEPENDS "${_OUT_Ultimate_prerelease}")

    file(SIZE   "${Ultimate_prerelease_C_SOURCE}" _PRE_C_SIZE)
    file(SHA256 "${Ultimate_prerelease_C_SOURCE}" _PRE_C_SHA)
    message(STATUS "Ultimate_prerelease_original.exe: compiling from Ultimate_prerelease.c rawData[]")
    message(STATUS "  Ultimate_prerelease.c       : ${Ultimate_prerelease_C_SOURCE}")
    message(STATUS "  Ultimate_prerelease.c size  : ${_PRE_C_SIZE} bytes")
    message(STATUS "  Ultimate_prerelease.c sha256: ${_PRE_C_SHA}")
    message(STATUS "  writer         : ${USM_C_WRITER_SOURCE}")
    message(STATUS "  output         : ${_OUT_Ultimate_prerelease} (expected: 5947392 bytes / 5.67 MiB)")
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
    if(TARGET USM_original)
        add_dependencies(USM USM_original)
    endif()

    if(TARGET USM)
        # USM is the release loader target itself.
        add_custom_command(TARGET USM POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:USM>"
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
        if(TARGET USM)
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
#   ultimate_release.exe — inject ultimate_release.dll into the release image
# ─────────────────────────────────────────────────────────────────────────
if(TARGET ultimate_release_image AND TARGET ultimate_release)
    add_dependencies(ultimate_release_image ultimate_release)

    if(EXISTS "${DLL2EXE}")
        _to_dll2exe_path("${_OUT_ultimate_release}"                    _OUT_RELEASE_ARG)
        _to_dll2exe_path("${CMAKE_BINARY_DIR}/ultimate_release.dll"    _RELEASE_DLL_ARG)
        _to_dll2exe_path("${CMAKE_BINARY_DIR}/ultimate_release.exe"    _RELEASE_EXE_ARG)

        add_custom_command(TARGET ultimate_release_image POST_BUILD
            COMMAND ${_DLL2EXE_RUN} -efix -injimp
                    "${_OUT_RELEASE_ARG}"
                    "${_RELEASE_DLL_ARG}"
                    "${_RELEASE_EXE_ARG}"
            COMMENT "Injecting ultimate_release.dll into ultimate_release_original.exe → ultimate_release.exe"
            VERBATIM)

        # Keep an unambiguous copy of each ABI-specific release. PC and Xbox
        # mash/resource-directory layouts cannot safely coexist in one DLL.
        # The original game also requires its executable basename to remain
        # ultimate_release.exe, so package variants in separate directories
        # instead of renaming the injected image.
        if(ULTIMATE_RELEASE_XBPACK_MODE)
            set(_ULTIMATE_RELEASE_VARIANT_DIR "xbox")
        else()
            set(_ULTIMATE_RELEASE_VARIANT_DIR "pc")
        endif()
        add_custom_command(TARGET ultimate_release_image POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${CMAKE_BINARY_DIR}/ultimate_release_${_ULTIMATE_RELEASE_VARIANT_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_BINARY_DIR}/ultimate_release.dll"
                    "${CMAKE_BINARY_DIR}/ultimate_release_${_ULTIMATE_RELEASE_VARIANT_DIR}/ultimate_release.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_BINARY_DIR}/ultimate_release.exe"
                    "${CMAKE_BINARY_DIR}/ultimate_release_${_ULTIMATE_RELEASE_VARIANT_DIR}/ultimate_release.exe"
            COMMENT "Packaging ultimate_release_${_ULTIMATE_RELEASE_VARIANT_DIR}/ultimate_release.exe"
            VERBATIM)
        usm_pack_exe(ultimate_release_image
            "${CMAKE_BINARY_DIR}/ultimate_release.exe")
        # The image target runs again even when the linker DLL is unchanged.
        # Keep that injection input and the ABI-specific copy uncompressed;
        # modifying the target DLL here makes the next injection embed UPX
        # sections instead of the loader's native code/data sections.
        if(USM_PACK_DLLS)
            add_custom_command(TARGET ultimate_release_image POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:ultimate_release>"
                        "${CMAKE_BINARY_DIR}/ultimate_release_packed.dll"
                COMMENT "Copying retail loader for optional DLL compression"
                VERBATIM)
            usm_pack_dll(ultimate_release_image
                "${CMAKE_BINARY_DIR}/ultimate_release_packed.dll")
        endif()
        message(STATUS "ultimate_release.exe: dll2exe injector : ${DLL2EXE}")
        if(_IS_WSL)
            message(STATUS "ultimate_release.exe: WSL argv paths translated via ${WSLPATH}")
        endif()
    else()
        message(WARNING
            "dll2exe.exe not found at ${DLL2EXE} — ultimate_release.exe will not be produced.")
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

if(TARGET ultimate_prerelease_image AND TARGET ultimate_prerelease)
    add_dependencies(ultimate_prerelease_image ultimate_prerelease)

    # (1) deployable loader: binkw32_prelib.dll → USM_prelib.dll
    add_custom_command(TARGET ultimate_prerelease POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:ultimate_prerelease>"
            "${CMAKE_BINARY_DIR}/USM_prelib.dll"
        COMMENT "Renaming prerelease loader → USM_prelib.dll"
        VERBATIM)

    usm_pack_dll(ultimate_prerelease "${CMAKE_BINARY_DIR}/USM_prelib.dll")

    if(EXISTS "${DLL2EXE}")
        _to_dll2exe_path("${_OUT_Ultimate_prerelease}"                 _OUT_PRE_ARG)
        _to_dll2exe_path("${CMAKE_BINARY_DIR}/USM_prelib_stripped.dll" _PRE_STRIPPED_ARG)
        _to_dll2exe_path("${CMAKE_BINARY_DIR}/Ultimate_prerelease.exe" _PRE_EXE_ARG)

        # (2)+(3) strip a copy of binkw32_prelib.dll, then inject it.
        add_custom_command(TARGET ultimate_prerelease_image POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                    "-DSRC=$<TARGET_FILE:ultimate_prerelease>"
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
        usm_pack_exe(ultimate_prerelease_image "${CMAKE_BINARY_DIR}/Ultimate_prerelease.exe")
        usm_pack_dll(ultimate_prerelease_image "${CMAKE_BINARY_DIR}/USM_prelib_stripped.dll")
        usm_pack_dll(ultimate_prerelease_image "$<TARGET_FILE:ultimate_prerelease>")
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

if(TARGET ultimate_build)
    if(TARGET ultimate_build_original)
        add_dependencies(ultimate_build ultimate_build_original)
    endif()

    # (1) deployable loader: binkw32_build.dll → USM_build.dll
    add_custom_command(TARGET ultimate_build POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:ultimate_build>"
            "${CMAKE_BINARY_DIR}/USM_build.dll"
        COMMENT "Renaming build loader → USM_build.dll"
        VERBATIM)

    usm_pack_dll(ultimate_build "${CMAKE_BINARY_DIR}/USM_build.dll")

    if(EXISTS "${DLL2EXE}")
        _to_dll2exe_path("${_OUT_ultimate_build}"                     _OUT_BUILD_ARG)
        _to_dll2exe_path("${CMAKE_BINARY_DIR}/USM_build_stripped.dll" _BUILD_STRIPPED_ARG)
        _to_dll2exe_path("${CMAKE_BINARY_DIR}/Ultimate_Build.exe"     _BUILD_EXE_ARG)

        # (2)+(3) strip a copy of binkw32_build.dll, then inject it.
        add_custom_command(TARGET ultimate_build POST_BUILD
            COMMAND ${CMAKE_COMMAND}
                    "-DSRC=$<TARGET_FILE:ultimate_build>"
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
        usm_pack_dll(ultimate_build "$<TARGET_FILE:ultimate_build>")
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
#   Runtime tests (ENABLE_TESTS=ON). Run with:  ctest --test-dir build -L runtime -V
#   Each test backs up the install's executable once, deploys the built image,
#   runs it windowed for USM_TEST_SECONDS, terminates only its own PID,
#   restores the executable and fails on a crash or on changed saves/game.ini.
#   Logs and screenshots go to build/runtime-tests/<label>/.
# ─────────────────────────────────────────────────────────────────────────
if(ENABLE_TESTS)
    _to_dll2exe_path("${CMAKE_CURRENT_SOURCE_DIR}/tools/runtime/runtime_test.ps1" _RT_SCRIPT)
    _to_dll2exe_path("${CMAKE_BINARY_DIR}/Ultimate_prerelease.exe" _RT_PRERELEASE_EXE)
    _to_dll2exe_path("${CMAKE_BINARY_DIR}/ultimate_release.exe" _RT_RELEASE_EXE)
    _to_dll2exe_path("${CMAKE_BINARY_DIR}/runtime-tests" _RT_OUT)
    math(EXPR _RT_TIMEOUT "${USM_TEST_SECONDS} + 120")

    # XBPACK v10: boot the June 2005 install into city_arena (Press Start,
    # Continue) and keep it running for USM_TEST_SECONDS.
    add_test(NAME runtime_prerelease_city
        COMMAND "${USM_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass -File "${_RT_SCRIPT}"
                -GameDir "${USM_TEST_GAME_DIR}" -Exe "${_RT_PRERELEASE_EXE}"
                -Label prerelease_city -OutDir "${_RT_OUT}/prerelease_city"
                -Seconds "${USM_TEST_SECONDS}" -ScreenshotEvery 15
                -Keys "25:ENTER,33:ENTER")
    set_tests_properties(runtime_prerelease_city PROPERTIES
        LABELS "runtime;v10" TIMEOUT ${_RT_TIMEOUT} RUN_SERIAL TRUE)

    if(USM_TEST_RELEASE_GAME_DIR)
        add_test(NAME runtime_release_city
            COMMAND "${USM_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass -File "${_RT_SCRIPT}"
                    -GameDir "${USM_TEST_RELEASE_GAME_DIR}" -Exe "${_RT_RELEASE_EXE}"
                    -Label release_city -OutDir "${_RT_OUT}/release_city"
                    -Seconds "${USM_TEST_SECONDS}" -ScreenshotEvery 15
                    -Keys "25:ENTER,33:ENTER")
        set_tests_properties(runtime_release_city PROPERTIES
            LABELS "runtime;release" TIMEOUT ${_RT_TIMEOUT} RUN_SERIAL TRUE)
    endif()
endif()


# ─────────────────────────────────────────────────────────────────────────
#   Optional deployment into ../Ultimate Spider-Man/. Disabled by default so
#   normal builds cannot overwrite a game installation unexpectedly.
# ─────────────────────────────────────────────────────────────────────────

option(USM_AUTO_DEPLOY "Copy built artifacts into ../Ultimate Spider-Man when that folder exists" OFF)

set(_GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Ultimate Spider-Man")
if(USM_AUTO_DEPLOY AND EXISTS "${_GAME_DIR}/")

    # --- USM ---
    if(TARGET USM)
        add_custom_command(TARGET USM POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${_OUT_USM}" "${_GAME_DIR}/USM_original.exe"
            COMMENT "Deploying USM_original.exe to ${_GAME_DIR}"
            VERBATIM)
        if(EXISTS "${DLL2EXE}" AND TARGET USM)
            add_custom_command(TARGET USM POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_BINARY_DIR}/USM.exe" "${_GAME_DIR}/USM.exe"
                COMMENT "Deploying USM.exe to ${_GAME_DIR}"
                VERBATIM)
        endif()
    endif()

    # --- release loader binkw32.dll ---
    if(TARGET USM)
        add_custom_command(TARGET USM POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:USM>" "${_GAME_DIR}/binkw32.dll"
            COMMENT "Deploying binkw32.dll to ${_GAME_DIR}"
            VERBATIM)
    endif()

    # --- prerelease loader, deployed under the renamed USM_prelib.dll ---
    if(TARGET ultimate_prerelease)
        add_custom_command(TARGET ultimate_prerelease POST_BUILD
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
        if(EXISTS "${DLL2EXE}" AND TARGET USM)
            add_custom_command(TARGET USMITA POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_BINARY_DIR}/USMITA.exe" "${_GAME_DIR}/USMITA.exe"
                COMMENT "Deploying USMITA.exe to ${_GAME_DIR}"
                VERBATIM)
        endif()
    endif()

    # --- Ultimate_prerelease ---
    if(TARGET ultimate_prerelease_image)
        add_custom_command(TARGET ultimate_prerelease_image POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${_OUT_Ultimate_prerelease}" "${_GAME_DIR}/Ultimate_prerelease_original.exe"
            COMMENT "Deploying Ultimate_prerelease_original.exe to ${_GAME_DIR}"
            VERBATIM)
        if(EXISTS "${DLL2EXE}" AND TARGET ultimate_prerelease)
            add_custom_command(TARGET ultimate_prerelease_image POST_BUILD
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
        if(EXISTS "${DLL2EXE}" AND TARGET ultimate_build)
            add_custom_command(TARGET ultimate_build POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_BINARY_DIR}/Ultimate_Build.exe" "${_GAME_DIR}/Ultimate_Build.exe"
                COMMENT "Deploying Ultimate_Build.exe to ${_GAME_DIR}"
                VERBATIM)
        endif()
    endif()

    # --- build loader, deployed under the renamed USM_build.dll ---
    if(TARGET ultimate_build)
        add_custom_command(TARGET ultimate_build POST_BUILD
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

if(EXISTS "${DLL2EXE}" AND TARGET USM)
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
