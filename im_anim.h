// im_anim.h — Dear ImGui animation helpers.
// Author: Soufiane KHIAT
// License: MIT
// Version: 1.0.0
//
// - Channels: float, vec2, vec4, int, color (sRGB/Linear/HSV/OKLAB/OKLCH blending).
// - Easing: presets + cubicBezier/steps/back/elastic/bounce/spring.
// - Caching: ImPool + ImGuiStorage keyed by (ImGuiID, channel_id) via ImHashData.
// - Resize helpers: relative targets, resolver callback, explicit rebase.

#pragma once

// Version information
#define IMANIM_VERSION          "1.0.0"
#define IMANIM_VERSION_NUM      10000   // 1.00.00
#define IMANIM_VERSION_MAJOR    1
#define IMANIM_VERSION_MINOR    0
#define IMANIM_VERSION_PATCH    0
#include "imgui.h"
#include <stdint.h>
#include <math.h>
#include <limits.h>
#include <float.h>

// ----------------------------------------------------
// IM_ANIM_API - shared library export/import
// ----------------------------------------------------
// ImAnim links statically by default, like Dear ImGui. To build or consume it
// as a shared library, define IM_ANIM_API before including this header, or in
// your build system:
//
//   Windows, building the DLL:    #define IM_ANIM_API __declspec(dllexport)
//   Windows, consuming the DLL:   #define IM_ANIM_API __declspec(dllimport)
//   GCC/Clang hidden visibility:  #define IM_ANIM_API __attribute__((visibility("default")))
//
// ImAnim keeps its animation pools in globals. Linking it statically into
// several modules gives each module its own independent state; building it once
// as a shared library gives every module a single shared state, which is what a
// plugin architecture normally wants.
#ifndef IM_ANIM_API
#define IM_ANIM_API
#endif

// ----------------------------------------------------
// IAM_VEC2 / IAM_VEC4 - vector types used by the public API
// ----------------------------------------------------
// The API uses iam_vec2/iam_vec4, which default to ImVec2/ImVec4. To use your own
// math types (glm, Eigen, engine types...), define before including this header:
//
//   #define IAM_VEC2              glm::vec2
//   #define IAM_VEC2_TO_IM(v)     ImVec2((v).x, (v).y)              // IAM_VEC2 -> ImVec2
//   #define IAM_VEC2_FROM_IM(v)   glm::vec2((v).x, (v).y)           // ImVec2 -> IAM_VEC2
//   #define IAM_VEC4              glm::vec4
//   #define IAM_VEC4_TO_IM(v)     ImVec4((v).x, (v).y, (v).z, (v).w)
//   #define IAM_VEC4_FROM_IM(v)   glm::vec4((v).x, (v).y, (v).z, (v).w)
//
// The conversion macros are optional when your type already converts implicitly
// (e.g. through IM_VEC2_CLASS_EXTRA / IM_VEC4_CLASS_EXTRA in imconfig.h).
// Define them identically for every translation unit, including im_anim.cpp.
// Internally ImAnim still computes with ImVec2/ImVec4.
#ifndef IAM_VEC2
#define IAM_VEC2 ImVec2
#endif
#ifndef IAM_VEC4
#define IAM_VEC4 ImVec4
#endif
#ifndef IAM_VEC2_TO_IM
#define IAM_VEC2_TO_IM(v) ImVec2(v)
#endif
#ifndef IAM_VEC2_FROM_IM
#define IAM_VEC2_FROM_IM(v) IAM_VEC2(v)
#endif
#ifndef IAM_VEC4_TO_IM
#define IAM_VEC4_TO_IM(v) ImVec4(v)
#endif
#ifndef IAM_VEC4_FROM_IM
#define IAM_VEC4_FROM_IM(v) IAM_VEC4(v)
#endif
typedef IAM_VEC2 iam_vec2;
typedef IAM_VEC4 iam_vec4;
static inline iam_vec2 iam_v2(float x, float y) { return IAM_VEC2_FROM_IM(ImVec2(x, y)); }
static inline iam_vec4 iam_v4(float x, float y, float z, float w) { return IAM_VEC4_FROM_IM(ImVec4(x, y, z, w)); }


#if defined(IMGUI_VERSION_NUM) && IMGUI_VERSION_NUM < 19200 //ImFontBaked and global ImGuiStoragePair were introduced in v19200.
#define IM_ANIM_PRE_19200_COMPATIBILITY
#endif
// PI constants
#ifndef IAM_PI
static float const IAM_PI = 3.1415926535897932384626433832795f;
#endif // !IAM_PI
#ifndef IAM_2PI
static float const IAM_2PI = IAM_PI * 2.0f;
#endif // !IAM_2PI

// ----------------------------------------------------
// Public enums & descriptors (C-style)
// ----------------------------------------------------
enum iam_ease_type {
	iam_ease_linear = 0,
	iam_ease_in_quad,  iam_ease_out_quad,  iam_ease_in_out_quad,
	iam_ease_in_cubic, iam_ease_out_cubic, iam_ease_in_out_cubic,
	iam_ease_in_quart, iam_ease_out_quart, iam_ease_in_out_quart,
	iam_ease_in_quint, iam_ease_out_quint, iam_ease_in_out_quint,
	iam_ease_in_sine,  iam_ease_out_sine,  iam_ease_in_out_sine,
	iam_ease_in_expo,  iam_ease_out_expo,  iam_ease_in_out_expo,
	iam_ease_in_circ,  iam_ease_out_circ,  iam_ease_in_out_circ,
	iam_ease_in_back,  iam_ease_out_back,  iam_ease_in_out_back,        // p0 = overshoot
	iam_ease_in_elastic, iam_ease_out_elastic, iam_ease_in_out_elastic,  // p0 = amplitude, p1 = period
	iam_ease_in_bounce,  iam_ease_out_bounce,  iam_ease_in_out_bounce,
	iam_ease_steps,         // p0 = steps (>=1), p1 = 0:end 1:start 2:both
	iam_ease_cubic_bezier,  // p0=x1 p1=y1 p2=x2 p3=y2
	iam_ease_spring,        // p0=mass p1=stiffness p2=damping p3=v0
	iam_ease_custom,        // User-defined easing function (use iam_ease_custom_fn)
	iam_ease_curve          // Custom bezier curve (iam_ease_curve_set / iam_ease_curve_desc); p0,p1 = curve id (16-bit halves)
};

enum iam_policy {
	iam_policy_crossfade = 0,	// smooth into new target
	iam_policy_cut,				// snap to target
	iam_policy_queue,			// queue one pending target
	iam_policy_additive,		// add animated delta to current value
	iam_policy_multiply			// multiply current value by animated factor
};

enum iam_color_space {
	iam_col_srgb = 0,			// blend in sRGB (not physically linear)
	iam_col_srgb_linear,		// sRGB<->linear, blend in linear, back to sRGB
	iam_col_hsv,				// blend H/S/V (hue shortest arc), keep A linear
	iam_col_oklab,				// sRGB<->OKLAB, blend in OKLAB, back to sRGB
	iam_col_oklch				// sRGB<->OKLCH (cylindrical OKLAB), blend in OKLCH, back to sRGB
};

enum iam_anchor_space {
	iam_anchor_window_content = 0,	// ImGui::GetContentRegionAvail()
	iam_anchor_window,				// ImGui::GetWindowSize()
	iam_anchor_viewport,			// ImGui::GetWindowViewport()->Size
	iam_anchor_last_item			// ImGui::GetItemRectSize()
};

// Descriptor for any easing (preset or parametric)
struct iam_ease_desc {
	int		type;	// iam_ease_type
	float	p0, p1, p2, p3;
};

// Custom easing function callback (t in [0,1], returns eased value)
typedef float (*iam_ease_fn)(float t);

// ----------------------------------------------------
// Public API declarations
// ----------------------------------------------------

// Frame management
IM_ANIM_API void iam_update_begin_frame();                                                      // Call once per frame before any tweens.
IM_ANIM_API void iam_gc(unsigned int max_age_frames = 600);                                     // Remove stale tween entries older than max_age_frames.
IM_ANIM_API void iam_pool_clear();																	// Manually clean up pools.
IM_ANIM_API void iam_reserve(int cap_float, int cap_vec2, int cap_vec4, int cap_int, int cap_color); // Pre-allocate pool capacity.
IM_ANIM_API void iam_set_ease_lut_samples(int count);                                           // Set LUT resolution for parametric easings (default: 256).

// Global time scale (for slow-motion / fast-forward debugging)
IM_ANIM_API void  iam_set_global_time_scale(float scale);                                       // Set global time multiplier (1.0 = normal, 0.5 = half speed, 2.0 = double).
IM_ANIM_API float iam_get_global_time_scale();                                                  // Get current global time scale.

// Reduced motion (accessibility, e.g. from an OS "reduce motion" setting): tweens and transitions complete
// instantly, springs/layout/presence jump to their targets, inertia lands without gliding, and procedural
// motion (oscillate/shake/wiggle/noise) amplitudes are multiplied by procedural_scale (0 = still).
// Clip playback is not changed: check iam_get_reduced_motion() to skip decorative clips.
IM_ANIM_API void  iam_set_reduced_motion(bool enabled, float procedural_scale = 0.0f);
IM_ANIM_API bool  iam_get_reduced_motion();

// Lazy Initialization - defer channel creation until animation is needed
IM_ANIM_API void iam_set_lazy_init(bool enable);                                                // Enable/disable lazy initialization (default: true).
IM_ANIM_API bool iam_is_lazy_init_enabled();                                                    // Check if lazy init is enabled.

// Custom easing functions
IM_ANIM_API void iam_register_custom_ease(int slot, iam_ease_fn fn);                            // Register custom easing in slot 0-15. Use with iam_ease_custom_fn(slot).
IM_ANIM_API iam_ease_fn iam_get_custom_ease(int slot);                                          // Get registered custom easing function.

// Debug UI
IM_ANIM_API void iam_show_unified_inspector(bool* p_open = nullptr);                            // Show unified inspector (merges debug window + animation inspector).
IM_ANIM_API void iam_show_debug_timeline(ImGuiID instance_id);                                  // Show debug timeline for a clip instance.

// Clip editor - edit a clip's tracks and keyframes visually while it plays (edits apply live).
// preview_instance_id: instance the playhead scrubs and controls (0 = the editor plays its own instance).
// Keys: drag to retime, double-click a lane to add, Del to delete, Ctrl+Z/Ctrl+Y undo/redo, Ctrl+A select all,
// Ctrl+wheel zoom, wheel/middle-drag pan. "Copy C++" puts an equivalent iam_clip::begin(...)...end() chain on the clipboard.
IM_ANIM_API bool iam_show_clip_editor(ImGuiID clip_id, bool* p_open = nullptr, ImGuiID preview_instance_id = 0); // Editor window. Returns true if the clip changed this frame.
IM_ANIM_API bool iam_clip_editor(ImGuiID clip_id, ImGuiID preview_instance_id = 0);             // Editor embedded in the current window. Returns true if the clip changed this frame.

// Named IDs (optional) - lets the clip editor, debug timeline and "Copy C++" show names instead of hashes.
IM_ANIM_API ImGuiID iam_id(char const* name);                                                   // Returns ImHashStr(name) and remembers the name. Use for clip and channel ids.
IM_ANIM_API void iam_set_id_name(ImGuiID id, char const* name);                                 // Name an existing id.
IM_ANIM_API char const* iam_get_id_name(ImGuiID id);                                            // Registered name, or nullptr.

