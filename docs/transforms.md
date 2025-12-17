# Transforms

Animate 2D transforms (position, rotation, scale) with rotation interpolation modes.

## Overview

The transform system provides a unified way to animate position, rotation, and scale together. It includes multiple rotation interpolation modes for controlling how angles are blended.

## iam_transform Structure

```cpp
struct iam_transform {
    ImVec2 position;    // Translation (x, y)
    ImVec2 scale;       // Scale (1, 1 = identity)
    float rotation;     // Rotation in radians
};

// Create transforms
iam_transform t1;                                          // Identity
iam_transform t2(ImVec2(100, 50));                        // Position only
iam_transform t3(ImVec2(100, 50), 0.5f);                  // Position + rotation
iam_transform t4(ImVec2(100, 50), 0.5f, ImVec2(2, 2));   // Position + rotation + scale
iam_transform identity = iam_transform::identity();
```

## Basic Usage

```cpp
iam_transform target;
target.position = ImVec2(200, 150);
target.rotation = IAM_PI * 0.25f;  // 45 degrees
target.scale = ImVec2(1.5f, 1.5f);

iam_transform current = iam_tween_transform(
    id, ImHashStr("transform"),
    target, 0.5f,
    iam_ease_preset(iam_ease_out_back),
    iam_policy_crossfade,
    iam_rotation_shortest,  // Rotation mode
    dt
);

// Apply transform to draw
ImVec2 center = current.position;
float angle = current.rotation;
ImVec2 scale = current.scale;
```

## Rotation Modes

```cpp
enum iam_rotation_mode {
    iam_rotation_shortest,   // Shortest path (default) - max 180 degrees
    iam_rotation_longest,    // Longest path - takes the long way
    iam_rotation_cw,         // Always clockwise
    iam_rotation_ccw,        // Always counter-clockwise
    iam_rotation_direct      // Direct lerp - no unwrapping
};
```

### Shortest (Default)

```cpp
// From 10 degrees to 350 degrees
// Shortest goes: 10 -> 0 -> 350 (20 degree turn)
iam_tween_transform(..., iam_rotation_shortest, dt);
```

### Longest

```cpp
// From 10 degrees to 350 degrees
// Longest goes: 10 -> 180 -> 350 (340 degree turn)
iam_tween_transform(..., iam_rotation_longest, dt);
```

### Clockwise / Counter-Clockwise

```cpp
// Force rotation direction regardless of distance
iam_tween_transform(..., iam_rotation_cw, dt);   // Always rotate CW
iam_tween_transform(..., iam_rotation_ccw, dt);  // Always rotate CCW
```

### Direct

```cpp
// No angle normalization - can cause spinning for large deltas
iam_tween_transform(..., iam_rotation_direct, dt);
```

## Transform Blending

Interpolate between transforms manually:

```cpp
iam_transform a(ImVec2(0, 0), 0.0f, ImVec2(1, 1));
iam_transform b(ImVec2(100, 100), IAM_PI, ImVec2(2, 2));

// Blend at 50%
iam_transform mid = iam_transform_lerp(a, b, 0.5f, iam_rotation_shortest);
```

## Applying Transforms

```cpp
iam_transform t = iam_tween_transform(...);

// Transform a point
ImVec2 original(10, 10);
ImVec2 transformed = t.apply(original);

// Combine transforms
iam_transform combined = t1 * t2;  // Apply t1, then t2

// Get inverse
iam_transform inv = t.inverse();
```

## Matrix Conversion

```cpp
// Convert to 3x2 matrix (row-major)
float matrix[6];
iam_transform_to_matrix(t, matrix);
// matrix = [m00, m01, tx, m10, m11, ty]

// Create from matrix
iam_transform t = iam_transform_from_matrix(
    m00, m01, m10, m11, tx, ty
);
```

## Example: Spinning Card

```cpp
static float target_rotation = 0.0f;

if (ImGui::Button("Flip Card")) {
    target_rotation += IAM_PI;  // Add 180 degrees
}

iam_transform card_transform;
card_transform.position = ImVec2(150, 100);
card_transform.rotation = target_rotation;
card_transform.scale = ImVec2(1, 1);

iam_transform current = iam_tween_transform(
    ImGui::GetID("card"), ImHashStr("flip"),
    card_transform, 0.4f,
    iam_ease_preset(iam_ease_out_back),
    iam_policy_crossfade,
    iam_rotation_cw,  // Always rotate clockwise
    dt
);

// Draw card with transform
ImDrawList* dl = ImGui::GetWindowDrawList();
ImVec2 center = screen_pos + current.position;

// Calculate rotated corners
ImVec2 half_size(50 * current.scale.x, 70 * current.scale.y);
float cos_r = cosf(current.rotation);
float sin_r = sinf(current.rotation);

ImVec2 corners[4];
corners[0] = center + ImVec2(-half_size.x * cos_r + half_size.y * sin_r,
                             -half_size.x * sin_r - half_size.y * cos_r);
// ... calculate other corners

dl->AddQuadFilled(corners[0], corners[1], corners[2], corners[3], card_color);
```

## Example: Hover Transform

```cpp
bool hovered = ImGui::IsItemHovered();

iam_transform target;
target.position = ImVec2(0, hovered ? -5 : 0);  // Lift on hover
target.rotation = hovered ? 0.05f : 0.0f;       // Slight tilt
target.scale = ImVec2(hovered ? 1.1f : 1.0f, hovered ? 1.1f : 1.0f);

iam_transform hover_fx = iam_tween_transform(
    item_id, ImHashStr("hover"),
    target, 0.2f,
    iam_ease_preset(iam_ease_out_back),
    iam_policy_crossfade,
    iam_rotation_shortest,
    dt
);
```

## Example: Orbit Animation

```cpp
static float orbit_angle = 0.0f;
orbit_angle += dt * 0.5f;  // Constant rotation

float orbit_radius = 80.0f;
iam_transform orbit;
orbit.position = ImVec2(
    cosf(orbit_angle) * orbit_radius,
    sinf(orbit_angle) * orbit_radius
);
orbit.rotation = orbit_angle + IAM_PI * 0.5f;  // Face tangent direction
orbit.scale = ImVec2(1, 1);

// Object orbits around center
ImVec2 world_pos = center + orbit.position;
```

## Example: Chain of Transforms

```cpp
// Parent-child relationship
iam_transform parent;
parent.position = ImVec2(100, 100);
parent.rotation = parent_angle;

iam_transform child;
child.position = ImVec2(50, 0);  // Offset from parent
child.rotation = child_angle;

// Combine: child in parent's space
iam_transform world = parent * child;

// World position of child
ImVec2 child_world_pos = world.position;
```

## API Reference

| Function | Description |
|----------|-------------|
| `iam_transform::identity()` | Create identity transform |
| `iam_transform::apply(point)` | Transform a point |
| `iam_transform::inverse()` | Get inverse transform |
| `iam_transform::operator*` | Combine two transforms |
| `iam_transform_lerp(a, b, t, mode)` | Blend transforms |
| `iam_tween_transform(...)` | Animate transform over time |
| `iam_transform_from_matrix(...)` | Create from 3x2 matrix |
| `iam_transform_to_matrix(t, out)` | Convert to 3x2 matrix |

## See Also

- [Motion Paths](motion-paths.md) - Path-based animation
- [Per-Axis Easing](per-axis-easing.md) - Different easing per component
