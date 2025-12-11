# ImAnim

**Animation Engine for [Dear ImGui](https://github.com/ocornut/imgui)**

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/soufianekhiat/ImAnim)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](../LICENSE)

![Image](https://github.com/user-attachments/assets/51fde8b3-6d55-4e71-b012-819cd97d728b)

ImAnim brings modern animation capabilities to ImGui applications. Write smooth UI animations with minimal code.

## Version 1.0.0 - First Official Release

This is the first stable release of ImAnim, featuring a complete animation toolkit for Dear ImGui.

```cpp
// Animate anything in one line
float alpha = iam_tween_float(id, channel, hovered ? 1.0f : 0.5f, 0.3f, ease, policy, dt);
```

## Why ImAnim?

- **Immediate-mode friendly** - Works naturally with ImGui's paradigm
- **Zero dependencies** - Only requires Dear ImGui
- **Large easing collection** - 30+ easing functions including spring physics
- **Perceptual color blending** - OKLAB and OKLCH
- **Responsive layouts** - Anchor-relative animations that survive window resizing

## Features at a Glance

| Category | Capabilities |
|----------|-------------|
| **Tweens** | Float, Vec2, Vec4, Int, Color with crossfade/cut/queue policies |
| **Clips** | Timeline keyframes, looping, callbacks, chaining, stagger |
| **Easing** | Quad to Bounce presets, cubic-bezier, steps, spring physics |
| **Paths** | Bezier curves, Catmull-Rom splines, text along paths |
| **Procedural** | Oscillators, shake, wiggle, Perlin/Simplex noise |
| **Extras** | Style interpolation, scroll animation, debug inspector |

## Quick Integration

**Step 1:** Copy `im_anim.h` and `im_anim.cpp` to your project.

**Step 2:** Add two lines after `ImGui::NewFrame()`:

```cpp
ImGui::NewFrame();

// Add these two lines
iam_update_begin_frame();
iam_clip_update(ImGui::GetIO().DeltaTime);

// Your UI code...
```

**Step 3:** Animate! Here's a hover effect:

```cpp
bool hovered = ImGui::IsItemHovered();
float scale = iam_tween_float(
    ImGui::GetID("btn"), ImHashStr("scale"),
    hovered ? 1.1f : 1.0f, 0.2f,
    iam_ease_preset(iam_ease_out_back),
    iam_policy_crossfade,
    ImGui::GetIO().DeltaTime
);
```

That's it. No build system changes, no external dependencies.

## Repository Structure

```
ImAnim/
├── im_anim.h                  # Main header (include this)
├── im_anim.cpp                # Implementation
├── im_anim_demo.cpp           # Interactive demo (optional)
├── LICENSE                    # MIT License
├── docs/                      # Documentation
│   ├── README.md              # This file
│   └── *.md                   # Feature documentation
└── examples/                  # Buildable examples
    ├── glfw_opengl3/          # GLFW + OpenGL3 example
    ├── sdl2_opengl3/          # SDL2 + OpenGL3 example
    ├── win32_directx11/       # Win32 + DirectX11 example
    ├── implatform/            # ImPlatform abstraction example
    ├── extern/                # Git submodules
    │   ├── imgui/             # Dear ImGui
    │   ├── ImPlatform/        # ImPlatform abstraction layer
    │   └── Sharpmake/         # Sharpmake build generator
    ├── sharpmake/             # Build configuration
    ├── tools/                 # Build tools (populated by bootstrap)
    ├── bootstrap.bat          # Windows: build Sharpmake & generate projects
    ├── bootstrap.sh           # Linux/macOS: build Sharpmake & generate projects
    ├── generate_projects.bat  # Windows: regenerate VS projects
    └── generate_projects.sh   # Linux/macOS: regenerate projects
```

## Building Examples

### Prerequisites

- [.NET 6.0 SDK](https://dotnet.microsoft.com/download) or later (for Sharpmake)
- Visual Studio 2022 (Windows)

### Windows

```batch
cd examples
bootstrap.bat
```

This will:
1. Initialize git submodules (imgui, ImPlatform, Sharpmake)
2. Build Sharpmake
3. Generate Visual Studio solution

Open `examples/ImAnim_vs2022_Win64.sln` and select a configuration:
- `Debug_GLFW_OpenGL3` / `Release_GLFW_OpenGL3`
- `Debug_SDL2_OpenGL3` / `Release_SDL2_OpenGL3`
- `Debug_Win32_DX11` / `Release_Win32_DX11`
- `Debug_ImPlatform` / `Release_ImPlatform`

### Linux / macOS

```bash
cd examples
chmod +x bootstrap.sh generate_projects.sh
./bootstrap.sh
```

### Regenerating Projects

After modifying Sharpmake configuration files:

```batch
# Windows
generate_projects.bat

# Linux/macOS
./generate_projects.sh
```

## Documentation

### Getting Started
- [Integration Guide](integration.md) - Add ImAnim to your project
- [Quick Start](quickstart.md) - Get running in 5 minutes

### Core Animation
- [Tweens](tweens.md) - Immediate-mode animation
- [Clips](clips.md) - Timeline-based keyframe animation
- [Easing](easing.md) - All 30+ easing functions

### Motion & Paths
- [Motion Paths](motion-paths.md) - Animate along curves
- [Path Morphing](path-morphing.md) - Interpolate between paths
- [Text Along Paths](text-along-paths.md) - Curved text rendering
- [Transforms](transforms.md) - 2D transform animation

### Effects
- [Stagger](stagger.md) - Cascading element animations
- [Text Stagger](text-stagger.md) - Per-character text effects
- [Oscillators](oscillators.md) - Continuous periodic motion
- [Shake & Wiggle](shake-wiggle.md) - Feedback and organic motion
- [Noise](noise.md) - Perlin/Simplex procedural animation

### Colors & Styles
- [Gradients](gradients.md) - Color gradient animation
- [Style Interpolation](style-interpolation.md) - Animated theme transitions
- [Per-Axis Easing](per-axis-easing.md) - Different easing per component

### Advanced Features
- [Variations](variations.md) - Per-loop parameter changes
- [Layering](layering.md) - Blend multiple animations
- [Resolved Tweens](resolved-tweens.md) - Dynamic target computation
- [Anchors](anchors.md) - Resize-aware animation
- [Drag Feedback](drag-feedback.md) - Animated drag operations
- [Scroll Animation](scroll-animation.md) - Smooth scrolling

### Tools & Debug
- [Debug](debug.md) - Inspector and debugging tools
- [Advanced](advanced.md) - Profiler, persistence, memory management

## Demo

The `im_anim_demo.cpp` file (optional) provides a comprehensive demo showcasing all features:

- Interactive easing curve visualizer
- Cubic bezier editor
- Spring physics playground
- All animation types with live controls
- Performance benchmarks

Call `ImAnimDemoWindow()` in your ImGui frame to display the demo window.

## Showcase:

List Stagger:

![Image](https://github.com/user-attachments/assets/a59968c6-5cbf-405f-8e67-f12e15205512)

Grid Stagger:

![Image](https://github.com/user-attachments/assets/6618002b-8326-45ec-aa16-11feed26b75f)

Card Stagger:

![Image](https://github.com/user-attachments/assets/ef29a857-c64c-45e0-bad2-3b67952da36d)

Color Blending:

![Image](https://github.com/user-attachments/assets/364e9fff-cc0f-46ab-97fd-e56a1e1618a9)

Waves:

![Image](https://github.com/user-attachments/assets/19c3fd48-10b7-4350-9eea-4cffc4ef4984)

![Image](https://github.com/user-attachments/assets/2a9aa767-6886-4737-8b90-9bee2792a225)

ImGui Integration:

![Image](https://github.com/user-attachments/assets/f6cdb73f-2712-4b7e-8f27-5cbfb884b7ce)

Transforms:

![Image](https://github.com/user-attachments/assets/d93dd1f6-9204-4687-9e90-ee8d11312a8b)

![Image](https://github.com/user-attachments/assets/62f67980-94d5-48b4-ba01-483e960ada89)

Gradient:

![Image](https://github.com/user-attachments/assets/04d10e9f-1303-4fdc-acda-83dd18fbaf85)

Custom Ease Functions:

![Image](https://github.com/user-attachments/assets/c9f35054-cd26-475e-bb49-7875922fa273)

Path:

![Image](https://github.com/user-attachments/assets/aa2fc751-36fb-4c7b-aae7-e2bac04b5f45)

Variations:

![Image](https://github.com/user-attachments/assets/d660b2a8-78a1-4d94-8bb8-8a9632127e64)

Text:

![Image](https://github.com/user-attachments/assets/b389a9f3-89a6-4e5f-b632-74acc8dea3b6)

Noise:

![Image](https://github.com/user-attachments/assets/8a644f6a-4516-4246-9d80-41a2ee036ea5)

Easing Functions:

![Image](https://github.com/user-attachments/assets/d918c4d9-6e22-4fa8-b591-68d6b3afec7e)

ImDrawList:

![Image](https://github.com/user-attachments/assets/c22f0855-4853-474d-b0d7-e1191e9d4789)

## Contributing

Development is supported through Patreon:

[![Patreon](https://img.shields.io/badge/Patreon-Support%20Development-f96854?style=for-the-badge&logo=patreon)](https://www.patreon.com/SoufianeKHIAT)

## License

MIT License - see [LICENSE](../LICENSE) for details.

---

*Made for the Dear ImGui community*
