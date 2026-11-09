# NFSU2 behavior-equivalent rewrite.
#   game/              platform-agnostic rewritten game code (static library)
#   host/win32-hook/   dinput8.dll proxy that runs game/ inside the original PC SPEED2.EXE (32-bit Windows only)
#   tests/             native unit tests for game/ (stub bridge)
#   tests/win32/       differential referee: original exe functions vs rewrites, under Wine
#
# Native tests:  cmake -B build-native -G Ninja && cmake --build build-native && ctest --test-dir build-native
# Win32 build:   cmake -B build-win32 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/i686-w64-mingw32.cmake && cmake --build build-win32
# Referee:       tests/win32/run_referee.sh
cmake_minimum_required(VERSION 3.20)
project(nfsu2port CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_options(-Wall -Wextra -Wno-unused-parameter)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS game/*.cpp)
add_library(nfsu2game STATIC ${GAME_SOURCES})
target_include_directories(nfsu2game PUBLIC game)

if(WIN32)
  add_library(dinput8 SHARED host/win32-hook/dinput8_proxy.cpp host/win32-hook/hooks.cpp
                             host/win32-hook/replacements.cpp host/win32-hook/bridge_win32.cpp)
  target_link_libraries(dinput8 PRIVATE nfsu2game)
  set_target_properties(dinput8 PROPERTIES PREFIX "" OUTPUT_NAME "dinput8")
  # undecorated stdcall export name, self-contained runtime (no extra DLLs next to the game)
  target_link_options(dinput8 PRIVATE -Wl,--kill-at -static)

  # referee: a dinput8.dll that compares original exe functions with rewrites, then exits (see run_referee.sh)
  add_library(referee SHARED tests/win32/referee.cpp tests/bridge_stub.cpp)
  target_include_directories(referee PRIVATE tests)
  target_link_libraries(referee PRIVATE nfsu2game)
  set_target_properties(referee PROPERTIES PREFIX "" OUTPUT_NAME "dinput8" LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/referee
                                           RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/referee
                                           ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/referee)
  target_link_options(referee PRIVATE -Wl,--kill-at -static)
else()
  enable_testing()
  add_executable(test_bstring tests/test_bstring.cpp tests/bridge_stub.cpp)
  add_executable(test_frontend tests/test_frontend.cpp tests/bridge_stub.cpp)
  foreach(t test_bstring test_frontend)
    target_include_directories(${t} PRIVATE tests)
    target_link_libraries(${t} PRIVATE nfsu2game)
  endforeach()
  add_test(NAME bstring COMMAND test_bstring ${CMAKE_SOURCE_DIR}/tests/data/nfsu2re_hashes.txt)
  add_test(NAME frontend COMMAND test_frontend)
endif()
