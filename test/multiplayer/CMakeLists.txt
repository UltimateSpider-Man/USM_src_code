cmake_minimum_required(VERSION 3.16)
project(usm_multiplayer_checks LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(usm_multiplayer_tests test_match.cpp)
target_include_directories(usm_multiplayer_tests PRIVATE ../../src)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(usm_multiplayer_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
enable_testing()
add_test(NAME multiplayer_rules_geometry_protocol COMMAND usm_multiplayer_tests)
# This adapter exercises the real production transport with POSIX sockets.
# It does not compile or validate the Windows game/renderer ABI.
if(UNIX)
  add_executable(usm_multiplayer_lan_tests test_lan.cpp ../../src/multiplayer_lan.cpp)
  target_include_directories(usm_multiplayer_lan_tests PRIVATE ../../src .)
  target_compile_definitions(usm_multiplayer_lan_tests PRIVATE USM_MULTIPLAYER_NET_TEST=1)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(usm_multiplayer_lan_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME multiplayer_tcp_loopback COMMAND usm_multiplayer_lan_tests)
  set_tests_properties(multiplayer_tcp_loopback PROPERTIES TIMEOUT 45)
endif()

add_executable(usm_multiplayer_gamepad_tests test_gamepad.cpp)
target_include_directories(usm_multiplayer_gamepad_tests PRIVATE ../../src)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(usm_multiplayer_gamepad_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME multiplayer_gamepad_mapping_state COMMAND usm_multiplayer_gamepad_tests)

# Historical arena orchestration retained as a regression fixture only.
# The shipped online mode is exercised by test/online, not by this legacy test.
# This does not verify Win32 SDK declarations, native hooking or actual rendering.
add_executable(usm_multiplayer_mode_tests test_mode.cpp)
target_include_directories(usm_multiplayer_mode_tests PRIVATE ../../src .)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(usm_multiplayer_mode_tests PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-misleading-indentation)
endif()
add_test(NAME legacy_arena_mode_controller_flow COMMAND usm_multiplayer_mode_tests)

# Exercises the selected production arena controller, independently of the
# historical fixture. Native assets/rendering and OS input are explicit fakes.
add_executable(usm_multiplayer_arena_tests test_arena_mode.cpp)
target_include_directories(usm_multiplayer_arena_tests PRIVATE ../../src .)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(usm_multiplayer_arena_tests PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-misleading-indentation)
endif()
add_test(NAME production_nds_arena_controller_flow COMMAND usm_multiplayer_arena_tests)
