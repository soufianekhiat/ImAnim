# Oscillators

Continuous periodic animations with multiple wave shapes.

## Overview

Oscillators generate continuous, repeating values without start/end points. Perfect for idle animations, pulsing effects, and Lissajous patterns.

## Basic Oscillation

```cpp
float offset = iam_oscillate(
    ImGui::GetID("pulse"),  // ID
    50.0f,                  // Amplitude
    1.0f,                   // Frequency (Hz)
    iam_wave_sine,          // Wave type
    0.0f,                   // Phase offset
    dt                      // Delta time
);
// Returns value between -50 and +50
```

## Wave Types

| Type | Description | Use Case |
|------|-------------|----------|
| `iam_wave_sine` | Smooth sinusoidal | Most natural motion |
| `iam_wave_triangle` | Linear ramp up/down | Mechanical motion |
| `iam_wave_sawtooth` | Ramp up, instant reset | Sweeping effects |
| `iam_wave_square` | On/off pulse | Blinking, digital |

## 2D Oscillation

Create Lissajous patterns with different frequencies per axis:

```cpp
ImVec2 offset = iam_oscillate_vec2(
    ImGui::GetID("lissajous"),
    ImVec2(40.0f, 40.0f),     // Amplitude per axis
    ImVec2(1.0f, 2.0f),       // Frequency per axis
    iam_wave_sine,
    ImVec2(0.0f, 0.0f),       // Phase per axis
    dt
);
```

### Lissajous Frequency Ratios

| X:Y Ratio | Pattern |
|-----------|---------|
| 1:1 | Circle/ellipse |
| 1:2 | Figure-8 |
| 2:3 | Complex loop |
| 3:4 | Trefoil pattern |

## 4D Oscillation

```cpp
ImVec4 osc = iam_oscillate_vec4(
    id,
    ImVec4(10, 20, 30, 40),   // Amplitudes
    ImVec4(1, 2, 0.5f, 0.3f), // Frequencies
    iam_wave_sine,
    ImVec4(0, 0, 0, 0),       // Phases
    dt
);
```

## Color Oscillation

Pulsing colors with color space support:

```cpp
ImVec4 color = iam_oscillate_color(
    ImGui::GetID("glow"),
    ImVec4(0.5f, 0.8f, 1.0f, 1.0f),  // Base color (sRGB)
    ImVec4(0.0f, 0.0f, 0.3f, 0.0f),  // Amplitude (affects V in HSV)
    2.0f,                             // Frequency
    iam_wave_sine,
    0.0f,                             // Phase
    iam_col_hsv,                      // Color space
    dt
);
```

### Color Space Effects

| Space | Effect |
|-------|--------|
| `iam_col_hsv` | Pulse brightness (V channel) |
| `iam_col_oklab` | Perceptually uniform pulsing |
| `iam_col_oklch` | Vibrant color cycling |

## Example: Pulsing Button

```cpp
void PulsingButton(const char* label) {
    ImGuiID id = ImGui::GetID(label);
    float dt = ImGui::GetIO().DeltaTime;

    // Subtle scale pulse
    float pulse = iam_oscillate(id, 0.05f, 2.0f, iam_wave_sine, 0.0f, dt);
    float scale = 1.0f + pulse;

    // Glowing color
    ImVec4 glow = iam_oscillate_color(
        ImGui::GetID("glow"),
        ImVec4(0.2f, 0.5f, 1.0f, 1.0f),
        ImVec4(0.0f, 0.0f, 0.2f, 0.0f),
        1.5f, iam_wave_sine, 0.0f,
        iam_col_hsv, dt
    );

    ImGui::PushStyleColor(ImGuiCol_Button, glow);
    ImGui::SetWindowFontScale(scale);
    ImGui::Button(label);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
}
```

## Example: Floating Animation

```cpp
// Gentle floating motion
ImVec2 float_offset = iam_oscillate_vec2(
    ImGui::GetID("float"),
    ImVec2(5.0f, 10.0f),      // Subtle X, more Y
    ImVec2(0.3f, 0.5f),       // Slow frequencies
    iam_wave_sine,
    ImVec2(0.0f, 0.25f),      // Offset phases for organic feel
    dt
);

ImVec2 base_pos = ImVec2(100, 100);
ImVec2 final_pos = ImVec2(base_pos.x + float_offset.x, base_pos.y + float_offset.y);
```

## Example: Loading Spinner

```cpp
// Rotating dots with phase offsets
int num_dots = 8;
for (int i = 0; i < num_dots; i++) {
    float phase = (float)i / num_dots;  // 0.0 to 0.875

    float alpha = iam_oscillate(
        ImGui::GetID("spinner") + i,
        0.4f,              // Amplitude
        1.0f,              // 1 rotation per second
        iam_wave_sine,
        phase,             // Phase offset
        dt
    );
    alpha = 0.6f + alpha;  // Remap to 0.2-1.0

    // Draw dot at angle with varying alpha
    float angle = phase * 2 * IM_PI;
    ImVec2 pos = ImVec2(
        center.x + cosf(angle) * radius,
        center.y + sinf(angle) * radius
    );
    // Draw with alpha...
}
```

## See Also

- [Shake & Wiggle](shake-wiggle.md) - Noise-based animation
- [Noise](noise.md) - Procedural noise channels
