# ImAnim Examples (v1.0.0)

Buildable examples demonstrating ImAnim integration with Dear ImGui.

Each example is a complete application that runs the ImAnim demo (`ImAnimDemoWindow()` from `im_anim_demo.cpp`).

## Available Examples

| Folder | Platform | Graphics | Description |
|--------|----------|----------|-------------|
| `glfw_opengl3/` | GLFW | OpenGL 3 | Cross-platform (Windows, macOS, Linux) |
| `sdl2_opengl3/` | SDL2 | OpenGL 3 | Cross-platform (Windows, macOS, Linux) |
| `win32_directx11/` | Win32 | DirectX 11 | Windows native |
| `implatform/` | ImPlatform | DX11 | Uses [ImPlatform](https://github.com/soufianekhiat/ImPlatform) abstraction |

## Building (Windows)

### Prerequisites

- [.NET 6.0 SDK](https://dotnet.microsoft.com/download) or later
- Visual Studio 2022

### Quick Start

```batch
bootstrap.bat
```

This will:
1. Initialize git submodules (imgui, ImPlatform, Sharpmake)
2. Build Sharpmake
3. Generate Visual Studio solution

Open `ImAnim_vs2022_Win64.sln` and select a configuration:
- `Debug_GLFW_OpenGL3` / `Release_GLFW_OpenGL3`
- `Debug_SDL2_OpenGL3` / `Release_SDL2_OpenGL3`
- `Debug_Win32_DX11` / `Release_Win32_DX11`
- `Debug_ImPlatform` / `Release_ImPlatform`

### Regenerating Projects

After modifying Sharpmake files in `sharpmake/`:

```batch
generate_projects.bat
```

## Building (Linux / macOS)

```bash
chmod +x bootstrap.sh generate_projects.sh
./bootstrap.sh
```

## Folder Structure

```
examples/
├── glfw_opengl3/          # GLFW + OpenGL3 main.cpp
├── sdl2_opengl3/          # SDL2 + OpenGL3 main.cpp
├── win32_directx11/       # Win32 + DirectX11 main.cpp
├── implatform/            # ImPlatform main.cpp
├── extern/                # Git submodules
│   ├── imgui/             # Dear ImGui
│   ├── ImPlatform/        # ImPlatform abstraction
│   └── Sharpmake/         # Sharpmake build generator
├── sharpmake/             # Build configurationconfiguration
├── tools/                 # Sharpmake binaries (after bootstrap)
├── bootstrap.bat          # Windows bootstrap script
├── bootstrap.sh           # Linux/macOS bootstrap script
├── generate_projects.bat  # Windows project regeneration
├── generate_projects.sh   # Linux/macOS project regeneration
└── ImAnim_vs2022_Win64.sln  # Generated VS solution
```

## Code Structure

Each `main.cpp` is minimal - it sets up the platform/graphics and calls the demo:

```cpp
#include "im_anim.h"

// From im_anim_demo.cpp
extern void ImAnimDemoWindow();

// In main loop, after ImGui::NewFrame():
iam_update_begin_frame();
iam_clip_update(io.DeltaTime);

// Show the demo
ImAnimDemoWindow();
```

## High DPI Support

All examples include proper high DPI support:
- Scaled window size based on monitor DPI
- `style.ScaleAllSizes()` for UI element scaling
- `style.FontScaleDpi` for font scaling

## Integrating Into Your Project

To add ImAnim to your existing project:

1. Copy `im_anim.h` and `im_anim.cpp` to your project
2. Add the two frame setup calls after `ImGui::NewFrame()`
3. Start animating!

See [docs/integration.md](../docs/integration.md) for detailed instructions.
