# Integration Guide

This guide explains how to integrate ImAnim into your existing Dear ImGui project.

## Step 1: Add Files

Copy these two files to your project:

```
src/im_anim.h
src/im_anim.cpp
```

Ensure Dear ImGui headers are in your include path. That's it - no other dependencies.

## Step 2: Include Header

```cpp
#include "im_anim.h"
```

## Step 3: Frame Lifecycle

ImAnim requires two function calls each frame, placed **after** `ImGui::NewFrame()`:

```cpp
// Your main loop
while (running)
{
    // ... platform/input handling ...

    ImGui::NewFrame();

    // >>> ImAnim frame setup (required) <<<
    iam_update_begin_frame();           // Reset per-frame state
    iam_clip_update(io.DeltaTime);      // Advance clip timelines

    // Your UI code with animations
    MyUI();

    ImGui::Render();
    // ... rendering ...
}
```

### Why This Order?

1. **`iam_update_begin_frame()`** - Resets internal frame counters and prepares the animation system. Must be called before any `iam_tween_*` or `iam_*` animation functions.

2. **`iam_clip_update(dt)`** - Advances all active clip timelines by `dt` seconds. Call this even if you're not using clips yet.

## Complete Integration Example

Here's a minimal integration into an existing ImGui application:

```cpp
#include "imgui.h"
#include "im_anim.h"

// Your existing main loop
void MainLoop()
{
    ImGuiIO& io = ImGui::GetIO();

    while (running)
    {
        // Platform/backend new frame
        ImGui_ImplXXX_NewFrame();
        ImGui::NewFrame();

        // ImAnim frame setup
        iam_update_begin_frame();
        iam_clip_update(io.DeltaTime);

        // Your UI code - now with animations!
        ShowMyWindow();

        // Render
        ImGui::Render();
        ImGui_ImplXXX_RenderDrawData(ImGui::GetDrawData());
    }
}
```

## Using Tweens

Tweens are immediate-mode animations. Call them every frame and they return the current animated value:

```cpp
void ShowMyWindow()
{
    ImGui::Begin("My Window");

    if (ImGui::Button("Hover Me"))
    {
        // Button clicked
    }

    // Animate button alpha based on hover state
    bool hovered = ImGui::IsItemHovered();
    float alpha = iam_tween_float(
        ImGui::GetID("btn"),             // Unique ID for this animation
        ImHashStr("alpha"),              // Channel name
        hovered ? 1.0f : 0.6f,           // Target value
        0.2f,                            // Duration (seconds)
        iam_ease_preset(iam_ease_out_cubic),
        iam_policy_crossfade,
        ImGui::GetIO().DeltaTime
    );

    // Use the animated value
    ImGui::GetStyle().Alpha = alpha;
    // ... or for custom drawing

    ImGui::End();
}
```

## Using Clips

Clips are timeline-based animations defined once and played on demand:

```cpp
// Define clips once (e.g., at startup or first use)
void DefineClips()
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    iam_clip::begin(ImHashStr("bounce"))
        .key_float(ImHashStr("scale"), 0.0f, 1.0f, iam_ease_out_elastic)
        .key_float(ImHashStr("scale"), 0.5f, 1.2f, iam_ease_in_out_cubic)
        .key_float(ImHashStr("scale"), 1.0f, 1.0f, iam_ease_out_bounce)
        .end();
}

// Play and query clips each frame
void ShowAnimatedWidget()
{
    DefineClips();

    static iam_instance inst;

    if (ImGui::Button("Play Animation"))
    {
        inst = iam_play(ImHashStr("bounce"), ImHashStr("widget1"));
    }

    float scale = 1.0f;
    if (inst.is_valid())
    {
        inst.get_float(ImHashStr("scale"), &scale);
    }

    // Use scale for your widget
}
```

## ID System

ImAnim uses ImGui's ID system. Each animation needs:

1. **Owner ID** - Identifies the widget/element being animated
2. **Channel ID** - Identifies which property is animated (e.g., "alpha", "scale", "pos")

```cpp
// Good: Unique IDs per element
for (int i = 0; i < items.size(); i++)
{
    ImGui::PushID(i);
    ImGuiID owner = ImGui::GetID("item");
    float alpha = iam_tween_float(owner, ImHashStr("alpha"), ...);
    ImGui::PopID();
}

// Also good: Hash-based IDs
ImGuiID owner = ImHashStr("my_unique_widget");
```

## Memory Management (Optional)

ImAnim manages memory automatically, but you can optimize for your workload:

### Pre-allocation

If you know your animation counts upfront:

```cpp
// At startup
iam_reserve(
    1000,   // float tweens
    500,    // vec2 tweens
    200,    // vec4 tweens
    100,    // int tweens
    300     // color tweens
);
```

### Garbage Collection

Stale animations are cleaned up automatically, but you can trigger it explicitly:

```cpp
// Every few seconds (e.g., every 300 frames at 60fps)
static int gc_counter = 0;
if (++gc_counter >= 300)
{
    gc_counter = 0;
    iam_gc(600);        // Remove tweens inactive for 600+ frames
    iam_clip_gc(600);   // Remove clip instances inactive for 600+ frames
}
```

## Common Patterns

### Hover Animation

```cpp
bool hovered = ImGui::IsItemHovered();
float scale = iam_tween_float(id, ImHashStr("scale"),
    hovered ? 1.1f : 1.0f, 0.15f,
    iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);
```

### Click Feedback

```cpp
if (ImGui::Button("Click"))
{
    // Trigger animation on click
    clicked_time = ImGui::GetTime();
}
float progress = ImClamp((float)(ImGui::GetTime() - clicked_time) / 0.3f, 0.0f, 1.0f);
```

### Staggered List Animation

```cpp
for (int i = 0; i < count; i++)
{
    float delay = i * 0.05f;  // 50ms between items
    float alpha = iam_tween_float(ImGui::GetID(i), ImHashStr("alpha"),
        visible ? 1.0f : 0.0f, 0.3f,
        iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);
}
```

## Build System Integration

### CMake

```cmake
add_executable(myapp
    main.cpp
    # ... your files ...
    path/to/im_anim.cpp
)
target_include_directories(myapp PRIVATE path/to/im_anim)
```

### Visual Studio

1. Add `im_anim.h` and `im_anim.cpp` to your project
2. Ensure ImGui headers are in your include path
3. Build

### Makefile

```makefile
SOURCES += im_anim.cpp
INCLUDES += -Ipath/to/im_anim
```

## Next Steps

- [Quick Start](quickstart.md) - More animation examples
- [Tweens](tweens.md) - All tween types and options
- [Clips](clips.md) - Timeline-based animations
- [Easing](easing.md) - All 30+ easing functions
- [Debug](debug.md) - Inspector and troubleshooting