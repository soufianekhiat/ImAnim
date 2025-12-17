# Repeat Variations

Change animation parameters on each loop iteration.

## Overview

Variations modify keyframe values across loop iterations. This creates evolving animations without defining multiple clips - values can increment, randomize, or follow custom patterns.

## Variation Modes

```cpp
enum iam_variation_mode {
    iam_var_none,        // No variation
    iam_var_increment,   // Add amount each iteration
    iam_var_decrement,   // Subtract amount each iteration
    iam_var_multiply,    // Multiply by factor each iteration
    iam_var_random,      // Random in range [-amount, +amount]
    iam_var_random_abs,  // Random in range [0, amount]
    iam_var_pingpong,    // Alternate +/- each iteration
    iam_var_callback     // Use custom callback
};
```

## Float Variation

```cpp
// Helper functions for float variations
iam_variation_float iam_varf_none()              // No variation
iam_variation_float iam_varf_inc(float amt)      // Increment by amt
iam_variation_float iam_varf_dec(float amt)      // Decrement by amt
iam_variation_float iam_varf_mul(float f)        // Multiply by f
iam_variation_float iam_varf_rand(float r)       // Random [-r, +r]
iam_variation_float iam_varf_rand_abs(float r)   // Random [0, r]
iam_variation_float iam_varf_pingpong(float amt) // Alternate +/-
iam_variation_float iam_varf_fn(callback, user)  // Custom callback
iam_variation_float iam_varf_clamp(v, min, max)  // Add clamp range
iam_variation_float iam_varf_seed(v, seed)       // Set random seed
```

### Example: Growing Scale

```cpp
iam_clip::begin(ImHashStr("grow"))
    .key_float_var(
        ImHashStr("scale"),
        0.0f, 1.0f,
        iam_varf_inc(0.2f),  // +0.2 each loop: 1.0, 1.2, 1.4, ...
        iam_ease_out_cubic
    )
    .key_float_var(
        ImHashStr("scale"),
        1.0f, 1.5f,
        iam_varf_inc(0.2f),  // End value also grows
        iam_ease_in_out_cubic
    )
    .set_loop(true, iam_dir_normal, 5)  // Loop 5 times
    .end();
```

### Example: Random Shake Intensity

```cpp
iam_clip::begin(ImHashStr("shake"))
    .key_float_var(
        ImHashStr("offset_x"),
        0.0f, 10.0f,
        iam_varf_rand(5.0f),  // Random variance [-5, +5]
        iam_ease_out_quad
    )
    .set_loop(true, iam_dir_alternate)
    .end();
```

## Int Variation

```cpp
iam_variation_int iam_vari_none()
iam_variation_int iam_vari_inc(int amt)
iam_variation_int iam_vari_dec(int amt)
iam_variation_int iam_vari_rand(int r)
iam_variation_int iam_vari_fn(callback, user)
iam_variation_int iam_vari_clamp(v, min, max)
iam_variation_int iam_vari_seed(v, seed)
```

### Example: Frame Counter

```cpp
iam_clip::begin(ImHashStr("frame_anim"))
    .key_int_var(
        ImHashStr("frame"),
        0.0f, 0,
        iam_vari_inc(1),     // Frame 0, 1, 2, 3, ...
        iam_ease_linear
    )
    .set_loop(true, iam_dir_normal)
    .end();
```

## Vec2 Variation

```cpp
// Global mode (same variation for both axes)
iam_variation_vec2 iam_varv2_none()
iam_variation_vec2 iam_varv2_inc(float x, float y)
iam_variation_vec2 iam_varv2_dec(float x, float y)
iam_variation_vec2 iam_varv2_mul(float f)
iam_variation_vec2 iam_varv2_rand(float x, float y)
iam_variation_vec2 iam_varv2_fn(callback, user)

// Per-axis mode (different variation per axis)
iam_variation_vec2 iam_varv2_axis(vx, vy)

iam_variation_vec2 iam_varv2_clamp(v, min, max)
iam_variation_vec2 iam_varv2_seed(v, seed)
```

### Example: Expanding Position

```cpp
iam_clip::begin(ImHashStr("expand"))
    .key_vec2_var(
        ImHashStr("pos"),
        0.0f, ImVec2(100, 100),
        iam_varv2_inc(10, 10),  // Move further each loop
        iam_ease_out_back
    )
    .set_loop(true, iam_dir_normal, 10)
    .end();
```

### Example: Per-Axis Variation

```cpp
// X increments, Y randomizes
iam_variation_vec2 var = iam_varv2_axis(
    iam_varf_inc(5.0f),      // X: +5 each loop
    iam_varf_rand(10.0f)     // Y: random [-10, +10]
);

iam_clip::begin(ImHashStr("mixed"))
    .key_vec2_var(ImHashStr("pos"), 0.0f, ImVec2(50, 50), var)
    .set_loop(true)
    .end();
```