// Performance Profiler
IM_ANIM_API void iam_profiler_enable(bool enable);                                              // Enable/disable the performance profiler.
IM_ANIM_API bool iam_profiler_is_enabled();                                                     // Check if profiler is enabled.
IM_ANIM_API void iam_profiler_begin_frame();                                                    // Call at frame start when profiler is enabled.
IM_ANIM_API void iam_profiler_end_frame();                                                      // Call at frame end when profiler is enabled.
IM_ANIM_API void iam_profiler_begin(const char* name);                                          // Begin a named profiler section.
IM_ANIM_API void iam_profiler_end();                                                            // End the current profiler section.

// Drag Feedback - animated feedback for drag operations
struct iam_drag_opts {
	iam_vec2 snap_grid;              // Grid size for snapping (0,0 = no grid)
	iam_vec2* snap_points;           // Array of custom snap points
	int snap_points_count;         // Number of snap points
	float snap_duration;           // Duration of snap animation
	float overshoot;               // Overshoot amount (0 = none, 1 = normal)
	int ease_type;                 // Easing type for snap animation

	iam_drag_opts() : snap_grid(iam_v2(0, 0)), snap_points(nullptr), snap_points_count(0),
		snap_duration(0.2f), overshoot(0), ease_type(iam_ease_out_cubic) {}
};

struct iam_drag_feedback {
	iam_vec2 position;               // Current animated position
	iam_vec2 offset;                 // Offset from drag start
	iam_vec2 velocity;               // Current velocity estimate
	bool is_dragging;              // Currently being dragged
	bool is_snapping;              // Currently snapping to target
	float snap_progress;           // Snap animation progress (0-1)
};

IM_ANIM_API iam_drag_feedback iam_drag_begin(ImGuiID id, iam_vec2 pos);                           // Start tracking drag at position.
IM_ANIM_API iam_drag_feedback iam_drag_update(ImGuiID id, iam_vec2 pos, float dt);                // Update drag position during drag.
IM_ANIM_API iam_drag_feedback iam_drag_release(ImGuiID id, iam_vec2 pos, iam_drag_opts const& opts, float dt); // Release drag with animated feedback.
IM_ANIM_API void iam_drag_cancel(ImGuiID id);                                                   // Cancel drag tracking.

// Oscillators - continuous periodic animations
enum iam_wave_type {
	iam_wave_sine = 0,      // Smooth sine wave
	iam_wave_triangle,      // Triangle wave (linear up/down)
	iam_wave_sawtooth,      // Sawtooth wave (linear up, instant reset)
	iam_wave_square         // Square wave (on/off pulse)
};
IM_ANIM_API float  iam_oscillate(ImGuiID id, float amplitude, float frequency, int wave_type, float phase, float dt);       // Returns oscillating value [-amplitude, +amplitude].
IM_ANIM_API int    iam_oscillate_int(ImGuiID id, int amplitude, float frequency, int wave_type, float phase, float dt);      // Returns oscillating integer value [-amplitude, +amplitude].
IM_ANIM_API iam_vec2 iam_oscillate_vec2(ImGuiID id, iam_vec2 amplitude, iam_vec2 frequency, int wave_type, iam_vec2 phase, float dt); // 2D oscillation.
IM_ANIM_API iam_vec4 iam_oscillate_vec4(ImGuiID id, iam_vec4 amplitude, iam_vec4 frequency, int wave_type, iam_vec4 phase, float dt); // 4D oscillation.
IM_ANIM_API iam_vec4 iam_oscillate_color(ImGuiID id, iam_vec4 base_color, iam_vec4 amplitude, float frequency, int wave_type, float phase, int color_space, float dt); // Color oscillation in specified color space.

// Shake/Wiggle - procedural noise animations
IM_ANIM_API float  iam_shake(ImGuiID id, float intensity, float frequency, float decay_time, float dt);       // Decaying random shake. Returns offset that decays to 0.
IM_ANIM_API int    iam_shake_int(ImGuiID id, int intensity, float frequency, float decay_time, float dt);     // Decaying random shake for integers.
IM_ANIM_API iam_vec2 iam_shake_vec2(ImGuiID id, iam_vec2 intensity, float frequency, float decay_time, float dt); // 2D decaying shake.
IM_ANIM_API iam_vec4 iam_shake_vec4(ImGuiID id, iam_vec4 intensity, float frequency, float decay_time, float dt); // 4D decaying shake.
IM_ANIM_API iam_vec4 iam_shake_color(ImGuiID id, iam_vec4 base_color, iam_vec4 intensity, float frequency, float decay_time, int color_space, float dt); // Color shake in specified color space.
IM_ANIM_API float  iam_wiggle(ImGuiID id, float amplitude, float frequency, float dt);                        // Continuous smooth random movement.
IM_ANIM_API int    iam_wiggle_int(ImGuiID id, int amplitude, float frequency, float dt);                      // Continuous smooth random movement for integers.
IM_ANIM_API iam_vec2 iam_wiggle_vec2(ImGuiID id, iam_vec2 amplitude, float frequency, float dt);                  // 2D continuous wiggle.
IM_ANIM_API iam_vec4 iam_wiggle_vec4(ImGuiID id, iam_vec4 amplitude, float frequency, float dt);                  // 4D continuous wiggle.
IM_ANIM_API iam_vec4 iam_wiggle_color(ImGuiID id, iam_vec4 base_color, iam_vec4 amplitude, float frequency, int color_space, float dt); // Color wiggle in specified color space.
IM_ANIM_API void   iam_trigger_shake(ImGuiID id);                                                             // Trigger/restart a shake animation.

// Easing evaluation
IM_ANIM_API float iam_eval_preset(int type, float t);                                           // Evaluate a preset easing function at time t (0-1).
IM_ANIM_API float iam_ease_eval(iam_ease_desc const& ez, float t);                               // Evaluate any ease descriptor (presets, parametric, custom, curve) at t in [0,1].
// Back/elastic parameters: iam_ease_back(overshoot) / iam_ease_elastic(amplitude, period) (and p0/p1 of those types,
// including clip keys) are honored only when enabled. Off by default so existing animations keep their look.
IM_ANIM_API void  iam_set_preset_ease_params(bool enabled);
IM_ANIM_API bool  iam_get_preset_ease_params();

// Tween API - smoothly interpolate values over time
// init_value: Initial value when channel is first created. Defaults to 0 (or white for color).
//             Use this to avoid unwanted animations when the first target differs from the default.
IM_ANIM_API float  iam_tween_float(ImGuiID id, ImGuiID channel_id, float target, float dur, iam_ease_desc const& ez, int policy, float dt, float init_value = 0.0f);   // Animate a float value.
IM_ANIM_API iam_vec2 iam_tween_vec2(ImGuiID id, ImGuiID channel_id, iam_vec2 target, float dur, iam_ease_desc const& ez, int policy, float dt, iam_vec2 init_value = iam_v2(0, 0));   // Animate a 2D vector.
IM_ANIM_API iam_vec4 iam_tween_vec4(ImGuiID id, ImGuiID channel_id, iam_vec4 target, float dur, iam_ease_desc const& ez, int policy, float dt, iam_vec4 init_value = iam_v4(0, 0, 0, 0));   // Animate a 4D vector.
IM_ANIM_API int    iam_tween_int(ImGuiID id, ImGuiID channel_id, int target, float dur, iam_ease_desc const& ez, int policy, float dt, int init_value = 0);       // Animate an integer value.
IM_ANIM_API iam_vec4 iam_tween_color(ImGuiID id, ImGuiID channel_id, iam_vec4 target_srgb, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt, iam_vec4 init_value = iam_v4(1, 1, 1, 1)); // Animate a color in specified color space.

// Resize-friendly helpers
IM_ANIM_API iam_vec2 iam_anchor_size(int space); // Get dimensions of anchor space (window, viewport, etc.).

// Relative target tweens (percent of anchor + pixel offset) - survive window resizes
IM_ANIM_API float  iam_tween_float_rel(ImGuiID id, ImGuiID channel_id, float percent, float px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, int axis, float dt);  // Float relative to anchor (axis: 0=x, 1=y).
IM_ANIM_API iam_vec2 iam_tween_vec2_rel(ImGuiID id, ImGuiID channel_id, iam_vec2 percent, iam_vec2 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt);           // Vec2 relative to anchor.
IM_ANIM_API iam_vec4 iam_tween_vec4_rel(ImGuiID id, ImGuiID channel_id, iam_vec4 percent, iam_vec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt);           // Vec4 with x,y relative to anchor.
IM_ANIM_API iam_vec4 iam_tween_color_rel(ImGuiID id, ImGuiID channel_id, iam_vec4 percent, iam_vec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int color_space, int anchor_space, float dt); // Color with component offsets.

// Resolver callbacks for dynamic target computation
typedef float  (*iam_float_resolver)(void* user);   // Returns float target value.
typedef iam_vec2 (*iam_vec2_resolver)(void* user);    // Returns vec2 target value.
typedef iam_vec4 (*iam_vec4_resolver)(void* user);    // Returns vec4 target value.
typedef iam_vec4 (*iam_color_resolver)(void* user);   // Returns color target (sRGB).
typedef int    (*iam_int_resolver)(void* user);     // Returns int target value.

// Resolved tweens - target computed dynamically by callback each frame
IM_ANIM_API float  iam_tween_float_resolved(ImGuiID id, ImGuiID channel_id, iam_float_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);                     // Float with dynamic target.
IM_ANIM_API iam_vec2 iam_tween_vec2_resolved(ImGuiID id, ImGuiID channel_id, iam_vec2_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);                       // Vec2 with dynamic target.
IM_ANIM_API iam_vec4 iam_tween_vec4_resolved(ImGuiID id, ImGuiID channel_id, iam_vec4_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);                       // Vec4 with dynamic target.
IM_ANIM_API iam_vec4 iam_tween_color_resolved(ImGuiID id, ImGuiID channel_id, iam_color_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt);    // Color with dynamic target.
IM_ANIM_API int    iam_tween_int_resolved(ImGuiID id, ImGuiID channel_id, iam_int_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);                         // Int with dynamic target.

// Rebase functions - change target of in-progress animation without restarting
IM_ANIM_API void iam_rebase_float(ImGuiID id, ImGuiID channel_id, float new_target, float dt);  // Smoothly redirect float animation to new target.
IM_ANIM_API void iam_rebase_vec2(ImGuiID id, ImGuiID channel_id, iam_vec2 new_target, float dt);  // Smoothly redirect vec2 animation to new target.
IM_ANIM_API void iam_rebase_vec4(ImGuiID id, ImGuiID channel_id, iam_vec4 new_target, float dt);  // Smoothly redirect vec4 animation to new target.
IM_ANIM_API void iam_rebase_color(ImGuiID id, ImGuiID channel_id, iam_vec4 new_target, float dt); // Smoothly redirect color animation to new target.
IM_ANIM_API void iam_rebase_int(ImGuiID id, ImGuiID channel_id, int new_target, float dt);      // Smoothly redirect int animation to new target.

// Color blending utility
IM_ANIM_API iam_vec4 iam_get_blended_color(iam_vec4 a_srgb, iam_vec4 b_srgb, float t, int color_space);  // Blend two sRGB colors in specified color space.

