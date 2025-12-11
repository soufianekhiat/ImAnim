# Animation Layering

Blend multiple animation instances together for complex effects.

## Overview

Animation layering lets you combine multiple clips into a single output. Each layer contributes with a weight, and the final value is a weighted blend. Use cases include:
- Combining base animation with additive effects
- Blending between different animation states
- Layering facial expressions or poses

## Basic Usage

```cpp
// Play two clips on different instances
iam_instance walk = iam_play(ImHashStr("walk"), ImHashStr("char_walk"));
iam_instance wave = iam_play(ImHashStr("wave"), ImHashStr("char_wave"));

// Set weights
walk.set_weight(0.7f);
wave.set_weight(0.3f);

// Begin layering into a target ID
ImGuiID blend_target = ImHashStr("char_blend");
iam_layer_begin(blend_target);
iam_layer_add(walk, 0.7f);
iam_layer_add(wave, 0.3f);
iam_layer_end(blend_target);

// Query blended values
float blended_arm_angle;
iam_get_blended_float(blend_target, ImHashStr("arm_angle"), &blended_arm_angle);
```

## Layer Functions

```cpp
// Start accumulating layers for target ID
void iam_layer_begin(ImGuiID target_id);

// Add a clip instance with weight to the blend
void iam_layer_add(iam_instance inst, float weight);

// Finalize and normalize weights
void iam_layer_end(ImGuiID target_id);

// Query blended values
bool iam_get_blended_float(ImGuiID target_id, ImGuiID channel, float* out);
bool iam_get_blended_vec2(ImGuiID target_id, ImGuiID channel, ImVec2* out);
bool iam_get_blended_vec4(ImGuiID target_id, ImGuiID channel, ImVec4* out);
bool iam_get_blended_int(ImGuiID target_id, ImGuiID channel, int* out);
```

## Example: State Machine Blending

```cpp
// Character animation states
enum CharState { IDLE, WALK, RUN };
static CharState current_state = IDLE;
static CharState prev_state = IDLE;
static float blend_progress = 1.0f;

// Play clips for each state
iam_instance idle_inst = iam_play(ImHashStr("idle"), ImHashStr("char_idle"));
iam_instance walk_inst = iam_play(ImHashStr("walk"), ImHashStr("char_walk"));
iam_instance run_inst  = iam_play(ImHashStr("run"),  ImHashStr("char_run"));

// When state changes, start blending
if (new_state != current_state) {
    prev_state = current_state;
    current_state = new_state;
    blend_progress = 0.0f;
}

// Animate blend
blend_progress = iam_tween_float(
    ImGui::GetID("state_blend"), ImHashStr("progress"),
    1.0f, 0.3f,
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade, dt
);

// Get instances for states
auto get_inst = [&](CharState s) -> iam_instance {
    switch (s) {
        case IDLE: return idle_inst;
        case WALK: return walk_inst;
        case RUN:  return run_inst;
    }
    return idle_inst;
};

// Blend between previous and current state
ImGuiID blend_id = ImHashStr("char_final");
iam_layer_begin(blend_id);
iam_layer_add(get_inst(prev_state), 1.0f - blend_progress);
iam_layer_add(get_inst(current_state), blend_progress);
iam_layer_end(blend_id);

// Use blended values
float leg_angle, arm_angle;
iam_get_blended_float(blend_id, ImHashStr("leg_angle"), &leg_angle);
iam_get_blended_float(blend_id, ImHashStr("arm_angle"), &arm_angle);
```

## Example: Additive Layers

```cpp
// Base locomotion animation
iam_instance base = iam_play(ImHashStr("locomotion"), ImHashStr("base"));

// Additive layers (damage flinch, breathing, etc.)
iam_instance flinch = iam_play(ImHashStr("flinch"), ImHashStr("flinch"));
iam_instance breathe = iam_play(ImHashStr("breathe"), ImHashStr("breathe"));

// Flinch intensity based on recent damage
float flinch_weight = damage_taken > 0 ? 0.5f : 0.0f;

// Always have subtle breathing
float breathe_weight = 0.1f;

// Combine
ImGuiID final_id = ImHashStr("final_pose");
iam_layer_begin(final_id);
iam_layer_add(base, 1.0f);
iam_layer_add(flinch, flinch_weight);
iam_layer_add(breathe, breathe_weight);
iam_layer_end(final_id);

// Weights don't need to sum to 1.0 - they'll be normalized
```

## Example: UI Panel States

```cpp
// Different panel configurations
iam_instance compact = iam_play(ImHashStr("panel_compact"), ImHashStr("compact"));
iam_instance expanded = iam_play(ImHashStr("panel_expanded"), ImHashStr("expanded"));

// Smoothly blend between states
float expand_ratio = iam_tween_float(
    id, ImHashStr("expand"),
    is_expanded ? 1.0f : 0.0f, 0.25f,
    iam_ease_preset(iam_ease_out_cubic),
    iam_policy_crossfade, dt
);

ImGuiID panel_blend = ImHashStr("panel_blend");
iam_layer_begin(panel_blend);
iam_layer_add(compact, 1.0f - expand_ratio);
iam_layer_add(expanded, expand_ratio);
iam_layer_end(panel_blend);

// Get blended panel dimensions
ImVec2 panel_size;
iam_get_blended_vec2(panel_blend, ImHashStr("size"), &panel_size);

float panel_alpha;
iam_get_blended_float(panel_blend, ImHashStr("alpha"), &panel_alpha);
```

## Weight Normalization

Weights are automatically normalized when `iam_layer_end` is called:

```cpp
// These are equivalent:
iam_layer_add(a, 0.5f);
iam_layer_add(b, 0.5f);
// Final: a=0.5, b=0.5 (sum=1.0)

iam_layer_add(a, 1.0f);
iam_layer_add(b, 1.0f);
// Final: a=0.5, b=0.5 (normalized from sum=2.0)

iam_layer_add(a, 3.0f);
iam_layer_add(b, 1.0f);
// Final: a=0.75, b=0.25 (normalized from sum=4.0)
```

## Instance Weights

You can also set weights directly on instances:

```cpp
iam_instance inst = iam_play(clip_id, inst_id);
inst.set_weight(0.5f);  // Instance-level weight

// When adding to layer, weights multiply
iam_layer_add(inst, 0.8f);
// Effective weight = 0.5 * 0.8 = 0.4
```

## API Reference

| Function | Description |
|----------|-------------|
| `iam_layer_begin(id)` | Start accumulating layers |
| `iam_layer_add(inst, weight)` | Add instance with weight |
| `iam_layer_end(id)` | Finalize and normalize |
| `iam_get_blended_float(id, ch, out)` | Get blended float |
| `iam_get_blended_vec2(id, ch, out)` | Get blended vec2 |
| `iam_get_blended_vec4(id, ch, out)` | Get blended vec4 |
| `iam_get_blended_int(id, ch, out)` | Get blended int |
| `inst.set_weight(w)` | Set instance weight |

## See Also

- [Clips](clips.md) - Timeline animations
- [Tweens](tweens.md) - Basic value animation
