#!/bin/sh
# Builds vec_roundtrip.cpp with the default ImVec2/ImVec4 types and with strict custom
# types (custom_vec_types.h), then checks both print identical values.
set -e
cd "$(dirname "$0")/../.."
IMGUI=${IMGUI_DIR:-examples/extern/imgui}
CXX=${CXX:-g++}
SRC="tests/vec_types/vec_roundtrip.cpp im_anim.cpp $IMGUI/imgui.cpp $IMGUI/imgui_draw.cpp $IMGUI/imgui_tables.cpp $IMGUI/imgui_widgets.cpp"
OUT=${OUT_DIR:-build/vec_types}
mkdir -p "$OUT"
$CXX -std=c++11 -O1 -Wno-unused-result -I"$IMGUI" -I. $SRC -o "$OUT/rt_default"
$CXX -std=c++11 -O1 -Wno-unused-result -include tests/vec_types/custom_vec_types.h -I"$IMGUI" -I. $SRC -o "$OUT/rt_custom"
"$OUT/rt_default" > "$OUT/default.txt"
"$OUT/rt_custom" > "$OUT/custom.txt"
diff "$OUT/default.txt" "$OUT/custom.txt"
echo "OK: custom vector types give identical results ($(wc -l < "$OUT/default.txt") lines)"