// ----------------------------------------------------
// Convenience shorthands for common easings
// ----------------------------------------------------
inline iam_ease_desc iam_ease_preset(int type) { iam_ease_desc e = { type, 0,0,0,0 }; return e; }                                               // Create descriptor from preset enum.
inline iam_ease_desc iam_ease_bezier(float x1, float y1, float x2, float y2) { iam_ease_desc e = { iam_ease_cubic_bezier, x1,y1,x2,y2 }; return e; } // Create cubic bezier easing.
inline iam_ease_desc iam_ease_steps_desc(int steps, int mode) { iam_ease_desc e = { iam_ease_steps, (float)steps,(float)mode,0,0 }; return e; }     // Create step function easing.
inline iam_ease_desc iam_ease_back(float overshoot) { iam_ease_desc e = { iam_ease_out_back, overshoot,0,0,0 }; return e; }                         // Create back easing with overshoot.
inline iam_ease_desc iam_ease_elastic(float amplitude, float period) { iam_ease_desc e = { iam_ease_out_elastic, amplitude, period,0,0 }; return e; } // Create elastic easing.
inline iam_ease_desc iam_ease_spring_desc(float mass, float stiffness, float damping, float v0) { iam_ease_desc e = { iam_ease_spring, mass, stiffness, damping, v0 }; return e; } // Create physics spring.
inline iam_ease_desc iam_ease_custom_fn(int slot) { iam_ease_desc e = { iam_ease_custom, (float)slot,0,0,0 }; return e; }                                                         // Use registered custom easing (slot 0-15).

// Custom curve eases (like GSAP CustomEase): a chain of cubic bezier segments from (0,0) to (1,y_end).
// points = P0, then (C1, C2, P1) per segment: count = 1 + 3 * segments. Anchor x must increase; y may
// overshoot (e.g. a bounce). Use anywhere an iam_ease_desc goes, and in clips with iam_ease_curve_ref().
IM_ANIM_API void          iam_ease_curve_set(ImGuiID curve_id, iam_vec2 const* points, int count);
IM_ANIM_API int           iam_ease_curve_get(ImGuiID curve_id, iam_vec2* out_points, int max_count);  // Returns the point count (0 if undefined).
IM_ANIM_API bool          iam_ease_curve_exists(ImGuiID curve_id);
inline iam_ease_desc      iam_ease_curve_desc(ImGuiID curve_id) { iam_ease_desc e = { iam_ease_curve, (float)(curve_id & 0xFFFF), (float)(curve_id >> 16), 0, 0 }; return e; }
IM_ANIM_API float const*  iam_ease_curve_ref(ImGuiID curve_id);  // For clip keys: key_float(ch, t, v, iam_ease_curve, iam_ease_curve_ref(id)).

// Ease editor widget: pick a preset, edit its parameters, drag cubic-bezier handles or edit curve
// anchors/handles on a graph (double-click: add anchor, right-click: remove). Returns true when changed.
// A curve ease edits the shared curve (iam_ease_curve_set). size = 0: fit width, height from width.
IM_ANIM_API bool iam_ease_editor(char const* label, iam_ease_desc* ez, iam_vec2 size = iam_v2(0, 0));

// Scroll animation - smooth scrolling for ImGui windows
IM_ANIM_API void iam_scroll_to_y(float target_y, float duration, iam_ease_desc const& ez = iam_ease_preset(iam_ease_out_cubic));           // Scroll current window to Y position.
IM_ANIM_API void iam_scroll_to_x(float target_x, float duration, iam_ease_desc const& ez = iam_ease_preset(iam_ease_out_cubic));           // Scroll current window to X position.
IM_ANIM_API void iam_scroll_to_top(float duration = 0.3f, iam_ease_desc const& ez = iam_ease_preset(iam_ease_out_cubic));                  // Scroll to top of window.
IM_ANIM_API void iam_scroll_to_bottom(float duration = 0.3f, iam_ease_desc const& ez = iam_ease_preset(iam_ease_out_cubic));               // Scroll to bottom of window.

// ----------------------------------------------------
// Per-axis easing - different easing per component
// ----------------------------------------------------

// Per-axis easing descriptor (for vec2/vec4/color)
struct iam_ease_per_axis {
	iam_ease_desc x;             // Easing for X component
	iam_ease_desc y;             // Easing for Y component
	iam_ease_desc z;             // Easing for Z component (vec4/color only)
	iam_ease_desc w;             // Easing for W/alpha component (vec4/color only)

	iam_ease_per_axis()
		: x(iam_ease_preset(iam_ease_linear)), y(iam_ease_preset(iam_ease_linear)),
		  z(iam_ease_preset(iam_ease_linear)), w(iam_ease_preset(iam_ease_linear)) {}

	iam_ease_per_axis(iam_ease_desc all)
		: x(all), y(all), z(all), w(all) {}

	iam_ease_per_axis(iam_ease_desc ex, iam_ease_desc ey)
		: x(ex), y(ey), z(iam_ease_preset(iam_ease_linear)), w(iam_ease_preset(iam_ease_linear)) {}

	iam_ease_per_axis(iam_ease_desc ex, iam_ease_desc ey, iam_ease_desc ez, iam_ease_desc ew)
		: x(ex), y(ey), z(ez), w(ew) {}
};

// Tween with per-axis easing - each component uses its own easing curve
IM_ANIM_API iam_vec2 iam_tween_vec2_per_axis(ImGuiID id, ImGuiID channel_id, iam_vec2 target, float dur, iam_ease_per_axis const& ez, int policy, float dt);
IM_ANIM_API iam_vec4 iam_tween_vec4_per_axis(ImGuiID id, ImGuiID channel_id, iam_vec4 target, float dur, iam_ease_per_axis const& ez, int policy, float dt);
IM_ANIM_API iam_vec4 iam_tween_color_per_axis(ImGuiID id, ImGuiID channel_id, iam_vec4 target_srgb, float dur, iam_ease_per_axis const& ez, int policy, int color_space, float dt);

// ----------------------------------------------------
// Motion Paths - animate along curves and splines
// ----------------------------------------------------

// Path segment types
enum iam_path_segment_type {
	iam_seg_line = 0,            // Linear segment to endpoint
	iam_seg_quadratic_bezier,    // Quadratic bezier (1 control point)
	iam_seg_cubic_bezier,        // Cubic bezier (2 control points)
	iam_seg_catmull_rom          // Catmull-rom spline segment
};

// Single-curve evaluation functions (stateless, for direct use)
IM_ANIM_API iam_vec2 iam_bezier_quadratic(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, float t);                              // Evaluate quadratic bezier at t [0,1].
IM_ANIM_API iam_vec2 iam_bezier_cubic(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, iam_vec2 p3, float t);                       // Evaluate cubic bezier at t [0,1].
IM_ANIM_API iam_vec2 iam_catmull_rom(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, iam_vec2 p3, float t, float tension = 0.5f);  // Evaluate Catmull-Rom spline at t [0,1]. Points go through p1 and p2.

// Derivatives (for tangent/velocity)
IM_ANIM_API iam_vec2 iam_bezier_quadratic_deriv(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, float t);                        // Derivative of quadratic bezier.
IM_ANIM_API iam_vec2 iam_bezier_cubic_deriv(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, iam_vec2 p3, float t);                 // Derivative of cubic bezier.
IM_ANIM_API iam_vec2 iam_catmull_rom_deriv(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, iam_vec2 p3, float t, float tension = 0.5f); // Derivative of Catmull-Rom.

// Forward declaration
struct iam_path_data;

// iam_path - fluent API for building multi-segment motion paths
class IM_ANIM_API iam_path {
public:
	static iam_path begin(ImGuiID path_id, iam_vec2 start);                                           // Start building a path at position.

	iam_path& line_to(iam_vec2 end);                                                                  // Add linear segment to endpoint.
	iam_path& quadratic_to(iam_vec2 ctrl, iam_vec2 end);                                                // Add quadratic bezier segment.
	iam_path& cubic_to(iam_vec2 ctrl1, iam_vec2 ctrl2, iam_vec2 end);                                     // Add cubic bezier segment.
	iam_path& catmull_to(iam_vec2 end, float tension = 0.5f);                                         // Add Catmull-Rom segment to endpoint.
	iam_path& close();                                                                              // Close path back to start point.

	void end();                                                                                     // Finalize and register path.

	ImGuiID id() const { return m_path_id; }

private:
	iam_path(ImGuiID path_id) : m_path_id(path_id) {}
	ImGuiID m_path_id;
};

// Query path info
IM_ANIM_API bool   iam_path_exists(ImGuiID path_id);                                                            // Check if path exists.
IM_ANIM_API float  iam_path_length(ImGuiID path_id);                                                            // Get approximate path length.
IM_ANIM_API iam_vec2 iam_path_evaluate(ImGuiID path_id, float t);                                                 // Sample path at t [0,1].
IM_ANIM_API iam_vec2 iam_path_tangent(ImGuiID path_id, float t);                                                  // Get tangent (normalized direction) at t.
IM_ANIM_API float  iam_path_angle(ImGuiID path_id, float t);                                                    // Get rotation angle (radians) at t.

// Tween along a path
IM_ANIM_API iam_vec2 iam_tween_path(ImGuiID id, ImGuiID channel_id, ImGuiID path_id, float dur, iam_ease_desc const& ez, int policy, float dt);   // Animate position along path.
IM_ANIM_API float  iam_tween_path_angle(ImGuiID id, ImGuiID channel_id, ImGuiID path_id, float dur, iam_ease_desc const& ez, int policy, float dt); // Animate rotation angle along path.

// ----------------------------------------------------
// Arc-length parameterization (for constant-speed animation)
// ----------------------------------------------------

// Build arc-length lookup table for a path (call once per path, improves accuracy)
IM_ANIM_API void   iam_path_build_arc_lut(ImGuiID path_id, int subdivisions = 64);                              // Build LUT with specified resolution.
IM_ANIM_API bool   iam_path_has_arc_lut(ImGuiID path_id);                                                       // Check if path has precomputed LUT.

// Distance-based path evaluation (uses arc-length LUT for constant speed)
IM_ANIM_API float  iam_path_distance_to_t(ImGuiID path_id, float distance);                                     // Convert arc-length distance to t parameter.
IM_ANIM_API iam_vec2 iam_path_evaluate_at_distance(ImGuiID path_id, float distance);                              // Get position at arc-length distance.
IM_ANIM_API float  iam_path_angle_at_distance(ImGuiID path_id, float distance);                                 // Get rotation angle at arc-length distance.
IM_ANIM_API iam_vec2 iam_path_tangent_at_distance(ImGuiID path_id, float distance);                               // Get tangent at arc-length distance.

// ----------------------------------------------------
// Path Morphing - interpolate between two paths
// ----------------------------------------------------

// Morph options for path interpolation
struct iam_morph_opts {
	int   samples;               // Number of sample points for resampling (default: 64)
	bool  match_endpoints;       // Force endpoints to match exactly (default: true)
	bool  use_arc_length;        // Use arc-length parameterization for smoother morphing (default: true)

	iam_morph_opts() : samples(64), match_endpoints(true), use_arc_length(true) {}
};

// Evaluate morphed path at parameter t [0,1] with blend factor [0,1]
// path_a at blend=0, path_b at blend=1
// Paths can have different numbers of segments - they are resampled to match
IM_ANIM_API iam_vec2 iam_path_morph(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts = iam_morph_opts());

