# Debug Tools

Debug windows and utilities for inspecting and controlling animations.

## Debug Window

Basic debug window with animation stats:

```cpp
static bool show_debug = true;
iam_show_debug_window(&show_debug);
```

Provides:
- Global time scale slider (0.1x to 2x)
- Active tween counts by type
- Clip system statistics
- Custom easing slot status

## Global Time Scale

Control animation speed globally:

```cpp
// Slow motion
iam_set_global_time_scale(0.25f);

// Normal speed
iam_set_global_time_scale(1.0f);

// Fast forward
iam_set_global_time_scale(2.0f);

// Query current scale
float scale = iam_get_global_time_scale();
```

### Keyboard Shortcut Example

```cpp
if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
    float scale = iam_get_global_time_scale();
    if (scale == 1.0f) {
        iam_set_global_time_scale(0.25f);  // Toggle slow-mo
    } else {
        iam_set_global_time_scale(1.0f);
    }
}
```

## Memory Statistics

Query animation system memory usage:

```cpp
// Get pool sizes
int float_count = iam_get_tween_count(iam_tween_float);
int vec2_count = iam_get_tween_count(iam_tween_vec2);
int clip_count = iam_get_clip_instance_count();
int path_count = iam_get_path_count();
```

## Custom Debug Overlay

Build your own debug display:

```cpp
void MyAnimDebugOverlay() {
    ImGui::Begin("Anim Debug");

    ImGui::Text("Time Scale: %.2fx", iam_get_global_time_scale());
    ImGui::SliderFloat("##scale", &scale, 0.1f, 3.0f);
    if (ImGui::IsItemEdited()) {
        iam_set_global_time_scale(scale);
    }

    ImGui::Separator();

    ImGui::Text("Active Tweens:");
    ImGui::BulletText("Float: %d", iam_get_tween_count(iam_tween_float));
    ImGui::BulletText("Vec2: %d", iam_get_tween_count(iam_tween_vec2));
    ImGui::BulletText("Color: %d", iam_get_tween_count(iam_tween_color));

    ImGui::Separator();

    ImGui::Text("Clip Instances: %d", iam_get_clip_instance_count());
    ImGui::Text("Motion Paths: %d", iam_get_path_count());

    ImGui::End();
}
```

## Performance Tips

### Garbage Collection

Periodically clean up stale entries:

```cpp
// In your main loop, every N frames
static int gc_counter = 0;
if (++gc_counter >= 300) {  // Every ~5 seconds at 60fps
    gc_counter = 0;
    iam_gc(600);       // Tweens older than 600 frames
    iam_clip_gc(600);  // Clip instances older than 600 frames
}
```

### Pre-allocation

Reserve memory upfront for known workloads:

```cpp
// At startup
iam_reserve(1000, 500, 200, 100, 300);  // float, vec2, vec4, int, color
```

### Easing LUT Resolution

Trade accuracy for speed:

```cpp
iam_set_ease_lut_samples(128);  // Faster, less accurate
iam_set_ease_lut_samples(512);  // Slower, more accurate
```

## Troubleshooting

### Animation not working / values not changing

**Problem:** `iam_tween_*` returns the target value immediately without animating.

**Solutions:**
1. Ensure `iam_update_begin_frame()` is called each frame before any animation calls
2. Ensure `iam_clip_update(dt)` is called each frame (even for tweens, this keeps the system synchronized)
3. Check that `dt` (delta time) is non-zero and reasonable (typically 0.016 for 60fps)

```cpp
// Correct order
ImGui::NewFrame();
iam_update_begin_frame();        // Must be first!
iam_clip_update(io.DeltaTime);   // Must be called every frame

// Your animations
float val = iam_tween_float(...);
```

### Linker error: undefined reference to `iam_*`

**Problem:** Compilation succeeds but linking fails with undefined symbols.

**Solutions:**
1. Ensure `im_anim.cpp` is added to your build (not just the header)
2. Check that `im_anim.cpp` is compiled with the same compiler settings as your project

```cmake
# CMake example
add_executable(myapp main.cpp im_anim.cpp)
```

### Animation plays once then stops

