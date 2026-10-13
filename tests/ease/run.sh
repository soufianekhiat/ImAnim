#!/bin/sh
# Curve eases, parametric clip eases, ease editor.
set -e
cd "$(dirname "$0")/../.."
IMGUI=${IMGUI_DIR:-examples/extern/imgui}
CXX=${CXX:-g++}
OUT=${OUT_DIR:-build/ease}
mkdir -p "$OUT"
$CXX -std=c++11 -O1 -Wno-unused-result -I"$IMGUI" -I. tests/ease/ease_test.cpp im_anim.cpp \
	"$IMGUI/imgui.cpp" "$IMGUI/imgui_draw.cpp" "$IMGUI/imgui_tables.cpp" "$IMGUI/imgui_widgets.cpp" -o "$OUT/ease_test"
(cd "$OUT" && ./ease_test)
