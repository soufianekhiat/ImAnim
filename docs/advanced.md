# Advanced Features

Performance profiling, lazy initialization, and clip persistence.

## Performance Profiler

ImAnim includes a built-in profiler for measuring animation performance.

### Enabling the Profiler

```cpp
// Enable profiler
iam_profiler_enable(true);

// Check if enabled
if (iam_profiler_is_enabled()) {
    // Profiler is active
}
```

### Frame Profiling

```cpp
// Main loop
while (running) {
    // Start frame profiling
    iam_profiler_begin_frame();

    ImGui::NewFrame();
    iam_update_begin_frame();
    iam_clip_update(dt);

    // Your UI code...

    ImGui::Render();

    // End frame profiling
    iam_profiler_end_frame();
}
```

### Custom Sections

Profile specific code sections:

```cpp
// Profile a specific section
iam_profiler_begin("UpdateParticles");
UpdateParticles(dt);
iam_profiler_end();

iam_profiler_begin("RenderEffects");
RenderEffects();
iam_profiler_end();
```

### Example: Debug Panel

```cpp
void ShowProfilerWindow() {
    if (ImGui::Begin("Profiler")) {
        static bool profiler_enabled = false;

        if (ImGui::Checkbox("Enable Profiler", &profiler_enabled)) {
            iam_profiler_enable(profiler_enabled);
        }

        if (profiler_enabled) {
            // Show profiler data via debug inspector
            iam_show_unified_inspector();
        }

        ImGui::End();
    }
}
```

## Lazy Initialization

By default, ImAnim defers channel creation until the first animation is needed. This reduces memory usage for widgets that may never be animated.

### Configuration

```cpp
// Disable lazy init (pre-create all channels)
iam_set_lazy_init(false);

// Enable lazy init (default)
iam_set_lazy_init(true);

// Check current setting
if (iam_is_lazy_init_enabled()) {
    // Channels created on demand
}
```

### When to Disable

Disable lazy init if you need:
- Predictable memory allocation at startup
- Guaranteed frame-time consistency (no allocation spikes)

```cpp
// Pre-allocate pools and disable lazy init
iam_reserve(1000, 500, 200, 100, 300);
iam_set_lazy_init(false);
```

## Clip Persistence

Save and load clip definitions to/from files.

### Saving Clips

```cpp
// Define a clip
iam_clip::begin(ImHashStr("my_clip"))
    .key_float(ImHashStr("scale"), 0.0f, 1.0f, iam_ease_out_elastic)
    .key_float(ImHashStr("scale"), 1.0f, 1.5f, iam_ease_in_out_cubic)
    .set_loop(true, iam_dir_alternate)
    .end();

// Save to file
iam_result result = iam_clip_save(ImHashStr("my_clip"), "animations/my_clip.ianim");
if (result == iam_ok) {
    // Saved successfully
}
```

### Loading Clips

```cpp
// Load from file
ImGuiID loaded_clip_id;
iam_result result = iam_clip_load("animations/my_clip.ianim", &loaded_clip_id);

if (result == iam_ok) {
    // Use the loaded clip
    iam_play(loaded_clip_id, ImHashStr("instance1"));
}
```

### Result Codes

```cpp
enum iam_result {
    iam_ok = 0,          // Success
    iam_err_not_found,   // Clip/file not found
    iam_err_bad_arg,     // Invalid argument
    iam_err_no_mem       // Out of memory
};
```

### Example: Animation Library

```cpp
void LoadAnimationLibrary() {
    const char* anim_files[] = {
        "animations/bounce.ianim",
        "animations/fade_in.ianim",
        "animations/slide_left.ianim",
        "animations/shake.ianim",
    };

    for (const char* file : anim_files) {
        ImGuiID clip_id;
        iam_result result = iam_clip_load(file, &clip_id);
        if (result != iam_ok) {
            printf("Failed to load: %s\n", file);
        }
    }
}

void SaveAnimationLibrary() {
    // Save all registered clips
    ImGuiID clip_ids[] = {
        ImHashStr("bounce"),
        ImHashStr("fade_in"),
        ImHashStr("slide_left"),
        ImHashStr("shake"),
    };

    const char* files[] = {
        "animations/bounce.ianim",
        "animations/fade_in.ianim",
        "animations/slide_left.ianim",
        "animations/shake.ianim",
    };

    for (int i = 0; i < 4; i++) {
        iam_clip_save(clip_ids[i], files[i]);
    }
}
```

