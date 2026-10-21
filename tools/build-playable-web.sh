#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
if ! command -v em++ >/dev/null; then source "$HOME/programs/emsdk/emsdk_env.sh"; fi
mkdir -p build-web
sources=()
if [[ "${OPENXOPS_TEST_HOOKS:-0}" == 1 ]]; then
  sources+=(tests/browser/qa_hooks.cpp)
fi
for n in ai collision config d3dgraphics-debugfont datafile debug event gamemain gui-object main object objectmanager parameter resource scene soundmanager statemachine; do sources+=("src/$n.cpp"); done
# XOPS ONLINE: network modules (protocol, session, exports) are picked up by glob.
shopt -s nullglob; for f in src/net/*.cpp; do sources+=("$f"); done; shopt -u nullglob
em++ -std=c++17 -O1 -g2 -Isrc -DGRAPHIC_ENGINE=8 -DINPUT_INTERFACE=3 -DSOUND_ENGINE=3 \
 -DOPENXOPS_PORTABLE_GL -DOPENXOPS_WINDOW_SDL -DOPENXOPS_INPUT_SDL -DOPENXOPS_SOUND_OPENAL -DENABLE_AUTOCREATECONFIG \
 "${sources[@]}" src/platform/common.cpp src/platform/input-common.cpp src/platform/vfs.cpp \
 src/platform/sdl/window-sdl.cpp src/platform/sdl/input-sdl.cpp src/platform/openal/sound-openal.cpp \
 src/platform/portable/d3dgraphics-portable.cpp src/platform/portable/browser-archive.cpp \
 src/platform/portable/browser-controls.cpp src/platform/text/text.cpp \
 -sUSE_SDL=2 -sUSE_ZLIB=1 -lopenal -sLEGACY_GL_EMULATION=1 -sGL_UNSAFE_OPTS=0 -sALLOW_MEMORY_GROWTH=1 \
 -sINITIAL_MEMORY=128MB -sSTACK_SIZE=8MB -sEXPORTED_RUNTIME_METHODS=FS,callMain,UTF8ToString,HEAPU8,HEAP32 -sINVOKE_RUN=0 \
 -sASSERTIONS=1 -sEXIT_RUNTIME=0 -o build-web/openxops.js
rm -f build-web/openxops-data.*.zip
python3 tools/build_pack.py --placeholders --out build-web
python3 tools/package_web.py
cp LICENSE build-web/LICENSE
# XOPS ONLINE: network modules (ES modules) and vendored libraries
rm -rf build-web/net build-web/vendor
cp -r web/net build-web/net
cp -r web/vendor build-web/vendor
