#!/usr/bin/env bash
# Build the ReaForge host on Linux and copy the .so into REAPER-Linux's
# UserPlugins directory. REAPER picks it up on next launch.
#
# Prerequisite: run scripts/install-build-deps.sh once. If you compiled
# Lua from source into ~/.local (the no-sudo fallback), the script
# already points meson at it; otherwise it finds liblua5.4-dev via
# pkg-config.

set -euo pipefail

here="$(cd "$(dirname "$0")/.." && pwd)"
cd "$here"

reaper_resource_dir="${REAPER_RESOURCE_DIR:-$HOME/.config/REAPER}"
reaper_userplugins="$reaper_resource_dir/UserPlugins"

if [[ ! -d "$reaper_resource_dir" ]]; then
    echo "REAPER resource dir not found at $reaper_resource_dir" >&2
    echo "Override with REAPER_RESOURCE_DIR=/path/to/REAPER" >&2
    exit 1
fi

echo "==[1]== meson setup"
meson setup build --reconfigure 2>&1 | tail -3
ninja -C build 2>&1 | tail -3

echo
echo "==[2]== copy .so into REAPER UserPlugins"
mkdir -p "$reaper_userplugins"
cp build/src/host/libreaper_reaforge_host.so "$reaper_userplugins/"
ls -la "$reaper_userplugins/libreaper_reaforge_host.so"

echo
echo "==[3]== launch REAPER"
reaper_cmd="${REAPER_BIN:-$(command -v reaper || true)}"
if [[ -z "$reaper_cmd" ]]; then
    echo "REAPER not found in PATH. Launch it manually with:"
    echo "  reaper"
    echo "Once it opens, the Extensions Manager panel appears in the bottom dock."
    exit 0
fi
echo "Starting: $reaper_cmd"
"$reaper_cmd" &
echo "REAPER is starting. Once it opens, the bottom dock has the 'ReaForge' panel."
