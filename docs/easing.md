# Easing

Over 30 easing functions for natural-feeling animations.

## Overview

Easing functions control the rate of change during an animation. Linear animations feel robotic; eased animations feel natural.

## Standard Presets

Use `iam_ease_preset()` to create an ease descriptor:

```cpp
iam_ease_preset(iam_ease_out_cubic)
```

### All Presets

| Category | In | Out | InOut |
|----------|-----|-----|-------|
| Linear | `iam_ease_linear` | - | - |
| Quad | `iam_ease_in_quad` | `iam_ease_out_quad` | `iam_ease_in_out_quad` |
| Cubic | `iam_ease_in_cubic` | `iam_ease_out_cubic` | `iam_ease_in_out_cubic` |
| Quart | `iam_ease_in_quart` | `iam_ease_out_quart` | `iam_ease_in_out_quart` |
| Quint | `iam_ease_in_quint` | `iam_ease_out_quint` | `iam_ease_in_out_quint` |
| Sine | `iam_ease_in_sine` | `iam_ease_out_sine` | `iam_ease_in_out_sine` |
| Expo | `iam_ease_in_expo` | `iam_ease_out_expo` | `iam_ease_in_out_expo` |
| Circ | `iam_ease_in_circ` | `iam_ease_out_circ` | `iam_ease_in_out_circ` |
| Back | `iam_ease_in_back` | `iam_ease_out_back` | `iam_ease_in_out_back` |
| Elastic | `iam_ease_in_elastic` | `iam_ease_out_elastic` | `iam_ease_in_out_elastic` |
| Bounce | `iam_ease_in_bounce` | `iam_ease_out_bounce` | `iam_ease_in_out_bounce` |

### In vs Out vs InOut

- **In** - Slow start, fast end
- **Out** - Fast start, slow end (most common for UI)
- **InOut** - Slow start and end, fast middle

## Cubic Bezier

Define custom curves like CSS `cubic-bezier()`:

```cpp
iam_ease_bezier(0.25f, 0.1f, 0.25f, 1.0f)  // ease (CSS default)
iam_ease_bezier(0.42f, 0.0f, 1.0f, 1.0f)   // ease-in
iam_ease_bezier(0.0f, 0.0f, 0.58f, 1.0f)   // ease-out
iam_ease_bezier(0.42f, 0.0f, 0.58f, 1.0f)  // ease-in-out
```

The four parameters are control point coordinates: `(x1, y1, x2, y2)`.

## Step Functions

Discrete stepping like CSS `steps()`:

```cpp
iam_ease_steps_desc(5, 0)   // 5 steps, jump-end (default)
iam_ease_steps_desc(5, 1)   // 5 steps, jump-start
iam_ease_steps_desc(10, 0)  // 10 steps
```

## Back Easing

Overshoot with customizable intensity:

```cpp
iam_ease_back(1.70158f)  // Default overshoot
iam_ease_back(3.0f)      // More dramatic overshoot
iam_ease_back(1.0f)      // Subtle overshoot
```

## Elastic Easing

Springy bounce with amplitude and period control:

```cpp
iam_ease_elastic(1.0f, 0.3f)   // Default (amplitude, period)
iam_ease_elastic(1.5f, 0.3f)   // Higher amplitude
iam_ease_elastic(1.0f, 0.2f)   // Faster oscillation
```

## Spring Physics

True physics-based spring simulation:

```cpp
iam_ease_spring_desc(
    1.0f,    // mass
    100.0f,  // stiffness
    10.0f,   // damping
    0.0f     // initial velocity
)
```

Spring parameters:
- **mass** - Higher = slower, more momentum
- **stiffness** - Higher = faster, snappier
- **damping** - Higher = less oscillation
- **velocity** - Initial velocity boost

## Custom Easing Functions

Register your own easing functions:

```cpp
// Define custom function
float MyCustomEase(float t) {
    return t * t * (3.0f - 2.0f * t);  // smoothstep
}

// Register in slot 0-15
iam_register_custom_ease(0, MyCustomEase);

// Use in animations
float val = iam_tween_float(id, ch, target, dur,
    iam_ease_custom_fn(0), iam_policy_crossfade, dt);
```

## Common Recipes

### UI Hover/Focus

```cpp
// Snappy but smooth
iam_ease_preset(iam_ease_out_cubic)
```

### Button Press

```cpp
// Quick response with slight bounce
iam_ease_preset(iam_ease_out_back)
```

### Modal/Popup Enter

```cpp
// Dramatic entrance
iam_ease_preset(iam_ease_out_elastic)
```

### Modal/Popup Exit

```cpp
// Quick exit
iam_ease_preset(iam_ease_in_cubic)
```

### Loading Spinner

```cpp
// Smooth continuous motion
iam_ease_preset(iam_ease_in_out_sine)
```

### Game Character Jump

```cpp
// Natural arc
iam_ease_preset(iam_ease_out_quad)  // Up
iam_ease_preset(iam_ease_in_quad)   // Down
```

## Performance

Easing functions use lookup tables for performance:

```cpp
// Configure LUT resolution (default: 256)
iam_set_ease_lut_samples(512);  // More accurate
iam_set_ease_lut_samples(128);  // Faster, less accurate
```

## See Also

- [Tweens](tweens.md) - Using easing with tweens
- [Clips](clips.md) - Using easing with keyframes
