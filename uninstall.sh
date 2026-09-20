#!/usr/bin/env bash
set -euo pipefail

prefix="${PREFIX:-/usr/local}"

rm -f "$prefix/bin/imageviewer"
rm -f "$prefix/share/applications/imageviewer.desktop"
rm -f "$prefix/share/mime/packages/imageviewer-mime.xml"
rm -f "$prefix/share/icons/hicolor/scalable/apps/imageviewer.svg"
rm -f "$prefix/share/doc/imageviewer/README.md"

if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "$prefix/share/applications" || true
fi
if command -v update-mime-database >/dev/null 2>&1; then
  update-mime-database "$prefix/share/mime" || true
fi

echo "Uninstalled imageviewer from $prefix"
echo "User configuration was preserved at ~/.config/imageviewer"
