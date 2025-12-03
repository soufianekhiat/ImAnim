# ImAnim

**Animation Engine for [Dear ImGui](https://github.com/ocornut/imgui)**

ImAnim brings modern animation capabilities to ImGui applications. Write smooth UI animations with minimal code.

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
├── demo_im_anim.cpp           # Interactive demo (optional)
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

- [Integration Guide](integration.md) - Add ImAnim to your project
- [Quick Start](quickstart.md) - Get running in 5 minutes
- [Tweens](tweens.md) - Immediate-mode animation
- [Clips](clips.md) - Timeline-based keyframe animation
- [Easing](easing.md) - All 30+ easing functions
- [Motion Paths](motion-paths.md) - Animate along curves
- [Text Along Paths](text-along-paths.md) - Curved text rendering
- [Stagger](stagger.md) - Cascading element animations
- [Oscillators](oscillators.md) - Continuous periodic motion
- [Shake & Wiggle](shake-wiggle.md) - Feedback and organic motion
- [Noise](noise.md) - Perlin/Simplex procedural animation
- [Text Stagger](text-stagger.md) - Per-character text effects
- [Style Interpolation](style-interpolation.md) - Animated theme transitions
- [Anchors](anchors.md) - Resize-aware animation
- [Debug](debug.md) - Inspector and debugging tools

## Demo

The `demo_im_anim.cpp` file (optional) provides a comprehensive demo showcasing all features:

- Interactive easing curve visualizer
- Cubic bezier editor
- Spring physics playground
- All animation types with live controls
- Performance benchmarks

Call `ImAnimDemoWindow()` in your ImGui frame to display the demo window.

## Contributing

Development is supported through Patreon:

[![Patreon](https://img.shields.io/badge/Patreon-Support%20Development-f96854?style=for-the-badge&logo=patreon)](https://www.patreon.com/SoufianeKHIAT)

## License

MIT License - see [LICENSE](../LICENSE) for details.

---

*Made for the Dear ImGui community*
