#!/usr/bin/env bash
set -euo pipefail

here="$(cd "$(dirname "$0")/.." && pwd)"
cd "$here"

n=${SPIKE_N:-1000}
budget_us=${SPIKE_BUDGET_US:-5000}
out=${SPIKE_OUT:-spike-results.md}

if [[ ! -d build ]]; then
    meson setup build
fi
ninja -C build

if ! ./build/reaforge_host_main >/dev/null; then
    echo "spike: host smoke test failed" >&2
    exit 1
fi

./build/reaforge_bench --n="$n" --budget-us="$budget_us" --out="$out"
cat "$out"
