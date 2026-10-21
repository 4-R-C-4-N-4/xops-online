cmake_minimum_required(VERSION 3.16)
project(OpenXOPS-web LANGUAGES CXX)

# OpenXOPS-web native build.
# Phase 0: upstream game logic + stub platform classes (no window, input,
# sound or rendering). Real SDL2/OpenAL/GLES backends come in Phase 1.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(OPENXOPS_SANITIZE "Build with -fsanitize=address,undefined" OFF)
option(OPENXOPS_AUTOCREATECONFIG "Define ENABLE_AUTOCREATECONFIG (write a default config.dat if missing)" OFF)
option(OPENXOPS_PATH_DELIMITER_SLASH "Define ENABLE_PATH_DELIMITER_SLASH (convert '\\\\' to '/' in data paths)" ON)
set(OPENXOPS_PLATFORM "" CACHE STRING "Shortcut: 'sdl' selects sdl/sdl/openal/gles for all components")

if(WIN32)
  message(FATAL_ERROR "This CMake build targets non-Windows platforms. Build upstream Windows "
                      "backends with the upstream Visual C++ project (add src/platform/common.cpp "
                      "and src/platform/input-common.cpp to it).")
endif()

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Upstream game logic (platform-neutral). Not compiled here: d3dgraphics-directx.cpp,
# d3dgraphics-opengl.cpp, sound-ezds.cpp, sound-directsound.cpp, input.cpp, window.cpp.
set(OPENXOPS_GAME_SOURCES
  ${SRC}/ai.cpp
  ${SRC}/collision.cpp
  ${SRC}/config.cpp
  ${SRC}/d3dgraphics-debugfont.cpp
  ${SRC}/datafile.cpp
  ${SRC}/debug.cpp
  ${SRC}/event.cpp
  ${SRC}/gamemain.cpp
  ${SRC}/gui-object.cpp
  ${SRC}/main.cpp
  ${SRC}/object.cpp
  ${SRC}/objectmanager.cpp
  ${SRC}/parameter.cpp
  ${SRC}/resource.cpp
  ${SRC}/scene.cpp
  ${SRC}/soundmanager.cpp
  ${SRC}/statemachine.cpp
)
# XOPS ONLINE network modules
file(GLOB OPENXOPS_NET_SOURCES CONFIGURE_DEPENDS ${SRC}/net/*.cpp)
list(APPEND OPENXOPS_GAME_SOURCES ${OPENXOPS_NET_SOURCES})

# Upstream helpers moved out of window.cpp / input.cpp (shared by all backends).
set(OPENXOPS_COMMON_SOURCES
  ${SRC}/platform/common.cpp
  ${SRC}/platform/input-common.cpp
)

# Platform backends are chosen per component. Each component's sources live in
# src/platform/<backend>/ and are wired up by a module in cmake/modules/*.cmake
# (one file per component or feature, so parallel work never edits the same CMake file).
#   OPENXOPS_WINDOW_BACKEND  stub | sdl      (WindowControl + main loop + Platform_GL*)
#   OPENXOPS_INPUT_BACKEND   stub | sdl      (InputControl)
#   OPENXOPS_SOUND_BACKEND   stub | openal   (SoundControl)
#   OPENXOPS_GFX_BACKEND     stub | gles     (D3DGraphics)
set(OPENXOPS_WINDOW_BACKEND "stub" CACHE STRING "WindowControl backend")
set(OPENXOPS_INPUT_BACKEND  "stub" CACHE STRING "InputControl backend")
set(OPENXOPS_SOUND_BACKEND  "stub" CACHE STRING "SoundControl backend")
set(OPENXOPS_GFX_BACKEND    "stub" CACHE STRING "D3DGraphics backend")
# Back-compat with Phase 0: OPENXOPS_PLATFORM=stub|sdl sets all four.
if(OPENXOPS_PLATFORM STREQUAL "sdl")
  set(OPENXOPS_WINDOW_BACKEND sdl CACHE STRING "" FORCE)
  set(OPENXOPS_INPUT_BACKEND sdl CACHE STRING "" FORCE)
  set(OPENXOPS_SOUND_BACKEND openal CACHE STRING "" FORCE)
  set(OPENXOPS_GFX_BACKEND gles CACHE STRING "" FORCE)
endif()

add_executable(openxops
  ${OPENXOPS_GAME_SOURCES}
  ${OPENXOPS_COMMON_SOURCES}
)

# Stub backends (built-in so a bare configure always links).
if(OPENXOPS_WINDOW_BACKEND STREQUAL "stub")
  target_sources(openxops PRIVATE ${SRC}/platform/stub/window-stub.cpp)
endif()
if(OPENXOPS_INPUT_BACKEND STREQUAL "stub")
  target_sources(openxops PRIVATE ${SRC}/platform/stub/input-stub.cpp)
endif()
if(OPENXOPS_SOUND_BACKEND STREQUAL "stub")
  target_sources(openxops PRIVATE ${SRC}/platform/stub/sound-stub.cpp)
endif()
if(OPENXOPS_GFX_BACKEND STREQUAL "stub")
  target_sources(openxops PRIVATE ${SRC}/platform/stub/d3dgraphics-stub.cpp)
endif()
target_compile_definitions(openxops PRIVATE OPENXOPS_PLATFORM_STUB)

enable_testing()
file(GLOB OPENXOPS_CMAKE_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/*.cmake)
list(SORT OPENXOPS_CMAKE_MODULES)
foreach(_m ${OPENXOPS_CMAKE_MODULES})
  include(${_m})
endforeach()

target_include_directories(openxops PRIVATE ${SRC})
target_compile_definitions(openxops PRIVATE ${OPENXOPS_PLATFORM_DEFS})
if(OPENXOPS_AUTOCREATECONFIG)
  target_compile_definitions(openxops PRIVATE ENABLE_AUTOCREATECONFIG)
endif()
# VFS must receive original CP932 bytes; blanket slash conversion can rewrite
# a trail byte. Keep the legacy option accepted but do not define it for VFS paths.

target_compile_options(openxops PRIVATE -Wall -Wextra)
target_link_libraries(openxops PRIVATE m)

if(OPENXOPS_SANITIZE)
  target_compile_options(openxops PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
  target_link_options(openxops PRIVATE -fsanitize=address,undefined)
endif()
