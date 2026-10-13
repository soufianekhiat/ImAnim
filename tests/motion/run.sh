#!/bin/sh
# Layout animation, scroll-linked progress/triggers and inertia.
set -e
cd "$(dirname "$0")/../.."
IMGUI=${IMGUI_DIR:-examples/extern/imgui}
CXX=${CXX:-g++}
OUT=${OUT_DIR:-build/motion}
mkdir -p "$OUT"
$CXX -std=c++11 -O1 -Wno-unused-result -I"$IMGUI" -I. tests/motion/motion_test.cpp im_anim.cpp \
	"$IMGUI/imgui.cpp" "$IMGUI/imgui_draw.cpp" "$IMGUI/imgui_tables.cpp" "$IMGUI/imgui_widgets.cpp" -o "$OUT/motion_test"
"$OUT/motion_test"
