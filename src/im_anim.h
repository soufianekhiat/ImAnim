// im_anim.h — Dear ImGui animation helpers.
// Author: Soufiane KHIAT
// License: MIT
//
// - Channels: float, vec2, vec4, int, color (sRGB/Linear/HSV/OKLAB/OKLCH blending).
// - Easing: presets + cubicBezier/steps/back/elastic/bounce/spring.
// - Caching: ImPool + ImGuiStorage keyed by (ImGuiID, channel_id) via ImHashData.
// - Resize helpers: relative targets, resolver callback, explicit rebase.
// Style: snake_case, tabs, east const.

#pragma once
#include "imgui.h"
#include <stdint.h>
#include <math.h>

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
	iam_ease_spring         // p0=mass p1=stiffness p2=damping p3=v0
};

enum iam_policy {
	iam_policy_crossfade = 0,	// smooth into new target
	iam_policy_cut,				// snap to target
	iam_policy_queue			// queue one pending target
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

// ----------------------------------------------------
// Public API declarations
// ----------------------------------------------------

// Frame management
void iam_update_begin_frame();                                                      // Call once per frame before any tweens.
void iam_gc(unsigned int max_age_frames = 600);                                     // Remove stale tween entries older than max_age_frames.
void iam_reserve(int cap_float, int cap_vec2, int cap_vec4, int cap_int, int cap_color); // Pre-allocate pool capacity.
void iam_set_ease_lut_samples(int count);                                           // Set LUT resolution for parametric easings (default: 256).

// Easing evaluation
float iam_eval_preset(int type, float t);                                           // Evaluate a preset easing function at time t (0-1).

// Tween API - smoothly interpolate values over time
float  iam_tween_float(ImGuiID id, ImGuiID channel_id, float target, float dur, iam_ease_desc const& ez, int policy, float dt);   // Animate a float value.
ImVec2 iam_tween_vec2(ImGuiID id, ImGuiID channel_id, ImVec2 target, float dur, iam_ease_desc const& ez, int policy, float dt);   // Animate a 2D vector.
ImVec4 iam_tween_vec4(ImGuiID id, ImGuiID channel_id, ImVec4 target, float dur, iam_ease_desc const& ez, int policy, float dt);   // Animate a 4D vector.
int    iam_tween_int(ImGuiID id, ImGuiID channel_id, int target, float dur, iam_ease_desc const& ez, int policy, float dt);       // Animate an integer value.
ImVec4 iam_tween_color(ImGuiID id, ImGuiID channel_id, ImVec4 target_srgb, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt); // Animate a color in specified color space.

// Resize-friendly helpers
ImVec2 iam_anchor_size(int space); // Get dimensions of anchor space (window, viewport, etc.).

// Relative target tweens (percent of anchor + pixel offset) - survive window resizes
float  iam_tween_float_rel(ImGuiID id, ImGuiID channel_id, float percent, float px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, int axis, float dt);  // Float relative to anchor (axis: 0=x, 1=y).
ImVec2 iam_tween_vec2_rel(ImGuiID id, ImGuiID channel_id, ImVec2 percent, ImVec2 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt);           // Vec2 relative to anchor.
ImVec4 iam_tween_vec4_rel(ImGuiID id, ImGuiID channel_id, ImVec4 percent, ImVec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt);           // Vec4 with x,y relative to anchor.
ImVec4 iam_tween_color_rel(ImGuiID id, ImGuiID channel_id, ImVec4 percent, ImVec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int color_space, int anchor_space, float dt); // Color with component offsets.

// Resolver callbacks for dynamic target computation
typedef float  (*iam_float_resolver)(void* user);   // Returns float target value.
typedef ImVec2 (*iam_vec2_resolver)(void* user);    // Returns vec2 target value.
typedef ImVec4 (*iam_vec4_resolver)(void* user);    // Returns vec4 target value.
typedef ImVec4 (*iam_color_resolver)(void* user);   // Returns color target (sRGB).

// Resolved tweens - target computed dynamically by callback each frame
float  iam_tween_float_resolved(ImGuiID id, ImGuiID channel_id, iam_float_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);                     // Float with dynamic target.
ImVec2 iam_tween_vec2_resolved(ImGuiID id, ImGuiID channel_id, iam_vec2_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);                       // Vec2 with dynamic target.
ImVec4 iam_tween_vec4_resolved(ImGuiID id, ImGuiID channel_id, iam_vec4_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);                       // Vec4 with dynamic target.
ImVec4 iam_tween_color_resolved(ImGuiID id, ImGuiID channel_id, iam_color_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt);    // Color with dynamic target.

// Rebase functions - change target of in-progress animation without restarting
void iam_rebase_float(ImGuiID id, ImGuiID channel_id, float new_target, float dt);  // Smoothly redirect float animation to new target.
void iam_rebase_vec2(ImGuiID id, ImGuiID channel_id, ImVec2 new_target, float dt);  // Smoothly redirect vec2 animation to new target.
void iam_rebase_vec4(ImGuiID id, ImGuiID channel_id, ImVec4 new_target, float dt);  // Smoothly redirect vec4 animation to new target.
void iam_rebase_color(ImGuiID id, ImGuiID channel_id, ImVec4 new_target, float dt); // Smoothly redirect color animation to new target.

// ----------------------------------------------------
// Convenience shorthands for common easings
// ----------------------------------------------------
inline iam_ease_desc iam_ease_preset(int type) { iam_ease_desc e = { type, 0,0,0,0 }; return e; }                                               // Create descriptor from preset enum.
inline iam_ease_desc iam_ease_bezier(float x1, float y1, float x2, float y2) { iam_ease_desc e = { iam_ease_cubic_bezier, x1,y1,x2,y2 }; return e; } // Create cubic bezier easing.
inline iam_ease_desc iam_ease_steps_desc(int steps, int mode) { iam_ease_desc e = { iam_ease_steps, (float)steps,(float)mode,0,0 }; return e; }     // Create step function easing.
inline iam_ease_desc iam_ease_back(float overshoot) { iam_ease_desc e = { iam_ease_out_back, overshoot,0,0,0 }; return e; }                         // Create back easing with overshoot.
inline iam_ease_desc iam_ease_elastic(float amplitude, float period) { iam_ease_desc e = { iam_ease_out_elastic, amplitude, period,0,0 }; return e; } // Create elastic easing.
inline iam_ease_desc iam_ease_spring_desc(float mass, float stiffness, float damping, float v0) { iam_ease_desc e = { iam_ease_spring, mass, stiffness, damping, v0 }; return e; } // Create physics spring.

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
	iam_chan_int
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

// Forward declarations
struct iam_clip_data;
struct iam_instance_data;

// Callback types (use instance ID instead of pointer for safety)
typedef void (*iam_clip_callback)(ImGuiID inst_id, void* user_data);

// ----------------------------------------------------
// iam_clip - fluent API for authoring animations
// ----------------------------------------------------
class iam_clip {
public:
	// Start building a new clip with the given ID
	static iam_clip begin(ImGuiID clip_id);

