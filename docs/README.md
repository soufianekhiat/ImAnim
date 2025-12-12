# ImAnim

**Animation Engine for [Dear ImGui](https://github.com/ocornut/imgui)**

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/soufianekhiat/ImAnim)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](../LICENSE)

![ImAnimDemo_Hero_1 0 0](https://github.com/user-attachments/assets/a9e7931c-7e31-4585-80dc-9ac93664ec3e)

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
| **Clips** | Timeline keyframes, looping, callbacks, chaining, stagger, variations |
| **Easing** | Quad to Bounce presets, cubic-bezier, steps, spring physics, per-axis |
| **Paths** | Bezier curves, Catmull-Rom splines, path morphing, text along paths |
| **Transforms** | Position, rotation, scale with rotation modes |
| **Procedural** | Oscillators, shake, wiggle, Perlin/Simplex noise |
| **Colors** | Gradients, style interpolation, OKLAB/OKLCH blending |
| **Advanced** | Layering/blending, resolved tweens, drag feedback, scroll animation |
| **Tools** | Debug inspector, profiler, save/load, memory management |

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

## Debug helper

* Timeline for clips with tooltip helpers:
![ImAnimDemo_XukMAvrwhw](https://github.com/user-attachments/assets/cb821e95-fba9-478d-90fe-ebfc710abf62)

* Support for delayed start:
![ImAnimDemo_2fQKeGBaLG](https://github.com/user-attachments/assets/452d3818-93f3-42e0-8683-9d963178c3f5)

## Building Examples

### Windows

Open `examples/ImAnim_vs2022_Win64.sln` in Visual Studio 2022 and select a configuration:

| Configuration | Backend |
|---------------|---------|
| `Release_Win32_DX11` | Win32 + DirectX11 |
| `Release_GLFW_OpenGL3` | GLFW + OpenGL3 |
| `Release_SDL2_OpenGL3` | SDL2 + OpenGL3 |
| `Release_ImPlatform` | ImPlatform abstraction |

Debug configurations are also available for each backend.

### Linux / macOS

Each backend has its own Makefile:

```bash
cd examples/glfw_opengl3
make

# or
cd examples/sdl2_opengl3
make
```

## Documentation

Comprehensive guides for every ImAnim feature, organized by category. Each doc includes code examples and API references.

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

## Interactive Demo

The `im_anim_demo.cpp` file provides a comprehensive playground for exploring all ImAnim features. Call `ImAnimDemoWindow()` to display it.

### Demo Categories

| Category | What You'll Find |
|----------|-----------------|
| **Easing & Tweens** | Easing gallery, custom bezier editor, spring physics, per-axis easing |
| **Interactive Widgets** | Animated buttons, toggle switches, hover cards, progress indicators |
| **Clip-Based Animations** | Timeline keyframes, looping, callbacks, chaining, layering system |
| **Procedural Animations** | Oscillators, shake/wiggle effects, Perlin/Simplex noise |
| **Motion Paths** | Bezier curves, Catmull-Rom splines, path morphing, text along paths |
| **Advanced Interpolation** | Gradients, transforms, style interpolation, text stagger |
| **Utilities** | ImDrawList animations, resize-aware helpers, scroll animation, drag feedback |
| **Debug Tools** | Inspector, profiler, memory stats |
| **Stress Test** | Performance benchmarks with thousands of concurrent animations |

### Use Cases

The demo showcases practical UI patterns you can adapt:

- **Animated Buttons** - Hover scaling, press feedback, icon animations
- **Toggle Switches** - Smooth state transitions with easing
- **Hover Cards** - Expand/collapse with spring physics
- **List Stagger** - Cascading item reveal animations
- **Grid Reveal** - Wave-based grid animations
- **Dropping Cards** - Physics-inspired card animations
- **Health Bars** - Color gradient health indicators
- **Loading Indicators** - Pulsing rings, spinning elements
- **Error Feedback** - Shake effects for invalid input
- **Scroll Navigation** - Smooth scroll-to-section animations

| Animated Tag |
|:------------:|
| ![ImAnimDemo_a1h8amHeJp](https://github.com/user-attachments/assets/6ac30639-b737-47be-9c47-be4f4de48af3) |

| Ripple Button |
|:------------:|
| ![ImAnimDemo_XVoOtKSf3G](https://github.com/user-attachments/assets/9f3cac6f-b1bf-4fd5-b746-16ef93bf8cc4) |

| "Wait" Button |
|:------------:|
| ![ImAnimDemo_2RPgqEAHHm](https://github.com/user-attachments/assets/274de702-f79c-4589-acac-b61d8a91900b) |

| Data Visualization |
|:------------:|
| ![ImAnimDemo_VnmpjDbMZh](https://github.com/user-attachments/assets/cd01aab9-6dd8-46a6-9614-92491f8f9b60) |
| ![ImAnimDemo_zI6R85WazP](https://github.com/user-attachments/assets/7027587b-f2b7-4bfc-aa15-bbadccd87baa) |

## Showcase

Visual examples of ImAnim capabilities in action.

### Stagger Animations

| List Stagger | Grid Stagger | Card Stagger |
|:------------:|:------------:|:------------:|
| ![List](https://github.com/user-attachments/assets/a59968c6-5cbf-405f-8e67-f12e15205512) | ![Grid](https://github.com/user-attachments/assets/6618002b-8326-45ec-aa16-11feed26b75f) | ![Cards](https://github.com/user-attachments/assets/ef29a857-c64c-45e0-bad2-3b67952da36d) |

### Easing & Curves

| Easing Gallery | Custom Bezier | Wave Animations |
|:-------------:|:-------------:|:---------------:|
| ![Easing](https://github.com/user-attachments/assets/d918c4d9-6e22-4fa8-b591-68d6b3afec7e) | ![Bezier](https://github.com/user-attachments/assets/c9f35054-cd26-475e-bb49-7875922fa273) | ![Waves](https://github.com/user-attachments/assets/19c3fd48-10b7-4350-9eea-4cffc4ef4984) |

### Colors & Transforms

| Color Blending | Gradient | Transforms |
|:-------------:|:--------:|:----------:|
| ![Color](https://github.com/user-attachments/assets/364e9fff-cc0f-46ab-97fd-e56a1e1618a9) | ![Gradient](https://github.com/user-attachments/assets/04d10e9f-1303-4fdc-acda-83dd18fbaf85) | ![Transform](https://github.com/user-attachments/assets/d93dd1f6-9204-4687-9e90-ee8d11312a8b) |

### Paths & Text

| Motion Path | Text Effects | Variations |
|:-----------:|:------------:|:----------:|
| ![Path](https://github.com/user-attachments/assets/aa2fc751-36fb-4c7b-aae7-e2bac04b5f45) | ![Text](https://github.com/user-attachments/assets/b389a9f3-89a6-4e5f-b632-74acc8dea3b6) | ![Variations](https://github.com/user-attachments/assets/d660b2a8-78a1-4d94-8bb8-8a9632127e64) |

### Procedural & Integration

| Noise | ImGui Widgets | ImDrawList |
|:-----:|:-------------:|:----------:|
| ![Noise](https://github.com/user-attachments/assets/8a644f6a-4516-4246-9d80-41a2ee036ea5) | ![ImGui](https://github.com/user-attachments/assets/f6cdb73f-2712-4b7e-8f27-5cbfb884b7ce) | ![DrawList](https://github.com/user-attachments/assets/c22f0855-4853-474d-b0d7-e1191e9d4789) |

### Additional Examples

| Oscillator Waves | Transform Layers |
|:----------------:|:----------------:|
| ![Waves2](https://github.com/user-attachments/assets/2a9aa767-6886-4737-8b90-9bee2792a225) | ![Transform2](https://github.com/user-attachments/assets/62f67980-94d5-48b4-ba01-483e960ada89) |

## Contributing

Development is supported through Patreon:

[![Patreon](https://img.shields.io/badge/Patreon-Support%20Development-f96854?style=for-the-badge&logo=patreon)](https://www.patreon.com/SoufianeKHIAT)

### For Contributors: Regenerating Projects

The Visual Studio solution is generated using [Sharpmake](https://github.com/ubisoft/Sharpmake). To regenerate projects after modifying the build configuration:

**Prerequisites:** [.NET 6.0 SDK](https://dotnet.microsoft.com/download) or later

```batch
# First time setup (builds Sharpmake)
cd examples
bootstrap.bat        # Windows
./bootstrap.sh       # Linux/macOS

# Regenerate projects
generate_projects.bat    # Windows
./generate_projects.sh   # Linux/macOS
```

## License

MIT License - see [LICENSE](../LICENSE) for details.

---

*Made for the Dear ImGui community*
