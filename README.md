# ImAnim

Animation helpers for [Dear ImGui](https://github.com/ocornut/imgui). Provides smooth, production-ready tweening and clip-based animation systems.

## Features

- **Tween API** - Immediate-mode animation for floats, vec2, vec4, int, and colors
- **Clip API** - Timeline-based keyframe animations (anime.js-style)
- **Motion Paths** - Animate along bezier curves and Catmull-Rom splines
- **Timeline Markers** - Trigger callbacks at specific times within clips
- **Animation Chaining** - Fluent `.then()` syntax to chain clips together
- **Stagger Animations** - Cascading effects for lists, grids, and UI elements
- **Oscillators** - Continuous sine, triangle, sawtooth, square wave generators
- **Shake & Wiggle** - Procedural noise animations for feedback and organic movement
- **Scroll Animation** - Smooth animated scrolling within ImGui windows
- **30+ Easing Functions** - Standard presets plus cubic-bezier, steps, spring physics
- **Color Blending** - sRGB, Linear, HSV, OKLAB, and OKLCH color spaces
- **Tween Policies** - Crossfade, cut, or queue animations
- **Layering** - Blend multiple animation instances together
- **Resize-Aware** - Helpers for responsive layouts that survive window resizing
- **Zero Dependencies** - Only requires Dear ImGui

## Quick Start

```cpp
#include "im_anim.h"

// Call once per frame before any tweens
iam_update_begin_frame();
iam_clip_update(ImGui::GetIO().DeltaTime);

// Tween a float with easing
float alpha = iam_tween_float(
    ImGui::GetID("MyWidget"),      // owner ID
    ImHashStr("alpha"),            // channel ID
    hovered ? 1.0f : 0.5f,         // target
    0.3f,                          // duration (seconds)
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade,
    ImGui::GetIO().DeltaTime
);
```

## Tween API

### Channel Types

```cpp
float  iam_tween_float(id, channel, target, duration, ease, policy, dt);
ImVec2 iam_tween_vec2(id, channel, target, duration, ease, policy, dt);
ImVec4 iam_tween_vec4(id, channel, target, duration, ease, policy, dt);
int    iam_tween_int(id, channel, target, duration, ease, policy, dt);
ImVec4 iam_tween_color(id, channel, target_srgb, duration, ease, policy, color_space, dt);
```

### Easing Presets

| Category | Functions |
|----------|-----------|
| Linear | `iam_ease_linear` |
| Quad | `iam_ease_in_quad`, `iam_ease_out_quad`, `iam_ease_in_out_quad` |
| Cubic | `iam_ease_in_cubic`, `iam_ease_out_cubic`, `iam_ease_in_out_cubic` |
| Quart | `iam_ease_in_quart`, `iam_ease_out_quart`, `iam_ease_in_out_quart` |
| Quint | `iam_ease_in_quint`, `iam_ease_out_quint`, `iam_ease_in_out_quint` |
| Sine | `iam_ease_in_sine`, `iam_ease_out_sine`, `iam_ease_in_out_sine` |
| Expo | `iam_ease_in_expo`, `iam_ease_out_expo`, `iam_ease_in_out_expo` |
| Circ | `iam_ease_in_circ`, `iam_ease_out_circ`, `iam_ease_in_out_circ` |
| Back | `iam_ease_in_back`, `iam_ease_out_back`, `iam_ease_in_out_back` |
| Elastic | `iam_ease_in_elastic`, `iam_ease_out_elastic`, `iam_ease_in_out_elastic` |
| Bounce | `iam_ease_in_bounce`, `iam_ease_out_bounce`, `iam_ease_in_out_bounce` |
| Special | `iam_ease_steps`, `iam_ease_cubic_bezier`, `iam_ease_spring` |

### Ease Helpers

```cpp
iam_ease_preset(iam_ease_out_cubic)              // Standard preset
iam_ease_bezier(0.25f, 0.1f, 0.25f, 1.0f)        // Custom cubic-bezier
iam_ease_steps_desc(5, 0)                        // Step function (5 steps, jump-end)
iam_ease_back(1.7f)                              // Back with custom overshoot
iam_ease_elastic(1.0f, 0.3f)                     // Elastic with amplitude/period
iam_ease_spring_desc(1.0f, 100.0f, 10.0f, 0.0f) // Physics spring
```

### Tween Policies

| Policy | Behavior |
|--------|----------|
| `iam_policy_crossfade` | Smoothly blend to new target |
| `iam_policy_cut` | Snap immediately to target |
| `iam_policy_queue` | Queue target, play after current |

### Color Spaces

```cpp
iam_tween_color(id, ch, target, dur, ease, policy, iam_col_srgb, dt);        // sRGB blend
iam_tween_color(id, ch, target, dur, ease, policy, iam_col_srgb_linear, dt); // Linear RGB
iam_tween_color(id, ch, target, dur, ease, policy, iam_col_hsv, dt);         // HSV blend
iam_tween_color(id, ch, target, dur, ease, policy, iam_col_oklab, dt);       // OKLAB (perceptual)
iam_tween_color(id, ch, target, dur, ease, policy, iam_col_oklch, dt);       // OKLCH (cylindrical OKLAB)
```

## Clip API

Timeline-based animations with keyframes, looping, and callbacks.

### Defining Clips

```cpp
// Define once at startup
static const ImGuiID CH_SCALE = ImHashStr("scale");

iam_clip::begin(ImHashStr("bounce_clip"))
    .key_float(CH_SCALE, 0.0f, 1.0f, iam_ease_out_elastic)
    .key_float(CH_SCALE, 1.0f, 1.5f, iam_ease_in_out_cubic)
    .key_float(CH_SCALE, 2.0f, 1.0f, iam_ease_out_bounce)
    .set_loop(true, iam_dir_alternate)
    .end();
```

### Playing Clips

```cpp
// Start playback
iam_instance inst = iam_play(ImHashStr("bounce_clip"), ImHashStr("widget_1"));

// Query animated values
float scale;
inst.get_float(CH_SCALE, &scale);

// Playback control
inst.pause();
inst.resume();
inst.seek(0.5f);
inst.set_time_scale(2.0f);
inst.stop();     // Stop playback but keep instance
inst.destroy();  // Remove instance entirely (valid() returns false after)
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
.set_loop(true, iam_dir_normal)     // Forward loop
.set_loop(true, iam_dir_reverse)    // Backward loop
.set_loop(true, iam_dir_alternate)  // Ping-pong
.set_loop(true, iam_dir_normal, 3)  // Loop 3 times then stop
```

### Callbacks

```cpp
iam_clip::begin(id)
    .on_begin([](ImGuiID inst_id, void* user) { /* started */ }, nullptr)
    .on_update([](ImGuiID inst_id, void* user) { /* each frame */ }, nullptr)
    .on_complete([](ImGuiID inst_id, void* user) { /* finished */ }, nullptr)
    .end();
```

### Timeline Markers

Trigger callbacks at specific times during clip playback:

```cpp
void my_marker_callback(ImGuiID inst_id, ImGuiID marker_id, float marker_time, void* user) {
    // Called when playback crosses this time point
    printf("Marker hit at %.2fs!\n", marker_time);
}

iam_clip::begin(ImHashStr("marked_clip"))
    .key_float(CH_VALUE, 0.0f, 0.0f, iam_ease_linear)
    .key_float(CH_VALUE, 3.0f, 100.0f, iam_ease_linear)
    .marker(0.5f, my_marker_callback)                           // Auto-generated ID
    .marker(1.0f, ImHashStr("checkpoint"), my_marker_callback)  // Named marker
    .marker(2.0f, my_marker_callback, my_user_data)             // With user data
    .end();
```

Markers are automatically triggered:
- During normal playback when the time crosses the marker
- During reverse playback
- On each loop iteration when looping

### Animation Chaining

Chain clips to play in sequence using the fluent `.then()` API:

```cpp
// Play clip A, then automatically start clip B when A completes
iam_play(clip_a, ImHashStr("inst_a"))
    .then(clip_b)                     // Auto-generates instance ID for clip_b
    .then_delay(0.1f);                // Optional delay before clip_b starts

// Chain with explicit instance IDs for later querying
iam_play(clip_a, ImHashStr("inst_a"))
    .then(clip_b, ImHashStr("inst_b"))
    .then_delay(0.2f);

// Multi-step chains (set up B->C when B starts)
iam_instance inst_b = iam_get_instance(ImHashStr("inst_b"));
if (inst_b.is_playing()) {
    inst_b.then(clip_c, ImHashStr("inst_c"));
}
```

## Motion Paths

Animate positions along bezier curves and Catmull-Rom splines.

### Defining Paths

```cpp
// Quadratic bezier path
iam_path::begin(ImHashStr("my_path"), ImVec2(0, 0))   // Start point
    .quadratic_to(ImVec2(50, -50), ImVec2(100, 0))   // Control point, end point
    .line_to(ImVec2(150, 0))                          // Linear segment
    .end();

// Cubic bezier path
iam_path::begin(ImHashStr("cubic_path"), ImVec2(0, 50))
    .cubic_to(ImVec2(30, 0), ImVec2(70, 100), ImVec2(100, 50))  // 2 control points, end
    .cubic_to(ImVec2(130, 0), ImVec2(170, 100), ImVec2(200, 50))
    .end();

// Catmull-Rom spline (smooth curve through points)
iam_path::begin(ImHashStr("smooth_path"), ImVec2(0, 50))
    .catmull_to(ImVec2(50, 20))           // Curve passes through these points
    .catmull_to(ImVec2(100, 80))
    .catmull_to(ImVec2(150, 50))
    .catmull_to(ImVec2(200, 50), 0.7f)    // Custom tension (0.5 = default)
    .close()                               // Close path back to start
    .end();
```

### Animating Along Paths

```cpp
// Tween position along a path
ImVec2 pos = iam_tween_path(
    id, channel_id, path_id,
    duration, ease, policy, dt
);

// Get rotation angle (radians) aligned to path tangent
float angle = iam_tween_path_angle(
    id, channel_id, path_id,
    duration, ease, policy, dt
);

// Manual path evaluation (no animation state)
ImVec2 point = iam_path_evaluate(path_id, t);     // t in [0,1]
ImVec2 tangent = iam_path_tangent(path_id, t);    // Normalized direction
float angle = iam_path_angle(path_id, t);         // Rotation in radians
float length = iam_path_length(path_id);          // Approximate total length
```

### Direct Curve Functions

For one-off curve evaluation without building paths:

```cpp
// Bezier curves
ImVec2 p = iam_bezier_quadratic(p0, p1, p2, t);          // Quadratic
ImVec2 p = iam_bezier_cubic(p0, p1, p2, p3, t);          // Cubic

// Catmull-Rom (curve passes through p1 and p2)
ImVec2 p = iam_catmull_rom(p0, p1, p2, p3, t, tension);

// Derivatives (for velocity/direction)
ImVec2 vel = iam_bezier_quadratic_deriv(p0, p1, p2, t);
ImVec2 vel = iam_bezier_cubic_deriv(p0, p1, p2, p3, t);
ImVec2 vel = iam_catmull_rom_deriv(p0, p1, p2, p3, t, tension);
```

## Stagger Animations

Create cascading effects for multiple elements with automatic delay offsets.

### Defining Stagger Clips

```cpp
// Channel IDs for animation properties
static const ImGuiID CH_ALPHA = ImHashStr("alpha");
static const ImGuiID CH_SCALE = ImHashStr("scale");
static const ImGuiID CH_POS_Y = ImHashStr("pos_y");

// Define a stagger clip for 12 items with 60ms delay between each
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

### Playing Stagger Animations

```cpp
// Create instance IDs for each element
static ImGuiID inst_ids[12];
for (int i = 0; i < 12; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "item_%d", i);
    inst_ids[i] = ImHashStr(buf);
}

