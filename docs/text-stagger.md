# Text Stagger

Per-character animation effects for text with various visual effects.

## Overview

Text stagger animates text character-by-character with effects like fade, scale, slide, rotate, bounce, and wave.

## Basic Usage

```cpp
iam_text_stagger_opts opts;
opts.pos = ImVec2(100, 200);           // Base position
opts.effect = iam_text_fx_fade;         // Effect type
opts.char_delay = 0.05f;                // 50ms between characters
opts.char_duration = 0.3f;              // 300ms per character
opts.color = IM_COL32(255, 255, 255, 255);

// Animate with progress (0.0 to 1.0)
float progress = iam_tween_float(
    ImGui::GetID("text"),
    ImHashStr("progress"),
    1.0f, 2.0f,
    iam_ease_preset(iam_ease_linear),
    iam_policy_crossfade, dt
);

iam_text_stagger(ImHashStr("text"), "Hello World!", progress, opts);
```

## Effect Types

| Effect | Description |
|--------|-------------|
| `iam_text_fx_none` | Instant appear |
| `iam_text_fx_fade` | Fade in alpha |
| `iam_text_fx_scale` | Scale from center |
| `iam_text_fx_slide_up` | Slide up from below |
| `iam_text_fx_slide_down` | Slide down from above |
| `iam_text_fx_slide_left` | Slide in from right |
| `iam_text_fx_slide_right` | Slide in from left |
| `iam_text_fx_rotate` | Rotate in |
| `iam_text_fx_bounce` | Bounce in with overshoot |
| `iam_text_fx_wave` | Continuous wave motion |
| `iam_text_fx_typewriter` | Typewriter style (instant per char) |

## Options Reference

```cpp
iam_text_stagger_opts opts;
opts.pos = ImVec2(x, y);              // Base position
opts.effect = iam_text_fx_fade;        // Effect type
opts.char_delay = 0.05f;               // Delay between chars (seconds)
opts.char_duration = 0.3f;             // Duration per char (seconds)
opts.color = IM_COL32(255, 255, 255, 255);
opts.font = nullptr;                   // Use current font
opts.font_scale = 1.0f;                // Font scale
opts.slide_distance = 20.0f;           // Distance for slide effects
opts.rotation_amount = 0.5f;           // Rotation in radians
opts.wave_amplitude = 10.0f;           // Wave height
opts.wave_frequency = 2.0f;            // Wave speed
```

## Helper Functions

```cpp
// Calculate total text width
float width = iam_text_stagger_width("Hello World!", opts);

// Calculate total animation duration
float duration = iam_text_stagger_duration("Hello World!", opts);
```

## Example: Title Animation

```cpp
void AnimatedTitle(const char* title) {
    static float start_time = -1.0f;
    float current_time = ImGui::GetTime();

    if (start_time < 0.0f) {
        start_time = current_time;
    }

    float elapsed = current_time - start_time;

    iam_text_stagger_opts opts;
    opts.pos = ImVec2(100, 50);
    opts.effect = iam_text_fx_bounce;
    opts.char_delay = 0.08f;
    opts.char_duration = 0.5f;
    opts.color = IM_COL32(255, 220, 100, 255);
    opts.font_scale = 2.0f;

    float duration = iam_text_stagger_duration(title, opts);
    float progress = ImClamp(elapsed / duration, 0.0f, 1.0f);

    iam_text_stagger(ImHashStr("title"), title, progress, opts);
}
```

## Example: Typewriter Effect

```cpp
void TypewriterText(const char* text) {
    float dt = ImGui::GetIO().DeltaTime;

    iam_text_stagger_opts opts;
    opts.pos = ImVec2(50, 100);
    opts.effect = iam_text_fx_typewriter;
    opts.char_delay = 0.05f;  // Typing speed
    opts.char_duration = 0.0f; // Instant per char
    opts.color = IM_COL32(0, 255, 0, 255);

    // Calculate progress based on time
    float duration = iam_text_stagger_duration(text, opts);
    static float time = 0.0f;
    time += dt;
    float progress = fmodf(time / duration, 1.0f);

    iam_text_stagger(ImHashStr("typewriter"), text, progress, opts);
}
```

## Example: Wave Text

```cpp
void WaveText(const char* text) {
    iam_text_stagger_opts opts;
    opts.pos = ImVec2(100, 200);
    opts.effect = iam_text_fx_wave;
    opts.wave_amplitude = 15.0f;
    opts.wave_frequency = 3.0f;
    opts.color = IM_COL32(100, 200, 255, 255);

    // Wave is continuous, progress controls reveal
    iam_text_stagger(ImHashStr("wave"), text, 1.0f, opts);
}
```

## Example: Button Hover Effect

```cpp
void HoverAnimatedLabel(const char* label, bool hovered) {
    ImGuiID id = ImGui::GetID(label);
    float dt = ImGui::GetIO().DeltaTime;

    // Animate progress based on hover
    float progress = iam_tween_float(
        id, ImHashStr("hover_progress"),
        hovered ? 1.0f : 0.0f,
        0.3f,
        iam_ease_preset(iam_ease_out_cubic),
        iam_policy_crossfade,
        dt
    );

    iam_text_stagger_opts opts;
    opts.pos = ImGui::GetCursorScreenPos();
    opts.effect = iam_text_fx_scale;
    opts.char_delay = 0.02f;
    opts.char_duration = 0.2f;

    // Reverse animation: fully visible when not hovered
    float display_progress = hovered ? progress : 1.0f;

    iam_text_stagger(id, label, display_progress, opts);
}
```

## Text Stagger vs Text Along Paths

| Feature | Text Stagger | Text Along Paths |
|---------|--------------|------------------|
| Layout | Linear/grid | Curved path |
| Effects | Many built-in | Position/rotation |
| Use Case | Reveal effects | Curved text |

## See Also

- [Text Along Paths](text-along-paths.md) - Text on curves
- [Stagger](stagger.md) - Element stagger animations
