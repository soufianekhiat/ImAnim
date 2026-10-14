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

## UI interactions

The player has no state: you own the playhead, so any UI can drive it.

```cpp
// Toggle (checkbox, switch, like): play toward the end while on, back while off,
// at the file's speed; a click mid-way reverses from where it is
static bool on = false;
static float t = 0.0f;
if (ImGui::InvisibleButton("##toggle", size)) on = !on;
t = ImClamp(t + (on ? dt : -dt), 0.0f, iam_lottie_duration(anim));
iam_lottie_draw(dl, anim, t, pos, size);

// Progress: a value (download, slider, tween) mapped onto a marker segment
float start, dur;
iam_lottie_marker(anim, "progress", &start, &dur);
iam_lottie_draw(dl, anim, start + value * dur, pos, size);
```

The demo's *Lottie Interactions* section shows a 3D flip card, a day / night switch (its progress
also tints the panel), a checkbox, a like button, a menu icon whose items follow
the playhead, and a download ring driven by a tween then a "done" segment. These
animations were made for the demo and ship under ImAnim's license.

## What is supported

- Layers: shape, solid, null, precomp (with time remap), text, parenting, auto-orient, in/out points, time stretch
- 3D layers (position / anchor / scale in z, X / Y / Z rotation, orientation) seen through the comp's camera layer (zoom, point of interest, parenting) or After Effects' default 50 mm camera; 2D layers in the same comp stay flat
- Older Bodymovin exports are upgraded on load like lottie-web (0..255 colours, closed flags on shapes, single text documents)
- Transforms: anchor, position (also split and curved motion paths), scale, rotation, skew, opacity
- Keyframes: bezier easing (per component), hold keys, old (`e`) and new formats
- Shapes: rect (rounded), ellipse, star / polygon (with roundness), paths and shape morphs, groups
- Styles: fill with non-zero or even-odd rule, stroke (butt / round / square caps, miter / round / bevel joins, dashes), linear and radial gradients (with highlight)
- Modifiers: trim paths (each path or all together), round corners, pucker / bloat, zig-zag (corner and smooth), offset path, twist, merge paths (merge, add, subtract, intersect, exclude), repeater
- Masks (add, subtract, intersect, difference, inverted) and alpha track mattes, markers
- Effects: Fill and Tint recolour a layer (and the layers of a precomp); Gaussian blur and drop shadow (colour, opacity, direction, distance, softness, shadow only); expression controls (sliders, checkboxes, ...) are ignored as they draw nothing
- Text layers drawn from the glyph outlines the file embeds (`chars`): lines, justification, tracking, line height, fill and stroke colours

Fills use an anti-aliased trapezoid sweep with exact non-zero / even-odd rules,
holes and self-intersections (adapted from DearWidgets). Strokes are outlined and
filled once, so translucent strokes never darken where they overlap themselves.

## GPU fill (optional, ImPlatform)

Build `im_anim_lottie.cpp` with `IAM_LOTTIE_IMPLATFORM` defined, ImPlatform's
`ImPlatform/` folder on the include path and `IM_CURRENT_GFX` set (the ImPlatform
CMake targets export both). ImPlatform is not part of ImAnim; nothing changes
without the define.

Solid fills and strokes are then drawn by a winding-number pixel shader (adapted
from DearWidgets): the CPU only builds the outline and the GPU does the coverage,
one quad per horizontal band of at most 256 segments. On the sample files this cuts
the CPU cost per draw 5-10x for stroke-heavy animations.

```cpp
iam_lottie_set_gpu(false);          // force the CPU fill (on by default)
bool on = iam_lottie_gpu_active();  // compiled in, enabled and the shader compiled
```

On D3D10 / D3D11 it also draws layers that use a blend mode or a track matte into
offscreen textures (ImPlatform render textures) and composites them with a shader: all
17 After Effects blend modes (W3C formulas, against a copy of the backbuffer), exact
track mattes, alpha and luma, inverted or not, including the matte's opacity and
gradients, and a true Gaussian blur and drop shadow (two separable passes, sigma as in
lottie-web) instead of the CPU's soft edges. Offscreen layers do not nest (an inner one draws as with the CPU path) and
use the main viewport's backbuffer.

The fill runs on the D3D10/11/12 backends (their custom-shader constant buffers hold the
segments). Other backends, a failed shader compile, gradients and anything under a
mask or matte use the CPU fill, so output is the same everywhere apart from
anti-aliasing: the shader keeps the inside opaque and fades half a pixel outside,
where the CPU fill fades across the edge.

## Performance

Every draw rebuilds the frame on the CPU. Single convex contours and smooth,
opaque, undashed strokes go straight to `AddConvexPolyFilled` / `AddPolyline`; the
rest uses the sweep fill, with its scratch buffers kept across draws (a few dozen
allocations per draw on typical files).

A draw repeated with the same time, position, size and clip (a paused animation,
an icon drawn several times) replays the vertices recorded the second time it is
seen: each animation keeps the last 8 such frames. Turn it off with
`iam_lottie_set_cache(anim, false)` to save that memory on animations that never
repeat a frame.

## Limits

Reported by `iam_lottie_warnings()` and skipped: expressions, effects other than
Fill, Tint, Gaussian blur and drop shadow, image layers, text without embedded glyphs
(exported with system fonts) and text animators, blend modes. Also:

- Without the GPU path, mattes and masks clip by geometry: luma mattes act as alpha
  mattes, blend modes draw as normal, feather is ignored, and opacity is approximated (a single add mask's opacity and an alpha
  matte layer's own opacity scale the layer; the matte's fill opacity and gradients
  do not count). Difference masks XOR with the add masks before them, exact unless
  those overlap each other.
- Merge paths (add, subtract, intersect, exclude) and offset path change how the
  shapes are filled; a stroke after them still follows the original paths, and an
  offset path fills the holes of the shapes it offsets.
- Text boxes do not wrap; the text is laid out as point text.
- Without the GPU path, blur and drop shadow are drawn without an offscreen layer: each shape's edge becomes a
  soft ramp (the shape stacked grown and shrunk across the edge), so a blur softens
  edges rather than mixing colours of shapes that touch, and directional blur is
  applied in both directions.
- Twist has no reference implementation (lottie-web, Skottie, ThorVG and rlottie skip it):
  it follows After Effects' description, each point turning by angle * (1 - d / R)
  around the centre, so it can differ from After Effects in detail.
- 3D: layers draw in file order (no depth sorting), the top camera layer is used for
  the whole animation, and curves are projected through their control points (a
  close-up in strong perspective can bend slightly differently).
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