## Vec4 Variation

```cpp
iam_variation_vec4 iam_varv4_none()
iam_variation_vec4 iam_varv4_inc(float x, y, z, w)
iam_variation_vec4 iam_varv4_dec(float x, y, z, w)
iam_variation_vec4 iam_varv4_mul(float f)
iam_variation_vec4 iam_varv4_rand(float x, y, z, w)
iam_variation_vec4 iam_varv4_fn(callback, user)

// Per-axis mode
iam_variation_vec4 iam_varv4_axis(vx, vy, vz, vw)

iam_variation_vec4 iam_varv4_clamp(v, min, max)
iam_variation_vec4 iam_varv4_seed(v, seed)
```

## Color Variation

```cpp
iam_variation_color iam_varc_none()
iam_variation_color iam_varc_inc(float r, g, b, a)
iam_variation_color iam_varc_dec(float r, g, b, a)
iam_variation_color iam_varc_mul(float f)
iam_variation_color iam_varc_rand(float r, g, b, a)
iam_variation_color iam_varc_fn(callback, user)

// Per-channel mode
iam_variation_color iam_varc_channel(vr, vg, vb, va)

iam_variation_color iam_varc_space(v, color_space)
iam_variation_color iam_varc_clamp(v, min, max)
iam_variation_color iam_varc_seed(v, seed)
```

### Example: Fading Color

```cpp
iam_clip::begin(ImHashStr("fade_pulse"))
    .key_color_var(
        ImHashStr("color"),
        0.0f, ImVec4(1, 0, 0, 1),
        iam_varc_dec(0, 0, 0, 0.1f),  // Alpha decreases each loop
        iam_col_oklab
    )
    .set_loop(true, iam_dir_alternate)
    .end();
```

### Example: Color Shift

```cpp
// Hue rotates through colors
iam_variation_color hue_shift = iam_varc_channel(
    iam_varf_inc(0.1f),    // R shifts
    iam_varf_inc(0.05f),   // G shifts slower
    iam_varf_dec(0.1f),    // B shifts opposite
    iam_varf_none()        // A stays constant
);

iam_clip::begin(ImHashStr("rainbow"))
    .key_color_var(ImHashStr("col"), 0.0f, ImVec4(1, 0, 0, 1), hue_shift)
    .set_loop(true)
    .end();
```

## Timing Variations

Vary timing parameters per loop:

```cpp
iam_clip::begin(ImHashStr("timing_var"))
    .key_float(ImHashStr("val"), 0.0f, 0.0f)
    .key_float(ImHashStr("val"), 1.0f, 100.0f)
    .set_loop(true, iam_dir_normal, 10)
    .set_duration_var(iam_varf_mul(0.9f))    // 10% faster each loop
    .set_delay_var(iam_varf_inc(0.1f))       // +0.1s delay between loops
    .set_timescale_var(iam_varf_rand(0.2f))  // Random speed variation
    .end();
```

## Custom Callbacks

```cpp
float my_variation(int index, void* user) {
    // index = loop iteration (0, 1, 2, ...)
    // Return the variation delta to apply
    return sinf(index * 0.5f) * 10.0f;
}

iam_clip::begin(ImHashStr("custom"))
    .key_float_var(
        ImHashStr("value"),
        0.0f, 50.0f,
        iam_varf_fn(my_variation, nullptr)
    )
    .set_loop(true)
    .end();
```

## Clamping Values

Prevent values from exceeding bounds:

```cpp
// Scale grows but never exceeds 3.0
iam_variation_float scale_var = iam_varf_clamp(
    iam_varf_inc(0.5f),
    0.5f,   // min
    3.0f    // max
);

iam_clip::begin(ImHashStr("clamped"))
    .key_float_var(ImHashStr("scale"), 0.0f, 1.0f, scale_var)
    .set_loop(true)
    .end();
```

## Deterministic Random

Use seeds for reproducible randomness:

```cpp
// Same random sequence every time
iam_variation_float rand_var = iam_varf_seed(
    iam_varf_rand(10.0f),
    12345  // Fixed seed
);
```

## API Reference

| Helper | Description |
|--------|-------------|
| `iam_varf_*` | Float variation helpers |
| `iam_vari_*` | Int variation helpers |
| `iam_varv2_*` | Vec2 variation helpers |
| `iam_varv4_*` | Vec4 variation helpers |
| `iam_varc_*` | Color variation helpers |
| `key_*_var(...)` | Keyframe with variation |
| `set_duration_var(...)` | Duration variation |
| `set_delay_var(...)` | Loop delay variation |
| `set_timescale_var(...)` | Playback speed variation |

## See Also

- [Clips](clips.md) - Timeline animations
- [Stagger](stagger.md) - Cascading animations
