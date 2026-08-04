#!/usr/bin/env bash
#
# Rebuilds the interface typeface for all three parts of Oh-a-synth.
#
# Archivo (SIL OFL 1.1, Omnibus-Type) is the face the product site uses. This
# pulls the upstream variable fonts, pins them to width 100, and writes:
#
#   css/fonts.css                     web app  — both faces inlined as base64
#   assets/fonts/Archivo-*.ttf        native   — statics compiled into the binary
#   assets/fonts/OFL.txt              the licence, which ships with the fonts
#
# Nothing here runs at build time. The outputs are committed, so the web app
# still opens with zero build steps and CMake just reads the .ttf files. Run
# this only when the typeface itself needs to change.
#
# Requires python3 (for fontTools, installed into a throwaway venv) and curl.
set -euo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# The characters the two panels actually draw: Latin, plus the typographic
# punctuation used in the legends. Waveform glyphs (⊓ ⊿) are not in Archivo
# and fall through to the system font by design.
unicodes="U+0020-007E,U+00A0,U+00B0,U+00B7,U+00D7,U+2013,U+2014,U+2018,U+2019,U+201C,U+201D,U+2026,U+2039,U+203A,U+2191,U+2193"
features_web="kern,liga,calt,ccmp,locl,mark,mkmk,rvrn,tnum,case"
features_ttf="kern,liga,calt,ccmp,locl"

upstream="https://raw.githubusercontent.com/google/fonts/main/ofl/archivo"

echo "==> fontTools"
python3 -m venv "$work/venv"
"$work/venv/bin/pip" -q install fonttools brotli
py="$work/venv/bin/python"
subset="$work/venv/bin/pyftsubset"

echo "==> upstream Archivo"
curl -sfL -o "$work/roman.ttf"  "$upstream/Archivo%5Bwdth%2Cwght%5D.ttf"
curl -sfL -o "$work/italic.ttf" "$upstream/Archivo-Italic%5Bwdth%2Cwght%5D.ttf"
curl -sfL -o "$repo/assets/fonts/OFL.txt" "$upstream/OFL.txt"

# ---- web: variable weight, one width, inlined ---------------------------------
echo "==> css/fonts.css"
for face in roman italic; do
  "$py" -m fontTools.varLib.instancer "$work/$face.ttf" wdth=100 wght=300:800 \
        -o "$work/$face-var.ttf" >/dev/null
  "$subset" "$work/$face-var.ttf" --unicodes="$unicodes" \
        --layout-features="$features_web" --name-IDs='*' --name-legacy \
        --notdef-outline --flavor=woff2 --output-file="$work/$face.woff2"
done

REPO="$repo" WORK="$work" "$py" - <<'PY'
import base64, os, pathlib
work = pathlib.Path(os.environ['WORK']); repo = pathlib.Path(os.environ['REPO'])
faces = {s: base64.b64encode((work / f'{f}.woff2').read_bytes()).decode()
         for s, f in (('normal', 'roman'), ('italic', 'italic'))}
head = """/* Oh-a-synth — interface typeface.
 *
 * Archivo (SIL OFL 1.1, Omnibus-Type) — the same face the product site uses,
 * so the app and the site read as one thing. See assets/fonts/OFL.txt.
 *
 * The font is inlined as base64 rather than linked: browsers treat a page
 * opened straight from disk (file://) as an opaque origin and refuse the
 * font fetch, and opening index.html with no server is a supported way to
 * run this app. A data: URI is same-origin everywhere.
 *
 * GENERATED — do not hand-edit. Regenerate with tools/make-fonts.sh, which
 * also rebuilds the .ttf files the native builds embed. Subset to Latin at
 * width 100, weights 300-800: ~%d KB of CSS for both faces.
 */
""" % (sum(len(v) for v in faces.values()) // 1024)
face = """
@font-face {
  font-family: 'Archivo';
  src: url(data:font/woff2;charset=utf-8;base64,%s) format('woff2');
  font-weight: 300 800;
  font-style: %s;
  font-display: swap;
}
"""
(repo / 'css/fonts.css').write_text(head + ''.join(face % (b64, style)
                                                  for style, b64 in faces.items()))
PY

# ---- native: static instances, named so JUCE reports them sanely --------------
echo "==> assets/fonts/*.ttf"
make_static () { # source weight output-stem subfamily
  "$py" -m fontTools.varLib.instancer "$work/$1.ttf" "wght=$2" wdth=100 \
        -o "$work/$3-inst.ttf" >/dev/null 2>&1
  "$subset" "$work/$3-inst.ttf" --unicodes="$unicodes" \
        --layout-features="$features_ttf" --name-IDs='*' --name-legacy \
        --notdef-outline --output-file="$repo/assets/fonts/$3.ttf"
  FONT="$repo/assets/fonts/$3.ttf" SUB="$4" STEM="$3" "$py" - <<'PY'
import os
from fontTools.ttLib import TTFont
f = TTFont(os.environ['FONT']); n = f['name']
sub, stem = os.environ['SUB'], os.environ['STEM']
for rec in list(n.names):
    if rec.nameID in (1, 2, 3, 4, 6, 16, 17):
        n.removeNames(rec.nameID, rec.platformID, rec.platEncID, rec.langID)
for pid, eid, lid in ((3, 1, 0x409), (1, 0, 0)):
    n.setName('Archivo', 1, pid, eid, lid)
    n.setName(sub, 2, pid, eid, lid)
    n.setName(f'2.001;Oha;{stem}', 3, pid, eid, lid)
    n.setName(f'Archivo {sub}', 4, pid, eid, lid)
    n.setName(stem, 6, pid, eid, lid)
f.save(os.environ['FONT'])
PY
}

# Regular and Bold carry the panel; Bold Italic is the wordmark, at the same
# 800 the site sets on its own brand line.
make_static roman  400 Archivo-Regular    "Regular"
make_static roman  700 Archivo-Bold       "Bold"
make_static italic 800 Archivo-BoldItalic "Bold Italic"

ls -l "$repo/assets/fonts" "$repo/css/fonts.css"
echo "done"