// Play with stagger - each element starts with computed delay
for (int i = 0; i < 12; i++) {
    iam_play_stagger(CLIP_STAGGER, inst_ids[i], i);
}

// Query values per element
for (int i = 0; i < 12; i++) {
    iam_instance inst = iam_get_instance(inst_ids[i]);
    float alpha = 0.0f, scale = 0.0f, pos_y = 0.0f;
    if (inst.valid()) {
        inst.get_float(CH_ALPHA, &alpha);
        inst.get_float(CH_SCALE, &scale);
        inst.get_float(CH_POS_Y, &pos_y);
    }
    // Draw element with animated values...
}
```

### Stagger Options

```cpp
// Start from first element (wave effect)
.set_stagger(count, 0.06f, 0.0f)

// Start from center (radial effect)
.set_stagger(count, 0.06f, 1.0f)

// Start from edges (pinch effect)
.set_stagger(count, 0.06f, -1.0f)
```

## Oscillators

Continuous periodic animations with multiple wave shapes.

```cpp
// Basic oscillation (returns value between -amplitude and +amplitude)
float offset_y = iam_oscillate(
    ImGui::GetID("pulse"),
    50.0f,              // amplitude
    1.0f,               // frequency (Hz)
    iam_wave_sine,      // wave type
    0.0f,               // phase offset
    dt                  // delta time
);

