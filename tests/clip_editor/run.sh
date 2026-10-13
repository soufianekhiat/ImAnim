#!/bin/sh
# Drives the clip editor headlessly with simulated input, then checks that the
# code it exports ("Copy C++") compiles.
set -e
cd "$(dirname "$0")/../.."
IMGUI=${IMGUI_DIR:-examples/extern/imgui}
CXX=${CXX:-g++}
OUT=${OUT_DIR:-build/clip_editor}
mkdir -p "$OUT"
$CXX -std=c++11 -O1 -Wno-unused-result -I"$IMGUI" -I. tests/clip_editor/editor_test.cpp im_anim.cpp \
	"$IMGUI/imgui.cpp" "$IMGUI/imgui_draw.cpp" "$IMGUI/imgui_tables.cpp" "$IMGUI/imgui_widgets.cpp" -o "$OUT/editor_test"
(cd "$OUT" && ./editor_test)
$CXX -std=c++11 -fsyntax-only -I"$IMGUI" -I. -I"$OUT" tests/clip_editor/compile_export.cpp
echo "OK: exported clip code compiles"
