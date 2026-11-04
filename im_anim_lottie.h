// im_anim_lottie.h -- optional Lottie (Bodymovin JSON) player for ImAnim.
//
// Not part of the core: add im_anim_lottie.cpp to your build next to im_anim.cpp
// when you want it. Draws straight into an ImDrawList (no texture, no shader).
//
// Supported: shape / solid / null / precomp / text layers (text from the file's embedded
// glyphs), 3D layers and cameras, parenting, auto-orient, transforms (anchor, position incl. split and curved
// motion paths, scale, rotation, skew, opacity), keyframes with bezier and hold easing,
// rect / ellipse / star / path shapes and shape morphs, fill (non-zero / even-odd),
// stroke (caps, joins, dashes), linear / radial gradients (with highlight), trim paths,
// round corners, pucker / bloat, zig-zag, offset path, twist, merge paths, repeater, masks (add,
// subtract, intersect, difference) and alpha track mattes (from the matte layer's
// geometry), Fill / Tint / Gaussian blur / drop shadow effects, time remap, markers.
// With the GPU path on DX10 / DX11 (see below): blend modes, exact track mattes (alpha with
// opacity and gradients, luma) and true Gaussian blur / drop shadow; without it blend modes
// draw as normal, luma mattes as alpha mattes, blur and shadow as soft edges.
// Not supported (reported by iam_lottie_warnings): expressions, other effects, image
// layers, text without embedded glyphs and text animators. Older Bodymovin exports (0..255 colours, ...) are
// upgraded on load like lottie-web does.

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

// A draw repeated with the same time, position, size and clip rect (paused animation, idle
// widget) replays the vertices it produced the previous time. On by default; turn it off
// to always evaluate (for example when profiling).
IM_ANIM_API void         iam_lottie_set_cache(iam_lottie* anim, bool enabled);

// Draw only the file's own frames (time rounded down to 1 / frame rate): a 30 fps animation
// on a 144 Hz display then repeats each frame ~5 times and replays it from the cache. Off by
// default (in-between frames are interpolated).
IM_ANIM_API void         iam_lottie_set_frame_snap(iam_lottie* anim, bool enabled);

// GPU fill: build with IAM_LOTTIE_IMPLATFORM (and ImPlatform on the include path, with
// IM_CURRENT_GFX defined) to fill solid shapes with a winding-number shader through
// ImPlatform's custom shaders on the D3D10/11/12 backends, and on D3D10/11 to draw layers
// with a blend mode or a track matte offscreen and composite them with a shader. Gradients,
// masks, other backends, or a failed shader compile use the CPU path.
IM_ANIM_API bool         iam_lottie_gpu_active();               // GPU fill compiled in, enabled and working
IM_ANIM_API void         iam_lottie_set_gpu(bool enabled);      // on by default

// Renderer hook: a fill or stroke handed to another renderer (a vector painter) before the player draws it. The
// outline is in screen space, as cubic chains per contour (pts[first]: start, then (control, control, end) per
// segment; lines have their handles on their vertices). Return true when drawn; false lets the player draw it.
// Only plain shapes are offered: not inside a matte, mask, soft edge (blur, drop shadow) or offscreen layer, nor merged
// or offset paths. Set while drawing (null: none). The function and 'user' are part of the replay cache's key
// (iam_lottie_set_cache): pass another 'user' when the renderer's output changes for the same frame.
struct iam_lottie_contour { int first, count; bool closed; };
struct iam_lottie_outline {
	ImVec2 const* pts;
	iam_lottie_contour const* contours;
	int contour_count;
	int fill_rule;                  // 0 non-zero, 1 even-odd
};
struct iam_lottie_paint {
	int type;                       // 0 solid, 1 linear gradient, 2 radial gradient
	ImU32 color;                    // solid (opacity included)
	ImVec2 p0, p1, focal;           // linear: from p0 to p1; radial: centre p0, radius |p1 - p0|, highlight at focal
	int stop_count;
	float const* stop_offsets;
	float const (*stop_rgba)[4];    // straight alpha, opacity included; interpolated premultiplied
};
struct iam_lottie_stroke {
	float width;
	int cap;                        // 1 butt, 2 round, 3 square
	int join;                       // 1 miter, 2 round, 3 bevel
	float miter_limit;
	float const* dashes;            // dash, gap, ... (screen units; odd lists already doubled); dash_count 0: solid
	int dash_count;
	float dash_offset;
};
typedef bool (*iam_lottie_render_fn)(ImDrawList* dl, iam_lottie_outline const* outline, iam_lottie_paint const* paint, iam_lottie_stroke const* stroke, void* user);  // stroke null: a fill
IM_ANIM_API void         iam_lottie_set_renderer(iam_lottie_render_fn fn, void* user);
