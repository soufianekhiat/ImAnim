# Shake & Wiggle

Procedural noise-based animations for UI feedback and organic movement.

## Overview

Shake provides triggered, decaying animation (error feedback, impacts). Wiggle provides continuous organic motion (idle animations).

## Shake

Triggered animation that decays over time.

### Triggering Shake

```cpp
ImGuiID shake_id = ImGui::GetID("error");

// Trigger when validation fails
if (validation_failed) {
    iam_trigger_shake(shake_id);
}

// Get current shake offset (decays to 0)
float offset_x = iam_shake(
    shake_id,
    10.0f,   // Intensity (max displacement)
    30.0f,   // Frequency (oscillations per second)
    0.5f,    // Decay time (seconds)
    dt
);
```

### 2D Shake

```cpp
ImVec2 offset = iam_shake_vec2(
    id,
    ImVec2(10.0f, 5.0f),  // Different intensity per axis
    30.0f,                 // Frequency
    0.5f,                  // Decay time
    dt
);
```

### 4D Shake

```cpp
ImVec4 shake = iam_shake_vec4(
    id,
    ImVec4(10, 10, 5, 5),  // Intensity per component
    30.0f,                  // Frequency
    0.5f,                   // Decay time
    dt
);
```

### Color Shake

Flickering color effect:

```cpp
ImVec4 error_color = iam_shake_color(
    ImGui::GetID("error_indicator"),
    ImVec4(1.0f, 0.2f, 0.2f, 1.0f),  // Base color
    ImVec4(0.1f, 0.05f, 0.0f, 0.0f), // Intensity
    20.0f,                            // Frequency
    0.5f,                             // Decay time
    iam_col_oklab,                    // Color space
    dt
);
```

## Wiggle

Continuous organic motion without triggering.

### Basic Wiggle

```cpp
float wiggle = iam_wiggle(
    ImGui::GetID("idle"),
    5.0f,   // Amplitude
    3.0f,   // Frequency
    dt
);
```

### 2D Wiggle

```cpp
ImVec2 offset = iam_wiggle_vec2(
    id,
    ImVec2(5.0f, 5.0f),  // Amplitude
    3.0f,                 // Frequency
    dt
);
```

### 4D Wiggle

```cpp
ImVec4 wiggle = iam_wiggle_vec4(
    id,
    ImVec4(5, 5, 5, 5),  // Amplitude
    3.0f,                 // Frequency
    dt
);
```

### Color Wiggle

Ambient color variation:

```cpp
ImVec4 ambient = iam_wiggle_color(
    ImGui::GetID("ambient"),
    ImVec4(0.3f, 0.5f, 0.8f, 1.0f),  // Base color
    ImVec4(0.0f, 0.05f, 0.1f, 0.0f), // Amplitude
    1.5f,                             // Frequency
    iam_col_oklch,                    // Color space
    dt
);
```

## Example: Error Input Field

```cpp
void ErrorTextField(const char* label, char* buf, size_t buf_size, bool has_error) {
    ImGuiID id = ImGui::GetID(label);
    float dt = ImGui::GetIO().DeltaTime;

    static bool prev_error = false;
    if (has_error && !prev_error) {
        iam_trigger_shake(id);
    }
    prev_error = has_error;

    float shake_x = iam_shake(id, 8.0f, 25.0f, 0.4f, dt);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(pos.x + shake_x, pos.y));

    if (has_error) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
    }

    ImGui::InputText(label, buf, buf_size);

    if (has_error) {
        ImGui::PopStyleColor();
    }
}
```

## Example: Floating Icon

```cpp
void FloatingIcon(const char* icon, ImVec2 base_pos) {
    ImGuiID id = ImGui::GetID(icon);
    float dt = ImGui::GetIO().DeltaTime;

    // Gentle continuous wiggle
    ImVec2 offset = iam_wiggle_vec2(id, ImVec2(3.0f, 5.0f), 0.8f, dt);

    ImGui::SetCursorScreenPos(ImVec2(base_pos.x + offset.x, base_pos.y + offset.y));
    ImGui::Text("%s", icon);
}
```

## Example: Impact Effect

```cpp
void OnImpact(ImGuiID impact_id, float intensity) {
    // Trigger with varying intensity
    iam_trigger_shake(impact_id);
    // Note: intensity is passed to iam_shake() call below
}

// In render loop:
float shake = iam_shake(impact_id, impact_intensity, 40.0f, 0.3f, dt);
camera_offset_x += shake;
```

## Shake vs Wiggle

| Feature | Shake | Wiggle |
|---------|-------|--------|
| Trigger | `iam_trigger_shake()` | None (continuous) |
| Decay | Yes, to zero | No, always active |
| Use Case | Feedback, impacts | Idle animation |

## See Also

- [Oscillators](oscillators.md) - Periodic wave animation
- [Noise](noise.md) - Perlin/Simplex noise channels
