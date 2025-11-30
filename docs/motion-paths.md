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

## See Also

- [Text Along Paths](text-along-paths.md) - Render text along curves
- [Easing](easing.md) - Control path animation timing
