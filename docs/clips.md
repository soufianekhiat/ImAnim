# Clips

Timeline-based animations with keyframes, looping, callbacks, and chaining.

## Overview

Clips are pre-defined animation timelines. Define them once, play them multiple times with different instances.

## Defining Clips

```cpp
// Define channel IDs (do this once)
static const ImGuiID CH_SCALE = ImHashStr("scale");
static const ImGuiID CH_ALPHA = ImHashStr("alpha");
static const ImGuiID CLIP_BOUNCE = ImHashStr("bounce");

// Define the clip
iam_clip::begin(CLIP_BOUNCE)
    .key_float(CH_SCALE, 0.0f, 1.0f, iam_ease_out_elastic)
    .key_float(CH_SCALE, 1.0f, 1.5f, iam_ease_in_out_cubic)
    .key_float(CH_SCALE, 2.0f, 1.0f, iam_ease_out_bounce)
    .key_float(CH_ALPHA, 0.0f, 0.0f, iam_ease_out_quad)
    .key_float(CH_ALPHA, 0.5f, 1.0f, iam_ease_out_quad)
    .end();
```

## Keyframe Types

```cpp
.key_float(channel, time, value, ease_type)
.key_vec2(channel, time, ImVec2(x, y), ease_type)
.key_vec4(channel, time, ImVec4(x, y, z, w), ease_type)
.key_int(channel, time, value, ease_type)
.key_color(channel, time, ImVec4(r, g, b, a), color_space, ease_type)

// Physics-based spring keyframes
.key_float_spring(channel, time, target, spring_params)

// Anchor-relative keyframes (see Anchors doc)
.key_float_rel(channel, time, percent, px_bias, anchor_space, axis, ease_type)
.key_vec2_rel(channel, time, percent, px_bias, anchor_space, ease_type)
```

## Playing Clips

```cpp
// Start playback
iam_instance inst = iam_play(CLIP_BOUNCE, ImHashStr("widget_1"));

// Query animated values
float scale, alpha;
inst.get_float(CH_SCALE, &scale);
inst.get_float(CH_ALPHA, &alpha);
```

## Playback Control

```cpp
iam_instance inst = iam_get_instance(ImHashStr("widget_1"));

inst.pause();              // Pause playback
inst.resume();             // Resume from pause
inst.seek(0.5f);           // Jump to time (seconds)
inst.set_time_scale(2.0f); // Double speed
inst.stop();               // Stop but keep instance
inst.destroy();            // Remove instance entirely

// Query state
bool playing = inst.is_playing();
bool paused = inst.is_paused();
float time = inst.get_time();
```

## Looping

```cpp
// Loop forever, forward
.set_loop(true, iam_dir_normal)

// Loop forever, backward
.set_loop(true, iam_dir_reverse)

// Ping-pong (alternate direction each loop)
.set_loop(true, iam_dir_alternate)

// Loop exactly 3 times
.set_loop(true, iam_dir_normal, 3)
```

## Callbacks

```cpp
iam_clip::begin(CLIP_ID)
    .key_float(CH, 0.0f, 0.0f, iam_ease_linear)
    .key_float(CH, 1.0f, 1.0f, iam_ease_linear)
    .on_begin([](ImGuiID inst_id, void* user) {
        printf("Animation started!\n");
    }, nullptr)
    .on_update([](ImGuiID inst_id, void* user) {
        // Called every frame during playback
    }, nullptr)
    .on_complete([](ImGuiID inst_id, void* user) {
        printf("Animation finished!\n");
    }, nullptr)
    .end();
```

## Timeline Markers

Trigger callbacks at specific times:

```cpp
void on_marker(ImGuiID inst_id, ImGuiID marker_id, float marker_time, void* user) {
    printf("Marker hit at %.2fs!\n", marker_time);
}

iam_clip::begin(CLIP_ID)
    .key_float(CH, 0.0f, 0.0f, iam_ease_linear)
    .key_float(CH, 3.0f, 100.0f, iam_ease_linear)
    .marker(0.5f, on_marker)                          // Auto-generated ID
    .marker(1.0f, ImHashStr("checkpoint"), on_marker) // Named marker
    .marker(2.0f, on_marker, my_user_data)            // With user data
    .end();
```

