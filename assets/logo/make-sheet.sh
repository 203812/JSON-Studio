#!/usr/bin/env bash
# Assembles all NN-*.svg candidates into one comparison sheet and renders it.
# Each cell shows the mark at 256px plus 48/24px previews for small-size legibility.
set -euo pipefail
cd "$(dirname "$0")"

RSVG="/c/msys64/mingw64/bin/rsvg-convert.exe"
cols=3
cellw=360
cellh=380
pad=30

files=(0*-*.svg)
n=${#files[@]}
rows=$(( (n + cols - 1) / cols ))
W=$(( cols * cellw + pad ))
H=$(( rows * cellh + pad + 20 ))

out=_sheet.svg
{
  echo "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"$W\" height=\"$H\" viewBox=\"0 0 $W $H\">"
  echo "<rect width=\"$W\" height=\"$H\" fill=\"#141510\"/>"
  i=0
  for f in "${files[@]}"; do
    col=$(( i % cols )); row=$(( i / cols ))
    x=$(( pad + col * cellw )); y=$(( pad + row * cellh ))
    label="${f%.svg}"
    # Strip the outer <?xml> and <svg ...> wrapper, keep inner drawing.
    body=$(sed -e '1{/<?xml/d}' -e 's#<svg[^>]*>##' -e 's#</svg>##' "$f")
    # Main 256px mark
    echo "<svg x=\"$x\" y=\"$y\" width=\"256\" height=\"256\" viewBox=\"0 0 512 512\">$body</svg>"
    # 48px and 24px previews stacked to the right
    echo "<svg x=\"$((x+276))\" y=\"$((y+40))\" width=\"48\" height=\"48\" viewBox=\"0 0 512 512\">$body</svg>"
    echo "<text x=\"$((x+300))\" y=\"$((y+108))\" fill=\"#75715E\" font-family=\"sans-serif\" font-size=\"13\" text-anchor=\"middle\">48px</text>"
    echo "<svg x=\"$((x+288))\" y=\"$((y+130))\" width=\"24\" height=\"24\" viewBox=\"0 0 512 512\">$body</svg>"
    echo "<text x=\"$((x+300))\" y=\"$((y+170))\" fill=\"#75715E\" font-family=\"sans-serif\" font-size=\"13\" text-anchor=\"middle\">24px</text>"
    echo "<text x=\"$((x+128))\" y=\"$((y+296))\" fill=\"#F8F8F2\" font-family=\"sans-serif\" font-size=\"22\" text-anchor=\"middle\">$label</text>"
    i=$(( i + 1 ))
  done
  echo "</svg>"
} > "$out"

"$RSVG" -w $((W*2)) -h $((H*2)) "$out" -o sheet.png
echo "sheet.png ($(($W*2))x$(($H*2)))"
