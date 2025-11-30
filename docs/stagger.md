# Stagger Animations

Create cascading effects for multiple elements with automatic delay offsets.

## Overview

Stagger animations apply the same animation to multiple elements with incrementing delays, creating wave-like cascading effects.

## Defining Stagger Clips

```cpp
static const ImGuiID CH_ALPHA = ImHashStr("alpha");
static const ImGuiID CH_SCALE = ImHashStr("scale");
static const ImGuiID CH_POS_Y = ImHashStr("pos_y");
static const ImGuiID CLIP_STAGGER = ImHashStr("stagger");

// Define for 12 items with 60ms delay between each
iam_clip::begin(CLIP_STAGGER)
    .key_float(CH_POS_Y, 0.0f, 40.0f, iam_ease_out_back)
    .key_float(CH_POS_Y, 0.5f, 0.0f, iam_ease_out_back)
    .key_float(CH_SCALE, 0.0f, 0.0f, iam_ease_out_elastic)
    .key_float(CH_SCALE, 0.6f, 1.0f, iam_ease_out_elastic)
    .key_float(CH_ALPHA, 0.0f, 0.0f, iam_ease_out_quad)
    .key_float(CH_ALPHA, 0.3f, 1.0f, iam_ease_out_quad)
    .set_stagger(12, 0.06f, 0.0f)  // count, delay, center_bias
    .end();
```

## set_stagger Parameters

```cpp
.set_stagger(count, delay, center_bias)
```

| Parameter | Description |
|-----------|-------------|
| `count` | Number of elements in the stagger group |
| `delay` | Delay in seconds between each element |
| `center_bias` | Where animation starts from (-1 to 1) |

### Center Bias Values

| Value | Effect |
|-------|--------|
| `0.0f` | Start from first element (wave left to right) |
| `1.0f` | Start from center (radial outward) |
| `-1.0f` | Start from edges (pinch inward) |

## Playing Stagger Animations

```cpp
// Create instance IDs for each element
static ImGuiID inst_ids[12];
for (int i = 0; i < 12; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "item_%d", i);
    inst_ids[i] = ImHashStr(buf);
}

// Play with stagger - each gets its computed delay
for (int i = 0; i < 12; i++) {
    iam_play_stagger(CLIP_STAGGER, inst_ids[i], i);
}
```

## Querying Stagger Values

```cpp
for (int i = 0; i < 12; i++) {
    iam_instance inst = iam_get_instance(inst_ids[i]);

    float alpha = 0.0f, scale = 0.0f, pos_y = 0.0f;
    if (inst.valid()) {
        inst.get_float(CH_ALPHA, &alpha);
        inst.get_float(CH_SCALE, &scale);
        inst.get_float(CH_POS_Y, &pos_y);
    }

    // Draw element with animated values
    ImVec2 pos = ImVec2(start_x + i * spacing, base_y + pos_y);
    ImGui::SetCursorScreenPos(pos);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::SetWindowFontScale(scale);
    ImGui::Text("Item %d", i);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleVar();
}
```

## Example: List Slide-In

```cpp
// Clip: items slide in from the right
iam_clip::begin(ImHashStr("list_slide"))
    .key_float(CH_POS_X, 0.0f, 100.0f, iam_ease_out_cubic)
    .key_float(CH_POS_X, 0.4f, 0.0f, iam_ease_out_cubic)
    .key_float(CH_ALPHA, 0.0f, 0.0f, iam_ease_out_quad)
    .key_float(CH_ALPHA, 0.3f, 1.0f, iam_ease_out_quad)
    .set_stagger(5, 0.08f, 0.0f)
    .end();

// Play on button click
if (ImGui::Button("Show List")) {
    for (int i = 0; i < 5; i++) {
        iam_play_stagger(ImHashStr("list_slide"), list_inst_ids[i], i);
    }
}
```

## Example: Grid Reveal

```cpp
// 4x4 grid with center-out reveal
int grid_size = 16;
iam_clip::begin(ImHashStr("grid_reveal"))
    .key_float(CH_SCALE, 0.0f, 0.0f, iam_ease_out_back)
    .key_float(CH_SCALE, 0.5f, 1.0f, iam_ease_out_back)
    .key_float(CH_ALPHA, 0.0f, 0.0f, iam_ease_out_quad)
    .key_float(CH_ALPHA, 0.3f, 1.0f, iam_ease_out_quad)
    .set_stagger(grid_size, 0.05f, 1.0f)  // center_bias = 1 for radial
    .end();

// Calculate stagger index based on distance from center
int center = grid_size / 2;
for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
        int idx = row * 4 + col;
        // Distance from center determines stagger order
        float dx = col - 1.5f;
        float dy = row - 1.5f;
        int dist_idx = (int)(sqrtf(dx*dx + dy*dy) * 4);
        iam_play_stagger(ImHashStr("grid_reveal"), grid_inst_ids[idx], dist_idx);
    }
}
```

## Example: Rainbow Wave

```cpp
// Rainbow colors with wave motion
static const int RAINBOW_COUNT = 7;
iam_clip::begin(ImHashStr("rainbow"))
    .key_float(CH_POS_Y, 0.0f, -20.0f, iam_ease_in_out_sine)
    .key_float(CH_POS_Y, 0.5f, 20.0f, iam_ease_in_out_sine)
    .key_float(CH_POS_Y, 1.0f, -20.0f, iam_ease_in_out_sine)
    .set_loop(true, iam_dir_normal)
    .set_stagger(RAINBOW_COUNT, 0.1f, 0.0f)
    .end();

ImU32 rainbow_colors[] = {
    IM_COL32(255, 0, 0, 255),     // Red
    IM_COL32(255, 127, 0, 255),   // Orange
    IM_COL32(255, 255, 0, 255),   // Yellow
    IM_COL32(0, 255, 0, 255),     // Green
    IM_COL32(0, 0, 255, 255),     // Blue
    IM_COL32(75, 0, 130, 255),    // Indigo
    IM_COL32(148, 0, 211, 255),   // Violet
};
```

## See Also

- [Clips](clips.md) - Clip system basics
- [Text Stagger](text-stagger.md) - Per-character text animation
