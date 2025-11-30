# Anchors & Resize-Aware Animation

Keep animations smooth when windows resize with anchor-relative positioning.

## Overview

Anchor-relative animations specify positions as `percent * anchor_size + px_bias`, which automatically adapts to container size changes.

## Anchor Spaces

| Space | Reference Size |
|-------|---------------|
| `iam_anchor_window_content` | `ImGui::GetContentRegionAvail()` |
| `iam_anchor_window` | `ImGui::GetWindowSize()` |
| `iam_anchor_viewport` | `ImGui::GetWindowViewport()->Size` |
| `iam_anchor_last_item` | `ImGui::GetItemRectSize()` |

## Tween API

### Relative Vec2 Tween

```cpp
ImVec2 pos = iam_tween_vec2_rel(
    id, channel,
    ImVec2(0.5f, 0.5f),    // 50% of container
    ImVec2(10, 20),        // +10px X, +20px Y offset
    duration, ease, policy,
    iam_anchor_window_content,
    dt
);
```

### Resolved Tween

For custom position calculation:

```cpp
ImVec2 pos = iam_tween_vec2_resolved(
    id, channel,
    [](void* user) -> ImVec2 {
        // Custom calculation
        return calculate_dynamic_position();
    },
    nullptr,  // user data
    duration, ease, policy, dt
);
```

### Rebase Animation

Smoothly adjust target without interrupting animation:

```cpp
iam_rebase_vec2(id, channel, new_target, dt);
```

## Clip API

### Anchor-Relative Keyframes

```cpp
// Float: specify percent, pixel bias, anchor space, and axis (0=X, 1=Y)
.key_float_rel(channel, time, percent, px_bias, anchor_space, axis, ease_type)

// Vec2: both X and Y are relative
.key_vec2_rel(channel, time, percent, px_bias, anchor_space, ease_type)

// Vec4: X,Y are relative, Z,W are absolute
.key_vec4_rel(channel, time, percent, px_bias, anchor_space, ease_type)

// Color: relative color values
.key_color_rel(channel, time, percent, px_bias, color_space, anchor_space, ease_type)
```

### Example: Responsive Race Animation

```cpp
static const ImGuiID CLIP_RACE = ImHashStr("race");
static const ImGuiID CH_POS_X = ImHashStr("pos_x");

// Define clip with anchor-relative positions
iam_clip::begin(CLIP_RACE)
    // Start: 0% + 35px margin
    .key_float_rel(CH_POS_X, 0.0f, 0.0f, 35.0f, iam_anchor_window_content, 0, iam_ease_linear)
    // End: 100% - 35px margin
    .key_float_rel(CH_POS_X, 3.0f, 1.0f, -35.0f, iam_anchor_window_content, 0, iam_ease_linear)
    .end();

// Query - pos_x is already resolved to actual pixels
float pos_x;
inst.get_float(CH_POS_X, &pos_x);
// pos_x automatically adapts to window width!
```

## How Resolution Works

```
final_value = percent * anchor_size + px_bias
```

For a 400px wide window with `percent=0.5, px_bias=10`:
```
final_value = 0.5 * 400 + 10 = 210px
```

If window resizes to 600px:
```
final_value = 0.5 * 600 + 10 = 310px
```

The animation automatically adapts.

## Backwards Compatibility

Existing absolute keyframes work unchanged:

```cpp
// These still work exactly as before
.key_float(channel, time, absolute_value, ease_type)
.key_vec2(channel, time, ImVec2(x, y), ease_type)
```

The same getter API returns values based on track type:

```cpp
float value;
inst.get_float(channel, &value);
// Returns absolute value for key_float()
// Returns resolved relative value for key_float_rel()
```

## Example: Centered Modal

```cpp
iam_clip::begin(ImHashStr("modal_appear"))
    // Position at center (50%) with no offset
    .key_vec2_rel(CH_POS, 0.0f, ImVec2(0.5f, 0.5f), ImVec2(0, 0),
                  iam_anchor_viewport, iam_ease_out_back)
    // Start scaled down
    .key_float(CH_SCALE, 0.0f, 0.0f, iam_ease_out_back)
    .key_float(CH_SCALE, 0.3f, 1.0f, iam_ease_out_back)
    .end();
```

## Example: Edge-to-Edge Slide

```cpp
iam_clip::begin(ImHashStr("slide_in"))
    // Start off-screen left (-100px from 0%)
    .key_float_rel(CH_X, 0.0f, 0.0f, -100.0f, iam_anchor_window, 0, iam_ease_out_cubic)
    // End at center (50%)
    .key_float_rel(CH_X, 0.5f, 0.5f, 0.0f, iam_anchor_window, 0, iam_ease_out_cubic)
    .end();
```

## Notes

- Anchor values are resolved at `get_*()` time, not keyframe definition time
- This ensures animations remain responsive during playback
- Mixing absolute and relative keyframes in the same track is not supported
- For vec4_rel, only X and Y components are relative (useful for rectangles)

## See Also

- [Clips](clips.md) - Clip system basics
- [Tweens](tweens.md) - Tween API
