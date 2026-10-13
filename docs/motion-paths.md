# Motion Paths

Animate positions along bezier curves and Catmull-Rom splines.

## Overview

Motion paths let you animate objects along complex curves rather than straight lines. Define a path once, then animate along it.

## Defining Paths

### Quadratic Bezier

```cpp
iam_path::begin(ImHashStr("my_path"), ImVec2(0, 0))   // Start point
    .quadratic_to(ImVec2(50, -50), ImVec2(100, 0))   // Control, End
    .quadratic_to(ImVec2(150, 50), ImVec2(200, 0))
    .end();
```

### Cubic Bezier

```cpp
iam_path::begin(ImHashStr("cubic_path"), ImVec2(0, 50))
    .cubic_to(ImVec2(30, 0), ImVec2(70, 100), ImVec2(100, 50))   // Ctrl1, Ctrl2, End
    .cubic_to(ImVec2(130, 0), ImVec2(170, 100), ImVec2(200, 50))
    .end();
```

### Catmull-Rom Spline

Smooth curves that pass through all control points:

```cpp
iam_path::begin(ImHashStr("smooth_path"), ImVec2(0, 50))
    .catmull_to(ImVec2(50, 20))            // Curve passes through these points
    .catmull_to(ImVec2(100, 80))
    .catmull_to(ImVec2(150, 50))
    .catmull_to(ImVec2(200, 50), 0.7f)     // Custom tension (0.5 = default)
    .end();
```

### Line Segments

```cpp
iam_path::begin(ImHashStr("mixed"), ImVec2(0, 0))
    .line_to(ImVec2(50, 0))                // Straight line
    .quadratic_to(ImVec2(75, -25), ImVec2(100, 0))
    .line_to(ImVec2(150, 0))
    .end();
```

### Closed Paths

```cpp
iam_path::begin(ImHashStr("closed"), ImVec2(0, 0))
    .quadratic_to(ImVec2(50, -50), ImVec2(100, 0))
    .quadratic_to(ImVec2(50, 50), ImVec2(0, 0))
    .close()  // Connect back to start
    .end();
```

## Animating Along Paths

### Tween Path Position

```cpp
ImVec2 pos = iam_tween_path(
    id, channel_id, path_id,
    duration, ease, policy, dt
);
```

### Tween Path Angle

Get rotation aligned to path tangent:

```cpp
float angle = iam_tween_path_angle(
    id, channel_id, path_id,
    duration, ease, policy, dt
);

// Use for rotation
ImGui::SetCursorScreenPos(pos);
// ... draw rotated element at angle ...
```

## Manual Path Evaluation

Evaluate paths without animation state:

```cpp
// Position at t (0.0 to 1.0)
ImVec2 point = iam_path_evaluate(path_id, t);

// Tangent direction (normalized)
ImVec2 tangent = iam_path_tangent(path_id, t);

// Rotation angle in radians
float angle = iam_path_angle(path_id, t);

// Approximate total length
float length = iam_path_length(path_id);
```

## Arc-Length Parameterization

For constant-speed motion along curves:

```cpp
// Build lookup table (do once after path creation)
iam_path_build_arc_lut(path_id, 128);  // 128 samples

// Check if path has LUT
if (iam_path_has_arc_lut(path_id)) {
    // Evaluate by distance (pixels)
    float distance = 100.0f;
    ImVec2 pos = iam_path_evaluate_at_distance(path_id, distance);
    float angle = iam_path_angle_at_distance(path_id, distance);
    ImVec2 tangent = iam_path_tangent_at_distance(path_id, distance);
}

// Convert between distance and t parameter
float t = iam_path_distance_to_t(path_id, distance);
```

## Direct Curve Functions

For one-off evaluation without building paths:

### Bezier Curves

```cpp
// Quadratic bezier
ImVec2 p = iam_bezier_quadratic(p0, p1, p2, t);
ImVec2 vel = iam_bezier_quadratic_deriv(p0, p1, p2, t);

// Cubic bezier
ImVec2 p = iam_bezier_cubic(p0, p1, p2, p3, t);
ImVec2 vel = iam_bezier_cubic_deriv(p0, p1, p2, p3, t);
```

