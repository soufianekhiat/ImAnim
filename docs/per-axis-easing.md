# Per-Axis Easing

Apply different easing curves to each component of a vector or color.

## Overview

Standard tweens use the same easing for all components. Per-axis easing lets you specify different curves for X, Y, Z, and W/Alpha components independently.

## Basic Usage

```cpp
// Different easing for X and Y
iam_ease_per_axis ez;
ez.x = iam_ease_preset(iam_ease_out_bounce);  // X bounces
ez.y = iam_ease_preset(iam_ease_out_cubic);   // Y is smooth

ImVec2 pos = iam_tween_vec2_per_axis(
    id, ImHashStr("pos"),
    target_pos, 0.5f,
    ez, iam_policy_crossfade, dt
);
```

## iam_ease_per_axis Structure

```cpp
struct iam_ease_per_axis {
    iam_ease_desc x;    // Easing for X component
    iam_ease_desc y;    // Easing for Y component
    iam_ease_desc z;    // Easing for Z component (vec4/color)
    iam_ease_desc w;    // Easing for W/alpha (vec4/color)
};

// Constructors
iam_ease_per_axis()                                    // All linear
iam_ease_per_axis(iam_ease_desc all)                   // Same for all axes
iam_ease_per_axis(iam_ease_desc x, iam_ease_desc y)    // For vec2
iam_ease_per_axis(iam_ease_desc x, y, z, w)            // For vec4/color
```

## Vec2 Per-Axis

```cpp
// Ball drop: bounce on Y, smooth on X
iam_ease_per_axis ball_drop;
ball_drop.x = iam_ease_preset(iam_ease_out_quad);
ball_drop.y = iam_ease_preset(iam_ease_out_bounce);

ImVec2 ball_pos = iam_tween_vec2_per_axis(
    ImGui::GetID("ball"), ImHashStr("pos"),
    ImVec2(target_x, ground_y), 0.8f,
    ball_drop, iam_policy_crossfade, dt
);
```

## Vec4 Per-Axis

```cpp
// Animate a rectangle with different timing per edge
iam_ease_per_axis rect_ease;
rect_ease.x = iam_ease_preset(iam_ease_out_cubic);    // Left
rect_ease.y = iam_ease_preset(iam_ease_out_back);     // Top
rect_ease.z = iam_ease_preset(iam_ease_in_out_quad);  // Right
rect_ease.w = iam_ease_preset(iam_ease_out_elastic);  // Bottom

ImVec4 rect = iam_tween_vec4_per_axis(
    id, ImHashStr("rect"),
    ImVec4(0, 0, 200, 100), 0.6f,
    rect_ease, iam_policy_crossfade, dt
);
```

## Color Per-Axis

Animate color channels independently:

```cpp
// Flash red channel, smooth others
iam_ease_per_axis color_flash;
color_flash.x = iam_ease_preset(iam_ease_out_elastic);  // R - elastic
color_flash.y = iam_ease_preset(iam_ease_out_cubic);    // G - smooth
color_flash.z = iam_ease_preset(iam_ease_out_cubic);    // B - smooth
color_flash.w = iam_ease_preset(iam_ease_linear);       // A - linear

ImVec4 color = iam_tween_color_per_axis(
    id, ImHashStr("color"),
    ImVec4(1, 0, 0, 1), 0.4f,
    color_flash, iam_policy_crossfade,
    iam_col_srgb, dt
);
```

## Use Cases

### Diagonal Slide with Different Speeds

```cpp
// Element slides in: fast horizontally, bouncy vertically
iam_ease_per_axis slide;
slide.x = iam_ease_preset(iam_ease_out_expo);
slide.y = iam_ease_preset(iam_ease_out_back);

ImVec2 pos = iam_tween_vec2_per_axis(
    id, ImHashStr("slide"),
    visible ? ImVec2(100, 100) : ImVec2(-200, -50), 0.4f,
    slide, iam_policy_crossfade, dt
);
```

### Scale with Squash/Stretch

```cpp
// Squash and stretch effect
iam_ease_per_axis squash;
squash.x = iam_ease_preset(iam_ease_out_back);   // Width overshoots
squash.y = iam_ease_preset(iam_ease_out_cubic);  // Height catches up

ImVec2 scale = iam_tween_vec2_per_axis(
    id, ImHashStr("scale"),
    ImVec2(1.0f, 1.0f), 0.3f,
    squash, iam_policy_crossfade, dt
);
```

### Reveal Animation

```cpp
// Content reveals: width first, then height
iam_ease_per_axis reveal;
reveal.x = iam_ease_bezier(0.2f, 0.0f, 0.0f, 1.0f);  // Fast start X
reveal.y = iam_ease_bezier(0.4f, 0.0f, 0.2f, 1.0f);  // Delayed Y

ImVec2 size = iam_tween_vec2_per_axis(
    id, ImHashStr("size"),
    expanded ? ImVec2(300, 200) : ImVec2(0, 0), 0.5f,
    reveal, iam_policy_crossfade, dt
);
```

## API Reference

| Function | Description |
|----------|-------------|
| `iam_tween_vec2_per_axis(id, ch, target, dur, ez, policy, dt)` | Vec2 with per-axis easing |
| `iam_tween_vec4_per_axis(id, ch, target, dur, ez, policy, dt)` | Vec4 with per-axis easing |
| `iam_tween_color_per_axis(id, ch, target, dur, ez, policy, cs, dt)` | Color with per-channel easing |

## See Also

- [Easing](easing.md) - All easing functions
- [Tweens](tweens.md) - Basic tweens
