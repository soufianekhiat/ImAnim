#!/bin/sh
# Performance benchmarks (optimized build). Prints per-frame costs; fails only on gross regressions.
set -e
cd "$(dirname "$0")/../.."
IMGUI=${IMGUI_DIR:-examples/extern/imgui}
CXX=${CXX:-g++}
OUT=${OUT_DIR:-build/bench}
mkdir -p "$OUT"
$CXX -std=c++11 -O2 -DNDEBUG -Wno-unused-result -I"$IMGUI" -I. tests/bench/bench.cpp im_anim.cpp im_anim_lottie.cpp \
	"$IMGUI/imgui.cpp" "$IMGUI/imgui_draw.cpp" "$IMGUI/imgui_tables.cpp" "$IMGUI/imgui_widgets.cpp" -o "$OUT/bench"
"$OUT/bench"