// Get tangent of morphed path
IM_ANIM_API iam_vec2 iam_path_morph_tangent(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts = iam_morph_opts());

// Get angle (radians) of morphed path
IM_ANIM_API float  iam_path_morph_angle(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts = iam_morph_opts());

// Tween along a morphing path - animates both position along path AND the morph blend
IM_ANIM_API iam_vec2 iam_tween_path_morph(ImGuiID id, ImGuiID channel_id, ImGuiID path_a, ImGuiID path_b,
                            float target_blend, float dur, iam_ease_desc const& path_ease,
                            iam_ease_desc const& morph_ease, int policy, float dt,
                            iam_morph_opts const& opts = iam_morph_opts());

// Path drawing - stroke part of a path, [t0, t1] as a fraction of its arc length (uniform speed).
// Animate t1 from 0 to 1 for a "draw on" effect; t1 < t0 wraps around (marching segment on closed paths).
// segments = 0 picks a count from the drawn length.
IM_ANIM_API void iam_path_draw(ImDrawList* dl, ImGuiID path_id, float t0, float t1, ImU32 col, float thickness = 1.0f, iam_vec2 offset = iam_v2(0, 0), int segments = 0);
IM_ANIM_API void iam_path_morph_draw(ImDrawList* dl, ImGuiID path_a, ImGuiID path_b, float blend, float t0, float t1, ImU32 col, float thickness = 1.0f,
                                     iam_vec2 offset = iam_v2(0, 0), iam_morph_opts const& opts = iam_morph_opts(), int segments = 0);

// Dashed stroke of [t0, t1] (fractions of arc length): dash and gap in pixels; animate phase (pixels) for "marching ants".
IM_ANIM_API void iam_path_draw_dashed(ImDrawList* dl, ImGuiID path_id, float t0, float t1, float dash, float gap, float phase, ImU32 col, float thickness = 1.0f, iam_vec2 offset = iam_v2(0, 0));
// Fading trail behind t_head over `length` (fraction of the path): alpha (and thickness when taper) grow toward the head.
IM_ANIM_API void iam_path_draw_trail(ImDrawList* dl, ImGuiID path_id, float t_head, float length, ImU32 col, float thickness = 2.0f, iam_vec2 offset = iam_v2(0, 0), bool taper = true);

// Trail behind any moving point (mouse, widget, particle): call every frame with the current position; the last
// `duration` seconds of positions are drawn as a fading (optionally tapering) stroke. dl may be null to only record.
IM_ANIM_API void iam_trail_draw(ImDrawList* dl, ImGuiID id, iam_vec2 pos, float duration, ImU32 col, float thickness = 2.0f, bool taper = true);
IM_ANIM_API void iam_trail_clear(ImGuiID id);  // Forget the history (e.g. after a teleport).

// Build a path from SVG path data ("d" attribute): M L H V C S Q T A Z and their relative forms.
// Arcs are converted to cubic beziers. A new subpath (second M) is joined with a line. Points are
// transformed as p * scale + offset. Returns false (and builds nothing) if the data can't be parsed.
IM_ANIM_API bool iam_path_from_svg(ImGuiID path_id, char const* svg_d, float scale = 1.0f, iam_vec2 offset = iam_v2(0, 0));

// Text outlines as paths (for "handwriting" draw-on effects): every glyph contour of text becomes a path with id
// iam_text_path_id(base_id, i), i in [0, count). Laid out at pos (top-left) with the given pixel size and the font's
// kerning ('\n' starts a new line). font = nullptr: current font. Returns the contour count (0 if the font has no
// outline data, or with IAM_DISABLE_TEXT_PATHS).
IM_ANIM_API int     iam_text_to_paths(ImGuiID base_id, char const* text, float size, iam_vec2 pos = iam_v2(0, 0), ImFont* font = nullptr);
IM_ANIM_API ImGuiID iam_text_path_id(ImGuiID base_id, int index);
// Draw the contours on with progress [0,1]; overlap 0 = one after another, 1 = all at once.
IM_ANIM_API void    iam_text_paths_draw(ImDrawList* dl, ImGuiID base_id, int count, float progress, ImU32 col, float thickness = 1.5f, iam_vec2 offset = iam_v2(0, 0), float overlap = 0.5f);

// Get current morph blend value from a tween (for querying state)
IM_ANIM_API float  iam_get_morph_blend(ImGuiID id, ImGuiID channel_id);

// ----------------------------------------------------
// Text along motion paths
// ----------------------------------------------------

// Text alignment along path
enum iam_text_path_align {
	iam_text_align_start = 0,    // Text starts at path start (or offset)
	iam_text_align_center,       // Text centered on path
	iam_text_align_end           // Text ends at path end
};

// Text path options
struct iam_text_path_opts {
	iam_vec2 origin;               // Screen-space origin for rendering (path coords are offset by this)
	float offset;                // Starting offset along path (pixels)
	float letter_spacing;        // Extra spacing between characters (pixels)
	int   align;                 // iam_text_path_align value
	bool  flip_y;                // Flip text vertically (for paths going right-to-left)
	ImU32 color;                 // Text color (default: white)
	ImFont* font;                // Font to use (nullptr = current font)
	float font_scale;            // Additional font scale (1.0 = normal)

	iam_text_path_opts() : origin(iam_v2(0, 0)), offset(0), letter_spacing(0), align(iam_text_align_start),
	                       flip_y(false), color(IM_COL32_WHITE), font(nullptr), font_scale(1.0f) {}
};

// Render text along a path (static - no animation)
IM_ANIM_API void iam_text_path(ImGuiID path_id, const char* text, iam_text_path_opts const& opts = iam_text_path_opts());

// Animated text along path (characters appear progressively)
IM_ANIM_API void iam_text_path_animated(ImGuiID path_id, const char* text, float progress, iam_text_path_opts const& opts = iam_text_path_opts());

// Helper: Get text width for path layout calculations
IM_ANIM_API float iam_text_path_width(const char* text, iam_text_path_opts const& opts = iam_text_path_opts());

// ----------------------------------------------------
// Quad transform helpers (for advanced custom rendering)
// ----------------------------------------------------

// Transform a quad (4 vertices) by rotation and translation
IM_ANIM_API void iam_transform_quad(iam_vec2* quad, iam_vec2 center, float angle_rad, iam_vec2 translation);

// Create a rotated quad for a glyph at a position on the path
IM_ANIM_API void iam_make_glyph_quad(iam_vec2* quad, iam_vec2 pos, float angle_rad, float glyph_width, float glyph_height, float baseline_offset = 0.0f);

// ----------------------------------------------------
// Text Stagger - per-character animation effects
// ----------------------------------------------------

// Text stagger effect types
enum iam_text_stagger_effect {
	iam_text_fx_none = 0,        // No effect (instant appear)
	iam_text_fx_fade,            // Fade in alpha
	iam_text_fx_scale,           // Scale from center
	iam_text_fx_slide_up,        // Slide up from below
	iam_text_fx_slide_down,      // Slide down from above
	iam_text_fx_slide_left,      // Slide in from right
	iam_text_fx_slide_right,     // Slide in from left
	iam_text_fx_rotate,          // Rotate in
	iam_text_fx_bounce,          // Bounce in with overshoot
	iam_text_fx_wave,            // Wave motion (continuous)
	iam_text_fx_typewriter       // Typewriter style (instant per char)
};

// Text stagger split units: what one stagger step reveals
enum iam_text_split {
	iam_text_split_chars = 0,    // One character per step (default)
	iam_text_split_words,        // One word per step (characters of a word animate together)
	iam_text_split_lines         // One line per step ('\n' separated)
};

// Per-glyph state passed to iam_text_stagger_opts::glyph_callback (after the built-in effect).
// Modify offset/scale/rotation/alpha/color to build custom effects (use iam_text_fx_none for a blank slate).
struct iam_text_glyph {
	int          index;          // Character index in the text
	int          unit;           // Stagger unit index (character, word or line, see split)
	unsigned int codepoint;
	float        t;              // Eased progress of this unit [0,1] (may overshoot with back/elastic eases)
	float        raw_t;          // Linear progress of this unit [0,1]
	iam_vec2     offset;         // Pixel offset (in/out)
	float        scale;          // Scale around the glyph center (in/out)
	float        rotation;       // Radians (in/out)
	float        alpha;          // 0..1 (in/out)
	ImU32        color;          // Glyph color, alpha applied after (in/out)
};
typedef void (*iam_text_glyph_fn)(iam_text_glyph* glyph, void* user);

// Text stagger options
struct iam_text_stagger_opts {
	iam_vec2 pos;                  // Base position for text
	int    effect;               // iam_text_stagger_effect
	float  char_delay;           // Delay between each stagger unit (character, word or line; seconds)
	float  char_duration;        // Duration of each character's animation (seconds)
	float  effect_intensity;     // Intensity of effect (pixels for slide, degrees for rotate, scale factor)
	iam_ease_desc ease;          // Easing for character animation
	ImU32  color;                // Text color
	ImFont* font;                // Font to use (nullptr = current)
	float  font_scale;           // Font scale multiplier
	float  letter_spacing;       // Extra spacing between characters
	int    split;                // iam_text_split: stagger by character, word or line ('\n' starts a new line)
	float  line_spacing;         // Extra spacing between lines (pixels)
	iam_text_glyph_fn glyph_callback;  // Optional per-glyph hook (custom effects)
	void*  glyph_user;           // User data for glyph_callback

	iam_text_stagger_opts()
		: pos(iam_v2(0, 0)), effect(iam_text_fx_fade), char_delay(0.05f), char_duration(0.3f),
		  effect_intensity(20.0f), ease(iam_ease_preset(iam_ease_out_cubic)),
		  color(IM_COL32_WHITE), font(nullptr), font_scale(1.0f), letter_spacing(0.0f),
		  split(iam_text_split_chars), line_spacing(0.0f), glyph_callback(nullptr), glyph_user(nullptr) {}
};

// Render text with per-character stagger animation
IM_ANIM_API void iam_text_stagger(ImGuiID id, const char* text, float progress, iam_text_stagger_opts const& opts = iam_text_stagger_opts());

// Get text width for layout calculations
IM_ANIM_API float iam_text_stagger_width(const char* text, iam_text_stagger_opts const& opts = iam_text_stagger_opts());

// Get total animation duration for text (accounts for stagger delays)
IM_ANIM_API float iam_text_stagger_duration(const char* text, iam_text_stagger_opts const& opts = iam_text_stagger_opts());

// Text scramble ("decode" effect): characters not yet revealed show random symbols that change over time.
// Stateless: writes into out and returns its length, so it works in any label (Text, Button, ...).
struct iam_text_scramble_opts {
	char const* charset;        // Symbols for unrevealed characters (default: letters, digits, symbols)
	float       change_rate;    // Symbol changes per second (default 20)
	bool        random_order;   // Reveal in random order instead of left to right (default false)
	bool        keep_spaces;    // Leave spaces untouched (default true)
	unsigned    seed;           // Varies the random symbols / order between labels
	iam_text_scramble_opts() : charset("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789#%&*+=?!<>/"),
		change_rate(20.0f), random_order(false), keep_spaces(true), seed(0) {}
};
IM_ANIM_API int iam_text_scramble(char* out, int out_size, char const* text, float progress, iam_text_scramble_opts const& opts = iam_text_scramble_opts()); // progress 0 = all scrambled, 1 = final text.

