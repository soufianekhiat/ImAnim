# Text Along Paths

Render text along curves with each character properly rotated to follow the path tangent.

## Overview

Text along paths renders each character positioned and rotated to follow a curve, creating effects like curved banners, circular text, and animated text reveals.

## Basic Usage

```cpp
// First, create a path
ImGuiID path_id = ImHashStr("text_path");
iam_path::begin(path_id, ImVec2(50, 100))
    .quadratic_to(ImVec2(150, 50), ImVec2(250, 100))
    .quadratic_to(ImVec2(350, 150), ImVec2(450, 100))
    .end();

// Build arc-length LUT for accurate placement
iam_path_build_arc_lut(path_id, 128);

// Render static text
iam_text_path(path_id, "Hello World!");

// Render animated text (characters appear progressively)
float progress = 0.5f;  // 0.0 to 1.0
iam_text_path_animated(path_id, "Hello World!", progress);
```

## Text Path Options

```cpp
iam_text_path_opts opts;
opts.origin = ImVec2(100, 200);         // Screen-space origin
opts.offset = 10.0f;                    // Starting offset along path
opts.letter_spacing = 2.0f;             // Extra spacing between chars
opts.align = iam_text_align_center;     // Alignment
opts.flip_y = false;                    // Flip for right-to-left paths
opts.color = IM_COL32(255, 200, 100, 255);
opts.font = nullptr;                    // Use current font
opts.font_scale = 1.2f;                 // Scale text size

iam_text_path(path_id, "Curved Text", opts);
```

### Alignment Options

| Value | Description |
|-------|-------------|
| `iam_text_align_start` | Text starts at path beginning |
| `iam_text_align_center` | Text centered on path |
| `iam_text_align_end` | Text ends at path end |

## Animated Text Reveal

```cpp
// Animate progress for character-by-character reveal
float progress = iam_tween_float(
    ImGui::GetID("text_reveal"),
    ImHashStr("progress"),
    1.0f,                               // Target (fully revealed)
    2.0f,                               // Duration
    iam_ease_preset(iam_ease_out_quad),
    iam_policy_crossfade,
    dt
);

iam_text_path_animated(path_id, "Revealing Text!", progress);
```

## Helper Functions

```cpp
// Calculate text width for layout
float width = iam_text_path_width("Hello World!", opts);

// Transform a quad by rotation and translation
ImVec2 quad[4] = { ... };
iam_transform_quad(quad, center, angle_rad, translation);

// Create a rotated quad for a glyph
ImVec2 glyph_quad[4];
iam_make_glyph_quad(glyph_quad, pos, angle, width, height, baseline_offset);
```

## Arc-Length Parameterization

For accurate constant-speed text placement:

```cpp
// Build with higher sample count for longer paths
iam_path_build_arc_lut(path_id, 256);

// Distance-based evaluation
float distance = 100.0f;  // Distance in pixels
ImVec2 pos = iam_path_evaluate_at_distance(path_id, distance);
float angle = iam_path_angle_at_distance(path_id, distance);
ImVec2 tangent = iam_path_tangent_at_distance(path_id, distance);

// Convert between distance and t
float t = iam_path_distance_to_t(path_id, distance);
```

## Example: Wave Text

```cpp
// Create wave path
iam_path::begin(ImHashStr("wave"), ImVec2(0, 50))
    .quadratic_to(ImVec2(50, 0), ImVec2(100, 50))
    .quadratic_to(ImVec2(150, 100), ImVec2(200, 50))
    .quadratic_to(ImVec2(250, 0), ImVec2(300, 50))
    .end();

iam_path_build_arc_lut(ImHashStr("wave"), 128);

// Render with animation
static float anim_time = 0.0f;
anim_time += dt;

iam_text_path_opts opts;
opts.origin = ImVec2(100, 200);
opts.offset = sinf(anim_time) * 20.0f;  // Oscillating offset
opts.color = IM_COL32(100, 200, 255, 255);

iam_text_path(ImHashStr("wave"), "Wave Text Animation", opts);
```

## Example: Circular Text

```cpp
// Create circular path
iam_path::begin(ImHashStr("circle"), ImVec2(100, 0))
    .cubic_to(ImVec2(100, 55), ImVec2(55, 100), ImVec2(0, 100))
    .cubic_to(ImVec2(-55, 100), ImVec2(-100, 55), ImVec2(-100, 0))
    .cubic_to(ImVec2(-100, -55), ImVec2(-55, -100), ImVec2(0, -100))
    .cubic_to(ImVec2(55, -100), ImVec2(100, -55), ImVec2(100, 0))
    .close()
    .end();

iam_path_build_arc_lut(ImHashStr("circle"), 256);

iam_text_path_opts opts;
opts.origin = ImVec2(200, 200);  // Circle center
opts.align = iam_text_align_center;

iam_text_path(ImHashStr("circle"), "CIRCULAR TEXT AROUND THE EDGE", opts);
```

## See Also

- [Motion Paths](motion-paths.md) - Path creation and animation
- [Text Stagger](text-stagger.md) - Per-character effects without paths
