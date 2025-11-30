# Noise Channels

Perlin and Simplex noise for organic, procedural movement.

## Overview

Noise channels provide smooth, coherent randomness that's more natural than simple random values. Use for organic motion, terrain, and procedural effects.

## Noise Types

| Type | Description |
|------|-------------|
| `iam_noise_perlin` | Classic Perlin noise, smooth gradients |
| `iam_noise_simplex` | Faster, fewer artifacts than Perlin |
| `iam_noise_value` | Simple value noise, blocky |

## Basic Noise Sampling

```cpp
iam_noise_opts opts;
opts.type = iam_noise_perlin;
opts.octaves = 4;           // Fractal detail layers
opts.persistence = 0.5f;    // Amplitude decay per octave
opts.lacunarity = 2.0f;     // Frequency increase per octave
opts.seed = 42;             // Random seed

// 2D noise (returns -1 to 1)
float value = iam_noise(x, y, opts);

// 3D noise
float value_3d = iam_noise_3d(x, y, z, opts);
```

## Noise Options

| Option | Description | Default |
|--------|-------------|---------|
| `type` | Noise algorithm | `iam_noise_perlin` |
| `octaves` | Number of fractal layers | 1 |
| `persistence` | Amplitude multiplier per octave | 0.5 |
| `lacunarity` | Frequency multiplier per octave | 2.0 |
| `seed` | Random seed | 0 |

### Octaves Effect

| Octaves | Result |
|---------|--------|
| 1 | Smooth, simple shapes |
| 2-4 | Natural detail |
| 6+ | High detail, more expensive |

## Animated Noise Channels

Continuous noise that evolves over time:

```cpp
// 1D noise channel
float offset = iam_noise_channel(
    ImHashStr("wobble"),
    0.5f,     // Frequency
    20.0f,    // Amplitude
    opts,
    dt
);
```

### 2D Noise Channel

```cpp
ImVec2 wobble = iam_noise_channel_vec2(
    ImHashStr("pos"),
    ImVec2(0.5f, 0.7f),    // Frequency per axis
    ImVec2(30.0f, 20.0f),  // Amplitude per axis
    opts,
    dt
);
```

## Simplified Smooth Noise

For quick organic motion without configuring options:

```cpp
// Simple 1D smooth noise
float noise = iam_smooth_noise(
    ImHashStr("simple"),
    10.0f,  // Amplitude
    0.5f,   // Frequency
    dt
);

// Simple 2D smooth noise
ImVec2 noise2d = iam_smooth_noise_vec2(
    ImHashStr("simple2d"),
    ImVec2(10.0f, 10.0f),  // Amplitude
    0.5f,                   // Frequency
    dt
);
```

## Example: Organic Floating

```cpp
void FloatingElement(ImVec2 base_pos) {
    float dt = ImGui::GetIO().DeltaTime;

    iam_noise_opts opts;
    opts.type = iam_noise_simplex;
    opts.octaves = 2;
    opts.persistence = 0.5f;

    ImVec2 offset = iam_noise_channel_vec2(
        ImHashStr("float"),
        ImVec2(0.3f, 0.4f),
        ImVec2(15.0f, 20.0f),
        opts,
        dt
    );

    ImVec2 pos = ImVec2(base_pos.x + offset.x, base_pos.y + offset.y);
    ImGui::SetCursorScreenPos(pos);
    ImGui::Text("Floating");
}
```

## Example: Procedural Background

```cpp
void DrawProceduralBackground(ImDrawList* draw_list, ImVec2 origin, ImVec2 size) {
    static float time = 0.0f;
    time += ImGui::GetIO().DeltaTime;

    iam_noise_opts opts;
    opts.type = iam_noise_perlin;
    opts.octaves = 4;
    opts.persistence = 0.5f;
    opts.seed = 123;

    int step = 20;
    for (float y = 0; y < size.y; y += step) {
        for (float x = 0; x < size.x; x += step) {
            // Sample noise with time as Z coordinate
            float n = iam_noise_3d(x * 0.01f, y * 0.01f, time * 0.2f, opts);
            n = (n + 1.0f) * 0.5f;  // Remap to 0-1

            ImU32 color = IM_COL32(
                (int)(n * 100),
                (int)(n * 150),
                (int)(n * 200),
                255
            );

            draw_list->AddRectFilled(
                ImVec2(origin.x + x, origin.y + y),
                ImVec2(origin.x + x + step, origin.y + y + step),
                color
            );
        }
    }
}
```

## Example: Camera Shake with Noise

```cpp
void CameraShakeWithNoise(ImVec2& camera_offset) {
    static float shake_intensity = 0.0f;
    float dt = ImGui::GetIO().DeltaTime;

    // Decay intensity
    shake_intensity *= 0.95f;

    if (shake_intensity > 0.01f) {
        iam_noise_opts opts;
        opts.type = iam_noise_simplex;
        opts.octaves = 2;

        ImVec2 shake = iam_noise_channel_vec2(
            ImHashStr("camera_shake"),
            ImVec2(15.0f, 15.0f),       // High frequency for shake
            ImVec2(shake_intensity, shake_intensity),
            opts,
            dt
        );

        camera_offset = shake;
    } else {
        camera_offset = ImVec2(0, 0);
    }
}

void TriggerCameraShake(float intensity) {
    shake_intensity = intensity;
}
```

## Noise vs Wiggle vs Oscillate

| Feature | Noise | Wiggle | Oscillate |
|---------|-------|--------|-----------|
| Pattern | Coherent random | Simple random | Periodic |
| Repeat | Never | Never | Always |
| Control | Full options | Amplitude/freq | Wave type |
| Use Case | Organic motion | Quick randomness | Rhythmic |

## See Also

- [Shake & Wiggle](shake-wiggle.md) - Simpler noise animation
- [Oscillators](oscillators.md) - Periodic motion
