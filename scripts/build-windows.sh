#!/usr/bin/env bash
# Cross-compile the ReaForge host to a Windows .dll using mingw-w64.
# The output goes to build-windows/reaper_reaforge_host.dll.
# Copy it manually to %APPDATA%\\REAPER\\UserPlugins\\ on the Windows host.
#
# Prerequisite: mingw-w64 from scripts/install-build-deps.sh.

set -euo pipefail

here="$(cd "$(dirname "$0")/.." && pwd)"
cd "$here"

mingw_gxx="${MINGW_GXX:-x86_64-w64-mingw32-g++}"
mingw_lua_dir="${MINGW_LUA_DIR:-/usr/x86_64-w64-mingw32}"

if ! command -v "$mingw_gxx" >/dev/null 2>&1; then
    echo "$mingw_gxx not found. Run scripts/install-build-deps.sh first." >&2
    exit 1
fi

echo "==[1]== cross-compile with mingw-w64"
echo "NOTE: REAPER Windows plugins require MSVC ABI; mingw may produce a"
echo "non-loadable .dll. If REAPER does not list the host after restart,"
echo "the recommended path is to build with Visual Studio Build Tools on"
echo "Windows: vcvarsall.bat amd64 && meson setup build && ninja -C build"
echo

mkdir -p build-windows
"$mingw_gxx" -shared -o build-windows/reaper_reaforge_host.dll \
    -fPIC -O2 -std=c++17 \
    -I "$mingw_lua_dir/include" \
    -I third_party/quickjs-ng \
    -I third_party/reaper-sdk/sdk \
    -D_GNU_SOURCE -DCONFIG_LINUX \
    -Wl,--out-implib,build-windows/libreaforge.a \
    src/host/host.cpp \
    src/host/panel.cpp \
    src/host/context_menu.cpp \
    src/host/extension_loader.cpp \
    src/host/executor.cpp \
    src/host/runtime/lua_runtime.cpp \
    src/host/runtime/quickjs_runtime.cpp \
    src/host/runtime/jsfx_runtime.cpp \
    src/host/bridge/bridge.cpp \
    src/host/bridge/bridge_lua.cpp \
    src/host/bridge/bridge_quickjs.cpp \
    src/host/bridge/bridge_jsfx.cpp \
    third_party/quickjs-ng/quickjs.c \
    third_party/quickjs-ng/quickjs-libc.c \
    third_party/quickjs-ng/libregexp.c \
    third_party/quickjs-ng/libunicode.c \
    third_party/quickjs-ng/dtoa.c \
    -L "$mingw_lua_dir/lib" -llua \
    -lws2_32 -lwinmm -lole32 -luser32 -lkernel32 \
    2>&1 | tail -10

ls -la build-windows/reaper_reaforge_host.dll 2>&1
echo
echo "If a .dll was produced, copy it to:"
echo "  C:\\Users\\<user>\\AppData\\Roaming\\REAPER\\UserPlugins\\"
echo "from the Windows host."
