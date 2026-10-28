#!/usr/bin/env bash
# Build and install RAINFALL into ~/.local (no sudo). Pass "uninstall" to remove.
set -euo pipefail
cd "$(dirname "$0")"

if [[ "${1:-}" == "uninstall" ]]; then
    exec make uninstall
fi

for cmd in make g++ pkg-config; do
    command -v "$cmd" >/dev/null || { echo "missing: $cmd  (sudo pacman -S --needed base-devel)"; exit 1; }
done
pkg-config --exists ncursesw || { echo "missing ncursesw  (sudo pacman -S --needed ncurses)"; exit 1; }

make
make install

case ":$PATH:" in
    *":$HOME/.local/bin:"*) ;;
    *) echo "note: add ~/.local/bin to your PATH" ;;
esac
echo "run: rainfall   (or find 'Rainfall' in the app launcher)"