// ----------------------------------------------------
// Noise Channels - Perlin/Simplex noise for organic movement
// ----------------------------------------------------

// Noise types
enum iam_noise_type {
	iam_noise_perlin = 0,        // Classic Perlin noise
	iam_noise_simplex,           // Simplex noise (faster, fewer artifacts)
	iam_noise_value,             // Value noise (blocky)
	iam_noise_worley             // Worley/cellular noise
};

// Noise options
struct iam_noise_opts {
	int   type;                  // iam_noise_type
	int   octaves;               // Number of octaves for fractal noise (1-8)
	float persistence;           // Amplitude multiplier per octave (0.0-1.0)
	float lacunarity;            // Frequency multiplier per octave (typically 2.0)
	int   seed;                  // Random seed for noise generation

	iam_noise_opts()
		: type(iam_noise_perlin), octaves(4), persistence(0.5f), lacunarity(2.0f), seed(0) {}
};

// Sample noise at a point (returns value in [-1, 1])
IM_ANIM_API float  iam_noise_2d(float x, float y, iam_noise_opts const& opts = iam_noise_opts());                    // 2D noise
IM_ANIM_API float  iam_noise_3d(float x, float y, float z, iam_noise_opts const& opts = iam_noise_opts());        // 3D noise

// Animated noise channels - continuous noise that evolves over time
IM_ANIM_API float  iam_noise_channel_float(ImGuiID id, float frequency, float amplitude, iam_noise_opts const& opts, float dt);  // 1D animated noise
IM_ANIM_API iam_vec2 iam_noise_channel_vec2(ImGuiID id, iam_vec2 frequency, iam_vec2 amplitude, iam_noise_opts const& opts, float dt); // 2D animated noise
IM_ANIM_API iam_vec4 iam_noise_channel_vec4(ImGuiID id, iam_vec4 frequency, iam_vec4 amplitude, iam_noise_opts const& opts, float dt); // 4D animated noise
IM_ANIM_API iam_vec4 iam_noise_channel_color(ImGuiID id, iam_vec4 base_color, iam_vec4 amplitude, float frequency, iam_noise_opts const& opts, int color_space, float dt); // Animated color noise in specified color space

// Convenience: smooth random movement (like wiggle but using noise)
IM_ANIM_API float  iam_smooth_noise_float(ImGuiID id, float amplitude, float speed, float dt);                          // Simple 1D smooth noise
IM_ANIM_API iam_vec2 iam_smooth_noise_vec2(ImGuiID id, iam_vec2 amplitude, float speed, float dt);                    // Simple 2D smooth noise
IM_ANIM_API iam_vec4 iam_smooth_noise_vec4(ImGuiID id, iam_vec4 amplitude, float speed, float dt);                    // Simple 4D smooth noise
IM_ANIM_API iam_vec4 iam_smooth_noise_color(ImGuiID id, iam_vec4 base_color, iam_vec4 amplitude, float speed, int color_space, float dt); // Smooth noise for colors in specified color space

// ----------------------------------------------------
// Style Interpolation - animate between ImGuiStyle themes
// ----------------------------------------------------

// Register a named style for interpolation
IM_ANIM_API void iam_style_register(ImGuiID style_id, ImGuiStyle const& style);                                   // Register a style snapshot
IM_ANIM_API void iam_style_register_current(ImGuiID style_id);                                                    // Register current ImGui style

// Blend between two registered styles (result applied to ImGui::GetStyle())
// Uses iam_color_space for color blending mode (iam_col_oklab recommended)
IM_ANIM_API void iam_style_blend(ImGuiID style_a, ImGuiID style_b, float t, int color_space = iam_col_oklab);

// Tween between styles over time
IM_ANIM_API void iam_style_tween(ImGuiID id, ImGuiID target_style, float duration, iam_ease_desc const& ease, int color_space, float dt);

// Get interpolated style without applying
IM_ANIM_API void iam_style_blend_to(ImGuiID style_a, ImGuiID style_b, float t, ImGuiStyle* out_style, int color_space = iam_col_oklab);

// Check if a style is registered
IM_ANIM_API bool iam_style_exists(ImGuiID style_id);

// Remove a registered style
IM_ANIM_API void iam_style_unregister(ImGuiID style_id);

// ----------------------------------------------------
// Gradient Interpolation - animate between color gradients
// ----------------------------------------------------

// Color gradient with any number of stops (sorted by position)
struct iam_gradient {
	ImVector<float> positions;   // Positions along gradient [0,1], kept sorted
	ImVector<ImVec4> colors;       // Colors at each position (sRGB, internal storage)

	iam_gradient() {}

	// Add a stop to the gradient (automatically sorted by position)
	iam_gradient& add(float position, iam_vec4 color);
	iam_gradient& add(float position, ImU32 color) {
		return add(position, IAM_VEC4_FROM_IM(ImGui::ColorConvertU32ToFloat4(color)));
	}

	// Get stop count
	int stop_count() const { return positions.Size; }

	// Sample the gradient at position t [0,1]
	iam_vec4 sample(float t, int color_space = iam_col_oklab) const;

	// Create common gradients
	static iam_gradient solid(iam_vec4 color);
	static iam_gradient two_color(iam_vec4 start, iam_vec4 end);
	static iam_gradient three_color(iam_vec4 start, iam_vec4 mid, iam_vec4 end);
};

// Blend between two gradients
IM_ANIM_API iam_gradient iam_gradient_lerp(iam_gradient const& a, iam_gradient const& b, float t, int color_space = iam_col_oklab);

// Tween between gradients over time
IM_ANIM_API iam_gradient iam_tween_gradient(ImGuiID id, ImGuiID channel_id, iam_gradient const& target, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt);

// ----------------------------------------------------
// Transform Interpolation - animate 2D transforms
// ----------------------------------------------------

// Rotation interpolation modes
enum iam_rotation_mode {
	iam_rotation_shortest = 0,   // Shortest path (default) - never rotates more than 180 degrees
	iam_rotation_longest,        // Longest path - always takes the long way around
	iam_rotation_cw,             // Clockwise - always rotates clockwise (positive direction)
	iam_rotation_ccw,            // Counter-clockwise - always rotates counter-clockwise
	iam_rotation_direct          // Direct lerp - no angle unwrapping, can cause spinning for large deltas
};

// 2D transform (position, rotation, scale)
struct iam_transform {
	iam_vec2 position;             // Translation
	iam_vec2 scale;                // Scale (1,1 = identity)
	float rotation;              // Rotation in radians

	iam_transform() : position(iam_v2(0, 0)), scale(iam_v2(1, 1)), rotation(0) {}
	iam_transform(iam_vec2 pos, float rot = 0, iam_vec2 scl = iam_v2(1, 1))
		: position(pos), scale(scl), rotation(rot) {}

	// Create identity transform
	static iam_transform identity() { return iam_transform(); }

	// Combine transforms (this * other)
	iam_transform operator*(iam_transform const& other) const;

	// Apply transform to a point
	iam_vec2 apply(iam_vec2 point) const;

	// Get inverse transform
	iam_transform inverse() const;
};

// Blend between two transforms with rotation interpolation
IM_ANIM_API iam_transform iam_transform_lerp(iam_transform const& a, iam_transform const& b, float t, int rotation_mode = iam_rotation_shortest);

// Tween between transforms over time
IM_ANIM_API iam_transform iam_tween_transform(ImGuiID id, ImGuiID channel_id, iam_transform const& target, float dur, iam_ease_desc const& ez, int policy, int rotation_mode, float dt);

// Decompose a 3x2 matrix into transform components
IM_ANIM_API iam_transform iam_transform_from_matrix(float m00, float m01, float m10, float m11, float tx, float ty);

// Convert transform to 3x2 matrix (row-major: [m00 m01 tx; m10 m11 ty])
IM_ANIM_API void iam_transform_to_matrix(iam_transform const& t, float* out_matrix);

// ============================================================
// CLIP-BASED ANIMATION SYSTEM
// ============================================================

// Direction for looping animations
enum iam_direction {
	iam_dir_normal = 0,		// play forward
	iam_dir_reverse,		// play backward
	iam_dir_alternate		// ping-pong
};

// Channel types for keyframes
enum iam_channel_type {
	iam_chan_float = 0,
	iam_chan_vec2,
	iam_chan_vec4,
	iam_chan_int,
	iam_chan_color,     // Color with color space (stores in vec4 + color_space metadata)
	iam_chan_float_rel, // Float relative to anchor (percent + px_bias)
	iam_chan_vec2_rel,  // Vec2 relative to anchor (percent.xy + px_bias.xy)
	iam_chan_vec4_rel,  // Vec4 relative to anchor (percent.xy + px_bias.xy for x,y; z,w absolute)
	iam_chan_color_rel  // Color relative to anchor (for position-based color effects)
};

// Result codes
enum iam_result {
	iam_ok = 0,
	iam_err_not_found,
	iam_err_bad_arg,
	iam_err_no_mem
};

// Spring parameters for physics-based animation
struct iam_spring_params {
	float mass;
	float stiffness;
	float damping;
	float initial_velocity;
};

// ----------------------------------------------------
// Spring channels - physically simulated, no fixed duration
// ----------------------------------------------------
// Call every frame with the target. When the target changes mid-motion the value keeps its velocity
// (no restart from rest), which suits drag release, follow-the-mouse and interrupted transitions.
// Simulated once per frame per (id, channel); dt is scaled by iam_set_global_time_scale().
// params.initial_velocity applies when the channel is created. Defaults: mass 1, stiffness 170, damping 26.
static inline iam_spring_params iam_spring_default() { iam_spring_params p = { 1.0f, 170.0f, 26.0f, 0.0f }; return p; }
IM_ANIM_API float    iam_spring_float(ImGuiID id, ImGuiID channel_id, float target, iam_spring_params const& params, float dt, float init_value = 0.0f);
IM_ANIM_API iam_vec2 iam_spring_vec2(ImGuiID id, ImGuiID channel_id, iam_vec2 target, iam_spring_params const& params, float dt, iam_vec2 init_value = iam_v2(0, 0));
IM_ANIM_API iam_vec4 iam_spring_vec4(ImGuiID id, ImGuiID channel_id, iam_vec4 target, iam_spring_params const& params, float dt, iam_vec4 init_value = iam_v4(0, 0, 0, 0));
IM_ANIM_API iam_vec4 iam_spring_color(ImGuiID id, ImGuiID channel_id, iam_vec4 target_srgb, iam_spring_params const& params, int color_space, float dt, iam_vec4 init_value = iam_v4(1, 1, 1, 1)); // Simulated in color_space (oklab recommended; hue spaces move hue linearly).

// ----------------------------------------------------
// Presence - enter/exit animations
// ----------------------------------------------------
// Call every frame with the desired visibility. Returns true while the element should be drawn
// (visible, or still animating out); *out_t goes 0 -> 1 on enter and 1 -> 0 on exit (may overshoot
// with back/elastic eases). Reversing mid-way continues from the current value.
//   float t;
//   if (iam_presence(id, show_popup, &t, dt)) { ImGui::SetNextWindowBgAlpha(t); ... draw ... }
struct iam_presence_opts {
	float         enter_duration;   // Seconds for 0 -> 1 (default 0.2)
	float         exit_duration;    // Seconds for 1 -> 0 (default 0.15)
	iam_ease_desc enter_ease;       // Default out_cubic
	iam_ease_desc exit_ease;        // Default in_cubic
	bool          animate_first;    // Animate in when first seen visible (default true); false starts fully shown