Markers fire:
- During normal playback when time crosses the marker
- During reverse playback
- On each loop iteration

## Animation Chaining

Chain clips to play in sequence:

```cpp
// Play A, then automatically start B when A completes
iam_play(clip_a, ImHashStr("inst_a"))
    .then(clip_b)                      // Auto-generates instance ID
    .then_delay(0.1f);                 // Optional delay before B starts

// Chain with explicit instance IDs
iam_play(clip_a, ImHashStr("inst_a"))
    .then(clip_b, ImHashStr("inst_b"))
    .then_delay(0.2f);

// Build longer chains
iam_instance inst_b = iam_get_instance(ImHashStr("inst_b"));
if (inst_b.is_playing()) {
    inst_b.then(clip_c, ImHashStr("inst_c"));
}
```

## Timeline Grouping

Organize keyframes into sequential and parallel blocks.

### Sequential Blocks

Keyframes after `seq_end()` start after the sequential block completes:

```cpp
iam_clip::begin(ImHashStr("sequence"))
    .seq_begin()
        .key_float(CH_X, 0.0f, 0.0f, iam_ease_out_cubic)
        .key_float(CH_X, 1.0f, 100.0f, iam_ease_out_cubic)  // X moves first
    .seq_end()
    .key_float(CH_Y, 0.0f, 0.0f, iam_ease_out_cubic)        // Y starts after X finishes
    .key_float(CH_Y, 1.0f, 100.0f, iam_ease_out_cubic)
    .end();
```

### Parallel Blocks

Keyframes in `par_begin/end` play at the same time offset:

```cpp
iam_clip::begin(ImHashStr("parallel"))
    .par_begin()
        .key_float(CH_X, 0.0f, 0.0f, iam_ease_out_cubic)
        .key_float(CH_X, 0.5f, 100.0f, iam_ease_out_cubic)
        .key_float(CH_Y, 0.0f, 0.0f, iam_ease_out_cubic)
        .key_float(CH_Y, 0.5f, 100.0f, iam_ease_out_cubic)
    .par_end()  // X and Y animate together
    .key_float(CH_ALPHA, 0.0f, 0.0f, iam_ease_linear)
    .key_float(CH_ALPHA, 0.3f, 1.0f, iam_ease_linear)       // Alpha starts after par block
    .end();
```

### Nested Blocks

```cpp
iam_clip::begin(ImHashStr("complex"))
    .seq_begin()
        .par_begin()
            .key_float(CH_X, 0.0f, 0.0f)
            .key_float(CH_X, 0.3f, 50.0f)
            .key_float(CH_Y, 0.0f, 0.0f)
            .key_float(CH_Y, 0.3f, 50.0f)
        .par_end()  // X and Y move together
        .key_float(CH_SCALE, 0.0f, 1.0f)
        .key_float(CH_SCALE, 0.2f, 1.5f)  // Then scale
    .seq_end()
    .key_float(CH_ALPHA, 0.0f, 1.0f)  // Then fade
    .key_float(CH_ALPHA, 0.3f, 0.0f)
    .end();
```

## Stagger Clips

Cascading effects for multiple elements:

```cpp
// Define stagger clip for 12 items
iam_clip::begin(CLIP_STAGGER)
    .key_float(CH_ALPHA, 0.0f, 0.0f, iam_ease_out_quad)
    .key_float(CH_ALPHA, 0.3f, 1.0f, iam_ease_out_quad)
    .key_float(CH_POS_Y, 0.0f, 40.0f, iam_ease_out_back)
    .key_float(CH_POS_Y, 0.5f, 0.0f, iam_ease_out_back)
    .set_stagger(12, 0.06f, 0.0f)  // count, delay, center_bias
    .end();

// Play with stagger index
for (int i = 0; i < 12; i++) {
    iam_play_stagger(CLIP_STAGGER, inst_ids[i], i);
}
```

See [Stagger](stagger.md) for more details.

## Memory Management

```cpp
// Garbage collect old clip instances
iam_clip_gc(600);  // Remove instances older than 600 frames
```

## See Also

- [Stagger](stagger.md) - Cascading animations
- [Easing](easing.md) - All easing functions
- [Anchors](anchors.md) - Anchor-relative keyframes
