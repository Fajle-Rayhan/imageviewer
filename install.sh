#!/usr/bin/env bash
set -euo pipefail

prefix="${PREFIX:-/usr/local}"
build_dir="${BUILD_DIR:-build}"

cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix"
cmake --build "$build_dir" -j"$(nproc)"
cmake --install "$build_dir"

if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "$prefix/share/applications" || true
fi
if command -v update-mime-database >/dev/null 2>&1; then
  update-mime-database "$prefix/share/mime" || true
fi
if command -v xdg-mime >/dev/null 2>&1; then
  xdg-mime default imageviewer.desktop image/png image/jpeg image/webp image/gif image/bmp image/svg+xml image/tiff || true
fi

echo "Installed imageviewer to $prefix"