**Problem:** Animation works the first time but doesn't respond to subsequent target changes.

**Solutions:**
1. Ensure the target value actually changes between frames
2. Check policy - `iam_policy_cut` snaps immediately without animation
3. Verify the same ID is used consistently

```cpp
// This won't animate if 'target' doesn't change
float target = some_condition ? 1.0f : 0.0f;  // Must actually change
float val = iam_tween_float(id, ch, target, ...);
```

### ID collision - multiple elements sharing animation state

**Problem:** Different UI elements animate together unexpectedly.

**Solutions:**
1. Use unique owner IDs for each animated element
2. In loops, use `ImGui::PushID(i)` / `PopID()` or encode index into ID

```cpp
// Wrong: Same ID for all items
for (int i = 0; i < count; i++) {
    float alpha = iam_tween_float(ImHashStr("item"), ...);  // All share one animation!
}

// Correct: Unique ID per item
for (int i = 0; i < count; i++) {
    ImGui::PushID(i);
    float alpha = iam_tween_float(ImGui::GetID("item"), ...);  // Each has own animation
    ImGui::PopID();
}

// Also correct: Hash-based unique ID
for (int i = 0; i < count; i++) {
    ImGuiID id = ImHashData(&i, sizeof(i), ImHashStr("item"));
    float alpha = iam_tween_float(id, ...);
}
```

### Animation is jerky or inconsistent

**Problem:** Animation stutters or has inconsistent speed.

**Solutions:**
1. Use a consistent delta time source (usually `ImGui::GetIO().DeltaTime`)
2. Don't use a fixed dt value - use actual frame time
3. Check for frame rate spikes causing large dt values

```cpp
// Wrong: Fixed timestep
float val = iam_tween_float(..., 0.016f);

// Correct: Actual delta time
float val = iam_tween_float(..., ImGui::GetIO().DeltaTime);
```

### Clip not playing

**Problem:** `iam_play()` is called but clip doesn't animate.

**Solutions:**
1. Ensure the clip is defined before playing (call your `iam_clip::begin()...end()` setup)
2. Check that `iam_clip_update(dt)` is called every frame
3. Verify the clip ID matches between definition and playback

```cpp
// Definition (must happen first)
iam_clip::begin(ImHashStr("my_clip"))
    .key_float(...)
    .end();

// Playback (must use same ID)
iam_instance inst = iam_play(ImHashStr("my_clip"), instance_id);
```

### Memory usage growing over time

**Problem:** Animation pools keep growing without bound.

**Solutions:**
1. Call `iam_gc()` periodically to clean up stale entries
2. Pre-allocate with `iam_reserve()` if you know your workload

```cpp
// Every few seconds
static int frame_counter = 0;
if (++frame_counter >= 300) {  // Every 5 seconds at 60fps
    frame_counter = 0;
    iam_gc(600);       // Remove tweens unused for 600+ frames
    iam_clip_gc(600);  // Remove clip instances unused for 600+ frames
}
```

### Color animation looks wrong

**Problem:** Color transitions have unexpected intermediate values or look muddy.

**Solutions:**
1. Try a different color space - `iam_col_oklab` or `iam_col_oklch` often look better than `iam_col_srgb`
2. For hue transitions, use `iam_col_hsv` or `iam_col_oklch` for smooth hue rotation

```cpp
// Muddy transition in sRGB
ImVec4 col = iam_tween_color(..., iam_col_srgb, ...);

// Better perceptual transition
ImVec4 col = iam_tween_color(..., iam_col_oklab, ...);

// Best for hue shifts (red -> blue)
ImVec4 col = iam_tween_color(..., iam_col_oklch, ...);
```

### Getting animation state for debugging

Use `iam_show_debug_window()` or `iam_show_unified_inspector()` to visualize active animations:

```cpp
static bool show_inspector = false;
if (ImGui::IsKeyPressed(ImGuiKey_F12))
    show_inspector = !show_inspector;
if (show_inspector)
    iam_show_unified_inspector(&show_inspector);
```

## See Also

- [Quick Start](quickstart.md) - Basic setup
- [Integration](integration.md) - Adding ImAnim to your project
- [Clips](clips.md) - Clip instance control
