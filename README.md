# ImAnim

Animation helpers for [Dear ImGui](https://github.com/ocornut/imgui). Provides smooth, production-ready tweening and clip-based animation systems.

## Features

- **Tween API** - Immediate-mode animation for floats, vec2, vec4, int, and colors
- **Clip API** - Timeline-based keyframe animations (anime.js-style)
- **30+ Easing Functions** - Standard presets plus cubic-bezier, steps, spring physics
- **Color Blending** - sRGB, Linear, HSV, and OKLAB color spaces
- **Tween Policies** - Crossfade, cut, or queue animations
- **Resize-Aware** - Helpers for responsive layouts that survive window resizing
- **Zero Dependencies** - Only requires Dear ImGui

## Quick Start

```cpp
#include "im_anim.h"
using namespace im_anim;

// Call once per frame before any tweens
update_begin_frame();
clip_update(ImGui::GetIO().DeltaTime);

// Tween a float with easing
float alpha = tween_float(
    ImGui::GetID("MyWidget"),      // owner ID
    ImHashStr("alpha"),            // channel ID
    hovered ? 1.0f : 0.5f,         // target
    0.3f,                          // duration (seconds)
    ease_preset(im_anim_ease_out_cubic),
    im_anim_policy_crossfade,
    ImGui::GetIO().DeltaTime
);
```

## Tween API

### Channel Types

```cpp
float  tween_float(id, channel, target, duration, ease, policy, dt);
ImVec2 tween_vec2(id, channel, target, duration, ease, policy, dt);
ImVec4 tween_vec4(id, channel, target, duration, ease, policy, dt);
int    tween_int(id, channel, target, duration, ease, policy, dt);
ImVec4 tween_color(id, channel, target_srgb, duration, ease, policy, color_space, dt);
```

### Easing Presets

| Category | Functions |
|----------|-----------|
| Linear | `im_anim_ease_linear` |
| Quad | `in_quad`, `out_quad`, `in_out_quad` |
| Cubic | `in_cubic`, `out_cubic`, `in_out_cubic` |
| Quart | `in_quart`, `out_quart`, `in_out_quart` |
| Quint | `in_quint`, `out_quint`, `in_out_quint` |
| Sine | `in_sine`, `out_sine`, `in_out_sine` |
| Expo | `in_expo`, `out_expo`, `in_out_expo` |
| Circ | `in_circ`, `out_circ`, `in_out_circ` |
| Back | `in_back`, `out_back`, `in_out_back` |
| Elastic | `in_elastic`, `out_elastic`, `in_out_elastic` |
| Bounce | `in_bounce`, `out_bounce`, `in_out_bounce` |
| Special | `steps`, `cubic_bezier`, `spring` |

### Ease Helpers

```cpp
ease_preset(im_anim_ease_out_cubic)           // Standard preset
ease_bezier(0.25f, 0.1f, 0.25f, 1.0f)         // Custom cubic-bezier
ease_steps(5, 0)                               // Step function (5 steps, jump-end)
ease_back(1.7f)                                // Back with custom overshoot
ease_elastic(1.0f, 0.3f)                       // Elastic with amplitude/period
ease_spring(1.0f, 100.0f, 10.0f, 0.0f)        // Physics spring
```

### Tween Policies

| Policy | Behavior |
|--------|----------|
| `im_anim_policy_crossfade` | Smoothly blend to new target |
| `im_anim_policy_cut` | Snap immediately to target |
| `im_anim_policy_queue` | Queue target, play after current |

### Color Spaces

```cpp
tween_color(id, channel, target, dur, ease, policy, im_anim_col_srgb, dt);        // sRGB blend
tween_color(id, channel, target, dur, ease, policy, im_anim_col_srgb_linear, dt); // Linear RGB
tween_color(id, channel, target, dur, ease, policy, im_anim_col_hsv, dt);         // HSV blend
tween_color(id, channel, target, dur, ease, policy, im_anim_col_oklab, dt);       // OKLAB (perceptual)
```

## Clip API

Timeline-based animations with keyframes, looping, and callbacks.

### Defining Clips

```cpp
// Define once at startup
Clip::begin(ImHashStr("bounce_clip"))
    .key_float(ImHashStr("scale"), 0.0f, 1.0f, im_anim_ease_out_elastic)
    .key_float(ImHashStr("scale"), 1.0f, 1.5f, im_anim_ease_in_out_cubic)
    .key_float(ImHashStr("scale"), 2.0f, 1.0f, im_anim_ease_out_bounce)
    .set_loop(true, im_anim_dir_alternate)
    .end();
```

### Playing Clips

```cpp
// Start playback
Instance inst = play(ImHashStr("bounce_clip"), ImHashStr("widget_1"));

// Query animated values
float scale;
inst.get_float(ImHashStr("scale"), &scale);

// Playback control
inst.pause();
inst.resume();
inst.seek(0.5f);
inst.set_time_scale(2.0f);
inst.stop();
```

### Keyframe Types

```cpp
.key_float(channel, time, value, ease_type)
.key_vec2(channel, time, ImVec2(x, y), ease_type)
.key_vec4(channel, time, ImVec4(x, y, z, w), ease_type)
.key_int(channel, time, value, ease_type)
.key_float_spring(channel, time, target, spring_params)  // Physics-based
```

### Loop Directions

```cpp
.set_loop(true, im_anim_dir_normal)     // Forward loop
.set_loop(true, im_anim_dir_reverse)    // Backward loop
.set_loop(true, im_anim_dir_alternate)  // Ping-pong
.set_loop(true, im_anim_dir_normal, 3)  // Loop 3 times then stop
```

### Callbacks

```cpp
Clip::begin(id)
    .on_begin([](ImGuiID inst_id, void* user) { /* started */ }, nullptr)
    .on_update([](ImGuiID inst_id, void* user) { /* each frame */ }, nullptr)
    .on_complete([](ImGuiID inst_id, void* user) { /* finished */ }, nullptr)
    .end();
```

## Resize-Aware Helpers

Keep animations smooth when windows resize:

```cpp
// Animate relative to container size (percent + pixel offset)
ImVec2 pos = tween_vec2_rel(
    id, channel,
    ImVec2(0.5f, 0.5f),    // 50% of container
    ImVec2(10, 20),        // +10px, +20px offset
    duration, ease, policy,
    im_anim_anchor_window_content,
    dt
);

// Custom resolver function
ImVec2 pos = tween_vec2_resolved(
    id, channel,
    [](void* user) { return calculate_position(); },
    nullptr,
    duration, ease, policy, dt
);

// Rebase animation to new target without interruption
rebase_vec2(id, channel, new_target, dt);
```

### Anchor Spaces

| Space | Reference |
|-------|-----------|
| `im_anim_anchor_window_content` | `ImGui::GetContentRegionAvail()` |
| `im_anim_anchor_window` | `ImGui::GetWindowSize()` |
| `im_anim_anchor_viewport` | `ImGui::GetWindowViewport()->Size` |
| `im_anim_anchor_last_item` | `ImGui::GetItemRectSize()` |

## Memory Management

```cpp
// Optional: pre-allocate pools
reserve(1000, 500, 200, 100, 300);  // float, vec2, vec4, int, color

// Garbage collect stale entries (call periodically)
gc(600);       // Tween entries older than 600 frames
clip_gc(600);  // Clip instances older than 600 frames

// Configure easing LUT resolution (default: 256)
set_ease_lut_samples(512);
```

## Integration

### Minimal Setup

```cpp
// In your main loop
while (running) {
    // ... platform/graphics setup ...

    ImGui::NewFrame();

    // Update animation systems
    im_anim::update_begin_frame();
    im_anim::clip_update(ImGui::GetIO().DeltaTime);

    // Your UI code with tweens/clips
    MyAnimatedUI();

    ImGui::Render();
    // ... render ...
}
```

### Example: Animated Button

```cpp
void AnimatedButton(const char* label) {
    ImGui::PushID(label);
    ImGuiID id = ImGui::GetID("btn");
    float dt = ImGui::GetIO().DeltaTime;

    bool hovered = ImGui::IsItemHovered();

    float scale = tween_float(id, ImHashStr("scale"),
        hovered ? 1.1f : 1.0f, 0.15f,
        ease_preset(im_anim_ease_out_back),
        im_anim_policy_crossfade, dt);

    ImVec4 color = tween_color(id, ImHashStr("color"),
        hovered ? ImVec4(0.3f, 0.7f, 1.0f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f),
        0.2f, ease_preset(im_anim_ease_out_cubic),
        im_anim_policy_crossfade, im_anim_col_oklab, dt);

    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::SetWindowFontScale(scale);
    ImGui::Button(label);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::PopID();
}
```

## Building

ImAnim is a single header+source pair. Add to your project:

```
src/im_anim.h
src/im_anim.cpp
```

Requires Dear ImGui headers in your include path.

## Demo

The `demo/` folder contains a comprehensive demo application showing all features:

- Easing function visualizer with interactive curves
- Tween examples (float, vec2, color with multiple color spaces)
- Tween policy comparison (crossfade vs cut vs queue)
- Clip system with playback controls
- ImDrawList animations (3D rotating cube, morphing shapes, particles, orbits)

## License

MIT License
