# Gradients

Animate between color gradients with multiple stops.

## Overview

ImAnim provides gradient interpolation for smooth color transitions. Gradients can have any number of color stops, and you can animate between different gradient configurations.

## Creating Gradients

```cpp
// Manual construction
iam_gradient grad;
grad.add(0.0f, ImVec4(1, 0, 0, 1));   // Red at start
grad.add(0.5f, ImVec4(1, 1, 0, 1));   // Yellow at middle
grad.add(1.0f, ImVec4(0, 1, 0, 1));   // Green at end

// Helper methods
iam_gradient solid = iam_gradient::solid(ImVec4(1, 0, 0, 1));
iam_gradient two = iam_gradient::two_color(
    ImVec4(1, 0, 0, 1),   // Start
    ImVec4(0, 0, 1, 1)    // End
);
iam_gradient three = iam_gradient::three_color(
    ImVec4(1, 0, 0, 1),   // Start
    ImVec4(1, 1, 0, 1),   // Middle
    ImVec4(0, 1, 0, 1)    // End
);
```

## Sampling Gradients

```cpp
iam_gradient grad;
grad.add(0.0f, ImVec4(1, 0, 0, 1));
grad.add(0.5f, ImVec4(1, 1, 1, 1));
grad.add(1.0f, ImVec4(0, 0, 1, 1));

// Sample at position (0-1)
ImVec4 color_at_25 = grad.sample(0.25f);                    // Between red and white
ImVec4 color_oklab = grad.sample(0.5f, iam_col_oklab);      // Sample in OKLAB space
```

## Gradient Blending

Interpolate between two gradients:

```cpp
iam_gradient grad_a;
grad_a.add(0.0f, ImVec4(1, 0, 0, 1));
grad_a.add(1.0f, ImVec4(1, 1, 0, 1));

iam_gradient grad_b;
grad_b.add(0.0f, ImVec4(0, 0, 1, 1));
grad_b.add(0.5f, ImVec4(0, 1, 1, 1));
grad_b.add(1.0f, ImVec4(0, 1, 0, 1));

// Blend at 50%
iam_gradient blended = iam_gradient_lerp(grad_a, grad_b, 0.5f, iam_col_oklab);
```

## Animated Gradient Tweening

Smoothly animate from one gradient to another:

```cpp
static iam_gradient current_grad = day_gradient;

current_grad = iam_tween_gradient(
    ImGui::GetID("sky"), ImHashStr("gradient"),
    is_night ? night_gradient : day_gradient,
    2.0f,                                      // Duration
    iam_ease_preset(iam_ease_in_out_cubic),
    iam_policy_crossfade,
    iam_col_oklab,                             // Color space
    dt
);

// Draw gradient bar
ImDrawList* dl = ImGui::GetWindowDrawList();
ImVec2 p = ImGui::GetCursorScreenPos();
float width = 300.0f;
float height = 50.0f;

for (float x = 0; x < width; x += 2.0f) {
    float t = x / width;
    ImVec4 col = current_grad.sample(t, iam_col_oklab);
    ImU32 c = ImGui::ColorConvertFloat4ToU32(col);
    dl->AddRectFilled(
        ImVec2(p.x + x, p.y),
        ImVec2(p.x + x + 2, p.y + height),
        c
    );
}
```

## Example: Day/Night Sky

```cpp
// Define gradients
iam_gradient day_sky;
day_sky.add(0.0f, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));   // Light blue
day_sky.add(0.5f, ImVec4(0.6f, 0.85f, 1.0f, 1.0f));  // Lighter
day_sky.add(1.0f, ImVec4(0.9f, 0.95f, 1.0f, 1.0f));  // Near white

iam_gradient sunset_sky;
sunset_sky.add(0.0f, ImVec4(0.1f, 0.1f, 0.3f, 1.0f));  // Dark blue
sunset_sky.add(0.3f, ImVec4(0.6f, 0.2f, 0.4f, 1.0f));  // Purple
sunset_sky.add(0.5f, ImVec4(1.0f, 0.4f, 0.2f, 1.0f));  // Orange
sunset_sky.add(0.7f, ImVec4(1.0f, 0.7f, 0.3f, 1.0f));  // Yellow
sunset_sky.add(1.0f, ImVec4(0.9f, 0.85f, 0.6f, 1.0f)); // Light yellow

iam_gradient night_sky;
night_sky.add(0.0f, ImVec4(0.02f, 0.02f, 0.1f, 1.0f));  // Very dark blue
night_sky.add(0.5f, ImVec4(0.05f, 0.05f, 0.15f, 1.0f)); // Dark blue
night_sky.add(1.0f, ImVec4(0.1f, 0.1f, 0.2f, 1.0f));    // Slightly lighter

// Animate based on time of day
iam_gradient target = time_of_day < 0.25f ? night_sky :
                      time_of_day < 0.5f  ? day_sky :
                      time_of_day < 0.75f ? sunset_sky : night_sky;

static iam_gradient sky = day_sky;
sky = iam_tween_gradient(
    id, ImHashStr("sky"),
    target, 3.0f,
    iam_ease_preset(iam_ease_in_out_sine),
    iam_policy_crossfade,
    iam_col_oklab, dt
);
```

## Example: Health Bar

```cpp
// Gradient changes based on health
iam_gradient full_health;
full_health.add(0.0f, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
full_health.add(1.0f, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));

iam_gradient low_health;
low_health.add(0.0f, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
low_health.add(0.5f, ImVec4(1.0f, 0.4f, 0.0f, 1.0f));
low_health.add(1.0f, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));

float health_ratio = current_health / max_health;
iam_gradient bar_gradient = iam_gradient_lerp(
    low_health, full_health,
    health_ratio,
    iam_col_oklab
);

// Draw health bar with gradient
for (float x = 0; x < bar_width * health_ratio; x += 1.0f) {
    float t = x / bar_width;
    ImVec4 col = bar_gradient.sample(t);
    // Draw pixel...
}
```

## Example: Button Hover Gradient

```cpp
iam_gradient normal;
normal.add(0.0f, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
normal.add(1.0f, ImVec4(0.3f, 0.3f, 0.4f, 1.0f));

iam_gradient hovered;
hovered.add(0.0f, ImVec4(0.3f, 0.4f, 0.6f, 1.0f));
hovered.add(1.0f, ImVec4(0.4f, 0.5f, 0.7f, 1.0f));

iam_gradient pressed;
pressed.add(0.0f, ImVec4(0.2f, 0.3f, 0.5f, 1.0f));
pressed.add(1.0f, ImVec4(0.25f, 0.35f, 0.55f, 1.0f));

iam_gradient target = is_pressed ? pressed : (is_hovered ? hovered : normal);

static iam_gradient btn_grad = normal;
btn_grad = iam_tween_gradient(
    btn_id, ImHashStr("bg"),
    target, 0.15f,
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade,
    iam_col_oklab, dt
);
```

## API Reference

| Function | Description |
|----------|-------------|
| `iam_gradient::add(pos, color)` | Add color stop to gradient |
| `iam_gradient::sample(t, color_space)` | Sample color at position |
| `iam_gradient::solid(color)` | Create single-color gradient |
| `iam_gradient::two_color(a, b)` | Create two-stop gradient |
| `iam_gradient::three_color(a, b, c)` | Create three-stop gradient |
| `iam_gradient_lerp(a, b, t, cs)` | Blend two gradients |
| `iam_tween_gradient(...)` | Animate gradient transition |

## See Also

- [Tweens](tweens.md) - Color tweening
- [Style Interpolation](style-interpolation.md) - Theme transitions
