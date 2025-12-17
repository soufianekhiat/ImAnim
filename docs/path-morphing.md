# Path Morphing

Smoothly interpolate between two different motion paths.

## Overview

Path morphing allows you to blend between two paths, creating smooth shape transitions. Both paths are resampled to match, enabling morphing between paths with different numbers of segments.

## Basic Usage

```cpp
// Define two paths
iam_path::begin(ImHashStr("path_a"), ImVec2(0, 0))
    .line_to(ImVec2(100, 0))
    .line_to(ImVec2(100, 100))
    .end();

iam_path::begin(ImHashStr("path_b"), ImVec2(0, 50))
    .cubic_to(ImVec2(30, 0), ImVec2(70, 100), ImVec2(100, 50))
    .end();

// Morph between them
float blend = 0.5f;  // 0 = path_a, 1 = path_b
float t = 0.3f;      // Position along path (0-1)

ImVec2 pos = iam_path_morph(
    ImHashStr("path_a"), ImHashStr("path_b"),
    t, blend
);
```

## Morph Options

```cpp
struct iam_morph_opts {
    int   samples;          // Resample resolution (default: 64)
    bool  match_endpoints;  // Force endpoints to match (default: true)
    bool  use_arc_length;   // Use arc-length for smoother morphing (default: true)
};

iam_morph_opts opts;
opts.samples = 128;           // Higher quality
opts.match_endpoints = true;  // Endpoints stay fixed
opts.use_arc_length = true;   // Constant-speed sampling

ImVec2 pos = iam_path_morph(path_a, path_b, t, blend, opts);
```

## Animated Morphing

Animate the blend value over time:

```cpp
static float morph_blend = 0.0f;
float target_blend = show_shape_b ? 1.0f : 0.0f;

morph_blend = iam_tween_float(
    ImGui::GetID("morph"), ImHashStr("blend"),
    target_blend, 0.5f,
    iam_ease_preset(iam_ease_in_out_cubic),
    iam_policy_crossfade, dt
);

// Draw morphed path
ImDrawList* dl = ImGui::GetWindowDrawList();
ImVec2 origin = ImGui::GetCursorScreenPos();

for (float t = 0; t <= 1.0f; t += 0.01f) {
    ImVec2 p = iam_path_morph(path_a, path_b, t, morph_blend);
    dl->PathLineTo(ImVec2(origin.x + p.x, origin.y + p.y));
}
dl->PathStroke(IM_COL32(255, 255, 255, 255), 0, 2.0f);
```

## Tween Path Morph

Animate both position along path AND the morph blend simultaneously:

```cpp
ImVec2 pos = iam_tween_path_morph(
    id, ImHashStr("morph_pos"),
    path_a, path_b,
    target_blend,                               // Target blend value
    duration,                                   // Animation duration
    iam_ease_preset(iam_ease_out_cubic),       // Path position easing
    iam_ease_preset(iam_ease_in_out_quad),     // Morph blend easing
    iam_policy_crossfade,
    dt
);

// Query current blend value
float current_blend = iam_get_morph_blend(id, ImHashStr("morph_pos"));
```

## Tangent and Angle

Get direction information during morphing:

```cpp
// Tangent (normalized direction)
ImVec2 tangent = iam_path_morph_tangent(path_a, path_b, t, blend);

// Rotation angle in radians
float angle = iam_path_morph_angle(path_a, path_b, t, blend);
```

## Example: Shape Transition

```cpp
// Circle path
iam_path::begin(ImHashStr("circle"), ImVec2(50, 0))
    .cubic_to(ImVec2(50, 27), ImVec2(27, 50), ImVec2(0, 50))
    .cubic_to(ImVec2(-27, 50), ImVec2(-50, 27), ImVec2(-50, 0))
    .cubic_to(ImVec2(-50, -27), ImVec2(-27, -50), ImVec2(0, -50))
    .cubic_to(ImVec2(27, -50), ImVec2(50, -27), ImVec2(50, 0))
    .end();

// Square path
iam_path::begin(ImHashStr("square"), ImVec2(50, -50))
    .line_to(ImVec2(50, 50))
    .line_to(ImVec2(-50, 50))
    .line_to(ImVec2(-50, -50))
    .close()
    .end();

// Animate between shapes
static float shape_blend = 0.0f;
if (ImGui::Button("Toggle Shape")) {
    target_shape = 1.0f - target_shape;
}

shape_blend = iam_tween_float(
    ImGui::GetID("shape"), ImHashStr("blend"),
    target_shape, 0.6f,
    iam_ease_preset(iam_ease_in_out_back),
    iam_policy_crossfade, dt
);

// Draw morphed shape
ImDrawList* dl = ImGui::GetWindowDrawList();
ImVec2 center = ImGui::GetCursorScreenPos() + ImVec2(100, 100);

for (float t = 0; t <= 1.0f; t += 0.02f) {
    ImVec2 p = iam_path_morph(
        ImHashStr("circle"), ImHashStr("square"),
        t, shape_blend
    );
    dl->PathLineTo(center + p);
}
dl->PathStroke(IM_COL32(100, 200, 255, 255), ImDrawFlags_Closed, 2.0f);
```

## Example: Animated Icon

```cpp
// Menu icon (three lines)
iam_path::begin(ImHashStr("menu"), ImVec2(0, 0))
    .line_to(ImVec2(30, 0))
    .end();

// Arrow icon
iam_path::begin(ImHashStr("arrow"), ImVec2(0, -10))
    .line_to(ImVec2(15, 0))
    .line_to(ImVec2(0, 10))
    .end();

// Morph menu to back arrow on click
float icon_blend = iam_tween_float(
    ImGui::GetID("icon"), ImHashStr("morph"),
    menu_open ? 1.0f : 0.0f, 0.3f,
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade, dt
);

// Draw 3 morphed lines
for (int i = 0; i < 3; i++) {
    ImVec2 offset(0, i * 10 - 10);
    for (float t = 0; t <= 1.0f; t += 0.05f) {
        ImVec2 p = iam_path_morph(
            ImHashStr("menu"), ImHashStr("arrow"),
            t, icon_blend
        );
        // Apply rotation based on which line
        // ...
    }
}
```

## API Reference

| Function | Description |
|----------|-------------|
| `iam_path_morph(a, b, t, blend, opts)` | Get position on morphed path |
| `iam_path_morph_tangent(a, b, t, blend, opts)` | Get tangent of morphed path |
| `iam_path_morph_angle(a, b, t, blend, opts)` | Get angle of morphed path |
| `iam_tween_path_morph(...)` | Animate position and morph blend |
| `iam_get_morph_blend(id, ch)` | Get current morph blend value |

## See Also

- [Motion Paths](motion-paths.md) - Basic path animation
- [Easing](easing.md) - Easing functions
