#!/bin/sh
# Reduced motion, velocity tracking, SVG path import and text scramble.
set -e
cd "$(dirname "$0")/../.."
IMGUI=${IMGUI_DIR:-examples/extern/imgui}
CXX=${CXX:-g++}
OUT=${OUT_DIR:-build/utils}
mkdir -p "$OUT"
$CXX -std=c++11 -O1 -Wno-unused-result -I"$IMGUI" -I. tests/utils/utils_test.cpp im_anim.cpp \
	"$IMGUI/imgui.cpp" "$IMGUI/imgui_draw.cpp" "$IMGUI/imgui_tables.cpp" "$IMGUI/imgui_widgets.cpp" -o "$OUT/utils_test"
"$OUT/utils_test"
