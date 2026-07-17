#!/usr/bin/env bash
# Renders the logo at macOS icon sizes and packs them into JsonStudio.icns.
set -euo pipefail
cd "$(dirname "$0")"

RSVG="${RSVG:-rsvg-convert}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

# Small sizes use the compact variant so the mark stays legible.
"$RSVG" -w 16   -h 16   01-classic-compact.svg -o "$tmp/m16.png"
"$RSVG" -w 32   -h 32   01-classic-compact.svg -o "$tmp/m32.png"
"$RSVG" -w 64   -h 64   01-classic.svg -o "$tmp/m64.png"
"$RSVG" -w 128  -h 128  01-classic.svg -o "$tmp/m128.png"
"$RSVG" -w 256  -h 256  01-classic.svg -o "$tmp/m256.png"
"$RSVG" -w 512  -h 512  01-classic.svg -o "$tmp/m512.png"
"$RSVG" -w 1024 -h 1024 01-classic.svg -o "$tmp/m1024.png"

python3 make-icns.py "$tmp" JsonStudio.icns