// 2D oscillation (Lissajous patterns)
ImVec2 offset = iam_oscillate_vec2(
    ImGui::GetID("lissajous"),
    ImVec2(40.0f, 40.0f),    // amplitude
    ImVec2(1.0f, 2.0f),      // frequency
    iam_wave_sine,           // wave type
    ImVec2(0.0f, 0.0f),      // phase
    dt
);

// 4D oscillation
ImVec4 osc4 = iam_oscillate_vec4(id, amp4, freq4, wave_type, phase4, dt);

// Color oscillation (pulsing glow in HSV)
ImVec4 color = iam_oscillate_color(
    ImGui::GetID("glow"),
    ImVec4(0.5f, 0.8f, 1.0f, 1.0f),  // base color (sRGB)
    ImVec4(0.0f, 0.0f, 0.3f, 0.0f),  // amplitude (V in HSV)
    2.0f,                             // frequency
    iam_wave_sine,                    // wave type
    0.0f,                             // phase
    iam_col_hsv,                      // color space
    dt
);
```

### Wave Types

| Type | Description |
|------|-------------|
| `iam_wave_sine` | Smooth sinusoidal wave |
| `iam_wave_triangle` | Linear ramp up and down |
| `iam_wave_sawtooth` | Linear ramp up, instant reset |
| `iam_wave_square` | On/off pulse wave |

## Shake & Wiggle

Procedural noise-based animations for UI feedback.

```cpp
// Decaying shake (error feedback, impacts)
// Triggered by iam_trigger_shake(), then decays to 0
ImGuiID shake_id = ImGui::GetID("error");
if (validate_failed) {
    iam_trigger_shake(shake_id);
}
float offset_x = iam_shake(shake_id, 10.0f, 30.0f, 0.5f, dt);
//                         ^id      ^intensity ^freq  ^decay_time