	iam_presence_opts() : enter_duration(0.2f), exit_duration(0.15f), animate_first(true) {
		enter_ease.type = iam_ease_out_cubic; enter_ease.p0 = enter_ease.p1 = enter_ease.p2 = enter_ease.p3 = 0.0f;
		exit_ease.type = iam_ease_in_cubic; exit_ease.p0 = exit_ease.p1 = exit_ease.p2 = exit_ease.p3 = 0.0f;
	}
};
IM_ANIM_API bool iam_presence(ImGuiID id, bool visible, float* out_t, float dt, iam_presence_opts const& opts = iam_presence_opts());
IM_ANIM_API bool iam_presence_is_animating(ImGuiID id);  // True while entering or exiting.

// ----------------------------------------------------
// Layout animation - smooth position/size changes caused by layout (reorder, resize, insert)
// ----------------------------------------------------
// Pass the rect the layout wants this frame; get the animated rect to draw at. Springs keep velocity when
// the layout changes again mid-motion. First sight snaps (no fly-in). Use window-relative coordinates if
// the window can move, or items will lag behind window drags. Returns true while animating.
//   ImVec2 slot = ImGui::GetCursorPos(), pos, size;
//   iam_layout(id, slot, item_size, &pos, &size, dt);
//   ImGui::SetCursorPos(pos); ...draw item...; ImGui::SetCursorPos(slot); ImGui::Dummy(item_size);
struct iam_layout_opts {
	iam_spring_params spring;   // Default: mass 1, stiffness 320, damping 36 (fast, critically damped: no overshoot)
	bool animate_position;      // Default true
	bool animate_size;          // Default true
	iam_layout_opts() : animate_position(true), animate_size(true) { spring = iam_spring_default(); spring.stiffness = 320.0f; spring.damping = 36.0f; }
};
IM_ANIM_API bool iam_layout(ImGuiID id, iam_vec2 target_pos, iam_vec2 target_size, iam_vec2* out_pos, iam_vec2* out_size, float dt, iam_layout_opts const& opts = iam_layout_opts());

// ----------------------------------------------------
// Scroll-linked progress and triggers (current window, last submitted item)
// ----------------------------------------------------
enum iam_scroll_event {
	iam_scroll_none    = 0,
	iam_scroll_entered = 1 << 0,   // Became visible this frame
	iam_scroll_left    = 1 << 1    // Stopped being visible this frame
};
IM_ANIM_API float iam_scroll_window_progress(int axis = 1);                        // Scroll position / max in [0,1] (axis 0 = x, 1 = y; 0 when not scrollable).
IM_ANIM_API float iam_scroll_item_progress();                                     // 0 when the last item's top reaches the bottom of the visible area, 1 when its bottom passes the top.
IM_ANIM_API bool  iam_scroll_item_visible(float min_fraction = 0.0f);             // Last item visible (more than min_fraction of its height) in the current window.
IM_ANIM_API int   iam_scroll_trigger(ImGuiID id, float min_fraction = 0.0f, bool* out_visible = nullptr); // iam_scroll_event flags for the last item (enter/leave edges).

// ----------------------------------------------------
// Inertia - fling with friction, rubber-band bounds and snapping
// ----------------------------------------------------
// While held, the value follows pos and its velocity is tracked. On release it keeps moving and slows down
// (friction), springs back inside bounds (rubber band), and with snapping lands on the snap target closest
// to where it would have stopped. Returns the value to display.
struct iam_inertia_opts {
	float             decay_time;     // Velocity time constant in seconds (default 0.325, iOS-like); larger = glides further
	bool              use_bounds;     // Clamp to [bounds_min, bounds_max] with rubber band (default false)
	iam_vec2          bounds_min, bounds_max;
	float             rubber_band;    // Resistance past bounds while held: 0 = hard stop, 1 = free (default 0.35)
	iam_vec2          snap_grid;      // Snap spacing per axis (0 = none)
	iam_vec2 const*   snap_points;    // Optional snap points (vec2 version; x only for the float version)
	int               snap_points_count;
	iam_spring_params spring;         // Spring for bounce-back and snapping (default stiffness 220, damping 26)
	iam_inertia_opts() : decay_time(0.325f), use_bounds(false), bounds_min(iam_v2(0, 0)), bounds_max(iam_v2(0, 0)), rubber_band(0.35f),
		snap_grid(iam_v2(0, 0)), snap_points(nullptr), snap_points_count(0) { spring = iam_spring_default(); spring.stiffness = 220.0f; spring.damping = 26.0f; }
};
IM_ANIM_API float    iam_inertia_float(ImGuiID id, float pos, bool held, float dt, iam_inertia_opts const& opts = iam_inertia_opts());   // Uses the .x of bounds/grid/points.
IM_ANIM_API iam_vec2 iam_inertia_vec2(ImGuiID id, iam_vec2 pos, bool held, float dt, iam_inertia_opts const& opts = iam_inertia_opts());
IM_ANIM_API bool     iam_inertia_is_moving(ImGuiID id);   // True while coasting, bouncing or snapping after release.
IM_ANIM_API void     iam_inertia_set(ImGuiID id, iam_vec2 pos);  // Teleport and stop (e.g. programmatic jump).

// ----------------------------------------------------
// Tween state queries - work for iam_tween_* and iam_spring_* channels of any type
// ----------------------------------------------------
IM_ANIM_API bool     iam_tween_is_active(ImGuiID id, ImGuiID channel_id);       // True while animating (or a queued target is pending).
IM_ANIM_API float    iam_tween_progress(ImGuiID id, ImGuiID channel_id);        // Time progress [0,1] of the current tween (1 when idle or unknown; springs: 0 moving, 1 settled).
IM_ANIM_API float    iam_tween_velocity_float(ImGuiID id, ImGuiID channel_id);  // Units per second (0 when idle or unknown).
IM_ANIM_API iam_vec2 iam_tween_velocity_vec2(ImGuiID id, ImGuiID channel_id);
IM_ANIM_API iam_vec4 iam_tween_velocity_vec4(ImGuiID id, ImGuiID channel_id);   // Also for color channels (per component, in their blend space).

// ----------------------------------------------------
// Repeat with Variation - per-loop parameter changes
// ----------------------------------------------------

// Variation modes for repeat animations
enum iam_variation_mode {
	iam_var_none = 0,       // No variation
	iam_var_increment,      // Add amount each iteration
	iam_var_decrement,      // Subtract amount each iteration
	iam_var_multiply,       // Multiply by factor each iteration
	iam_var_random,         // Random in range [-amount, +amount]
	iam_var_random_abs,     // Random in range [0, amount]
	iam_var_pingpong,       // Alternate +/- each iteration
	iam_var_callback        // Use custom callback
};

// Velocity of any value you feed every frame (mouse, scroll position, your own state), smoothed with a
// time constant in seconds. Units per second; 0 on the first call. Uses real dt (not the global time scale).
IM_ANIM_API float    iam_track_velocity_float(ImGuiID id, float value, float dt, float smoothing = 0.05f);
IM_ANIM_API iam_vec2 iam_track_velocity_vec2(ImGuiID id, iam_vec2 value, float dt, float smoothing = 0.05f);

// Callback types for custom variation logic
typedef float  (*iam_variation_float_fn)(int index, void* user);
typedef int    (*iam_variation_int_fn)(int index, void* user);
typedef iam_vec2 (*iam_variation_vec2_fn)(int index, void* user);
typedef iam_vec4 (*iam_variation_vec4_fn)(int index, void* user);

// Float variation
struct iam_variation_float {
	int                     mode;
	float                   amount;
	float                   min_clamp;
	float                   max_clamp;
	unsigned int            seed;       // 0 = global random, non-zero = deterministic
	iam_variation_float_fn  callback;
	void*                   user;
};

// Int variation
struct iam_variation_int {
	int                   mode;
	int                   amount;
	int                   min_clamp;
	int                   max_clamp;
	unsigned int          seed;
	iam_variation_int_fn  callback;
	void*                 user;
};

// Vec2 variation (global mode or per-axis)
struct iam_variation_vec2 {
	int                    mode;        // Global mode (iam_var_none = use per-axis)
	iam_vec2                 amount;
	iam_vec2                 min_clamp;
	iam_vec2                 max_clamp;
	unsigned int           seed;
	iam_variation_vec2_fn  callback;
	void*                  user;
	iam_variation_float    x, y;        // Per-axis (used when mode == iam_var_none)
};

// Vec4 variation (global mode or per-axis)
struct iam_variation_vec4 {
	int                    mode;
	iam_vec4                 amount;
	iam_vec4                 min_clamp;
	iam_vec4                 max_clamp;
	unsigned int           seed;
	iam_variation_vec4_fn  callback;
	void*                  user;
	iam_variation_float    x, y, z, w;  // Per-axis
};

// Color variation (global mode or per-channel)
struct iam_variation_color {
	int                    mode;
	iam_vec4                 amount;
	iam_vec4                 min_clamp;
	iam_vec4                 max_clamp;
	int                    color_space; // iam_col_oklab, etc.
	unsigned int           seed;
	iam_variation_vec4_fn  callback;    // Returns color delta
	void*                  user;
	iam_variation_float    r, g, b, a;  // Per-channel
};

// ----------------------------------------------------
// Variation helper functions (C11-style inline)
// ----------------------------------------------------

