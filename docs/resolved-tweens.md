# Resolved Tweens

Dynamic target computation via callbacks for responsive animations.

## Overview

Standard tweens have fixed target values. Resolved tweens compute the target dynamically each frame via a callback function. This is useful when:
- Target depends on runtime state (window size, mouse position)
- Target is computed from other values
- Animation needs to follow a moving target

## Basic Usage

```cpp
// Resolver callback returns target value
float my_target_resolver(void* user) {
    // Compute target based on current state
    MyWidget* widget = (MyWidget*)user;
    return widget->is_expanded ? 200.0f : 50.0f;
}

// Use resolved tween
float height = iam_tween_float_resolved(
    id, ImHashStr("height"),
    my_target_resolver, &my_widget,  // Callback and user data
    0.3f,                            // Duration
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade,
    dt
);
```

## Resolver Types

```cpp
// Float resolver
typedef float (*iam_float_resolver)(void* user);

// Vec2 resolver
typedef ImVec2 (*iam_vec2_resolver)(void* user);

// Vec4 resolver
typedef ImVec4 (*iam_vec4_resolver)(void* user);

// Color resolver
typedef ImVec4 (*iam_color_resolver)(void* user);

// Int resolver
typedef int (*iam_int_resolver)(void* user);
```

## Example: Follow Mouse

```cpp
ImVec2 get_mouse_target(void* user) {
    return ImGui::GetMousePos();
}

ImVec2 follower_pos = iam_tween_vec2_resolved(
    ImGui::GetID("follower"), ImHashStr("pos"),
    get_mouse_target, nullptr,
    0.2f,  // Smooth follow with 0.2s delay
    iam_ease_preset(iam_ease_out_quad),
    iam_policy_crossfade,
    dt
);

// Draw follower at smoothed position
ImGui::GetWindowDrawList()->AddCircleFilled(follower_pos, 10.0f, IM_COL32(255, 100, 100, 255));
```

## Example: Window-Relative Position

```cpp
ImVec2 get_window_center(void* user) {
    ImVec2 size = ImGui::GetWindowSize();
    return ImVec2(size.x * 0.5f, size.y * 0.5f);
}

// Element always animates toward window center (even during resize)
ImVec2 centered_pos = iam_tween_vec2_resolved(
    id, ImHashStr("center_pos"),
    get_window_center, nullptr,
    0.4f,
    iam_ease_preset(iam_ease_out_back),
    iam_policy_crossfade,
    dt
);
```

## Example: Computed Color

```cpp
struct ColorContext {
    float health;
    float max_health;
};

ImVec4 get_health_color(void* user) {
    ColorContext* ctx = (ColorContext*)user;
    float ratio = ctx->health / ctx->max_health;

    // Red when low, green when high
    return ImVec4(1.0f - ratio, ratio, 0.0f, 1.0f);
}

static ColorContext health_ctx = { 75.0f, 100.0f };

ImVec4 bar_color = iam_tween_color_resolved(
    id, ImHashStr("health_color"),
    get_health_color, &health_ctx,
    0.3f,
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade,
    iam_col_oklab,
    dt
);
```

## Example: Target List Item

```cpp
struct ListContext {
    int* items;
    int selected_index;
    float item_height;
};

float get_selected_y(void* user) {
    ListContext* ctx = (ListContext*)user;
    return ctx->selected_index * ctx->item_height;
}

static ListContext list_ctx;

// Selection indicator follows selected item
float indicator_y = iam_tween_float_resolved(
    id, ImHashStr("indicator_y"),
    get_selected_y, &list_ctx,
    0.15f,
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade,
    dt
);
```

## Example: Snapped Grid Position

```cpp
struct GridContext {
    ImVec2 raw_pos;
    float grid_size;
};

ImVec2 get_snapped_pos(void* user) {
    GridContext* ctx = (GridContext*)user;
    return ImVec2(
        roundf(ctx->raw_pos.x / ctx->grid_size) * ctx->grid_size,
        roundf(ctx->raw_pos.y / ctx->grid_size) * ctx->grid_size
    );
}

static GridContext grid_ctx = { {123, 87}, 50.0f };

// Smoothly snap to grid
ImVec2 snapped = iam_tween_vec2_resolved(
    id, ImHashStr("grid_pos"),
    get_snapped_pos, &grid_ctx,
    0.2f,
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade,
    dt
);
```

## Lambdas (C++11)

With C++11 or later, you can use lambdas if they don't capture (or use a static/global for context):

```cpp
// Non-capturing lambda works as callback
static float target_value = 100.0f;

float value = iam_tween_float_resolved(
    id, ImHashStr("val"),
    [](void*) { return target_value; }, nullptr,
    0.3f, iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade, dt
);
```

## Rebase vs Resolved

**Rebase**: Change target once for an in-progress animation
```cpp
iam_rebase_float(id, channel, new_target, dt);  // One-time redirect
```

**Resolved**: Compute target every frame
```cpp
iam_tween_float_resolved(id, ch, resolver_fn, user, ...);  // Continuous
```

Use rebase for discrete target changes. Use resolved for continuously computed targets.

## API Reference

| Function | Description |
|----------|-------------|
| `iam_tween_float_resolved(id, ch, fn, user, dur, ez, policy, dt)` | Float with dynamic target |
| `iam_tween_vec2_resolved(id, ch, fn, user, dur, ez, policy, dt)` | Vec2 with dynamic target |
| `iam_tween_vec4_resolved(id, ch, fn, user, dur, ez, policy, dt)` | Vec4 with dynamic target |
| `iam_tween_color_resolved(id, ch, fn, user, dur, ez, policy, cs, dt)` | Color with dynamic target |
| `iam_tween_int_resolved(id, ch, fn, user, dur, ez, policy, dt)` | Int with dynamic target |
| `iam_rebase_float(id, ch, target, dt)` | Redirect float animation |
| `iam_rebase_vec2(id, ch, target, dt)` | Redirect vec2 animation |
| `iam_rebase_vec4(id, ch, target, dt)` | Redirect vec4 animation |
| `iam_rebase_color(id, ch, target, dt)` | Redirect color animation |
| `iam_rebase_int(id, ch, target, dt)` | Redirect int animation |

## See Also

- [Tweens](tweens.md) - Basic value animation
- [Anchors](anchors.md) - Resize-aware animation
