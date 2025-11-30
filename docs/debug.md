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

## See Also

- [Quick Start](quickstart.md) - Basic setup
- [Clips](clips.md) - Clip instance control