## Memory Management

### Pre-allocation

Reserve pool capacity upfront to avoid runtime allocations:

```cpp
// Reserve capacity for tween pools
iam_reserve(
    1000,   // float tweens
    500,    // vec2 tweens
    200,    // vec4 tweens
    100,    // int tweens
    300     // color tweens
);
```

### Garbage Collection

Clean up stale animation data:

```cpp
// Every few seconds (e.g., every 300 frames)
static int gc_counter = 0;
if (++gc_counter >= 300) {
    gc_counter = 0;

    // Remove tweens inactive for 600+ frames
    iam_gc(600);

    // Remove clip instances inactive for 600+ frames
    iam_clip_gc(600);
}
```

### Clip System Init/Shutdown

```cpp
// Optional explicit initialization
iam_clip_init(
    256,    // Initial clip capacity
    4096    // Initial instance capacity
);

// Shutdown (cleans up all clip data)
iam_clip_shutdown();
```

## Easing LUT Configuration

Parametric easings (bezier, spring) use lookup tables for performance. Configure resolution:

```cpp
// Set LUT resolution (default: 256)
iam_set_ease_lut_samples(512);  // Higher quality

// Lower for memory-constrained scenarios
iam_set_ease_lut_samples(128);
```

## Global Time Scale

Slow down or speed up all animations globally:

```cpp
// Slow motion (50% speed)
iam_set_global_time_scale(0.5f);

// Double speed
iam_set_global_time_scale(2.0f);

// Normal speed
iam_set_global_time_scale(1.0f);

// Query current scale
float scale = iam_get_global_time_scale();
```

### Example: Debug Slow-Mo Toggle

```cpp
static bool slow_mo = false;

if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
    slow_mo = !slow_mo;
    iam_set_global_time_scale(slow_mo ? 0.25f : 1.0f);
}
```

## Debug Inspector

Show the unified debug inspector:

```cpp
static bool show_inspector = false;

if (ImGui::IsKeyPressed(ImGuiKey_F12)) {
    show_inspector = !show_inspector;
}

if (show_inspector) {
    iam_show_unified_inspector(&show_inspector);
}
```

The inspector shows:
- Active tweens by type (float, vec2, vec4, int, color)
- Active clip instances
- Global time scale control
- Memory usage statistics
- Profiler data (if enabled)

### Debug Timeline

Show timeline for a specific clip instance:

```cpp
// Show debug timeline for instance
iam_show_debug_timeline(ImHashStr("my_instance"));
```

## API Reference

| Function | Description |
|----------|-------------|
| `iam_profiler_enable(bool)` | Enable/disable profiler |
| `iam_profiler_is_enabled()` | Check profiler state |
| `iam_profiler_begin_frame()` | Start frame profiling |
| `iam_profiler_end_frame()` | End frame profiling |
| `iam_profiler_begin(name)` | Start named section |
| `iam_profiler_end()` | End current section |
| `iam_set_lazy_init(bool)` | Enable/disable lazy init |
| `iam_is_lazy_init_enabled()` | Check lazy init state |
| `iam_clip_save(id, path)` | Save clip to file |
| `iam_clip_load(path, out_id)` | Load clip from file |
| `iam_reserve(...)` | Pre-allocate pool capacity |
| `iam_gc(max_age)` | Garbage collect tweens |
| `iam_clip_gc(max_age)` | Garbage collect instances |
| `iam_clip_init(clips, instances)` | Initialize clip system |
| `iam_clip_shutdown()` | Shutdown clip system |
| `iam_set_ease_lut_samples(n)` | Set LUT resolution |
| `iam_set_global_time_scale(s)` | Set global time scale |
| `iam_get_global_time_scale()` | Get global time scale |
| `iam_show_unified_inspector()` | Show debug inspector |
| `iam_show_debug_timeline(id)` | Show instance timeline |

## See Also

- [Debug](debug.md) - Debug tools and inspector
- [Integration](integration.md) - Setup and initialization