// 2D shake
ImVec2 offset = iam_shake_vec2(id, ImVec2(10.0f, 10.0f), 30.0f, 0.5f, dt);

// 4D shake
ImVec4 shake4 = iam_shake_vec4(id, intensity4, freq, decay_time, dt);

// Color shake (flickering error indicator in OKLAB)
ImVec4 error_color = iam_shake_color(
    ImGui::GetID("error_indicator"),
    ImVec4(1.0f, 0.2f, 0.2f, 1.0f),  // base color (sRGB)
    ImVec4(0.1f, 0.05f, 0.0f, 0.0f), // intensity
    20.0f,                            // frequency
    0.5f,                             // decay time
    iam_col_oklab,                    // color space
    dt
);

// Continuous wiggle (idle animations, organic movement)
float wiggle = iam_wiggle(ImGui::GetID("idle"), 5.0f, 3.0f, dt);
//                                              ^amplitude ^frequency

// 2D wiggle
ImVec2 wiggle_offset = iam_wiggle_vec2(id, ImVec2(5.0f, 5.0f), 3.0f, dt);

// 4D wiggle
ImVec4 wiggle4 = iam_wiggle_vec4(id, amplitude4, freq, dt);

// Color wiggle (ambient color variation)
ImVec4 ambient_color = iam_wiggle_color(
    ImGui::GetID("ambient"),
    ImVec4(0.3f, 0.5f, 0.8f, 1.0f),  // base color
    ImVec4(0.0f, 0.05f, 0.1f, 0.0f), // amplitude
    1.5f,                             // frequency
    iam_col_oklch,                    // color space (perceptual)
    dt
);
```

## Scroll Animation

Smooth animated scrolling within ImGui windows.

```cpp
// Inside your scrollable window/child:

// Scroll to specific Y position
if (ImGui::Button("Go to Section 3")) {
    iam_scroll_to_y(500.0f, 0.5f);  // target_y, duration
}

// Scroll to top/bottom with default easing
iam_scroll_to_top(0.3f);
iam_scroll_to_bottom(0.3f);

// With custom easing
iam_scroll_to_y(target_y, 0.5f, iam_ease_preset(iam_ease_out_back));

// Horizontal scroll
iam_scroll_to_x(target_x, 0.5f);
```

## Layering

Blend multiple animation instances for complex effects.

```cpp
// Start blending into a target instance
iam_layer_begin(target_inst_id);