### Catmull-Rom

```cpp
// Curve passes through p1 and p2
ImVec2 p = iam_catmull_rom(p0, p1, p2, p3, t, tension);
ImVec2 vel = iam_catmull_rom_deriv(p0, p1, p2, p3, t, tension);
```

## Example: Orbiting Object

```cpp
// Define circular-ish path
iam_path::begin(ImHashStr("orbit"), ImVec2(100, 0))
    .cubic_to(ImVec2(100, 55), ImVec2(55, 100), ImVec2(0, 100))
    .cubic_to(ImVec2(-55, 100), ImVec2(-100, 55), ImVec2(-100, 0))
    .cubic_to(ImVec2(-100, -55), ImVec2(-55, -100), ImVec2(0, -100))
    .cubic_to(ImVec2(55, -100), ImVec2(100, -55), ImVec2(100, 0))
    .close()
    .end();

// Animate
static float t = 0.0f;
t += dt * 0.2f;  // Speed
if (t > 1.0f) t -= 1.0f;

ImVec2 center = ImVec2(200, 200);
ImVec2 offset = iam_path_evaluate(ImHashStr("orbit"), t);
ImVec2 pos = ImVec2(center.x + offset.x, center.y + offset.y);
```

## Drawing Paths

`iam_path_draw()` strokes part of a path, `[t0, t1]` as a fraction of its arc
length, so the stroke grows at constant speed:

```cpp
// Draw-on effect
float t = iam_tween_float(id, ImHashStr("draw"), 1.0f, 1.2f, iam_ease_preset(iam_ease_in_out_cubic), iam_policy_crossfade, dt);
iam_path_draw(dl, path_id, 0.0f, t, IM_COL32(91, 194, 231, 255), 3.0f, origin);

// Segment running around a closed path (t1 < t0 wraps)
float a = fmodf(time * 0.4f, 1.0f);
iam_path_draw(dl, loop_id, a, fmodf(a + 0.25f, 1.0f), IM_COL32_WHITE, 4.0f, origin);
```

`iam_path_morph_draw()` does the same on a morph between two paths.
`segments = 0` picks a segment count from the drawn length.

## Dashes and Trails

```cpp
// Marching ants: animate the phase (pixels)
iam_path_draw_dashed(dl, path_id, 0.0f, 1.0f, 8.0f /*dash*/, 6.0f /*gap*/, time * 30.0f /*phase*/, col, 2.0f, origin);

// Comet: fading, tapering stroke behind a point at t_head, over 35% of the path
iam_path_draw_trail(dl, path_id, t_head, 0.35f, col, 5.0f, origin);

// Any moving point (mouse, widget, particle): keeps the last 0.35s of positions
iam_trail_draw(dl, ImGui::GetID("cursor"), ImGui::GetMousePos(), 0.35f, col, 6.0f);
iam_trail_clear(ImGui::GetID("cursor"));   // e.g. after a teleport
```

## Paths from SVG

`iam_path_from_svg()` builds a path from SVG path data (the `d` attribute), so
icons and logos drawn in a vector editor can be used directly:

```cpp
iam_path_from_svg(ImHashStr("heart"),
    "M50 88 C20 65 5 48 5 30 A22 22 0 0 1 50 22 A22 22 0 0 1 95 30 C95 48 80 65 50 88 Z",
    2.0f,                    // scale
    iam_v2(10, 10));         // offset
iam_path_draw(dl, ImHashStr("heart"), 0.0f, t, col, 3.0f, origin);
```

Supports `M L H V C S Q T A Z` and their relative forms; arcs become cubic
beziers. A second `M` (new subpath) is joined with a line, since a path is a
single continuous curve. Returns false and builds nothing on malformed data.

## See Also

- [Text Along Paths](text-along-paths.md) - Render text along curves
- [Easing](easing.md) - Control path animation timing
