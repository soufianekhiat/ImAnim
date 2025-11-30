# Style Interpolation

Animate between ImGui style themes with smooth color transitions.

## Overview

Style interpolation allows smooth animated transitions between different ImGui themes/styles, blending all colors perceptually.

## Registering Styles

### Register Current Style

```cpp
// Set up your theme
ImGui::StyleColorsLight();
iam_style_register_current(ImHashStr("light_theme"));

// Switch and register another
ImGui::StyleColorsDark();
iam_style_register_current(ImHashStr("dark_theme"));
```

### Register Custom Style

```cpp
ImGuiStyle my_style;
// ... configure colors and sizes ...
iam_style_register(ImHashStr("custom_theme"), my_style);
```

## Blending Styles

### Direct Blend

Blend between two styles at a specific ratio:

```cpp
// 0.0 = style_a, 1.0 = style_b
iam_style_blend(
    ImHashStr("light_theme"),
    ImHashStr("dark_theme"),
    0.5f,           // Blend factor
    iam_col_oklab   // Color space for blending
);
```

### Animated Tween

Smooth animated transition to a target style:

```cpp
static ImGuiID current_theme = ImHashStr("light_theme");

if (ImGui::Button("Toggle Theme")) {
    current_theme = (current_theme == ImHashStr("light_theme"))
        ? ImHashStr("dark_theme")
        : ImHashStr("light_theme");
}

iam_style_tween(
    ImHashStr("theme_tween"),          // Tween ID
    current_theme,                      // Target style
    0.5f,                               // Duration
    iam_ease_preset(iam_ease_out_cubic),
    iam_col_oklab,                      // Color space
    dt
);
```

## Color Spaces

| Space | Description | Best For |
|-------|-------------|----------|
| `iam_col_srgb` | Direct sRGB blend | Simple themes |
| `iam_col_srgb_linear` | Linear RGB | Physical accuracy |
| `iam_col_hsv` | HSV blend | Hue-preserving |
| `iam_col_oklab` | Perceptual (recommended) | Most themes |
| `iam_col_oklch` | Cylindrical perceptual | Vibrant colors |

## Example: Theme Selector

```cpp
void ThemeSelector() {
    static int selected_theme = 0;
    static ImGuiID theme_ids[] = {
        ImHashStr("light"),
        ImHashStr("dark"),
        ImHashStr("classic"),
        ImHashStr("custom")
    };
    const char* theme_names[] = { "Light", "Dark", "Classic", "Custom" };

    // Register themes once
    static bool themes_registered = false;
    if (!themes_registered) {
        ImGui::StyleColorsLight();
        iam_style_register_current(theme_ids[0]);

        ImGui::StyleColorsDark();
        iam_style_register_current(theme_ids[1]);

        ImGui::StyleColorsClassic();
        iam_style_register_current(theme_ids[2]);

        // Custom theme
        ImGuiStyle custom;
        custom.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.2f, 1.0f);
        // ... more colors ...
        iam_style_register(theme_ids[3], custom);

        themes_registered = true;
    }

    // Theme buttons
    for (int i = 0; i < 4; i++) {
        if (ImGui::RadioButton(theme_names[i], selected_theme == i)) {
            selected_theme = i;
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();

    // Animate to selected theme
    float dt = ImGui::GetIO().DeltaTime;
    iam_style_tween(
        ImHashStr("main_theme"),
        theme_ids[selected_theme],
        0.4f,
        iam_ease_preset(iam_ease_out_cubic),
        iam_col_oklab,
        dt
    );
}
```

## Example: Time-Based Theme

```cpp
void AutoTheme() {
    float dt = ImGui::GetIO().DeltaTime;

    // Get hour (0-23)
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    int hour = t->tm_hour;

    // Day: 7am-7pm, Night: 7pm-7am
    ImGuiID target = (hour >= 7 && hour < 19)
        ? ImHashStr("light_theme")
        : ImHashStr("dark_theme");

    // Slow transition (2 seconds)
    iam_style_tween(
        ImHashStr("auto_theme"),
        target,
        2.0f,
        iam_ease_preset(iam_ease_in_out_cubic),
        iam_col_oklab,
        dt
    );
}
```

## Example: Blend Slider

```cpp
void ThemeBlendSlider() {
    static float blend = 0.0f;
    ImGui::SliderFloat("Theme Blend", &blend, 0.0f, 1.0f);

    iam_style_blend(
        ImHashStr("light_theme"),
        ImHashStr("dark_theme"),
        blend,
        iam_col_oklab
    );
}
```

## Notes

- Style blending affects all ImGuiCol values
- Non-color style values (sizes, rounding) are not interpolated
- OKLAB color space provides the most visually pleasing transitions
- Register styles before using them in blend/tween calls

## See Also

- [Tweens](tweens.md) - Color tweening basics
- [Debug](debug.md) - Animation inspector for style tweens