	// Add keyframes for different channel types
	iam_clip& key_float(ImGuiID channel, float time, float value, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec2(ImGuiID channel, float time, ImVec2 value, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_vec4(ImGuiID channel, float time, ImVec4 value, int ease_type = iam_ease_linear, float const* bezier4 = nullptr);
	iam_clip& key_int(ImGuiID channel, float time, int value, int ease_type = iam_ease_linear);

	// Spring-based keyframe (float only)
	iam_clip& key_float_spring(ImGuiID channel, float time, float target, iam_spring_params const& spring);

	// Timeline grouping (for future expansion)
	iam_clip& seq_begin();
	iam_clip& seq_end();
	iam_clip& par_begin();
	iam_clip& par_end();

	// Clip options
	iam_clip& set_loop(bool loop, int direction = iam_dir_normal, int loop_count = -1);
	iam_clip& set_delay(float delay_seconds);
	iam_clip& set_stagger(int count, float each_delay, float from_center_bias = 0.0f);

	// Callbacks
	iam_clip& on_begin(iam_clip_callback cb, void* user = nullptr);
	iam_clip& on_update(iam_clip_callback cb, void* user = nullptr);
	iam_clip& on_complete(iam_clip_callback cb, void* user = nullptr);

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
class iam_instance {
public:
	iam_instance() : m_inst_id(0) {}
	iam_instance(ImGuiID inst_id) : m_inst_id(inst_id) {}

	// Playback control
	void pause();
	void resume();
	void stop();
	void destroy();  // Remove instance from system (valid() will return false after this)
	void seek(float time);
	void set_time_scale(float scale);
	void set_weight(float weight);  // for layering/blending

	// Query state
	float time() const;
	float duration() const;
	bool is_playing() const;
	bool is_paused() const;

	// Get animated values
	bool get_float(ImGuiID channel, float* out) const;
	bool get_vec2(ImGuiID channel, ImVec2* out) const;
	bool get_vec4(ImGuiID channel, ImVec4* out) const;
	bool get_int(ImGuiID channel, int* out) const;

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
void iam_clip_init(int initial_clip_cap = 256, int initial_inst_cap = 4096);
void iam_clip_shutdown();

// Per-frame update (call after iam_update_begin_frame)
void iam_clip_update(float dt);

// Garbage collection for instances
void iam_clip_gc(unsigned int max_age_frames = 600);

// Play a clip on an instance (creates or reuses instance)
iam_instance iam_play(ImGuiID clip_id, ImGuiID instance_id);

// Get an existing instance (returns invalid iam_instance if not found)
iam_instance iam_get_instance(ImGuiID instance_id);

// Query clip info
float iam_clip_duration(ImGuiID clip_id);                                       // Get clip duration in seconds.
bool iam_clip_exists(ImGuiID clip_id);                                          // Check if clip exists.

// Stagger helpers - compute delay for indexed instances
float iam_stagger_delay(ImGuiID clip_id, int index);                            // Get stagger delay for element at index.
iam_instance iam_play_stagger(ImGuiID clip_id, ImGuiID instance_id, int index); // Play with stagger delay applied.

// Layering support - blend multiple animation instances
void iam_layer_begin(ImGuiID instance_id);                                      // Start blending into target instance.
void iam_layer_add(iam_instance inst, float weight);                            // Add source instance with weight.
void iam_layer_end(ImGuiID instance_id);                                        // Finalize blending and normalize weights.
bool iam_get_blended_float(ImGuiID instance_id, ImGuiID channel, float* out);   // Get blended float value.
bool iam_get_blended_vec2(ImGuiID instance_id, ImGuiID channel, ImVec2* out);   // Get blended vec2 value.
bool iam_get_blended_vec4(ImGuiID instance_id, ImGuiID channel, ImVec4* out);   // Get blended vec4 value.
bool iam_get_blended_int(ImGuiID instance_id, ImGuiID channel, int* out);       // Get blended int value.

// Persistence (optional)
iam_result iam_clip_save(ImGuiID clip_id, char const* path);
iam_result iam_clip_load(char const* path, ImGuiID* out_clip_id);

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
