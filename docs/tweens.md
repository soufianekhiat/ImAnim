# Tweens

Immediate-mode animations that smoothly interpolate values over time.

## Overview

Tweens are the simplest way to animate in ImAnim. Call a tween function each frame with a target value, and ImAnim handles the smooth transition.

## Channel Types

```cpp
float  iam_tween_float(id, channel, target, duration, ease, policy, dt);
ImVec2 iam_tween_vec2(id, channel, target, duration, ease, policy, dt);
ImVec4 iam_tween_vec4(id, channel, target, duration, ease, policy, dt);
int    iam_tween_int(id, channel, target, duration, ease, policy, dt);
ImVec4 iam_tween_color(id, channel, target_srgb, duration, ease, policy, color_space, dt);
```

## Parameters

| Parameter | Description |
|-----------|-------------|
| `id` | Owner ID - typically `ImGui::GetID("widget")` |
| `channel` | Property ID - e.g., `ImHashStr("alpha")` |
| `target` | Target value to animate towards |
| `duration` | Animation duration in seconds |
| `ease` | Easing function (see [Easing](easing.md)) |
| `policy` | How to handle target changes mid-animation |
| `dt` | Delta time, usually `ImGui::GetIO().DeltaTime` |

## Tween Policies

Control how animations behave when the target changes mid-animation.

| Policy | Behavior |
|--------|----------|
| `iam_policy_crossfade` | Smoothly blend to new target from current position |
| `iam_policy_cut` | Snap immediately to new target |
| `iam_policy_queue` | Queue new target, play after current animation completes |

### Example: Policy Comparison

```cpp
// Crossfade: Smooth transition, never jarring
float alpha_smooth = iam_tween_float(id, ImHashStr("a"),
    hovered ? 1.0f : 0.0f, 0.3f,
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade, dt);

// Cut: Instant change, good for discrete states
float alpha_instant = iam_tween_float(id, ImHashStr("b"),
    hovered ? 1.0f : 0.0f, 0.3f,
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_cut, dt);

// Queue: Complete current animation first
float alpha_queue = iam_tween_float(id, ImHashStr("c"),
    hovered ? 1.0f : 0.0f, 0.3f,
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_queue, dt);
```

## Color Tweening

Color tweening supports multiple color spaces for different visual results.

```cpp
ImVec4 color = iam_tween_color(
    id, channel,
    target_color,          // Target in sRGB
    duration, ease, policy,
    color_space,           // Blending color space
    dt
);
```

### Color Spaces

| Space | Description | Best For |
|-------|-------------|----------|
| `iam_col_srgb` | Direct sRGB blend | Simple blends |
| `iam_col_srgb_linear` | Linear RGB blend | Physical accuracy |
| `iam_col_hsv` | HSV blend (shortest hue arc) | Rainbow effects |
| `iam_col_oklab` | OKLAB perceptual blend | UI transitions |
| `iam_col_oklch` | OKLCH cylindrical blend | Vibrant gradients |

### Example: Color Space Comparison

```cpp
// sRGB: Can get muddy in transitions
ImVec4 c1 = iam_tween_color(id, ch, target, 0.5f, ease, policy, iam_col_srgb, dt);

// OKLAB: Perceptually uniform, great for UI
ImVec4 c2 = iam_tween_color(id, ch, target, 0.5f, ease, policy, iam_col_oklab, dt);

// HSV: Good for hue rotations
ImVec4 c3 = iam_tween_color(id, ch, target, 0.5f, ease, policy, iam_col_hsv, dt);
```

## Animated Button Example

```cpp
void AnimatedButton(const char* label) {
    ImGui::PushID(label);
    ImGuiID id = ImGui::GetID("btn");
    float dt = ImGui::GetIO().DeltaTime;

    // Need to check hover before the button for this frame
    ImVec2 size = ImGui::CalcTextSize(label);
    ImVec2 pos = ImGui::GetCursorScreenPos();
    bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + size.x + 20, pos.y + size.y + 10));

    float scale = iam_tween_float(id, ImHashStr("scale"),
        hovered ? 1.1f : 1.0f, 0.15f,
        iam_ease_preset(iam_ease_out_back),
        iam_policy_crossfade, dt);

    ImVec4 color = iam_tween_color(id, ImHashStr("color"),
        hovered ? ImVec4(0.3f, 0.7f, 1.0f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f),
        0.2f, iam_ease_preset(iam_ease_out_cubic),
        iam_policy_crossfade, iam_col_oklab, dt);

    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::SetWindowFontScale(scale);
    ImGui::Button(label);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::PopID();
}
```

## Memory Management

```cpp
// Pre-allocate tween pools for performance
iam_reserve(1000, 500, 200, 100, 300);  // float, vec2, vec4, int, color

// Garbage collect stale entries periodically
iam_gc(600);  // Remove entries older than 600 frames
```

## See Also

- [Easing](easing.md) - All easing functions
- [Clips](clips.md) - Timeline-based animations
- [Anchors](anchors.md) - Resize-aware tweening
