#!/bin/sh
# Spring channels and tween state queries.
set -e
cd "$(dirname "$0")/../.."
IMGUI=${IMGUI_DIR:-examples/extern/imgui}
CXX=${CXX:-g++}
OUT=${OUT_DIR:-build/springs}
mkdir -p "$OUT"
$CXX -std=c++11 -O1 -Wno-unused-result -I"$IMGUI" -I. tests/springs/spring_test.cpp im_anim.cpp \
	"$IMGUI/imgui.cpp" "$IMGUI/imgui_draw.cpp" "$IMGUI/imgui_tables.cpp" "$IMGUI/imgui_widgets.cpp" -o "$OUT/spring_test"
"$OUT/spring_test"
