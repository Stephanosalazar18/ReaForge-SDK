#!/usr/bin/env bash
# Install everything ReaForge needs to build and run on this WSL2 host.
# Requires sudo. After this runs you can:
#   ./scripts/build-and-load-linux.sh
# to compile the host and copy the .so into REAPER's user plugins dir.

set -euo pipefail

if ! command -v sudo >/dev/null; then
    echo "sudo not found; install the packages manually as root." >&2
    exit 1
fi

sudo apt update
sudo apt install -y \
    build-essential \
    g++ \
    meson \
    ninja-build \
    pkg-config \
    liblua5.4-dev \
    libasound2-dev \
    libjack-jackd2-dev \
    libgtk-3-dev \
    libx11-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxext-dev \
    wine64 \
    mingw-w64 \
    curl \
    xz-utils

echo
echo "All deps installed. You can now run:"
echo "  ./scripts/build-and-load-linux.sh    # build host + load into REAPER-Linux"
echo "  ./scripts/build-windows.sh           # cross-compile to .dll for REAPER-Windows"
