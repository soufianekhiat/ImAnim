# Quick Start

Get up and running with ImAnim in minutes.

## Installation

ImAnim is a single header+source pair. Add to your project:

```
src/im_anim.h
src/im_anim.cpp
```

Requires Dear ImGui headers in your include path.

## Basic Setup

```cpp
#include "im_anim.h"

// In your main loop, before any animation code:
while (running) {
    // ... platform/graphics setup ...

    ImGui::NewFrame();

    // Update animation systems (required every frame)
    iam_update_begin_frame();
    iam_clip_update(ImGui::GetIO().DeltaTime);

    // Your UI code with animations
    MyAnimatedUI();

    ImGui::Render();
    // ... render ...
}
```

## Your First Tween

Tweens are immediate-mode animations that smoothly interpolate values:

```cpp
float dt = ImGui::GetIO().DeltaTime;
bool hovered = ImGui::IsItemHovered();

// Animate alpha based on hover state
float alpha = iam_tween_float(
    ImGui::GetID("MyWidget"),           // Owner ID (widget identity)
    ImHashStr("alpha"),                 // Channel ID (property name)
    hovered ? 1.0f : 0.5f,              // Target value
    0.3f,                               // Duration in seconds
    iam_ease_preset(iam_ease_out_cubic),// Easing function
    iam_policy_crossfade,               // Transition policy
    dt                                  // Delta time
);
```

## Your First Clip

Clips are timeline-based animations with keyframes:

```cpp
// Define once at startup
static const ImGuiID CH_SCALE = ImHashStr("scale");
static const ImGuiID CLIP_BOUNCE = ImHashStr("bounce");

iam_clip::begin(CLIP_BOUNCE)
    .key_float(CH_SCALE, 0.0f, 1.0f, iam_ease_out_elastic)
    .key_float(CH_SCALE, 1.0f, 1.5f, iam_ease_in_out_cubic)
    .key_float(CH_SCALE, 2.0f, 1.0f, iam_ease_out_bounce)
    .set_loop(true, iam_dir_alternate)
    .end();

// Play the clip
iam_instance inst = iam_play(CLIP_BOUNCE, ImHashStr("widget_1"));

// Query animated values each frame
float scale;
inst.get_float(CH_SCALE, &scale);
```

## Next Steps

- [Tweens](tweens.md) - Learn all tween types and options
- [Clips](clips.md) - Master timeline-based animations
- [Easing](easing.md) - Explore all easing functions
- [Motion Paths](motion-paths.md) - Animate along curves
