# Lottie Player

`im_anim_lottie.h` / `im_anim_lottie.cpp` play Lottie (Bodymovin JSON) files
straight into an `ImDrawList`: no texture, no shader, no extra dependency. It is
optional: add `im_anim_lottie.cpp` to your build next to `im_anim.cpp` only if you
want it.

```cpp
#include "im_anim_lottie.h"

static iam_lottie* anim = iam_lottie_load("loader.json");   // or iam_lottie_load_from_memory(json, size)

static float t = 0.0f;
t = fmodf(t + ImGui::GetIO().DeltaTime, iam_lottie_duration(anim));   // loop
ImVec2 p = ImGui::GetCursorScreenPos();
iam_lottie_draw(ImGui::GetWindowDrawList(), anim, t, iam_v2(p.x, p.y), iam_v2(128, 128));
ImGui::Dummy(ImVec2(128, 128));

iam_lottie_free(anim);   // when done
```

`time` is in seconds and is clamped to the animation, so looping, ping-pong or
driving it from a tween (`iam_tween_float`) or a clip is up to you. The frame is
fitted into `[pos, pos + size]` keeping its aspect ratio (size 0 = composition
size). Play a named segment with markers:

```cpp
float start, dur;
if (iam_lottie_marker(anim, "success", &start, &dur))
    iam_lottie_draw(dl, anim, start + ImMin(elapsed, dur), pos, size);
```

| Query | |
|-------|-|
| `iam_lottie_duration`, `iam_lottie_frame_rate`, `iam_lottie_size` | Seconds, fps, composition size |
| `iam_lottie_warnings` | Unsupported features found in the file (`""` if none) |

## What is supported

- Layers: shape, solid, null, precomp (with time remap), parenting, in/out points, time stretch
- Transforms: anchor, position (also split and curved motion paths), scale, rotation, skew, opacity
- Keyframes: bezier easing (per component), hold keys, old (`e`) and new formats
- Shapes: rect (rounded), ellipse, star / polygon (with roundness), paths and shape morphs, groups
- Styles: fill with non-zero or even-odd rule, stroke (butt / round / square caps, miter / round / bevel joins, dashes), linear and radial gradients
- Modifiers: trim paths (each path or all together), round corners, repeater
- Masks (add, subtract, intersect, inverted) and alpha track mattes, markers

Fills use an anti-aliased trapezoid sweep with exact non-zero / even-odd rules,
holes and self-intersections (adapted from DearWidgets). Strokes are outlined and
filled once, so translucent strokes never darken where they overlap themselves.

## Limits

Reported by `iam_lottie_warnings()` and skipped: expressions, effects, text and
image layers, blend modes, 3D layers, merge paths and other path modifiers
(twist, pucker, zig-zag, offset), gradient highlights. Also:

- Mattes and masks clip by geometry: the matte layer's opacity, gradients and
  luma are ignored (luma mattes act as alpha mattes), mask opacity and feather too.
- Layer opacity is applied to each shape, not to the composited layer, so
  overlapping shapes of a translucent layer show through each other.
- A repeater repeats the shapes and styles above it, not geometry seen by styles below it.
- Alpha gradient stops are sampled at the colour stops.

Files come from untrusted sources often: loading checks the JSON structure, bounds
nesting depth and returns `nullptr` on malformed input.

## Demo

The demo shows it under *Procedural Animations > Lottie Player* when built with
`im_anim_lottie.cpp` and `IAM_DEMO_LOTTIE` defined (the CMake and Sharpmake demo
builds do). You can load your own file there.
