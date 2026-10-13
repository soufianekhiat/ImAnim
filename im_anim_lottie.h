// im_anim_lottie.h -- optional Lottie (Bodymovin JSON) player for ImAnim.
//
// Not part of the core: add im_anim_lottie.cpp to your build next to im_anim.cpp
// when you want it. Draws straight into an ImDrawList (no texture, no shader).
//
// Supported: shape / solid / null / precomp layers, parenting, transforms (anchor,
// position incl. split and curved motion paths, scale, rotation, skew, opacity),
// keyframes with bezier and hold easing, rect / ellipse / star / path shapes and
// shape morphs, fill (non-zero / even-odd), stroke (caps, joins, dashes), linear /
// radial gradients, trim paths, round corners, repeater, masks (add, subtract,
// intersect) and alpha track mattes (from the matte layer's geometry), time remap,
// markers.
// Not supported (reported by iam_lottie_warnings): expressions, effects, text and
// image layers, blend modes, 3D, merge paths and other path modifiers, luma mattes
// (drawn as alpha mattes), gradient highlights.

#pragma once

#include "im_anim.h"

struct iam_lottie;

// Load from a file or from memory (the JSON text). Returns nullptr on error.
IM_ANIM_API iam_lottie*  iam_lottie_load(char const* path);
IM_ANIM_API iam_lottie*  iam_lottie_load_from_memory(char const* json, size_t size);
IM_ANIM_API void         iam_lottie_free(iam_lottie* anim);

IM_ANIM_API float        iam_lottie_duration(iam_lottie const* anim);    // seconds
IM_ANIM_API float        iam_lottie_frame_rate(iam_lottie const* anim);  // frames per second
IM_ANIM_API iam_vec2     iam_lottie_size(iam_lottie const* anim);        // composition size
IM_ANIM_API char const*  iam_lottie_warnings(iam_lottie const* anim);    // unsupported features found ("" if none)

// Marker by name: start and duration in seconds. Returns false if not found.
IM_ANIM_API bool         iam_lottie_marker(iam_lottie const* anim, char const* name, float* out_start, float* out_duration);

// Draws the frame at `time` seconds (clamped to the animation; loop with fmodf) into
// [pos, pos + size], keeping the aspect ratio. size 0 = composition size.
IM_ANIM_API void         iam_lottie_draw(ImDrawList* dl, iam_lottie const* anim, float time, iam_vec2 pos, iam_vec2 size = iam_v2(0, 0));