// Float variation helpers
static inline iam_variation_float iam_varf_none(void) {
	iam_variation_float v = {iam_var_none, 0, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_inc(float amt) {
	iam_variation_float v = {iam_var_increment, amt, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_dec(float amt) {
	iam_variation_float v = {iam_var_decrement, amt, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_mul(float f) {
	iam_variation_float v = {iam_var_multiply, f, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_rand(float r) {
	iam_variation_float v = {iam_var_random, r, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_rand_abs(float r) {
	iam_variation_float v = {iam_var_random_abs, r, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_pingpong(float amt) {
	iam_variation_float v = {iam_var_pingpong, amt, -FLT_MAX, FLT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_float iam_varf_fn(iam_variation_float_fn fn, void* user) {
	iam_variation_float v = {iam_var_callback, 0, -FLT_MAX, FLT_MAX, 0, fn, user};
	return v;
}
static inline iam_variation_float iam_varf_clamp(iam_variation_float v, float mn, float mx) {
	v.min_clamp = mn; v.max_clamp = mx; return v;
}
static inline iam_variation_float iam_varf_seed(iam_variation_float v, unsigned int s) {
	v.seed = s; return v;
}

// Int variation helpers
static inline iam_variation_int iam_vari_none(void) {
	iam_variation_int v = {iam_var_none, 0, INT_MIN, INT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_int iam_vari_inc(int amt) {
	iam_variation_int v = {iam_var_increment, amt, INT_MIN, INT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_int iam_vari_dec(int amt) {
	iam_variation_int v = {iam_var_decrement, amt, INT_MIN, INT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_int iam_vari_rand(int r) {
	iam_variation_int v = {iam_var_random, r, INT_MIN, INT_MAX, 0, 0, 0};
	return v;
}
static inline iam_variation_int iam_vari_fn(iam_variation_int_fn fn, void* user) {
	iam_variation_int v = {iam_var_callback, 0, INT_MIN, INT_MAX, 0, fn, user};
	return v;
}
static inline iam_variation_int iam_vari_clamp(iam_variation_int v, int mn, int mx) {
	v.min_clamp = mn; v.max_clamp = mx; return v;
}
static inline iam_variation_int iam_vari_seed(iam_variation_int v, unsigned int s) {
	v.seed = s; return v;
}

// Vec2 variation helpers (global)
static inline iam_variation_vec2 iam_varv2_none(void) {
	iam_variation_vec2 v = {iam_var_none, iam_v2(0,0), iam_v2(-FLT_MAX,-FLT_MAX), iam_v2(FLT_MAX,FLT_MAX), 0, 0, 0, {0}, {0}};
	return v;
}
static inline iam_variation_vec2 iam_varv2_inc(float x, float y) {
	iam_variation_vec2 v = {iam_var_increment, iam_v2(x,y), iam_v2(-FLT_MAX,-FLT_MAX), iam_v2(FLT_MAX,FLT_MAX), 0, 0, 0, {0}, {0}};
	return v;
}
static inline iam_variation_vec2 iam_varv2_dec(float x, float y) {
	iam_variation_vec2 v = {iam_var_decrement, iam_v2(x,y), iam_v2(-FLT_MAX,-FLT_MAX), iam_v2(FLT_MAX,FLT_MAX), 0, 0, 0, {0}, {0}};
	return v;
}
static inline iam_variation_vec2 iam_varv2_mul(float f) {
	iam_variation_vec2 v = {iam_var_multiply, iam_v2(f,f), iam_v2(-FLT_MAX,-FLT_MAX), iam_v2(FLT_MAX,FLT_MAX), 0, 0, 0, {0}, {0}};
	return v;
}
static inline iam_variation_vec2 iam_varv2_rand(float x, float y) {
	iam_variation_vec2 v = {iam_var_random, iam_v2(x,y), iam_v2(-FLT_MAX,-FLT_MAX), iam_v2(FLT_MAX,FLT_MAX), 0, 0, 0, {0}, {0}};
	return v;
}
static inline iam_variation_vec2 iam_varv2_fn(iam_variation_vec2_fn fn, void* user) {
	iam_variation_vec2 v = {iam_var_callback, iam_v2(0,0), iam_v2(-FLT_MAX,-FLT_MAX), iam_v2(FLT_MAX,FLT_MAX), 0, fn, user, {0}, {0}};
	return v;
}
// Vec2 per-axis helper
static inline iam_variation_vec2 iam_varv2_axis(iam_variation_float vx, iam_variation_float vy) {
	iam_variation_vec2 v = {iam_var_none, iam_v2(0,0), iam_v2(-FLT_MAX,-FLT_MAX), iam_v2(FLT_MAX,FLT_MAX), 0, 0, 0, vx, vy};
	return v;
}
static inline iam_variation_vec2 iam_varv2_clamp(iam_variation_vec2 v, iam_vec2 mn, iam_vec2 mx) {
	v.min_clamp = mn; v.max_clamp = mx; return v;
}
static inline iam_variation_vec2 iam_varv2_seed(iam_variation_vec2 v, unsigned int s) {
	v.seed = s; return v;
}

// Vec4 variation helpers (global)
static inline iam_variation_vec4 iam_varv4_none(void) {
	iam_variation_vec4 v = {iam_var_none, iam_v4(0,0,0,0), iam_v4(-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX), iam_v4(FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX), 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_vec4 iam_varv4_inc(float x, float y, float z, float w) {
	iam_variation_vec4 v = {iam_var_increment, iam_v4(x,y,z,w), iam_v4(-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX), iam_v4(FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX), 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_vec4 iam_varv4_dec(float x, float y, float z, float w) {
	iam_variation_vec4 v = {iam_var_decrement, iam_v4(x,y,z,w), iam_v4(-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX), iam_v4(FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX), 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_vec4 iam_varv4_mul(float f) {
	iam_variation_vec4 v = {iam_var_multiply, iam_v4(f,f,f,f), iam_v4(-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX), iam_v4(FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX), 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_vec4 iam_varv4_rand(float x, float y, float z, float w) {
	iam_variation_vec4 v = {iam_var_random, iam_v4(x,y,z,w), iam_v4(-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX), iam_v4(FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX), 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_vec4 iam_varv4_fn(iam_variation_vec4_fn fn, void* user) {
	iam_variation_vec4 v = {iam_var_callback, iam_v4(0,0,0,0), iam_v4(-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX), iam_v4(FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX), 0, fn, user, {0}, {0}, {0}, {0}};
	return v;
}
// Vec4 per-axis helper
static inline iam_variation_vec4 iam_varv4_axis(iam_variation_float vx, iam_variation_float vy, iam_variation_float vz, iam_variation_float vw) {
	iam_variation_vec4 v = {iam_var_none, iam_v4(0,0,0,0), iam_v4(-FLT_MAX,-FLT_MAX,-FLT_MAX,-FLT_MAX), iam_v4(FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX), 0, 0, 0, vx, vy, vz, vw};
	return v;
}
static inline iam_variation_vec4 iam_varv4_clamp(iam_variation_vec4 v, iam_vec4 mn, iam_vec4 mx) {
	v.min_clamp = mn; v.max_clamp = mx; return v;
}
static inline iam_variation_vec4 iam_varv4_seed(iam_variation_vec4 v, unsigned int s) {
	v.seed = s; return v;
}

// Color variation helpers (global)
static inline iam_variation_color iam_varc_none(void) {
	iam_variation_color v = {iam_var_none, iam_v4(0,0,0,0), iam_v4(0,0,0,0), iam_v4(1,1,1,1), iam_col_oklab, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_color iam_varc_inc(float r, float g, float b, float a) {
	iam_variation_color v = {iam_var_increment, iam_v4(r,g,b,a), iam_v4(0,0,0,0), iam_v4(1,1,1,1), iam_col_oklab, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_color iam_varc_dec(float r, float g, float b, float a) {
	iam_variation_color v = {iam_var_decrement, iam_v4(r,g,b,a), iam_v4(0,0,0,0), iam_v4(1,1,1,1), iam_col_oklab, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_color iam_varc_mul(float f) {
	iam_variation_color v = {iam_var_multiply, iam_v4(f,f,f,1), iam_v4(0,0,0,0), iam_v4(1,1,1,1), iam_col_oklab, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_color iam_varc_rand(float r, float g, float b, float a) {
	iam_variation_color v = {iam_var_random, iam_v4(r,g,b,a), iam_v4(0,0,0,0), iam_v4(1,1,1,1), iam_col_oklab, 0, 0, 0, {0}, {0}, {0}, {0}};
	return v;
}
static inline iam_variation_color iam_varc_fn(iam_variation_vec4_fn fn, void* user) {
	iam_variation_color v = {iam_var_callback, iam_v4(0,0,0,0), iam_v4(0,0,0,0), iam_v4(1,1,1,1), iam_col_oklab, 0, fn, user, {0}, {0}, {0}, {0}};
	return v;
}
// Color per-channel helper
static inline iam_variation_color iam_varc_channel(iam_variation_float vr, iam_variation_float vg, iam_variation_float vb, iam_variation_float va) {
	iam_variation_color v = {iam_var_none, iam_v4(0,0,0,0), iam_v4(0,0,0,0), iam_v4(1,1,1,1), iam_col_oklab, 0, 0, 0, vr, vg, vb, va};
	return v;
}
static inline iam_variation_color iam_varc_space(iam_variation_color v, int space) {
	v.color_space = space; return v;
}
static inline iam_variation_color iam_varc_clamp(iam_variation_color v, iam_vec4 mn, iam_vec4 mx) {
	v.min_clamp = mn; v.max_clamp = mx; return v;
}
static inline iam_variation_color iam_varc_seed(iam_variation_color v, unsigned int s) {
	v.seed = s; return v;
}

// Forward declarations
struct iam_clip_data;
struct iam_instance_data;

// Callback types (use instance ID instead of pointer for safety)
typedef void (*iam_clip_callback)(ImGuiID inst_id, void* user_data);

// Loop callback (includes the 0-based loop index)
typedef void (*iam_loop_callback)(ImGuiID inst_id, int loop_index, void* user_data);

// Marker callback (includes marker name/id for identification)
typedef void (*iam_marker_callback)(ImGuiID inst_id, ImGuiID marker_id, float marker_time, void* user_data);

// ----------------------------------------------------
// iam_clip - fluent API for authoring animations
// ----------------------------------------------------
class IM_ANIM_API iam_clip {
public:
	// Start building a new clip with the given ID
	static iam_clip begin(ImGuiID clip_id);

	// Add keyframes for different channel types
	iam_clip& key_float(ImGuiID channel, float time, float value, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec2(ImGuiID channel, float time, iam_vec2 value, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec4(ImGuiID channel, float time, iam_vec4 value, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_int(ImGuiID channel, float time, int value, int ease_type = iam_ease_linear);
	iam_clip& key_color(ImGuiID channel, float time, iam_vec4 value, int color_space = iam_col_oklab, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);

	// Keyframes with repeat variation (value changes per loop iteration)
	iam_clip& key_float_var(ImGuiID channel, float time, float value, iam_variation_float const& var, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec2_var(ImGuiID channel, float time, iam_vec2 value, iam_variation_vec2 const& var, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec4_var(ImGuiID channel, float time, iam_vec4 value, iam_variation_vec4 const& var, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_int_var(ImGuiID channel, float time, int value, iam_variation_int const& var, int ease_type = iam_ease_linear);
	iam_clip& key_color_var(ImGuiID channel, float time, iam_vec4 value, iam_variation_color const& var, int color_space = iam_col_oklab, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);

	// Spring-based keyframe (float only)
	iam_clip& key_float_spring(ImGuiID channel, float time, float target, iam_spring_params const& spring);

	// Anchor-relative keyframes (values resolved relative to window/viewport at get time)
	iam_clip& key_float_rel(ImGuiID channel, float time, float percent, float px_bias, int anchor_space, int axis, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec2_rel(ImGuiID channel, float time, iam_vec2 percent, iam_vec2 px_bias, int anchor_space, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec4_rel(ImGuiID channel, float time, iam_vec4 percent, iam_vec4 px_bias, int anchor_space, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_color_rel(ImGuiID channel, float time, iam_vec4 percent, iam_vec4 px_bias, int color_space, int anchor_space, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);

	// Timeline grouping - sequential and parallel keyframe blocks
	iam_clip& seq_begin();  // Start sequential block (keyframes after seq_end start after this block)
	iam_clip& seq_end();
	iam_clip& par_begin();  // Start parallel block (keyframes play at same time offset)
	iam_clip& par_end();

	// Timeline markers - callbacks at specific times during playback
	iam_clip& marker(float time, ImGuiID marker_id, iam_marker_callback cb, void* user = nullptr);  // Add marker at specific time.
	iam_clip& marker(float time, iam_marker_callback cb, void* user = nullptr);                     // Add marker (auto-generated ID).

	// Labels - named times for iam_instance::seek_label / tween_to_label (relative to the current seq/par offset, like markers)
	iam_clip& label(ImGuiID label_id, float time);

	// Clip options
	iam_clip& set_loop(bool loop, int direction = iam_dir_normal, int loop_count = -1);
	iam_clip& set_loop_delay(float delay_seconds);  // Delay between loop iterations
	iam_clip& set_delay(float delay_seconds);
	iam_clip& set_stagger(int count, float each_delay, float from_center_bias = 0.0f);
	iam_clip& set_stagger_ease(int ease_type);  // Ease the delay distribution (default: linear)

	// Timing variation per loop iteration
	iam_clip& set_duration_var(iam_variation_float const& var);   // Vary clip duration per loop
	iam_clip& set_delay_var(iam_variation_float const& var);      // Vary delay between loops
	iam_clip& set_timescale_var(iam_variation_float const& var);  // Vary playback speed per loop

	// Callbacks
	iam_clip& on_begin(iam_clip_callback cb, void* user = nullptr);
	iam_clip& on_update(iam_clip_callback cb, void* user = nullptr);
	iam_clip& on_complete(iam_clip_callback cb, void* user = nullptr);
	iam_clip& on_loop(iam_loop_callback cb, void* user = nullptr);    // Callback per loop iteration
	iam_clip& on_pause(iam_clip_callback cb, void* user = nullptr);   // Callback when paused

	// Finalize the clip
	void end();

	// Get the clip ID
	ImGuiID id() const { return m_clip_id; }

private:
	iam_clip(ImGuiID clip_id) : m_clip_id(clip_id) {}
	ImGuiID m_clip_id;
};

// ----------------------------------------------------
// iam_instance - playback control for a clip
// ----------------------------------------------------
class IM_ANIM_API iam_instance {
public:
	iam_instance() : m_inst_id(0) {}
	iam_instance(ImGuiID inst_id) : m_inst_id(inst_id) {}

	// Playback control
	void pause();
	void resume();
	void stop();
	void restart();  // Reset to t=0 and start playing (equivalent to stop + play on same clip)
	void reset();    // Reset to t=0 and re-evaluate tracks, but do not start playing
	void refresh();  // Re-evaluate all tracks at the current time (re-read current values)
	void destroy();  // Remove instance from system (valid() will return false after this)
	void seek(float time);
	void set_time_scale(float scale);

	// Playhead control
	void reverse();                                    // Flip playback direction now (a finished instance plays back the other way).
	bool is_reversed() const;
	void tween_to(float time, float duration, int ease_type = iam_ease_in_out_cubic);  // Ease the playhead to time, then hold there (paused). Works from paused/stopped too.
	bool tween_to_label(ImGuiID label_id, float duration, int ease_type = iam_ease_in_out_cubic);  // false if the clip has no such label.
	bool seek_label(ImGuiID label_id);                 // Jump to a label (and re-evaluate). false if the clip has no such label.
	void set_weight(float weight);  // for layering/blending

	// Animation chaining - play another clip when this one completes
	iam_instance& then(ImGuiID next_clip_id);                                        // Chain another clip to play after this one.
	iam_instance& then(ImGuiID next_clip_id, ImGuiID next_instance_id);              // Chain with specific instance ID.
	iam_instance& then_delay(float delay);                                           // Set delay before chained clip starts.

	// Query state
	float time() const;
	float duration() const;
	bool is_playing() const;
	bool is_paused() const;

	// Get animated values
	bool get_float(ImGuiID channel, float* out) const;
	bool get_vec2(ImGuiID channel, iam_vec2* out) const;
	bool get_vec4(ImGuiID channel, iam_vec4* out) const;
	bool get_int(ImGuiID channel, int* out) const;
	bool get_color(ImGuiID channel, iam_vec4* out, int color_space = iam_col_oklab) const;  // Color blended in specified color space.

	// Check validity
	bool valid() const;
	operator bool() const { return valid(); }

	ImGuiID id() const { return m_inst_id; }

private:
	ImGuiID m_inst_id;
};

// ----------------------------------------------------
// Clip System API
// ----------------------------------------------------

// Initialize/shutdown (optional - auto-init on first use)
IM_ANIM_API void iam_clip_init(int initial_clip_cap = 256, int initial_inst_cap = 4096);
IM_ANIM_API void iam_clip_shutdown();

// Per-frame update (call after iam_update_begin_frame)
IM_ANIM_API void iam_clip_update(float dt);

// Garbage collection for instances
IM_ANIM_API void iam_clip_gc(unsigned int max_age_frames = 600);

// Play a clip on an instance (creates or reuses instance)
IM_ANIM_API iam_instance iam_play(ImGuiID clip_id, ImGuiID instance_id);

// Get an existing instance (returns invalid iam_instance if not found)
IM_ANIM_API iam_instance iam_get_instance(ImGuiID instance_id);

// Query clip info
IM_ANIM_API float iam_clip_duration(ImGuiID clip_id);                                       // Get clip duration in seconds.
IM_ANIM_API bool iam_clip_exists(ImGuiID clip_id);                                          // Check if clip exists.
IM_ANIM_API float iam_clip_label_time(ImGuiID clip_id, ImGuiID label_id);                   // Time of a label, or -1 if missing.

// Stagger helpers - compute delay for indexed instances
IM_ANIM_API float iam_stagger_delay(ImGuiID clip_id, int index);                            // Get stagger delay for element at index.
IM_ANIM_API iam_instance iam_play_stagger(ImGuiID clip_id, ImGuiID instance_id, int index); // Play with stagger delay applied.

// Grid stagger - 2D grid-based delay distribution
enum iam_stagger_from {
	iam_stagger_first  = 0,     // Stagger from first element (index 0)
	iam_stagger_last,           // Stagger from last element
	iam_stagger_center,         // Stagger from center of grid
	iam_stagger_index           // Stagger from a specific index (use from_index)
};

enum iam_stagger_axis {
	iam_stagger_both = 0,       // Distance on both axes
	iam_stagger_x,              // Distance on X axis only (column)
	iam_stagger_y               // Distance on Y axis only (row)
};

struct iam_stagger_grid_opts {
	int   cols;                 // Number of columns
	int   rows;                 // Number of rows
	int   from;                 // iam_stagger_from - origin for distance calculation
	int   from_index;           // Used when from == iam_stagger_index
	int   axis;                 // iam_stagger_axis - constrain distance to an axis
	float delay;                // Base delay between elements (seconds)
	int   ease;                 // iam_ease_type - easing applied to the delay distribution
	float start_delay;          // Initial delay offset before first element (seconds)
	bool  reversed;             // Reverse the order (farthest element first)
	float jitter;               // Random +/- seconds added per element (deterministic per index and seed)
	unsigned seed;              // Seed for jitter

	iam_stagger_grid_opts() : cols(1), rows(1), from(iam_stagger_first),
	                          from_index(0), axis(iam_stagger_both),
	                          delay(0.05f), ease(iam_ease_linear), start_delay(0),
	                          reversed(false), jitter(0.0f), seed(0) {}
};

// Get stagger delay for element at (col, row) in a grid
IM_ANIM_API float iam_stagger_grid_delay(int col, int row, iam_stagger_grid_opts const& opts);

// Get stagger delay for a linear index in the grid (index = row * cols + col)
IM_ANIM_API float iam_stagger_grid_delay_index(int index, iam_stagger_grid_opts const& opts);

// Stagger values (like anime.js stagger(range)): spread [start, end] across the grid using the same distance
// (from/axis/ease/reversed) as the delays; jitter adds a random +/- amount (deterministic per index and opts.seed).
// For a list of N items use cols = N, rows = 1.
IM_ANIM_API float iam_stagger_value(int col, int row, float start, float end, iam_stagger_grid_opts const& opts, float jitter = 0.0f);
IM_ANIM_API float iam_stagger_value_index(int index, float start, float end, iam_stagger_grid_opts const& opts, float jitter = 0.0f);

// Play a clip instance with a specific delay (useful with grid stagger)
IM_ANIM_API iam_instance iam_play_with_delay(ImGuiID clip_id, ImGuiID instance_id, float delay);

// Layering support - blend multiple animation instances
IM_ANIM_API void iam_layer_begin(ImGuiID instance_id);                                      // Start blending into target instance.
IM_ANIM_API void iam_layer_add(iam_instance inst, float weight);                            // Add source instance with weight.
IM_ANIM_API void iam_layer_end(ImGuiID instance_id);                                        // Finalize blending and normalize weights.
IM_ANIM_API bool iam_get_blended_float(ImGuiID instance_id, ImGuiID channel, float* out);   // Get blended float value.
IM_ANIM_API bool iam_get_blended_vec2(ImGuiID instance_id, ImGuiID channel, iam_vec2* out);   // Get blended vec2 value.
IM_ANIM_API bool iam_get_blended_vec4(ImGuiID instance_id, ImGuiID channel, iam_vec4* out);   // Get blended vec4 value.
IM_ANIM_API bool iam_get_blended_int(ImGuiID instance_id, ImGuiID channel, int* out);       // Get blended int value.

// State-driven clip blending (a state machine without the graph): call iam_state_play() every frame with the
// clip of the current state. Changing state crossfades from the previous clip(s) to the new one over crossfade
// seconds; the new clip plays from its start (coming back to a state that is still fading out keeps it playing).
// Read the blended result with iam_state_get_*. Machines unused for a while are collected by iam_gc().
IM_ANIM_API void         iam_state_play(ImGuiID machine_id, ImGuiID state_clip_id, float crossfade = 0.25f);
IM_ANIM_API ImGuiID      iam_state_current(ImGuiID machine_id);                        // Current state's clip (0 if none).
IM_ANIM_API float        iam_state_blend(ImGuiID machine_id);                          // Weight of the current state [0,1] (1 when the crossfade is done).
IM_ANIM_API iam_instance iam_state_instance(ImGuiID machine_id, ImGuiID state_clip_id); // Instance playing a state's clip (to pause, seek, set time scale...).
IM_ANIM_API bool         iam_state_get_float(ImGuiID machine_id, ImGuiID channel, float* out);
IM_ANIM_API bool         iam_state_get_int(ImGuiID machine_id, ImGuiID channel, int* out);
IM_ANIM_API bool         iam_state_get_vec2(ImGuiID machine_id, ImGuiID channel, iam_vec2* out);
IM_ANIM_API bool         iam_state_get_vec4(ImGuiID machine_id, ImGuiID channel, iam_vec4* out);
IM_ANIM_API bool         iam_state_get_color(ImGuiID machine_id, ImGuiID channel, iam_vec4* out, int color_space = iam_col_oklab);

// Persistence (optional)
IM_ANIM_API iam_result iam_clip_save(ImGuiID clip_id, char const* path);
IM_ANIM_API iam_result iam_clip_load(char const* path, ImGuiID* out_clip_id);

// ----------------------------------------------------
// Usage notes (summary)
// ----------------------------------------------------
// TWEEN API:
// 1) Call iam_update_begin_frame() once per frame; use ImGui::GetIO().DeltaTime as dt.
// 2) For each widget/object, pick a stable ImGuiID (e.g. ImGui::GetItemID()) and a channel_id (ImHashStr("alpha")).
// 3) Call iam_tween_* to get the animated value, then apply it (PushStyleVar, etc).
// 4) Optionally call iam_gc(600) every second to bound memory.
// 5) Use iam_tween_vec2_rel / iam_tween_vec2_resolved / iam_rebase_vec2 to keep animations smooth on window/dock/viewport changes.
//
// CLIP API:
// 1) Author clips once at startup using iam_clip::begin(id).key_*(...).end()
// 2) Call iam_clip_update(dt) each frame after iam_update_begin_frame()
// 3) Use iam_play(clip_id, instance_id) to start playback; returns iam_instance for queries
// 4) Call inst.get_float/vec2/vec4/int() to sample animated values
// 5) Optionally call iam_clip_gc(600) to bound instance memory