// Add source instances with weights
iam_layer_add(idle_inst, 0.3f);
iam_layer_add(walk_inst, 0.7f);

// Finalize blending
iam_layer_end(target_inst_id);

// Get blended values
float blended_value;
iam_get_blended_float(target_inst_id, CH_SCALE, &blended_value);
```

## Resize-Aware Helpers

Keep animations smooth when windows resize:

```cpp
// Animate relative to container size (percent + pixel offset)
ImVec2 pos = iam_tween_vec2_rel(
    id, channel,
    ImVec2(0.5f, 0.5f),    // 50% of container
    ImVec2(10, 20),        // +10px, +20px offset
    duration, ease, policy,
    iam_anchor_window_content,
    dt
);

// Custom resolver function
ImVec2 pos = iam_tween_vec2_resolved(
    id, channel,
    [](void* user) { return calculate_position(); },
    nullptr,
    duration, ease, policy, dt
);

// Rebase animation to new target without interruption
iam_rebase_vec2(id, channel, new_target, dt);
```

### Anchor Spaces

| Space | Reference |
|-------|-----------|
| `iam_anchor_window_content` | `ImGui::GetContentRegionAvail()` |
| `iam_anchor_window` | `ImGui::GetWindowSize()` |
| `iam_anchor_viewport` | `ImGui::GetWindowViewport()->Size` |
| `iam_anchor_last_item` | `ImGui::GetItemRectSize()` |

## Memory Management

```cpp
// Optional: pre-allocate pools
iam_reserve(1000, 500, 200, 100, 300);  // float, vec2, vec4, int, color

// Garbage collect stale entries (call periodically)
iam_gc(600);       // Tween entries older than 600 frames
iam_clip_gc(600);  // Clip instances older than 600 frames

// Configure easing LUT resolution (default: 256)
iam_set_ease_lut_samples(512);
```

## Custom Easing Functions

Register your own easing functions for unique animation curves:

```cpp
// Define a custom easing function
float MyCustomEase(float t) {
    return t * t * (3.0f - 2.0f * t);  // smoothstep
}

// Register in a slot (0-15 available)
iam_register_custom_ease(0, MyCustomEase);

// Use it in tweens
float val = iam_tween_float(id, ch, target, dur,
    iam_ease_custom_fn(0), iam_policy_crossfade, dt);
```

## Debug Tools

### Debug Window

Show an ImGui debug window with animation stats and controls:

```cpp
static bool show_debug = true;
iam_show_debug_window(&show_debug);
```

The debug window provides:
- Global time scale slider with presets (0.1x to 2x)
- Active tween counts by type
- Clip system statistics
- Custom easing slot status

### Slow-Motion Mode

Control animation speed globally for debugging:

```cpp
iam_set_global_time_scale(0.25f);  // Quarter speed
iam_set_global_time_scale(1.0f);   // Normal speed
iam_set_global_time_scale(2.0f);   // Double speed

float scale = iam_get_global_time_scale();
```

## Integration

### Minimal Setup

```cpp
// In your main loop
while (running) {
    // ... platform/graphics setup ...

    ImGui::NewFrame();

    // Update animation systems
    iam_update_begin_frame();
    iam_clip_update(ImGui::GetIO().DeltaTime);

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
- Cubic bezier editor with visual preview
- Spring physics simulation
- Step function animation
- Custom easing functions (smoothstep, bouncy, wobble examples)
- Tween examples (float, vec2, color with multiple color spaces)
- Tween policy comparison (crossfade vs cut vs queue)
- Clip system with playback controls and callbacks
- ImDrawList animations (3D rotating cube, morphing shapes, particles, orbits)
- Stagger animations (rainbow wave, list slide-in, grid reveal, dropping cards)
- Oscillators (wave types, Lissajous patterns, pulsing UI elements)
- Shake & Wiggle (error feedback, continuous organic movement)
- Scroll animation (smooth animated scrolling within windows)
- Motion paths (bezier curves, Catmull-Rom splines, mixed path segments)
- Timeline markers (callbacks at specific clip times, visual timeline)
- Animation chaining (A->B->C clip sequences with delays)
- Debug window with time scale control and stats

## Contributing

Development is incentivized through Patreon:

[![Patreon](https://img.shields.io/badge/Patreon-Become%20a%20Patron-f96854?style=for-the-badge&logo=patreon)](https://www.patreon.com/SoufianeKHIAT)

---

## License

MIT License — see [LICENSE](LICENSE) for details
