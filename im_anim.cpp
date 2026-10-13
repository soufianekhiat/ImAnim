// im_anim.cpp — Dear ImGui animation helpers implementation.
// Author: Soufiane KHIAT
// License: MIT

#include "im_anim.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef IM_ANIM_PRE_19200_COMPATIBILITY
	// ImGuiStoragePair is nested in ImGuiStorage in Pre-1.92.0 versions
#define IMGUI_STORAGE_PAIR ImGuiStorage::ImGuiStoragePair
typedef ImFont FontType;
#else
	// ImGuiStoragePair is in the global namespace since ImGui version 1.92.0 (19200)
#define IMGUI_STORAGE_PAIR ImGuiStoragePair
typedef ImFontBaked FontType;
#endif

static FontType* GetBakedFont(ImFont* font, float font_size) {
#ifdef IM_ANIM_PRE_19200_COMPATIBILITY
	(void)font_size; // suppress unused warning
	return font;
#else
	return font->GetFontBaked(font_size);
#endif
}

// Internal ImVec2/ImVec4 implementations of the public API (see public wrappers at the end of this file).
static iam_drag_feedback iam_drag_begin_impl(ImGuiID id, ImVec2 pos);
static iam_drag_feedback iam_drag_update_impl(ImGuiID id, ImVec2 pos, float dt);
static iam_drag_feedback iam_drag_release_impl(ImGuiID id, ImVec2 pos, iam_drag_opts const& opts, float dt);
static ImVec2 iam_oscillate_vec2_impl(ImGuiID id, ImVec2 amplitude, ImVec2 frequency, int wave_type, ImVec2 phase, float dt);
static ImVec4 iam_oscillate_vec4_impl(ImGuiID id, ImVec4 amplitude, ImVec4 frequency, int wave_type, ImVec4 phase, float dt);
static ImVec4 iam_oscillate_color_impl(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float frequency, int wave_type, float phase, int color_space, float dt);
static ImVec2 iam_shake_vec2_impl(ImGuiID id, ImVec2 intensity, float frequency, float decay_time, float dt);
static ImVec4 iam_shake_vec4_impl(ImGuiID id, ImVec4 intensity, float frequency, float decay_time, float dt);
static ImVec4 iam_shake_color_impl(ImGuiID id, ImVec4 base_color, ImVec4 intensity, float frequency, float decay_time, int color_space, float dt);
static ImVec2 iam_wiggle_vec2_impl(ImGuiID id, ImVec2 amplitude, float frequency, float dt);
static ImVec4 iam_wiggle_vec4_impl(ImGuiID id, ImVec4 amplitude, float frequency, float dt);
static ImVec4 iam_wiggle_color_impl(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float frequency, int color_space, float dt);
static ImVec2 iam_tween_vec2_impl(ImGuiID id, ImGuiID channel_id, ImVec2 target, float dur, iam_ease_desc const& ez, int policy, float dt, ImVec2 init_value = ImVec2(0, 0));
static ImVec4 iam_tween_vec4_impl(ImGuiID id, ImGuiID channel_id, ImVec4 target, float dur, iam_ease_desc const& ez, int policy, float dt, ImVec4 init_value = ImVec4(0, 0, 0, 0));
static ImVec4 iam_tween_color_impl(ImGuiID id, ImGuiID channel_id, ImVec4 target_srgb, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt, ImVec4 init_value = ImVec4(1, 1, 1, 1));
static ImVec2 iam_anchor_size_impl(int space);
static ImVec2 iam_tween_vec2_rel_impl(ImGuiID id, ImGuiID channel_id, ImVec2 percent, ImVec2 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt);
static ImVec4 iam_tween_vec4_rel_impl(ImGuiID id, ImGuiID channel_id, ImVec4 percent, ImVec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt);
static ImVec4 iam_tween_color_rel_impl(ImGuiID id, ImGuiID channel_id, ImVec4 percent, ImVec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int color_space, int anchor_space, float dt);
static ImVec2 iam_tween_vec2_resolved_impl(ImGuiID id, ImGuiID channel_id, iam_vec2_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);
static ImVec4 iam_tween_vec4_resolved_impl(ImGuiID id, ImGuiID channel_id, iam_vec4_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt);
static ImVec4 iam_tween_color_resolved_impl(ImGuiID id, ImGuiID channel_id, iam_color_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt);
static void iam_rebase_vec2_impl(ImGuiID id, ImGuiID channel_id, ImVec2 new_target, float dt);
static void iam_rebase_vec4_impl(ImGuiID id, ImGuiID channel_id, ImVec4 new_target, float dt);
static void iam_rebase_color_impl(ImGuiID id, ImGuiID channel_id, ImVec4 new_target, float dt);
static ImVec4 iam_get_blended_color_impl(ImVec4 a_srgb, ImVec4 b_srgb, float t, int color_space);
static ImVec2 iam_tween_vec2_per_axis_impl(ImGuiID id, ImGuiID channel_id, ImVec2 target, float dur, iam_ease_per_axis const& ez, int policy, float dt);
static ImVec4 iam_tween_vec4_per_axis_impl(ImGuiID id, ImGuiID channel_id, ImVec4 target, float dur, iam_ease_per_axis const& ez, int policy, float dt);
static ImVec4 iam_tween_color_per_axis_impl(ImGuiID id, ImGuiID channel_id, ImVec4 target_srgb, float dur, iam_ease_per_axis const& ez, int policy, int color_space, float dt);
static ImVec2 iam_bezier_quadratic_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, float t);
static ImVec2 iam_bezier_cubic_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t);
static ImVec2 iam_catmull_rom_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t, float tension = 0.5f);
static ImVec2 iam_bezier_quadratic_deriv_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, float t);
static ImVec2 iam_bezier_cubic_deriv_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t);
static ImVec2 iam_catmull_rom_deriv_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t, float tension = 0.5f);
static ImVec2 iam_path_evaluate_impl(ImGuiID path_id, float t);
static ImVec2 iam_path_tangent_impl(ImGuiID path_id, float t);
static ImVec2 iam_tween_path_impl(ImGuiID id, ImGuiID channel_id, ImGuiID path_id, float dur, iam_ease_desc const& ez, int policy, float dt);
static ImVec2 iam_path_evaluate_at_distance_impl(ImGuiID path_id, float distance);
static ImVec2 iam_path_tangent_at_distance_impl(ImGuiID path_id, float distance);
static ImVec2 iam_path_morph_impl(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts = iam_morph_opts());
static ImVec2 iam_path_morph_tangent_impl(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts = iam_morph_opts());
static ImVec2 iam_tween_path_morph_impl(ImGuiID id, ImGuiID channel_id, ImGuiID path_a, ImGuiID path_b, float target_blend, float dur, iam_ease_desc const& path_ease, iam_ease_desc const& morph_ease, int policy, float dt, iam_morph_opts const& opts = iam_morph_opts());
static void iam_transform_quad_impl(ImVec2* quad, ImVec2 center, float angle_rad, ImVec2 translation);
static void iam_make_glyph_quad_impl(ImVec2* quad, ImVec2 pos, float angle_rad, float glyph_width, float glyph_height, float baseline_offset = 0.0f);
static ImVec2 iam_noise_channel_vec2_impl(ImGuiID id, ImVec2 frequency, ImVec2 amplitude, iam_noise_opts const& opts, float dt);
static ImVec4 iam_noise_channel_vec4_impl(ImGuiID id, ImVec4 frequency, ImVec4 amplitude, iam_noise_opts const& opts, float dt);
static ImVec4 iam_noise_channel_color_impl(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float frequency, iam_noise_opts const& opts, int color_space, float dt);
static ImVec2 iam_smooth_noise_vec2_impl(ImGuiID id, ImVec2 amplitude, float speed, float dt);
static ImVec4 iam_smooth_noise_vec4_impl(ImGuiID id, ImVec4 amplitude, float speed, float dt);
static ImVec4 iam_smooth_noise_color_impl(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float speed, int color_space, float dt);
static bool iam_get_blended_vec2_impl(ImGuiID instance_id, ImGuiID channel, ImVec2* out);
static bool iam_get_blended_vec4_impl(ImGuiID instance_id, ImGuiID channel, ImVec4* out);

ImU32 const ZimaBlue = IM_COL32( 91, 194, 231, 255 );
ImU32 const AgedCopper = IM_COL32( 204, 120, 88, 255 );

// ----------------------------------------------------
// Internal: parameterized easing LUT cache (ImPool)
// ----------------------------------------------------
namespace iam_detail {

// ----------------------------------------------------
// Easing constants - named values for clarity
// ----------------------------------------------------

// Bounce easing constants (derived from Robert Penner's equations)
static float const BOUNCE_N1 = 7.5625f;       // Bounce amplitude multiplier
static float const BOUNCE_D1 = 2.75f;         // Bounce timing divisor

// Back easing constants
static float const BACK_OVERSHOOT = 1.70158f;           // Default overshoot amount (~10% overshoot)
static float const BACK_OVERSHOOT_INOUT = 1.70158f * 1.525f;  // Scaled overshoot for in-out

// Elastic easing defaults
static float const ELASTIC_AMPLITUDE = 1.0f;  // Default amplitude
static float const ELASTIC_PERIOD = 0.3f;     // Default period for in/out
static float const ELASTIC_PERIOD_INOUT = 0.45f;  // Period for in-out variant

// Spring physics defaults
static float const SPRING_MASS = 1.0f;        // Default mass
static float const SPRING_STIFFNESS = 120.0f; // Default stiffness (k)
static float const SPRING_DAMPING = 20.0f;    // Default damping (c)

// Floating point comparison epsilon
static float const EASE_EPSILON = 1e-6f;

struct ease_lut {
	iam_ease_desc		desc;
	ImVector<float>		samples;
	int					count;
	ease_lut() { count = 0; }
};

struct ease_lut_pool {
	ImPool<ease_lut>	pool;
	ImGuiStorage		map;
	int					sample_count;
	ease_lut_pool() { sample_count = 129; }

	static ImGuiID hash_desc(iam_ease_desc const& d) { return ImHashData(&d, sizeof(d)); }

	static float bounce_out(float t) {
		if (t < 1.0f / BOUNCE_D1) return BOUNCE_N1 * t * t;
		else if (t < 2.0f / BOUNCE_D1) { t -= 1.5f / BOUNCE_D1; return BOUNCE_N1 * t * t + 0.75f; }
		else if (t < 2.5f / BOUNCE_D1) { t -= 2.25f / BOUNCE_D1; return BOUNCE_N1 * t * t + 0.9375f; }
		else { t -= 2.625f / BOUNCE_D1; return BOUNCE_N1 * t * t + 0.984375f; }
	}

	static float elastic_core(float t, float a, float p) {
		if (t == 0.f || t == 1.f) return t;
		float A = (a <= 0.f ? ELASTIC_AMPLITUDE : a);
		float P = (p <= 0.f ? ELASTIC_PERIOD : p);
		float s = (P / IAM_2PI) * asinf(1.0f / A);
		return -(A * ImPow(2.f, 10.f * (t - 1.f)) * ImSin((t - 1.f - s) * IAM_2PI / P));
	}

	static float back_core(float t, float s) { return t * t * ((s + 1.f) * t - s); }

	static float cubic_bezier_y(float x, float x1, float y1, float x2, float y2) {
		float t = x;
		for (int i = 0; i < 5; ++i) {
			float mt = 1.f - t;
			float bx = 3.f*mt*mt*t*x1 + 3.f*mt*t*t*x2 + t*t*t;
			float dx = 3.f*mt*mt*x1 + 6.f*mt*t*(x2 - x1) + 3.f*t*t*(1.f - x2);
			if (dx != 0.f) t = t - (bx - x) / dx;
			if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
		}
		float mt = 1.f - t;
		float by = 3.f*mt*mt*t*y1 + 3.f*mt*t*t*y2 + t*t*t;
		return by;
	}

	static float spring_unit(float u, float mass, float k, float c, float v0) {
		float m = (mass <= 0.f ? 1.f : mass);
		float wn = ImSqrt(k / m);
		float zeta = c / (2.f * ImSqrt(k * m));
		float t = u;
		if (zeta < 1.f) {
			float wdn = wn * ImSqrt(1.f - zeta*zeta);
			float A = 1.f;
			float B = (zeta * wn * A + v0) / wdn;
			float e = expf(-zeta * wn * t);
			return 1.f - e * (A * ImCos(wdn*t) + B * ImSin(wdn*t));
		} else if (zeta == 1.f) {
			float e = expf(-wn * t);
			return 1.f - e * (1.f + wn * t);
		} else {
			float wd = wn * ImSqrt(zeta*zeta - 1.f);
			float e1 = expf(-(zeta * wn - wd) * t);
			float e2 = expf(-(zeta * wn + wd) * t);
			return 1.f - 0.5f*(e1 + e2);
		}
	}

	void build_lut(ease_lut& lut) {
		iam_ease_desc const& d = lut.desc;
		lut.samples.resize(sample_count);
		for (int i = 0; i < sample_count; ++i) {
			float x = (float)i / (float)(sample_count - 1);
			float y = x;
			switch (d.type) {
				case iam_ease_cubic_bezier:
					y = cubic_bezier_y(x, d.p0, d.p1, d.p2, d.p3);
					break;
				case iam_ease_steps: {
					int n = (int)(d.p0 < 1.f ? 1.f : d.p0);
					int mode = (int)d.p1;
					if (mode == 1)		y = ImFloor(x * n + EASE_EPSILON) / (float)n;
					else if (mode == 2)	{ y = (ImFloor(x * n - 0.5f + EASE_EPSILON) + 0.5f) / (float)n; if (y<0) y=0; if (y>1) y=1; }
					else				y = ImFloor(x * n + EASE_EPSILON) / (float)n;
					break;
				}
				case iam_ease_in_elastic: {
					float a = (d.p0 <= 0.f ? ELASTIC_AMPLITUDE : d.p0), p = (d.p1 <= 0.f ? ELASTIC_PERIOD : d.p1);
					y = 1.f + elastic_core(1.f - x, a, p);
					break;
				}
				case iam_ease_out_elastic: {
					float a = (d.p0 <= 0.f ? ELASTIC_AMPLITUDE : d.p0), p = (d.p1 <= 0.f ? ELASTIC_PERIOD : d.p1);
					y = 1.f - elastic_core(x, a, p);
					break;
				}
				case iam_ease_in_out_elastic: {
					float a = (d.p0 <= 0.f ? ELASTIC_AMPLITUDE : d.p0), p = (d.p1 <= 0.f ? ELASTIC_PERIOD_INOUT : d.p1);
					if (x < 0.5f) y = 0.5f * (1.f + elastic_core(1.f - 2.f*x, a, p));
					else y = 0.5f * (1.f - elastic_core(2.f*x - 1.f, a, p)) + 0.5f;
					break;
				}
				case iam_ease_in_back: {
					float s = (d.p0 == 0.f ? BACK_OVERSHOOT : d.p0);
					y = back_core(x, s);
					break;
				}
				case iam_ease_out_back: {
					float s = (d.p0 == 0.f ? BACK_OVERSHOOT : d.p0);
					y = 1.f - back_core(1.f - x, s);
					break;
				}
				case iam_ease_in_out_back: {
					float s = (d.p0 == 0.f ? BACK_OVERSHOOT_INOUT : d.p0);
					if (x < 0.5f) y = 0.5f * back_core(2.f*x, s);
					else y = 1.f - 0.5f * back_core(2.f*(1.f - x), s);
					break;
				}
				case iam_ease_in_bounce:
					y = 1.f - bounce_out(1.f - x);
					break;
				case iam_ease_out_bounce:
					y = bounce_out(x);
					break;
				case iam_ease_in_out_bounce:
					y = (x < 0.5f) ? (0.5f * (1.f - bounce_out(1.f - 2.f*x))) : (0.5f * bounce_out(2.f*x - 1.f) + 0.5f);
					break;
				case iam_ease_spring:
					y = spring_unit(x, (d.p0<=0.f ? SPRING_MASS : d.p0), (d.p1<=0.f ? SPRING_STIFFNESS : d.p1), (d.p2<=0.f ? SPRING_DAMPING : d.p2), d.p3);
					break;
				default: y = x; break;
			}
			//if (y < 0.f) y = 0.f; if (y > 1.f) y = 1.f;
			lut.samples[i] = y;
		}
	}

	float eval_lut(iam_ease_desc const& d, float t) {
		if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
		ImGuiID key = hash_desc(d);
		int idx = map.GetInt(key, -1);
		ease_lut* lut;
		if (idx == -1) {
			lut = pool.GetOrAddByKey(key);
			lut->desc = d;
			build_lut(*lut);
			map.SetInt(key, pool.GetIndex(lut));
		} else {
			lut = pool.GetByIndex(idx);
		}
		float fi = t * (float)(sample_count - 1);
		int i0 = (int)fi;
		int i1 = i0 + 1;
		if (i1 >= sample_count) i1 = sample_count - 1;
		float frac = fi - (float)i0;
		return lut->samples[i0] + (lut->samples[i1] - lut->samples[i0]) * frac;
	}
};

static ease_lut_pool& ease_lut_pool_singleton() { static ease_lut_pool S; return S; }

// Forward declaration for custom easing (defined later with other globals)
static iam_ease_fn g_custom_ease[16];

// ----------------------------------------------------
// Easing implementation - base functions + transforms
// ----------------------------------------------------

// Easing families (groups of in/out/in_out)
enum ease_family { ease_quad, ease_cubic, ease_quart, ease_quint, ease_sine, ease_expo, ease_circ, ease_back, ease_elastic, ease_bounce };
enum ease_variant { ease_in, ease_out, ease_in_out };

// Derived constants for standalone easing functions
static float const EASE_PI = 3.1415926535f;
static float const BACK_C1 = BACK_OVERSHOOT;  // Alias for back easing formula
static float const BACK_C3 = BACK_C1 + 1.f;   // Back easing cubic coefficient
static float const ELASTIC_C4 = (2.f * EASE_PI) / 3.f;  // Elastic angular frequency

// Base "in" easing functions - all others derived via transforms
static float ease_in_quad(float t)    { return t * t; }
static float ease_in_cubic(float t)   { return t * t * t; }
static float ease_in_quart(float t)   { return t * t * t * t; }
static float ease_in_quint(float t)   { return t * t * t * t * t; }
static float ease_in_sine(float t)    { return 1.f - ImCos((t * EASE_PI) / 2.f); }
static float ease_in_expo(float t)    { return (t == 0.f) ? 0.f : ImPow(2.f, 10.f * t - 10.f); }
static float ease_in_circ(float t)    { return 1.f - ImSqrt(1.f - t * t); }
static float ease_in_back(float t)    { return BACK_C3 * t * t * t - BACK_C1 * t * t; }
static float ease_in_elastic(float t) { return (t == 0.f || t == 1.f) ? t : -ImPow(2.f, 10.f * t - 10.f) * ImSin((t * 10.f - 10.75f) * ELASTIC_C4); }

// Bounce is naturally defined as "out" - special case
static float ease_out_bounce(float t) {
	if (t < 1.f / BOUNCE_D1)     return BOUNCE_N1 * t * t;
	if (t < 2.f / BOUNCE_D1)     { t -= 1.5f / BOUNCE_D1;   return BOUNCE_N1 * t * t + 0.75f; }
	if (t < 2.5f / BOUNCE_D1)    { t -= 2.25f / BOUNCE_D1;  return BOUNCE_N1 * t * t + 0.9375f; }
	t -= 2.625f / BOUNCE_D1; return BOUNCE_N1 * t * t + 0.984375f;
}

// Evaluate base "in" function by family
static float eval_ease_in(int family, float t) {
	switch (family) {
		case ease_quad:    return ease_in_quad(t);
		case ease_cubic:   return ease_in_cubic(t);
		case ease_quart:   return ease_in_quart(t);
		case ease_quint:   return ease_in_quint(t);
		case ease_sine:    return ease_in_sine(t);
		case ease_expo:    return ease_in_expo(t);
		case ease_circ:    return ease_in_circ(t);
		case ease_back:    return ease_in_back(t);
		case ease_elastic: return ease_in_elastic(t);
		case ease_bounce:  return 1.f - ease_out_bounce(1.f - t);  // bounce defined as out
		default:           return t;
	}
}

// Transform: in -> out  =>  out(t) = 1 - in(1 - t)
static float transform_out(int family, float t) {
	return 1.f - eval_ease_in(family, 1.f - t);
}

// Transform: in -> in_out  =>  in_out(t) = t < 0.5 ? in(2t)/2 : 1 - in(2-2t)/2
static float transform_in_out(int family, float t) {
	return (t < 0.5f)
		? eval_ease_in(family, 2.f * t) / 2.f
		: 1.f - eval_ease_in(family, 2.f - 2.f * t) / 2.f;
}

static float eval_preset_internal(int type, float t) {
	t = ImClamp(t, 0.f, 1.f);
	if (type == iam_ease_linear) return t;

	// Decompose type into family and variant
	// Enum layout: linear=0, then groups of 3 (in, out, in_out) for each family
	int idx = type - 1;  // 0-based index after linear
	int family = idx / 3;
	int variant = idx % 3;

	switch (variant) {
		case ease_in:     return eval_ease_in(family, t);
		case ease_out:    return transform_out(family, t);
		case ease_in_out: return transform_in_out(family, t);
		default:          return t;
	}
}

// ----------------------------------------------------
// Custom curve eases (iam_ease_curve)
// ----------------------------------------------------
struct ease_curve {
	ImGuiID id;
	ImVector<ImVec2> pts;   // P0, then (C1, C2, P1) per segment
	float lut[129];         // y sampled at uniform x in [0,1]
};
static ImGuiStorage g_curve_map;  // id -> ease_curve* (heap, never moves)

static ease_curve* find_curve(ImGuiID id) { return (ease_curve*)g_curve_map.GetVoidPtr(id); }
static ImGuiID curve_id_from(float lo, float hi) { return ((ImGuiID)(hi < 0.0f ? 0.0f : hi) << 16) | ((ImGuiID)(lo < 0.0f ? 0.0f : lo) & 0xFFFF); }

static ImVec2 bezier_point(ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d, float u) {
	float v = 1.0f - u;
	float w0 = v * v * v, w1 = 3 * v * v * u, w2 = 3 * v * u * u, w3 = u * u * u;
	return ImVec2(a.x * w0 + b.x * w1 + c.x * w2 + d.x * w3, a.y * w0 + b.y * w1 + c.y * w2 + d.y * w3);
}

static void build_curve_lut(ease_curve* c) {
	int const segs = (c->pts.Size - 1) / 3;
	int const per = 48;
	ImVector<ImVec2> s;
	s.reserve(segs * per + 1);
	for (int k = 0; k < segs; k++) {
		ImVec2 const* p = &c->pts[k * 3];
		for (int i = 0; i < per; i++) s.push_back(bezier_point(p[0], p[1], p[2], p[3], (float)i / per));
	}
	s.push_back(c->pts.back());
	for (int i = 1; i < s.Size; i++) if (s[i].x < s[i - 1].x) s[i].x = s[i - 1].x;  // keep x monotonic
	int j = 0;
	for (int i = 0; i <= 128; i++) {
		float x = (float)i / 128.0f;
		while (j < s.Size - 2 && s[j + 1].x < x) j++;
		ImVec2 a = s[j], b = s[j + 1];
		float f = (b.x - a.x) > 1e-7f ? ImClamp((x - a.x) / (b.x - a.x), 0.0f, 1.0f) : 1.0f;
		c->lut[i] = a.y + (b.y - a.y) * f;
	}
}

static float curve_eval(ImGuiID id, float t) {
	ease_curve* c = find_curve(id);
	if (!c) return t;  // undefined curve: linear
	t = ImClamp(t, 0.0f, 1.0f);
	float f = t * 128.0f;
	int i = (int)f;
	if (i >= 128) return c->lut[128];
	return c->lut[i] + (c->lut[i + 1] - c->lut[i]) * (f - (float)i);
}

static float eval(iam_ease_desc const& d, float t) {
	switch (d.type) {
		case iam_ease_linear:
		case iam_ease_in_quad:    case iam_ease_out_quad:    case iam_ease_in_out_quad:
		case iam_ease_in_cubic:   case iam_ease_out_cubic:   case iam_ease_in_out_cubic:
		case iam_ease_in_quart:   case iam_ease_out_quart:   case iam_ease_in_out_quart:
		case iam_ease_in_quint:   case iam_ease_out_quint:   case iam_ease_in_out_quint:
		case iam_ease_in_sine:    case iam_ease_out_sine:    case iam_ease_in_out_sine:
		case iam_ease_in_expo:    case iam_ease_out_expo:    case iam_ease_in_out_expo:
		case iam_ease_in_circ:    case iam_ease_out_circ:    case iam_ease_in_out_circ:
		case iam_ease_in_back:    case iam_ease_out_back:    case iam_ease_in_out_back:
		case iam_ease_in_elastic: case iam_ease_out_elastic: case iam_ease_in_out_elastic:
		case iam_ease_in_bounce:  case iam_ease_out_bounce:  case iam_ease_in_out_bounce:
			return eval_preset_internal(d.type, t);
		case iam_ease_custom: {
			int slot = (int)d.p0;
			if (slot >= 0 && slot < 16 && g_custom_ease[slot]) {
				if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
				return g_custom_ease[slot](t);
			}
			return t; // fallback to linear if no callback registered
		}
		case iam_ease_curve:
			return curve_eval(curve_id_from(d.p0, d.p1), t);
		default:
			return ease_lut_pool_singleton().eval_lut(d, t);
	}
}

// ----------------------------------------------------
// Color conversions & blending spaces
// ----------------------------------------------------
namespace color {

static float srgb_to_linear1(float c) { return (c <= 0.04045f) ? (c / 12.92f) : ImPow((c + 0.055f) / 1.055f, 2.4f); }
static float linear_to_srgb1(float c) { return (c <= 0.0031308f) ? (12.92f * c) : (1.055f * ImPow(c, 1.f/2.4f) - 0.055f); }

static ImVec4 srgb_to_linear(ImVec4 c) { return ImVec4(srgb_to_linear1(c.x), srgb_to_linear1(c.y), srgb_to_linear1(c.z), c.w); }
static ImVec4 linear_to_srgb(ImVec4 c) { return ImVec4(linear_to_srgb1(c.x), linear_to_srgb1(c.y), linear_to_srgb1(c.z), c.w); }

static ImVec4 hsv_to_srgb(ImVec4 hsv) {
	float H=hsv.x, S=hsv.y, V=hsv.z, A=hsv.w;
	if (S <= 0.f) return ImVec4(V,V,V,A);
	H = ImFmod(H, 1.f); if (H < 0.f) H += 1.f;
	float h = H * 6.f;
	int   i = (int)ImFloor(h);
	float f = h - (float)i;
	float p = V * (1.f - S);
	float q = V * (1.f - S * f);
	float t = V * (1.f - S * (1.f - f));
	float r,g,b;
	switch (i % 6) {
		case 0: r=V; g=t; b=p; break;
		case 1: r=q; g=V; b=p; break;
		case 2: r=p; g=V; b=t; break;
		case 3: r=p; g=q; b=V; break;
		case 4: r=t; g=p; b=V; break;
		default:r=V; g=p; b=q; break;
	}
	return ImVec4(r,g,b,A);
}

static ImVec4 srgb_to_hsv(ImVec4 c) {
	float r=c.x, g=c.y, b=c.z, a=c.w;
	float mx = r > g ? r : g; if (b > mx) mx = b;
	float mn = r < g ? r : g; if (b < mn) mn = b;
	float d = mx - mn;
	float h = 0.f, s = (mx == 0.f) ? 0.f : d / mx, v = mx;
	if (d != 0.f) {
		if (mx == r) h = ImFmod((g - b) / d, 6.f);
		else if (mx == g) h = (b - r) / d + 2.f;
		else h = (r - g) / d + 4.f;
		h /= 6.f; if (h < 0.f) h += 1.f;
	}
	return ImVec4(h,s,v,a);
}

static ImVec4 srgb_to_oklab(ImVec4 c) {
	ImVec4 l = srgb_to_linear(c);
	float lR=l.x, lG=l.y, lB=l.z;
	float l_ = 0.4122214708f*lR + 0.5363325363f*lG + 0.0514459929f*lB;
	float m_ = 0.2119034982f*lR + 0.6806995451f*lG + 0.1073969566f*lB;
	float s_ = 0.0883024619f*lR + 0.2817188376f*lG + 0.6299787005f*lB;
	float l_c = cbrtf(l_), m_c = cbrtf(m_), s_c = cbrtf(s_);
	float L = 0.2104542553f*l_c + 0.7936177850f*m_c - 0.0040720468f*s_c;
	float A = 1.9779984951f*l_c - 2.4285922050f*m_c + 0.4505937099f*s_c;
	float B = 0.0259040371f*l_c + 0.7827717662f*m_c - 0.8086757660f*s_c;
	return ImVec4(L,A,B,c.w);
}

static ImVec4 oklab_to_srgb(ImVec4 L) {
	float l = L.x, a = L.y, b = L.z;
	float l_c = l + 0.3963377774f*a + 0.2158037573f*b;
	float m_c = l - 0.1055613458f*a - 0.0638541728f*b;
	float s_c = l - 0.0894841775f*a - 1.2914855480f*b;
	float l3 = l_c*l_c*l_c, m3 = m_c*m_c*m_c, s3 = s_c*s_c*s_c;
	float R = +4.0767416621f*l3 - 3.3077115913f*m3 + 0.2309699292f*s3;
	float G = -1.2684380046f*l3 + 2.6097574011f*m3 - 0.3413193965f*s3;
	float B = -0.0041960863f*l3 - 0.7034186147f*m3 + 1.7076147010f*s3;
	// Clamp to valid linear sRGB range to avoid NaN from ImPow with negative values
	R = ImClamp(R, 0.0f, 1.0f);
	G = ImClamp(G, 0.0f, 1.0f);
	B = ImClamp(B, 0.0f, 1.0f);
	return linear_to_srgb(ImVec4(R,G,B,L.w));
}

// OKLCH: cylindrical form of OKLAB (L=lightness, C=chroma, H=hue)
static ImVec4 oklab_to_oklch(ImVec4 lab) {
	float L = lab.x, a = lab.y, b = lab.z;
	float C = ImSqrt(a*a + b*b);
	float H = atan2f(b, a) / (2.0f * 3.1415926535f);  // normalize to 0-1
	if (H < 0.0f) H += 1.0f;
	return ImVec4(L, C, H, lab.w);
}

static ImVec4 oklch_to_oklab(ImVec4 lch) {
	float L = lch.x, C = lch.y, H = lch.z;
	float h_rad = H * 2.0f * 3.1415926535f;
	float a = C * ImCos(h_rad);
	float b = C * ImSin(h_rad);
	return ImVec4(L, a, b, lch.w);
}

static ImVec4 srgb_to_oklch(ImVec4 c) { return oklab_to_oklch(srgb_to_oklab(c)); }
static ImVec4 oklch_to_srgb(ImVec4 c) { return oklab_to_srgb(oklch_to_oklab(c)); }

// Convert sRGB to a target color space
static ImVec4 to_space(ImVec4 srgb, int space) {
	switch (space) {
		case iam_col_srgb_linear: return srgb_to_linear(srgb);
		case iam_col_hsv:         return srgb_to_hsv(srgb);
		case iam_col_oklab:       return srgb_to_oklab(srgb);
		case iam_col_oklch:       return srgb_to_oklch(srgb);
		default:                  return srgb;  // iam_col_srgb
	}
}

// Convert from a color space back to sRGB
static ImVec4 from_space(ImVec4 c, int space) {
	switch (space) {
		case iam_col_srgb_linear: return linear_to_srgb(c);
		case iam_col_hsv:         return hsv_to_srgb(c);
		case iam_col_oklab:       return oklab_to_srgb(c);
		case iam_col_oklch:       return oklch_to_srgb(c);
		default:                  return c;  // iam_col_srgb
	}
}

static float lerp1(float a, float b, float t) { return a + (b - a) * t; }
static ImVec4 lerp4(ImVec4 a, ImVec4 b, float t) { return ImVec4(lerp1(a.x,b.x,t), lerp1(a.y,b.y,t), lerp1(a.z,b.z,t), lerp1(a.w,b.w,t)); }

static ImVec4 lerp_color(ImVec4 a_srgb, ImVec4 b_srgb, float t, int space) {
	switch (space) {
		case iam_col_srgb_linear: {
			ImVec4 A = srgb_to_linear(a_srgb), B = srgb_to_linear(b_srgb);
			return linear_to_srgb(lerp4(A,B,t));
		}
		case iam_col_hsv: {
			ImVec4 A = srgb_to_hsv(a_srgb), B = srgb_to_hsv(b_srgb);
			float dh = ImFmod(B.x - A.x + 1.5f, 1.f) - 0.5f;
			ImVec4 H = ImVec4(A.x + dh * t, lerp1(A.y,B.y,t), lerp1(A.z,B.z,t), lerp1(A.w,B.w,t));
			if (H.x < 0.f) H.x += 1.f; if (H.x > 1.f) H.x -= 1.f;
			return hsv_to_srgb(H);
		}
		case iam_col_oklab: {
			ImVec4 A = srgb_to_oklab(a_srgb), B = srgb_to_oklab(b_srgb);
			return oklab_to_srgb(lerp4(A,B,t));
		}
		case iam_col_oklch: {
			ImVec4 A = srgb_to_oklch(a_srgb), B = srgb_to_oklch(b_srgb);
			// L,C interpolate linearly; H uses shortest arc
			float dh = ImFmod(B.z - A.z + 1.5f, 1.f) - 0.5f;
			ImVec4 LCH = ImVec4(lerp1(A.x,B.x,t), lerp1(A.y,B.y,t), A.z + dh * t, lerp1(A.w,B.w,t));
			if (LCH.z < 0.f) LCH.z += 1.f; if (LCH.z > 1.f) LCH.z -= 1.f;
			return oklch_to_srgb(LCH);
		}
		default:
			return lerp4(a_srgb, b_srgb, t);
	}
}

} // namespace color

// ----------------------------------------------------
// Channel state (per key) and pools (ImPool)
// ----------------------------------------------------

static ImGuiID make_key(ImGuiID id, ImGuiID ch) {
	struct { ImGuiID a, b; } k = { id, ch };
	return ImHashData(&k, sizeof(k));
}

// Forward declare global time for channels to use
static double g_global_time = 0.0;

// Minimum duration to avoid division by zero
static float const MIN_DURATION = 1e-6f;

// Reduced motion (iam_set_reduced_motion)
static bool g_reduced_motion = false;
static float g_reduced_procedural_scale = 0.0f;
static float motion_dur(float d) { return g_reduced_motion ? MIN_DURATION : d; }
static float procedural_scale() { return g_reduced_motion ? g_reduced_procedural_scale : 1.0f; }
static int g_procedural_depth = 0;  // procedural functions call each other: scale only the outermost call
struct procedural_guard { bool outer; procedural_guard() : outer(g_procedural_depth++ == 0) {} ~procedural_guard() { g_procedural_depth--; } };
static ImVec2 procedural_scaled(ImVec2 v) { float k = procedural_scale(); return ImVec2(v.x * k, v.y * k); }
static ImVec4 procedural_scaled(ImVec4 v) { float k = procedural_scale(); return ImVec4(v.x * k, v.y * k, v.z * k, v.w * k); }

// ----------------------------------------------------
// Channel interpolation traits - define how each type lerps
// ----------------------------------------------------
template<typename T> struct chan_traits;

template<> struct chan_traits<float> {
	static float default_value() { return 0.0f; }
	static float lerp(float a, float b, float k) { return a + (b - a) * k; }
};

template<> struct chan_traits<ImVec2> {
	static ImVec2 default_value() { return ImVec2(0, 0); }
	static ImVec2 lerp(ImVec2 a, ImVec2 b, float k) {
		return ImVec2(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k);
	}
};

template<> struct chan_traits<ImVec4> {
	static ImVec4 default_value() { return ImVec4(1, 1, 1, 1); }
	static ImVec4 lerp(ImVec4 a, ImVec4 b, float k) {
		return ImVec4(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k,
		              a.z + (b.z - a.z) * k, a.w + (b.w - a.w) * k);
	}
};

template<> struct chan_traits<int> {
	static int default_value() { return 0; }
	static int lerp(int a, int b, float k) {
		float v = (float)a + ((float)b - (float)a) * k;
		return (int)ImFloor(v + 0.5f);
	}
};

// ----------------------------------------------------
// Base channel template - shared logic for all channel types
// ----------------------------------------------------
template<typename T>
struct base_chan {
	T		current, start, target;
	float	dur, t;  // t is cached progress for backward compatibility
	double	start_time;
	iam_ease_desc ez;
	int		policy;
	unsigned last_seen_frame;
	unsigned has_pending;
	unsigned sleeping;
	T		pending_target;

	base_chan() {
		current = chan_traits<T>::default_value();
		start = current;
		target = current;
		pending_target = chan_traits<T>::default_value();
		dur = MIN_DURATION;
		t = 1.0f;
		start_time = 0;
		ez = { iam_ease_out_cubic, 0, 0, 0, 0 };
		policy = iam_policy_crossfade;
		last_seen_frame = 0;
		has_pending = 0;
		sleeping = 1;
	}

	void set(T trg, float d, iam_ease_desc const& e, int pol) {
		start = current;
		target = trg;
		d = motion_dur(d);
		dur = (d <= MIN_DURATION ? MIN_DURATION : d);
		start_time = g_global_time;
		t = 0;
		ez = e;
		policy = pol;
		sleeping = 0;
		if (g_reduced_motion) { current = target; t = 1.0f; sleeping = 1; }  // reduced motion: land immediately
	}

	float progress() {
		if (sleeping) { t = 1.0f; return 1.0f; }
		t = (float)((g_global_time - start_time) / dur);
		if (t < 0.f) t = 0.f;
		else if (t > 1.f) t = 1.f;
		return t;
	}

	T evaluate() {
		if (sleeping) return current;
		progress();
		if (t >= 1.f) { current = target; sleeping = 1; return current; }
		float k = eval(ez, t);
		current = chan_traits<T>::lerp(start, target, k);
		return current;
	}

	void tick(float) { evaluate(); }
};

// Concrete channel types using the base template
using float_chan = base_chan<float>;
using vec2_chan = base_chan<ImVec2>;
using vec4_chan = base_chan<ImVec4>;
using int_chan = base_chan<int>;

// Color channel needs special handling for color space blending
struct color_chan {
	ImVec4	current, start, target;
	float	dur, t;
	double	start_time;
	iam_ease_desc ez;
	int		policy;
	int		space;
	unsigned last_seen_frame;
	unsigned sleeping;

	color_chan() {
		current = ImVec4(1, 1, 1, 1);
		start = current;
		target = current;
		dur = MIN_DURATION;
		t = 1.0f;
		start_time = 0;
		ez = { iam_ease_out_cubic, 0, 0, 0, 0 };
		policy = iam_policy_crossfade;
		space = iam_col_srgb_linear;
		last_seen_frame = 0;
		sleeping = 1;
	}

	void set(ImVec4 trg, float d, iam_ease_desc const& e, int pol, int sp) {
		start = current;
		target = trg;
		d = motion_dur(d);
		dur = (d <= MIN_DURATION ? MIN_DURATION : d);
		start_time = g_global_time;
		t = 0;
		ez = e;
		policy = pol;
		space = sp;
		sleeping = 0;
		if (g_reduced_motion) { current = target; t = 1.0f; sleeping = 1; }  // reduced motion: land immediately
	}

	float progress() {
		if (sleeping) { t = 1.0f; return 1.0f; }
		t = (float)((g_global_time - start_time) / dur);
		if (t < 0.f) t = 0.f;
		else if (t > 1.f) t = 1.f;
		return t;
	}

	ImVec4 evaluate() {
		if (sleeping) return current;
		progress();
		if (t >= 1.f) { current = target; sleeping = 1; return current; }
		float k = eval(ez, t);
		current = color::lerp_color(start, target, k, space);
		return current;
	}

	void tick(float) { evaluate(); }
};

// Per-type pools
template<typename T>
struct pool_t {
	ImPool<T> pool;
	unsigned frame = 0;
	void begin() { ++frame; }
	T* get(ImGuiID key) { T* c = pool.GetOrAddByKey(key); c->last_seen_frame = frame; return c; }
	T* try_get(ImGuiID key) { return pool.GetByKey(key); }  // Returns nullptr if not found
	bool exists(ImGuiID key) { return pool.GetByKey(key) != nullptr; }
	void gc(unsigned max_age) {
		for (int i = 0; i < pool.GetMapSize(); ++i) {
			if (T* c = pool.TryGetMapData(i)) {
				if (frame - c->last_seen_frame > max_age) {
					ImGuiID k = pool.Map.Data[i].key;
					pool.Remove(k, pool.GetIndex(c));
				}
			}
		}
	}
};

static pool_t<float_chan> g_float;
static pool_t<vec2_chan>  g_vec2;
static pool_t<vec4_chan>  g_vec4;
static pool_t<int_chan>   g_int;
static pool_t<color_chan> g_color;

// Global time scale for slow-motion / fast-forward
static float g_time_scale = 1.0f;

// Global frame counter for oscillators and procedural animations
static unsigned g_frame = 0;

// Lazy initialization - defer channel creation until animation is needed
static bool g_lazy_init_enabled = true;

// Note: g_custom_ease is forward-declared earlier and initialized to nullptr here

// ----------------------------------------------------
// Profiler data structures
// ----------------------------------------------------
static int const PROFILER_MAX_SECTIONS = 64;
static int const PROFILER_MAX_STACK = 16;
static int const PROFILER_HISTORY_SIZE = 120; // 2 seconds at 60fps

struct profiler_section {
	char name[64];
	double start_time;
	double accumulated_time;  // Total time this frame
	int call_count;           // Number of calls this frame
	float history[PROFILER_HISTORY_SIZE];  // History for graph
	int history_idx;
	bool active;

	profiler_section() : start_time(0), accumulated_time(0), call_count(0), history_idx(0), active(false) {
		name[0] = '\0';
		for (int i = 0; i < PROFILER_HISTORY_SIZE; i++) history[i] = 0.0f;
	}
};

struct profiler_state {
	bool enabled;
	double frame_start_time;
	double frame_total_time;
	float frame_history[PROFILER_HISTORY_SIZE];
	int frame_history_idx;
	profiler_section sections[PROFILER_MAX_SECTIONS];
	int section_count;
	int stack[PROFILER_MAX_STACK];  // Stack of section indices
	int stack_depth;

	profiler_state() : enabled(false), frame_start_time(0), frame_total_time(0),
		frame_history_idx(0), section_count(0), stack_depth(0) {
		for (int i = 0; i < PROFILER_HISTORY_SIZE; i++) frame_history[i] = 0.0f;
	}

	int find_or_create_section(char const* name) {
		// Find existing section
		for (int i = 0; i < section_count; i++) {
			if (strcmp(sections[i].name, name) == 0) return i;
		}
		// Create new section
		if (section_count >= PROFILER_MAX_SECTIONS) return -1;
		int idx = section_count++;
		strncpy(sections[idx].name, name, 63);
		sections[idx].name[63] = '\0';
		sections[idx].active = true;
		return idx;
	}
};

static profiler_state g_profiler;

static double get_time_ms() {
#ifdef _WIN32
	static bool freq_initialized = false;
	static double inv_freq = 0.0;
	if (!freq_initialized) {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		inv_freq = 1000.0 / (double)freq.QuadPart;
		freq_initialized = true;
	}
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (double)counter.QuadPart * inv_freq;
#else
	return (double)ImGui::GetTime() * 1000.0;
#endif
}

} // namespace iam_detail

// Forward declaration for scroll update
static void iam_scroll_update_internal(float dt);

// ----------------------------------------------------
// Public API implementations
// ----------------------------------------------------

void iam_update_begin_frame() {
	iam_detail::g_float.begin();
	iam_detail::g_vec2.begin();
	iam_detail::g_vec4.begin();
	iam_detail::g_int.begin();
	iam_detail::g_color.begin();
	iam_detail::g_frame++;
	// Accumulate global time (scaled)
	iam_detail::g_global_time += ImGui::GetIO().DeltaTime * iam_detail::g_time_scale;
	iam_scroll_update_internal(ImGui::GetIO().DeltaTime);
}

static void iam_spring_gc(unsigned max_age);  // Spring channels section
static void iam_spring_clear();
static void iam_presence_gc(unsigned max_age);  // Presence section
static void iam_presence_clear();
static void iam_inertia_gc(unsigned max_age);  // Inertia section
static void iam_inertia_clear();
static void iam_track_velocity_gc(unsigned max_age);  // Velocity tracking section
static void iam_track_velocity_clear();
static void iam_state_gc(unsigned max_age);  // State blending section

void iam_gc(unsigned int max_age_frames) {
	iam_spring_gc(max_age_frames);
	iam_presence_gc(max_age_frames);
	iam_inertia_gc(max_age_frames);
	iam_track_velocity_gc(max_age_frames);
	iam_state_gc(max_age_frames);
	iam_detail::g_float.gc(max_age_frames);
	iam_detail::g_vec2.gc(max_age_frames);
	iam_detail::g_vec4.gc(max_age_frames);
	iam_detail::g_int.gc(max_age_frames);
	iam_detail::g_color.gc(max_age_frames);
}

void iam_pool_clear() {
	// Clean up all pools
	iam_spring_clear();
	iam_presence_clear();
	iam_inertia_clear();
	iam_track_velocity_clear();
	iam_detail::g_float.pool.Clear();
	iam_detail::g_vec2.pool.Clear();
	iam_detail::g_vec4.pool.Clear();
	iam_detail::g_int.pool.Clear();
	iam_detail::g_color.pool.Clear();
}

void iam_reserve(int cap_float, int cap_vec2, int cap_vec4, int cap_int, int cap_color) {
	if (cap_float  > 0) iam_detail::g_float.pool.Reserve(cap_float);
	if (cap_vec2   > 0) iam_detail::g_vec2.pool.Reserve(cap_vec2);
	if (cap_vec4   > 0) iam_detail::g_vec4.pool.Reserve(cap_vec4);
	if (cap_int    > 0) iam_detail::g_int.pool.Reserve(cap_int);
	if (cap_color  > 0) iam_detail::g_color.pool.Reserve(cap_color);
}

void iam_set_ease_lut_samples(int count) {
	if (count < 9) count = 9;
	iam_detail::ease_lut_pool_singleton().sample_count = count;
}

void iam_set_global_time_scale(float scale) {
	iam_detail::g_time_scale = scale > 0.0f ? scale : 0.0f;
}

float iam_get_global_time_scale() {
	return iam_detail::g_time_scale;
}

void iam_set_reduced_motion(bool enabled, float procedural_scale) {
	iam_detail::g_reduced_motion = enabled;
	iam_detail::g_reduced_procedural_scale = ImClamp(procedural_scale, 0.0f, 1.0f);
}

bool iam_get_reduced_motion() {
	return iam_detail::g_reduced_motion;
}

// ----------------------------------------------------
// Lazy Initialization API implementations
// ----------------------------------------------------

void iam_set_lazy_init(bool enable) {
	iam_detail::g_lazy_init_enabled = enable;
}

bool iam_is_lazy_init_enabled() {
	return iam_detail::g_lazy_init_enabled;
}

// ----------------------------------------------------
// Profiler API implementations
// ----------------------------------------------------

void iam_profiler_enable(bool enable) {
	iam_detail::g_profiler.enabled = enable;
	if (enable) {
		// Reset all sections when enabling
		for (int i = 0; i < iam_detail::g_profiler.section_count; i++) {
			iam_detail::g_profiler.sections[i].accumulated_time = 0;
			iam_detail::g_profiler.sections[i].call_count = 0;
		}
	}
}

bool iam_profiler_is_enabled() {
	return iam_detail::g_profiler.enabled;
}

void iam_profiler_begin_frame() {
	if (!iam_detail::g_profiler.enabled) return;
	iam_detail::g_profiler.frame_start_time = iam_detail::get_time_ms();
	iam_detail::g_profiler.stack_depth = 0;
	// Reset per-frame accumulators
	for (int i = 0; i < iam_detail::g_profiler.section_count; i++) {
		iam_detail::g_profiler.sections[i].accumulated_time = 0;
		iam_detail::g_profiler.sections[i].call_count = 0;
	}
}

void iam_profiler_end_frame() {
	if (!iam_detail::g_profiler.enabled) return;
	double end_time = iam_detail::get_time_ms();
	iam_detail::g_profiler.frame_total_time = end_time - iam_detail::g_profiler.frame_start_time;

	// Store frame time in history
	int& idx = iam_detail::g_profiler.frame_history_idx;
	iam_detail::g_profiler.frame_history[idx] = (float)iam_detail::g_profiler.frame_total_time;
	idx = (idx + 1) % iam_detail::PROFILER_HISTORY_SIZE;

	// Store section times in history
	for (int i = 0; i < iam_detail::g_profiler.section_count; i++) {
		auto& sec = iam_detail::g_profiler.sections[i];
		sec.history[sec.history_idx] = (float)sec.accumulated_time;
		sec.history_idx = (sec.history_idx + 1) % iam_detail::PROFILER_HISTORY_SIZE;
	}
}

void iam_profiler_begin(char const* name) {
	if (!iam_detail::g_profiler.enabled) return;
	int idx = iam_detail::g_profiler.find_or_create_section(name);
	if (idx < 0) return;

	auto& sec = iam_detail::g_profiler.sections[idx];
	sec.start_time = iam_detail::get_time_ms();
	sec.call_count++;

	// Push to stack
	if (iam_detail::g_profiler.stack_depth < iam_detail::PROFILER_MAX_STACK) {
		iam_detail::g_profiler.stack[iam_detail::g_profiler.stack_depth++] = idx;
	}
}

void iam_profiler_end() {
	if (!iam_detail::g_profiler.enabled) return;
	if (iam_detail::g_profiler.stack_depth <= 0) return;

	int idx = iam_detail::g_profiler.stack[--iam_detail::g_profiler.stack_depth];
	auto& sec = iam_detail::g_profiler.sections[idx];
	double end_time = iam_detail::get_time_ms();
	sec.accumulated_time += end_time - sec.start_time;
}

void iam_register_custom_ease(int slot, iam_ease_fn fn) {
	if (slot >= 0 && slot < 16) {
		iam_detail::g_custom_ease[slot] = fn;
	}
}

void iam_ease_curve_set(ImGuiID curve_id, iam_vec2 const* points, int count) {
	using namespace iam_detail;
	if (!points || count < 4 || (count - 1) % 3 != 0) return;
	ease_curve* c = find_curve(curve_id);
	if (!c) {
		c = IM_NEW(ease_curve)();
		c->id = curve_id;
		g_curve_map.SetVoidPtr(curve_id, c);
	}
	c->pts.resize(count);
	for (int i = 0; i < count; i++) c->pts[i] = IAM_VEC2_TO_IM(points[i]);
	build_curve_lut(c);
}

int iam_ease_curve_get(ImGuiID curve_id, iam_vec2* out_points, int max_count) {
	iam_detail::ease_curve* c = iam_detail::find_curve(curve_id);
	if (!c) return 0;
	if (out_points) for (int i = 0; i < c->pts.Size && i < max_count; i++) out_points[i] = IAM_VEC2_FROM_IM(c->pts[i]);
	return c->pts.Size;
}

bool iam_ease_curve_exists(ImGuiID curve_id) {
	return iam_detail::find_curve(curve_id) != nullptr;
}

float const* iam_ease_curve_ref(ImGuiID curve_id) {
	static float ring[8][4];
	static int n = 0;
	float* r = ring[n++ & 7];
	r[0] = (float)(curve_id & 0xFFFF);
	r[1] = (float)(curve_id >> 16);
	r[2] = r[3] = 0.0f;
	return r;
}

iam_ease_fn iam_get_custom_ease(int slot) {
	if (slot >= 0 && slot < 16) {
		return iam_detail::g_custom_ease[slot];
	}
	return nullptr;
}

float iam_eval_preset(int type, float t) {
	return iam_detail::eval_preset_internal(type, t);
}

float iam_ease_eval(iam_ease_desc const& ez, float t) {
	return iam_detail::eval(ez, t);
}

float iam_tween_float(ImGuiID id, ImGuiID channel_id, float target, float dur, iam_ease_desc const& ez, int policy, float /*dt*/, float init_value) {
	using namespace iam_detail;
	ImGuiID key = make_key(id, channel_id);

	// Lazy init: if channel doesn't exist and target equals init_value, skip channel creation
	float_chan* c = g_float.try_get(key);
	bool const is_new = (c == nullptr);

	if (is_new) {
		if (g_lazy_init_enabled && fabsf(target - init_value) <= 1e-6f) {
			return target;
		}
		c = g_float.get(key);
		c->current = c->start = c->target = init_value;
	}

	// Fast path: sleeping and target unchanged
	if (c->sleeping && fabsf(c->target - target) <= 1e-6f && !c->has_pending) {
		return c->current;
	}

	// Compute current progress
	float t_now = c->sleeping ? 1.0f : (float)((g_global_time - c->start_time) / c->dur);
	bool anim_complete = t_now >= 1.0f;

	bool const change = (c->policy!=policy) || (c->ez.type!=ez.type) ||
	                    (c->ez.p0!=ez.p0) || (c->ez.p1!=ez.p1) || (c->ez.p2!=ez.p2) || (c->ez.p3!=ez.p3) ||
	                    (fabsf(c->target - target) > 1e-6f) || anim_complete;
	if (change) {
		if (policy == iam_policy_queue && !anim_complete && !c->has_pending) {
			c->pending_target = target; c->has_pending = 1;
		}
		else if (policy == iam_policy_cut) {
			c->current = c->start = c->target = target; c->dur = 1e-6f; c->ez = ez; c->policy = policy; c->sleeping = 1;
		}
		else if (policy == iam_policy_additive) {
			c->evaluate();
			c->set(c->current + target, dur, ez, policy);
		}
		else if (policy == iam_policy_multiply) {
			c->evaluate();
			c->set(c->current * target, dur, ez, policy);
		}
		else {
			c->evaluate();  // Update current before setting new target
			c->set(target, dur, ez, policy);
		}
	}
	if (anim_complete && c->has_pending) { c->set(c->pending_target, dur, ez, policy); c->has_pending = 0; }
	return c->evaluate();
}

static ImVec2 iam_tween_vec2_impl(ImGuiID id, ImGuiID channel_id, ImVec2 target, float dur, iam_ease_desc const& ez, int policy, float /*dt*/, ImVec2 init_value) {
	using namespace iam_detail;
	ImGuiID key = make_key(id, channel_id);

	// Lazy init: if channel doesn't exist and target equals init_value, skip channel creation
	vec2_chan* c = g_vec2.try_get(key);
	bool const is_new = (c == nullptr);

	if (is_new) {
		if (g_lazy_init_enabled && fabsf(target.x - init_value.x) + fabsf(target.y - init_value.y) <= 1e-6f) {
			return target;
		}
		c = g_vec2.get(key);
		c->current = c->start = c->target = init_value;
	}

	if (c->sleeping && fabsf(c->target.x - target.x) + fabsf(c->target.y - target.y) <= 1e-6f && !c->has_pending) {
		return c->current;
	}

	float t_now = c->sleeping ? 1.0f : (float)((g_global_time - c->start_time) / c->dur);
	bool anim_complete = t_now >= 1.0f;

	bool const change = (c->policy!=policy) || (c->ez.type!=ez.type) ||
	                    (c->ez.p0!=ez.p0) || (c->ez.p1!=ez.p1) || (c->ez.p2!=ez.p2) || (c->ez.p3!=ez.p3) ||
	                    (fabsf(c->target.x - target.x) + fabsf(c->target.y - target.y) > 1e-6f) || anim_complete;
	if (change) {
		if (policy == iam_policy_queue && !anim_complete && !c->has_pending) { c->pending_target = target; c->has_pending = 1; }
		else if (policy == iam_policy_cut) { c->current = c->start = c->target = target; c->dur = 1e-6f; c->ez = ez; c->policy = policy; c->sleeping = 1; }
		else if (policy == iam_policy_additive) { c->evaluate(); c->set(ImVec2(c->current.x + target.x, c->current.y + target.y), dur, ez, policy); }
		else if (policy == iam_policy_multiply) { c->evaluate(); c->set(ImVec2(c->current.x * target.x, c->current.y * target.y), dur, ez, policy); }
		else { c->evaluate(); c->set(target, dur, ez, policy); }
	}
	if (anim_complete && c->has_pending) { c->set(c->pending_target, dur, ez, policy); c->has_pending = 0; }
	return c->evaluate();
}

static ImVec4 iam_tween_vec4_impl(ImGuiID id, ImGuiID channel_id, ImVec4 target, float dur, iam_ease_desc const& ez, int policy, float /*dt*/, ImVec4 init_value) {
	using namespace iam_detail;
	ImGuiID key = make_key(id, channel_id);

	// Lazy init: if channel doesn't exist and target equals init_value, skip channel creation
	vec4_chan* c = g_vec4.try_get(key);
	bool const is_new = (c == nullptr);

	if (is_new) {
		if (g_lazy_init_enabled && fabsf(target.x - init_value.x) + fabsf(target.y - init_value.y) + fabsf(target.z - init_value.z) + fabsf(target.w - init_value.w) <= 1e-6f) {
			return target;
		}
		c = g_vec4.get(key);
		c->current = c->start = c->target = init_value;
	}

	if (c->sleeping && fabsf(c->target.x-target.x)+fabsf(c->target.y-target.y)+fabsf(c->target.z-target.z)+fabsf(c->target.w-target.w) <= 1e-6f && !c->has_pending) {
		return c->current;
	}

	float t_now = c->sleeping ? 1.0f : (float)((g_global_time - c->start_time) / c->dur);
	bool anim_complete = t_now >= 1.0f;

	bool const change = (c->policy!=policy) || (c->ez.type!=ez.type) ||
	                    (c->ez.p0!=ez.p0) || (c->ez.p1!=ez.p1) || (c->ez.p2!=ez.p2) || (c->ez.p3!=ez.p3) ||
	                    (fabsf(c->target.x-target.x)+fabsf(c->target.y-target.y)+fabsf(c->target.z-target.z)+fabsf(c->target.w-target.w) > 1e-6f) || anim_complete;
	if (change) {
		if (policy == iam_policy_queue && !anim_complete && !c->has_pending) { c->pending_target = target; c->has_pending = 1; }
		else if (policy == iam_policy_cut) { c->current = c->start = c->target = target; c->dur = 1e-6f; c->ez = ez; c->policy = policy; c->sleeping = 1; }
		else if (policy == iam_policy_additive) { c->evaluate(); c->set(ImVec4(c->current.x+target.x, c->current.y+target.y, c->current.z+target.z, c->current.w+target.w), dur, ez, policy); }
		else if (policy == iam_policy_multiply) { c->evaluate(); c->set(ImVec4(c->current.x*target.x, c->current.y*target.y, c->current.z*target.z, c->current.w*target.w), dur, ez, policy); }
		else { c->evaluate(); c->set(target, dur, ez, policy); }
	}
	if (anim_complete && c->has_pending) { c->set(c->pending_target, dur, ez, policy); c->has_pending = 0; }
	return c->evaluate();
}

int iam_tween_int(ImGuiID id, ImGuiID channel_id, int target, float dur, iam_ease_desc const& ez, int policy, float /*dt*/, int init_value) {
	using namespace iam_detail;
	ImGuiID key = make_key(id, channel_id);

	// Lazy init: if channel doesn't exist and target equals init_value, skip channel creation
	int_chan* c = g_int.try_get(key);
	bool const is_new = (c == nullptr);

	if (is_new) {
		if (g_lazy_init_enabled && target == init_value) {
			return target;
		}
		c = g_int.get(key);
		c->current = c->start = c->target = init_value;
	}

	if (c->sleeping && c->target == target && !c->has_pending) { return c->current; }

	float t_now = c->sleeping ? 1.0f : (float)((g_global_time - c->start_time) / c->dur);
	bool anim_complete = t_now >= 1.0f;

	bool const change = (c->policy!=policy) || (c->ez.type!=ez.type) ||
	                    (c->ez.p0!=ez.p0) || (c->ez.p1!=ez.p1) || (c->ez.p2!=ez.p2) || (c->ez.p3!=ez.p3) ||
	                    (c->target != target) || anim_complete;
	if (change) {
		if (policy == iam_policy_queue && !anim_complete && !c->has_pending) { c->pending_target = target; c->has_pending = 1; }
		else if (policy == iam_policy_cut) { c->current = c->start = c->target = target; c->dur = 1e-6f; c->ez = ez; c->policy = policy; c->sleeping = 1; }
		else if (policy == iam_policy_additive) { c->evaluate(); c->set(c->current + target, dur, ez, policy); }
		else if (policy == iam_policy_multiply) { c->evaluate(); c->set(c->current * target, dur, ez, policy); }
		else { c->evaluate(); c->set(target, dur, ez, policy); }
	}
	if (anim_complete && c->has_pending) { c->set(c->pending_target, dur, ez, policy); c->has_pending = 0; }
	return c->evaluate();
}

static ImVec4 iam_tween_color_impl(ImGuiID id, ImGuiID channel_id, ImVec4 target_srgb, float dur, iam_ease_desc const& ez, int policy, int color_space, float /*dt*/, ImVec4 init_value) {
	using namespace iam_detail;
	ImGuiID key = make_key(id, channel_id);

	// Lazy init: if channel doesn't exist and target equals init_value, skip channel creation
	color_chan* c = g_color.try_get(key);
	bool const is_new = (c == nullptr);

	if (is_new) {
		if (g_lazy_init_enabled && fabsf(target_srgb.x - init_value.x) + fabsf(target_srgb.y - init_value.y) + fabsf(target_srgb.z - init_value.z) + fabsf(target_srgb.w - init_value.w) <= 1e-6f) {
			return target_srgb;
		}
		c = g_color.get(key);
		c->current = c->start = c->target = init_value;
	}

	if (c->sleeping && (fabsf(c->target.x-target_srgb.x)+fabsf(c->target.y-target_srgb.y)+fabsf(c->target.z-target_srgb.z)+fabsf(c->target.w-target_srgb.w)) <= 1e-6f) { return c->current; }

	float t_now = c->sleeping ? 1.0f : (float)((g_global_time - c->start_time) / c->dur);
	bool anim_complete = t_now >= 1.0f;

	bool const change = (c->policy!=policy) || (c->space != color_space) || (c->ez.type!=ez.type) ||
	                    (c->ez.p0!=ez.p0) || (c->ez.p1!=ez.p1) || (c->ez.p2!=ez.p2) || (c->ez.p3!=ez.p3) ||
	                    (fabsf(c->target.x-target_srgb.x)+fabsf(c->target.y-target_srgb.y)+fabsf(c->target.z-target_srgb.z)+fabsf(c->target.w-target_srgb.w) > 1e-6f) || anim_complete;
	if (change) {
		if (policy == iam_policy_cut) { c->current = c->start = c->target = target_srgb; c->dur = 1e-6f; c->ez = ez; c->policy = policy; c->space = color_space; c->sleeping = 1; }
		else if (policy == iam_policy_additive) {
			c->evaluate();
			ImVec4 add_target(c->current.x+target_srgb.x, c->current.y+target_srgb.y, c->current.z+target_srgb.z, c->current.w+target_srgb.w);
			c->set(add_target, dur, ez, policy, color_space);
		}
		else if (policy == iam_policy_multiply) {
			c->evaluate();
			ImVec4 mul_target(c->current.x*target_srgb.x, c->current.y*target_srgb.y, c->current.z*target_srgb.z, c->current.w*target_srgb.w);
			c->set(mul_target, dur, ez, policy, color_space);
		}
		else { c->evaluate(); c->set(target_srgb, dur, ez, policy, color_space); }
	}
	return c->evaluate();
}

// ============================================================
// PER-AXIS EASING - Different easing per component
// ============================================================

static ImVec2 iam_tween_vec2_per_axis_impl(ImGuiID id, ImGuiID channel_id, ImVec2 target, float dur, iam_ease_per_axis const& ez, int policy, float dt) {
	// Use separate float tweens for each axis with their own easing
	ImGuiID ch_x = ImHashData(&channel_id, sizeof(channel_id), ImHashStr("_pa_x"));
	ImGuiID ch_y = ImHashData(&channel_id, sizeof(channel_id), ImHashStr("_pa_y"));

	float x = iam_tween_float(id, ch_x, target.x, dur, ez.x, policy, dt);
	float y = iam_tween_float(id, ch_y, target.y, dur, ez.y, policy, dt);

	return ImVec2(x, y);
}

static ImVec4 iam_tween_vec4_per_axis_impl(ImGuiID id, ImGuiID channel_id, ImVec4 target, float dur, iam_ease_per_axis const& ez, int policy, float dt) {
	// Use separate float tweens for each axis with their own easing
	ImGuiID ch_x = ImHashData(&channel_id, sizeof(channel_id), ImHashStr("_pa_x"));
	ImGuiID ch_y = ImHashData(&channel_id, sizeof(channel_id), ImHashStr("_pa_y"));
	ImGuiID ch_z = ImHashData(&channel_id, sizeof(channel_id), ImHashStr("_pa_z"));
	ImGuiID ch_w = ImHashData(&channel_id, sizeof(channel_id), ImHashStr("_pa_w"));

	float x = iam_tween_float(id, ch_x, target.x, dur, ez.x, policy, dt);
	float y = iam_tween_float(id, ch_y, target.y, dur, ez.y, policy, dt);
	float z = iam_tween_float(id, ch_z, target.z, dur, ez.z, policy, dt);
	float w = iam_tween_float(id, ch_w, target.w, dur, ez.w, policy, dt);

	return ImVec4(x, y, z, w);
}

static ImVec4 iam_tween_color_per_axis_impl(ImGuiID id, ImGuiID channel_id, ImVec4 target_srgb, float dur, iam_ease_per_axis const& ez, int policy, int color_space, float dt) {
	// For colors, we apply per-axis easing in the target color space
	// Convert target to working space
	ImVec4 target_work = iam_detail::color::to_space(target_srgb, color_space);

	// Use separate float tweens for each channel with their own easing
	ImGuiID ch_r = ImHashData(&channel_id, sizeof(channel_id), ImHashStr("_pa_r"));
	ImGuiID ch_g = ImHashData(&channel_id, sizeof(channel_id), ImHashStr("_pa_g"));
	ImGuiID ch_b = ImHashData(&channel_id, sizeof(channel_id), ImHashStr("_pa_b"));
	ImGuiID ch_a = ImHashData(&channel_id, sizeof(channel_id), ImHashStr("_pa_a"));

	// Get current values in working space for interpolation
	dt *= iam_detail::g_time_scale;
	ImGuiID key_r = iam_detail::make_key(id, ch_r);
	ImGuiID key_g = iam_detail::make_key(id, ch_g);
	ImGuiID key_b = iam_detail::make_key(id, ch_b);
	ImGuiID key_a = iam_detail::make_key(id, ch_a);

	iam_detail::float_chan* cr = iam_detail::g_float.get(key_r);
	iam_detail::float_chan* cg = iam_detail::g_float.get(key_g);
	iam_detail::float_chan* cb = iam_detail::g_float.get(key_b);
	iam_detail::float_chan* ca = iam_detail::g_float.get(key_a);

	// Check if this is a new animation (target changed)
	bool change_r = fabsf(cr->target - target_work.x) > 1e-6f || cr->t >= 1.0f;
	bool change_g = fabsf(cg->target - target_work.y) > 1e-6f || cg->t >= 1.0f;
	bool change_b = fabsf(cb->target - target_work.z) > 1e-6f || cb->t >= 1.0f;
	bool change_a = fabsf(ca->target - target_work.w) > 1e-6f || ca->t >= 1.0f;

	// Update each channel
	auto update_chan = [&](iam_detail::float_chan* c, float target_val, iam_ease_desc const& e, bool changed) {
		if (changed) {
			if (policy == iam_policy_cut) {
				c->current = c->start = c->target = target_val;
				c->sleeping = 1; c->dur = 1e-6f; c->ez = e; c->policy = policy;
			} else {
				if (c->progress() < 1.0f && dt > 0) c->tick(dt);
				c->set(target_val, dur, e, policy);
				c->tick(dt);
			}
		} else {
			c->tick(dt);
		}
		return c->current;
	};

	float r = update_chan(cr, target_work.x, ez.x, change_r);
	float g = update_chan(cg, target_work.y, ez.y, change_g);
	float b = update_chan(cb, target_work.z, ez.z, change_b);
	float a = update_chan(ca, target_work.w, ez.w, change_a);

	// Convert back to sRGB
	return iam_detail::color::from_space(ImVec4(r, g, b, a), color_space);
}

static ImVec2 iam_anchor_size_impl(int space) {
	switch (space) {
		case iam_anchor_window_content: return ImGui::GetContentRegionAvail();
		case iam_anchor_window:         return ImGui::GetWindowSize();
		case iam_anchor_viewport:
#ifdef IMGUI_HAS_VIEWPORT
			return ImGui::GetWindowViewport()->Size;
#else
			return ImGui::GetIO().DisplaySize;
#endif
		case iam_anchor_last_item: {
			ImVec2 mi = ImGui::GetItemRectMin(), ma = ImGui::GetItemRectMax();
			return ImVec2(ma.x - mi.x, ma.y - mi.y);
		}
		default: return ImVec2(0,0);
	}
}

static ImVec2 iam_tween_vec2_rel_impl(ImGuiID id, ImGuiID channel_id, ImVec2 percent, ImVec2 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt) {
	ImVec2 base = iam_anchor_size_impl(anchor_space);
	ImVec2 target = ImVec2(base.x*percent.x + px_bias.x, base.y*percent.y + px_bias.y);
	return iam_tween_vec2_impl(id, channel_id, target, dur, ez, policy, dt);
}

static ImVec2 iam_tween_vec2_resolved_impl(ImGuiID id, ImGuiID channel_id, iam_vec2_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt) {
	ImVec2 target = fn ? IAM_VEC2_TO_IM(fn(user)) : ImVec2(0,0);
	return iam_tween_vec2_impl(id, channel_id, target, dur, ez, policy, dt);
}

static void iam_rebase_vec2_impl(ImGuiID id, ImGuiID channel_id, ImVec2 new_target, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	int idx = iam_detail::g_vec2.pool.Map.GetInt(key, -1);
	if (idx == -1) return;
	iam_detail::vec2_chan* c = iam_detail::g_vec2.pool.GetByIndex(idx);
	if (c->progress() < 1.0f && dt > 0) c->tick(dt);
	float remain = (1.0f - (c->progress() < 1.0f ? c->t : 1.0f)) * c->dur;
	c->start = c->current;
	c->target = new_target;
	c->start_time = iam_detail::g_global_time; c->sleeping = 0;
	c->dur = iam_detail::motion_dur(remain <= 1e-6f ? 1e-6f : remain);
}

// Additional resolved/rel/rebase functions for all types

float iam_tween_float_rel(ImGuiID id, ImGuiID channel_id, float percent, float px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, int axis, float dt) {
	ImVec2 base = iam_anchor_size_impl(anchor_space);
	float target = (axis == 0 ? base.x : base.y) * percent + px_bias;
	return iam_tween_float(id, channel_id, target, dur, ez, policy, dt);
}

static ImVec4 iam_tween_vec4_rel_impl(ImGuiID id, ImGuiID channel_id, ImVec4 percent, ImVec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt) {
	ImVec2 base = iam_anchor_size_impl(anchor_space);
	ImVec4 target = ImVec4(base.x*percent.x + px_bias.x, base.y*percent.y + px_bias.y, percent.z + px_bias.z, percent.w + px_bias.w);
	return iam_tween_vec4_impl(id, channel_id, target, dur, ez, policy, dt);
}

static ImVec4 iam_tween_color_rel_impl(ImGuiID id, ImGuiID channel_id, ImVec4 percent, ImVec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int color_space, int anchor_space, float dt) {
	// For color, percent/px_bias represent color component modifiers (not spatial anchors)
	(void)anchor_space; // Color doesn't use spatial anchors
	ImVec4 target = ImVec4(percent.x + px_bias.x, percent.y + px_bias.y, percent.z + px_bias.z, percent.w + px_bias.w);
	return iam_tween_color_impl(id, channel_id, target, dur, ez, policy, color_space, dt);
}

float iam_tween_float_resolved(ImGuiID id, ImGuiID channel_id, iam_float_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt) {
	float target = fn ? fn(user) : 0.0f;
	return iam_tween_float(id, channel_id, target, dur, ez, policy, dt);
}

static ImVec4 iam_tween_vec4_resolved_impl(ImGuiID id, ImGuiID channel_id, iam_vec4_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt) {
	ImVec4 target = fn ? IAM_VEC4_TO_IM(fn(user)) : ImVec4(0,0,0,0);
	return iam_tween_vec4_impl(id, channel_id, target, dur, ez, policy, dt);
}

static ImVec4 iam_tween_color_resolved_impl(ImGuiID id, ImGuiID channel_id, iam_color_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt) {
	ImVec4 target = fn ? IAM_VEC4_TO_IM(fn(user)) : ImVec4(0,0,0,1);
	return iam_tween_color_impl(id, channel_id, target, dur, ez, policy, color_space, dt);
}

int iam_tween_int_resolved(ImGuiID id, ImGuiID channel_id, iam_int_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt) {
	int target = fn ? fn(user) : 0;
	return iam_tween_int(id, channel_id, target, dur, ez, policy, dt);
}

void iam_rebase_float(ImGuiID id, ImGuiID channel_id, float new_target, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	int idx = iam_detail::g_float.pool.Map.GetInt(key, -1);
	if (idx == -1) return;
	iam_detail::float_chan* c = iam_detail::g_float.pool.GetByIndex(idx);
	if (c->progress() < 1.0f && dt > 0) c->tick(dt);
	float remain = (1.0f - (c->progress() < 1.0f ? c->t : 1.0f)) * c->dur;
	c->start = c->current;
	c->target = new_target;
	c->start_time = iam_detail::g_global_time; c->sleeping = 0;
	c->dur = iam_detail::motion_dur(remain <= 1e-6f ? 1e-6f : remain);
}

static void iam_rebase_vec4_impl(ImGuiID id, ImGuiID channel_id, ImVec4 new_target, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	int idx = iam_detail::g_vec4.pool.Map.GetInt(key, -1);
	if (idx == -1) return;
	iam_detail::vec4_chan* c = iam_detail::g_vec4.pool.GetByIndex(idx);
	if (c->progress() < 1.0f && dt > 0) c->tick(dt);
	float remain = (1.0f - (c->progress() < 1.0f ? c->t : 1.0f)) * c->dur;
	c->start = c->current;
	c->target = new_target;
	c->start_time = iam_detail::g_global_time; c->sleeping = 0;
	c->dur = iam_detail::motion_dur(remain <= 1e-6f ? 1e-6f : remain);
}

static void iam_rebase_color_impl(ImGuiID id, ImGuiID channel_id, ImVec4 new_target, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	int idx = iam_detail::g_color.pool.Map.GetInt(key, -1);
	if (idx == -1) return;
	iam_detail::color_chan* c = iam_detail::g_color.pool.GetByIndex(idx);
	if (c->progress() < 1.0f && dt > 0) c->tick(dt);
	float remain = (1.0f - (c->progress() < 1.0f ? c->t : 1.0f)) * c->dur;
	c->start = c->current;
	c->target = new_target;
	c->start_time = iam_detail::g_global_time; c->sleeping = 0;
	c->dur = iam_detail::motion_dur(remain <= 1e-6f ? 1e-6f : remain);
}

void iam_rebase_int(ImGuiID id, ImGuiID channel_id, int new_target, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	int idx = iam_detail::g_int.pool.Map.GetInt(key, -1);
	if (idx == -1) return;
	iam_detail::int_chan* c = iam_detail::g_int.pool.GetByIndex(idx);
	if (c->progress() < 1.0f && dt > 0) c->tick(dt);
	float remain = (1.0f - (c->progress() < 1.0f ? c->t : 1.0f)) * c->dur;
	c->start = c->current;
	c->target = new_target;
	c->start_time = iam_detail::g_global_time; c->sleeping = 0;
	c->dur = iam_detail::motion_dur(remain <= 1e-6f ? 1e-6f : remain);
}

static ImVec4 iam_get_blended_color_impl(ImVec4 a_srgb, ImVec4 b_srgb, float t, int color_space) {
	return iam_detail::color::lerp_color(a_srgb, b_srgb, t, color_space);
}

// ----------------------------------------------------
// Spring channels
// ----------------------------------------------------
namespace iam_spring_detail {
	struct spring_chan {
		float x[4], v[4];
		int space;               // color space for color springs (-1 otherwise)
		unsigned last_seen_frame;
		unsigned last_step_frame;
		bool settled;
		spring_chan() : space(-1), last_seen_frame(0), last_step_frame(0), settled(true) {
			for (int i = 0; i < 4; i++) x[i] = v[i] = 0.0f;
		}
	};
	static ImPool<spring_chan> g_springs;

	// Distinct key per value type so a float and a vec2 spring can share (id, channel)
	static ImGuiID key(ImGuiID id, ImGuiID channel, int n) {
		ImGuiID k = iam_detail::make_key(id, channel);
		return ImHashData(&n, sizeof(n), k);
	}

	static spring_chan* get(ImGuiID k, float const* init, int n, iam_spring_params const& p, bool* created) {
		spring_chan* s = g_springs.GetByKey(k);
		*created = (s == nullptr);
		if (!s) {
			s = g_springs.GetOrAddByKey(k);
			for (int i = 0; i < n; i++) { s->x[i] = init[i]; s->v[i] = p.initial_velocity; }
			s->settled = false;
			s->last_step_frame = iam_detail::g_frame - 1;  // allow a step this frame
		}
		s->last_seen_frame = iam_detail::g_frame;
		return s;
	}

	// Semi-implicit Euler with fixed substeps; once per frame
	static void step(spring_chan* s, float const* target, int n, iam_spring_params const& p, float dt) {
		bool moved_target = false;
		for (int i = 0; i < n; i++) moved_target |= ImFabs(s->x[i] - target[i]) > 1e-6f;
		if (s->settled && !moved_target) return;
		if (iam_detail::g_reduced_motion) {  // reduced motion: jump to the target
			for (int i = 0; i < n; i++) { s->x[i] = target[i]; s->v[i] = 0.0f; }
			s->settled = true;
			return;
		}
		if (s->last_step_frame == iam_detail::g_frame) return;
		s->last_step_frame = iam_detail::g_frame;
		dt *= iam_detail::g_time_scale;
		if (dt <= 0.0f) return;
		if (dt > 0.1f) dt = 0.1f;
		float const m = p.mass > 1e-4f ? p.mass : 1e-4f;
		float const k = p.stiffness > 0.0f ? p.stiffness : 0.0f;
		float const c = p.damping > 0.0f ? p.damping : 0.0f;
		int const substeps = ImClamp((int)ImCeil(dt / (1.0f / 240.0f)), 1, 32);
		float const h = dt / substeps;
		for (int s_i = 0; s_i < substeps; s_i++)
			for (int i = 0; i < n; i++) {
				float a = (-k * (s->x[i] - target[i]) - c * s->v[i]) / m;
				s->v[i] += a * h;
				s->x[i] += s->v[i] * h;
			}
		// Settle: close enough and slow enough -> snap to the target
		float scale = 1e-3f;
		for (int i = 0; i < n; i++) scale = ImMax(scale, ImFabs(target[i]) * 1e-4f);
		bool settled = true;
		for (int i = 0; i < n; i++) settled &= ImFabs(s->x[i] - target[i]) < scale && ImFabs(s->v[i]) < scale * 10.0f;
		if (settled) for (int i = 0; i < n; i++) { s->x[i] = target[i]; s->v[i] = 0.0f; }
		s->settled = settled;
	}

	// io: target in, value out
	static void run(ImGuiID id, ImGuiID channel, float* io, float const* init, int n, iam_spring_params const& p, float dt) {
		bool created;
		spring_chan* s = get(key(id, channel, n), init, n, p, &created);
		step(s, io, n, p, dt);
		for (int i = 0; i < n; i++) io[i] = s->x[i];
	}

	static void gc(unsigned max_age) {
		for (int i = 0; i < g_springs.GetMapSize(); ++i) {
			if (spring_chan* s = g_springs.TryGetMapData(i)) {
				if (iam_detail::g_frame - s->last_seen_frame > max_age) {
					ImGuiID k = g_springs.Map.Data[i].key;
					g_springs.Remove(k, g_springs.GetIndex(s));
				}
			}
		}
	}

	static spring_chan* find(ImGuiID id, ImGuiID channel, int* out_n) {
		static int const ns[] = { 1, 2, 4, 5 };  // 5 = color
		for (int i = 0; i < 4; i++) {
			spring_chan* s = g_springs.GetByKey(key(id, channel, ns[i]));
			if (s) { if (out_n) *out_n = ns[i] == 5 ? 4 : ns[i]; return s; }
		}
		return nullptr;
	}
} // namespace iam_spring_detail

static void iam_spring_gc(unsigned max_age) { iam_spring_detail::gc(max_age); }
static void iam_spring_clear() { iam_spring_detail::g_springs.Clear(); }

float iam_spring_float(ImGuiID id, ImGuiID channel_id, float target, iam_spring_params const& params, float dt, float init_value) {
	float v[1] = { target }, init[1] = { init_value };
	iam_spring_detail::run(id, channel_id, v, init, 1, params, dt);
	return v[0];
}

iam_vec2 iam_spring_vec2(ImGuiID id, ImGuiID channel_id, iam_vec2 target_in, iam_spring_params const& params, float dt, iam_vec2 init_in) {
	ImVec2 t = IAM_VEC2_TO_IM(target_in), i0 = IAM_VEC2_TO_IM(init_in);
	float v[2] = { t.x, t.y }, init[2] = { i0.x, i0.y };
	iam_spring_detail::run(id, channel_id, v, init, 2, params, dt);
	return IAM_VEC2_FROM_IM(ImVec2(v[0], v[1]));
}

iam_vec4 iam_spring_vec4(ImGuiID id, ImGuiID channel_id, iam_vec4 target_in, iam_spring_params const& params, float dt, iam_vec4 init_in) {
	ImVec4 t = IAM_VEC4_TO_IM(target_in), i0 = IAM_VEC4_TO_IM(init_in);
	float v[4] = { t.x, t.y, t.z, t.w }, init[4] = { i0.x, i0.y, i0.z, i0.w };
	iam_spring_detail::run(id, channel_id, v, init, 4, params, dt);
	return IAM_VEC4_FROM_IM(ImVec4(v[0], v[1], v[2], v[3]));
}

iam_vec4 iam_spring_color(ImGuiID id, ImGuiID channel_id, iam_vec4 target_srgb, iam_spring_params const& params, int color_space, float dt, iam_vec4 init_value) {
	using namespace iam_spring_detail;
	ImVec4 t = iam_detail::color::to_space(IAM_VEC4_TO_IM(target_srgb), color_space);
	ImVec4 i0 = iam_detail::color::to_space(IAM_VEC4_TO_IM(init_value), color_space);
	float v[4] = { t.x, t.y, t.z, t.w }, init[4] = { i0.x, i0.y, i0.z, i0.w };
	bool created;
	spring_chan* s = get(key(id, channel_id, 5), init, 4, params, &created);
	if (!created && s->space >= 0 && s->space != color_space) {
		// Color space changed: re-express the current value (velocity restarts)
		ImVec4 cur = iam_detail::color::from_space(ImVec4(s->x[0], s->x[1], s->x[2], s->x[3]), s->space);
		cur = iam_detail::color::to_space(cur, color_space);
		s->x[0] = cur.x; s->x[1] = cur.y; s->x[2] = cur.z; s->x[3] = cur.w;
		for (int i = 0; i < 4; i++) s->v[i] = 0.0f;
		s->settled = false;
	}
	s->space = color_space;
	step(s, v, 4, params, dt);
	return IAM_VEC4_FROM_IM(iam_detail::color::from_space(ImVec4(s->x[0], s->x[1], s->x[2], s->x[3]), color_space));
}

// ----------------------------------------------------
// Tween state queries
// ----------------------------------------------------
namespace iam_query_detail {
	using namespace iam_detail;

	// Slope of the ease curve at t (numeric, one-sided at the ends)
	static float ease_slope(iam_ease_desc const& ez, float t) {
		float const h = 1e-3f;
		float a = ImMax(t - h, 0.0f), b = ImMin(t + h, 1.0f);
		return (b > a) ? (eval(ez, b) - eval(ez, a)) / (b - a) : 0.0f;
	}

	static bool differs(float a, float b) { return ImFabs(a - b) > 1e-6f; }
	static bool differs(int a, int b) { return a != b; }
	static bool differs(ImVec2 a, ImVec2 b) { return differs(a.x, b.x) || differs(a.y, b.y); }
	static bool differs(ImVec4 a, ImVec4 b) { return differs(a.x, b.x) || differs(a.y, b.y) || differs(a.z, b.z) || differs(a.w, b.w); }

	// Running and actually moving (a finished tween may be restarted with start == target)
	template<typename C> static bool chan_active(C* c) {
		if (!c || c->sleeping) return false;
		return (g_global_time - c->start_time) < c->dur && differs(c->start, c->target);
	}
	template<typename C> static float chan_progress(C* c) {
		if (!chan_active(c)) return 1.0f;
		return ImClamp((float)((g_global_time - c->start_time) / c->dur), 0.0f, 1.0f);
	}
	// d(value)/dt = (target - start) * chan_speed for a lerp(start, target, ease(t)) tween
	template<typename C> static float chan_speed(C* c) {
		if (!chan_active(c)) return 0.0f;
		return ease_slope(c->ez, chan_progress(c)) / c->dur;
	}
} // namespace iam_query_detail

bool iam_tween_is_active(ImGuiID id, ImGuiID channel_id) {
	using namespace iam_query_detail;
	ImGuiID k = make_key(id, channel_id);
	float_chan* f = g_float.try_get(k); if (chan_active(f) || (f && f->has_pending)) return true;
	vec2_chan* v2 = g_vec2.try_get(k);  if (chan_active(v2) || (v2 && v2->has_pending)) return true;
	vec4_chan* v4 = g_vec4.try_get(k);  if (chan_active(v4) || (v4 && v4->has_pending)) return true;
	int_chan* in = g_int.try_get(k);    if (chan_active(in) || (in && in->has_pending)) return true;
	if (chan_active(g_color.try_get(k))) return true;
	iam_spring_detail::spring_chan* s = iam_spring_detail::find(id, channel_id, nullptr);
	return s && !s->settled;
}

float iam_tween_progress(ImGuiID id, ImGuiID channel_id) {
	using namespace iam_query_detail;
	ImGuiID k = make_key(id, channel_id);
	if (float_chan* c = g_float.try_get(k)) return chan_progress(c);
	if (vec2_chan* c = g_vec2.try_get(k)) return chan_progress(c);
	if (vec4_chan* c = g_vec4.try_get(k)) return chan_progress(c);
	if (int_chan* c = g_int.try_get(k)) return chan_progress(c);
	if (color_chan* c = g_color.try_get(k)) return chan_progress(c);
	iam_spring_detail::spring_chan* s = iam_spring_detail::find(id, channel_id, nullptr);
	return (s && !s->settled) ? 0.0f : 1.0f;
}

float iam_tween_velocity_float(ImGuiID id, ImGuiID channel_id) {
	using namespace iam_query_detail;
	ImGuiID k = make_key(id, channel_id);
	if (float_chan* c = g_float.try_get(k)) return (c->target - c->start) * chan_speed(c);
	if (int_chan* c = g_int.try_get(k)) return (float)(c->target - c->start) * chan_speed(c);
	iam_spring_detail::spring_chan* s = iam_spring_detail::find(id, channel_id, nullptr);
	return s ? s->v[0] : 0.0f;
}

iam_vec2 iam_tween_velocity_vec2(ImGuiID id, ImGuiID channel_id) {
	using namespace iam_query_detail;
	ImGuiID k = make_key(id, channel_id);
	if (vec2_chan* c = g_vec2.try_get(k)) {
		float sp = chan_speed(c);
		return IAM_VEC2_FROM_IM(ImVec2((c->target.x - c->start.x) * sp, (c->target.y - c->start.y) * sp));
	}
	int n = 0;
	iam_spring_detail::spring_chan* s = iam_spring_detail::find(id, channel_id, &n);
	return IAM_VEC2_FROM_IM(s && n >= 2 ? ImVec2(s->v[0], s->v[1]) : ImVec2(0, 0));
}

iam_vec4 iam_tween_velocity_vec4(ImGuiID id, ImGuiID channel_id) {
	using namespace iam_query_detail;
	ImGuiID k = make_key(id, channel_id);
	if (vec4_chan* c = g_vec4.try_get(k)) {
		float sp = chan_speed(c);
		return IAM_VEC4_FROM_IM(ImVec4((c->target.x - c->start.x) * sp, (c->target.y - c->start.y) * sp, (c->target.z - c->start.z) * sp, (c->target.w - c->start.w) * sp));
	}
	if (color_chan* c = g_color.try_get(k)) {
		ImVec4 a = color::to_space(c->start, c->space), b = color::to_space(c->target, c->space);
		float sp = chan_speed(c);
		return IAM_VEC4_FROM_IM(ImVec4((b.x - a.x) * sp, (b.y - a.y) * sp, (b.z - a.z) * sp, (b.w - a.w) * sp));
	}
	int n = 0;
	iam_spring_detail::spring_chan* s = iam_spring_detail::find(id, channel_id, &n);
	return IAM_VEC4_FROM_IM(s && n == 4 ? ImVec4(s->v[0], s->v[1], s->v[2], s->v[3]) : ImVec4(0, 0, 0, 0));
}

// ----------------------------------------------------
// Presence (enter/exit)
// ----------------------------------------------------
namespace iam_presence_detail {
	struct presence_state {
		float from, to;        // current segment: value goes from -> to
		float u;               // segment progress [0,1]
		float dur;             // segment duration (scaled by distance)
		float value;
		bool  entering;        // segment direction (to == 1)
		unsigned last_seen_frame;
		unsigned last_step_frame;
	};
	static ImPool<presence_state> g_states;

	static void gc(unsigned max_age) {
		for (int i = 0; i < g_states.GetMapSize(); ++i) {
			if (presence_state* s = g_states.TryGetMapData(i)) {
				if (iam_detail::g_frame - s->last_seen_frame > max_age) {
					ImGuiID k = g_states.Map.Data[i].key;
					g_states.Remove(k, g_states.GetIndex(s));
				}
			}
		}
	}
} // namespace iam_presence_detail

static void iam_presence_gc(unsigned max_age) { iam_presence_detail::gc(max_age); }
static void iam_presence_clear() { iam_presence_detail::g_states.Clear(); }

bool iam_presence(ImGuiID id, bool visible, float* out_t, float dt, iam_presence_opts const& opts) {
	using namespace iam_presence_detail;
	presence_state* s = g_states.GetByKey(id);
	if (!s) {
		s = g_states.GetOrAddByKey(id);
		float v0 = (visible && !opts.animate_first) ? 1.0f : 0.0f;
		s->from = s->to = s->value = v0;
		s->u = 1.0f;
		s->dur = 0.0f;
		s->entering = visible;
		s->last_step_frame = iam_detail::g_frame - 1;
	}
	s->last_seen_frame = iam_detail::g_frame;

	// New direction: start a segment from the current value (continuous when reversing mid-way)
	float target = visible ? 1.0f : 0.0f;
	if (s->to != target) {
		s->from = s->value;
		s->to = target;
		s->u = 0.0f;
		s->entering = visible;
		float full = visible ? opts.enter_duration : opts.exit_duration;
		float dist = ImClamp(ImFabs(target - s->value), 0.0f, 1.0f);
		s->dur = iam_detail::motion_dur(full * dist);
	}

	// Advance once per frame
	if (s->last_step_frame != iam_detail::g_frame) {
		s->last_step_frame = iam_detail::g_frame;
		if (s->u < 1.0f) {
			float step = dt * iam_detail::g_time_scale;
			s->u = (s->dur <= 1e-6f) ? 1.0f : ImMin(s->u + step / s->dur, 1.0f);
		}
	}
	float w = (s->u >= 1.0f) ? 1.0f : iam_detail::eval(s->entering ? opts.enter_ease : opts.exit_ease, s->u);
	s->value = s->from + (s->to - s->from) * w;
	if (s->u >= 1.0f) s->value = s->to;

	if (out_t) *out_t = s->value;
	return visible || s->u < 1.0f;
}

bool iam_presence_is_animating(ImGuiID id) {
	iam_presence_detail::presence_state* s = iam_presence_detail::g_states.GetByKey(id);
	return s && s->u < 1.0f;
}

// ----------------------------------------------------
// Path drawing
// ----------------------------------------------------
namespace iam_path_draw_detail {
	// Fill pts with positions at fractions [a, b] using sample(t); returns point count
	template<typename F>
	static int sample_range(ImVector<ImVec2>& pts, float a, float b, int n, ImVec2 offset, F sample) {
		for (int i = 0; i <= n; i++) {
			float t = a + (b - a) * (float)i / (float)n;
			ImVec2 p = sample(t);
			pts.push_back(ImVec2(p.x + offset.x, p.y + offset.y));
		}
		return n + 1;
	}

	// Handles wrapping (t1 < t0) by drawing [t0, 1] then [0, t1]
	template<typename F>
	static void draw(ImDrawList* dl, float t0, float t1, float length, ImU32 col, float thickness, ImVec2 offset, int segments, F sample) {
		if (!dl || (col & IM_COL32_A_MASK) == 0) return;
		t0 = ImClamp(t0, 0.0f, 1.0f);
		t1 = ImClamp(t1, 0.0f, 1.0f);
		float spans[2][2];
		int span_count = 0;
		if (t1 >= t0) { spans[0][0] = t0; spans[0][1] = t1; span_count = 1; }
		else { spans[0][0] = t0; spans[0][1] = 1.0f; spans[1][0] = 0.0f; spans[1][1] = t1; span_count = 2; }
		static ImVector<ImVec2> pts;
		for (int s = 0; s < span_count; s++) {
			float a = spans[s][0], b = spans[s][1];
			if (b - a <= 1e-6f) continue;
			int n = segments > 0 ? segments : ImClamp((int)(length * (b - a) / 4.0f), 8, 512);
			pts.resize(0);
			sample_range(pts, a, b, n, offset, sample);
			dl->AddPolyline(pts.Data, pts.Size, col, 0, thickness);
		}
	}
} // namespace iam_path_draw_detail

void iam_path_draw(ImDrawList* dl, ImGuiID path_id, float t0, float t1, ImU32 col, float thickness, iam_vec2 offset, int segments) {
	if (!iam_path_exists(path_id)) return;
	float len = iam_path_length(path_id);
	if (len <= 0.0f) return;
	iam_path_draw_detail::draw(dl, t0, t1, len, col, thickness, IAM_VEC2_TO_IM(offset), segments,
		[&](float t) { return iam_path_evaluate_at_distance_impl(path_id, t * len); });
}

void iam_path_morph_draw(ImDrawList* dl, ImGuiID path_a, ImGuiID path_b, float blend, float t0, float t1, ImU32 col, float thickness,
                         iam_vec2 offset, iam_morph_opts const& opts, int segments) {
	if (!iam_path_exists(path_a) || !iam_path_exists(path_b)) return;
	float len = iam_path_length(path_a) + (iam_path_length(path_b) - iam_path_length(path_a)) * ImClamp(blend, 0.0f, 1.0f);
	iam_path_draw_detail::draw(dl, t0, t1, len, col, thickness, IAM_VEC2_TO_IM(offset), segments,
		[&](float t) { return iam_path_morph_impl(path_a, path_b, t, blend, opts); });
}

// ----------------------------------------------------
// Layout animation
// ----------------------------------------------------
bool iam_layout(ImGuiID id, iam_vec2 target_pos, iam_vec2 target_size, iam_vec2* out_pos, iam_vec2* out_size, float dt, iam_layout_opts const& opts) {
	static ImGuiID const ch_pos = ImHashStr("##iam_layout_pos");
	static ImGuiID const ch_size = ImHashStr("##iam_layout_size");
	bool animating = false;
	iam_vec2 pos = target_pos, size = target_size;
	if (opts.animate_position) {
		pos = iam_spring_vec2(id, ch_pos, target_pos, opts.spring, dt, target_pos);  // init = target: first sight snaps
		animating |= iam_tween_is_active(id, ch_pos);
	}
	if (opts.animate_size) {
		size = iam_spring_vec2(id, ch_size, target_size, opts.spring, dt, target_size);
		animating |= iam_tween_is_active(id, ch_size);
	}
	if (out_pos) *out_pos = pos;
	if (out_size) *out_size = size;
	return animating;
}

// ----------------------------------------------------
// Scroll-linked progress and triggers
// ----------------------------------------------------
namespace iam_scroll_link_detail {
	static ImGuiStorage g_trigger_state;  // id -> 0 unknown, 1 hidden, 2 visible

	// Vertical extent of the visible area of the current window and of the last item, in screen space
	static void ranges(float* view_top, float* view_bottom, float* item_top, float* item_bottom) {
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		*view_top = window->InnerRect.Min.y;
		*view_bottom = window->InnerRect.Max.y;
		*item_top = ImGui::GetItemRectMin().y;
		*item_bottom = ImGui::GetItemRectMax().y;
	}

	static float visible_fraction() {
		float vt, vb, it, ib;
		ranges(&vt, &vb, &it, &ib);
		float h = ib - it;
		float overlap = ImMin(ib, vb) - ImMax(it, vt);
		if (h <= 0.0f) return (it >= vt && it <= vb) ? 1.0f : 0.0f;
		return ImClamp(overlap / h, 0.0f, 1.0f);
	}
} // namespace iam_scroll_link_detail

float iam_scroll_window_progress(int axis) {
	float mx = axis == 0 ? ImGui::GetScrollMaxX() : ImGui::GetScrollMaxY();
	if (mx <= 0.0f) return 0.0f;
	return ImClamp((axis == 0 ? ImGui::GetScrollX() : ImGui::GetScrollY()) / mx, 0.0f, 1.0f);
}

float iam_scroll_item_progress() {
	float vt, vb, it, ib;
	iam_scroll_link_detail::ranges(&vt, &vb, &it, &ib);
	float travel = (vb - vt) + (ib - it);
	if (travel <= 0.0f) return 0.0f;
	return ImClamp((vb - it) / travel, 0.0f, 1.0f);
}

bool iam_scroll_item_visible(float min_fraction) {
	float f = iam_scroll_link_detail::visible_fraction();
	return min_fraction <= 0.0f ? f > 0.0f : f >= min_fraction;
}

int iam_scroll_trigger(ImGuiID id, float min_fraction, bool* out_visible) {
	bool visible = iam_scroll_item_visible(min_fraction);
	if (out_visible) *out_visible = visible;
	int prev = iam_scroll_link_detail::g_trigger_state.GetInt(id, 0);
	int now = visible ? 2 : 1;
	iam_scroll_link_detail::g_trigger_state.SetInt(id, now);
	if (now == prev) return iam_scroll_none;
	if (visible) return iam_scroll_entered;       // includes first sight while visible
	return prev == 2 ? iam_scroll_left : iam_scroll_none;
}

// ----------------------------------------------------
// Inertia
// ----------------------------------------------------
namespace iam_inertia_detail {
	enum mode_t { mode_idle = 0, mode_held, mode_coast, mode_spring };
	struct inertia_state {
		float x[2], v[2], target[2];
		int mode;
		unsigned last_seen_frame, last_step_frame;
	};
	static ImPool<inertia_state> g_states;

	static float snap_axis(float value, float grid, iam_vec2 const* points, int count, int axis, int n) {
		float best = value;
		float best_d = FLT_MAX;
		bool any = false;
		if (grid > 0.0f) { best = ImFloor(value / grid + 0.5f) * grid; best_d = ImFabs(best - value); any = true; }
		(void)n;
		if (points && count > 0 && n == 1) {
			for (int i = 0; i < count; i++) {
				ImVec2 p = IAM_VEC2_TO_IM(points[i]);
				float c = axis == 0 ? p.x : p.y;
				float d = ImFabs(c - value);
				if (!any || d < best_d) { best = c; best_d = d; any = true; }
			}
		}
		return best;
	}

	// Snap target for an n-dim projected landing point
	static void snap_target(float const* projected, float* out, int n, iam_inertia_opts const& o) {
		ImVec2 grid = IAM_VEC2_TO_IM(o.snap_grid);
		for (int i = 0; i < n; i++) out[i] = snap_axis(projected[i], i == 0 ? grid.x : grid.y, o.snap_points, o.snap_points_count, i, n);
		if (n == 2 && o.snap_points && o.snap_points_count > 0) {
			// 2D points: nearest point overrides the grid
			float best_d = FLT_MAX;
			for (int i = 0; i < o.snap_points_count; i++) {
				ImVec2 p = IAM_VEC2_TO_IM(o.snap_points[i]);
				float dx = p.x - projected[0], dy = p.y - projected[1];
				float d = dx * dx + dy * dy;
				if (d < best_d) { best_d = d; out[0] = p.x; out[1] = p.y; }
			}
		}
	}

	static bool has_snap(iam_inertia_opts const& o) {
		ImVec2 g = IAM_VEC2_TO_IM(o.snap_grid);
		return g.x > 0.0f || g.y > 0.0f || (o.snap_points && o.snap_points_count > 0);
	}

	static void bounds(iam_inertia_opts const& o, int axis, float* mn, float* mx) {
		ImVec2 a = IAM_VEC2_TO_IM(o.bounds_min), b = IAM_VEC2_TO_IM(o.bounds_max);
		*mn = axis == 0 ? a.x : a.y;
		*mx = axis == 0 ? b.x : b.y;
	}

	static void update(ImGuiID id, float const* pos, bool held, float dt, iam_inertia_opts const& o, int n, float* out) {
		inertia_state* s = g_states.GetByKey(id);
		if (!s) {
			s = g_states.GetOrAddByKey(id);
			for (int i = 0; i < 2; i++) { s->x[i] = i < n ? pos[i] : 0.0f; s->v[i] = 0.0f; s->target[i] = s->x[i]; }
			s->mode = mode_idle;
			s->last_step_frame = iam_detail::g_frame - 1;
		}
		s->last_seen_frame = iam_detail::g_frame;
		bool const step = s->last_step_frame != iam_detail::g_frame;
		s->last_step_frame = iam_detail::g_frame;
		float const h = ImClamp(dt * iam_detail::g_time_scale, 0.0f, 0.1f);
		float const tau = ImMax(o.decay_time, 0.01f);

		if (held) {
			if (step) {
				for (int i = 0; i < n; i++) {
					float d = pos[i] - s->x[i];
					float nx = s->x[i] + d;
					if (o.use_bounds) {
						float mn, mx;
						bounds(o, i, &mn, &mx);
						if (d > 0.0f && nx > mx) { float inside = ImMax(0.0f, mx - s->x[i]); nx = s->x[i] + inside + (d - inside) * o.rubber_band; }
						if (d < 0.0f && nx < mn) { float inside = ImMin(0.0f, mn - s->x[i]); nx = s->x[i] + inside + (d - inside) * o.rubber_band; }
					}
					float inst_v = h > 0.0f ? (nx - s->x[i]) / h : 0.0f;
					float a = h > 0.0f ? 1.0f - expf(-h / 0.04f) : 0.0f;  // smooth the velocity estimate
					if (s->mode != mode_held) s->v[i] = inst_v;
					else s->v[i] += (inst_v - s->v[i]) * a;
					s->x[i] = nx;
				}
			}
			s->mode = mode_held;
		} else {
			if (s->mode == mode_held) {
				// Release
				if (has_snap(o)) {
					float projected[2], tgt[2];
					for (int i = 0; i < n; i++) projected[i] = s->x[i] + s->v[i] * tau;
					snap_target(projected, tgt, n, o);
					for (int i = 0; i < n; i++) {
						if (o.use_bounds) { float mn, mx; bounds(o, i, &mn, &mx); tgt[i] = ImClamp(tgt[i], mn, mx); }
						s->target[i] = tgt[i];
					}
					s->mode = mode_spring;
				} else {
					s->mode = mode_coast;
				}
			} else if (s->mode == mode_idle) {
				for (int i = 0; i < n; i++) s->x[i] = pos[i];  // follow programmatic changes
			}
			if (iam_detail::g_reduced_motion && (s->mode == mode_coast || s->mode == mode_spring)) {
				// Reduced motion: no glide; land on the snap target (or stay, clamped to bounds)
				for (int i = 0; i < n; i++) {
					if (s->mode == mode_spring) s->x[i] = s->target[i];
					else if (o.use_bounds) { float mn, mx; bounds(o, i, &mn, &mx); s->x[i] = ImClamp(s->x[i], mn, mx); }
					s->v[i] = 0.0f;
				}
				s->mode = mode_idle;
			}
			if (step && h > 0.0f && s->mode == mode_coast) {
				float k = expf(-h / tau);
				bool slow = true, outside = false;
				for (int i = 0; i < n; i++) {
					s->x[i] += s->v[i] * tau * (1.0f - k);
					s->v[i] *= k;
					slow &= ImFabs(s->v[i]) < 2.0f;
					if (o.use_bounds) {
						float mn, mx;
						bounds(o, i, &mn, &mx);
						s->target[i] = ImClamp(s->x[i], mn, mx);
						outside |= s->target[i] != s->x[i];
					}
				}
				if (outside) s->mode = mode_spring;  // bounce back with the current velocity
				else if (slow) { s->mode = mode_idle; for (int i = 0; i < n; i++) s->v[i] = 0.0f; }
			}
			if (step && h > 0.0f && s->mode == mode_spring) {
				iam_spring_params const& sp = o.spring;
				float const m = ImMax(sp.mass, 1e-4f), kk = ImMax(sp.stiffness, 0.0f), c = ImMax(sp.damping, 0.0f);
				int const substeps = ImClamp((int)ImCeil(h / (1.0f / 240.0f)), 1, 32);
				float const sh = h / substeps;
				for (int it = 0; it < substeps; it++)
					for (int i = 0; i < n; i++) {
						float a = (-kk * (s->x[i] - s->target[i]) - c * s->v[i]) / m;
						s->v[i] += a * sh;
						s->x[i] += s->v[i] * sh;
					}
				bool settled = true;
				for (int i = 0; i < n; i++) settled &= ImFabs(s->x[i] - s->target[i]) < 0.05f && ImFabs(s->v[i]) < 1.0f;
				if (settled) { for (int i = 0; i < n; i++) { s->x[i] = s->target[i]; s->v[i] = 0.0f; } s->mode = mode_idle; }
			}
		}
		for (int i = 0; i < n; i++) out[i] = s->x[i];
	}

	static void gc(unsigned max_age) {
		for (int i = 0; i < g_states.GetMapSize(); ++i) {
			if (inertia_state* s = g_states.TryGetMapData(i)) {
				if (iam_detail::g_frame - s->last_seen_frame > max_age) {
					ImGuiID k = g_states.Map.Data[i].key;
					g_states.Remove(k, g_states.GetIndex(s));
				}
			}
		}
	}
} // namespace iam_inertia_detail

static void iam_inertia_gc(unsigned max_age) { iam_inertia_detail::gc(max_age); }
static void iam_inertia_clear() { iam_inertia_detail::g_states.Clear(); }

float iam_inertia_float(ImGuiID id, float pos, bool held, float dt, iam_inertia_opts const& opts) {
	float out[1];
	iam_inertia_detail::update(id, &pos, held, dt, opts, 1, out);
	return out[0];
}

iam_vec2 iam_inertia_vec2(ImGuiID id, iam_vec2 pos_in, bool held, float dt, iam_inertia_opts const& opts) {
	ImVec2 p = IAM_VEC2_TO_IM(pos_in);
	float pos[2] = { p.x, p.y }, out[2];
	iam_inertia_detail::update(id, pos, held, dt, opts, 2, out);
	return IAM_VEC2_FROM_IM(ImVec2(out[0], out[1]));
}

bool iam_inertia_is_moving(ImGuiID id) {
	iam_inertia_detail::inertia_state* s = iam_inertia_detail::g_states.GetByKey(id);
	return s && (s->mode == iam_inertia_detail::mode_coast || s->mode == iam_inertia_detail::mode_spring);
}

void iam_inertia_set(ImGuiID id, iam_vec2 pos_in) {
	iam_inertia_detail::inertia_state* s = iam_inertia_detail::g_states.GetOrAddByKey(id);
	ImVec2 p = IAM_VEC2_TO_IM(pos_in);
	s->x[0] = s->target[0] = p.x; s->x[1] = s->target[1] = p.y;
	s->v[0] = s->v[1] = 0.0f;
	s->mode = iam_inertia_detail::mode_idle;
	s->last_seen_frame = iam_detail::g_frame;
	s->last_step_frame = iam_detail::g_frame - 1;
}

// ----------------------------------------------------
// Velocity tracking
// ----------------------------------------------------
namespace iam_track_velocity_detail {
	struct track_state {
		float prev[2], vel[2];
		unsigned last_seen_frame, last_step_frame;
	};
	static ImPool<track_state> g_states;

	static void update(ImGuiID id, float const* value, int n, float dt, float smoothing, float* out) {
		ImGuiID k = ImHashData(&n, sizeof(n), id);
		track_state* s = g_states.GetByKey(k);
		if (!s) {
			s = g_states.GetOrAddByKey(k);
			for (int i = 0; i < 2; i++) { s->prev[i] = i < n ? value[i] : 0.0f; s->vel[i] = 0.0f; }
			s->last_step_frame = iam_detail::g_frame;
		}
		s->last_seen_frame = iam_detail::g_frame;
		if (s->last_step_frame != iam_detail::g_frame && dt > 0.0f) {
			s->last_step_frame = iam_detail::g_frame;
			float a = smoothing > 0.0f ? 1.0f - expf(-dt / smoothing) : 1.0f;
			for (int i = 0; i < n; i++) {
				float inst = (value[i] - s->prev[i]) / dt;
				s->vel[i] += (inst - s->vel[i]) * a;
				s->prev[i] = value[i];
			}
		}
		for (int i = 0; i < n; i++) out[i] = s->vel[i];
	}

	static void gc(unsigned max_age) {
		for (int i = 0; i < g_states.GetMapSize(); ++i) {
			if (track_state* s = g_states.TryGetMapData(i)) {
				if (iam_detail::g_frame - s->last_seen_frame > max_age) {
					ImGuiID k = g_states.Map.Data[i].key;
					g_states.Remove(k, g_states.GetIndex(s));
				}
			}
		}
	}
} // namespace iam_track_velocity_detail

static void iam_track_velocity_gc(unsigned max_age) { iam_track_velocity_detail::gc(max_age); }
static void iam_track_velocity_clear() { iam_track_velocity_detail::g_states.Clear(); }

float iam_track_velocity_float(ImGuiID id, float value, float dt, float smoothing) {
	float out;
	iam_track_velocity_detail::update(id, &value, 1, dt, smoothing, &out);
	return out;
}

iam_vec2 iam_track_velocity_vec2(ImGuiID id, iam_vec2 value_in, float dt, float smoothing) {
	ImVec2 v = IAM_VEC2_TO_IM(value_in);
	float in[2] = { v.x, v.y }, out[2];
	iam_track_velocity_detail::update(id, in, 2, dt, smoothing, out);
	return IAM_VEC2_FROM_IM(ImVec2(out[0], out[1]));
}

// ----------------------------------------------------
// SVG path import
// ----------------------------------------------------
namespace iam_svg_detail {
	struct parser {
		char const* p;
		bool ok;
		void skip_sep() { while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') p++; }
		bool at_number() {
			skip_sep();
			return (*p >= '0' && *p <= '9') || *p == '-' || *p == '+' || *p == '.';
		}
		float number() {
			skip_sep();
			char const* s = p;
			if (*p == '+' || *p == '-') p++;
			bool digits = false, dot = false;
			while ((*p >= '0' && *p <= '9') || (*p == '.' && !dot)) { if (*p == '.') dot = true; else digits = true; p++; }
			if (digits && (*p == 'e' || *p == 'E')) {
				char const* e = p + 1;
				if (*e == '+' || *e == '-') e++;
				if (*e >= '0' && *e <= '9') { p = e; while (*p >= '0' && *p <= '9') p++; }
			}
			if (!digits) { ok = false; return 0.0f; }
			return (float)atof(s);  // atof stops at the right place; we already advanced p
		}
		float flag() {  // arc flags may be written without separators ("a1 1 0 01 1 1")
			skip_sep();
			if (*p == '0' || *p == '1') { float f = (float)(*p - '0'); p++; return f; }
			ok = false;
			return 0.0f;
		}
	};

	// Parsed segment (path coordinates, before scale/offset)
	struct op { int type; ImVec2 p[3]; };  // 0 line, 1 quadratic (c, e), 2 cubic (c1, c2, e), 3 close
	static void push(ImVector<op>& ops, int type, ImVec2 a, ImVec2 b = ImVec2(0, 0), ImVec2 c = ImVec2(0, 0)) {
		op o; o.type = type; o.p[0] = a; o.p[1] = b; o.p[2] = c; ops.push_back(o);
	}

	static float arc_angle(float ux, float uy, float vx, float vy) {
		float d = ux * vx + uy * vy, l = ImSqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
		float a = acosf(ImClamp(l > 0.0f ? d / l : 1.0f, -1.0f, 1.0f));
		return (ux * vy - uy * vx < 0.0f) ? -a : a;
	}

	// SVG arc (endpoint parameterization) as cubic beziers
	static void arc_to(ImVector<op>& ops, ImVec2 p0, float rx, float ry, float angle_deg, bool large, bool sweep, ImVec2 p1) {
		if (rx == 0.0f || ry == 0.0f || (p0.x == p1.x && p0.y == p1.y)) { push(ops, 0, p1); return; }
		rx = ImFabs(rx); ry = ImFabs(ry);
		float phi = angle_deg * IAM_PI / 180.0f, cp = ImCos(phi), sp = ImSin(phi);
		float dx = (p0.x - p1.x) * 0.5f, dy = (p0.y - p1.y) * 0.5f;
		float x1 = cp * dx + sp * dy, y1 = -sp * dx + cp * dy;
		float lambda = (x1 * x1) / (rx * rx) + (y1 * y1) / (ry * ry);
		if (lambda > 1.0f) { float sq = ImSqrt(lambda); rx *= sq; ry *= sq; }
		float num = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1;
		float den = rx * rx * y1 * y1 + ry * ry * x1 * x1;
		float coef = (den > 0.0f && num > 0.0f) ? ImSqrt(num / den) : 0.0f;
		if (large == sweep) coef = -coef;
		float cx1 = coef * rx * y1 / ry, cy1 = -coef * ry * x1 / rx;
		float cx = cp * cx1 - sp * cy1 + (p0.x + p1.x) * 0.5f, cy = sp * cx1 + cp * cy1 + (p0.y + p1.y) * 0.5f;
		float th1 = arc_angle(1.0f, 0.0f, (x1 - cx1) / rx, (y1 - cy1) / ry);
		float dth = arc_angle((x1 - cx1) / rx, (y1 - cy1) / ry, (-x1 - cx1) / rx, (-y1 - cy1) / ry);
		if (!sweep && dth > 0.0f) dth -= IAM_2PI;
		else if (sweep && dth < 0.0f) dth += IAM_2PI;
		int segs = ImMax(1, (int)ImCeil(ImFabs(dth) / (IAM_PI * 0.5f) - 1e-4f));
		float delta = dth / segs;
		float t = 4.0f / 3.0f * tanf(delta * 0.25f);
		float a0 = th1;
		for (int i = 0; i < segs; i++) {
			float a1 = a0 + delta;
			float c0 = ImCos(a0), s0 = ImSin(a0), c1 = ImCos(a1), s1 = ImSin(a1);
			ImVec2 e0(cx + rx * c0 * cp - ry * s0 * sp, cy + rx * c0 * sp + ry * s0 * cp);
			ImVec2 e1 = (i == segs - 1) ? p1 : ImVec2(cx + rx * c1 * cp - ry * s1 * sp, cy + rx * c1 * sp + ry * s1 * cp);
			ImVec2 d0(-rx * s0 * cp - ry * c0 * sp, -rx * s0 * sp + ry * c0 * cp), d1(-rx * s1 * cp - ry * c1 * sp, -rx * s1 * sp + ry * c1 * cp);
			push(ops, 2, ImVec2(e0.x + t * d0.x, e0.y + t * d0.y), ImVec2(e1.x - t * d1.x, e1.y - t * d1.y), e1);
			a0 = a1;
		}
	}

	// Parse into start + ops; false on error
	static bool parse(char const* d, ImVec2* out_start, ImVector<op>& ops) {
		parser ps = { d, true };
		bool begun = false;
		ImVec2 cur(0, 0), start(0, 0), last_ctrl(0, 0);
		char prev = 0, cmd = 0;
		while (true) {
			ps.skip_sep();
			if (!*ps.p) break;
			if ((*ps.p >= 'A' && *ps.p <= 'Z') || (*ps.p >= 'a' && *ps.p <= 'z')) cmd = *ps.p++;
			else if (!cmd) return false;  // number before any command
			bool rel = (cmd >= 'a' && cmd <= 'z');
			char C = (char)(rel ? cmd - 32 : cmd);
			ImVec2 base = rel ? cur : ImVec2(0, 0);
			if (C == 'Z') {
				if (!begun) return false;
				push(ops, 3, start);
				cur = start; prev = 'Z'; cmd = 0;
				continue;
			}
			if (!ps.at_number()) return false;
			if (C != 'M' && !begun) return false;
			switch (C) {
				case 'M': {
					float x = ps.number(), y = ps.number();
					ImVec2 p(base.x + x, base.y + y);
					if (!begun) { *out_start = p; begun = true; }
					else push(ops, 0, p);  // new subpath: joined with a line
					cur = start = p;
					cmd = rel ? 'l' : 'L';  // further pairs are implicit linetos
					break;
				}
				case 'L': { float x = ps.number(), y = ps.number(); cur = ImVec2(base.x + x, base.y + y); push(ops, 0, cur); break; }
				case 'H': { float x = ps.number(); cur.x = base.x + x; push(ops, 0, cur); break; }
				case 'V': { float y = ps.number(); cur.y = (rel ? cur.y : 0.0f) + y; push(ops, 0, cur); break; }
				case 'C': {
					float x1 = ps.number(), y1 = ps.number(), x2 = ps.number(), y2 = ps.number(), x = ps.number(), y = ps.number();
					ImVec2 c2(base.x + x2, base.y + y2), e(base.x + x, base.y + y);
					push(ops, 2, ImVec2(base.x + x1, base.y + y1), c2, e);
					last_ctrl = c2; cur = e;
					break;
				}
				case 'S': {
					float x2 = ps.number(), y2 = ps.number(), x = ps.number(), y = ps.number();
					ImVec2 c1 = (prev == 'C' || prev == 'S') ? ImVec2(2 * cur.x - last_ctrl.x, 2 * cur.y - last_ctrl.y) : cur;
					ImVec2 c2(base.x + x2, base.y + y2), e(base.x + x, base.y + y);
					push(ops, 2, c1, c2, e);
					last_ctrl = c2; cur = e;
					break;
				}
				case 'Q': {
					float x1 = ps.number(), y1 = ps.number(), x = ps.number(), y = ps.number();
					ImVec2 c(base.x + x1, base.y + y1), e(base.x + x, base.y + y);
					push(ops, 1, c, e);
					last_ctrl = c; cur = e;
					break;
				}
				case 'T': {
					float x = ps.number(), y = ps.number();
					ImVec2 c = (prev == 'Q' || prev == 'T') ? ImVec2(2 * cur.x - last_ctrl.x, 2 * cur.y - last_ctrl.y) : cur;
					ImVec2 e(base.x + x, base.y + y);
					push(ops, 1, c, e);
					last_ctrl = c; cur = e;
					break;
				}
				case 'A': {
					float rx = ps.number(), ry = ps.number(), rot = ps.number();
					bool large = ps.flag() != 0.0f, sweep = ps.flag() != 0.0f;
					float x = ps.number(), y = ps.number();
					ImVec2 e(base.x + x, base.y + y);
					arc_to(ops, cur, rx, ry, rot, large, sweep, e);
					cur = e;
					break;
				}
				default: return false;
			}
			if (!ps.ok) return false;
			prev = C;
		}
		return begun;
	}
} // namespace iam_svg_detail

bool iam_path_from_svg(ImGuiID path_id, char const* svg_d, float scale, iam_vec2 offset_in) {
	using namespace iam_svg_detail;
	if (!svg_d) return false;
	ImVec2 start(0, 0);
	ImVector<op> ops;
	if (!parse(svg_d, &start, ops)) return false;
	ImVec2 const off = IAM_VEC2_TO_IM(offset_in);
	#define IAM_SVG_TX(q) IAM_VEC2_FROM_IM(ImVec2((q).x * scale + off.x, (q).y * scale + off.y))
	iam_path b = iam_path::begin(path_id, IAM_SVG_TX(start));
	for (int i = 0; i < ops.Size; i++) {
		op const& o = ops[i];
		switch (o.type) {
			case 0: b.line_to(IAM_SVG_TX(o.p[0])); break;
			case 1: b.quadratic_to(IAM_SVG_TX(o.p[0]), IAM_SVG_TX(o.p[1])); break;
			case 2: b.cubic_to(IAM_SVG_TX(o.p[0]), IAM_SVG_TX(o.p[1]), IAM_SVG_TX(o.p[2])); break;
			case 3: b.close(); break;
		}
	}
	#undef IAM_SVG_TX
	b.end();
	return true;
}

// ----------------------------------------------------
// Text scramble
// ----------------------------------------------------
int iam_text_scramble(char* out, int out_size, char const* text, float progress, iam_text_scramble_opts const& opts) {
	if (!out || out_size <= 0) return 0;
	out[0] = 0;
	if (!text) return 0;
	char const* charset = (opts.charset && opts.charset[0]) ? opts.charset : "#";
	int const charset_len = (int)strlen(charset);
	int count = 0;  // codepoints
	for (char const* c = text; *c; ) { unsigned int cp; c += ImTextCharFromUtf8(&cp, c, nullptr); count++; }
	unsigned const tick = (unsigned)(iam_detail::g_global_time * (opts.change_rate > 0.0f ? opts.change_rate : 0.0f));
	int len = 0, i = 0;
	for (char const* c = text; *c && len < out_size - 1; i++) {
		unsigned int cp;
		int bytes = ImTextCharFromUtf8(&cp, c, nullptr);
		float threshold;
		if (opts.random_order) {
			unsigned h = (unsigned)i * 0x9E3779B9u ^ (opts.seed + 0x7F4A7C15u);  // murmur3 finalizer: uniform for sequential i
			h ^= h >> 16; h *= 0x85EBCA6Bu; h ^= h >> 13; h *= 0xC2B2AE35u; h ^= h >> 16;
			threshold = (float)(h & 0xFFFFFF) / (float)0x1000000;
		} else {
			threshold = (float)i / (float)(count > 0 ? count : 1);
		}
		bool keep = progress >= 1.0f || (progress > threshold) || (opts.keep_spaces && (cp == ' ' || cp == '\t' || cp == '\n'));
		if (keep) {
			if (len + bytes >= out_size) break;
			memcpy(out + len, c, bytes);
			len += bytes;
		} else {
			unsigned key[3] = { (unsigned)i, tick, opts.seed };
			out[len++] = charset[ImHashData(key, sizeof(key), 0) % (unsigned)charset_len];
		}
		c += bytes;
	}
	out[len] = 0;
	return len;
}

// ----------------------------------------------------
// State-driven clip blending
// ----------------------------------------------------
namespace iam_state_detail {
	struct layer {
		ImGuiID clip, inst;
		float weight;       // current weight, moves toward target
		float target;       // 1 for the current state, 0 for states fading out
	};
	struct machine {
		ImGuiID current;
		float crossfade;
		double last_time;
		unsigned last_seen_frame;
		ImVector<layer> layers;
	};
	static ImPool<machine> g_machines;

	static ImGuiID layer_instance(ImGuiID machine_id, ImGuiID clip_id) { return ImHashData(&clip_id, sizeof(clip_id), machine_id ^ 0x5157A7E5u); }

	static void gc(unsigned max_age) {
		for (int i = 0; i < g_machines.GetMapSize(); ++i) {
			if (machine* m = g_machines.TryGetMapData(i)) {
				if (iam_detail::g_frame - m->last_seen_frame > max_age) {
					for (int l = 0; l < m->layers.Size; l++) iam_instance(m->layers[l].inst).destroy();
					m->layers.clear();
					ImGuiID k = g_machines.Map.Data[i].key;
					g_machines.Remove(k, g_machines.GetIndex(m));
				}
			}
		}
	}

	// Total weight of layers that have the channel (via has(inst)), and accumulate weighted values
	static float total_weight(machine const* m) {
		float w = 0.0f;
		for (int l = 0; l < m->layers.Size; l++) w += m->layers[l].weight;
		return w;
	}
} // namespace iam_state_detail

static void iam_state_gc(unsigned max_age) { iam_state_detail::gc(max_age); }

void iam_state_play(ImGuiID machine_id, ImGuiID state_clip_id, float crossfade) {
	using namespace iam_state_detail;
	machine* m = g_machines.GetByKey(machine_id);
	bool const first = (m == nullptr);
	if (!m) {
		m = g_machines.GetOrAddByKey(machine_id);
		m->current = 0;
		m->crossfade = 0.0f;
		m->last_time = iam_detail::g_global_time;
	}
	m->last_seen_frame = iam_detail::g_frame;
	m->crossfade = iam_detail::motion_dur(ImMax(crossfade, 0.0f));

	// Advance weights (once per frame: time-based)
	float dt = (float)(iam_detail::g_global_time - m->last_time);
	m->last_time = iam_detail::g_global_time;
	if (dt > 0.0f) {
		float step = m->crossfade > 1e-5f ? dt / m->crossfade : 1.0f;
		for (int l = 0; l < m->layers.Size; l++) {
			layer& L = m->layers[l];
			L.weight = (L.target > L.weight) ? ImMin(L.weight + step, L.target) : ImMax(L.weight - step, L.target);
		}
	}

	// State change
	if (state_clip_id != m->current) {
		m->current = state_clip_id;
		int found = -1;
		for (int l = 0; l < m->layers.Size; l++) {
			if (m->layers[l].clip == state_clip_id) found = l;
			m->layers[l].target = 0.0f;
		}
		if (found >= 0) {
			m->layers[found].target = 1.0f;  // coming back to a state still fading out: keep it playing
		} else if (state_clip_id != 0) {
			layer L;
			L.clip = state_clip_id;
			L.inst = layer_instance(machine_id, state_clip_id);
			L.weight = (first || m->crossfade <= 1e-5f || m->layers.Size == 0) ? 1.0f : 0.0f;
			L.target = 1.0f;
			iam_play(state_clip_id, L.inst);
			m->layers.push_back(L);
		}
		if (m->crossfade <= 1e-5f)
			for (int l = 0; l < m->layers.Size; l++) m->layers[l].weight = m->layers[l].target;
	}

	// Drop layers that faded out
	for (int l = m->layers.Size - 1; l >= 0; l--) {
		if (m->layers[l].target == 0.0f && m->layers[l].weight <= 0.0f) {
			iam_instance(m->layers[l].inst).destroy();
			m->layers.erase(m->layers.Data + l);
		}
	}
}

ImGuiID iam_state_current(ImGuiID machine_id) {
	iam_state_detail::machine* m = iam_state_detail::g_machines.GetByKey(machine_id);
	return m ? m->current : 0;
}

float iam_state_blend(ImGuiID machine_id) {
	using namespace iam_state_detail;
	machine* m = g_machines.GetByKey(machine_id);
	if (!m) return 1.0f;
	float total = total_weight(m);
	for (int l = 0; l < m->layers.Size; l++)
		if (m->layers[l].clip == m->current) return total > 0.0f ? m->layers[l].weight / total : 1.0f;
	return 1.0f;
}

iam_instance iam_state_instance(ImGuiID machine_id, ImGuiID state_clip_id) {
	iam_state_detail::machine* m = iam_state_detail::g_machines.GetByKey(machine_id);
	if (!m) return iam_instance(0);
	for (int l = 0; l < m->layers.Size; l++)
		if (m->layers[l].clip == state_clip_id) return iam_instance(m->layers[l].inst);
	return iam_instance(0);
}

// Weighted average over the layers that provide the channel
#define IAM_STATE_BLEND(TYPE, N, GET, TOARR, FROMARR)                                                   \
	using namespace iam_state_detail;                                                                   \
	machine* m = g_machines.GetByKey(machine_id);                                                       \
	if (!m || !out) return false;                                                                       \
	float acc[4] = { 0, 0, 0, 0 }, wsum = 0.0f;                                                         \
	for (int l = 0; l < m->layers.Size; l++) {                                                          \
		float w = m->layers[l].weight;                                                                   \
		if (w <= 0.0f) continue;                                                                         \
		TYPE v;                                                                                          \
		iam_instance inst(m->layers[l].inst);                                                            \
		if (!(GET)) continue;                                                                            \
		float a[4] = { 0, 0, 0, 0 };                                                                     \
		TOARR;                                                                                           \
		for (int i = 0; i < N; i++) acc[i] += a[i] * w;                                                  \
		wsum += w;                                                                                       \
	}                                                                                                   \
	if (wsum <= 0.0f) return false;                                                                     \
	for (int i = 0; i < N; i++) acc[i] /= wsum;                                                         \
	FROMARR;                                                                                            \
	return true;

bool iam_state_get_float(ImGuiID machine_id, ImGuiID channel, float* out) {
	IAM_STATE_BLEND(float, 1, inst.get_float(channel, &v), a[0] = v, *out = acc[0])
}

bool iam_state_get_int(ImGuiID machine_id, ImGuiID channel, int* out) {
	IAM_STATE_BLEND(int, 1, inst.get_int(channel, &v), a[0] = (float)v, *out = (int)ImFloor(acc[0] + 0.5f))
}

bool iam_state_get_vec2(ImGuiID machine_id, ImGuiID channel, iam_vec2* out) {
	IAM_STATE_BLEND(iam_vec2, 2, inst.get_vec2(channel, &v),
		ImVec2 x = IAM_VEC2_TO_IM(v); a[0] = x.x; a[1] = x.y,
		*out = IAM_VEC2_FROM_IM(ImVec2(acc[0], acc[1])))
}

bool iam_state_get_vec4(ImGuiID machine_id, ImGuiID channel, iam_vec4* out) {
	IAM_STATE_BLEND(iam_vec4, 4, inst.get_vec4(channel, &v),
		ImVec4 x = IAM_VEC4_TO_IM(v); a[0] = x.x; a[1] = x.y; a[2] = x.z; a[3] = x.w,
		*out = IAM_VEC4_FROM_IM(ImVec4(acc[0], acc[1], acc[2], acc[3])))
}

bool iam_state_get_color(ImGuiID machine_id, ImGuiID channel, iam_vec4* out, int color_space) {
	// Blend in color_space (hue spaces blend hue linearly; oklab recommended)
	IAM_STATE_BLEND(iam_vec4, 4, inst.get_color(channel, &v, color_space),
		ImVec4 x = iam_detail::color::to_space(IAM_VEC4_TO_IM(v), color_space); a[0] = x.x; a[1] = x.y; a[2] = x.z; a[3] = x.w,
		*out = IAM_VEC4_FROM_IM(iam_detail::color::from_space(ImVec4(acc[0], acc[1], acc[2], acc[3]), color_space)))
}
#undef IAM_STATE_BLEND

// ============================================================
// CLIP-BASED ANIMATION SYSTEM IMPLEMENTATION
// ============================================================

namespace iam_clip_detail {

static unsigned g_keyframe_uid = 0;  // Source of keyframe::uid

// keyframe data - using plain floats to avoid union constructor issues
struct keyframe {
	unsigned	uid;		// Unique per keyframe (stable across sorting), used by the clip editor selection
	ImGuiID		channel;
	float		time;
	int			type;		// iam_channel_type
	int			ease_type;	// iam_ease_type
	int			color_space;// iam_color_space (for iam_chan_color)
	float		bezier[4];
	bool		has_bezier;
	bool		is_spring;
	iam_spring_params spring;
	float		value[4];	// f=value[0], v2=(value[0],value[1]), v4=(value[0..3]), i=*(int*)&value[0], color=(value[0..3])
	float		value_ext[4];// Extended storage for relative types (px_bias for vec4_rel/color_rel)

	// Variation data for repeat with variation feature
	bool				has_variation;
	iam_variation_float	var_float;
	iam_variation_int	var_int;
	iam_variation_vec2	var_vec2;
	iam_variation_vec4	var_vec4;
	iam_variation_color	var_color;

	keyframe() : uid(++g_keyframe_uid), channel(0), time(0), type(0), ease_type(iam_ease_linear), color_space(iam_col_oklab), has_bezier(false), is_spring(false), has_variation(false) {
		bezier[0] = bezier[1] = bezier[2] = bezier[3] = 0;
		spring = { 1.0f, 120.0f, 20.0f, 0.0f };
		value[0] = value[1] = value[2] = value[3] = 0;
		value_ext[0] = value_ext[1] = value_ext[2] = value_ext[3] = 0;
		memset(&var_float, 0, sizeof(var_float));
		memset(&var_int, 0, sizeof(var_int));
		memset(&var_vec2, 0, sizeof(var_vec2));
		memset(&var_vec4, 0, sizeof(var_vec4));
		memset(&var_color, 0, sizeof(var_color));
	}

	void set_float(float f) { value[0] = f; }
	float get_float() const { return value[0]; }
	void set_vec2(ImVec2 v) { value[0] = v.x; value[1] = v.y; }
	ImVec2 get_vec2() const { return ImVec2(value[0], value[1]); }
	void set_vec4(ImVec4 v) { value[0] = v.x; value[1] = v.y; value[2] = v.z; value[3] = v.w; }
	ImVec4 get_vec4() const { return ImVec4(value[0], value[1], value[2], value[3]); }
	void set_int(int i) { memcpy(&value[0], &i, sizeof(int)); }
	int get_int() const { int i; memcpy(&i, &value[0], sizeof(int)); return i; }
	void set_color(ImVec4 c) { value[0] = c.x; value[1] = c.y; value[2] = c.z; value[3] = c.w; }
	ImVec4 get_color() const { return ImVec4(value[0], value[1], value[2], value[3]); }

	// Relative (anchor) value accessors: float uses value[0]=percent, value[1]=px_bias
	void set_float_rel(float percent, float px_bias) { value[0] = percent; value[1] = px_bias; }
	float get_float_rel_percent() const { return value[0]; }
	float get_float_rel_px_bias() const { return value[1]; }

	// Relative vec2: value[0,1]=percent.xy, value[2,3]=px_bias.xy
	void set_vec2_rel(ImVec2 percent, ImVec2 px_bias) { value[0] = percent.x; value[1] = percent.y; value[2] = px_bias.x; value[3] = px_bias.y; }
	ImVec2 get_vec2_rel_percent() const { return ImVec2(value[0], value[1]); }
	ImVec2 get_vec2_rel_px_bias() const { return ImVec2(value[2], value[3]); }

	// Relative vec4: value[0..3]=percent.xyzw, value_ext[0..3]=px_bias.xyzw
	void set_vec4_rel(ImVec4 percent, ImVec4 px_bias) {
		value[0] = percent.x; value[1] = percent.y; value[2] = percent.z; value[3] = percent.w;
		value_ext[0] = px_bias.x; value_ext[1] = px_bias.y; value_ext[2] = px_bias.z; value_ext[3] = px_bias.w;
	}
	ImVec4 get_vec4_rel_percent() const { return ImVec4(value[0], value[1], value[2], value[3]); }
	ImVec4 get_vec4_rel_px_bias() const { return ImVec4(value_ext[0], value_ext[1], value_ext[2], value_ext[3]); }

	// Relative color: value[0..3]=percent.rgba, value_ext[0..3]=px_bias.rgba
	void set_color_rel(ImVec4 percent, ImVec4 px_bias) {
		value[0] = percent.x; value[1] = percent.y; value[2] = percent.z; value[3] = percent.w;
		value_ext[0] = px_bias.x; value_ext[1] = px_bias.y; value_ext[2] = px_bias.z; value_ext[3] = px_bias.w;
	}
	ImVec4 get_color_rel_percent() const { return ImVec4(value[0], value[1], value[2], value[3]); }
	ImVec4 get_color_rel_px_bias() const { return ImVec4(value_ext[0], value_ext[1], value_ext[2], value_ext[3]); }
};

// iam_track: sorted keyframes for a single channel
struct iam_track {
	ImGuiID				channel;
	int					type;
	int					color_space;  // For iam_chan_color tracks
	ImVector<keyframe>	keys;

	// Anchor-relative support (for key_float_rel, key_vec2_rel)
	bool				is_relative;	// If true, values are percent+px_bias, resolved at get time
	int					anchor_space;	// iam_anchor_space (window_content, window, viewport, etc.)
	int					anchor_axis;	// For float: 0=x, 1=y (ignored for vec2/vec4)

	iam_track() : channel(0), type(0), color_space(iam_col_oklab), is_relative(false), anchor_space(0), anchor_axis(0) {}
};

// Named time on a clip's timeline (iam_clip::label)
struct clip_label {
	ImGuiID	id;
	float	time;
};

// Timeline marker
struct iam_marker {
	float					time;
	ImGuiID					marker_id;
	iam_marker_callback		callback;
	void*					user_data;
	iam_marker() : time(0), marker_id(0), callback(nullptr), user_data(nullptr) {}
};

} // namespace iam_clip_detail

// iam_clip_data definition
struct iam_clip_data {
	ImGuiID					id;
	float					delay;
	float					duration;
	int						loop_count;		// -1 = infinite, 0 = no loop, >0 = repeat count
	int						direction;		// iam_direction
	ImVector<iam_clip_detail::iam_track>	iam_tracks;

	// Timeline markers
	ImVector<iam_clip_detail::iam_marker>	markers;

	// Named times (labels)
	ImVector<iam_clip_detail::clip_label>	labels;

	// Loop delay
	float					loop_delay;		// Delay between loop iterations (seconds)

	// Callbacks
	iam_clip_callback		cb_begin;
	iam_clip_callback		cb_update;
	iam_clip_callback		cb_complete;
	iam_clip_callback		cb_pause;
	iam_loop_callback		cb_loop;
	void*					cb_begin_user;
	void*					cb_update_user;
	void*					cb_complete_user;
	void*					cb_pause_user;
	void*					cb_loop_user;

	// Build-time state
	ImVector<iam_clip_detail::keyframe>	build_keys;

	// Seq/par grouping state (build-time)
	struct group_state {
		float	base_time;		// base time offset for this group
		float	max_time;		// max time seen in this group (for seq_end)
		bool	is_seq;			// true = sequential, false = parallel
	};
	ImVector<group_state>	group_stack;
	float					build_time_offset;	// current time offset for keyframes

	// Stagger state
	int						stagger_count;
	float					stagger_delay;
	float					stagger_center_bias;
	int						stagger_ease;		// iam_ease_type for delay distribution

	// Timing variation per loop iteration
	bool					has_duration_var;
	bool					has_delay_var;
	bool					has_timescale_var;
	iam_variation_float		duration_var;
	iam_variation_float		delay_var;
	iam_variation_float		timescale_var;

	iam_clip_data() : id(0), delay(0), duration(0), loop_count(0), direction(iam_dir_normal),
		loop_delay(0),
		cb_begin(nullptr), cb_update(nullptr), cb_complete(nullptr), cb_pause(nullptr), cb_loop(nullptr),
		cb_begin_user(nullptr), cb_update_user(nullptr), cb_complete_user(nullptr),
		cb_pause_user(nullptr), cb_loop_user(nullptr),
		build_time_offset(0), stagger_count(0), stagger_delay(0), stagger_center_bias(0),
		has_duration_var(false), has_delay_var(false), has_timescale_var(false) {
		memset(&duration_var, 0, sizeof(duration_var));
		memset(&delay_var, 0, sizeof(delay_var));
		memset(&timescale_var, 0, sizeof(timescale_var));
	}
};

// iam_instance_data definition
struct iam_instance_data {
	ImGuiID		inst_id;
	ImGuiID		clip_id;		// Store clip ID instead of pointer to avoid invalidation
	float		time;
	float		time_scale;
	float		weight;
	float		delay_left;
	bool		playing;
	bool		paused;
	bool		begin_called;	// iam_track whether on_begin has been called
	int			dir_sign;
	int			loops_left;
	unsigned	last_seen_frame;

	// Per-channel current values (cached after evaluation)
	ImGuiStorage	values_float;
	ImGuiStorage	values_int;
	// vec2/vec4/color stored as float pairs/quads in separate arrays
	struct vec2_entry { ImGuiID ch; ImVec2 v; };
	struct vec4_entry { ImGuiID ch; ImVec4 v; };
	struct color_entry { ImGuiID ch; ImVec4 v; int color_space; };
	// Relative types need 8 floats: percent(4) + px_bias(4)
	struct vec4_rel_entry { ImGuiID ch; ImVec4 percent; ImVec4 px_bias; };
	struct color_rel_entry { ImGuiID ch; ImVec4 percent; ImVec4 px_bias; int color_space; };
	ImVector<vec2_entry> values_vec2;
	ImVector<vec4_entry> values_vec4;
	ImVector<color_entry> values_color;
	ImVector<vec4_rel_entry> values_vec4_rel;
	ImVector<color_rel_entry> values_color_rel;

	// Layered blending output (written by iam_layer_end)
	ImGuiStorage	blended_float;
	ImGuiStorage	blended_int;
	ImVector<vec2_entry> blended_vec2;
	ImVector<vec4_entry> blended_vec4;
	ImVector<color_entry> blended_color;
	bool			has_blended;	// true if blended values are valid

	// Marker tracking - bitset for triggered markers (reset on loop)
	ImVector<bool>	markers_triggered;
	float			prev_time;		// Previous time for marker crossing detection

	// Animation chaining - next clip to play when this one completes
	ImGuiID		chain_next_clip_id;		// Clip to play next (0 = none)
	ImGuiID		chain_next_inst_id;		// Instance ID for next clip (0 = auto-generate)
	float		chain_delay;			// Delay before starting chained clip

	// Loop variation tracking
	int			current_loop;			// Current loop iteration (0-based), used for variation calculations
	unsigned int var_rng_state;			// RNG state for deterministic variation random

	// Playhead tween (iam_instance::tween_to)
	bool		tw_active;
	float		tw_from, tw_to, tw_elapsed, tw_dur;
	int			tw_ease;

	iam_instance_data() : inst_id(0), clip_id(0), time(0), time_scale(1.0f), weight(1.0f),
		delay_left(0), playing(false), paused(false), begin_called(false), dir_sign(1), loops_left(0), last_seen_frame(0),
		has_blended(false), prev_time(0), chain_next_clip_id(0), chain_next_inst_id(0), chain_delay(0),
		current_loop(0), var_rng_state(12345), tw_active(false), tw_from(0), tw_to(0), tw_elapsed(0), tw_dur(0), tw_ease(iam_ease_in_out_cubic) {}
};

namespace iam_clip_detail {

// Global clip system state
static struct iam_clip_system {
	ImVector<iam_clip_data>		clips;
	ImVector<iam_instance_data>	instances;
	ImGuiStorage				clip_map;		// clip_id -> index+1
	ImGuiStorage				inst_map;		// inst_id -> index+1
	unsigned					frame_counter;
	bool						initialized;

	iam_clip_system() : frame_counter(0), initialized(false) {}
} g_clip_sys;

static iam_clip_data* find_clip(ImGuiID clip_id) {
	int idx = g_clip_sys.clip_map.GetInt(clip_id, 0);
	if (idx == 0) return nullptr;
	return &g_clip_sys.clips[idx - 1];
}

static iam_instance_data* find_instance(ImGuiID inst_id) {
	int idx = g_clip_sys.inst_map.GetInt(inst_id, 0);
	if (idx == 0) return nullptr;
	return &g_clip_sys.instances[idx - 1];
}

// Evaluate easing for clip keyframes
static float eval_clip_ease(int ease_type, float t, float const* bezier, bool has_bezier) {
	if (ease_type <= iam_ease_in_out_bounce) return iam_detail::eval_preset_internal(ease_type, t);
	if (ease_type == iam_ease_cubic_bezier && !has_bezier) return iam_detail::eval_preset_internal(iam_ease_linear, t);
	// Parametric eases (steps, cubic bezier, spring, custom slot, curve) keep their params in bezier[]
	iam_ease_desc d = { ease_type, bezier[0], bezier[1], bezier[2], bezier[3] };
	return iam_detail::eval(d, t);
}

// Evaluate spring
static float eval_clip_spring(float u, iam_spring_params const& sp) {
	return iam_detail::ease_lut_pool_singleton().spring_unit(u, sp.mass, sp.stiffness, sp.damping, sp.initial_velocity);
}

// ----------------------------------------------------
// Variation evaluation helpers
// ----------------------------------------------------

// Simple xorshift random for deterministic variation
static unsigned int var_xorshift(unsigned int* state) {
	unsigned int x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	return x;
}

// Get random float in [0, 1]
static float var_rand_unit(unsigned int* state) {
	return (float)(var_xorshift(state) & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

// Get random float in [-1, 1]
static float var_rand_signed(unsigned int* state) {
	return var_rand_unit(state) * 2.0f - 1.0f;
}

// Clamp helper
static float var_clampf(float v, float mn, float mx) {
	return v < mn ? mn : (v > mx ? mx : v);
}
static int var_clampi(int v, int mn, int mx) {
	return v < mn ? mn : (v > mx ? mx : v);
}

// Compute float variation delta for given loop index
static float compute_var_float(iam_variation_float const& var, int loop_index, unsigned int* rng_state) {
	if (var.mode == iam_var_none) return 0.0f;

	// Use seed for deterministic random if provided
	unsigned int rng = var.seed != 0 ? (var.seed + (unsigned int)loop_index * 1664525u) : *rng_state;

	float delta = 0.0f;
	switch (var.mode) {
		case iam_var_callback:
			if (var.callback) return var.callback(loop_index, var.user);
			return 0.0f;
		case iam_var_increment:
			delta = var.amount * (float)loop_index;
			break;
		case iam_var_decrement:
			delta = -var.amount * (float)loop_index;
			break;
		case iam_var_multiply:
			// For multiply, return the multiplier (to be applied differently)
			return ImPow(var.amount, (float)loop_index);
		case iam_var_random:
			delta = var_rand_signed(&rng) * var.amount;
			if (var.seed == 0) *rng_state = rng;
			break;
		case iam_var_random_abs:
			delta = var_rand_unit(&rng) * var.amount;
			if (var.seed == 0) *rng_state = rng;
			break;
		case iam_var_pingpong:
			delta = (loop_index % 2 == 0) ? 0.0f : var.amount;
			if (loop_index % 4 >= 2) delta = -delta;
			break;
		default:
			return 0.0f;
	}
	return delta;
}

// Apply float variation to a base value
static float apply_var_float(float base, iam_variation_float const& var, int loop_index, unsigned int* rng_state) {
	if (var.mode == iam_var_none) return base;
	if (var.mode == iam_var_callback && var.callback) {
		return var_clampf(var.callback(loop_index, var.user), var.min_clamp, var.max_clamp);
	}
	if (var.mode == iam_var_multiply) {
		float mult = compute_var_float(var, loop_index, rng_state);
		return var_clampf(base * mult, var.min_clamp, var.max_clamp);
	}
	float delta = compute_var_float(var, loop_index, rng_state);
	return var_clampf(base + delta, var.min_clamp, var.max_clamp);
}

// Apply int variation to a base value
static int apply_var_int(int base, iam_variation_int const& var, int loop_index, unsigned int* rng_state) {
	if (var.mode == iam_var_none) return base;
	if (var.mode == iam_var_callback && var.callback) {
		return var_clampi(var.callback(loop_index, var.user), var.min_clamp, var.max_clamp);
	}

	unsigned int rng = var.seed != 0 ? (var.seed + (unsigned int)loop_index * 1664525u) : *rng_state;
	int delta = 0;
	switch (var.mode) {
		case iam_var_increment:
			delta = var.amount * loop_index;
			break;
		case iam_var_decrement:
			delta = -var.amount * loop_index;
			break;
		case iam_var_multiply:
			return var_clampi((int)(base * ImPow((float)var.amount, (float)loop_index)), var.min_clamp, var.max_clamp);
		case iam_var_random:
			delta = (int)(var_rand_signed(&rng) * (float)var.amount);
			if (var.seed == 0) *rng_state = rng;
			break;
		case iam_var_random_abs:
			delta = (int)(var_rand_unit(&rng) * (float)var.amount);
			if (var.seed == 0) *rng_state = rng;
			break;
		case iam_var_pingpong:
			delta = (loop_index % 2 == 0) ? 0 : var.amount;
			if (loop_index % 4 >= 2) delta = -delta;
			break;
		default:
			return base;
	}
	return var_clampi(base + delta, var.min_clamp, var.max_clamp);
}

// Apply vec2 variation (global or per-axis)
static ImVec2 apply_var_vec2(ImVec2 base, iam_variation_vec2 const& var, int loop_index, unsigned int* rng_state) {
	ImVec2 var_amount = IAM_VEC2_TO_IM(var.amount), var_min = IAM_VEC2_TO_IM(var.min_clamp), var_max = IAM_VEC2_TO_IM(var.max_clamp);
	// Per-axis variation (when global mode is none but per-axis has modes)
	if (var.mode == iam_var_none) {
		if (var.x.mode != iam_var_none || var.y.mode != iam_var_none) {
			return ImVec2(
				apply_var_float(base.x, var.x, loop_index, rng_state),
				apply_var_float(base.y, var.y, loop_index, rng_state)
			);
		}
		return base;
	}

	// Callback
	if (var.mode == iam_var_callback && var.callback) {
		ImVec2 result = IAM_VEC2_TO_IM(var.callback(loop_index, var.user));
		return ImVec2(
			var_clampf(result.x, var_min.x, var_max.x),
			var_clampf(result.y, var_min.y, var_max.y)
		);
	}

	// Global variation
	unsigned int rng = var.seed != 0 ? (var.seed + (unsigned int)loop_index * 1664525u) : *rng_state;
	ImVec2 result = base;

	switch (var.mode) {
		case iam_var_increment:
			result.x += var_amount.x * (float)loop_index;
			result.y += var_amount.y * (float)loop_index;
			break;
		case iam_var_decrement:
			result.x -= var_amount.x * (float)loop_index;
			result.y -= var_amount.y * (float)loop_index;
			break;
		case iam_var_multiply: {
			float mult = ImPow(var_amount.x, (float)loop_index);
			result.x *= mult;
			result.y *= ImPow(var_amount.y, (float)loop_index);
			break;
		}
		case iam_var_random:
			result.x += var_rand_signed(&rng) * var_amount.x;
			result.y += var_rand_signed(&rng) * var_amount.y;
			if (var.seed == 0) *rng_state = rng;
			break;
		case iam_var_random_abs:
			result.x += var_rand_unit(&rng) * var_amount.x;
			result.y += var_rand_unit(&rng) * var_amount.y;
			if (var.seed == 0) *rng_state = rng;
			break;
		case iam_var_pingpong: {
			float sign = (loop_index % 4 >= 2) ? -1.0f : 1.0f;
			float mult = (loop_index % 2 == 0) ? 0.0f : 1.0f;
			result.x += var_amount.x * mult * sign;
			result.y += var_amount.y * mult * sign;
			break;
		}
	}

	return ImVec2(
		var_clampf(result.x, var_min.x, var_max.x),
		var_clampf(result.y, var_min.y, var_max.y)
	);
}

// Apply vec4 variation (global or per-axis)
static ImVec4 apply_var_vec4(ImVec4 base, iam_variation_vec4 const& var, int loop_index, unsigned int* rng_state) {
	ImVec4 var_amount = IAM_VEC4_TO_IM(var.amount), var_min = IAM_VEC4_TO_IM(var.min_clamp), var_max = IAM_VEC4_TO_IM(var.max_clamp);
	// Per-axis variation
	if (var.mode == iam_var_none) {
		if (var.x.mode != iam_var_none || var.y.mode != iam_var_none ||
		    var.z.mode != iam_var_none || var.w.mode != iam_var_none) {
			return ImVec4(
				apply_var_float(base.x, var.x, loop_index, rng_state),
				apply_var_float(base.y, var.y, loop_index, rng_state),
				apply_var_float(base.z, var.z, loop_index, rng_state),
				apply_var_float(base.w, var.w, loop_index, rng_state)
			);
		}
		return base;
	}

	// Callback
	if (var.mode == iam_var_callback && var.callback) {
		ImVec4 result = IAM_VEC4_TO_IM(var.callback(loop_index, var.user));
		return ImVec4(
			var_clampf(result.x, var_min.x, var_max.x),
			var_clampf(result.y, var_min.y, var_max.y),
			var_clampf(result.z, var_min.z, var_max.z),
			var_clampf(result.w, var_min.w, var_max.w)
		);
	}

	// Global variation
	unsigned int rng = var.seed != 0 ? (var.seed + (unsigned int)loop_index * 1664525u) : *rng_state;
	ImVec4 result = base;

	switch (var.mode) {
		case iam_var_increment:
			result.x += var_amount.x * (float)loop_index;
			result.y += var_amount.y * (float)loop_index;
			result.z += var_amount.z * (float)loop_index;
			result.w += var_amount.w * (float)loop_index;
			break;
		case iam_var_decrement:
			result.x -= var_amount.x * (float)loop_index;
			result.y -= var_amount.y * (float)loop_index;
			result.z -= var_amount.z * (float)loop_index;
			result.w -= var_amount.w * (float)loop_index;
			break;
		case iam_var_multiply:
			result.x *= ImPow(var_amount.x, (float)loop_index);
			result.y *= ImPow(var_amount.y, (float)loop_index);
			result.z *= ImPow(var_amount.z, (float)loop_index);
			result.w *= ImPow(var_amount.w, (float)loop_index);
			break;
		case iam_var_random:
			result.x += var_rand_signed(&rng) * var_amount.x;
			result.y += var_rand_signed(&rng) * var_amount.y;
			result.z += var_rand_signed(&rng) * var_amount.z;
			result.w += var_rand_signed(&rng) * var_amount.w;
			if (var.seed == 0) *rng_state = rng;
			break;
		case iam_var_random_abs:
			result.x += var_rand_unit(&rng) * var_amount.x;
			result.y += var_rand_unit(&rng) * var_amount.y;
			result.z += var_rand_unit(&rng) * var_amount.z;
			result.w += var_rand_unit(&rng) * var_amount.w;
			if (var.seed == 0) *rng_state = rng;
			break;
		case iam_var_pingpong: {
			float sign = (loop_index % 4 >= 2) ? -1.0f : 1.0f;
			float mult = (loop_index % 2 == 0) ? 0.0f : 1.0f;
			result.x += var_amount.x * mult * sign;
			result.y += var_amount.y * mult * sign;
			result.z += var_amount.z * mult * sign;
			result.w += var_amount.w * mult * sign;
			break;
		}
	}

	return ImVec4(
		var_clampf(result.x, var_min.x, var_max.x),
		var_clampf(result.y, var_min.y, var_max.y),
		var_clampf(result.z, var_min.z, var_max.z),
		var_clampf(result.w, var_min.w, var_max.w)
	);
}

// Apply color variation (global or per-channel) in the specified color space
// Input and output are in sRGB, variation is applied in the target color space
static ImVec4 apply_var_color(ImVec4 base_srgb, iam_variation_color const& var, int loop_index, unsigned int* rng_state) {
	ImVec4 var_amount = IAM_VEC4_TO_IM(var.amount), var_min = IAM_VEC4_TO_IM(var.min_clamp), var_max = IAM_VEC4_TO_IM(var.max_clamp);
	int color_space = var.color_space;

	// Convert sRGB to working color space
	ImVec4 base = iam_detail::color::to_space(base_srgb, color_space);

	// Per-channel variation (in working color space)
	if (var.mode == iam_var_none) {
		if (var.r.mode != iam_var_none || var.g.mode != iam_var_none ||
		    var.b.mode != iam_var_none || var.a.mode != iam_var_none) {
			ImVec4 result(
				apply_var_float(base.x, var.r, loop_index, rng_state),
				apply_var_float(base.y, var.g, loop_index, rng_state),
				apply_var_float(base.z, var.b, loop_index, rng_state),
				apply_var_float(base.w, var.a, loop_index, rng_state)
			);
			// Convert back to sRGB
			return iam_detail::color::from_space(result, color_space);
		}
		return base_srgb;
	}

	// Callback (assumed to return sRGB directly)
	if (var.mode == iam_var_callback && var.callback) {
		ImVec4 result = IAM_VEC4_TO_IM(var.callback(loop_index, var.user));
		return ImVec4(
			var_clampf(result.x, var_min.x, var_max.x),
			var_clampf(result.y, var_min.y, var_max.y),
			var_clampf(result.z, var_min.z, var_max.z),
			var_clampf(result.w, var_min.w, var_max.w)
		);
	}

	// Global variation in working color space
	unsigned int rng = var.seed != 0 ? (var.seed + (unsigned int)loop_index * 1664525u) : *rng_state;
	ImVec4 result = base;

	switch (var.mode) {
		case iam_var_increment:
			result.x += var_amount.x * (float)loop_index;
			result.y += var_amount.y * (float)loop_index;
			result.z += var_amount.z * (float)loop_index;
			result.w += var_amount.w * (float)loop_index;
			break;
		case iam_var_decrement:
			result.x -= var_amount.x * (float)loop_index;
			result.y -= var_amount.y * (float)loop_index;
			result.z -= var_amount.z * (float)loop_index;
			result.w -= var_amount.w * (float)loop_index;
			break;
		case iam_var_multiply:
			result.x *= ImPow(var_amount.x, (float)loop_index);
			result.y *= ImPow(var_amount.y, (float)loop_index);
			result.z *= ImPow(var_amount.z, (float)loop_index);
			result.w *= ImPow(var_amount.w, (float)loop_index);
			break;
		case iam_var_random:
			result.x += var_rand_signed(&rng) * var_amount.x;
			result.y += var_rand_signed(&rng) * var_amount.y;
			result.z += var_rand_signed(&rng) * var_amount.z;
			result.w += var_rand_signed(&rng) * var_amount.w;
			if (var.seed == 0) *rng_state = rng;
			break;
		case iam_var_random_abs:
			result.x += var_rand_unit(&rng) * var_amount.x;
			result.y += var_rand_unit(&rng) * var_amount.y;
			result.z += var_rand_unit(&rng) * var_amount.z;
			result.w += var_rand_unit(&rng) * var_amount.w;
			if (var.seed == 0) *rng_state = rng;
			break;
		case iam_var_pingpong: {
			float sign = (loop_index % 4 >= 2) ? -1.0f : 1.0f;
			float mult = (loop_index % 2 == 0) ? 0.0f : 1.0f;
			result.x += var_amount.x * mult * sign;
			result.y += var_amount.y * mult * sign;
			result.z += var_amount.z * mult * sign;
			result.w += var_amount.w * mult * sign;
			break;
		}
	}

	// Clamp in working space
	result = ImVec4(
		var_clampf(result.x, var_min.x, var_max.x),
		var_clampf(result.y, var_min.y, var_max.y),
		var_clampf(result.z, var_min.z, var_max.z),
		var_clampf(result.w, var_min.w, var_max.w)
	);

	// Convert back to sRGB
	return iam_detail::color::from_space(result, color_space);
}

// Find keyframes bracketing time t for a iam_track
static bool find_keys(iam_track const& trk, float t, keyframe const** k0, keyframe const** k1) {
	if (trk.keys.Size == 0) return false;

	// Single keyframe case
	if (trk.keys.Size == 1) {
		*k0 = *k1 = &trk.keys[0];
		return true;
	}

	// Before first keyframe
	if (t <= trk.keys[0].time) {
		*k0 = *k1 = &trk.keys[0];
		return true;
	}

	// After last keyframe
	if (t >= trk.keys[trk.keys.Size - 1].time) {
		*k0 = *k1 = &trk.keys[trk.keys.Size - 1];
		return true;
	}

	// Find bracketing keyframes
	for (int i = 0; i < trk.keys.Size - 1; ++i) {
		if (t >= trk.keys[i].time && t <= trk.keys[i + 1].time) {
			*k0 = &trk.keys[i];
			*k1 = &trk.keys[i + 1];
			return true;
		}
	}

	// Fallback (shouldn't reach here)
	*k0 = *k1 = &trk.keys[trk.keys.Size - 1];
	return true;
}

// Evaluate a iam_track at time t
static void eval_iam_track(iam_track const& trk, float t, iam_instance_data* inst) {
	if (!inst || trk.keys.Size == 0) return;
	keyframe const* k0; keyframe const* k1;
	if (!find_keys(trk, t, &k0, &k1)) return;
	if (!k0 || !k1) return;  // Safety check

	float u = (k1->time == k0->time) ? 1.0f : (t - k0->time) / (k1->time - k0->time);
	float w;
	if (k0->is_spring && trk.type == iam_chan_float) {
		w = eval_clip_spring(u, k0->spring);
	} else {
		w = eval_clip_ease(k0->ease_type, u, k0->bezier, k0->has_bezier);
	}

	// Get current loop index for variation
	int loop_index = inst->current_loop;

	switch (trk.type) {
		case iam_chan_float: {
			float a = k0->get_float(), b = k1->get_float();
			// Apply variation if present
			if (k0->has_variation) {
				a = apply_var_float(a, k0->var_float, loop_index, &inst->var_rng_state);
			}
			if (k1->has_variation) {
				b = apply_var_float(b, k1->var_float, loop_index, &inst->var_rng_state);
			}
			float v = a + (b - a) * w;
			inst->values_float.SetFloat(trk.channel, v);
			break;
		}
		case iam_chan_vec2: {
			ImVec2 a = k0->get_vec2(), b = k1->get_vec2();
			// Apply variation if present
			if (k0->has_variation) {
				a = apply_var_vec2(a, k0->var_vec2, loop_index, &inst->var_rng_state);
			}
			if (k1->has_variation) {
				b = apply_var_vec2(b, k1->var_vec2, loop_index, &inst->var_rng_state);
			}
			ImVec2 v(a.x + (b.x - a.x) * w, a.y + (b.y - a.y) * w);
			// Store in vec2 array
			bool found = false;
			for (int i = 0; i < inst->values_vec2.Size; ++i) {
				if (inst->values_vec2[i].ch == trk.channel) {
					inst->values_vec2[i].v = v;
					found = true;
					break;
				}
			}
			if (!found) {
				iam_instance_data::vec2_entry e; e.ch = trk.channel; e.v = v;
				inst->values_vec2.push_back(e);
			}
			break;
		}
		case iam_chan_vec4: {
			ImVec4 a = k0->get_vec4(), b = k1->get_vec4();
			// Apply variation if present
			if (k0->has_variation) {
				a = apply_var_vec4(a, k0->var_vec4, loop_index, &inst->var_rng_state);
			}
			if (k1->has_variation) {
				b = apply_var_vec4(b, k1->var_vec4, loop_index, &inst->var_rng_state);
			}
			ImVec4 v(a.x + (b.x - a.x) * w, a.y + (b.y - a.y) * w, a.z + (b.z - a.z) * w, a.w + (b.w - a.w) * w);
			bool found = false;
			for (int i = 0; i < inst->values_vec4.Size; ++i) {
				if (inst->values_vec4[i].ch == trk.channel) {
					inst->values_vec4[i].v = v;
					found = true;
					break;
				}
			}
			if (!found) {
				iam_instance_data::vec4_entry e; e.ch = trk.channel; e.v = v;
				inst->values_vec4.push_back(e);
			}
			break;
		}
		case iam_chan_int: {
			int a = k0->get_int(), b = k1->get_int();
			// Apply variation if present
			if (k0->has_variation) {
				a = apply_var_int(a, k0->var_int, loop_index, &inst->var_rng_state);
			}
			if (k1->has_variation) {
				b = apply_var_int(b, k1->var_int, loop_index, &inst->var_rng_state);
			}
			int v = (int)(a + (int)((float)(b - a) * w + 0.5f));
			inst->values_int.SetInt(trk.channel, v);
			break;
		}
		case iam_chan_color: {
			ImVec4 a = k0->get_color(), b = k1->get_color();
			// Apply variation if present
			if (k0->has_variation) {
				a = apply_var_color(a, k0->var_color, loop_index, &inst->var_rng_state);
			}
			if (k1->has_variation) {
				b = apply_var_color(b, k1->var_color, loop_index, &inst->var_rng_state);
			}
			// Blend in the specified color space
			ImVec4 v = iam_detail::color::lerp_color(a, b, w, trk.color_space);
			bool found = false;
			for (int i = 0; i < inst->values_color.Size; ++i) {
				if (inst->values_color[i].ch == trk.channel) {
					inst->values_color[i].v = v;
					inst->values_color[i].color_space = trk.color_space;
					found = true;
					break;
				}
			}
			if (!found) {
				iam_instance_data::color_entry e; e.ch = trk.channel; e.v = v; e.color_space = trk.color_space;
				inst->values_color.push_back(e);
			}
			break;
		}
		case iam_chan_float_rel: {
			// Interpolate percent and px_bias separately, store as vec2 (percent, px_bias)
			float percent_a = k0->get_float_rel_percent(), percent_b = k1->get_float_rel_percent();
			float px_bias_a = k0->get_float_rel_px_bias(), px_bias_b = k1->get_float_rel_px_bias();
			float percent = percent_a + (percent_b - percent_a) * w;
			float px_bias = px_bias_a + (px_bias_b - px_bias_a) * w;
			// Store as vec2 for later resolution (percent in x, px_bias in y)
			ImVec2 v(percent, px_bias);
			bool found = false;
			for (int i = 0; i < inst->values_vec2.Size; ++i) {
				if (inst->values_vec2[i].ch == trk.channel) {
					inst->values_vec2[i].v = v;
					found = true;
					break;
				}
			}
			if (!found) {
				iam_instance_data::vec2_entry e; e.ch = trk.channel; e.v = v;
				inst->values_vec2.push_back(e);
			}
			break;
		}
		case iam_chan_vec2_rel: {
			// Interpolate percent and px_bias separately, store as vec4 (percent.xy, px_bias.xy)
			ImVec2 percent_a = k0->get_vec2_rel_percent(), percent_b = k1->get_vec2_rel_percent();
			ImVec2 px_bias_a = k0->get_vec2_rel_px_bias(), px_bias_b = k1->get_vec2_rel_px_bias();
			ImVec2 percent(percent_a.x + (percent_b.x - percent_a.x) * w, percent_a.y + (percent_b.y - percent_a.y) * w);
			ImVec2 px_bias(px_bias_a.x + (px_bias_b.x - px_bias_a.x) * w, px_bias_a.y + (px_bias_b.y - px_bias_a.y) * w);
			// Store as vec4 (percent.x, percent.y, px_bias.x, px_bias.y)
			ImVec4 v(percent.x, percent.y, px_bias.x, px_bias.y);
			bool found = false;
			for (int i = 0; i < inst->values_vec4.Size; ++i) {
				if (inst->values_vec4[i].ch == trk.channel) {
					inst->values_vec4[i].v = v;
					found = true;
					break;
				}
			}
			if (!found) {
				iam_instance_data::vec4_entry e; e.ch = trk.channel; e.v = v;
				inst->values_vec4.push_back(e);
			}
			break;
		}
		case iam_chan_vec4_rel: {
			// Interpolate percent and px_bias separately
			ImVec4 percent_a = k0->get_vec4_rel_percent(), percent_b = k1->get_vec4_rel_percent();
			ImVec4 px_bias_a = k0->get_vec4_rel_px_bias(), px_bias_b = k1->get_vec4_rel_px_bias();
			ImVec4 percent(
				percent_a.x + (percent_b.x - percent_a.x) * w,
				percent_a.y + (percent_b.y - percent_a.y) * w,
				percent_a.z + (percent_b.z - percent_a.z) * w,
				percent_a.w + (percent_b.w - percent_a.w) * w
			);
			ImVec4 px_bias(
				px_bias_a.x + (px_bias_b.x - px_bias_a.x) * w,
				px_bias_a.y + (px_bias_b.y - px_bias_a.y) * w,
				px_bias_a.z + (px_bias_b.z - px_bias_a.z) * w,
				px_bias_a.w + (px_bias_b.w - px_bias_a.w) * w
			);
			bool found = false;
			for (int i = 0; i < inst->values_vec4_rel.Size; ++i) {
				if (inst->values_vec4_rel[i].ch == trk.channel) {
					inst->values_vec4_rel[i].percent = percent;
					inst->values_vec4_rel[i].px_bias = px_bias;
					found = true;
					break;
				}
			}
			if (!found) {
				iam_instance_data::vec4_rel_entry e;
				e.ch = trk.channel;
				e.percent = percent;
				e.px_bias = px_bias;
				inst->values_vec4_rel.push_back(e);
			}
			break;
		}
		case iam_chan_color_rel: {
			// Interpolate percent and px_bias separately (color blending not applied to percent/bias)
			ImVec4 percent_a = k0->get_color_rel_percent(), percent_b = k1->get_color_rel_percent();
			ImVec4 px_bias_a = k0->get_color_rel_px_bias(), px_bias_b = k1->get_color_rel_px_bias();
			ImVec4 percent(
				percent_a.x + (percent_b.x - percent_a.x) * w,
				percent_a.y + (percent_b.y - percent_a.y) * w,
				percent_a.z + (percent_b.z - percent_a.z) * w,
				percent_a.w + (percent_b.w - percent_a.w) * w
			);
			ImVec4 px_bias(
				px_bias_a.x + (px_bias_b.x - px_bias_a.x) * w,
				px_bias_a.y + (px_bias_b.y - px_bias_a.y) * w,
				px_bias_a.z + (px_bias_b.z - px_bias_a.z) * w,
				px_bias_a.w + (px_bias_b.w - px_bias_a.w) * w
			);
			bool found = false;
			for (int i = 0; i < inst->values_color_rel.Size; ++i) {
				if (inst->values_color_rel[i].ch == trk.channel) {
					inst->values_color_rel[i].percent = percent;
					inst->values_color_rel[i].px_bias = px_bias;
					inst->values_color_rel[i].color_space = trk.color_space;
					found = true;
					break;
				}
			}
			if (!found) {
				iam_instance_data::color_rel_entry e;
				e.ch = trk.channel;
				e.percent = percent;
				e.px_bias = px_bias;
				e.color_space = trk.color_space;
				inst->values_color_rel.push_back(e);
			}
			break;
		}
	}
}

// Sort keyframes by time
static int cmp_keyframe(void const* a, void const* b) {
	keyframe const* A = (keyframe const*)a;
	keyframe const* B = (keyframe const*)b;
	if (A->time < B->time) return -1;
	if (A->time > B->time) return 1;
	return 0;
}

} // namespace iam_clip_detail

// ----------------------------------------------------
// iam_clip class implementation
// ----------------------------------------------------

// Helper to get clip data by ID (safe lookup)
static iam_clip_data* get_clip_data(ImGuiID clip_id) {
	using namespace iam_clip_detail;
	return find_clip(clip_id);
}

iam_clip iam_clip::begin(ImGuiID clip_id) {
	using namespace iam_clip_detail;
	if (!g_clip_sys.initialized) {
		iam_clip_init();
	}

	// Find or create clip
	int idx = g_clip_sys.clip_map.GetInt(clip_id, 0);
	iam_clip_data* clip;
	if (idx == 0) {
		g_clip_sys.clips.push_back(iam_clip_data());
		clip = &g_clip_sys.clips.back();
		clip->id = clip_id;
		g_clip_sys.clip_map.SetInt(clip_id, g_clip_sys.clips.Size);
	} else {
		clip = &g_clip_sys.clips[idx - 1];
	}

	// Reset for building
	clip->build_keys.clear();
	clip->iam_tracks.clear();
	clip->labels.clear();
	clip->group_stack.clear();
	clip->duration = 0;
	clip->delay = 0;
	clip->loop_count = 0;
	clip->direction = iam_dir_normal;
	clip->build_time_offset = 0;
	clip->stagger_count = 0;
	clip->stagger_delay = 0;
	clip->stagger_center_bias = 0;
	clip->stagger_ease = iam_ease_linear;
	clip->loop_delay = 0;
	clip->cb_begin = nullptr;  clip->cb_begin_user = nullptr;
	clip->cb_update = nullptr; clip->cb_update_user = nullptr;
	clip->cb_complete = nullptr; clip->cb_complete_user = nullptr;
	clip->cb_loop = nullptr;   clip->cb_loop_user = nullptr;
	clip->cb_pause = nullptr;  clip->cb_pause_user = nullptr;

	// Reset timing variation
	clip->has_duration_var = false;
	clip->has_delay_var = false;
	clip->has_timescale_var = false;
	memset(&clip->duration_var, 0, sizeof(clip->duration_var));
	memset(&clip->delay_var, 0, sizeof(clip->delay_var));
	memset(&clip->timescale_var, 0, sizeof(clip->timescale_var));

	return iam_clip(clip_id);
}

// Helper to compute actual keyframe time and update group state
static float compute_key_time(iam_clip_data* clip, float time) {
	float actual_time = time + clip->build_time_offset;
	// Update max_time in current group if any
	if (clip->group_stack.Size > 0) {
		iam_clip_data::group_state& gs = clip->group_stack.back();
		if (actual_time > gs.max_time) gs.max_time = actual_time;
	}
	return actual_time;
}

iam_clip& iam_clip::key_float(ImGuiID channel, float time, float value, int ease_type, float const* bezier4) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_float;
	k.ease_type = ease_type;
	k.set_float(value);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_vec2(ImGuiID channel, float time, iam_vec2 value_in, int ease_type, float const* bezier4) {
	ImVec2 value = IAM_VEC2_TO_IM(value_in);
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_vec2;
	k.ease_type = ease_type;
	k.set_vec2(value);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_vec4(ImGuiID channel, float time, iam_vec4 value_in, int ease_type, float const* bezier4) {
	ImVec4 value = IAM_VEC4_TO_IM(value_in);
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_vec4;
	k.ease_type = ease_type;
	k.set_vec4(value);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_int(ImGuiID channel, float time, int value, int ease_type) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_int;
	k.ease_type = ease_type;
	k.set_int(value);
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_color(ImGuiID channel, float time, iam_vec4 value_in, int color_space, int ease_type, float const* bezier4) {
	ImVec4 value = IAM_VEC4_TO_IM(value_in);
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_color;
	k.ease_type = ease_type;
	k.color_space = color_space;
	k.set_color(value);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_float_spring(ImGuiID channel, float time, float target, iam_spring_params const& spring) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_float;
	k.ease_type = iam_ease_spring;
	k.is_spring = true;
	k.spring = spring;
	k.set_float(target);
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

// Anchor-relative keyframes
iam_clip& iam_clip::key_float_rel(ImGuiID channel, float time, float percent, float px_bias, int anchor_space, int axis, int ease_type, float const* bezier4) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_float_rel;
	k.ease_type = ease_type;
	k.set_float_rel(percent, px_bias);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	// Store anchor info in the keyframe for track setup during end()
	// We abuse color_space field temporarily to store anchor_space | (axis << 8)
	k.color_space = anchor_space | (axis << 8);
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_vec2_rel(ImGuiID channel, float time, iam_vec2 percent_in, iam_vec2 px_bias_in, int anchor_space, int ease_type, float const* bezier4) {
	ImVec2 percent = IAM_VEC2_TO_IM(percent_in);
	ImVec2 px_bias = IAM_VEC2_TO_IM(px_bias_in);
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_vec2_rel;
	k.ease_type = ease_type;
	k.set_vec2_rel(percent, px_bias);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	// Store anchor info
	k.color_space = anchor_space;
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_vec4_rel(ImGuiID channel, float time, iam_vec4 percent_in, iam_vec4 px_bias_in, int anchor_space, int ease_type, float const* bezier4) {
	ImVec4 percent = IAM_VEC4_TO_IM(percent_in);
	ImVec4 px_bias = IAM_VEC4_TO_IM(px_bias_in);
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_vec4_rel;
	k.ease_type = ease_type;
	k.set_vec4_rel(percent, px_bias);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	// Store anchor info
	k.color_space = anchor_space;
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_color_rel(ImGuiID channel, float time, iam_vec4 percent_in, iam_vec4 px_bias_in, int color_space, int anchor_space, int ease_type, float const* bezier4) {
	ImVec4 percent = IAM_VEC4_TO_IM(percent_in);
	ImVec4 px_bias = IAM_VEC4_TO_IM(px_bias_in);
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_color_rel;
	k.ease_type = ease_type;
	k.color_space = color_space;
	k.set_color_rel(percent, px_bias);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	// Store anchor_space in a different way - use high bits of color_space
	// color_space uses low 8 bits, anchor_space in bits 8-15
	k.color_space = (color_space & 0xFF) | ((anchor_space & 0xFF) << 8);
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

// Keyframes with repeat variation
iam_clip& iam_clip::key_float_var(ImGuiID channel, float time, float value, iam_variation_float const& var, int ease_type, float const* bezier4) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_float;
	k.ease_type = ease_type;
	k.set_float(value);
	k.has_variation = true;
	k.var_float = var;
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_vec2_var(ImGuiID channel, float time, iam_vec2 value_in, iam_variation_vec2 const& var, int ease_type, float const* bezier4) {
	ImVec2 value = IAM_VEC2_TO_IM(value_in);
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_vec2;
	k.ease_type = ease_type;
	k.set_vec2(value);
	k.has_variation = true;
	k.var_vec2 = var;
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_vec4_var(ImGuiID channel, float time, iam_vec4 value_in, iam_variation_vec4 const& var, int ease_type, float const* bezier4) {
	ImVec4 value = IAM_VEC4_TO_IM(value_in);
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_vec4;
	k.ease_type = ease_type;
	k.set_vec4(value);
	k.has_variation = true;
	k.var_vec4 = var;
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_int_var(ImGuiID channel, float time, int value, iam_variation_int const& var, int ease_type) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_int;
	k.ease_type = ease_type;
	k.set_int(value);
	k.has_variation = true;
	k.var_int = var;
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::key_color_var(ImGuiID channel, float time, iam_vec4 value_in, iam_variation_color const& var, int color_space, int ease_type, float const* bezier4) {
	ImVec4 value = IAM_VEC4_TO_IM(value_in);
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float actual_time = compute_key_time(clip, time);
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = actual_time;
	k.type = iam_chan_color;
	k.ease_type = ease_type;
	k.color_space = color_space;
	k.set_color(value);
	k.has_variation = true;
	k.var_color = var;
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (actual_time > clip->duration) clip->duration = actual_time;
	return *this;
}

iam_clip& iam_clip::seq_begin() {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	iam_clip_data::group_state gs;
	gs.base_time = clip->build_time_offset;
	gs.max_time = clip->build_time_offset;
	gs.is_seq = true;
	clip->group_stack.push_back(gs);
	return *this;
}

iam_clip& iam_clip::seq_end() {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip || clip->group_stack.Size == 0) return *this;
	iam_clip_data::group_state gs = clip->group_stack.back();
	clip->group_stack.pop_back();
	// After seq_end, the time offset advances to max_time seen in the group
	if (gs.is_seq) {
		clip->build_time_offset = gs.max_time;
	}
	return *this;
}

iam_clip& iam_clip::par_begin() {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	iam_clip_data::group_state gs;
	gs.base_time = clip->build_time_offset;
	gs.max_time = clip->build_time_offset;
	gs.is_seq = false;
	clip->group_stack.push_back(gs);
	return *this;
}

iam_clip& iam_clip::par_end() {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip || clip->group_stack.Size == 0) return *this;
	iam_clip_data::group_state gs = clip->group_stack.back();
	clip->group_stack.pop_back();
	// After par_end, time offset advances to max_time (all parallel tracks complete)
	clip->build_time_offset = gs.max_time;
	return *this;
}

iam_clip& iam_clip::set_loop(bool loop, int direction, int loop_count) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->direction = direction;
	clip->loop_count = loop ? loop_count : 0;
	return *this;
}

iam_clip& iam_clip::set_delay(float delay_seconds) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->delay = delay_seconds;
	return *this;
}

iam_clip& iam_clip::set_stagger(int count, float each_delay, float from_center_bias) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->stagger_count = count > 0 ? count : 1;
	clip->stagger_delay = each_delay;
	clip->stagger_center_bias = ImClamp(from_center_bias, 0.0f, 1.0f);
	return *this;
}

iam_clip& iam_clip::set_stagger_ease(int ease_type) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->stagger_ease = ease_type;
	return *this;
}

iam_clip& iam_clip::set_duration_var(iam_variation_float const& var) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->has_duration_var = true;
	clip->duration_var = var;
	return *this;
}

iam_clip& iam_clip::set_delay_var(iam_variation_float const& var) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->has_delay_var = true;
	clip->delay_var = var;
	return *this;
}

iam_clip& iam_clip::set_timescale_var(iam_variation_float const& var) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->has_timescale_var = true;
	clip->timescale_var = var;
	return *this;
}

iam_clip& iam_clip::on_begin(iam_clip_callback cb, void* user) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->cb_begin = cb;
	clip->cb_begin_user = user;
	return *this;
}

iam_clip& iam_clip::on_update(iam_clip_callback cb, void* user) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->cb_update = cb;
	clip->cb_update_user = user;
	return *this;
}

iam_clip& iam_clip::on_complete(iam_clip_callback cb, void* user) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->cb_complete = cb;
	clip->cb_complete_user = user;
	return *this;
}

iam_clip& iam_clip::on_loop(iam_loop_callback cb, void* user) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->cb_loop = cb;
	clip->cb_loop_user = user;
	return *this;
}

iam_clip& iam_clip::on_pause(iam_clip_callback cb, void* user) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->cb_pause = cb;
	clip->cb_pause_user = user;
	return *this;
}

iam_clip& iam_clip::set_loop_delay(float delay_seconds) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	clip->loop_delay = delay_seconds;
	return *this;
}

// Auto-generate unique marker IDs
static ImGuiID generate_marker_id() {
	static unsigned s_marker_counter = 0;
	return ImHashData(&(++s_marker_counter), sizeof(s_marker_counter));
}

iam_clip& iam_clip::label(ImGuiID label_id, float time) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	float t = time + clip->build_time_offset;  // like markers: relative to the current seq/par offset
	for (int i = 0; i < clip->labels.Size; i++)
		if (clip->labels[i].id == label_id) { clip->labels[i].time = t; return *this; }
	iam_clip_detail::clip_label l;
	l.id = label_id;
	l.time = t;
	clip->labels.push_back(l);
	return *this;
}

iam_clip& iam_clip::marker(float time, ImGuiID marker_id, iam_marker_callback cb, void* user) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;

	iam_clip_detail::iam_marker m;
	m.time = time + clip->build_time_offset;  // Apply current time offset
	m.marker_id = marker_id;
	m.callback = cb;
	m.user_data = user;
	clip->markers.push_back(m);

	// Update duration if marker is past current end
	if (m.time > clip->duration) {
		clip->duration = m.time;
	}

	return *this;
}

iam_clip& iam_clip::marker(float time, iam_marker_callback cb, void* user) {
	return marker(time, generate_marker_id(), cb, user);
}

void iam_clip::end() {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return;
	using namespace iam_clip_detail;

	// Sort keyframes by time
	if (clip->build_keys.Size > 1) {
		qsort(clip->build_keys.Data, clip->build_keys.Size, sizeof(keyframe), cmp_keyframe);
	}

	// Build iam_tracks by grouping keyframes by (channel, type)
	for (int i = 0; i < clip->build_keys.Size; ++i) {
		keyframe const& k = clip->build_keys[i];

		// Find existing iam_track
		iam_track* trk = nullptr;
		for (int t = 0; t < clip->iam_tracks.Size; ++t) {
			if (clip->iam_tracks[t].channel == k.channel && clip->iam_tracks[t].type == k.type) {
				trk = &clip->iam_tracks[t];
				break;
			}
		}

		// Create new iam_track if needed
		if (!trk) {
			clip->iam_tracks.push_back(iam_track());
			trk = &clip->iam_tracks.back();
			trk->channel = k.channel;
			trk->type = k.type;
			if (k.type == iam_chan_color) {
				trk->color_space = k.color_space;
			}
			// Set up anchor info for relative tracks
			if (k.type == iam_chan_float_rel) {
				trk->is_relative = true;
				trk->anchor_space = k.color_space & 0xFF;
				trk->anchor_axis = (k.color_space >> 8) & 0xFF;
			} else if (k.type == iam_chan_vec2_rel) {
				trk->is_relative = true;
				trk->anchor_space = k.color_space;
				trk->anchor_axis = 0;  // Not used for vec2
			} else if (k.type == iam_chan_vec4_rel) {
				trk->is_relative = true;
				trk->anchor_space = k.color_space;
				trk->anchor_axis = 0;  // Not used for vec4
			} else if (k.type == iam_chan_color_rel) {
				trk->is_relative = true;
				trk->color_space = k.color_space & 0xFF;  // Extract color_space from low bits
				trk->anchor_space = (k.color_space >> 8) & 0xFF;  // Extract anchor_space from high bits
				trk->anchor_axis = 0;
			}
		}

		trk->keys.push_back(k);
	}

	// Clear build data
	clip->build_keys.clear();

	// Sort markers by time
	if (clip->markers.Size > 1) {
		for (int i = 0; i < clip->markers.Size - 1; ++i) {
			for (int j = i + 1; j < clip->markers.Size; ++j) {
				if (clip->markers[j].time < clip->markers[i].time) {
					iam_marker tmp = clip->markers[i];
					clip->markers[i] = clip->markers[j];
					clip->markers[j] = tmp;
				}
			}
		}
	}
}

// ----------------------------------------------------
// iam_instance class implementation
// ----------------------------------------------------

// Helper to get instance data by ID (safe lookup)
static iam_instance_data* get_instance_data(ImGuiID inst_id) {
	using namespace iam_clip_detail;
	return find_instance(inst_id);
}

bool iam_instance::valid() const {
	return m_inst_id != 0 && get_instance_data(m_inst_id) != nullptr;
}

void iam_instance::pause() {
	using namespace iam_clip_detail;
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst) return;
	inst->paused = true;
	iam_clip_data* clip = find_clip(inst->clip_id);
	if (clip && clip->cb_pause)
		clip->cb_pause(inst->inst_id, clip->cb_pause_user);
}

void iam_instance::resume() {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) inst->paused = false;
}

void iam_instance::stop() {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) { inst->playing = false; inst->time = 0; }
}

void iam_instance::restart() {
	using namespace iam_clip_detail;
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst) return;
	iam_clip_data* clip = find_clip(inst->clip_id);
	if (!clip) return;

	inst->time = 0.0f;
	inst->delay_left = clip->delay;
	inst->playing = true;
	inst->paused = false;
	inst->begin_called = false;
	inst->dir_sign = (clip->direction == iam_dir_reverse) ? -1 : 1;
	inst->time = (inst->dir_sign > 0) ? 0.0f : clip->duration;  // reverse plays from the end
	inst->loops_left = clip->loop_count;
	inst->last_seen_frame = g_clip_sys.frame_counter;
	inst->tw_active = false;
	inst->prev_time = (inst->dir_sign > 0) ? 0.0f : clip->duration;
	inst->markers_triggered.resize(clip->markers.Size);
	for (int m = 0; m < inst->markers_triggered.Size; m++)
		inst->markers_triggered[m] = false;
	inst->chain_next_clip_id = 0;
	inst->chain_next_inst_id = 0;
	inst->chain_delay = 0;
	inst->current_loop = 0;
	inst->var_rng_state = 12345 + m_inst_id;

	float initial_time = (inst->dir_sign > 0) ? 0.0f : clip->duration;
	for (int tr = 0; tr < clip->iam_tracks.Size; ++tr)
		eval_iam_track(clip->iam_tracks[tr], initial_time, inst);
}

void iam_instance::reset() {
	using namespace iam_clip_detail;
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst) return;
	iam_clip_data* clip = find_clip(inst->clip_id);
	if (!clip) return;

	float initial_time = (inst->dir_sign > 0) ? 0.0f : clip->duration;
	inst->time = initial_time;
	inst->playing = false;
	inst->tw_active = false;
	inst->paused = false;
	inst->delay_left = 0.0f;

	for (int tr = 0; tr < clip->iam_tracks.Size; ++tr)
		eval_iam_track(clip->iam_tracks[tr], initial_time, inst);
}

void iam_instance::refresh() {
	using namespace iam_clip_detail;
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst) return;
	iam_clip_data* clip = find_clip(inst->clip_id);
	if (!clip) return;

	for (int tr = 0; tr < clip->iam_tracks.Size; ++tr)
		eval_iam_track(clip->iam_tracks[tr], inst->time, inst);
}

void iam_instance::destroy() {
	using namespace iam_clip_detail;
	int idx = g_clip_sys.inst_map.GetInt(m_inst_id, 0);
	if (idx == 0) return;
	// Clear the instance data
	iam_instance_data* inst = &g_clip_sys.instances[idx - 1];
	inst->inst_id = 0;
	inst->clip_id = 0;
	inst->playing = false;
	inst->values_float.Clear();
	inst->values_vec2.clear();
	inst->values_vec4.clear();
	inst->values_int.Clear();
	// Remove from map
	g_clip_sys.inst_map.SetInt(m_inst_id, 0);
	m_inst_id = 0;
}

void iam_instance::seek(float time) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst) return;
	iam_clip_data* clip = get_clip_data(inst->clip_id);
	if (!clip) return;
	float dur = clip->duration;
	if (time < 0) time = 0;
	if (time > dur) time = dur;
	inst->time = time;
	inst->prev_time = time;
	inst->tw_active = false;
}

void iam_instance::reverse() {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst) return;
	inst->tw_active = false;
	inst->dir_sign = -inst->dir_sign;
	inst->playing = true;   // a finished instance plays back the other way
	inst->paused = false;
	inst->delay_left = 0.0f;
}

bool iam_instance::is_reversed() const {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	return inst && inst->dir_sign < 0;
}

void iam_instance::tween_to(float time, float duration, int ease_type) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst) return;
	iam_clip_data* clip = get_clip_data(inst->clip_id);
	if (!clip) return;
	inst->tw_active = true;
	inst->tw_from = inst->time;
	inst->tw_to = ImClamp(time, 0.0f, clip->duration);
	inst->tw_elapsed = 0.0f;
	inst->tw_dur = iam_detail::motion_dur(duration);
	inst->tw_ease = ease_type;
	inst->prev_time = inst->time;
	inst->delay_left = 0.0f;
}

bool iam_instance::tween_to_label(ImGuiID label_id, float duration, int ease_type) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst) return false;
	float t = iam_clip_label_time(inst->clip_id, label_id);
	if (t < 0.0f) return false;
	tween_to(t, duration, ease_type);
	return true;
}

bool iam_instance::seek_label(ImGuiID label_id) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst) return false;
	float t = iam_clip_label_time(inst->clip_id, label_id);
	if (t < 0.0f) return false;
	inst->tw_active = false;
	seek(t);
	refresh();
	return true;
}

float iam_clip_label_time(ImGuiID clip_id, ImGuiID label_id) {
	iam_clip_data* clip = get_clip_data(clip_id);
	if (!clip) return -1.0f;
	for (int i = 0; i < clip->labels.Size; i++)
		if (clip->labels[i].id == label_id) return clip->labels[i].time;
	return -1.0f;
}

void iam_instance::set_time_scale(float scale) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) inst->time_scale = scale;
}

void iam_instance::set_weight(float weight) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) inst->weight = weight;
}

// Animation chaining
static ImGuiID generate_chain_instance_id() {
	static unsigned s_chain_counter = 0;
	return ImHashData(&(++s_chain_counter), sizeof(s_chain_counter));
}

iam_instance& iam_instance::then(ImGuiID next_clip_id) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) {
		inst->chain_next_clip_id = next_clip_id;
		inst->chain_next_inst_id = generate_chain_instance_id();  // Auto-generate instance ID
	}
	return *this;
}

iam_instance& iam_instance::then(ImGuiID next_clip_id, ImGuiID next_instance_id) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) {
		inst->chain_next_clip_id = next_clip_id;
		inst->chain_next_inst_id = next_instance_id;
	}
	return *this;
}

iam_instance& iam_instance::then_delay(float delay) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) {
		inst->chain_delay = delay;
	}
	return *this;
}

float iam_instance::time() const {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	return inst ? inst->time : 0.0f;
}

float iam_instance::duration() const {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst) return 0.0f;
	iam_clip_data* clip = get_clip_data(inst->clip_id);
	return clip ? clip->duration : 0.0f;
}

bool iam_instance::is_playing() const {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	return inst ? inst->playing : false;
}

bool iam_instance::is_paused() const {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	return inst ? inst->paused : false;
}

bool iam_instance::get_float(ImGuiID channel, float* out) const {
	using namespace iam_clip_detail;
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst || !out) return false;

	// Check if this channel is a relative float track (stored as vec2: percent, px_bias)
	iam_clip_data* clip = find_clip(inst->clip_id);
	if (clip) {
		for (int t = 0; t < clip->iam_tracks.Size; ++t) {
			iam_track& trk = clip->iam_tracks[t];
			if (trk.channel == channel && trk.type == iam_chan_float_rel) {
				// Find the stored percent/px_bias in values_vec2
				for (int i = 0; i < inst->values_vec2.Size; ++i) {
					if (inst->values_vec2[i].ch == channel) {
						float percent = inst->values_vec2[i].v.x;
						float px_bias = inst->values_vec2[i].v.y;
						// Resolve anchor
						ImVec2 anchor = iam_anchor_size_impl(trk.anchor_space);
						float base = (trk.anchor_axis == 0) ? anchor.x : anchor.y;
						*out = base * percent + px_bias;
						return true;
					}
				}
				*out = 0.0f;
				return false;
			}
		}
	}

	// Normal float channel
	*out = inst->values_float.GetFloat(channel, 0.0f);
	return true;
}

bool iam_instance::get_vec2(ImGuiID channel, iam_vec2* out_user) const {
	auto body = [&](ImVec2* out) -> bool {
		using namespace iam_clip_detail;
		iam_instance_data* inst = get_instance_data(m_inst_id);
		if (!inst || !out) return false;

		// Check if this channel is a relative vec2 track (stored as vec4: percent.xy, px_bias.xy)
		iam_clip_data* clip = find_clip(inst->clip_id);
		if (clip) {
			for (int t = 0; t < clip->iam_tracks.Size; ++t) {
				iam_track& trk = clip->iam_tracks[t];
				if (trk.channel == channel && trk.type == iam_chan_vec2_rel) {
					// Find the stored percent/px_bias in values_vec4
					for (int i = 0; i < inst->values_vec4.Size; ++i) {
						if (inst->values_vec4[i].ch == channel) {
							ImVec4& v = inst->values_vec4[i].v;
							ImVec2 percent(v.x, v.y);
							ImVec2 px_bias(v.z, v.w);
							// Resolve anchor
							ImVec2 anchor = iam_anchor_size_impl(trk.anchor_space);
							*out = ImVec2(anchor.x * percent.x + px_bias.x, anchor.y * percent.y + px_bias.y);
							return true;
						}
					}
					*out = ImVec2(0, 0);
					return false;
				}
			}
		}

		// Normal vec2 channel
		for (int i = 0; i < inst->values_vec2.Size; ++i) {
			if (inst->values_vec2[i].ch == channel) {
				*out = inst->values_vec2[i].v;
				return true;
			}
		}
		*out = ImVec2(0, 0);
		return false;
	};
	if (!out_user) return false;
	ImVec2 v = IAM_VEC2_TO_IM(*out_user);  // body may leave it untouched, like the original out pointer
	bool ok = body(&v);
	*out_user = IAM_VEC2_FROM_IM(v);
	return ok;
}

bool iam_instance::get_vec4(ImGuiID channel, iam_vec4* out_user) const {
	auto body = [&](ImVec4* out) -> bool {
		using namespace iam_clip_detail;
		iam_instance_data* inst = get_instance_data(m_inst_id);
		if (!inst || !out) return false;

		// Check if this channel is a relative vec4 track
		iam_clip_data* clip = find_clip(inst->clip_id);
		if (clip) {
			for (int t = 0; t < clip->iam_tracks.Size; ++t) {
				iam_track& trk = clip->iam_tracks[t];
				if (trk.channel == channel && trk.type == iam_chan_vec4_rel) {
					// Find the stored percent/px_bias in values_vec4_rel
					for (int i = 0; i < inst->values_vec4_rel.Size; ++i) {
						if (inst->values_vec4_rel[i].ch == channel) {
							ImVec4 percent = inst->values_vec4_rel[i].percent;
							ImVec4 px_bias = inst->values_vec4_rel[i].px_bias;
							// Resolve anchor - x,y use anchor dimensions, z,w pass through
							ImVec2 anchor = iam_anchor_size_impl(trk.anchor_space);
							*out = ImVec4(
								anchor.x * percent.x + px_bias.x,
								anchor.y * percent.y + px_bias.y,
								percent.z + px_bias.z,  // z,w not anchor-relative
								percent.w + px_bias.w
							);
							return true;
						}
					}
					*out = ImVec4(0, 0, 0, 0);
					return false;
				}
			}
		}

		// Normal vec4 channel
		for (int i = 0; i < inst->values_vec4.Size; ++i) {
			if (inst->values_vec4[i].ch == channel) {
				*out = inst->values_vec4[i].v;
				return true;
			}
		}
		*out = ImVec4(0, 0, 0, 0);
		return false;
	};
	if (!out_user) return false;
	ImVec4 v = IAM_VEC4_TO_IM(*out_user);  // body may leave it untouched, like the original out pointer
	bool ok = body(&v);
	*out_user = IAM_VEC4_FROM_IM(v);
	return ok;
}

bool iam_instance::get_int(ImGuiID channel, int* out) const {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst || !out) return false;
	*out = inst->values_int.GetInt(channel, 0);
	return true;
}

bool iam_instance::get_color(ImGuiID channel, iam_vec4* out_user, int color_space) const {
	auto body = [&](ImVec4* out) -> bool {
		using namespace iam_clip_detail;
		iam_instance_data* inst = get_instance_data(m_inst_id);
		if (!inst || !out) return false;

		// Check if this channel is a relative color track
		iam_clip_data* clip = find_clip(inst->clip_id);
		if (clip) {
			for (int t = 0; t < clip->iam_tracks.Size; ++t) {
				iam_track& trk = clip->iam_tracks[t];
				if (trk.channel == channel && trk.type == iam_chan_color_rel) {
					// Find the stored percent/px_bias in values_color_rel
					for (int i = 0; i < inst->values_color_rel.Size; ++i) {
						if (inst->values_color_rel[i].ch == channel) {
							ImVec4 percent = inst->values_color_rel[i].percent;
							ImVec4 px_bias = inst->values_color_rel[i].px_bias;
							// Resolve anchor - apply to all 4 components (RGBA)
							ImVec2 anchor = iam_anchor_size_impl(trk.anchor_space);
							// For color, use anchor.x for R,B and anchor.y for G,A (or could be all same)
							// Most common use: all components scale the same way
							*out = ImVec4(
								anchor.x * percent.x + px_bias.x,
								anchor.y * percent.y + px_bias.y,
								anchor.x * percent.z + px_bias.z,
								anchor.y * percent.w + px_bias.w
							);
							return true;
						}
					}
					*out = ImVec4(0, 0, 0, 1);
					return false;
				}
			}
		}

		// Normal color channel
		for (int i = 0; i < inst->values_color.Size; ++i) {
			if (inst->values_color[i].ch == channel) {
				// If requested color space differs from stored, convert
				if (color_space != inst->values_color[i].color_space) {
					// Convert from stored sRGB to requested color space and back
					// Note: values are stored in sRGB after lerp_color
					*out = inst->values_color[i].v;
				} else {
					*out = inst->values_color[i].v;
				}
				return true;
			}
		}
		*out = ImVec4(0, 0, 0, 1);
		return false;
	};
	if (!out_user) return false;
	ImVec4 v = IAM_VEC4_TO_IM(*out_user);  // body may leave it untouched, like the original out pointer
	bool ok = body(&v);
	*out_user = IAM_VEC4_FROM_IM(v);
	return ok;
}

// ----------------------------------------------------
// Clip System API implementation
// ----------------------------------------------------

void iam_clip_init(int initial_clip_cap, int initial_inst_cap) {
	using namespace iam_clip_detail;
	if (g_clip_sys.initialized) return;
	g_clip_sys.clips.reserve(initial_clip_cap);
	g_clip_sys.instances.reserve(initial_inst_cap);
	g_clip_sys.initialized = true;
}

static void iam_clip_editor_free_states();  // Clip editor section

void iam_clip_shutdown() {
	using namespace iam_clip_detail;
	iam_clip_editor_free_states();
	g_clip_sys.clips.clear();
	g_clip_sys.instances.clear();
	g_clip_sys.clip_map.Clear();
	g_clip_sys.inst_map.Clear();
	g_clip_sys.initialized = false;
}

void iam_clip_update(float dt) {
	using namespace iam_clip_detail;
	g_clip_sys.frame_counter++;

	// Apply global time scale
	dt *= iam_detail::g_time_scale;

	// Safety: clamp dt to reasonable range
	if (dt < 0.0f) dt = 0.0f;
	if (dt > 1.0f) dt = 1.0f;

	for (int i = 0; i < g_clip_sys.instances.Size; ++i) {
		iam_instance_data* inst = &g_clip_sys.instances[i];
		iam_clip_data* clip = find_clip(inst->clip_id);
		if (!clip) continue;

		// Playhead tween (tween_to): move the playhead itself, then hold (paused) at the target
		if (inst->tw_active) {
			inst->tw_elapsed += dt;
			float u = inst->tw_dur > 1e-6f ? ImMin(inst->tw_elapsed / inst->tw_dur, 1.0f) : 1.0f;
			float w = (u >= 1.0f) ? 1.0f : iam_detail::eval_preset_internal(inst->tw_ease, u);
			float t = (u >= 1.0f) ? inst->tw_to : ImClamp(inst->tw_from + (inst->tw_to - inst->tw_from) * w, 0.0f, clip->duration);
			float t_min = ImMin(inst->prev_time, t), t_max = ImMax(inst->prev_time, t);
			if (inst->markers_triggered.Size != clip->markers.Size) {
				inst->markers_triggered.resize(clip->markers.Size);
				for (int m = 0; m < inst->markers_triggered.Size; m++) inst->markers_triggered[m] = false;
			}
			for (int m = 0; m < clip->markers.Size; m++) {
				iam_marker const& marker = clip->markers[m];
				if (marker.time >= t_min && marker.time <= t_max && t_max > t_min && marker.callback)
					marker.callback(inst->inst_id, marker.marker_id, marker.time, marker.user_data);
			}
			inst->time = inst->prev_time = t;
			for (int tr = 0; tr < clip->iam_tracks.Size; ++tr) eval_iam_track(clip->iam_tracks[tr], t, inst);
			inst->last_seen_frame = g_clip_sys.frame_counter;
			if (u >= 1.0f) { inst->tw_active = false; inst->playing = true; inst->paused = true; }
			continue;
		}
		if (!inst->playing || inst->paused) continue;

		// Use local copy of dt for this instance to avoid affecting other instances
		float inst_dt = dt;

		// Handle delay
		if (inst->delay_left > 0.0f) {
			inst->delay_left -= inst_dt;
			if (inst->delay_left > 0.0f) {
				// Still evaluate tracks at t=0 so values are readable during delay
				for (int tr = 0; tr < clip->iam_tracks.Size; ++tr) {
					eval_iam_track(clip->iam_tracks[tr], 0.0f, inst);
				}
				inst->last_seen_frame = g_clip_sys.frame_counter;
				continue;
			}
			inst_dt = -inst->delay_left;
			inst->delay_left = 0.0f;
			// Call on_begin when delay expires
			if (!inst->begin_called && clip->cb_begin) {
				inst->begin_called = true;
				clip->cb_begin(inst->inst_id, clip->cb_begin_user);
			}
		}

		// Call on_begin on first frame if no delay
		if (!inst->begin_called && clip->cb_begin) {
			inst->begin_called = true;
			clip->cb_begin(inst->inst_id, clip->cb_begin_user);
		}

		float t = inst->time;
		float dts = inst_dt * (inst->time_scale <= 0.0f ? 1.0f : inst->time_scale);
		t += dts * (float)inst->dir_sign;

		// Apply duration variation if present
		float dur = clip->duration;
		if (clip->has_duration_var) {
			dur = apply_var_float(clip->duration, clip->duration_var, inst->current_loop, &inst->var_rng_state);
			if (dur < 0.001f) dur = 0.001f;  // Minimum duration
		}
		bool done = false;

		if (dur <= 0.0f) { inst->time = 0.0f; continue; }

		// Handle looping (with safety limit to prevent infinite loops)
		int const MAX_LOOP_ITERS = 1000;
		int loop_iters = 0;
		if (clip->direction == iam_dir_alternate) {
			while ((t < 0.0f || t > dur) && loop_iters < MAX_LOOP_ITERS) {
				if (clip->loop_count == 0 && inst->loops_left == 0) { done = true; break; }
				if (inst->loops_left > 0) inst->loops_left--;
				inst->dir_sign = -inst->dir_sign;
				if (t < 0.0f) t = -t;
				if (t > dur) t = 2*dur - t;
				loop_iters++;
			}
		} else { // normal / reverse: wrap at the end we're moving toward (either direction, see reverse())
			while ((t > dur || t < 0.0f) && loop_iters < MAX_LOOP_ITERS) {
				if (clip->loop_count == 0 && inst->loops_left == 0) { done = true; break; }
				if (inst->loops_left > 0) inst->loops_left--;
				t += (t > dur) ? -dur : dur;
				loop_iters++;
			}
		}
		// Safety clamp
		if (t < 0.0f) t = 0.0f;
		if (t > dur) t = dur;

		// Reset markers on loop and increment loop counter for variation
		if (loop_iters > 0) {
			inst->current_loop += loop_iters;
			for (int m = 0; m < inst->markers_triggered.Size; m++) {
				inst->markers_triggered[m] = false;
			}
			// Reset prev_time to avoid triggering all markers at once after loop
			inst->prev_time = (inst->dir_sign > 0) ? 0.0f : dur;

			// Fire on_loop callback
			if (clip->cb_loop) {
				clip->cb_loop(inst->inst_id, inst->current_loop, clip->cb_loop_user);
			}

			// Apply loop delay (fixed delay between iterations)
			if (clip->loop_delay > 0.0f) {
				inst->delay_left += clip->loop_delay;
			}

			// Apply timing variations for new loop iteration
			if (clip->has_timescale_var) {
				float new_scale = apply_var_float(1.0f, clip->timescale_var, inst->current_loop, &inst->var_rng_state);
				inst->time_scale = new_scale > 0.0f ? new_scale : 1.0f;
			}
			if (clip->has_delay_var) {
				float var_delay = apply_var_float(0.0f, clip->delay_var, inst->current_loop, &inst->var_rng_state);
				if (var_delay > 0.0f) {
					inst->delay_left += var_delay;
				}
			}
		}

		if (done) {
			inst->playing = false;
			inst->time = (inst->dir_sign > 0) ? dur : 0.0f;
			// Evaluate final frame
			for (int tr = 0; tr < clip->iam_tracks.Size; ++tr) {
				eval_iam_track(clip->iam_tracks[tr], inst->time, inst);
			}
			inst->last_seen_frame = g_clip_sys.frame_counter;
			if (clip->cb_complete)
				clip->cb_complete(inst->inst_id, clip->cb_complete_user);

			// Start chained clip if any
			if (inst->chain_next_clip_id != 0) {
				ImGuiID next_clip = inst->chain_next_clip_id;
				ImGuiID next_inst = inst->chain_next_inst_id;
				float chain_delay = inst->chain_delay;

				// Clear the chain to prevent re-triggering
				inst->chain_next_clip_id = 0;
				inst->chain_next_inst_id = 0;
				inst->chain_delay = 0;

				// Play the chained clip
				iam_instance next = iam_play(next_clip, next_inst);
				if (next.valid() && chain_delay > 0) {
					// Apply chain delay
					iam_instance_data* next_data = find_instance(next_inst);
					if (next_data) {
						next_data->delay_left += chain_delay;
					}
				}
			}
			continue;
		}

		// Check for markers crossed between prev_time and t
		float prev_t = inst->prev_time;
		inst->time = t;

		// Initialize markers_triggered if needed
		if (inst->markers_triggered.Size != clip->markers.Size) {
			inst->markers_triggered.resize(clip->markers.Size);
			for (int m = 0; m < inst->markers_triggered.Size; m++) {
				inst->markers_triggered[m] = false;
			}
		}

		// Fire markers that were crossed
		// Handle both forward and backward playback
		float t_min = (prev_t < t) ? prev_t : t;
		float t_max = (prev_t < t) ? t : prev_t;
		for (int m = 0; m < clip->markers.Size; m++) {
			iam_marker const& marker = clip->markers[m];
			if (!inst->markers_triggered[m] && marker.time >= t_min && marker.time <= t_max) {
				inst->markers_triggered[m] = true;
				if (marker.callback) {
					marker.callback(inst->inst_id, marker.marker_id, marker.time, marker.user_data);
				}
			}
		}

		inst->prev_time = t;

		// Evaluate all iam_tracks
		for (int tr = 0; tr < clip->iam_tracks.Size; ++tr) {
			eval_iam_track(clip->iam_tracks[tr], t, inst);
		}

		if (clip->cb_update)
			clip->cb_update(inst->inst_id, clip->cb_update_user);

		inst->last_seen_frame = g_clip_sys.frame_counter;
	}
}

void iam_clip_gc(unsigned int max_age_frames) {
	using namespace iam_clip_detail;
	for (int i = 0; i < g_clip_sys.instances.Size; ++i) {
		iam_instance_data* inst = &g_clip_sys.instances[i];
		if (g_clip_sys.frame_counter - inst->last_seen_frame > max_age_frames) {
			g_clip_sys.inst_map.SetInt(inst->inst_id, 0);
			// Swap with last and remove
			g_clip_sys.instances[i] = g_clip_sys.instances[g_clip_sys.instances.Size - 1];
			g_clip_sys.instances.pop_back();
			// Update swapped instance's map entry
			if (i < g_clip_sys.instances.Size) {
				g_clip_sys.inst_map.SetInt(g_clip_sys.instances[i].inst_id, i + 1);
			}
			i--;
		}
	}
}

iam_instance iam_play(ImGuiID clip_id, ImGuiID instance_id) {
	using namespace iam_clip_detail;
	if (!g_clip_sys.initialized) iam_clip_init();

	iam_clip_data* clip = find_clip(clip_id);
	if (!clip) return iam_instance(0);

	int idx = g_clip_sys.inst_map.GetInt(instance_id, 0);
	iam_instance_data* inst;
	if (idx == 0) {
		g_clip_sys.instances.push_back(iam_instance_data());
		inst = &g_clip_sys.instances.back();
		inst->inst_id = instance_id;
		g_clip_sys.inst_map.SetInt(instance_id, g_clip_sys.instances.Size);
	} else {
		inst = &g_clip_sys.instances[idx - 1];
	}

	inst->clip_id = clip_id;  // Store ID instead of pointer
	inst->time = 0.0f;
	inst->time_scale = 1.0f;
	inst->weight = 1.0f;
	inst->delay_left = clip->delay;
	inst->playing = true;
	inst->paused = false;
	inst->begin_called = false;  // Reset so on_begin will be called
	inst->dir_sign = (clip->direction == iam_dir_reverse) ? -1 : 1;
	inst->time = (inst->dir_sign > 0) ? 0.0f : clip->duration;  // reverse plays from the end
	inst->loops_left = clip->loop_count;
	inst->last_seen_frame = g_clip_sys.frame_counter;
	inst->tw_active = false;

	// Initialize marker tracking
	inst->prev_time = (inst->dir_sign > 0) ? 0.0f : clip->duration;
	inst->markers_triggered.resize(clip->markers.Size);
	for (int m = 0; m < inst->markers_triggered.Size; m++) {
		inst->markers_triggered[m] = false;
	}

	// Reset chaining (can be set after iam_play using .then())
	inst->chain_next_clip_id = 0;
	inst->chain_next_inst_id = 0;
	inst->chain_delay = 0;

	// Reset variation state
	inst->current_loop = 0;
	inst->var_rng_state = 12345 + instance_id;  // Deterministic but unique per instance

	// Evaluate initial frame immediately so values are available right away
	float initial_time = (inst->dir_sign > 0) ? 0.0f : clip->duration;
	for (int tr = 0; tr < clip->iam_tracks.Size; ++tr) {
		eval_iam_track(clip->iam_tracks[tr], initial_time, inst);
	}

	return iam_instance(instance_id);  // Return iam_instance with ID
}

iam_instance iam_get_instance(ImGuiID instance_id) {
	using namespace iam_clip_detail;
	iam_instance_data* inst = find_instance(instance_id);
	return inst ? iam_instance(instance_id) : iam_instance(0);
}

float iam_clip_duration(ImGuiID clip_id) {
	using namespace iam_clip_detail;
	iam_clip_data* clip = find_clip(clip_id);
	return clip ? clip->duration : 0.0f;
}

bool iam_clip_exists(ImGuiID clip_id) {
	using namespace iam_clip_detail;
	return find_clip(clip_id) != nullptr;
}

// Stagger helpers
float iam_stagger_delay(ImGuiID clip_id, int index) {
	using namespace iam_clip_detail;
	iam_clip_data* clip = find_clip(clip_id);
	if (!clip || clip->stagger_count <= 1) return 0.0f;

	int count = clip->stagger_count;
	float delay = clip->stagger_delay;
	float bias = clip->stagger_center_bias;

	// Calculate raw delay based on index and center bias
	// bias = 0: start from beginning (index 0 has 0 delay)
	// bias = 1: start from center (center indices have 0 delay, edges have max delay)
	// bias = 0.5: mixed
	float raw_delay = 0.0f;
	if (bias <= 0.0f) {
		// Simple linear stagger from start
		raw_delay = (float)index * delay;
	} else {
		// Stagger from center
		float center = (float)(count - 1) * 0.5f;
		float dist_from_center = fabsf((float)index - center);
		float max_dist = center;
		if (max_dist > 0.0f) {
			float linear_delay = (float)index * delay;
			float center_delay = dist_from_center * delay * 2.0f / (float)count * (float)(count - 1);
			raw_delay = linear_delay * (1.0f - bias) + center_delay * bias;
		}
	}

	// Apply stagger easing to the delay distribution
	if (clip->stagger_ease != iam_ease_linear && count > 1) {
		float max_delay = (float)(count - 1) * delay;
		if (max_delay > 0.0f) {
			float t = raw_delay / max_delay;
			iam_ease_desc ez = { clip->stagger_ease, 0, 0, 0, 0 };
			t = iam_detail::eval(ez, t);
			raw_delay = t * max_delay;
		}
	}

	return raw_delay;
}

iam_instance iam_play_stagger(ImGuiID clip_id, ImGuiID instance_id, int index) {
	using namespace iam_clip_detail;
	if (!g_clip_sys.initialized) iam_clip_init();

	iam_clip_data* clip = find_clip(clip_id);
	if (!clip) return iam_instance(0);

	// Play with base delay + stagger delay
	iam_instance inst = iam_play(clip_id, instance_id);
	if (inst.valid()) {
		iam_instance_data* inst_data = find_instance(instance_id);
		if (inst_data) {
			inst_data->delay_left = clip->delay + iam_stagger_delay(clip_id, index);
		}
	}
	return inst;
}

// Grid stagger - 2D grid-based delay distribution
// Deterministic random in [-1, 1] per (index, seed)
static float stagger_jitter(int index, unsigned seed) {
	unsigned h = (unsigned)index * 0x9E3779B9u ^ (seed + 0x7F4A7C15u);
	h ^= h >> 16; h *= 0x85EBCA6Bu; h ^= h >> 13; h *= 0xC2B2AE35u; h ^= h >> 16;
	return (float)(h & 0xFFFFFF) / (float)0x7FFFFF - 1.0f;
}

float iam_stagger_grid_delay(int col, int row, iam_stagger_grid_opts const& opts) {
	int cols = opts.cols > 0 ? opts.cols : 1;
	int rows = opts.rows > 0 ? opts.rows : 1;
	int total = cols * rows;
	if (total <= 1) return opts.start_delay;

	// Determine origin position
	float origin_col = 0.0f, origin_row = 0.0f;
	switch (opts.from) {
		case iam_stagger_first:
			origin_col = 0.0f;
			origin_row = 0.0f;
			break;
		case iam_stagger_last:
			origin_col = (float)(cols - 1);
			origin_row = (float)(rows - 1);
			break;
		case iam_stagger_center:
			origin_col = (float)(cols - 1) * 0.5f;
			origin_row = (float)(rows - 1) * 0.5f;
			break;
		case iam_stagger_index: {
			int idx = ImClamp(opts.from_index, 0, total - 1);
			origin_col = (float)(idx % cols);
			origin_row = (float)(idx / cols);
			break;
		}
		default:
			break;
	}

	// Calculate distance based on axis constraint
	float dist = 0.0f;
	switch (opts.axis) {
		case iam_stagger_x:
			dist = fabsf((float)col - origin_col);
			break;
		case iam_stagger_y:
			dist = fabsf((float)row - origin_row);
			break;
		case iam_stagger_both:
		default:
			// Euclidean distance for radial effect
			float dx = (float)col - origin_col;
			float dy = (float)row - origin_row;
			dist = ImSqrt(dx * dx + dy * dy);
			break;
	}

	// Calculate max possible distance for normalization
	float max_dist = 0.0f;
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			float d = 0.0f;
			switch (opts.axis) {
				case iam_stagger_x:
					d = fabsf((float)c - origin_col);
					break;
				case iam_stagger_y:
					d = fabsf((float)r - origin_row);
					break;
				case iam_stagger_both:
				default: {
					float ddx = (float)c - origin_col;
					float ddy = (float)r - origin_row;
					d = ImSqrt(ddx * ddx + ddy * ddy);
					break;
				}
			}
			if (d > max_dist) max_dist = d;
		}
	}

	// Normalize and apply easing
	float t = (max_dist > 0.0f) ? (dist / max_dist) : 0.0f;
	if (opts.reversed) t = 1.0f - t;
	if (opts.ease != iam_ease_linear) {
		iam_ease_desc ez = { opts.ease, 0, 0, 0, 0 };
		t = iam_detail::eval(ez, t);
	}

	float delay = opts.start_delay + t * opts.delay * (float)(total - 1);
	if (opts.jitter != 0.0f) delay = ImMax(delay + stagger_jitter(row * cols + col, opts.seed) * opts.jitter, 0.0f);
	return delay;
}

float iam_stagger_value(int col, int row, float start, float end, iam_stagger_grid_opts const& opts, float jitter) {
	int cols = opts.cols > 0 ? opts.cols : 1;
	int rows = opts.rows > 0 ? opts.rows : 1;
	int total = cols * rows;
	float t = 0.0f;
	if (total > 1) {
		// Same distance/ease/reversed as the delays, normalized to [0,1]
		iam_stagger_grid_opts o = opts;
		o.start_delay = 0.0f;
		o.delay = 1.0f / (float)(total - 1);
		o.jitter = 0.0f;
		t = iam_stagger_grid_delay(col, row, o);
	}
	float v = start + (end - start) * t;
	if (jitter != 0.0f) v += stagger_jitter(row * cols + col, opts.seed ^ 0xA5A5A5A5u) * jitter;
	return v;
}

float iam_stagger_value_index(int index, float start, float end, iam_stagger_grid_opts const& opts, float jitter) {
	int cols = opts.cols > 0 ? opts.cols : 1;
	return iam_stagger_value(index % cols, index / cols, start, end, opts, jitter);
}

float iam_stagger_grid_delay_index(int index, iam_stagger_grid_opts const& opts) {
	int cols = opts.cols > 0 ? opts.cols : 1;
	int col = index % cols;
	int row = index / cols;
	return iam_stagger_grid_delay(col, row, opts);
}

iam_instance iam_play_with_delay(ImGuiID clip_id, ImGuiID instance_id, float delay) {
	using namespace iam_clip_detail;
	if (!g_clip_sys.initialized) iam_clip_init();

	iam_clip_data* clip = find_clip(clip_id);
	if (!clip) return iam_instance(0);

	iam_instance inst = iam_play(clip_id, instance_id);
	if (inst.valid()) {
		iam_instance_data* inst_data = find_instance(instance_id);
		if (inst_data)
			inst_data->delay_left = delay;
	}
	return inst;
}

// Layering support - blends multiple instance outputs into one
static struct {
	ImGuiID target_id;
	float total_weight;
	// Accumulated weighted values
	ImGuiStorage acc_float;
	ImGuiStorage acc_int;
	ImVector<iam_instance_data::vec2_entry> acc_vec2;
	ImVector<iam_instance_data::vec4_entry> acc_vec4;
	// Weight sums per channel (for normalization)
	ImGuiStorage weight_float;
	ImGuiStorage weight_int;
	ImVector<iam_instance_data::vec2_entry> weight_vec2;  // stores weight in v.x
	ImVector<iam_instance_data::vec4_entry> weight_vec4;  // stores weight in v.x
} g_layer_state = { 0, 0 };

void iam_layer_begin(ImGuiID instance_id) {
	g_layer_state.target_id = instance_id;
	g_layer_state.total_weight = 0.0f;
	g_layer_state.acc_float.Clear();
	g_layer_state.acc_int.Clear();
	g_layer_state.acc_vec2.clear();
	g_layer_state.acc_vec4.clear();
	g_layer_state.weight_float.Clear();
	g_layer_state.weight_int.Clear();
	g_layer_state.weight_vec2.clear();
	g_layer_state.weight_vec4.clear();
}

void iam_layer_add(iam_instance inst, float weight) {
	using namespace iam_clip_detail;
	if (!inst.valid() || weight <= 0.0f) return;

	iam_instance_data* src = find_instance(inst.id());
	if (!src) return;

	g_layer_state.total_weight += weight;

	// Accumulate float values
	for (int i = 0; i < src->values_float.Data.Size; ++i) {
		IMGUI_STORAGE_PAIR& p = src->values_float.Data[i];
		ImGuiID ch = p.key;
		float val = *(float*)&p.val_i;
		float acc = g_layer_state.acc_float.GetFloat(ch, 0.0f);
		float w = g_layer_state.weight_float.GetFloat(ch, 0.0f);
		g_layer_state.acc_float.SetFloat(ch, acc + val * weight);
		g_layer_state.weight_float.SetFloat(ch, w + weight);
	}

	// Accumulate int values
	for (int i = 0; i < src->values_int.Data.Size; ++i) {
		IMGUI_STORAGE_PAIR& p = src->values_int.Data[i];
		ImGuiID ch = p.key;
		int val = p.val_i;
		float acc = (float)g_layer_state.acc_int.GetInt(ch, 0);
		float w = g_layer_state.weight_int.GetFloat(ch, 0.0f);
		g_layer_state.acc_int.SetInt(ch, (int)(acc + (float)val * weight));
		g_layer_state.weight_int.SetFloat(ch, w + weight);
	}

	// Accumulate vec2 values
	for (int i = 0; i < src->values_vec2.Size; ++i) {
		iam_instance_data::vec2_entry& e = src->values_vec2[i];
		// Find or create accumulator entry
		int found = -1;
		for (int j = 0; j < g_layer_state.acc_vec2.Size; ++j) {
			if (g_layer_state.acc_vec2[j].ch == e.ch) { found = j; break; }
		}
		if (found < 0) {
			iam_instance_data::vec2_entry acc_e = { e.ch, ImVec2(0, 0) };
			iam_instance_data::vec2_entry w_e = { e.ch, ImVec2(0, 0) };
			g_layer_state.acc_vec2.push_back(acc_e);
			g_layer_state.weight_vec2.push_back(w_e);
			found = g_layer_state.acc_vec2.Size - 1;
		}
		g_layer_state.acc_vec2[found].v.x += e.v.x * weight;
		g_layer_state.acc_vec2[found].v.y += e.v.y * weight;
		g_layer_state.weight_vec2[found].v.x += weight;
	}

	// Accumulate vec4 values
	for (int i = 0; i < src->values_vec4.Size; ++i) {
		iam_instance_data::vec4_entry& e = src->values_vec4[i];
		int found = -1;
		for (int j = 0; j < g_layer_state.acc_vec4.Size; ++j) {
			if (g_layer_state.acc_vec4[j].ch == e.ch) { found = j; break; }
		}
		if (found < 0) {
			iam_instance_data::vec4_entry acc_e = { e.ch, ImVec4(0, 0, 0, 0) };
			iam_instance_data::vec4_entry w_e = { e.ch, ImVec4(0, 0, 0, 0) };
			g_layer_state.acc_vec4.push_back(acc_e);
			g_layer_state.weight_vec4.push_back(w_e);
			found = g_layer_state.acc_vec4.Size - 1;
		}
		g_layer_state.acc_vec4[found].v.x += e.v.x * weight;
		g_layer_state.acc_vec4[found].v.y += e.v.y * weight;
		g_layer_state.acc_vec4[found].v.z += e.v.z * weight;
		g_layer_state.acc_vec4[found].v.w += e.v.w * weight;
		g_layer_state.weight_vec4[found].v.x += weight;
	}
}

void iam_layer_end(ImGuiID instance_id) {
	using namespace iam_clip_detail;
	if (g_layer_state.target_id != instance_id) return;
	if (g_layer_state.total_weight <= 0.0f) return;

	iam_instance_data* target = find_instance(instance_id);
	if (!target) return;

	// Normalize and write blended values
	target->blended_float.Clear();
	target->blended_int.Clear();
	target->blended_vec2.clear();
	target->blended_vec4.clear();

	// Floats
	for (int i = 0; i < g_layer_state.acc_float.Data.Size; ++i) {
		IMGUI_STORAGE_PAIR& p = g_layer_state.acc_float.Data[i];
		float w = g_layer_state.weight_float.GetFloat(p.key, 1.0f);
		float val = *(float*)&p.val_i / (w > 0.0f ? w : 1.0f);
		target->blended_float.SetFloat(p.key, val);
	}

	// Ints
	for (int i = 0; i < g_layer_state.acc_int.Data.Size; ++i) {
		IMGUI_STORAGE_PAIR& p = g_layer_state.acc_int.Data[i];
		float w = g_layer_state.weight_int.GetFloat(p.key, 1.0f);
		int val = (int)((float)p.val_i / (w > 0.0f ? w : 1.0f));
		target->blended_int.SetInt(p.key, val);
	}

	// Vec2s
	for (int i = 0; i < g_layer_state.acc_vec2.Size; ++i) {
		iam_instance_data::vec2_entry& e = g_layer_state.acc_vec2[i];
		float w = g_layer_state.weight_vec2[i].v.x;
		if (w <= 0.0f) w = 1.0f;
		iam_instance_data::vec2_entry out = { e.ch, ImVec2(e.v.x / w, e.v.y / w) };
		target->blended_vec2.push_back(out);
	}

	// Vec4s
	for (int i = 0; i < g_layer_state.acc_vec4.Size; ++i) {
		iam_instance_data::vec4_entry& e = g_layer_state.acc_vec4[i];
		float w = g_layer_state.weight_vec4[i].v.x;
		if (w <= 0.0f) w = 1.0f;
		iam_instance_data::vec4_entry out = { e.ch, ImVec4(e.v.x / w, e.v.y / w, e.v.z / w, e.v.w / w) };
		target->blended_vec4.push_back(out);
	}

	target->has_blended = true;
	g_layer_state.target_id = 0;
}

bool iam_get_blended_float(ImGuiID instance_id, ImGuiID channel, float* out) {
	using namespace iam_clip_detail;
	iam_instance_data* inst = find_instance(instance_id);
	if (!inst || !inst->has_blended || !out) return false;
	// Check if channel exists in blended data
	for (int i = 0; i < inst->blended_float.Data.Size; ++i) {
		if (inst->blended_float.Data[i].key == channel) {
			*out = *(float*)&inst->blended_float.Data[i].val_i;
			return true;
		}
	}
	return false;
}

static bool iam_get_blended_vec2_impl(ImGuiID instance_id, ImGuiID channel, ImVec2* out) {
	using namespace iam_clip_detail;
	iam_instance_data* inst = find_instance(instance_id);
	if (!inst || !inst->has_blended || !out) return false;
	for (int i = 0; i < inst->blended_vec2.Size; ++i) {
		if (inst->blended_vec2[i].ch == channel) {
			*out = inst->blended_vec2[i].v;
			return true;
		}
	}
	return false;
}

static bool iam_get_blended_vec4_impl(ImGuiID instance_id, ImGuiID channel, ImVec4* out) {
	using namespace iam_clip_detail;
	iam_instance_data* inst = find_instance(instance_id);
	if (!inst || !inst->has_blended || !out) return false;
	for (int i = 0; i < inst->blended_vec4.Size; ++i) {
		if (inst->blended_vec4[i].ch == channel) {
			*out = inst->blended_vec4[i].v;
			return true;
		}
	}
	return false;
}

bool iam_get_blended_int(ImGuiID instance_id, ImGuiID channel, int* out) {
	using namespace iam_clip_detail;
	iam_instance_data* inst = find_instance(instance_id);
	if (!inst || !inst->has_blended || !out) return false;
	for (int i = 0; i < inst->blended_int.Data.Size; ++i) {
		if (inst->blended_int.Data[i].key == channel) {
			*out = inst->blended_int.Data[i].val_i;
			return true;
		}
	}
	return false;
}

// Persistence - binary format
// Header: "IAMC" (4 bytes) + version (4 bytes) + clip_id (4 bytes)
// Clip data: duration, delay, loop_count, direction, stagger params
// Tracks: count + for each: channel, type, num_keys, keys...

static char const IAM_CLIP_MAGIC[4] = { 'I', 'A', 'M', 'C' };
static int const IAM_CLIP_VERSION = 6;      // 4: + loop delay, track color space/anchor, key color space/value_ext; 5: + labels; 6: + curve eases
static int const IAM_CLIP_VERSION_MIN = 3;  // Oldest version iam_clip_load() accepts

iam_result iam_clip_save(ImGuiID clip_id, char const* path) {
	using namespace iam_clip_detail;
	iam_clip_data* clip = find_clip(clip_id);
	if (!clip) return iam_err_not_found;
	if (!path) return iam_err_bad_arg;

	FILE* f = fopen(path, "wb");
	if (!f) return iam_err_bad_arg;

	// Write header
	fwrite(IAM_CLIP_MAGIC, 1, 4, f);
	int version = IAM_CLIP_VERSION;
	fwrite(&version, sizeof(int), 1, f);
	fwrite(&clip_id, sizeof(ImGuiID), 1, f);

	// Write clip properties
	fwrite(&clip->duration, sizeof(float), 1, f);
	fwrite(&clip->delay, sizeof(float), 1, f);
	fwrite(&clip->loop_count, sizeof(int), 1, f);
	fwrite(&clip->direction, sizeof(int), 1, f);
	fwrite(&clip->stagger_count, sizeof(int), 1, f);
	fwrite(&clip->stagger_delay, sizeof(float), 1, f);
	fwrite(&clip->stagger_center_bias, sizeof(float), 1, f);
	fwrite(&clip->loop_delay, sizeof(float), 1, f);

	// Write tracks
	int track_count = clip->iam_tracks.Size;
	fwrite(&track_count, sizeof(int), 1, f);

	for (int t = 0; t < track_count; ++t) {
		iam_track& trk = clip->iam_tracks[t];
		fwrite(&trk.channel, sizeof(ImGuiID), 1, f);
		fwrite(&trk.type, sizeof(int), 1, f);
		int is_relative_i = trk.is_relative ? 1 : 0;
		fwrite(&trk.color_space, sizeof(int), 1, f);
		fwrite(&is_relative_i, sizeof(int), 1, f);
		fwrite(&trk.anchor_space, sizeof(int), 1, f);
		fwrite(&trk.anchor_axis, sizeof(int), 1, f);

		int key_count = trk.keys.Size;
		fwrite(&key_count, sizeof(int), 1, f);

		for (int k = 0; k < key_count; ++k) {
			keyframe& kf = trk.keys[k];
			fwrite(&kf.time, sizeof(float), 1, f);
			fwrite(&kf.ease_type, sizeof(int), 1, f);
			// Write bools as int to avoid size/alignment issues
			int has_bezier_i = kf.has_bezier ? 1 : 0;
			int is_spring_i = kf.is_spring ? 1 : 0;
			fwrite(&has_bezier_i, sizeof(int), 1, f);
			fwrite(kf.bezier, sizeof(float), 4, f);
			fwrite(&is_spring_i, sizeof(int), 1, f);
			fwrite(&kf.spring.mass, sizeof(float), 1, f);
			fwrite(&kf.spring.stiffness, sizeof(float), 1, f);
			fwrite(&kf.spring.damping, sizeof(float), 1, f);
			fwrite(&kf.spring.initial_velocity, sizeof(float), 1, f);
			fwrite(kf.value, sizeof(float), 4, f);
			fwrite(&kf.color_space, sizeof(int), 1, f);
			fwrite(kf.value_ext, sizeof(float), 4, f);
		}
	}

	// Labels (v5)
	int label_count = clip->labels.Size;
	fwrite(&label_count, sizeof(int), 1, f);
	for (int l = 0; l < label_count; ++l) {
		fwrite(&clip->labels[l].id, sizeof(ImGuiID), 1, f);
		fwrite(&clip->labels[l].time, sizeof(float), 1, f);
	}

	// Curve eases used by keys (v6)
	ImVector<ImGuiID> curves;
	for (int t = 0; t < track_count; ++t)
		for (int k = 0; k < clip->iam_tracks[t].keys.Size; ++k) {
			keyframe const& kf = clip->iam_tracks[t].keys[k];
			if (kf.ease_type != iam_ease_curve) continue;
			ImGuiID cid = iam_detail::curve_id_from(kf.bezier[0], kf.bezier[1]);
			if (iam_detail::find_curve(cid) && !curves.contains(cid)) curves.push_back(cid);
		}
	int curve_count = curves.Size;
	fwrite(&curve_count, sizeof(int), 1, f);
	for (int c = 0; c < curve_count; ++c) {
		iam_detail::ease_curve* ec = iam_detail::find_curve(curves[c]);
		int n = ec->pts.Size;
		fwrite(&curves[c], sizeof(ImGuiID), 1, f);
		fwrite(&n, sizeof(int), 1, f);
		for (int i = 0; i < n; ++i) { fwrite(&ec->pts[i].x, sizeof(float), 1, f); fwrite(&ec->pts[i].y, sizeof(float), 1, f); }
	}

	// Note: callbacks cannot be serialized

	fclose(f);
	return iam_ok;
}

iam_result iam_clip_load(char const* path, ImGuiID* out_clip_id) {
	using namespace iam_clip_detail;
	if (!path || !out_clip_id) return iam_err_bad_arg;

	FILE* f = fopen(path, "rb");
	if (!f) return iam_err_not_found;

	// Read and verify header
	char magic[4];
	if (fread(magic, 1, 4, f) != 4 || memcmp(magic, IAM_CLIP_MAGIC, 4) != 0) {
		fclose(f);
		return iam_err_bad_arg;
	}

	int version;
	if (fread(&version, sizeof(int), 1, f) != 1 || version < IAM_CLIP_VERSION_MIN || version > IAM_CLIP_VERSION) {
		fclose(f);
		return iam_err_bad_arg;
	}

	ImGuiID clip_id;
	if (fread(&clip_id, sizeof(ImGuiID), 1, f) != 1) {
		fclose(f);
		return iam_err_bad_arg;
	}

	// Initialize system if needed
	if (!g_clip_sys.initialized) iam_clip_init();

	// Create or get clip
	int idx = g_clip_sys.clip_map.GetInt(clip_id, 0);
	iam_clip_data* clip;
	if (idx == 0) {
		g_clip_sys.clips.push_back(iam_clip_data());
		clip = &g_clip_sys.clips.back();
		clip->id = clip_id;
		g_clip_sys.clip_map.SetInt(clip_id, g_clip_sys.clips.Size);
	} else {
		clip = &g_clip_sys.clips[idx - 1];
		clip->iam_tracks.clear();
	}

	// Read clip properties
	fread(&clip->duration, sizeof(float), 1, f);
	fread(&clip->delay, sizeof(float), 1, f);
	fread(&clip->loop_count, sizeof(int), 1, f);
	fread(&clip->direction, sizeof(int), 1, f);
	fread(&clip->stagger_count, sizeof(int), 1, f);
	fread(&clip->stagger_delay, sizeof(float), 1, f);
	fread(&clip->stagger_center_bias, sizeof(float), 1, f);
	clip->loop_delay = 0.0f;
	if (version >= 4) fread(&clip->loop_delay, sizeof(float), 1, f);

	// Read tracks
	int track_count;
	if (fread(&track_count, sizeof(int), 1, f) != 1) {
		fclose(f);
		return iam_err_bad_arg;
	}

	for (int t = 0; t < track_count; ++t) {
		// Add track to clip first, then work with it directly to avoid ImVector copy issues
		clip->iam_tracks.push_back(iam_track());
		iam_track& trk = clip->iam_tracks.back();

		fread(&trk.channel, sizeof(ImGuiID), 1, f);
		fread(&trk.type, sizeof(int), 1, f);
		if (version >= 4) {
			int is_relative_i = 0;
			fread(&trk.color_space, sizeof(int), 1, f);
			fread(&is_relative_i, sizeof(int), 1, f);
			fread(&trk.anchor_space, sizeof(int), 1, f);
			fread(&trk.anchor_axis, sizeof(int), 1, f);
			trk.is_relative = (is_relative_i != 0);
		}

		int key_count;
		fread(&key_count, sizeof(int), 1, f);

		for (int k = 0; k < key_count; ++k) {
			keyframe kf;
			fread(&kf.time, sizeof(float), 1, f);
			fread(&kf.ease_type, sizeof(int), 1, f);
			// Read bools as int to match save format
			int has_bezier_i, is_spring_i;
			fread(&has_bezier_i, sizeof(int), 1, f);
			fread(kf.bezier, sizeof(float), 4, f);
			fread(&is_spring_i, sizeof(int), 1, f);
			fread(&kf.spring.mass, sizeof(float), 1, f);
			fread(&kf.spring.stiffness, sizeof(float), 1, f);
			fread(&kf.spring.damping, sizeof(float), 1, f);
			fread(&kf.spring.initial_velocity, sizeof(float), 1, f);
			fread(kf.value, sizeof(float), 4, f);
			if (version >= 4) {
				fread(&kf.color_space, sizeof(int), 1, f);
				fread(kf.value_ext, sizeof(float), 4, f);
			}
			kf.has_bezier = (has_bezier_i != 0);
			kf.is_spring = (is_spring_i != 0);
			kf.channel = trk.channel;
			kf.type = trk.type;
			trk.keys.push_back(kf);
		}
	}

	clip->labels.clear();
	if (version >= 5) {
		int label_count = 0;
		if (fread(&label_count, sizeof(int), 1, f) == 1) {
			for (int l = 0; l < label_count; ++l) {
				iam_clip_detail::clip_label cl;
				if (fread(&cl.id, sizeof(ImGuiID), 1, f) != 1 || fread(&cl.time, sizeof(float), 1, f) != 1) break;
				clip->labels.push_back(cl);
			}
		}
	}
	if (version >= 6) {
		int curve_count = 0;
		if (fread(&curve_count, sizeof(int), 1, f) == 1) {
			for (int c = 0; c < curve_count; ++c) {
				ImGuiID cid = 0;
				int n = 0;
				if (fread(&cid, sizeof(ImGuiID), 1, f) != 1 || fread(&n, sizeof(int), 1, f) != 1 || n < 0 || n > 4096) break;
				ImVector<ImVec2> pts;
				pts.resize(n);
				bool ok = true;
				for (int i = 0; i < n && ok; ++i) ok = fread(&pts[i].x, sizeof(float), 1, f) == 1 && fread(&pts[i].y, sizeof(float), 1, f) == 1;
				if (!ok) break;
				ImVector<iam_vec2> conv;
				for (int i = 0; i < n; ++i) conv.push_back(IAM_VEC2_FROM_IM(pts[i]));
				iam_ease_curve_set(cid, conv.Data, n);
			}
		}
	}

	fclose(f);
	*out_clip_id = clip_id;
	return iam_ok;
}

// ----------------------------------------------------
// Oscillators
// ----------------------------------------------------
namespace iam_osc_detail {

struct osc_state {
	float time;
	unsigned last_frame;
	osc_state() : time(0), last_frame(0) {}
};

static ImGuiStorage g_osc_map;

static osc_state* get_osc(ImGuiID id) {
	osc_state* s = (osc_state*)g_osc_map.GetVoidPtr(id);
	if (!s) {
		s = IM_NEW(osc_state)();
		g_osc_map.SetVoidPtr(id, s);
	}
	return s;
}

static float eval_wave(int wave_type, float t) {
	// t is in [0, 1) representing one period
	t = t - ImFloor(t); // wrap to [0, 1)
	switch (wave_type) {
		case iam_wave_sine:
			return ImSin(t * 2.0f * IM_PI);
		case iam_wave_triangle:
			return (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
		case iam_wave_sawtooth:
			return 2.0f * t - 1.0f;
		case iam_wave_square:
			return (t < 0.5f) ? 1.0f : -1.0f;
		default:
			return ImSin(t * 2.0f * IM_PI);
	}
}

} // namespace iam_osc_detail

float iam_oscillate(ImGuiID id, float amplitude, float frequency, int wave_type, float phase, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude *= iam_detail::procedural_scale();
	using namespace iam_osc_detail;
	dt *= iam_detail::g_time_scale;
	osc_state* s = get_osc(id);
	if (s->last_frame != iam_detail::g_frame) {
		s->time += dt;
		s->last_frame = iam_detail::g_frame;
	}
	float t = s->time * frequency + phase;
	return amplitude * eval_wave(wave_type, t);
}int iam_oscillate_int(ImGuiID id, int amplitude, float frequency, int wave_type, float phase, float dt) {	float result = iam_oscillate(id, (float)amplitude, frequency, wave_type, phase, dt);	return (int)(result + 0.5f * (result > 0 ? 1 : -1));  // Round to nearest int
}

static ImVec2 iam_oscillate_vec2_impl(ImGuiID id, ImVec2 amplitude, ImVec2 frequency, int wave_type, ImVec2 phase, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);
	using namespace iam_osc_detail;
	dt *= iam_detail::g_time_scale;
	osc_state* s = get_osc(id);
	if (s->last_frame != iam_detail::g_frame) {
		s->time += dt;
		s->last_frame = iam_detail::g_frame;
	}
	float tx = s->time * frequency.x + phase.x;
	float ty = s->time * frequency.y + phase.y;
	return ImVec2(amplitude.x * eval_wave(wave_type, tx), amplitude.y * eval_wave(wave_type, ty));
}

static ImVec4 iam_oscillate_vec4_impl(ImGuiID id, ImVec4 amplitude, ImVec4 frequency, int wave_type, ImVec4 phase, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);
	using namespace iam_osc_detail;
	dt *= iam_detail::g_time_scale;
	osc_state* s = get_osc(id);
	if (s->last_frame != iam_detail::g_frame) {
		s->time += dt;
		s->last_frame = iam_detail::g_frame;
	}
	float tx = s->time * frequency.x + phase.x;
	float ty = s->time * frequency.y + phase.y;
	float tz = s->time * frequency.z + phase.z;
	float tw = s->time * frequency.w + phase.w;
	return ImVec4(
		amplitude.x * eval_wave(wave_type, tx),
		amplitude.y * eval_wave(wave_type, ty),
		amplitude.z * eval_wave(wave_type, tz),
		amplitude.w * eval_wave(wave_type, tw)
	);
}

static ImVec4 iam_oscillate_color_impl(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float frequency, int wave_type, float phase, int color_space, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);
	using namespace iam_osc_detail;
	dt *= iam_detail::g_time_scale;
	osc_state* s = get_osc(id);
	if (s->last_frame != iam_detail::g_frame) {
		s->time += dt;
		s->last_frame = iam_detail::g_frame;
	}
	float t = s->time * frequency + phase;
	float wave = eval_wave(wave_type, t);

	// Convert base color to target color space, apply oscillation, convert back
	ImVec4 working;
	switch (color_space) {
		case iam_col_srgb_linear:
			working = iam_detail::color::srgb_to_linear(base_color);
			working.x += amplitude.x * wave;
			working.y += amplitude.y * wave;
			working.z += amplitude.z * wave;
			working.w += amplitude.w * wave;
			return iam_detail::color::linear_to_srgb(working);
		case iam_col_hsv:
			working = iam_detail::color::srgb_to_hsv(base_color);
			working.x = ImFmod(working.x + amplitude.x * wave + 1.0f, 1.0f);
			working.y = ImClamp(working.y + amplitude.y * wave, 0.0f, 1.0f);
			working.z = ImClamp(working.z + amplitude.z * wave, 0.0f, 1.0f);
			working.w = ImClamp(working.w + amplitude.w * wave, 0.0f, 1.0f);
			return iam_detail::color::hsv_to_srgb(working);
		case iam_col_oklab:
			working = iam_detail::color::srgb_to_oklab(base_color);
			working.x += amplitude.x * wave;
			working.y += amplitude.y * wave;
			working.z += amplitude.z * wave;
			working.w += amplitude.w * wave;
			return iam_detail::color::oklab_to_srgb(working);
		case iam_col_oklch:
			working = iam_detail::color::srgb_to_oklch(base_color);
			working.x += amplitude.x * wave;
			working.y += amplitude.y * wave;
			working.z = ImFmod(working.z + amplitude.z * wave + 1.0f, 1.0f);
			working.w += amplitude.w * wave;
			return iam_detail::color::oklch_to_srgb(working);
		default: // iam_col_srgb
			return ImVec4(
				ImClamp(base_color.x + amplitude.x * wave, 0.0f, 1.0f),
				ImClamp(base_color.y + amplitude.y * wave, 0.0f, 1.0f),
				ImClamp(base_color.z + amplitude.z * wave, 0.0f, 1.0f),
				ImClamp(base_color.w + amplitude.w * wave, 0.0f, 1.0f)
			);
	}
}

// ----------------------------------------------------
// Shake/Wiggle
// ----------------------------------------------------
namespace iam_shake_detail {

struct shake_state {
	float time_since_trigger;
	float noise_time;
	unsigned last_frame;
	bool triggered;
	// Simple noise state
	float noise_val[4];
	int noise_idx;
	shake_state() : time_since_trigger(0), noise_time(0), last_frame(0), triggered(false), noise_idx(0) {
		for (int i = 0; i < 4; i++) noise_val[i] = 0;
	}
};

static ImGuiStorage g_shake_map;

static shake_state* get_shake(ImGuiID id) {
	shake_state* s = (shake_state*)g_shake_map.GetVoidPtr(id);
	if (!s) {
		s = IM_NEW(shake_state)();
		g_shake_map.SetVoidPtr(id, s);
	}
	return s;
}

// Simple pseudo-random based on ID and time
static float hash_noise(unsigned int seed) {
	seed = (seed ^ 61) ^ (seed >> 16);
	seed = seed + (seed << 3);
	seed = seed ^ (seed >> 4);
	seed = seed * 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return ((float)(seed & 0xFFFF) / 32768.0f) - 1.0f; // [-1, 1]
}

} // namespace iam_shake_detail

void iam_trigger_shake(ImGuiID id) {
	using namespace iam_shake_detail;
	shake_state* s = get_shake(id);
	s->triggered = true;
	s->time_since_trigger = 0;
}

float iam_shake(ImGuiID id, float intensity, float frequency, float decay_time, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) intensity *= iam_detail::procedural_scale();
	using namespace iam_shake_detail;
	dt *= iam_detail::g_time_scale;
	shake_state* s = get_shake(id);

	if (s->last_frame != iam_detail::g_frame) {
		if (s->triggered) {
			s->time_since_trigger += dt;
		}
		s->noise_time += dt;
		s->last_frame = iam_detail::g_frame;
	}

	if (!s->triggered || s->time_since_trigger >= decay_time) {
		s->triggered = false;
		return 0.0f;
	}

	// Decay factor
	float decay = 1.0f - (s->time_since_trigger / decay_time);
	decay = decay * decay; // quadratic decay

	// Generate noise at frequency
	float period = 1.0f / frequency;
	int sample = (int)(s->noise_time / period);
	float frac = (s->noise_time - sample * period) / period;

	// Interpolate between noise samples
	float n0 = hash_noise((unsigned int)id + sample);
	float n1 = hash_noise((unsigned int)id + sample + 1);
	float noise = n0 + (n1 - n0) * frac; // linear interpolation

	return noise * intensity * decay;
}int iam_shake_int(ImGuiID id, int intensity, float frequency, float decay_time, float dt) {	float result = iam_shake(id, (float)intensity, frequency, decay_time, dt);	return (int)(result + 0.5f * (result > 0 ? 1 : -1));
}

static ImVec2 iam_shake_vec2_impl(ImGuiID id, ImVec2 intensity, float frequency, float decay_time, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) intensity = iam_detail::procedural_scaled(intensity);
	ImGuiID id_x = id;
	ImGuiID id_y = id ^ 0x12345678;
	return ImVec2(
		iam_shake(id_x, intensity.x, frequency, decay_time, dt),
		iam_shake(id_y, intensity.y, frequency, decay_time, dt)
	);
}

static ImVec4 iam_shake_vec4_impl(ImGuiID id, ImVec4 intensity, float frequency, float decay_time, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) intensity = iam_detail::procedural_scaled(intensity);
	ImGuiID id_x = id;
	ImGuiID id_y = id ^ 0x12345678;
	ImGuiID id_z = id ^ 0x23456789;
	ImGuiID id_w = id ^ 0x3456789A;
	return ImVec4(
		iam_shake(id_x, intensity.x, frequency, decay_time, dt),
		iam_shake(id_y, intensity.y, frequency, decay_time, dt),
		iam_shake(id_z, intensity.z, frequency, decay_time, dt),
		iam_shake(id_w, intensity.w, frequency, decay_time, dt)
	);
}

static ImVec4 iam_shake_color_impl(ImGuiID id, ImVec4 base_color, ImVec4 intensity, float frequency, float decay_time, int color_space, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) intensity = iam_detail::procedural_scaled(intensity);
	ImVec4 shake = iam_shake_vec4_impl(id, intensity, frequency, decay_time, dt);

	// Apply shake offset in the specified color space
	ImVec4 working;
	switch (color_space) {
		case iam_col_srgb_linear:
			working = iam_detail::color::srgb_to_linear(base_color);
			working.x += shake.x;
			working.y += shake.y;
			working.z += shake.z;
			working.w += shake.w;
			return iam_detail::color::linear_to_srgb(working);
		case iam_col_hsv:
			working = iam_detail::color::srgb_to_hsv(base_color);
			working.x = ImFmod(working.x + shake.x + 1.0f, 1.0f);
			working.y = ImClamp(working.y + shake.y, 0.0f, 1.0f);
			working.z = ImClamp(working.z + shake.z, 0.0f, 1.0f);
			working.w = ImClamp(working.w + shake.w, 0.0f, 1.0f);
			return iam_detail::color::hsv_to_srgb(working);
		case iam_col_oklab:
			working = iam_detail::color::srgb_to_oklab(base_color);
			working.x += shake.x;
			working.y += shake.y;
			working.z += shake.z;
			working.w += shake.w;
			return iam_detail::color::oklab_to_srgb(working);
		case iam_col_oklch:
			working = iam_detail::color::srgb_to_oklch(base_color);
			working.x += shake.x;
			working.y += shake.y;
			working.z = ImFmod(working.z + shake.z + 1.0f, 1.0f);
			working.w += shake.w;
			return iam_detail::color::oklch_to_srgb(working);
		default: // iam_col_srgb
			return ImVec4(
				ImClamp(base_color.x + shake.x, 0.0f, 1.0f),
				ImClamp(base_color.y + shake.y, 0.0f, 1.0f),
				ImClamp(base_color.z + shake.z, 0.0f, 1.0f),
				ImClamp(base_color.w + shake.w, 0.0f, 1.0f)
			);
	}
}

float iam_wiggle(ImGuiID id, float amplitude, float frequency, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude *= iam_detail::procedural_scale();
	using namespace iam_shake_detail;
	dt *= iam_detail::g_time_scale;
	shake_state* s = get_shake(id);

	if (s->last_frame != iam_detail::g_frame) {
		s->noise_time += dt;
		s->last_frame = iam_detail::g_frame;
	}

	// Generate smooth continuous noise
	float period = 1.0f / frequency;
	int sample = (int)(s->noise_time / period);
	float frac = (s->noise_time - sample * period) / period;

	// Smoothstep interpolation for smoother movement
	float t = frac * frac * (3.0f - 2.0f * frac);

	float n0 = hash_noise((unsigned int)id + sample);
	float n1 = hash_noise((unsigned int)id + sample + 1);

	return amplitude * (n0 + (n1 - n0) * t);
}int iam_wiggle_int(ImGuiID id, int amplitude, float frequency, float dt) {	float result = iam_wiggle(id, (float)amplitude, frequency, dt);	return (int)(result + 0.5f * (result > 0 ? 1 : -1));
}

static ImVec2 iam_wiggle_vec2_impl(ImGuiID id, ImVec2 amplitude, float frequency, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);
	ImGuiID id_x = id;
	ImGuiID id_y = id ^ 0x12345678;
	return ImVec2(
		iam_wiggle(id_x, amplitude.x, frequency, dt),
		iam_wiggle(id_y, amplitude.y, frequency, dt)
	);
}

static ImVec4 iam_wiggle_vec4_impl(ImGuiID id, ImVec4 amplitude, float frequency, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);
	ImGuiID id_x = id;
	ImGuiID id_y = id ^ 0x12345678;
	ImGuiID id_z = id ^ 0x23456789;
	ImGuiID id_w = id ^ 0x3456789A;
	return ImVec4(
		iam_wiggle(id_x, amplitude.x, frequency, dt),
		iam_wiggle(id_y, amplitude.y, frequency, dt),
		iam_wiggle(id_z, amplitude.z, frequency, dt),
		iam_wiggle(id_w, amplitude.w, frequency, dt)
	);
}

static ImVec4 iam_wiggle_color_impl(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float frequency, int color_space, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);
	ImVec4 wiggle = iam_wiggle_vec4_impl(id, amplitude, frequency, dt);

	// Apply wiggle offset in the specified color space
	ImVec4 working;
	switch (color_space) {
		case iam_col_srgb_linear:
			working = iam_detail::color::srgb_to_linear(base_color);
			working.x += wiggle.x;
			working.y += wiggle.y;
			working.z += wiggle.z;
			working.w += wiggle.w;
			return iam_detail::color::linear_to_srgb(working);
		case iam_col_hsv:
			working = iam_detail::color::srgb_to_hsv(base_color);
			working.x = ImFmod(working.x + wiggle.x + 1.0f, 1.0f);
			working.y = ImClamp(working.y + wiggle.y, 0.0f, 1.0f);
			working.z = ImClamp(working.z + wiggle.z, 0.0f, 1.0f);
			working.w = ImClamp(working.w + wiggle.w, 0.0f, 1.0f);
			return iam_detail::color::hsv_to_srgb(working);
		case iam_col_oklab:
			working = iam_detail::color::srgb_to_oklab(base_color);
			working.x += wiggle.x;
			working.y += wiggle.y;
			working.z += wiggle.z;
			working.w += wiggle.w;
			return iam_detail::color::oklab_to_srgb(working);
		case iam_col_oklch:
			working = iam_detail::color::srgb_to_oklch(base_color);
			working.x += wiggle.x;
			working.y += wiggle.y;
			working.z = ImFmod(working.z + wiggle.z + 1.0f, 1.0f);
			working.w += wiggle.w;
			return iam_detail::color::oklch_to_srgb(working);
		default: // iam_col_srgb
			return ImVec4(
				ImClamp(base_color.x + wiggle.x, 0.0f, 1.0f),
				ImClamp(base_color.y + wiggle.y, 0.0f, 1.0f),
				ImClamp(base_color.z + wiggle.z, 0.0f, 1.0f),
				ImClamp(base_color.w + wiggle.w, 0.0f, 1.0f)
			);
	}
}

// ----------------------------------------------------
// Scroll Animation
// ----------------------------------------------------
namespace iam_scroll_detail {

struct scroll_anim {
	ImGuiID window_id;
	float start_x, start_y;
	float target_x, target_y;
	float duration;
	float elapsed;
	iam_ease_desc ease;
	bool active_x, active_y;
	unsigned last_frame;
};

static ImVector<scroll_anim> g_scroll_anims;

static scroll_anim* find_or_create(ImGuiID window_id) {
	for (int i = 0; i < g_scroll_anims.Size; i++) {
		if (g_scroll_anims[i].window_id == window_id) {
			return &g_scroll_anims[i];
		}
	}
	scroll_anim sa;
	sa.window_id = window_id;
	sa.active_x = sa.active_y = false;
	sa.elapsed = 0;
	sa.last_frame = 0;
	g_scroll_anims.push_back(sa);
	return &g_scroll_anims.back();
}

} // namespace iam_scroll_detail

void iam_scroll_to_y(float target_y, float duration, iam_ease_desc const& ez) {
	using namespace iam_scroll_detail;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (!window) return;

	scroll_anim* sa = find_or_create(window->ID);
	sa->start_y = window->Scroll.y;
	sa->target_y = target_y;
	sa->duration = duration;
	sa->elapsed = 0;
	sa->ease = ez;
	sa->active_y = true;
}

void iam_scroll_to_x(float target_x, float duration, iam_ease_desc const& ez) {
	using namespace iam_scroll_detail;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (!window) return;

	scroll_anim* sa = find_or_create(window->ID);
	sa->start_x = window->Scroll.x;
	sa->target_x = target_x;
	sa->duration = duration;
	sa->elapsed = 0;
	sa->ease = ez;
	sa->active_x = true;
}

void iam_scroll_to_top(float duration, iam_ease_desc const& ez) {
	iam_scroll_to_y(0.0f, duration, ez);
}

void iam_scroll_to_bottom(float duration, iam_ease_desc const& ez) {
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (!window) return;
	float max_y = window->ScrollMax.y;
	iam_scroll_to_y(max_y, duration, ez);
}

// Call this in iam_update_begin_frame to process scroll animations
static void iam_scroll_update_internal(float dt) {
	using namespace iam_scroll_detail;
	dt *= iam_detail::g_time_scale;

	for (int i = g_scroll_anims.Size - 1; i >= 0; i--) {
		scroll_anim& sa = g_scroll_anims[i];
		if (!sa.active_x && !sa.active_y) {
			g_scroll_anims.erase(&g_scroll_anims[i]);
			continue;
		}

		sa.elapsed += dt;
		float t = (sa.duration > 0) ? ImClamp(sa.elapsed / sa.duration, 0.0f, 1.0f) : 1.0f;
		float eased_t = iam_detail::eval(sa.ease, t);

		ImGuiWindow* window = ImGui::FindWindowByID(sa.window_id);
		if (!window) {
			sa.active_x = sa.active_y = false;
			continue;
		}

		if (sa.active_y) {
			float new_y = sa.start_y + (sa.target_y - sa.start_y) * eased_t;
			window->Scroll.y = new_y;
			if (t >= 1.0f) sa.active_y = false;
		}

		if (sa.active_x) {
			float new_x = sa.start_x + (sa.target_x - sa.start_x) * eased_t;
			window->Scroll.x = new_x;
			if (t >= 1.0f) sa.active_x = false;
		}
	}
}

// ----------------------------------------------------
// Motion Paths - animate along curves and splines
// ----------------------------------------------------
namespace iam_path_detail {

// Evaluate quadratic bezier: B(t) = (1-t)²P₀ + 2(1-t)tP₁ + t²P₂
static ImVec2 eval_quadratic(ImVec2 p0, ImVec2 p1, ImVec2 p2, float t) {
	float u = 1.0f - t;
	float tt = t * t;
	float uu = u * u;
	float ut2 = 2.0f * u * t;
	return ImVec2(
		uu * p0.x + ut2 * p1.x + tt * p2.x,
		uu * p0.y + ut2 * p1.y + tt * p2.y
	);
}

// Derivative of quadratic bezier: B'(t) = 2(1-t)(P₁-P₀) + 2t(P₂-P₁)
static ImVec2 eval_quadratic_deriv(ImVec2 p0, ImVec2 p1, ImVec2 p2, float t) {
	float u = 1.0f - t;
	return ImVec2(
		2.0f * u * (p1.x - p0.x) + 2.0f * t * (p2.x - p1.x),
		2.0f * u * (p1.y - p0.y) + 2.0f * t * (p2.y - p1.y)
	);
}

// Evaluate cubic bezier: B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃
static ImVec2 eval_cubic(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
	float u = 1.0f - t;
	float tt = t * t;
	float ttt = tt * t;
	float uu = u * u;
	float uuu = uu * u;
	return ImVec2(
		uuu * p0.x + 3.0f * uu * t * p1.x + 3.0f * u * tt * p2.x + ttt * p3.x,
		uuu * p0.y + 3.0f * uu * t * p1.y + 3.0f * u * tt * p2.y + ttt * p3.y
	);
}

// Derivative of cubic bezier: B'(t) = 3(1-t)²(P₁-P₀) + 6(1-t)t(P₂-P₁) + 3t²(P₃-P₂)
static ImVec2 eval_cubic_deriv(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
	float u = 1.0f - t;
	float uu = u * u;
	float tt = t * t;
	return ImVec2(
		3.0f * uu * (p1.x - p0.x) + 6.0f * u * t * (p2.x - p1.x) + 3.0f * tt * (p3.x - p2.x),
		3.0f * uu * (p1.y - p0.y) + 6.0f * u * t * (p2.y - p1.y) + 3.0f * tt * (p3.y - p2.y)
	);
}

// Evaluate Catmull-Rom spline (goes through p1 and p2)
static ImVec2 eval_catmull_rom(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t, float tension) {
	float t2 = t * t;
	float t3 = t2 * t;
	float s = (1.0f - tension) / 2.0f;

	// Catmull-Rom basis functions
	float h1 = -s * t3 + 2.0f * s * t2 - s * t;
	float h2 = (2.0f - s) * t3 + (s - 3.0f) * t2 + 1.0f;
	float h3 = (s - 2.0f) * t3 + (3.0f - 2.0f * s) * t2 + s * t;
	float h4 = s * t3 - s * t2;

	return ImVec2(
		h1 * p0.x + h2 * p1.x + h3 * p2.x + h4 * p3.x,
		h1 * p0.y + h2 * p1.y + h3 * p2.y + h4 * p3.y
	);
}

// Derivative of Catmull-Rom
static ImVec2 eval_catmull_rom_deriv(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t, float tension) {
	float t2 = t * t;
	float s = (1.0f - tension) / 2.0f;

	// Derivatives of basis functions
	float dh1 = -3.0f * s * t2 + 4.0f * s * t - s;
	float dh2 = 3.0f * (2.0f - s) * t2 + 2.0f * (s - 3.0f) * t;
	float dh3 = 3.0f * (s - 2.0f) * t2 + 2.0f * (3.0f - 2.0f * s) * t + s;
	float dh4 = 3.0f * s * t2 - 2.0f * s * t;

	return ImVec2(
		dh1 * p0.x + dh2 * p1.x + dh3 * p2.x + dh4 * p3.x,
		dh1 * p0.y + dh2 * p1.y + dh3 * p2.y + dh4 * p3.y
	);
}

// Path segment storage
struct path_segment {
	int type;           // iam_path_segment_type
	ImVec2 p0, p1, p2, p3;  // Control points (usage depends on type)
	float tension;      // For catmull-rom
	float length;       // Approximate segment length

	ImVec2 evaluate(float t) const {
		switch (type) {
			case iam_seg_line:
				return ImVec2(p0.x + (p1.x - p0.x) * t, p0.y + (p1.y - p0.y) * t);
			case iam_seg_quadratic_bezier:
				return eval_quadratic(p0, p1, p2, t);
			case iam_seg_cubic_bezier:
				return eval_cubic(p0, p1, p2, p3, t);
			case iam_seg_catmull_rom:
				return eval_catmull_rom(p0, p1, p2, p3, t, tension);
			default:
				return p0;
		}
	}

	ImVec2 derivative(float t) const {
		switch (type) {
			case iam_seg_line:
				return ImVec2(p1.x - p0.x, p1.y - p0.y);
			case iam_seg_quadratic_bezier:
				return eval_quadratic_deriv(p0, p1, p2, t);
			case iam_seg_cubic_bezier:
				return eval_cubic_deriv(p0, p1, p2, p3, t);
			case iam_seg_catmull_rom:
				return eval_catmull_rom_deriv(p0, p1, p2, p3, t, tension);
			default:
				return ImVec2(1, 0);
		}
	}
};

// Approximate segment length using subdivision
static float approx_segment_length(path_segment const& seg, int subdivisions = 16) {
	float len = 0.0f;
	ImVec2 prev = seg.evaluate(0.0f);
	for (int i = 1; i <= subdivisions; i++) {
		float t = (float)i / (float)subdivisions;
		ImVec2 cur = seg.evaluate(t);
		float dx = cur.x - prev.x;
		float dy = cur.y - prev.y;
		len += ImSqrt(dx * dx + dy * dy);
		prev = cur;
	}
	return len;
}

// Arc-length LUT entry
struct arc_lut_entry {
	float distance;   // Cumulative arc-length distance
	float t;          // Global t parameter [0,1]
};

// Path data storage
struct path_data {
	ImVector<path_segment> segments;
	ImVec2 start_point;
	float total_length;
	bool closed;

	// Arc-length LUT for constant-speed evaluation
	ImVector<arc_lut_entry> arc_lut;
	bool has_arc_lut;

	path_data() : start_point(0, 0), total_length(0), closed(false), has_arc_lut(false) {}

	void compute_lengths() {
		total_length = 0;
		for (int i = 0; i < segments.Size; i++) {
			segments[i].length = approx_segment_length(segments[i]);
			total_length += segments[i].length;
		}
	}

	// Find segment and local t for global t in [0,1]
	void find_segment(float global_t, int* out_seg_idx, float* out_local_t) const {
		if (segments.Size == 0) {
			*out_seg_idx = -1;
			*out_local_t = 0;
			return;
		}
		if (global_t <= 0.0f) {
			*out_seg_idx = 0;
			*out_local_t = 0;
			return;
		}
		if (global_t >= 1.0f) {
			*out_seg_idx = segments.Size - 1;
			*out_local_t = 1.0f;
			return;
		}

		float target_dist = global_t * total_length;
		float accum = 0;
		for (int i = 0; i < segments.Size; i++) {
			if (accum + segments[i].length >= target_dist) {
				*out_seg_idx = i;
				float local_dist = target_dist - accum;
				*out_local_t = (segments[i].length > 0) ? (local_dist / segments[i].length) : 0;
				return;
			}
			accum += segments[i].length;
		}
		// Fallback
		*out_seg_idx = segments.Size - 1;
		*out_local_t = 1.0f;
	}

	ImVec2 evaluate(float t) const {
		int seg_idx;
		float local_t;
		find_segment(t, &seg_idx, &local_t);
		if (seg_idx < 0) return start_point;
		return segments[seg_idx].evaluate(local_t);
	}

	ImVec2 derivative(float t) const {
		int seg_idx;
		float local_t;
		find_segment(t, &seg_idx, &local_t);
		if (seg_idx < 0) return ImVec2(1, 0);
		return segments[seg_idx].derivative(local_t);
	}

	// Build arc-length lookup table for constant-speed evaluation
	void build_arc_lut(int subdivisions) {
		arc_lut.clear();
		if (segments.Size == 0 || total_length <= 0) {
			has_arc_lut = false;
			return;
		}

		// Sample path at regular t intervals and record cumulative distance
		arc_lut_entry first;
		first.distance = 0;
		first.t = 0;
		arc_lut.push_back(first);

		ImVec2 prev = evaluate(0.0f);
		float cumulative = 0;

		for (int i = 1; i <= subdivisions; i++) {
			float t = (float)i / (float)subdivisions;
			ImVec2 cur = evaluate(t);
			float dx = cur.x - prev.x;
			float dy = cur.y - prev.y;
			cumulative += ImSqrt(dx * dx + dy * dy);

			arc_lut_entry entry;
			entry.distance = cumulative;
			entry.t = t;
			arc_lut.push_back(entry);

			prev = cur;
		}

		// Update total_length from LUT (more accurate)
		if (arc_lut.Size > 0) {
			total_length = arc_lut[arc_lut.Size - 1].distance;
		}
		has_arc_lut = true;
	}

	// Convert arc-length distance to t parameter using LUT (binary search)
	float distance_to_t(float distance) const {
		if (!has_arc_lut || arc_lut.Size < 2) {
			// Fallback: linear approximation
			return (total_length > 0) ? ImClamp(distance / total_length, 0.0f, 1.0f) : 0.0f;
		}

		if (distance <= 0) return 0.0f;
		if (distance >= total_length) return 1.0f;

		// TODO: Add a define to choose between binary and
		// an tabulated inverse CDF read, that's an approximation
		// but faster for real-time.

		// Binary search for the interval containing this distance
		int lo = 0, hi = arc_lut.Size - 1;
		while (lo < hi - 1) {
			int mid = (lo + hi) / 2;
			if (arc_lut[mid].distance < distance) {
				lo = mid;
			} else {
				hi = mid;
			}
		}

		// Linear interpolation within the interval
		float d0 = arc_lut[lo].distance;
		float d1 = arc_lut[hi].distance;
		float t0 = arc_lut[lo].t;
		float t1 = arc_lut[hi].t;

		if (d1 - d0 <= 0) return t0;
		float u = (distance - d0) / (d1 - d0);
		return t0 + (t1 - t0) * u;
	}

	// Evaluate at arc-length distance
	ImVec2 evaluate_at_distance(float distance) const {
		float t = distance_to_t(distance);
		return evaluate(t);
	}

	// Get angle at arc-length distance
	float angle_at_distance(float distance) const {
		float t = distance_to_t(distance);
		ImVec2 d = derivative(t);
		return ImAtan2(d.y, d.x);
	}

	// Get tangent at arc-length distance
	ImVec2 tangent_at_distance(float distance) const {
		float t = distance_to_t(distance);
		ImVec2 d = derivative(t);
		float len = ImSqrt(d.x * d.x + d.y * d.y);
		if (len > 0.0001f) {
			d.x /= len;
			d.y /= len;
		}
		return d;
	}
};

// Global path storage
static ImPool<path_data> g_paths;
static ImGuiStorage g_path_map;

// Currently building path
static path_data* g_building_path = nullptr;
static ImGuiID g_building_path_id = 0;
static ImVec2 g_current_point;
static ImVector<ImVec2> g_catmull_points;  // For collecting catmull-rom points

static path_data* get_path(ImGuiID path_id) {
	int idx = g_path_map.GetInt(path_id, -1);
	if (idx < 0) return nullptr;
	return g_paths.GetByIndex(idx);
}

} // namespace iam_path_detail

// Public curve evaluation functions
static ImVec2 iam_bezier_quadratic_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, float t) {
	return iam_path_detail::eval_quadratic(p0, p1, p2, t);
}

static ImVec2 iam_bezier_cubic_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
	return iam_path_detail::eval_cubic(p0, p1, p2, p3, t);
}

static ImVec2 iam_catmull_rom_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t, float tension) {
	return iam_path_detail::eval_catmull_rom(p0, p1, p2, p3, t, tension);
}

static ImVec2 iam_bezier_quadratic_deriv_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, float t) {
	return iam_path_detail::eval_quadratic_deriv(p0, p1, p2, t);
}

static ImVec2 iam_bezier_cubic_deriv_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
	return iam_path_detail::eval_cubic_deriv(p0, p1, p2, p3, t);
}

static ImVec2 iam_catmull_rom_deriv_impl(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t, float tension) {
	return iam_path_detail::eval_catmull_rom_deriv(p0, p1, p2, p3, t, tension);
}

// iam_path fluent builder
iam_path iam_path::begin(ImGuiID path_id, iam_vec2 start_in) {
	ImVec2 start = IAM_VEC2_TO_IM(start_in);
	using namespace iam_path_detail;

	// Clean up existing path if re-defining
	int existing_idx = g_path_map.GetInt(path_id, -1);
	if (existing_idx >= 0) {
		g_paths.Remove(path_id, g_paths.GetByIndex(existing_idx));
	}

	// Create new path
	g_building_path = g_paths.GetOrAddByKey(path_id);
	g_building_path_id = path_id;
	g_building_path->segments.clear();
	g_building_path->start_point = start;
	g_building_path->total_length = 0;
	g_building_path->closed = false;
	g_current_point = start;
	g_catmull_points.clear();
	g_catmull_points.push_back(start);

	return iam_path(path_id);
}

iam_path& iam_path::line_to(iam_vec2 end_in) {
	ImVec2 end = IAM_VEC2_TO_IM(end_in);
	using namespace iam_path_detail;
	if (!g_building_path) return *this;

	path_segment seg;
	seg.type = iam_seg_line;
	seg.p0 = g_current_point;
	seg.p1 = end;
	seg.tension = 0;
	g_building_path->segments.push_back(seg);
	g_current_point = end;
	g_catmull_points.push_back(end);

	return *this;
}

iam_path& iam_path::quadratic_to(iam_vec2 ctrl_in, iam_vec2 end_in) {
	ImVec2 ctrl = IAM_VEC2_TO_IM(ctrl_in);
	ImVec2 end = IAM_VEC2_TO_IM(end_in);
	using namespace iam_path_detail;
	if (!g_building_path) return *this;

	path_segment seg;
	seg.type = iam_seg_quadratic_bezier;
	seg.p0 = g_current_point;
	seg.p1 = ctrl;
	seg.p2 = end;
	seg.tension = 0;
	g_building_path->segments.push_back(seg);
	g_current_point = end;
	g_catmull_points.push_back(end);

	return *this;
}

iam_path& iam_path::cubic_to(iam_vec2 ctrl1_in, iam_vec2 ctrl2_in, iam_vec2 end_in) {
	ImVec2 ctrl1 = IAM_VEC2_TO_IM(ctrl1_in);
	ImVec2 ctrl2 = IAM_VEC2_TO_IM(ctrl2_in);
	ImVec2 end = IAM_VEC2_TO_IM(end_in);
	using namespace iam_path_detail;
	if (!g_building_path) return *this;

	path_segment seg;
	seg.type = iam_seg_cubic_bezier;
	seg.p0 = g_current_point;
	seg.p1 = ctrl1;
	seg.p2 = ctrl2;
	seg.p3 = end;
	seg.tension = 0;
	g_building_path->segments.push_back(seg);
	g_current_point = end;
	g_catmull_points.push_back(end);

	return *this;
}

iam_path& iam_path::catmull_to(iam_vec2 end_in, float tension) {
	ImVec2 end = IAM_VEC2_TO_IM(end_in);
	using namespace iam_path_detail;
	if (!g_building_path) return *this;

	// For catmull-rom, we need 4 points. Use previous points as context
	int n = g_catmull_points.Size;
	ImVec2 p0 = (n >= 2) ? g_catmull_points[n - 2] : g_current_point;
	ImVec2 p1 = g_current_point;
	ImVec2 p2 = end;
	ImVec2 p3 = end;  // Will be updated by next point or mirror

	path_segment seg;
	seg.type = iam_seg_catmull_rom;
	seg.p0 = p0;
	seg.p1 = p1;
	seg.p2 = p2;
	seg.p3 = p3;
	seg.tension = tension;
	g_building_path->segments.push_back(seg);
	g_current_point = end;
	g_catmull_points.push_back(end);

	// Update previous catmull-rom segment's p3 if any
	int seg_count = g_building_path->segments.Size;
	if (seg_count >= 2) {
		path_segment& prev = g_building_path->segments[seg_count - 2];
		if (prev.type == iam_seg_catmull_rom) {
			prev.p3 = end;
		}
	}

	return *this;
}

iam_path& iam_path::close() {
	using namespace iam_path_detail;
	if (!g_building_path) return *this;

	// Add line back to start if not already there
	if (g_current_point.x != g_building_path->start_point.x ||
		g_current_point.y != g_building_path->start_point.y) {
		line_to(IAM_VEC2_FROM_IM(g_building_path->start_point));
	}
	g_building_path->closed = true;

	return *this;
}

void iam_path::end() {
	using namespace iam_path_detail;
	if (!g_building_path) return;

	// Compute segment lengths
	g_building_path->compute_lengths();

	// Register in map
	g_path_map.SetInt(g_building_path_id, g_paths.GetIndex(g_building_path));

	g_building_path = nullptr;
	g_building_path_id = 0;
	g_catmull_points.clear();
}

// Path query functions
bool iam_path_exists(ImGuiID path_id) {
	return iam_path_detail::get_path(path_id) != nullptr;
}

float iam_path_length(ImGuiID path_id) {
	iam_path_detail::path_data* p = iam_path_detail::get_path(path_id);
	return p ? p->total_length : 0.0f;
}

static ImVec2 iam_path_evaluate_impl(ImGuiID path_id, float t) {
	iam_path_detail::path_data* p = iam_path_detail::get_path(path_id);
	return p ? p->evaluate(t) : ImVec2(0, 0);
}

static ImVec2 iam_path_tangent_impl(ImGuiID path_id, float t) {
	iam_path_detail::path_data* p = iam_path_detail::get_path(path_id);
	if (!p) return ImVec2(1, 0);

	ImVec2 d = p->derivative(t);
	float len = ImSqrt(d.x * d.x + d.y * d.y);
	if (len > 1e-6f) {
		d.x /= len;
		d.y /= len;
	}
	return d;
}

float iam_path_angle(ImGuiID path_id, float t) {
	ImVec2 tangent = iam_path_tangent_impl(path_id, t);
	return atan2f(tangent.y, tangent.x);
}

// Tween along path
static ImVec2 iam_tween_path_impl(ImGuiID id, ImGuiID channel_id, ImGuiID path_id, float dur, iam_ease_desc const& ez, int policy, float dt) {
	using namespace iam_detail;

	// Apply global time scale
	dt *= g_time_scale;

	// Get path
	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	if (!path || path->segments.Size == 0) {
		return ImVec2(0, 0);
	}

	// Use float channel to track progress (0 to 1)
	ImGuiID key = make_key(id, channel_id);
	float_chan* c = g_float.get(key);

	// Check if target changed (always 1.0 for path progress)
	float target = 1.0f;
	bool changed = (c->target != target);
	if (changed) {
		switch (c->policy) {
			case iam_policy_cut:
				c->current = 0.0f;  // Start from beginning
				c->set(target, dur, ez, policy);
				break;
			case iam_policy_queue:
				if (c->progress() < 1.0f && !c->has_pending) {
					c->has_pending = 1;
					c->pending_target = target;
				} else {
					c->set(target, dur, ez, policy);
				}
				break;
			default: // crossfade
				c->set(target, dur, ez, policy);
				break;
		}
	}

	c->tick(dt);

	// Handle pending
	if (c->has_pending && c->progress() >= 1.0f) {
		c->set(c->pending_target, dur, ez, policy);
		c->has_pending = 0;
	}

	// Evaluate path at current progress
	float progress = c->current;

	// Use arc-length parameterization for constant speed if LUT is available
	if (path->has_arc_lut) {
		float distance = progress * path->total_length;
		float t = path->distance_to_t(distance);
		return path->evaluate(t);
	}

	return path->evaluate(progress);
}

float iam_tween_path_angle(ImGuiID id, ImGuiID channel_id, ImGuiID path_id, float dur, iam_ease_desc const& ez, int policy, float dt) {
	using namespace iam_detail;

	// Apply global time scale
	dt *= g_time_scale;

	// Get path
	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	if (!path || path->segments.Size == 0) {
		return 0.0f;
	}

	// Use float channel to track progress (0 to 1)
	// Use different channel for angle to allow independent queries
	ImGuiID angle_channel = ImHashStr("_angle", 0, channel_id);
	ImGuiID key = make_key(id, angle_channel);
	float_chan* c = g_float.get(key);

	float target = 1.0f;
	bool changed = (c->target != target);
	if (changed) {
		switch (c->policy) {
			case iam_policy_cut:
				c->current = 0.0f;
				c->set(target, dur, ez, policy);
				break;
			case iam_policy_queue:
				if (c->progress() < 1.0f && !c->has_pending) {
					c->has_pending = 1;
					c->pending_target = target;
				} else {
					c->set(target, dur, ez, policy);
				}
				break;
			default:
				c->set(target, dur, ez, policy);
				break;
		}
	}

	c->tick(dt);

	if (c->has_pending && c->progress() >= 1.0f) {
		c->set(c->pending_target, dur, ez, policy);
		c->has_pending = 0;
	}

	float progress = c->current;

	// Use arc-length parameterization for constant speed if LUT is available
	if (path->has_arc_lut) {
		float distance = progress * path->total_length;
		return iam_path_angle_at_distance(path_id, distance);
	}

	return iam_path_angle(path_id, progress);
}

// ----------------------------------------------------
// Arc-length parameterization
// ----------------------------------------------------

void iam_path_build_arc_lut(ImGuiID path_id, int subdivisions) {
	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	if (path) {
		path->build_arc_lut(subdivisions);
	}
}

bool iam_path_has_arc_lut(ImGuiID path_id) {
	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	return path && path->has_arc_lut;
}

float iam_path_distance_to_t(ImGuiID path_id, float distance) {
	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	if (!path) return 0.0f;
	return path->distance_to_t(distance);
}

static ImVec2 iam_path_evaluate_at_distance_impl(ImGuiID path_id, float distance) {
	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	if (!path) return ImVec2(0, 0);
	return path->evaluate_at_distance(distance);
}

float iam_path_angle_at_distance(ImGuiID path_id, float distance) {
	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	if (!path) return 0.0f;
	return path->angle_at_distance(distance);
}

static ImVec2 iam_path_tangent_at_distance_impl(ImGuiID path_id, float distance) {
	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	if (!path) return ImVec2(1, 0);
	return path->tangent_at_distance(distance);
}

// ============================================================
// PATH MORPHING - Interpolate between two paths
// ============================================================

namespace iam_morph_detail {

// State for morph tweens
struct morph_state {
	float blend;         // Current blend value (0 = path_a, 1 = path_b)
	float path_t;        // Current position along morphed path
	ImGuiID last_frame;
};

static ImPool<morph_state> g_morph_states;

morph_state* get_morph_state(ImGuiID id, ImGuiID channel_id) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	morph_state* s = g_morph_states.GetByKey(key);
	if (!s) {
		s = g_morph_states.GetOrAddByKey(key);
		s->blend = 0.0f;
		s->path_t = 0.0f;
		s->last_frame = 0;
	}
	return s;
}

// Sample a path at uniform intervals for morphing
void sample_path(ImGuiID path_id, int samples, ImVector<ImVec2>& out_points, bool use_arc_length) {
	out_points.resize(samples);

	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	if (!path || path->segments.Size == 0) {
		// Fill with zeros if path doesn't exist
		for (int i = 0; i < samples; i++) {
			out_points[i] = ImVec2(0, 0);
		}
		return;
	}

	// Build arc-length LUT if needed and using arc-length sampling
	if (use_arc_length && !path->has_arc_lut) {
		path->build_arc_lut(64);
	}

	for (int i = 0; i < samples; i++) {
		float t = (samples > 1) ? (float)i / (float)(samples - 1) : 0.0f;

		if (use_arc_length && path->has_arc_lut) {
			float distance = t * path->total_length;
			float path_t = path->distance_to_t(distance);
			out_points[i] = path->evaluate(path_t);
		} else {
			out_points[i] = path->evaluate(t);
		}
	}
}

// Interpolate between two point arrays
ImVec2 lerp_sampled(ImVector<ImVec2> const& a, ImVector<ImVec2> const& b, float t, float blend) {
	int n = a.Size;
	if (n == 0) return ImVec2(0, 0);

	// Handle endpoints
	if (t <= 0.0f) {
		return ImVec2(
			a[0].x + (b[0].x - a[0].x) * blend,
			a[0].y + (b[0].y - a[0].y) * blend
		);
	}
	if (t >= 1.0f) {
		return ImVec2(
			a[n-1].x + (b[n-1].x - a[n-1].x) * blend,
			a[n-1].y + (b[n-1].y - a[n-1].y) * blend
		);
	}

	// Find segment
	float scaled_t = t * (n - 1);
	int idx = (int)scaled_t;
	float frac = scaled_t - idx;

	if (idx >= n - 1) {
		idx = n - 2;
		frac = 1.0f;
	}

	// Interpolate within segment for both paths
	ImVec2 pa = ImVec2(
		a[idx].x + (a[idx+1].x - a[idx].x) * frac,
		a[idx].y + (a[idx+1].y - a[idx].y) * frac
	);
	ImVec2 pb = ImVec2(
		b[idx].x + (b[idx+1].x - b[idx].x) * frac,
		b[idx].y + (b[idx+1].y - b[idx].y) * frac
	);

	// Blend between paths
	return ImVec2(
		pa.x + (pb.x - pa.x) * blend,
		pa.y + (pb.y - pa.y) * blend
	);
}

// Get tangent from sampled points
ImVec2 tangent_sampled(ImVector<ImVec2> const& a, ImVector<ImVec2> const& b, float t, float blend) {
	int n = a.Size;
	if (n < 2) return ImVec2(1, 0);

	// Small delta for numerical derivative
	float dt = 0.001f;
	float t0 = ImMax(0.0f, t - dt);
	float t1 = ImMin(1.0f, t + dt);

	ImVec2 p0 = lerp_sampled(a, b, t0, blend);
	ImVec2 p1 = lerp_sampled(a, b, t1, blend);

	ImVec2 d = ImVec2(p1.x - p0.x, p1.y - p0.y);
	float len = ImSqrt(d.x * d.x + d.y * d.y);
	if (len > 1e-6f) {
		d.x /= len;
		d.y /= len;
	} else {
		d = ImVec2(1, 0);
	}
	return d;
}

} // namespace iam_morph_detail

static ImVec2 iam_path_morph_impl(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts) {
	using namespace iam_morph_detail;

	// Clamp inputs
	t = ImClamp(t, 0.0f, 1.0f);
	blend = ImClamp(blend, 0.0f, 1.0f);

	// Fast path: no blending needed
	if (blend <= 0.0f) {
		return iam_path_evaluate_impl(path_a, t);
	}
	if (blend >= 1.0f) {
		return iam_path_evaluate_impl(path_b, t);
	}

	// Sample both paths
	static ImVector<ImVec2> samples_a;
	static ImVector<ImVec2> samples_b;

	sample_path(path_a, opts.samples, samples_a, opts.use_arc_length);
	sample_path(path_b, opts.samples, samples_b, opts.use_arc_length);

	// Interpolate
	return lerp_sampled(samples_a, samples_b, t, blend);
}

static ImVec2 iam_path_morph_tangent_impl(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts) {
	using namespace iam_morph_detail;

	// Clamp inputs
	t = ImClamp(t, 0.0f, 1.0f);
	blend = ImClamp(blend, 0.0f, 1.0f);

	// Fast path: no blending needed
	if (blend <= 0.0f) {
		return iam_path_tangent_impl(path_a, t);
	}
	if (blend >= 1.0f) {
		return iam_path_tangent_impl(path_b, t);
	}

	// Sample both paths
	static ImVector<ImVec2> samples_a;
	static ImVector<ImVec2> samples_b;

	sample_path(path_a, opts.samples, samples_a, opts.use_arc_length);
	sample_path(path_b, opts.samples, samples_b, opts.use_arc_length);

	return tangent_sampled(samples_a, samples_b, t, blend);
}

float iam_path_morph_angle(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts) {
	ImVec2 tangent = iam_path_morph_tangent_impl(path_a, path_b, t, blend, opts);
	return atan2f(tangent.y, tangent.x);
}

static ImVec2 iam_tween_path_morph_impl(ImGuiID id, ImGuiID channel_id, ImGuiID path_a, ImGuiID path_b,
                            float target_blend, float dur, iam_ease_desc const& path_ease,
                            iam_ease_desc const& morph_ease, int policy, float dt,
                            iam_morph_opts const& opts) {
	using namespace iam_detail;
	using namespace iam_morph_detail;

	// Apply global time scale
	dt *= g_time_scale;

	// Get or create morph state
	morph_state* ms = get_morph_state(id, channel_id);

	// Use separate float tweens for path progress and blend
	ImGuiID path_ch = ImHashStr("_morph_path", 0, channel_id);
	ImGuiID blend_ch = ImHashStr("_morph_blend", 0, channel_id);

	// Animate path progress (0 to 1)
	ImGuiID path_key = make_key(id, path_ch);
	float_chan* path_c = g_float.get(path_key);

	// Check if path tween needs update
	float path_target = 1.0f;
	if (path_c->target != path_target || path_c->progress() >= 1.0f) {
		if (policy == iam_policy_cut) {
			path_c->current = 0.0f;
			path_c->set(path_target, dur, path_ease, policy);
		} else {
			path_c->set(path_target, dur, path_ease, policy);
		}
	}
	path_c->tick(dt);

	// Animate morph blend
	ImGuiID blend_key = make_key(id, blend_ch);
	float_chan* blend_c = g_float.get(blend_key);

	// Check if blend tween needs update
	if (fabsf(blend_c->target - target_blend) > 1e-6f || blend_c->progress() >= 1.0f) {
		if (policy == iam_policy_cut) {
			blend_c->current = blend_c->start = blend_c->target = target_blend;
			blend_c->sleeping = 1;
		} else {
			blend_c->set(target_blend, dur, morph_ease, policy);
		}
	}
	blend_c->tick(dt);

	// Store state for queries
	ms->path_t = path_c->current;
	ms->blend = blend_c->current;

	// Evaluate morphed path
	return iam_path_morph_impl(path_a, path_b, ms->path_t, ms->blend, opts);
}

float iam_get_morph_blend(ImGuiID id, ImGuiID channel_id) {
	iam_morph_detail::morph_state* ms = iam_morph_detail::g_morph_states.GetByKey(
		iam_detail::make_key(id, channel_id)
	);
	return ms ? ms->blend : 0.0f;
}

// ----------------------------------------------------
// Quad transform helpers
// ----------------------------------------------------

static void iam_transform_quad_impl(ImVec2* quad, ImVec2 center, float angle_rad, ImVec2 translation) {
	float cos_a = ImCos(angle_rad);
	float sin_a = ImSin(angle_rad);

	for (int i = 0; i < 4; i++) {
		// Translate to origin (relative to center)
		float x = quad[i].x - center.x;
		float y = quad[i].y - center.y;

		// Rotate
		float rx = x * cos_a - y * sin_a;
		float ry = x * sin_a + y * cos_a;

		// Translate to final position
		quad[i].x = rx + center.x + translation.x;
		quad[i].y = ry + center.y + translation.y;
	}
}

static void iam_make_glyph_quad_impl(ImVec2* quad, ImVec2 pos, float angle_rad, float glyph_width, float glyph_height, float baseline_offset) {
	float cos_a = ImCos(angle_rad);
	float sin_a = ImSin(angle_rad);

	// Perpendicular vector (90 degrees rotated from tangent, screen coords Y-down)
	float perp_x = sin_a;
	float perp_y = -cos_a;

	// Offset along perpendicular for baseline
	float base_offset_x = perp_x * baseline_offset;
	float base_offset_y = perp_y * baseline_offset;

	// Create quad corners (centered horizontally, baseline-aligned vertically)
	// Forward vector (along path tangent)
	float fwd_x = cos_a;
	float fwd_y = sin_a;

	float half_w = glyph_width * 0.5f;

	// Bottom-left
	quad[0].x = pos.x - fwd_x * half_w + base_offset_x;
	quad[0].y = pos.y - fwd_y * half_w + base_offset_y;

	// Bottom-right
	quad[1].x = pos.x + fwd_x * half_w + base_offset_x;
	quad[1].y = pos.y + fwd_y * half_w + base_offset_y;

	// Top-right
	quad[2].x = pos.x + fwd_x * half_w + base_offset_x - perp_x * glyph_height;
	quad[2].y = pos.y + fwd_y * half_w + base_offset_y - perp_y * glyph_height;

	// Top-left
	quad[3].x = pos.x - fwd_x * half_w + base_offset_x - perp_x * glyph_height;
	quad[3].y = pos.y - fwd_y * half_w + base_offset_y - perp_y * glyph_height;
}

// ----------------------------------------------------
// Text along motion paths
// ----------------------------------------------------

float iam_text_path_width(char const* text, iam_text_path_opts const& opts) {
	ImFont* font = opts.font ? opts.font : ImGui::GetFont();
	float font_size = ImGui::GetFontSize() * opts.font_scale;
	FontType* baked = GetBakedFont(font, font_size);
	if (!baked) return 0.0f;

	float total_width = 0;
	char const* p = text;
	while (*p) {
		unsigned int c = 0;
		int char_len = ImTextCharFromUtf8(&c, p, nullptr);
		if (char_len == 0) break;

		ImFontGlyph const* glyph = baked->FindGlyph((ImWchar)c);
		if (glyph) {
			total_width += glyph->AdvanceX;
			total_width += opts.letter_spacing;
		}
		p += char_len;
	}
	// Remove last letter_spacing
	if (total_width > 0 && opts.letter_spacing != 0) {
		total_width -= opts.letter_spacing;
	}
	return total_width;
}

void iam_text_path(ImGuiID path_id, char const* text, iam_text_path_opts const& opts) {
	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	if (!path || path->segments.Size == 0 || !text || !*text) return;

	// Ensure path has arc-length LUT for constant-speed text placement
	if (!path->has_arc_lut) {
		path->build_arc_lut(64);
	}

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImFont* font = opts.font ? opts.font : ImGui::GetFont();
	float font_size = ImGui::GetFontSize() * opts.font_scale;
	FontType* baked = GetBakedFont(font, font_size);
	if (!baked) return;

	// Calculate text width and starting offset
	float text_width = iam_text_path_width(text, opts);
	float path_len = path->total_length;

	float start_offset = opts.offset;
	switch (opts.align) {
		case iam_text_align_center:
			start_offset = (path_len - text_width) * 0.5f + opts.offset;
			break;
		case iam_text_align_end:
			start_offset = path_len - text_width + opts.offset;
			break;
		default: // iam_text_align_start
			break;
	}

	// Render each character
	float current_dist = start_offset;
	char const* p = text;

	while (*p) {
		unsigned int c = 0;
		int char_len = ImTextCharFromUtf8(&c, p, nullptr);
		if (char_len == 0) break;

		ImFontGlyph const* glyph = baked->FindGlyph((ImWchar)c);
		if (!glyph) {
			p += char_len;
			continue;
		}

		float glyph_advance = glyph->AdvanceX;
		float glyph_width = glyph->X1 - glyph->X0;
		float glyph_height = glyph->Y1 - glyph->Y0;
		float glyph_offset_x = glyph->X0;
		float glyph_offset_y = glyph->Y0;

		// Position at center of glyph advance
		float char_center_dist = current_dist + glyph_advance * 0.5f;

		// Skip if outside path bounds
		if (char_center_dist >= 0 && char_center_dist <= path_len) {
			ImVec2 pos = path->evaluate_at_distance(char_center_dist);
			float angle = path->angle_at_distance(char_center_dist);

			if (opts.flip_y) {
				angle += IM_PI;
			}

			float cos_a = ImCos(angle);
			float sin_a = ImSin(angle);

			// Perpendicular (up) direction in screen coords (Y-down)
			float perp_x = sin_a;
			float perp_y = -cos_a;

			// Calculate quad corners for the glyph
			// Shift glyph up so it sits ON TOP of the path (path = baseline)
			float local_x0 = -glyph_advance * 0.5f + glyph_offset_x;
			float local_x1 = local_x0 + glyph_width;
			float local_y0 = glyph_offset_y - glyph_height;  // Top of glyph (shifted up)
			float local_y1 = glyph_offset_y;                  // Bottom of glyph (at path)

			// Transform to path coordinates + origin offset
			float ox = IAM_VEC2_TO_IM(opts.origin).x;
			float oy = IAM_VEC2_TO_IM(opts.origin).y;
			ImVec2 corners[4];
			// Bottom-left (UV: u0, v0)
			corners[0].x = ox + pos.x + cos_a * local_x0 - perp_x * local_y0;
			corners[0].y = oy + pos.y + sin_a * local_x0 - perp_y * local_y0;
			// Bottom-right (UV: u1, v0)
			corners[1].x = ox + pos.x + cos_a * local_x1 - perp_x * local_y0;
			corners[1].y = oy + pos.y + sin_a * local_x1 - perp_y * local_y0;
			// Top-right (UV: u1, v1)
			corners[2].x = ox + pos.x + cos_a * local_x1 - perp_x * local_y1;
			corners[2].y = oy + pos.y + sin_a * local_x1 - perp_y * local_y1;
			// Top-left (UV: u0, v1)
			corners[3].x = ox + pos.x + cos_a * local_x0 - perp_x * local_y1;
			corners[3].y = oy + pos.y + sin_a * local_x0 - perp_y * local_y1;

			// Draw textured quad
			draw_list->PrimReserve(6, 4);
			draw_list->PrimQuadUV(
				corners[0], corners[1], corners[2], corners[3],
				ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V0),
				ImVec2(glyph->U1, glyph->V1), ImVec2(glyph->U0, glyph->V1),
				opts.color
			);
		}

		current_dist += glyph_advance + opts.letter_spacing;
		p += char_len;
	}
}

void iam_text_path_animated(ImGuiID path_id, char const* text, float progress, iam_text_path_opts const& opts) {
	iam_path_detail::path_data* path = iam_path_detail::get_path(path_id);
	if (!path || path->segments.Size == 0 || !text || !*text) return;

	// Ensure path has arc-length LUT
	if (!path->has_arc_lut) {
		path->build_arc_lut(64);
	}

	// Clamp progress
	progress = ImClamp(progress, 0.0f, 1.0f);
	if (progress <= 0.0f) return;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImFont* font = opts.font ? opts.font : ImGui::GetFont();
	float font_size = ImGui::GetFontSize() * opts.font_scale;
	FontType* baked = GetBakedFont(font, font_size);
	if (!baked) return;

	// Calculate text width and starting offset
	float text_width = iam_text_path_width(text, opts);
	float path_len = path->total_length;

	float start_offset = opts.offset;
	switch (opts.align) {
		case iam_text_align_center:
			start_offset = (path_len - text_width) * 0.5f + opts.offset;
			break;
		case iam_text_align_end:
			start_offset = path_len - text_width + opts.offset;
			break;
		default:
			break;
	}

	// Count characters for progress calculation
	int char_count = 0;
	char const* p = text;
	while (*p) {
		unsigned int c = 0;
		int char_len = ImTextCharFromUtf8(&c, p, nullptr);
		if (char_len == 0) break;
		char_count++;
		p += char_len;
	}

	// How many characters to show
	int visible_chars = (int)(progress * char_count + 0.999f);  // Round up
	float partial_char_alpha = ImFmod(progress * char_count, 1.0f);
	if (progress >= 1.0f) partial_char_alpha = 1.0f;

	// Render characters
	float current_dist = start_offset;
	p = text;
	int char_idx = 0;

	while (*p && char_idx < visible_chars) {
		unsigned int c = 0;
		int char_len = ImTextCharFromUtf8(&c, p, nullptr);
		if (char_len == 0) break;

		ImFontGlyph const* glyph = baked->FindGlyph((ImWchar)c);
		if (!glyph) {
			p += char_len;
			char_idx++;
			continue;
		}

		float glyph_advance = glyph->AdvanceX;
		float glyph_width = glyph->X1 - glyph->X0;
		float glyph_height = glyph->Y1 - glyph->Y0;
		float glyph_offset_x = glyph->X0;
		float glyph_offset_y = glyph->Y0;

		float char_center_dist = current_dist + glyph_advance * 0.5f;

		if (char_center_dist >= 0 && char_center_dist <= path_len) {
			ImVec2 pos = path->evaluate_at_distance(char_center_dist);
			float angle = path->angle_at_distance(char_center_dist);

			if (opts.flip_y) {
				angle += IM_PI;
			}

			float cos_a = ImCos(angle);
			float sin_a = ImSin(angle);
			float perp_x = sin_a;
			float perp_y = -cos_a;

			// Shift glyph up so it sits ON TOP of the path (path = baseline)
			float local_x0 = -glyph_advance * 0.5f + glyph_offset_x;
			float local_x1 = local_x0 + glyph_width;
			float local_y0 = glyph_offset_y - glyph_height;  // Top of glyph (shifted up)
			float local_y1 = glyph_offset_y;                  // Bottom of glyph (at path)

			// Transform to path coordinates + origin offset
			float ox = IAM_VEC2_TO_IM(opts.origin).x;
			float oy = IAM_VEC2_TO_IM(opts.origin).y;
			ImVec2 corners[4];
			corners[0].x = ox + pos.x + cos_a * local_x0 - perp_x * local_y0;
			corners[0].y = oy + pos.y + sin_a * local_x0 - perp_y * local_y0;
			corners[1].x = ox + pos.x + cos_a * local_x1 - perp_x * local_y0;
			corners[1].y = oy + pos.y + sin_a * local_x1 - perp_y * local_y0;
			corners[2].x = ox + pos.x + cos_a * local_x1 - perp_x * local_y1;
			corners[2].y = oy + pos.y + sin_a * local_x1 - perp_y * local_y1;
			corners[3].x = ox + pos.x + cos_a * local_x0 - perp_x * local_y1;
			corners[3].y = oy + pos.y + sin_a * local_x0 - perp_y * local_y1;

			// Calculate alpha for this character (fade in last character)
			ImU32 color = opts.color;
			if (char_idx == visible_chars - 1 && partial_char_alpha < 1.0f) {
				ImU32 alpha = (ImU32)((color >> IM_COL32_A_SHIFT) & 0xFF);
				alpha = (ImU32)(alpha * partial_char_alpha);
				color = (color & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
			}

			draw_list->PrimReserve(6, 4);
			draw_list->PrimQuadUV(
				corners[0], corners[1], corners[2], corners[3],
				ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V0),
				ImVec2(glyph->U1, glyph->V1), ImVec2(glyph->U0, glyph->V1),
				color
			);
		}

		current_dist += glyph_advance + opts.letter_spacing;
		p += char_len;
		char_idx++;
	}
}

// ============================================================
// TEXT STAGGER - Per-character animation effects
// ============================================================

// Stagger unit index for every character (chars: index, words: word index, lines: line index); returns the unit count
static int text_units(char const* text, int split, ImVector<int>& units) {
	units.resize(0);
	int unit = -1, idx = 0;
	bool prev_space = true;
	for (char const* p = text; *p; ) {
		unsigned int c;
		int len = ImTextCharFromUtf8(&c, p, nullptr);
		if (len == 0) break;
		bool space = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
		if (split == iam_text_split_words) { if (!space && prev_space) unit++; }
		else if (split == iam_text_split_lines) { if (idx == 0) unit = 0; }
		else unit = idx;
		units.push_back(unit < 0 ? 0 : unit);
		if (split == iam_text_split_lines && c == '\n') unit++;
		prev_space = space;
		p += len;
		idx++;
	}
	int count = 0;
	for (int i = 0; i < units.Size; i++) count = ImMax(count, units[i] + 1);
	return count;
}

float iam_text_stagger_width(char const* text, iam_text_stagger_opts const& opts) {
	ImFont* font = opts.font ? opts.font : ImGui::GetFont();
	float font_size = ImGui::GetFontSize() * opts.font_scale;
	FontType* baked = GetBakedFont(font, font_size);

	float width = 0.0f;
	char const* p = text;
	int char_count = 0;

	float line_width = 0.0f;
	while (*p) {
		unsigned int c;
		int char_len = ImTextCharFromUtf8(&c, p, nullptr);
		if (char_len == 0) break;

		if (c == '\n') { width = ImMax(width, line_width); line_width = 0.0f; p += char_len; char_count++; continue; }
		ImFontGlyph const* glyph = baked->FindGlyph((ImWchar)c);
		if (glyph) {
			line_width += glyph->AdvanceX;
			if (*(p + char_len) && *(p + char_len) != '\n') {
				line_width += opts.letter_spacing;
			}
		}
		p += char_len;
		char_count++;
	}

	return ImMax(width, line_width);
}

float iam_text_stagger_duration(char const* text, iam_text_stagger_opts const& opts) {
	if (!text || !*text) return 0.0f;
	ImVector<int> units;
	int unit_count = text_units(text, opts.split, units);
	if (unit_count == 0) return 0.0f;
	return (unit_count - 1) * opts.char_delay + opts.char_duration;
}

void iam_text_stagger(ImGuiID id, char const* text, float progress, iam_text_stagger_opts const& opts) {
	if (!text || !*text) return;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImFont* font = opts.font ? opts.font : ImGui::GetFont();
	float font_size = ImGui::GetFontSize() * opts.font_scale;
	FontType* baked = GetBakedFont(font, font_size);

	// Count characters
	int char_count = 0;
	char const* p = text;
	while (*p) {
		unsigned int c;
		int char_len = ImTextCharFromUtf8(&c, p, nullptr);
		if (char_len == 0) break;
		char_count++;
		p += char_len;
	}

	if (char_count == 0) return;

	float total_duration = iam_text_stagger_duration(text, opts);
	float current_time = progress * total_duration;

	ImVector<int> units;
	text_units(text, opts.split, units);
	float const start_x = IAM_VEC2_TO_IM(opts.pos).x;
	float cursor_x = start_x;
	float cursor_y = IAM_VEC2_TO_IM(opts.pos).y;
	p = text;
	int char_idx = 0;

	while (*p) {
		unsigned int c;
		int char_len = ImTextCharFromUtf8(&c, p, nullptr);
		if (char_len == 0) break;

		if (c == '\n') {  // new line
			cursor_x = start_x;
			cursor_y += font_size + opts.line_spacing;
			p += char_len;
			char_idx++;
			continue;
		}
		ImFontGlyph const* glyph = baked->FindGlyph((ImWchar)c);
		if (!glyph) {
			p += char_len;
			char_idx++;
			continue;
		}

		// Calculate character animation progress (characters of one unit share its timing)
		int const unit_idx = char_idx < units.Size ? units[char_idx] : char_idx;
		float char_start_time = unit_idx * opts.char_delay;
		float char_progress = 0.0f;
		float raw_progress = 0.0f;

		if (current_time >= char_start_time + opts.char_duration) {
			char_progress = 1.0f;
			raw_progress = 1.0f;
		} else if (current_time > char_start_time) {
			float local_t = (current_time - char_start_time) / opts.char_duration;
			char_progress = iam_detail::eval(opts.ease, local_t);
			raw_progress = local_t;
		}

		// Skip invisible characters (the callback may want to draw them)
		if (char_progress <= 0.0f && opts.effect != iam_text_fx_wave && !opts.glyph_callback) {
			cursor_x += glyph->AdvanceX + opts.letter_spacing;
			p += char_len;
			char_idx++;
			continue;
		}

		// Calculate effect transforms
		float alpha = 1.0f;
		float scale = 1.0f;
		float offset_x = 0.0f;
		float offset_y = 0.0f;
		float rotation = 0.0f;

		switch (opts.effect) {
			case iam_text_fx_none:
			case iam_text_fx_typewriter:
				// Instant appear
				alpha = char_progress > 0.0f ? 1.0f : 0.0f;
				break;

			case iam_text_fx_fade:
				alpha = char_progress;
				break;

			case iam_text_fx_scale:
				alpha = char_progress;
				scale = char_progress;
				break;

			case iam_text_fx_slide_up:
				alpha = char_progress;
				offset_y = (1.0f - char_progress) * opts.effect_intensity;
				break;

			case iam_text_fx_slide_down:
				alpha = char_progress;
				offset_y = -(1.0f - char_progress) * opts.effect_intensity;
				break;

			case iam_text_fx_slide_left:
				alpha = char_progress;
				offset_x = (1.0f - char_progress) * opts.effect_intensity;
				break;

			case iam_text_fx_slide_right:
				alpha = char_progress;
				offset_x = -(1.0f - char_progress) * opts.effect_intensity;
				break;

			case iam_text_fx_rotate:
				alpha = char_progress;
				rotation = (1.0f - char_progress) * opts.effect_intensity * (3.14159f / 180.0f);
				break;

			case iam_text_fx_bounce: {
				alpha = char_progress;
				// Overshoot effect using back easing
				float bounce_t = char_progress;
				if (bounce_t < 1.0f) {
					bounce_t = bounce_t * bounce_t * ((iam_detail::BACK_OVERSHOOT + 1) * bounce_t - iam_detail::BACK_OVERSHOOT);
				}
				scale = bounce_t;
				break;
			}

			case iam_text_fx_wave: {
				// Continuous wave effect based on time and character index
				float wave_offset = char_idx * 0.3f;
				float wave_time = progress * 6.28318f + wave_offset;
				offset_y = ImSin(wave_time) * opts.effect_intensity * 0.5f;
				alpha = 1.0f;
				break;
			}
		}

		// Per-glyph hook
		ImU32 glyph_color = opts.color;
		if (opts.glyph_callback) {
			iam_text_glyph g;
			g.index = char_idx;
			g.unit = unit_idx;
			g.codepoint = c;
			g.t = char_progress;
			g.raw_t = raw_progress;
			g.offset = IAM_VEC2_FROM_IM(ImVec2(offset_x, offset_y));
			g.scale = scale;
			g.rotation = rotation;
			g.alpha = alpha;
			g.color = glyph_color;
			opts.glyph_callback(&g, opts.glyph_user);
			ImVec2 off = IAM_VEC2_TO_IM(g.offset);
			offset_x = off.x; offset_y = off.y;
			scale = g.scale; rotation = g.rotation; alpha = g.alpha; glyph_color = g.color;
		}

		if (alpha <= 0.0f) {
			cursor_x += glyph->AdvanceX + opts.letter_spacing;
			p += char_len;
			char_idx++;
			continue;
		}

		// Calculate glyph dimensions
		float glyph_width = (glyph->X1 - glyph->X0);
		float glyph_height = (glyph->Y1 - glyph->Y0);
		float glyph_x = cursor_x + glyph->X0 + offset_x;
		float glyph_y = cursor_y + glyph->Y0 + offset_y;

		// Apply scale around center
		if (scale != 1.0f) {
			float center_x = glyph_x + glyph_width * 0.5f;
			float center_y = glyph_y + glyph_height * 0.5f;
			glyph_x = center_x + (glyph_x - center_x) * scale;
			glyph_y = center_y + (glyph_y - center_y) * scale;
			glyph_width *= scale;
			glyph_height *= scale;
		}

		// Apply color with alpha
		ImU32 color = glyph_color;
		ImU32 base_alpha = (color >> IM_COL32_A_SHIFT) & 0xFF;
		ImU32 final_alpha = (ImU32)(base_alpha * ImMin(alpha, 1.0f));
		color = (color & ~IM_COL32_A_MASK) | (final_alpha << IM_COL32_A_SHIFT);

		// Render the glyph
		if (rotation == 0.0f) {
			// Simple axis-aligned quad
			ImVec2 p0(glyph_x, glyph_y);
			ImVec2 p1(glyph_x + glyph_width, glyph_y + glyph_height);
			draw_list->PrimReserve(6, 4);
			draw_list->PrimRectUV(p0, p1, ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V1), color);
		} else {
			// Rotated quad
			float center_x = glyph_x + glyph_width * 0.5f;
			float center_y = glyph_y + glyph_height * 0.5f;
			float cos_r = ImCos(rotation);
			float sin_r = ImSin(rotation);

			ImVec2 corners[4];
			float hw = glyph_width * 0.5f;
			float hh = glyph_height * 0.5f;
			float local_corners[4][2] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};

			for (int i = 0; i < 4; i++) {
				float lx = local_corners[i][0];
				float ly = local_corners[i][1];
				corners[i].x = center_x + lx * cos_r - ly * sin_r;
				corners[i].y = center_y + lx * sin_r + ly * cos_r;
			}

			draw_list->PrimReserve(6, 4);
			draw_list->PrimQuadUV(
				corners[0], corners[1], corners[2], corners[3],
				ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V0),
				ImVec2(glyph->U1, glyph->V1), ImVec2(glyph->U0, glyph->V1),
				color
			);
		}

		cursor_x += glyph->AdvanceX + opts.letter_spacing;
		p += char_len;
		char_idx++;
	}
}

// ============================================================
// NOISE CHANNELS - Perlin/Simplex noise for organic movement
// ============================================================

namespace iam_noise_detail {

// Permutation table for noise
static int const perm[512] = {
	151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,
	125,136,171,168,68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,
	105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,
	82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,
	153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,
	106,157,184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,67,29,24,72,243,141,128,195,
	78,66,215,61,156,180,
	// Repeat
	151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,
	125,136,171,168,68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,
	105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,
	135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,
	82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,
	153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,228,
	251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,
	106,157,184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,67,29,24,72,243,141,128,195,
	78,66,215,61,156,180
};

inline float fade(float t) {
	return t * t * t * (t * (t * 6 - 15) + 10);
}

inline float lerp(float a, float b, float t) {
	return a + t * (b - a);
}

inline float grad2d(int hash, float x, float y) {
	int h = hash & 7;
	float u = h < 4 ? x : y;
	float v = h < 4 ? y : x;
	return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

// Simplex-specific gradient (returns values in [-1, 1] range)
inline float grad2d_simplex(int hash, float x, float y) {
	static float const grad2[][2] = {
		{1, 0}, {-1, 0}, {0, 1}, {0, -1},
		{0.7071067811865476f, 0.7071067811865476f},
		{-0.7071067811865476f, 0.7071067811865476f},
		{0.7071067811865476f, -0.7071067811865476f},
		{-0.7071067811865476f, -0.7071067811865476f}
	};
	int h = hash & 7;
	return grad2[h][0] * x + grad2[h][1] * y;
}

inline float grad3d(int hash, float x, float y, float z) {
	int h = hash & 15;
	float u = h < 8 ? x : y;
	float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float perlin_2d(float x, float y, int seed) {
	x += seed * 12.9898f;
	y += seed * 78.233f;

	int X = (int)floorf(x) & 255;
	int Y = (int)floorf(y) & 255;

	x -= floorf(x);
	y -= floorf(y);

	float u = fade(x);
	float v = fade(y);

	int A = perm[X] + Y;
	int B = perm[X + 1] + Y;

	return lerp(
		lerp(grad2d(perm[A], x, y), grad2d(perm[B], x - 1, y), u),
		lerp(grad2d(perm[A + 1], x, y - 1), grad2d(perm[B + 1], x - 1, y - 1), u),
		v
	) * 0.5f;
}

float perlin_3d(float x, float y, float z, int seed) {
	x += seed * 12.9898f;
	y += seed * 78.233f;
	z += seed * 37.719f;

	int X = (int)floorf(x) & 255;
	int Y = (int)floorf(y) & 255;
	int Z = (int)floorf(z) & 255;

	x -= floorf(x);
	y -= floorf(y);
	z -= floorf(z);

	float u = fade(x);
	float v = fade(y);
	float w = fade(z);

	int A = perm[X] + Y;
	int AA = perm[A] + Z;
	int AB = perm[A + 1] + Z;
	int B = perm[X + 1] + Y;
	int BA = perm[B] + Z;
	int BB = perm[B + 1] + Z;

	return lerp(
		lerp(
			lerp(grad3d(perm[AA], x, y, z), grad3d(perm[BA], x - 1, y, z), u),
			lerp(grad3d(perm[AB], x, y - 1, z), grad3d(perm[BB], x - 1, y - 1, z), u),
			v
		),
		lerp(
			lerp(grad3d(perm[AA + 1], x, y, z - 1), grad3d(perm[BA + 1], x - 1, y, z - 1), u),
			lerp(grad3d(perm[AB + 1], x, y - 1, z - 1), grad3d(perm[BB + 1], x - 1, y - 1, z - 1), u),
			v
		),
		w
	) * 0.5f;
}

// Simplex noise 2D
float simplex_2d(float x, float y, int seed) {
	float const F2 = 0.366025403f; // (sqrt(3) - 1) / 2
	float const G2 = 0.211324865f; // (3 - sqrt(3)) / 6

	x += seed * 12.9898f;
	y += seed * 78.233f;

	float s = (x + y) * F2;
	int i = (int)floorf(x + s);
	int j = (int)floorf(y + s);

	float t = (i + j) * G2;
	float X0 = i - t;
	float Y0 = j - t;
	float x0 = x - X0;
	float y0 = y - Y0;

	int i1, j1;
	if (x0 > y0) { i1 = 1; j1 = 0; }
	else { i1 = 0; j1 = 1; }

	float x1 = x0 - i1 + G2;
	float y1 = y0 - j1 + G2;
	float x2 = x0 - 1.0f + 2.0f * G2;
	float y2 = y0 - 1.0f + 2.0f * G2;

	int ii = i & 255;
	int jj = j & 255;

	float n0, n1, n2;

	float t0 = 0.5f - x0 * x0 - y0 * y0;
	if (t0 < 0) n0 = 0.0f;
	else {
		t0 *= t0;
		n0 = t0 * t0 * grad2d_simplex(perm[ii + perm[jj]], x0, y0);
	}

	float t1 = 0.5f - x1 * x1 - y1 * y1;
	if (t1 < 0) n1 = 0.0f;
	else {
		t1 *= t1;
		n1 = t1 * t1 * grad2d_simplex(perm[ii + i1 + perm[jj + j1]], x1, y1);
	}

	float t2 = 0.5f - x2 * x2 - y2 * y2;
	if (t2 < 0) n2 = 0.0f;
	else {
		t2 *= t2;
		n2 = t2 * t2 * grad2d_simplex(perm[ii + 1 + perm[jj + 1]], x2, y2);
	}

	// Scale to approximately [-1, 1] range
	return 45.23065f * (n0 + n1 + n2);
}

// Worley/Cellular noise 2D
float worley_2d(float x, float y, int seed) {
	x += seed * 12.9898f;
	y += seed * 78.233f;

	int xi = (int)floorf(x);
	int yi = (int)floorf(y);

	float min_dist = 1e10f;

	// Check 3x3 grid of cells
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			int cx = xi + dx;
			int cy = yi + dy;

			// Generate pseudo-random point in this cell
			int h = perm[(cx & 255) + perm[(cy & 255)]];
			float px = cx + (perm[h] / 255.0f);
			float py = cy + (perm[(h + 1) & 255] / 255.0f);

			// Distance to point
			float ddx = x - px;
			float ddy = y - py;
			float dist = sqrtf(ddx * ddx + ddy * ddy);

			if (dist < min_dist) {
				min_dist = dist;
			}
		}
	}

	// Normalize to approximately [-1, 1] range
	// Typical distances range from 0 to ~1.4
	return min_dist * 1.4f - 1.0f;
}

float value_noise_2d(float x, float y, int seed) {
	int xi = (int)floorf(x);
	int yi = (int)floorf(y);

	float xf = x - xi;
	float yf = y - yi;

	xi = xi & 255;
	yi = yi & 255;

	auto hash = [seed](int x, int y) {
		int h = perm[(x + seed) & 255];
		h = perm[(h + y) & 255];
		return (h / 255.0f) * 2.0f - 1.0f;
	};

	float u = fade(xf);
	float v = fade(yf);

	float n00 = hash(xi, yi);
	float n10 = hash(xi + 1, yi);
	float n01 = hash(xi, yi + 1);
	float n11 = hash(xi + 1, yi + 1);

	return lerp(
		lerp(n00, n10, u),
		lerp(n01, n11, u),
		v
	);
}

// Noise channel state
struct noise_state {
	float time;
	ImGuiID last_frame;
};

static ImPool<noise_state> g_noise_states;

noise_state* get_noise_state(ImGuiID id) {
	noise_state* s = g_noise_states.GetByKey(id);
	if (!s) {
		s = g_noise_states.GetOrAddByKey(id);
		s->time = 0.0f;
		s->last_frame = 0;
	}
	return s;
}

} // namespace iam_noise_detail

float iam_noise_2d(float x, float y, iam_noise_opts const& opts) {
	float total = 0.0f;
	float amplitude = 1.0f;
	float frequency = 1.0f;
	float max_value = 0.0f;

	for (int i = 0; i < opts.octaves; i++) {
		float nx = x * frequency;
		float ny = y * frequency;

		float value;
		switch (opts.type) {
			case iam_noise_simplex:
				value = iam_noise_detail::simplex_2d(nx, ny, opts.seed);
				break;
			case iam_noise_value:
				value = iam_noise_detail::value_noise_2d(nx, ny, opts.seed);
				break;
			case iam_noise_worley:
				value = iam_noise_detail::worley_2d(nx, ny, opts.seed);
				break;
			default: // perlin
				value = iam_noise_detail::perlin_2d(nx, ny, opts.seed);
				break;
		}

		total += value * amplitude;
		max_value += amplitude;

		amplitude *= opts.persistence;
		frequency *= opts.lacunarity;
	}

	return total / max_value;
}

float iam_noise_3d(float x, float y, float z, iam_noise_opts const& opts) {
	float total = 0.0f;
	float amplitude = 1.0f;
	float frequency = 1.0f;
	float max_value = 0.0f;

	for (int i = 0; i < opts.octaves; i++) {
		float nx = x * frequency;
		float ny = y * frequency;
		float nz = z * frequency;

		float value = iam_noise_detail::perlin_3d(nx, ny, nz, opts.seed);
		total += value * amplitude;
		max_value += amplitude;

		amplitude *= opts.persistence;
		frequency *= opts.lacunarity;
	}

	return total / max_value;
}

float iam_noise_channel_float(ImGuiID id, float frequency, float amplitude, iam_noise_opts const& opts, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude *= iam_detail::procedural_scale();
	using namespace iam_noise_detail;
	noise_state* s = get_noise_state(id);

	s->time += dt;
	float noise_val = iam_noise_2d(s->time * frequency, 0.0f, opts);
	return noise_val * amplitude;
}

static ImVec2 iam_noise_channel_vec2_impl(ImGuiID id, ImVec2 frequency, ImVec2 amplitude, iam_noise_opts const& opts, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);
	using namespace iam_noise_detail;
	noise_state* s = get_noise_state(id);

	s->time += dt;
	float nx = iam_noise_2d(s->time * frequency.x, 0.0f, opts);
	float ny = iam_noise_2d(s->time * frequency.y, 100.0f, opts); // Offset Y to get different values
	return ImVec2(nx * amplitude.x, ny * amplitude.y);
}

static ImVec4 iam_noise_channel_vec4_impl(ImGuiID id, ImVec4 frequency, ImVec4 amplitude, iam_noise_opts const& opts, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);
	using namespace iam_noise_detail;
	noise_state* s = get_noise_state(id);

	s->time += dt;
	float nx = iam_noise_2d(s->time * frequency.x, 0.0f, opts);
	float ny = iam_noise_2d(s->time * frequency.y, 100.0f, opts);
	float nz = iam_noise_2d(s->time * frequency.z, 200.0f, opts);
	float nw = iam_noise_2d(s->time * frequency.w, 300.0f, opts);
	return ImVec4(nx * amplitude.x, ny * amplitude.y, nz * amplitude.z, nw * amplitude.w);
}

float iam_smooth_noise_float(ImGuiID id, float amplitude, float speed, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude *= iam_detail::procedural_scale();
	iam_noise_opts opts;
	opts.octaves = 2;
	opts.persistence = 0.5f;
	return iam_noise_channel_float(id, speed, amplitude, opts, dt);
}

static ImVec2 iam_smooth_noise_vec2_impl(ImGuiID id, ImVec2 amplitude, float speed, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);
	iam_noise_opts opts;
	opts.octaves = 2;
	opts.persistence = 0.5f;
	return iam_noise_channel_vec2_impl(id, ImVec2(speed, speed), amplitude, opts, dt);
}

// ============================================================
static ImVec4 iam_smooth_noise_vec4_impl(ImGuiID id, ImVec4 amplitude, float speed, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);	iam_noise_opts opts;	opts.octaves = 2;	opts.persistence = 0.5f;	return iam_noise_channel_vec4_impl(id, ImVec4(speed, speed, speed, speed), amplitude, opts, dt);}ImVec4 iam_noise_channel_color_impl(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float frequency, iam_noise_opts const& opts, int color_space, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);	ImVec4 noise = iam_noise_channel_vec4_impl(id, ImVec4(frequency, frequency, frequency, frequency), amplitude, opts, dt);	ImVec4 working;	switch (color_space) {		case iam_col_srgb_linear:			working = iam_detail::color::srgb_to_linear(base_color);			working.x += noise.x; working.y += noise.y; working.z += noise.z; working.w += noise.w;			return iam_detail::color::linear_to_srgb(working);		case iam_col_hsv:			working = iam_detail::color::srgb_to_hsv(base_color);			working.x = ImFmod(working.x + noise.x + 1.0f, 1.0f);			working.y = ImClamp(working.y + noise.y, 0.0f, 1.0f);			working.z = ImClamp(working.z + noise.z, 0.0f, 1.0f);			working.w = ImClamp(working.w + noise.w, 0.0f, 1.0f);			return iam_detail::color::hsv_to_srgb(working);		case iam_col_oklab:			working = iam_detail::color::srgb_to_oklab(base_color);			working.x += noise.x; working.y += noise.y; working.z += noise.z; working.w += noise.w;			return iam_detail::color::oklab_to_srgb(working);		case iam_col_oklch:			working = iam_detail::color::srgb_to_oklch(base_color);			working.x += noise.x; working.y += noise.y;			working.z = ImFmod(working.z + noise.z + 360.0f, 360.0f);			working.w += noise.w;			return iam_detail::color::oklch_to_srgb(working);		default:			return ImVec4(ImClamp(base_color.x + noise.x, 0.0f, 1.0f), ImClamp(base_color.y + noise.y, 0.0f, 1.0f), ImClamp(base_color.z + noise.z, 0.0f, 1.0f), ImClamp(base_color.w + noise.w, 0.0f, 1.0f));	}}ImVec4 iam_smooth_noise_color_impl(ImGuiID id, ImVec4 base_color, ImVec4 amplitude, float speed, int color_space, float dt) {
	iam_detail::procedural_guard procedural_guard_; if (procedural_guard_.outer) amplitude = iam_detail::procedural_scaled(amplitude);	iam_noise_opts opts;	opts.octaves = 2;	opts.persistence = 0.5f;	return iam_noise_channel_color_impl(id, base_color, amplitude, speed, opts, color_space, dt);}
// STYLE INTERPOLATION - Animate between ImGuiStyle themes
// ============================================================

namespace iam_style_detail {

struct registered_style {
	ImGuiStyle style;
	bool valid;
};

static ImPool<registered_style> g_styles;

// Style tween state
struct style_tween_state {
	ImGuiID target_style;
	ImGuiID source_style;
	float t;
	float duration;
	iam_ease_desc ease;
	int color_space;
	bool active;
};

static ImPool<style_tween_state> g_style_tweens;

inline float lerp_float(float a, float b, float t) {
	return a + (b - a) * t;
}

inline ImVec2 lerp_vec2(ImVec2 a, ImVec2 b, float t) {
	return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

void blend_styles(ImGuiStyle const& a, ImGuiStyle const& b, float t, ImGuiStyle* out, int color_space) {
	// Blend all float properties
	out->Alpha = lerp_float(a.Alpha, b.Alpha, t);
	out->DisabledAlpha = lerp_float(a.DisabledAlpha, b.DisabledAlpha, t);
	out->WindowRounding = lerp_float(a.WindowRounding, b.WindowRounding, t);
	out->WindowBorderSize = lerp_float(a.WindowBorderSize, b.WindowBorderSize, t);
	out->ChildRounding = lerp_float(a.ChildRounding, b.ChildRounding, t);
	out->ChildBorderSize = lerp_float(a.ChildBorderSize, b.ChildBorderSize, t);
	out->PopupRounding = lerp_float(a.PopupRounding, b.PopupRounding, t);
	out->PopupBorderSize = lerp_float(a.PopupBorderSize, b.PopupBorderSize, t);
	out->FrameRounding = lerp_float(a.FrameRounding, b.FrameRounding, t);
	out->FrameBorderSize = lerp_float(a.FrameBorderSize, b.FrameBorderSize, t);
	out->IndentSpacing = lerp_float(a.IndentSpacing, b.IndentSpacing, t);
	out->ColumnsMinSpacing = lerp_float(a.ColumnsMinSpacing, b.ColumnsMinSpacing, t);
	out->ScrollbarSize = lerp_float(a.ScrollbarSize, b.ScrollbarSize, t);
	out->ScrollbarRounding = lerp_float(a.ScrollbarRounding, b.ScrollbarRounding, t);
	out->GrabMinSize = lerp_float(a.GrabMinSize, b.GrabMinSize, t);
	out->GrabRounding = lerp_float(a.GrabRounding, b.GrabRounding, t);
	out->TabRounding = lerp_float(a.TabRounding, b.TabRounding, t);
	out->TabBorderSize = lerp_float(a.TabBorderSize, b.TabBorderSize, t);
	out->TabBarBorderSize = lerp_float(a.TabBarBorderSize, b.TabBarBorderSize, t);
	out->SeparatorTextBorderSize = lerp_float(a.SeparatorTextBorderSize, b.SeparatorTextBorderSize, t);

	// Blend vec2 properties
	out->WindowPadding = lerp_vec2(a.WindowPadding, b.WindowPadding, t);
	out->WindowMinSize = lerp_vec2(a.WindowMinSize, b.WindowMinSize, t);
	out->WindowTitleAlign = lerp_vec2(a.WindowTitleAlign, b.WindowTitleAlign, t);
	out->FramePadding = lerp_vec2(a.FramePadding, b.FramePadding, t);
	out->ItemSpacing = lerp_vec2(a.ItemSpacing, b.ItemSpacing, t);
	out->ItemInnerSpacing = lerp_vec2(a.ItemInnerSpacing, b.ItemInnerSpacing, t);
	out->CellPadding = lerp_vec2(a.CellPadding, b.CellPadding, t);
	out->ButtonTextAlign = lerp_vec2(a.ButtonTextAlign, b.ButtonTextAlign, t);
	out->SelectableTextAlign = lerp_vec2(a.SelectableTextAlign, b.SelectableTextAlign, t);
	out->SeparatorTextAlign = lerp_vec2(a.SeparatorTextAlign, b.SeparatorTextAlign, t);
	out->SeparatorTextPadding = lerp_vec2(a.SeparatorTextPadding, b.SeparatorTextPadding, t);

	// Blend all colors using the existing color space infrastructure
	for (int i = 0; i < ImGuiCol_COUNT; i++) {
		out->Colors[i] = iam_detail::color::lerp_color(a.Colors[i], b.Colors[i], t, color_space);
	}
}

} // namespace iam_style_detail

void iam_style_register(ImGuiID style_id, ImGuiStyle const& style) {
	using namespace iam_style_detail;
	registered_style* s = g_styles.GetOrAddByKey(style_id);
	s->style = style;
	s->valid = true;
}

void iam_style_register_current(ImGuiID style_id) {
	iam_style_register(style_id, ImGui::GetStyle());
}

bool iam_style_exists(ImGuiID style_id) {
	using namespace iam_style_detail;
	registered_style* s = g_styles.GetByKey(style_id);
	return s && s->valid;
}

void iam_style_unregister(ImGuiID style_id) {
	using namespace iam_style_detail;
	registered_style* s = g_styles.GetByKey(style_id);
	if (s) {
		s->valid = false;
	}
}

void iam_style_blend_to(ImGuiID style_a, ImGuiID style_b, float t, ImGuiStyle* out_style, int color_space) {
	using namespace iam_style_detail;

	registered_style* sa = g_styles.GetByKey(style_a);
	registered_style* sb = g_styles.GetByKey(style_b);

	if (!sa || !sa->valid || !sb || !sb->valid) {
		return;
	}

	blend_styles(sa->style, sb->style, t, out_style, color_space);
}

void iam_style_blend(ImGuiID style_a, ImGuiID style_b, float t, int color_space) {
	ImGuiStyle& current = ImGui::GetStyle();
	iam_style_blend_to(style_a, style_b, t, &current, color_space);
}

void iam_style_tween(ImGuiID id, ImGuiID target_style, float duration, iam_ease_desc const& ease, int color_space, float dt) {
	using namespace iam_style_detail;

	style_tween_state* state = g_style_tweens.GetOrAddByKey(id);

	// Check if target changed
	if (state->target_style != target_style || !state->active) {
		// Save current style as source
		ImGuiID source_id = ImHashStr("__current_style_source");
		iam_style_register_current(source_id);

		state->source_style = source_id;
		state->target_style = target_style;
		state->t = 0.0f;
		state->duration = duration;
		state->ease = ease;
		state->color_space = color_space;
		state->active = true;
	}

	if (!state->active) return;

	state->t += dt / state->duration;
	if (state->t >= 1.0f) {
		state->t = 1.0f;
		state->active = false;
	}

	float eased_t = iam_detail::eval(state->ease, state->t);
	iam_style_blend(state->source_style, state->target_style, eased_t, state->color_space);
}

// ============================================================
// GRADIENT INTERPOLATION
// ============================================================
iam_gradient& iam_gradient::add(float position, iam_vec4 color_in) {
	ImVec4 color = IAM_VEC4_TO_IM(color_in);
	// Find insertion point to maintain sorted order
	int insert_idx = positions.Size;
	for (int i = 0; i < positions.Size; ++i) {
		if (position < positions[i]) {
			insert_idx = i;
			break;
		}
	}
	// Insert at the correct position
	positions.insert(positions.Data + insert_idx, position);
	colors.insert(colors.Data + insert_idx, color);
	return *this;
}

iam_vec4 iam_gradient::sample(float t, int color_space) const {
	auto body = [&]() -> ImVec4 {
		int count = positions.Size;
		if (count == 0) return ImVec4(1, 1, 1, 1);
		if (count == 1) return colors[0];
		// Clamp t
		if (t <= positions[0]) return colors[0];
		if (t >= positions[count - 1]) return colors[count - 1];
		// Find the two stops we are between (positions are already sorted)
		for (int i = 0; i < count - 1; ++i) {
			if (t >= positions[i] && t <= positions[i + 1]) {
				float range = positions[i + 1] - positions[i];
				float local_t = (range > 1e-6f) ? (t - positions[i]) / range : 0.0f;
				return iam_detail::color::lerp_color(colors[i], colors[i + 1], local_t, color_space);
			}
		}
		return colors[count - 1];
	};
	return IAM_VEC4_FROM_IM(body());
}

iam_gradient iam_gradient::solid(iam_vec4 color) {
	iam_gradient g;
	g.add(0.0f, color);
	g.add(1.0f, color);
	return g;
}

iam_gradient iam_gradient::two_color(iam_vec4 start, iam_vec4 end) {
	iam_gradient g;
	g.add(0.0f, start);
	g.add(1.0f, end);
	return g;
}

iam_gradient iam_gradient::three_color(iam_vec4 start, iam_vec4 mid, iam_vec4 end) {
	iam_gradient g;
	g.add(0.0f, start);
	g.add(0.5f, mid);
	g.add(1.0f, end);
	return g;
}

iam_gradient iam_gradient_lerp(iam_gradient const& a, iam_gradient const& b, float t, int color_space) {
	// Strategy: sample both gradients at unified positions and blend
	ImVector<float> all_positions;

	for (int i = 0; i < a.positions.Size; ++i) {
		all_positions.push_back(a.positions[i]);
	}
	for (int i = 0; i < b.positions.Size; ++i) {
		bool found = false;
		for (int j = 0; j < all_positions.Size; ++j) {
			if (fabsf(all_positions[j] - b.positions[i]) < 1e-6f) {
				found = true;
				break;
			}
		}
		if (!found) {
			all_positions.push_back(b.positions[i]);
		}
	}

	// Sort positions
	for (int i = 0; i < all_positions.Size - 1; ++i) {
		for (int j = i + 1; j < all_positions.Size; ++j) {
			if (all_positions[j] < all_positions[i]) {
				float tmp = all_positions[i];
				all_positions[i] = all_positions[j];
				all_positions[j] = tmp;
			}
		}
	}

	// Build result gradient
	iam_gradient result;
	for (int i = 0; i < all_positions.Size; ++i) {
		ImVec4 color_a = IAM_VEC4_TO_IM(a.sample(all_positions[i], color_space));
		ImVec4 color_b = IAM_VEC4_TO_IM(b.sample(all_positions[i], color_space));
		ImVec4 blended = iam_detail::color::lerp_color(color_a, color_b, t, color_space);
		result.add(all_positions[i], IAM_VEC4_FROM_IM(blended));
	}

	return result;
}

// Gradient channel for tweening
namespace iam_gradient_detail {

struct gradient_chan {
	iam_gradient current, start, target;
	float dur, t;  // t is cached progress for backward compatibility
	double start_time;
	iam_ease_desc ez;
	int policy;
	int color_space;
	unsigned last_seen_frame;
	unsigned sleeping;

	gradient_chan() {
		dur = 1e-6f; t = 1.0f; start_time = 0;
		ez = { iam_ease_out_cubic, 0, 0, 0, 0 };
		policy = iam_policy_crossfade;
		color_space = iam_col_oklab;
		last_seen_frame = 0;
		sleeping = 1;
	}

	void set(iam_gradient const& trg, float d, iam_ease_desc const& e, int pol, int cs) {
		start = current;
		target = trg;
		d = iam_detail::motion_dur(d);
		dur = (d <= 1e-6f ? 1e-6f : d);
		start_time = iam_detail::g_global_time;
		t = 0;
		ez = e;
		policy = pol;
		color_space = cs;
		sleeping = 0;
		if (iam_detail::g_reduced_motion) { current = target; t = 1.0f; sleeping = 1; }  // reduced motion: land immediately
	}

	float progress() {
		if (sleeping) { t = 1.0f; return 1.0f; }
		t = (float)((iam_detail::g_global_time - start_time) / dur);
		if (t < 0.f) t = 0.f; else if (t > 1.f) t = 1.f;
		return t;
	}

	iam_gradient evaluate() {
		if (sleeping) return current;
		progress();
		if (t >= 1.0f) { current = target; sleeping = 1; return current; }
		float k = iam_detail::eval(ez, t);
		current = iam_gradient_lerp(start, target, k, color_space);
		return current;
	}

	void tick(float) { evaluate(); }
};

static ImPool<gradient_chan> g_gradient_pool;
static unsigned g_gradient_frame = 0;

} // namespace iam_gradient_detail

iam_gradient iam_tween_gradient(ImGuiID id, ImGuiID channel_id, iam_gradient const& target, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt) {
	using namespace iam_gradient_detail;

	dt *= iam_detail::g_time_scale;
	ImGuiID key = iam_detail::make_key(id, channel_id);
	gradient_chan* c = g_gradient_pool.GetOrAddByKey(key);
	c->last_seen_frame = g_gradient_frame;

	// Fast path: sleeping and target unchanged
	if (c->sleeping && c->target.stop_count() == target.stop_count()) {
		bool same = true;
		for (int i = 0; i < target.stop_count() && same; ++i) {
			if (fabsf(c->target.positions[i] - target.positions[i]) > 1e-6f) same = false;
			if (fabsf(c->target.colors[i].x - target.colors[i].x) > 1e-6f) same = false;
			if (fabsf(c->target.colors[i].y - target.colors[i].y) > 1e-6f) same = false;
			if (fabsf(c->target.colors[i].z - target.colors[i].z) > 1e-6f) same = false;
			if (fabsf(c->target.colors[i].w - target.colors[i].w) > 1e-6f) same = false;
		}
		if (same) return c->current;
	}

	// Check if target changed
	bool change = (c->policy != policy) || (c->ez.type != ez.type) ||
	              (c->ez.p0 != ez.p0) || (c->ez.p1 != ez.p1) || (c->ez.p2 != ez.p2) || (c->ez.p3 != ez.p3) ||
	              (c->progress() >= 1.0f) || (c->target.stop_count() != target.stop_count());

	if (!change) {
		for (int i = 0; i < target.stop_count() && !change; ++i) {
			if (fabsf(c->target.positions[i] - target.positions[i]) > 1e-6f) change = true;
			if (fabsf(c->target.colors[i].x - target.colors[i].x) > 1e-6f) change = true;
			if (fabsf(c->target.colors[i].y - target.colors[i].y) > 1e-6f) change = true;
			if (fabsf(c->target.colors[i].z - target.colors[i].z) > 1e-6f) change = true;
			if (fabsf(c->target.colors[i].w - target.colors[i].w) > 1e-6f) change = true;
		}
	}

	if (change) {
		if (policy == iam_policy_cut) {
			c->current = c->start = c->target = target;
			c->sleeping = 1; c->dur = 1e-6f; c->ez = ez; c->policy = policy; c->color_space = color_space;
			c->sleeping = 1;
		} else {
			if (c->progress() < 1.0f && dt > 0) c->tick(dt);
			c->set(target, dur, ez, policy, color_space);
			c->tick(dt);
		}
	} else {
		c->tick(dt);
	}

	return c->current;
}

// ============================================================
// TRANSFORM INTERPOLATION
// ============================================================

iam_transform iam_transform::operator*(iam_transform const& other) const {
	// Combine: first apply other, then this
	// Result position = this.apply(other.position)
	// Result rotation = this.rotation + other.rotation
	// Result scale = this.scale * other.scale
	ImVec2 pos = IAM_VEC2_TO_IM(position), scl = IAM_VEC2_TO_IM(scale);
	ImVec2 other_position = IAM_VEC2_TO_IM(other.position), other_scale = IAM_VEC2_TO_IM(other.scale);
	iam_transform result;
	result.scale = IAM_VEC2_FROM_IM(ImVec2(scl.x * other_scale.x, scl.y * other_scale.y));
	result.rotation = rotation + other.rotation;

	// Apply this transform to other's position
	float c = ImCos(rotation);
	float s = ImSin(rotation);
	result.position = IAM_VEC2_FROM_IM(ImVec2(
		pos.x + (other_position.x * c - other_position.y * s) * scl.x,
		pos.y + (other_position.x * s + other_position.y * c) * scl.y));

	return result;
}

iam_vec2 iam_transform::apply(iam_vec2 point_in) const {
	ImVec2 point = IAM_VEC2_TO_IM(point_in);
	ImVec2 pos = IAM_VEC2_TO_IM(position), scl = IAM_VEC2_TO_IM(scale);
	// Apply scale, then rotation, then translation
	float c = ImCos(rotation);
	float s = ImSin(rotation);
	float sx = point.x * scl.x;
	float sy = point.y * scl.y;
	return IAM_VEC2_FROM_IM(ImVec2(
		pos.x + sx * c - sy * s,
		pos.y + sx * s + sy * c
	));
}

iam_transform iam_transform::inverse() const {
	ImVec2 pos = IAM_VEC2_TO_IM(position), scl = IAM_VEC2_TO_IM(scale);
	iam_transform result;

	// Inverse scale
	ImVec2 inv_scale(
		fabsf(scl.x) > 1e-6f ? 1.0f / scl.x : 1.0f,
		fabsf(scl.y) > 1e-6f ? 1.0f / scl.y : 1.0f
	);
	result.scale = IAM_VEC2_FROM_IM(inv_scale);

	// Inverse rotation
	result.rotation = -rotation;

	// Inverse translation (apply inverse rotation and scale to negated position)
	float c = ImCos(-rotation);
	float s = ImSin(-rotation);
	result.position = IAM_VEC2_FROM_IM(ImVec2(
		(-pos.x * c + pos.y * s) * inv_scale.x,
		(-pos.x * s - pos.y * c) * inv_scale.y));

	return result;
}

// Calculate rotation difference based on mode
static float angle_diff_mode(float from, float to, int mode) {
	// Normalize angles to [0, 2pi)
	float from_n = ImFmod(from, IAM_2PI);
	if (from_n < 0) from_n += IAM_2PI;
	float to_n = ImFmod(to, IAM_2PI);
	if (to_n < 0) to_n += IAM_2PI;

	float diff = to_n - from_n;

	switch (mode) {
	case iam_rotation_shortest:
		// Shortest path: wrap to [-IAM_PI, IAM_PI]
		if (diff > IAM_PI) diff -= IAM_2PI;
		else if (diff < -IAM_PI) diff += IAM_2PI;
		break;

	case iam_rotation_longest:
		// Longest path: take the long way around
		if (diff > 0 && diff < IAM_PI) diff -= IAM_2PI;
		else if (diff < 0 && diff > -IAM_PI) diff += IAM_2PI;
		break;

	case iam_rotation_cw:
		// Clockwise (positive): ensure diff is positive
		if (diff < 0) diff += IAM_2PI;
		break;

	case iam_rotation_ccw:
		// Counter-clockwise (negative): ensure diff is negative
		if (diff > 0) diff -= IAM_2PI;
		break;

	case iam_rotation_direct:
	default:
		// Direct: use raw difference (to - from), no wrapping
		diff = to - from;
		break;
	}

	return diff;
}

// Legacy helper for internal use (shortest path)
static float angle_diff(float from, float to) {
	return angle_diff_mode(from, to, iam_rotation_shortest);
}

iam_transform iam_transform_lerp(iam_transform const& a, iam_transform const& b, float t, int rotation_mode) {
	ImVec2 a_pos = IAM_VEC2_TO_IM(a.position), b_pos = IAM_VEC2_TO_IM(b.position);
	ImVec2 a_scl = IAM_VEC2_TO_IM(a.scale), b_scl = IAM_VEC2_TO_IM(b.scale);
	iam_transform result;

	// Lerp position linearly
	float pos_x = a_pos.x + (b_pos.x - a_pos.x) * t;
	float pos_y = a_pos.y + (b_pos.y - a_pos.y) * t;
	result.position = IAM_VEC2_FROM_IM(ImVec2(pos_x, pos_y));

	// Lerp scale linearly
	float scale_x = a_scl.x + (b_scl.x - a_scl.x) * t;
	float scale_y = a_scl.y + (b_scl.y - a_scl.y) * t;
	result.scale = IAM_VEC2_FROM_IM(ImVec2(scale_x, scale_y));

	// Lerp rotation using specified mode
	float diff = angle_diff_mode(a.rotation, b.rotation, rotation_mode);
	result.rotation = a.rotation + diff * t;

	return result;
}

iam_transform iam_transform_from_matrix(float m00, float m01, float m10, float m11, float tx, float ty) {
	iam_transform t;

	// Extract translation
	t.position = IAM_VEC2_FROM_IM(ImVec2(tx, ty));

	// Extract scale
	ImVec2 scale(sqrtf(m00 * m00 + m10 * m10), sqrtf(m01 * m01 + m11 * m11));

	// Check for reflection (negative determinant)
	float det = m00 * m11 - m01 * m10;
	if (det < 0) scale.x = -scale.x;
	t.scale = IAM_VEC2_FROM_IM(scale);

	// Extract rotation
	t.rotation = atan2f(m10, m00);

	return t;
}

void iam_transform_to_matrix(iam_transform const& t, float* out_matrix) {
	ImVec2 position = IAM_VEC2_TO_IM(t.position), scale = IAM_VEC2_TO_IM(t.scale);
	float c = ImCos(t.rotation);
	float s = ImSin(t.rotation);

	// Row-major: [m00 m01 tx; m10 m11 ty]
	out_matrix[0] = c * scale.x;  // m00
	out_matrix[1] = -s * scale.y; // m01
	out_matrix[2] = position.x;   // tx
	out_matrix[3] = s * scale.x;  // m10
	out_matrix[4] = c * scale.y;  // m11
	out_matrix[5] = position.y;   // ty
}

// Transform channel for tweening
namespace iam_transform_detail {

struct transform_chan {
	iam_transform current, start, target;
	float dur, t;  // t is cached progress for backward compatibility
	double start_time;
	iam_ease_desc ez;
	int policy;
	int rotation_mode;
	unsigned last_seen_frame;
	unsigned sleeping;

	transform_chan() : current(), start(), target() {
		dur = 1e-6f; t = 1.0f; start_time = 0;
		ez = { iam_ease_out_cubic, 0, 0, 0, 0 };
		policy = iam_policy_crossfade;
		rotation_mode = iam_rotation_shortest;
		last_seen_frame = 0;
		sleeping = 1;
	}

	void set(iam_transform const& trg, float d, iam_ease_desc const& e, int pol, int rot_mode) {
		start = current;
		target = trg;
		d = iam_detail::motion_dur(d);
		dur = (d <= 1e-6f ? 1e-6f : d);
		start_time = iam_detail::g_global_time;
		t = 0;
		ez = e;
		policy = pol;
		rotation_mode = rot_mode;
		sleeping = 0;
		if (iam_detail::g_reduced_motion) { current = target; t = 1.0f; sleeping = 1; }  // reduced motion: land immediately
	}

	float progress() {
		if (sleeping) { t = 1.0f; return 1.0f; }
		t = (float)((iam_detail::g_global_time - start_time) / dur);
		if (t < 0.f) t = 0.f; else if (t > 1.f) t = 1.f;
		return t;
	}

	iam_transform evaluate() {
		if (sleeping) return current;
		progress();
		if (t >= 1.0f) { current = target; sleeping = 1; return current; }
		float k = iam_detail::eval(ez, t);
		current = iam_transform_lerp(start, target, k, rotation_mode);
		return current;
	}

	void tick(float) { evaluate(); }
};

static ImPool<transform_chan> g_transform_pool;
static unsigned g_transform_frame = 0;

} // namespace iam_transform_detail

iam_transform iam_tween_transform(ImGuiID id, ImGuiID channel_id, iam_transform const& target, float dur, iam_ease_desc const& ez, int policy, int rotation_mode, float dt) {
	using namespace iam_transform_detail;

	dt *= iam_detail::g_time_scale;
	ImGuiID key = iam_detail::make_key(id, channel_id);

	// Check if channel exists first
	transform_chan* c = g_transform_pool.GetByKey(key);
	bool is_new = (c == nullptr);
	if (is_new) {
		c = g_transform_pool.GetOrAddByKey(key);
		// Explicitly initialize new channel
		c->current = target;  // Start at target position (no animation on first frame)
		c->start = target;
		c->target = target;
		c->dur = 1e-6f;
		c->t = 1.0f;
		c->start_time = iam_detail::g_global_time;
		c->ez = ez;
		c->policy = policy;
		c->rotation_mode = rotation_mode;
		c->sleeping = 1;
	}
	c->last_seen_frame = g_transform_frame;

	// Fast path: sleeping and target unchanged
	if (c->sleeping) {
		float pos_diff = fabsf(IAM_VEC2_TO_IM(c->target.position).x - IAM_VEC2_TO_IM(target.position).x) + fabsf(IAM_VEC2_TO_IM(c->target.position).y - IAM_VEC2_TO_IM(target.position).y);
		float rot_diff = fabsf(angle_diff(c->target.rotation, target.rotation));
		float scale_diff = fabsf(IAM_VEC2_TO_IM(c->target.scale).x - IAM_VEC2_TO_IM(target.scale).x) + fabsf(IAM_VEC2_TO_IM(c->target.scale).y - IAM_VEC2_TO_IM(target.scale).y);
		if (pos_diff <= 1e-6f && rot_diff <= 1e-6f && scale_diff <= 1e-6f) {
			return c->current;
		}
	}

	// Check if target changed
	float pos_diff = fabsf(IAM_VEC2_TO_IM(c->target.position).x - IAM_VEC2_TO_IM(target.position).x) + fabsf(IAM_VEC2_TO_IM(c->target.position).y - IAM_VEC2_TO_IM(target.position).y);
	float rot_diff = fabsf(angle_diff(c->target.rotation, target.rotation));
	float scale_diff = fabsf(IAM_VEC2_TO_IM(c->target.scale).x - IAM_VEC2_TO_IM(target.scale).x) + fabsf(IAM_VEC2_TO_IM(c->target.scale).y - IAM_VEC2_TO_IM(target.scale).y);

	bool change = (c->policy != policy) || (c->rotation_mode != rotation_mode) || (c->ez.type != ez.type) ||
	              (c->ez.p0 != ez.p0) || (c->ez.p1 != ez.p1) || (c->ez.p2 != ez.p2) || (c->ez.p3 != ez.p3) ||
	              (pos_diff > 1e-6f) || (rot_diff > 1e-6f) || (scale_diff > 1e-6f) || (c->progress() >= 1.0f);

	if (change) {
		if (policy == iam_policy_cut) {
			c->current = c->start = c->target = target;
			c->sleeping = 1; c->dur = 1e-6f; c->ez = ez; c->policy = policy;
			c->rotation_mode = rotation_mode;
			c->sleeping = 1;
		} else {
			if (c->progress() < 1.0f && dt > 0) c->tick(dt);
			c->set(target, dur, ez, policy, rotation_mode);
			c->tick(dt);
		}
	} else {
		c->tick(dt);
	}

	return c->current;
}

// ----------------------------------------------------
// Unified Inspector (combines Debug Window + Animation Inspector)
// ----------------------------------------------------

void iam_show_unified_inspector(bool* p_open) {
	if (!ImGui::Begin("ImAnim Inspector", p_open, ImGuiWindowFlags_None)) {
		ImGui::End();
		return;
	}

	if (ImGui::BeginTabBar("UnifiedInspectorTabs")) {
		// Debug Tab
		if (ImGui::BeginTabItem("Debug")) {
			// Time scale control
			if (ImGui::CollapsingHeader("Time Scale", ImGuiTreeNodeFlags_DefaultOpen)) {
				float scale = iam_detail::g_time_scale;
				ImGui::SliderFloat("Global Time Scale", &scale, 0.0f, 2.0f, "%.2fx");
				if (scale != iam_detail::g_time_scale) {
					iam_detail::g_time_scale = scale;
				}
				ImGui::SameLine();
				if (ImGui::Button("Reset##timescale")) {
					iam_detail::g_time_scale = 1.0f;
				}

				ImGui::Text("Presets:");
				ImGui::SameLine();
				if (ImGui::SmallButton("0.1x")) iam_detail::g_time_scale = 0.1f;
				ImGui::SameLine();
				if (ImGui::SmallButton("0.25x")) iam_detail::g_time_scale = 0.25f;
				ImGui::SameLine();
				if (ImGui::SmallButton("0.5x")) iam_detail::g_time_scale = 0.5f;
				ImGui::SameLine();
				if (ImGui::SmallButton("1x")) iam_detail::g_time_scale = 1.0f;
				ImGui::SameLine();
				if (ImGui::SmallButton("2x")) iam_detail::g_time_scale = 2.0f;
			}

			// Tween stats
			if (ImGui::CollapsingHeader("Tween Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Text("Active Tweens:");
				ImGui::Indent();
				ImGui::Text("Float:  %d", iam_detail::g_float.pool.GetAliveCount());
				ImGui::Text("Vec2:   %d", iam_detail::g_vec2.pool.GetAliveCount());
				ImGui::Text("Vec4:   %d", iam_detail::g_vec4.pool.GetAliveCount());
				ImGui::Text("Int:    %d", iam_detail::g_int.pool.GetAliveCount());
				ImGui::Text("Color:  %d", iam_detail::g_color.pool.GetAliveCount());
				int total = iam_detail::g_float.pool.GetAliveCount() +
				            iam_detail::g_vec2.pool.GetAliveCount() +
				            iam_detail::g_vec4.pool.GetAliveCount() +
				            iam_detail::g_int.pool.GetAliveCount() +
				            iam_detail::g_color.pool.GetAliveCount();
				ImGui::Unindent();
				ImGui::Text("Total:  %d", total);
			}

			// Clip stats
			if (ImGui::CollapsingHeader("Clip Stats")) {
				ImGui::Text("Registered Clips: %d", iam_clip_detail::g_clip_sys.clips.Size);
				ImGui::Text("Active Instances: %d", iam_clip_detail::g_clip_sys.instances.Size);
			}

			ImGui::EndTabItem();
		}

		// Animation Inspector Tab
		if (ImGui::BeginTabItem("Animations")) {
			// List active clips/instances
			auto& instances = iam_clip_detail::g_clip_sys.instances;
			if (instances.Size == 0) {
				ImGui::TextDisabled("No active animation instances");
			} else {
				for (int i = 0; i < instances.Size; i++) {
					auto& inst = instances[i];
					ImGui::PushID(i);
					if (ImGui::TreeNode("Instance", "Instance %d (clip 0x%08X)", i, inst.clip_id)) {
						ImGui::Text("Clip ID: 0x%08X", inst.clip_id);
						ImGui::Text("Time: %.2f", inst.time);
						ImGui::Text("Playing: %s", inst.playing ? "Yes" : "No");
						ImGui::Text("Loops Left: %d", inst.loops_left);
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
			ImGui::EndTabItem();
		}

		// Performance Tab
		if (ImGui::BeginTabItem("Performance")) {
			auto& prof = iam_detail::g_profiler;

			// Enable/disable toggle
			bool enabled = prof.enabled;
			if (ImGui::Checkbox("Enable Profiler", &enabled)) {
				iam_profiler_enable(enabled);
			}

			if (!prof.enabled) {
				ImGui::TextDisabled("Profiler is disabled. Enable to collect timing data.");
				ImGui::TextDisabled("Call iam_profiler_begin_frame() at frame start,");
				ImGui::TextDisabled("iam_profiler_end_frame() at frame end.");
			} else {
				ImGui::Separator();

				// Frame time graph
				if (ImGui::CollapsingHeader("Frame Time", ImGuiTreeNodeFlags_DefaultOpen)) {
					// Calculate average frame time
					float avg_frame_time = 0.0f;
					float max_frame_time = 0.0f;
					for (int i = 0; i < iam_detail::PROFILER_HISTORY_SIZE; i++) {
						avg_frame_time += prof.frame_history[i];
						if (prof.frame_history[i] > max_frame_time) max_frame_time = prof.frame_history[i];
					}
					avg_frame_time /= iam_detail::PROFILER_HISTORY_SIZE;

					ImGui::Text("Current: %.3f ms | Avg: %.3f ms | Max: %.3f ms",
						prof.frame_total_time, avg_frame_time, max_frame_time);

					// Plot frame time history
					char overlay[64];
					snprintf(overlay, sizeof(overlay), "%.2f ms", (float)prof.frame_total_time);
					ImGui::PlotLines("##FrameTime", prof.frame_history, iam_detail::PROFILER_HISTORY_SIZE,
						prof.frame_history_idx, overlay, 0.0f, max_frame_time * 1.2f, ImVec2(-1, 80));
				}

				// Per-section breakdown
				if (ImGui::CollapsingHeader("Section Breakdown", ImGuiTreeNodeFlags_DefaultOpen)) {
					if (prof.section_count == 0) {
						ImGui::TextDisabled("No profiler sections recorded.");
						ImGui::TextDisabled("Use iam_profiler_begin(\"name\") and iam_profiler_end() in code.");
					} else {
						float avail_width = ImGui::GetContentRegionAvail().x;
						float col0_width = avail_width * 0.25f;  // Section name
						float col1_width = avail_width * 0.125f; // Time
						float col2_width = avail_width * 0.125f; // Calls
						float col3_width = avail_width * 0.5f;   // Graph (remaining)

						ImGui::Columns(4, "ProfilerSections");
						ImGui::SetColumnWidth(0, col0_width);
						ImGui::SetColumnWidth(1, col1_width);
						ImGui::SetColumnWidth(2, col2_width);
						ImGui::SetColumnWidth(3, col3_width);
						ImGui::Text("Section"); ImGui::NextColumn();
						ImGui::Text("Time (ms)"); ImGui::NextColumn();
						ImGui::Text("Calls"); ImGui::NextColumn();
						ImGui::Text("Graph"); ImGui::NextColumn();
						ImGui::Separator();

						float row_height = ImGui::GetTextLineHeight() * 1.5f;
						for (int i = 0; i < prof.section_count; i++) {
							auto& sec = prof.sections[i];
							// Calculate max for this section's history
							float sec_max = 0.0f;
							for (int j = 0; j < iam_detail::PROFILER_HISTORY_SIZE; j++) {
								if (sec.history[j] > sec_max) sec_max = sec.history[j];
							}
							if (sec_max < 0.01f) sec_max = 0.01f;

							// Vertically center text with plot height
							float text_offset = (row_height - ImGui::GetTextLineHeight()) * 0.5f;
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + text_offset);
							ImGui::Text("%s", sec.name); ImGui::NextColumn();
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + text_offset);
							ImGui::Text("%.3f", sec.accumulated_time); ImGui::NextColumn();
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() + text_offset);
							ImGui::Text("%d", sec.call_count); ImGui::NextColumn();
							ImGui::PushID(i);
							ImGui::PlotLines("##SectionGraph", sec.history, iam_detail::PROFILER_HISTORY_SIZE,
								sec.history_idx, nullptr, 0.0f, sec_max * 1.2f, ImVec2(-1, row_height));
							ImGui::PopID();
							ImGui::NextColumn();
						}
						ImGui::Columns(1);
					}
				}
			}
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

// ----------------------------------------------------
// Drag Feedback - animated feedback for drag operations
// ----------------------------------------------------

namespace iam_detail {
	struct drag_state {
		ImVec2 start_pos;
		ImVec2 current_pos;
		ImVec2 prev_pos;
		ImVec2 velocity;
		bool is_dragging;
		bool is_snapping;
		float snap_progress;
		ImVec2 snap_start;
		ImVec2 snap_target;
		float snap_duration;
		int snap_ease;
	};
	static ImGuiStorage g_drag_states;
}

// Internal ImVec2 version of iam_drag_feedback, converted on return
struct iam_drag_feedback_im {
	ImVec2 position, offset, velocity;
	bool is_dragging, is_snapping;
	float snap_progress;
};
static iam_drag_feedback iam_drag_feedback_pub(iam_drag_feedback_im const& f) {
	iam_drag_feedback fb;
	fb.position = IAM_VEC2_FROM_IM(f.position);
	fb.offset = IAM_VEC2_FROM_IM(f.offset);
	fb.velocity = IAM_VEC2_FROM_IM(f.velocity);
	fb.is_dragging = f.is_dragging;
	fb.is_snapping = f.is_snapping;
	fb.snap_progress = f.snap_progress;
	return fb;
}

static iam_drag_feedback iam_drag_begin_impl(ImGuiID id, ImVec2 pos) {
	iam_detail::drag_state* state = (iam_detail::drag_state*)iam_detail::g_drag_states.GetVoidPtr(id);
	if (!state) {
		state = IM_NEW(iam_detail::drag_state)();
		iam_detail::g_drag_states.SetVoidPtr(id, state);
	}

	state->start_pos = pos;
	state->current_pos = pos;
	state->prev_pos = pos;
	state->velocity = ImVec2(0, 0);
	state->is_dragging = true;
	state->is_snapping = false;
	state->snap_progress = 0.0f;

	iam_drag_feedback_im fb;
	fb.position = pos;
	fb.offset = ImVec2(0, 0);
	fb.velocity = ImVec2(0, 0);
	fb.is_dragging = true;
	fb.is_snapping = false;
	fb.snap_progress = 0.0f;
	return iam_drag_feedback_pub(fb);
}

static iam_drag_feedback iam_drag_update_impl(ImGuiID id, ImVec2 pos, float dt) {
	iam_detail::drag_state* state = (iam_detail::drag_state*)iam_detail::g_drag_states.GetVoidPtr(id);

	iam_drag_feedback_im fb;
	fb.position = pos;
	fb.offset = ImVec2(0, 0);
	fb.velocity = ImVec2(0, 0);
	fb.is_dragging = false;
	fb.is_snapping = false;
	fb.snap_progress = 0.0f;

	if (!state) return iam_drag_feedback_pub(fb);

	// Calculate velocity
	if (dt > 0.0f) {
		state->velocity.x = (pos.x - state->prev_pos.x) / dt;
		state->velocity.y = (pos.y - state->prev_pos.y) / dt;
	}
	state->prev_pos = state->current_pos;
	state->current_pos = pos;

	fb.position = pos;
	fb.offset = ImVec2(pos.x - state->start_pos.x, pos.y - state->start_pos.y);
	fb.velocity = state->velocity;
	fb.is_dragging = state->is_dragging;
	fb.is_snapping = state->is_snapping;
	fb.snap_progress = state->snap_progress;
	return iam_drag_feedback_pub(fb);
}

static iam_drag_feedback iam_drag_release_impl(ImGuiID id, ImVec2 pos, iam_drag_opts const& opts, float dt) {
	iam_detail::drag_state* state = (iam_detail::drag_state*)iam_detail::g_drag_states.GetVoidPtr(id);

	iam_drag_feedback_im fb;
	fb.position = pos;
	fb.offset = ImVec2(0, 0);
	fb.velocity = ImVec2(0, 0);
	fb.is_dragging = false;
	fb.is_snapping = false;
	fb.snap_progress = 1.0f;

	if (!state) return iam_drag_feedback_pub(fb);

	state->is_dragging = false;
	ImVec2 snap_grid = IAM_VEC2_TO_IM(opts.snap_grid);

	// Check for snap targets
	ImVec2 snap_target = pos;
	bool should_snap = false;

	// Grid snapping
	if (snap_grid.x > 0 || snap_grid.y > 0) {
		if (snap_grid.x > 0) snap_target.x = ImFloor(pos.x / snap_grid.x + 0.5f) * snap_grid.x;
		if (snap_grid.y > 0) snap_target.y = ImFloor(pos.y / snap_grid.y + 0.5f) * snap_grid.y;
		should_snap = true;
	}

	// Point snapping (find closest)
	if (opts.snap_points && opts.snap_points_count > 0) {
		float min_dist = FLT_MAX;
		for (int i = 0; i < opts.snap_points_count; i++) {
			float dx = IAM_VEC2_TO_IM(opts.snap_points[i]).x - pos.x;
			float dy = IAM_VEC2_TO_IM(opts.snap_points[i]).y - pos.y;
			float dist = dx * dx + dy * dy;
			if (dist < min_dist) {
				min_dist = dist;
				snap_target = IAM_VEC2_TO_IM(opts.snap_points[i]);
			}
		}
		should_snap = true;
	}

	if (should_snap && opts.snap_duration > 0.0f) {
		state->is_snapping = true;
		state->snap_progress = 0.0f;
		state->snap_start = pos;
		state->snap_target = snap_target;
		state->snap_duration = opts.snap_duration;
		state->snap_ease = opts.ease_type;
	}

	// Update snapping animation
	if (state->is_snapping) {
		state->snap_progress += dt / state->snap_duration;
		if (state->snap_progress >= 1.0f) {
			state->snap_progress = 1.0f;
			state->is_snapping = false;
		}
		float t = iam_detail::eval_preset_internal(state->snap_ease, state->snap_progress);
		fb.position.x = state->snap_start.x + (state->snap_target.x - state->snap_start.x) * t;
		fb.position.y = state->snap_start.y + (state->snap_target.y - state->snap_start.y) * t;
	} else {
		fb.position = should_snap ? snap_target : pos;
	}

	fb.offset = ImVec2(fb.position.x - state->start_pos.x, fb.position.y - state->start_pos.y);
	fb.velocity = state->velocity;
	fb.is_dragging = false;
	fb.is_snapping = state->is_snapping;
	fb.snap_progress = state->snap_progress;
	return iam_drag_feedback_pub(fb);
}

void iam_drag_cancel(ImGuiID id) {
	iam_detail::drag_state* state = (iam_detail::drag_state*)iam_detail::g_drag_states.GetVoidPtr(id);
	if (state) {
		state->is_dragging = false;
		state->is_snapping = false;
	}
}

// ============================================================
// DEBUG TIMELINE VISUALIZATION
// ============================================================

void iam_show_debug_timeline(ImGuiID instance_id) {
	using namespace iam_clip_detail;

	// Brand colors
	ImU32 const C1 = ZimaBlue;
	ImU32 const C2 = AgedCopper;
	ImU32 const C1_dim = IM_COL32(91, 194, 231, 80);
	ImU32 const C2_dim = IM_COL32(204, 120, 88, 80);
	ImU32 const C1_highlight = IM_COL32(120, 210, 240, 255);
	ImU32 const C2_highlight = IM_COL32(230, 140, 110, 255);
	ImU32 const BG_color = IM_COL32(30, 32, 40, 255);
	ImU32 const BG_track = IM_COL32(40, 44, 55, 255);
	ImU32 const grid_color = IM_COL32(60, 65, 80, 255);
	ImU32 const text_color = IM_COL32(180, 185, 195, 255);
	ImU32 const playhead_color = IM_COL32(255, 255, 255, 220);

	// Find instance
	iam_instance_data* inst = find_instance(instance_id);
	if (!inst) {
		ImGui::TextDisabled("Instance 0x%08X not found", instance_id);
		return;
	}

	// Find clip
	iam_clip_data* clip = find_clip(inst->clip_id);
	if (!clip) {
		ImGui::TextDisabled("Clip 0x%08X not found", inst->clip_id);
		return;
	}

	// Layout constants
	float const track_height = 20.0f;
	float const track_spacing = 2.0f;
	float const header_height = 22.0f;
	float const time_ruler_height = 36.0f;
	float const margin = 4.0f;
	float const label_width = ImGui::CalcTextSize("float").x + 8.0f;

	// Calculate widget dimensions
	ImVec2 avail = ImGui::GetContentRegionAvail();
	int num_tracks = clip->iam_tracks.Size;
	if (num_tracks == 0) num_tracks = 1;  // At least show something
	float const progress_bar_height = 24.0f;  // Space for progress bar at bottom
	float total_height = header_height + time_ruler_height + (track_height + track_spacing) * num_tracks + progress_bar_height + margin * 2;
	float timeline_width = avail.x - label_width - margin * 2;
	if (timeline_width < 100.0f) timeline_width = 100.0f;

	// Duration, delay, and current time
	float clip_delay = clip->delay;
	float duration = clip->duration;
	if (duration <= 0.0f) duration = 1.0f;
	float total_duration = clip_delay + duration;  // Total timeline length including delay
	float current_time = inst->time;
	float delay_left = inst->delay_left;

	// Get draw list and cursor position
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 cp = ImGui::GetCursorScreenPos();

	// Reserve space
	ImGui::Dummy(ImVec2(avail.x, total_height));

	// Background
	dl->AddRectFilled(cp, ImVec2(cp.x + avail.x, cp.y + total_height), BG_color, 4.0f);

	// Header - instance info
	{
		char header_text[128];
		if (delay_left > 0) {
			snprintf(header_text, sizeof(header_text), "Clip 0x%08X | Delay: %.2fs | %s",
				inst->clip_id, delay_left,
				inst->paused ? "PAUSED" : (inst->playing ? "WAITING" : "STOPPED"));
		} else {
			snprintf(header_text, sizeof(header_text), "Clip 0x%08X | %.2fs / %.2fs | %s",
				inst->clip_id, current_time, duration,
				inst->paused ? "PAUSED" : (inst->playing ? "PLAYING" : "STOPPED"));
		}

		ImVec2 text_pos = ImVec2(cp.x + margin, cp.y + margin);
		dl->AddText(text_pos, inst->playing ? C1 : text_color, header_text);
	}

	// Timeline area origin (after header, tracks start immediately)
	float timeline_y = cp.y + header_height * 2.0f;
	float timeline_x = cp.x + label_width + margin;

	// Draw tracks
	for (int i = 0; i < clip->iam_tracks.Size; i++) {
		iam_track& track = clip->iam_tracks[i];
		float track_y = timeline_y + (track_height + track_spacing) * i;

		// Track background
		dl->AddRectFilled(
			ImVec2(timeline_x, track_y),
			ImVec2(timeline_x + timeline_width, track_y + track_height),
			BG_track, 2.0f
		);

		// Track label
		{
			char const* type_names[] = { "float", "vec2", "vec4", "int", "color", "float_rel", "vec2_rel", "vec4_rel", "color_rel" };
			char const* type_name = (track.type >= 0 && track.type < 9) ? type_names[track.type] : "?";
			char label[64];
			snprintf(label, sizeof(label), "%s", type_name);
			ImVec2 label_pos = ImVec2(cp.x + margin, track_y + (track_height - ImGui::GetTextLineHeight()) / 2);
			dl->AddText(label_pos, text_color, label);
		}

		// Draw keyframe segments
		bool use_coral = (i % 2 == 1);  // Alternate colors
		ImU32 segment_color = use_coral ? C2_dim : C1_dim;
		ImU32 segment_active = use_coral ? C2 : C1;
		ImU32 segment_highlight = use_coral ? C2_highlight : C1_highlight;
		ImU32 delay_color = IM_COL32(50, 55, 70, 255);  // Dim color for delay zone

		// Draw delay zone if clip has delay
		if (clip_delay > 0) {
			float delay_end_x = timeline_x + (clip_delay / total_duration) * timeline_width;
			float seg_y1 = track_y + 2;
			float seg_y2 = track_y + track_height - 2;
			dl->AddRectFilled(ImVec2(timeline_x, seg_y1), ImVec2(delay_end_x, seg_y2), delay_color, 2.0f);
			// Draw dashed pattern to indicate waiting period
			for (float dx = timeline_x + 4; dx < delay_end_x - 4; dx += 12) {
				dl->AddLine(ImVec2(dx, seg_y1 + 2), ImVec2(dx + 6, seg_y2 - 2), IM_COL32(70, 75, 90, 255), 1.0f);
			}
		}

		for (int k = 0; k < track.keys.Size; k++) {
			keyframe& key = track.keys[k];
			float key_time = key.time;

			// Find next keyframe time (or end of clip)
			float next_time = duration;
			if (k + 1 < track.keys.Size) {
				next_time = track.keys[k + 1].time;
			}

			// Segment positions (offset by clip_delay)
			float x1 = timeline_x + ((clip_delay + key_time) / total_duration) * timeline_width;
			float x2 = timeline_x + ((clip_delay + next_time) / total_duration) * timeline_width;
			float seg_y1 = track_y + 2;
			float seg_y2 = track_y + track_height - 2;

			// Check if current time is within this segment
			bool is_active = (current_time >= key_time && current_time < next_time) && delay_left <= 0;

			// Draw segment
			ImU32 seg_col = is_active ? segment_active : segment_color;
			if (is_active) {
				// Glow effect for active segment
				dl->AddRectFilled(
					ImVec2(x1 - 1, seg_y1 - 1),
					ImVec2(x2 + 1, seg_y2 + 1),
					segment_highlight, 3.0f
				);
			}
			dl->AddRectFilled(ImVec2(x1, seg_y1), ImVec2(x2, seg_y2), seg_col, 2.0f);

			// Draw keyframe marker (diamond/dot at start)
			float marker_size = 8.0f;
			ImVec2 marker_center = ImVec2(x1, track_y + track_height / 2);
			dl->AddCircleFilled(marker_center, marker_size, is_active ? playhead_color : segment_active);

			// Hover detection - keyframe has priority over segment
			ImVec2 mouse = ImGui::GetMousePos();
			float hover_radius = 8.0f;  // Larger hover region for keyframe
			float dx = mouse.x - marker_center.x;
			float dy = mouse.y - marker_center.y;
			bool keyframe_hovered = (dx * dx + dy * dy <= hover_radius * hover_radius);

			if (keyframe_hovered) {
				// Highlight hovered keyframe
				dl->AddCircle(marker_center, marker_size + 3.0f, playhead_color, 0, 2.0f);

				ImGui::BeginTooltip();
				ImGui::Text("Time: %.3fs", key.time);

				// Display value based on type with color preview
				switch (key.type) {
					case iam_chan_float:
						ImGui::Text("Value: %.4f", key.value[0]);
						// Show as grayscale color
						{
							float v = ImClamp(key.value[0], 0.0f, 1.0f);
							ImVec4 col(v, v, v, 1.0f);
							ImGui::ColorButton("##val", col, ImGuiColorEditFlags_NoTooltip, ImVec2(16, 16));
						}
						break;
					case iam_chan_vec2:
						ImGui::Text("Value: (%.3f, %.3f)", key.value[0], key.value[1]);
						// Show XY as RG color
						{
							float r = ImClamp(key.value[0], 0.0f, 1.0f);
							float g = ImClamp(key.value[1], 0.0f, 1.0f);
							ImVec4 col(r, g, 0.5f, 1.0f);
							ImGui::ColorButton("##val", col, ImGuiColorEditFlags_NoTooltip, ImVec2(16, 16));
						}
						break;
					case iam_chan_vec4:
						ImGui::Text("Value: (%.3f, %.3f, %.3f, %.3f)", key.value[0], key.value[1], key.value[2], key.value[3]);
						{
							ImVec4 col(key.value[0], key.value[1], key.value[2], key.value[3]);
							ImGui::ColorButton("##val", col, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview, ImVec2(16, 16));
						}
						break;
					case iam_chan_int:
						ImGui::Text("Value: %d", *(int*)&key.value[0]);
						break;
					case iam_chan_color:
						ImGui::Text("Color: (%.3f, %.3f, %.3f, %.3f)", key.value[0], key.value[1], key.value[2], key.value[3]);
						{
							ImVec4 col(key.value[0], key.value[1], key.value[2], key.value[3]);
							ImGui::ColorButton("##val", col, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreview, ImVec2(16, 16));
							char const* space_names[] = { "sRGB", "Linear", "HSV", "OKLAB", "OKLCH" };
							if (key.color_space >= 0 && key.color_space < 5) {
								ImGui::SameLine();
								ImGui::Text("(%s)", space_names[key.color_space]);
							}
						}
						break;
					default:
						ImGui::Text("Value: %.4f", key.value[0]);
						break;
				}

				// Easing info
				char const* ease_names[] = {
					"linear", "in_quad", "out_quad", "in_out_quad",
					"in_cubic", "out_cubic", "in_out_cubic",
					"in_quart", "out_quart", "in_out_quart",
					"in_quint", "out_quint", "in_out_quint",
					"in_sine", "out_sine", "in_out_sine",
					"in_expo", "out_expo", "in_out_expo",
					"in_circ", "out_circ", "in_out_circ",
					"in_back", "out_back", "in_out_back",
					"in_elastic", "out_elastic", "in_out_elastic",
					"in_bounce", "out_bounce", "in_out_bounce",
					"steps", "cubic_bezier", "spring", "custom"
				};
				if (key.ease_type >= 0 && key.ease_type < IM_ARRAYSIZE(ease_names))
					ImGui::Text("Ease: %s", ease_names[key.ease_type]);

				ImGui::EndTooltip();
			}
			else if (mouse.x >= x1 && mouse.x <= x2 && mouse.y >= seg_y1 && mouse.y <= seg_y2) {
				// Segment hover tooltip (for easing between keyframes)
				// Highlight hovered segment
#if IMGUI_VERSION_NUM < 19276
				dl->AddRect(ImVec2(x1, seg_y1), ImVec2(x2, seg_y2), playhead_color, 2.0f, 0, 2.0f);
#else
				dl->AddRect(ImVec2(x1, seg_y1), ImVec2(x2, seg_y2), playhead_color, 2.0f, 2.0f);
#endif

				ImGui::BeginTooltip();
				ImGui::Text("Segment: %.2fs - %.2fs", key_time, next_time);

				// Show easing applied from this keyframe to next
				char const* ease_names[] = {
					"linear", "in_quad", "out_quad", "in_out_quad",
					"in_cubic", "out_cubic", "in_out_cubic",
					"in_quart", "out_quart", "in_out_quart",
					"in_quint", "out_quint", "in_out_quint",
					"in_sine", "out_sine", "in_out_sine",
					"in_expo", "out_expo", "in_out_expo",
					"in_circ", "out_circ", "in_out_circ",
					"in_back", "out_back", "in_out_back",
					"in_elastic", "out_elastic", "in_out_elastic",
					"in_bounce", "out_bounce", "in_out_bounce",
					"steps", "cubic_bezier", "spring", "custom"
				};
				if (key.ease_type >= 0 && key.ease_type < IM_ARRAYSIZE(ease_names))
					ImGui::Text("Easing: %s", ease_names[key.ease_type]);
				else
					ImGui::Text("Easing: linear");

				if (key.has_bezier)
					ImGui::Text("Bezier: (%.2f, %.2f, %.2f, %.2f)", key.bezier[0], key.bezier[1], key.bezier[2], key.bezier[3]);

				if (key.is_spring)
					ImGui::Text("Spring: m=%.1f k=%.1f c=%.1f", key.spring.mass, key.spring.stiffness, key.spring.damping);

				ImGui::EndTooltip();
			}
		}

		// Grid lines (every 0.5s or so)
		float grid_step = 0.5f;
		if (duration > 10.0f) grid_step = 2.0f;
		else if (duration > 5.0f) grid_step = 1.0f;
		for (float t = grid_step; t < duration; t += grid_step) {
			float x = timeline_x + (t / duration) * timeline_width;
			dl->AddLine(ImVec2(x, track_y), ImVec2(x, track_y + track_height), grid_color, 0.5f);
		}
	}

	// If no tracks, show placeholder
	if (clip->iam_tracks.Size == 0) {
		float track_y = timeline_y;
		dl->AddRectFilled(
			ImVec2(timeline_x, track_y),
			ImVec2(timeline_x + timeline_width, track_y + track_height),
			BG_track, 2.0f
		);
		ImVec2 text_pos = ImVec2(timeline_x + 10, track_y + (track_height - ImGui::GetTextLineHeight()) / 2);
		dl->AddText(text_pos, text_color, "No tracks");
	}

	// Calculate tracks bottom position
	float tracks_bottom = timeline_y + (track_height + track_spacing) * ImMax(1, clip->iam_tracks.Size);

	// Time ruler (below tracks)
	float ruler_y = tracks_bottom + 2;
	{
		// Ruler background
		dl->AddRectFilled(
			ImVec2(timeline_x, ruler_y),
			ImVec2(timeline_x + timeline_width, ruler_y + time_ruler_height),
			BG_track
		);

		// Time markers (use total_duration to include delay)
		float time_step = 0.5f;  // Default step
		if (total_duration > 10.0f) time_step = 2.0f;
		else if (total_duration > 5.0f) time_step = 1.0f;
		else if (total_duration < 1.0f) time_step = 0.1f;

		for (float t = 0.0f; t <= total_duration + 0.001f; t += time_step) {
			float x = timeline_x + (t / total_duration) * timeline_width;
			dl->AddLine(ImVec2(x, ruler_y), ImVec2(x, ruler_y + 6), grid_color, 1.0f);

			char time_str[16];
			snprintf(time_str, sizeof(time_str), "%.1fs", t);
			ImVec2 text_size = ImGui::CalcTextSize(time_str);
			if (x + text_size.x / 2 < timeline_x + timeline_width) {
				dl->AddText(ImVec2(x - text_size.x / 2, ruler_y + 6), text_color, time_str);
			}
		}

		// Draw delay/animation boundary marker if delay exists
		if (clip_delay > 0) {
			float delay_x = timeline_x + (clip_delay / total_duration) * timeline_width;
			dl->AddLine(ImVec2(delay_x, ruler_y), ImVec2(delay_x, ruler_y + time_ruler_height), C1_dim, 2.0f);
			ImVec2 text_pos = ImVec2(delay_x + 2, ruler_y + 2);
			dl->AddText(text_pos, C1_dim, "Delay End");
		}
	}

	// Draw playhead (account for delay)
	{
		float effective_time;
		if (delay_left > 0) {
			// During delay: position within the delay zone
			effective_time = clip_delay - delay_left;
		} else {
			// After delay: position within the animation zone
			effective_time = clip_delay + current_time;
		}

		if (effective_time >= 0.0f && effective_time <= total_duration) {
			float playhead_x = timeline_x + (effective_time / total_duration) * timeline_width;
			dl->AddLine(ImVec2(playhead_x, timeline_y), ImVec2(playhead_x, ruler_y + time_ruler_height), playhead_color, 2.0f);

			// Playhead triangle at top
			ImVec2 tri[3] = {
				ImVec2(playhead_x, timeline_y),
				ImVec2(playhead_x - 5, timeline_y - 8),
				ImVec2(playhead_x + 5, timeline_y - 8)
			};
			dl->AddTriangleFilled(tri[0], tri[1], tri[2], playhead_color);
		}
	}
}

// ============================================================
// NAMED IDS
// ============================================================

namespace iam_names_detail {
	static ImGuiStorage g_names;  // id -> char* (ImStrdup'd, never moves)
}

void iam_set_id_name(ImGuiID id, char const* name) {
	using namespace iam_names_detail;
	if (!name) return;
	char* cur = (char*)g_names.GetVoidPtr(id);
	if (cur && strcmp(cur, name) == 0) return;
	if (cur) IM_FREE(cur);
	g_names.SetVoidPtr(id, ImStrdup(name));
}

char const* iam_get_id_name(ImGuiID id) {
	return (char const*)iam_names_detail::g_names.GetVoidPtr(id);
}

ImGuiID iam_id(char const* name) {
	ImGuiID id = ImHashStr(name);
	if (!iam_names_detail::g_names.GetVoidPtr(id)) iam_set_id_name(id, name);
	return id;
}

// ============================================================
// CLIP EDITOR
// ============================================================

namespace iam_clip_editor_detail {
	using namespace iam_clip_detail;

	static char const* const k_ease_names[] = {
		"linear",
		"in_quad", "out_quad", "in_out_quad",
		"in_cubic", "out_cubic", "in_out_cubic",
		"in_quart", "out_quart", "in_out_quart",
		"in_quint", "out_quint", "in_out_quint",
		"in_sine", "out_sine", "in_out_sine",
		"in_expo", "out_expo", "in_out_expo",
		"in_circ", "out_circ", "in_out_circ",
		"in_back", "out_back", "in_out_back",
		"in_elastic", "out_elastic", "in_out_elastic",
		"in_bounce", "out_bounce", "in_out_bounce",
		"steps", "cubic_bezier", "spring", "custom", "curve"
	};
	static int const k_ease_count = (int)(sizeof(k_ease_names) / sizeof(k_ease_names[0]));
	static char const* const k_type_names[] = { "float", "vec2", "vec4", "int", "color", "float_rel", "vec2_rel", "vec4_rel", "color_rel" };
	static char const* const k_type_calls[] = { "key_float", "key_vec2", "key_vec4", "key_int", "key_color", "key_float_rel", "key_vec2_rel", "key_vec4_rel", "key_color_rel" };
	static char const* const k_col_space_enums[] = { "iam_col_srgb", "iam_col_srgb_linear", "iam_col_hsv", "iam_col_oklab", "iam_col_oklch" };
	static char const* const k_anchor_enums[] = { "iam_anchor_window_content", "iam_anchor_window", "iam_anchor_viewport", "iam_anchor_last_item" };
	static char const* const k_dir_names[] = { "normal", "reverse", "alternate" };

	static char const* ease_name(int e) { return (e >= 0 && e < k_ease_count) ? k_ease_names[e] : "?"; }
	static char const* type_name(int t) { return (t >= 0 && t < 9) ? k_type_names[t] : "?"; }

	// Undo snapshot of a clip's tracks
	struct track_header {
		ImGuiID channel;
		int type, color_space, anchor_space, anchor_axis, key_count;
		bool is_relative;
	};
	struct snapshot {
		ImVector<track_header> tracks;
		ImVector<keyframe> keys;
		float duration, delay;
		int loop_count, direction;
	};

	struct editor_state {
		ImGuiID clip_id;
		ImVector<unsigned> selected;        // keyframe uids
		float view_t0, view_len;            // visible time range
		bool view_fitted;
		bool snap;
		float snap_step;
		// Key drag
		bool drag_keys, drag_moved;
		float drag_origin_t;
		ImVector<float> drag_orig_times;    // parallel to selected
		// Box select
		bool box_active;
		ImVec2 box_start;
		// Undo
		ImVector<snapshot*> undo, redo;
		// UI
		char path[256];
		char add_name[64];
		int add_type;
		char status[160];
		double status_until;
		bool modified;
		bool ease_editing;                  // an ease editor drag is in progress (one undo step per drag)

		editor_state() : clip_id(0), view_t0(0), view_len(1), view_fitted(false), snap(true), snap_step(0.05f),
			drag_keys(false), drag_moved(false), drag_origin_t(0), box_active(false), box_start(0, 0),
			add_type(0), status_until(0), modified(false), ease_editing(false) {
			path[0] = add_name[0] = status[0] = 0;
		}
		~editor_state() {
			for (int i = 0; i < undo.Size; i++) IM_DELETE(undo[i]);
			for (int i = 0; i < redo.Size; i++) IM_DELETE(redo[i]);
		}
		bool is_selected(unsigned uid) const { return selected.contains(uid); }
		void select(unsigned uid) { if (!selected.contains(uid)) selected.push_back(uid); }
		void deselect(unsigned uid) { selected.find_erase(uid); }
	};

	static ImGuiStorage g_states;  // clip_id -> editor_state*

	static editor_state* get_state(ImGuiID clip_id) {
		editor_state* s = (editor_state*)g_states.GetVoidPtr(clip_id);
		if (!s) {
			s = IM_NEW(editor_state)();
			s->clip_id = clip_id;
			char const* name = iam_get_id_name(clip_id);
			if (name) snprintf(s->path, sizeof(s->path), "%s.iamclip", name);
			else snprintf(s->path, sizeof(s->path), "clip_%08X.iamclip", clip_id);
			g_states.SetVoidPtr(clip_id, s);
		}
		return s;
	}

	static void free_states() {
		for (int i = 0; i < g_states.Data.Size; i++) {
			editor_state* s = (editor_state*)g_states.Data[i].val_p;
			if (s) IM_DELETE(s);
		}
		g_states.Clear();
	}

	static void set_status(editor_state* st, char const* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		vsnprintf(st->status, sizeof(st->status), fmt, args);
		va_end(args);
		st->status_until = ImGui::GetTime() + 4.0;
	}

	static void free_track_keys(iam_track& trk) { trk.keys.clear(); }

	static snapshot* take_snapshot(iam_clip_data* clip) {
		snapshot* s = IM_NEW(snapshot)();
		s->duration = clip->duration;
		s->delay = clip->delay;
		s->loop_count = clip->loop_count;
		s->direction = clip->direction;
		for (int t = 0; t < clip->iam_tracks.Size; t++) {
			iam_track& trk = clip->iam_tracks[t];
			track_header h;
			h.channel = trk.channel; h.type = trk.type; h.color_space = trk.color_space;
			h.anchor_space = trk.anchor_space; h.anchor_axis = trk.anchor_axis;
			h.key_count = trk.keys.Size; h.is_relative = trk.is_relative;
			s->tracks.push_back(h);
			for (int k = 0; k < trk.keys.Size; k++) s->keys.push_back(trk.keys[k]);
		}
		return s;
	}

	static void restore_snapshot(iam_clip_data* clip, snapshot const* s) {
		for (int t = 0; t < clip->iam_tracks.Size; t++) free_track_keys(clip->iam_tracks[t]);
		clip->iam_tracks.clear();
		int key_i = 0;
		for (int t = 0; t < s->tracks.Size; t++) {
			track_header const& h = s->tracks[t];
			clip->iam_tracks.push_back(iam_track());
			iam_track& trk = clip->iam_tracks.back();
			trk.channel = h.channel; trk.type = h.type; trk.color_space = h.color_space;
			trk.anchor_space = h.anchor_space; trk.anchor_axis = h.anchor_axis; trk.is_relative = h.is_relative;
			for (int k = 0; k < h.key_count; k++) trk.keys.push_back(s->keys[key_i++]);
		}
		clip->duration = s->duration;
		clip->delay = s->delay;
		clip->loop_count = s->loop_count;
		clip->direction = s->direction;
	}

	static void push_undo(editor_state* st, iam_clip_data* clip) {
		st->undo.push_back(take_snapshot(clip));
		if (st->undo.Size > 100) { IM_DELETE(st->undo[0]); st->undo.erase(st->undo.Data); }
		for (int i = 0; i < st->redo.Size; i++) IM_DELETE(st->redo[i]);
		st->redo.clear();
		st->modified = true;
	}

	// Sort keys (stable) and recompute the clip duration after an edit
	static void recompute(iam_clip_data* clip) {
		float dur = 0.0f;
		for (int t = 0; t < clip->iam_tracks.Size; t++) {
			ImVector<keyframe>& keys = clip->iam_tracks[t].keys;
			for (int i = 1; i < keys.Size; i++) {
				keyframe k = keys[i];
				int j = i - 1;
				while (j >= 0 && keys[j].time > k.time) { keys[j + 1] = keys[j]; j--; }
				keys[j + 1] = k;
			}
			if (keys.Size > 0 && keys.back().time > dur) dur = keys.back().time;
		}
		for (int m = 0; m < clip->markers.Size; m++) if (clip->markers[m].time > dur) dur = clip->markers[m].time;
		clip->duration = dur;
	}

	static void refresh_preview(ImGuiID preview_id) {
		iam_instance inst(preview_id);
		if (inst.valid()) inst.refresh();
	}

	static void after_edit(editor_state* st, iam_clip_data* clip, ImGuiID preview_id) {
		recompute(clip);
		refresh_preview(preview_id);
		st->modified = true;
	}

	static keyframe* find_key(iam_clip_data* clip, unsigned uid, int* out_track = nullptr) {
		for (int t = 0; t < clip->iam_tracks.Size; t++) {
			iam_track& trk = clip->iam_tracks[t];
			for (int k = 0; k < trk.keys.Size; k++) {
				if (trk.keys[k].uid == uid) {
					if (out_track) *out_track = t;
					return &trk.keys[k];
				}
			}
		}
		return nullptr;
	}

	static float key_ease_weight(keyframe const& k0, int track_type, float u) {
		if (k0.is_spring && track_type == iam_chan_float) return eval_clip_spring(u, k0.spring);
		return eval_clip_ease(k0.ease_type, u, k0.bezier, k0.has_bezier);
	}

	// Interpolate raw key data at time t (used to insert a key without changing the curve)
	static void sample_track(iam_track const& trk, float t, keyframe* out) {
		if (trk.keys.Size == 0) return;
		keyframe const* k0; keyframe const* k1;
		if (!find_keys(trk, t, &k0, &k1) || !k0 || !k1) return;
		unsigned uid = out->uid;
		*out = *k0;
		out->uid = uid;
		out->time = t;
		out->has_variation = false;
		if (k0 == k1) return;
		float u = (k1->time == k0->time) ? 1.0f : (t - k0->time) / (k1->time - k0->time);
		float w = key_ease_weight(*k0, trk.type, u);
		if (trk.type == iam_chan_int) {
			out->set_int((int)ImFloor(k0->get_int() + (k1->get_int() - k0->get_int()) * w + 0.5f));
			return;
		}
		for (int i = 0; i < 4; i++) {
			out->value[i] = k0->value[i] + (k1->value[i] - k0->value[i]) * w;
			out->value_ext[i] = k0->value_ext[i] + (k1->value_ext[i] - k0->value_ext[i]) * w;
		}
	}

	static float snap_time(editor_state const* st, float t) {
		if (st->snap && st->snap_step > 1e-5f && !ImGui::GetIO().KeyAlt) t = ImFloor(t / st->snap_step + 0.5f) * st->snap_step;
		return t < 0.0f ? 0.0f : t;
	}

	static float nice_step(float min_step) {
		static float const steps[] = { 0.001f, 0.002f, 0.005f, 0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.25f, 0.5f, 1, 2, 5, 10, 15, 30, 60, 120, 300, 600 };
		for (int i = 0; i < (int)(sizeof(steps) / sizeof(steps[0])); i++) if (steps[i] >= min_step) return steps[i];
		return 600.0f;
	}

	static void track_label(iam_track const& trk, char* buf, size_t buf_size) {
		char const* name = iam_get_id_name(trk.channel);
		if (name) snprintf(buf, buf_size, "%s", name);
		else snprintf(buf, buf_size, "0x%08X", trk.channel);
	}

	// Current value of a track from the preview instance, as text
	static void preview_value_text(ImGuiID preview_id, iam_track const& trk, char* buf, size_t buf_size) {
		buf[0] = 0;
		iam_instance inst(preview_id);
		if (!inst.valid()) return;
		switch (trk.type) {
			case iam_chan_float: case iam_chan_float_rel: { float f; if (inst.get_float(trk.channel, &f)) snprintf(buf, buf_size, "%.3f", f); break; }
			case iam_chan_int: { int i; if (inst.get_int(trk.channel, &i)) snprintf(buf, buf_size, "%d", i); break; }
			case iam_chan_vec2: case iam_chan_vec2_rel: {
				iam_vec2 v = iam_v2(0, 0);
				if (inst.get_vec2(trk.channel, &v)) { ImVec2 x = IAM_VEC2_TO_IM(v); snprintf(buf, buf_size, "%.1f, %.1f", x.x, x.y); }
				break;
			}
			case iam_chan_vec4: case iam_chan_vec4_rel: {
				iam_vec4 v = iam_v4(0, 0, 0, 0);
				if (inst.get_vec4(trk.channel, &v)) { ImVec4 x = IAM_VEC4_TO_IM(v); snprintf(buf, buf_size, "%.2f %.2f %.2f %.2f", x.x, x.y, x.z, x.w); }
				break;
			}
			default: break;  // colors are shown as a swatch
		}
	}

	static bool preview_color(ImGuiID preview_id, iam_track const& trk, ImVec4* out) {
		iam_instance inst(preview_id);
		if (!inst.valid() || (trk.type != iam_chan_color && trk.type != iam_chan_color_rel)) return false;
		iam_vec4 v = iam_v4(0, 0, 0, 0);
		if (!inst.get_color(trk.channel, &v, trk.color_space)) return false;
		*out = IAM_VEC4_TO_IM(v);
		return true;
	}

	// Draw an ease curve u in [0,1] -> weight inside rect (used in segments and the inspector)
	static void draw_ease_curve(ImDrawList* dl, ImVec2 p0, ImVec2 p1, keyframe const& k0, int track_type, ImU32 col, float thickness) {
		int const N = 32;
		ImVec2 pts[N + 1];
		for (int i = 0; i <= N; i++) {
			float u = (float)i / N;
			float w = key_ease_weight(k0, track_type, u);
			pts[i] = ImVec2(p0.x + (p1.x - p0.x) * u, p1.y - (p1.y - p0.y) * w);
		}
		dl->AddPolyline(pts, N + 1, col, 0, thickness);
	}

	// ---- C++ export ----
	static void append_id(ImGuiTextBuffer& b, ImGuiID id) {
		char const* name = iam_get_id_name(id);
		if (name) b.appendf("iam_id(\"%s\")", name);
		else b.appendf("0x%08Xu", id);
	}
	// Float as a valid C++ literal ("1.0f", "0.25f"). Rotating buffers: up to 8 per expression.
	static char const* fl(float v) {
		static char bufs[8][40];
		static int n = 0;
		char* buf = bufs[n++ & 7];
		snprintf(buf, 32, "%g", v);
		if (!strpbrk(buf, ".eEn")) strcat(buf, ".0");  // n: inf/nan
		strcat(buf, "f");
		return buf;
	}
	static void append_floats(ImGuiTextBuffer& b, char const* ctor, float const* v, int n) {
		if (n == 1) { b.append(fl(v[0])); return; }
		b.appendf("%s(", ctor);
		for (int i = 0; i < n; i++) b.appendf(i ? ", %s" : "%s", fl(v[i]));
		b.append(")");
	}

	// Key needs an explicit params argument (bezier4) in exported code
	static bool export_params(keyframe const& kf) {
		if (kf.is_spring || kf.ease_type <= iam_ease_in_out_bounce || kf.ease_type == iam_ease_curve) return false;
		return kf.has_bezier || kf.bezier[0] != 0.0f || kf.bezier[1] != 0.0f || kf.bezier[2] != 0.0f || kf.bezier[3] != 0.0f;
	}

	static void export_cpp(iam_clip_data* clip, ImGuiTextBuffer& b) {
		int bez_count = 0;
		ImVector<ImGuiID> curves;
		for (int t = 0; t < clip->iam_tracks.Size; t++)
			for (int k = 0; k < clip->iam_tracks[t].keys.Size; k++) {
				keyframe const& kf = clip->iam_tracks[t].keys[k];
				if (export_params(kf)) {
					b.appendf("static float const bezier_%d[4] = { %s, %s, %s, %s };\n", bez_count++, fl(kf.bezier[0]), fl(kf.bezier[1]), fl(kf.bezier[2]), fl(kf.bezier[3]));
				}
				if (kf.ease_type == iam_ease_curve && !kf.is_spring) {
					ImGuiID cid = iam_detail::curve_id_from(kf.bezier[0], kf.bezier[1]);
					iam_detail::ease_curve* c = iam_detail::find_curve(cid);
					if (c && !curves.contains(cid)) {
						b.appendf("static iam_vec2 const curve_%d[] = {", curves.Size);
						for (int i = 0; i < c->pts.Size; i++) b.appendf("%s iam_v2(%s, %s)", i ? "," : "", fl(c->pts[i].x), fl(c->pts[i].y));
						b.append(" };\n");
						b.append("iam_ease_curve_set(");
						append_id(b, cid);
						b.appendf(", curve_%d, %d);\n", curves.Size, c->pts.Size);
						curves.push_back(cid);
					}
				}
			}
		b.append("iam_clip::begin(");
		append_id(b, clip->id);
		b.append(")\n");
		bez_count = 0;
		for (int t = 0; t < clip->iam_tracks.Size; t++) {
			iam_track const& trk = clip->iam_tracks[t];
			for (int k = 0; k < trk.keys.Size; k++) {
				keyframe const& kf = trk.keys[k];
				char bez_arg[96] = "";
				if (export_params(kf)) snprintf(bez_arg, sizeof(bez_arg), ", bezier_%d", bez_count++);
				else if (kf.ease_type == iam_ease_curve && !kf.is_spring) {
					ImGuiID cid = iam_detail::curve_id_from(kf.bezier[0], kf.bezier[1]);
					char const* cname = iam_get_id_name(cid);
					if (cname) snprintf(bez_arg, sizeof(bez_arg), ", iam_ease_curve_ref(iam_id(\"%s\"))", cname);
					else snprintf(bez_arg, sizeof(bez_arg), ", iam_ease_curve_ref(0x%08Xu)", cid);
				}
				if (kf.is_spring && trk.type == iam_chan_float) {
					b.append("\t.key_float_spring(");
					append_id(b, trk.channel);
					b.appendf(", %s, %s, iam_spring_params{ %s, %s, %s, %s })\n", fl(kf.time), fl(kf.value[0]),
						fl(kf.spring.mass), fl(kf.spring.stiffness), fl(kf.spring.damping), fl(kf.spring.initial_velocity));
					continue;
				}
				b.appendf("\t.%s(", k_type_calls[trk.type < 9 ? trk.type : 0]);
				append_id(b, trk.channel);
				b.appendf(", %s, ", fl(kf.time));
				char const* ease = ease_name(kf.ease_type);
				switch (trk.type) {
					case iam_chan_float: append_floats(b, "", kf.value, 1); b.appendf(", iam_ease_%s%s", ease, bez_arg); break;
					case iam_chan_vec2: append_floats(b, "iam_v2", kf.value, 2); b.appendf(", iam_ease_%s%s", ease, bez_arg); break;
					case iam_chan_vec4: append_floats(b, "iam_v4", kf.value, 4); b.appendf(", iam_ease_%s%s", ease, bez_arg); break;
					case iam_chan_int: b.appendf("%d, iam_ease_%s", kf.get_int(), ease); break;
					case iam_chan_color: {
						int cs = kf.color_space;
						append_floats(b, "iam_v4", kf.value, 4);
						b.appendf(", %s, iam_ease_%s%s", (cs >= 0 && cs < 5) ? k_col_space_enums[cs] : "iam_col_oklab", ease, bez_arg);
						break;
					}
					case iam_chan_float_rel: {
						int anchor = trk.anchor_space, axis = trk.anchor_axis;
						b.appendf("%s, %s, %s, %d, iam_ease_%s%s", fl(kf.value[0]), fl(kf.value[1]), (anchor >= 0 && anchor < 4) ? k_anchor_enums[anchor] : "0", axis, ease, bez_arg);
						break;
					}
					case iam_chan_vec2_rel:
						append_floats(b, "iam_v2", kf.value, 2); b.append(", ");
						append_floats(b, "iam_v2", kf.value + 2, 2);
						b.appendf(", %s, iam_ease_%s%s", (trk.anchor_space >= 0 && trk.anchor_space < 4) ? k_anchor_enums[trk.anchor_space] : "0", ease, bez_arg);
						break;
					case iam_chan_vec4_rel:
					case iam_chan_color_rel:
						append_floats(b, "iam_v4", kf.value, 4); b.append(", ");
						append_floats(b, "iam_v4", kf.value_ext, 4);
						if (trk.type == iam_chan_color_rel) b.appendf(", %s", (trk.color_space >= 0 && trk.color_space < 5) ? k_col_space_enums[trk.color_space] : "iam_col_oklab");
						b.appendf(", %s, iam_ease_%s%s", (trk.anchor_space >= 0 && trk.anchor_space < 4) ? k_anchor_enums[trk.anchor_space] : "0", ease, bez_arg);
						break;
					default: break;
				}
				b.append(")");
				if (kf.has_variation) b.append("  // note: per-loop variation not exported");
				b.append("\n");
			}
		}
		for (int l = 0; l < clip->labels.Size; l++) {
			b.append("\t.label(");
			append_id(b, clip->labels[l].id);
			b.appendf(", %s)\n", fl(clip->labels[l].time));
		}
		if (clip->delay > 0.0f) b.appendf("\t.set_delay(%s)\n", fl(clip->delay));
		if (clip->loop_count != 0) {
			static char const* const dirs[] = { "iam_dir_normal", "iam_dir_reverse", "iam_dir_alternate" };
			b.appendf("\t.set_loop(true, %s, %d)\n", (clip->direction >= 0 && clip->direction < 3) ? dirs[clip->direction] : "iam_dir_normal", clip->loop_count);
		}
		if (clip->loop_delay > 0.0f) b.appendf("\t.set_loop_delay(%s)\n", fl(clip->loop_delay));
		b.append("\t.end();\n");
	}

	// Value widgets for one key; returns true when edited. *activated is set when an edit starts (for undo).
	static bool edit_key_value(keyframe& kf, iam_track const& trk, bool* activated) {
		bool changed = false;
		ImGui::PushID("value");
		switch (trk.type) {
			case iam_chan_float: changed = ImGui::DragFloat("Value", &kf.value[0], 0.01f); break;
			case iam_chan_vec2: changed = ImGui::DragFloat2("Value", kf.value, 0.5f); break;
			case iam_chan_vec4: changed = ImGui::DragFloat4("Value", kf.value, 0.01f); break;
			case iam_chan_int: { int v = kf.get_int(); if (ImGui::DragInt("Value", &v)) { kf.set_int(v); changed = true; } break; }
			case iam_chan_color: changed = ImGui::ColorEdit4("Value", kf.value, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar); break;
			case iam_chan_float_rel:
				changed = ImGui::DragFloat("Percent", &kf.value[0], 0.005f);
				*activated |= ImGui::IsItemActivated();
				changed |= ImGui::DragFloat("Px bias", &kf.value[1], 0.5f);
				break;
			case iam_chan_vec2_rel:
				changed = ImGui::DragFloat2("Percent", kf.value, 0.005f);
				*activated |= ImGui::IsItemActivated();
				changed |= ImGui::DragFloat2("Px bias", kf.value + 2, 0.5f);
				break;
			case iam_chan_vec4_rel:
			case iam_chan_color_rel:
				changed = ImGui::DragFloat4("Percent", kf.value, 0.005f);
				*activated |= ImGui::IsItemActivated();
				changed |= ImGui::DragFloat4("Px bias", kf.value_ext, 0.5f);
				break;
			default: break;
		}
		*activated |= ImGui::IsItemActivated();
		ImGui::PopID();
		return changed;
	}

	static bool ease_combo(char const* label, int* ease) {
		bool changed = false;
		if (ImGui::BeginCombo(label, ease_name(*ease))) {
			for (int e = 0; e < k_ease_count; e++) {
				if (e == iam_ease_spring || e == iam_ease_custom || e == iam_ease_curve) continue;  // spring: Spring checkbox; custom: slot; curve: ease editor
				bool sel = (e == *ease);
				if (ImGui::Selectable(k_ease_names[e], sel)) { *ease = e; changed = true; }
				if (sel) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		return changed;
	}

	static void delete_selected(editor_state* st, iam_clip_data* clip) {
		if (st->selected.Size == 0) return;
		push_undo(st, clip);
		for (int t = 0; t < clip->iam_tracks.Size; t++) {
			ImVector<keyframe>& keys = clip->iam_tracks[t].keys;
			for (int k = keys.Size - 1; k >= 0; k--)
				if (st->is_selected(keys[k].uid)) keys.erase(keys.Data + k);
		}
		st->selected.clear();
	}

	// ---- Main editor body ----
	static bool editor_body(ImGuiID clip_id, ImGuiID preview_instance_id) {
		ImGuiIO& io = ImGui::GetIO();
		iam_clip_data* clip = find_clip(clip_id);
		if (!clip) {
			ImGui::TextDisabled("Clip 0x%08X not found", clip_id);
			return false;
		}
		editor_state* st = get_state(clip_id);
		st->modified = false;
		ImGuiID const preview_id = preview_instance_id ? preview_instance_id : ImHashData(&clip_id, sizeof(clip_id), ImHashStr("iam_clip_editor_preview"));
		iam_instance preview(preview_id);
		if (preview.valid()) {
			iam_instance_data* pd = find_instance(preview_id);
			if (pd && pd->clip_id != clip_id) preview = iam_instance();  // instance plays another clip: don't drive it
		}
		bool const focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		ImGui::PushID((int)clip_id);

		// ---------------- Toolbar ----------------
		{
			bool playing = preview.valid() && preview.is_playing() && !preview.is_paused();
			if (ImGui::Button(playing ? "Pause" : "Play")) {
				if (!preview.valid() || !preview.is_playing()) {
					if (!preview_instance_id || !preview.valid()) preview = iam_play(clip_id, preview_id);
					else preview.restart();
				} else if (preview.is_paused()) preview.resume();
				else preview.pause();
			}
			ImGui::SameLine();
			if (ImGui::Button("Restart")) {
				if (preview.valid()) preview.restart();
				else preview = iam_play(clip_id, preview_id);
			}
			ImGui::SameLine();
			ImGui::Text("%.2f / %.2fs", preview.valid() ? preview.time() : 0.0f, clip->duration);
			ImGui::SameLine();
			ImGui::TextDisabled("|");
			ImGui::SameLine();
			bool loop = clip->loop_count != 0;
			if (ImGui::Checkbox("Loop", &loop)) { push_undo(st, clip); clip->loop_count = loop ? -1 : 0; }
			if (loop) {
				ImGui::SameLine();
				ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0f);
				int dir = clip->direction;
				if (ImGui::Combo("##dir", &dir, k_dir_names, 3)) { push_undo(st, clip); clip->direction = dir; }
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
			float delay = clip->delay;
			if (ImGui::DragFloat("Delay", &delay, 0.01f, 0.0f, 60.0f, "%.2fs")) { clip->delay = delay < 0 ? 0 : delay; st->modified = true; }
			if (ImGui::IsItemActivated()) push_undo(st, clip);
			ImGui::SameLine();
			ImGui::TextDisabled("|");
			ImGui::SameLine();
			ImGui::Checkbox("Snap", &st->snap);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 4.0f);
			ImGui::DragFloat("##snapstep", &st->snap_step, 0.005f, 0.001f, 10.0f, "%.3fs");
			ImGui::SameLine();
			if (ImGui::Button("Fit")) st->view_fitted = false;

			ImGui::BeginDisabled(st->undo.Size == 0);
			if (ImGui::Button("Undo")) { st->redo.push_back(take_snapshot(clip)); restore_snapshot(clip, st->undo.back()); IM_DELETE(st->undo.back()); st->undo.pop_back(); st->selected.clear(); refresh_preview(preview_id); st->modified = true; }
			ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::BeginDisabled(st->redo.Size == 0);
			if (ImGui::Button("Redo")) { st->undo.push_back(take_snapshot(clip)); restore_snapshot(clip, st->redo.back()); IM_DELETE(st->redo.back()); st->redo.pop_back(); st->selected.clear(); refresh_preview(preview_id); st->modified = true; }
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Add track")) ImGui::OpenPopup("##add_track");
			if (ImGui::BeginPopup("##add_track")) {
				ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
				ImGui::InputText("Channel name", st->add_name, sizeof(st->add_name));
				ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
				ImGui::Combo("Type", &st->add_type, k_type_names, 5);
				if (ImGui::Button("Add") && st->add_name[0]) {
					ImGuiID ch = iam_id(st->add_name);
					bool exists = false;
					for (int t = 0; t < clip->iam_tracks.Size; t++) exists |= (clip->iam_tracks[t].channel == ch && clip->iam_tracks[t].type == st->add_type);
					if (exists) set_status(st, "Track '%s' (%s) already exists", st->add_name, type_name(st->add_type));
					else {
						push_undo(st, clip);
						clip->iam_tracks.push_back(iam_track());
						iam_track& trk = clip->iam_tracks.back();
						trk.channel = ch;
						trk.type = st->add_type;
						keyframe kf;
						kf.channel = ch;
						kf.type = trk.type;
						kf.time = preview.valid() ? snap_time(st, preview.time()) : 0.0f;
						if (trk.type == iam_chan_color) kf.set_color(ImVec4(1, 1, 1, 1));
						trk.keys.push_back(kf);
						st->selected.clear();
						st->select(kf.uid);
						after_edit(st, clip, preview_id);
						st->add_name[0] = 0;
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Copy C++")) {
				ImGuiTextBuffer b;
				export_cpp(clip, b);
				ImGui::SetClipboardText(b.c_str());
				int lines = 0;
				for (char const* c = b.c_str(); *c; c++) lines += (*c == '\n');
				set_status(st, "Copied %d lines of C++ to the clipboard", lines);
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
			ImGui::InputText("##path", st->path, sizeof(st->path));
			ImGui::SameLine();
			if (ImGui::Button("Save")) {
				iam_result r = iam_clip_save(clip_id, st->path);
				set_status(st, r == iam_ok ? "Saved %s" : "Could not save %s", st->path);
			}
			ImGui::SameLine();
			if (ImGui::Button("Load")) {
				snapshot* before = take_snapshot(clip);
				ImGuiID loaded = 0;
				iam_result r = iam_clip_load(st->path, &loaded);
				clip = find_clip(clip_id);  // loading may grow the clip array
				if (r != iam_ok) { set_status(st, "Could not load %s", st->path); IM_DELETE(before); }
				else if (loaded != clip_id) { set_status(st, "%s holds clip 0x%08X, not this one", st->path, loaded); IM_DELETE(before); }
				else {
					st->undo.push_back(before);
					for (int i = 0; i < st->redo.Size; i++) IM_DELETE(st->redo[i]);
					st->redo.clear();
					st->selected.clear();
					after_edit(st, clip, preview_id);
					set_status(st, "Loaded %s", st->path);
				}
			}
			if (st->status[0] && ImGui::GetTime() < st->status_until) {
				ImGui::SameLine();
				ImGui::TextDisabled("%s", st->status);
			}
		}

		if (!clip) { ImGui::PopID(); return st->modified; }

		// ---------------- Layout ----------------
		ImGuiStyle const& style = ImGui::GetStyle();
		float const row_h = ImGui::GetFrameHeight();
		float const ruler_h = row_h;
		float const inspector_h = ImGui::GetFrameHeightWithSpacing() * 6.0f;
		float label_w = ImGui::CalcTextSize("0x00000000  vec4_rel").x + ImGui::GetFontSize() * 6.0f;
		ImVec2 avail = ImGui::GetContentRegionAvail();
		float const tl_w = ImMax(avail.x - label_w - style.ScrollbarSize, 50.0f);
		float lanes_h = ImMax(avail.y - ruler_h - inspector_h - style.ItemSpacing.y * 2.0f, row_h * 3.0f);

		if (!st->view_fitted) {
			float d = clip->duration > 0.0f ? clip->duration : 1.0f;
			st->view_t0 = -d * 0.02f;
			st->view_len = d * 1.06f;
			st->view_fitted = true;
		}

		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImU32 const col_bg = IM_COL32(30, 32, 40, 255);
		ImU32 const col_lane = IM_COL32(40, 44, 55, 255);
		ImU32 const col_lane_alt = IM_COL32(44, 48, 60, 255);
		ImU32 const col_grid = IM_COL32(60, 65, 80, 255);
		ImU32 const col_text = IM_COL32(180, 185, 195, 255);
		ImU32 const col_playhead = IM_COL32(255, 255, 255, 220);
		ImU32 const col_sel = IM_COL32(255, 255, 255, 255);
		ImU32 const col_box = IM_COL32(91, 194, 231, 60);
		ImU32 const col_box_border = IM_COL32(91, 194, 231, 200);

		float const play_t = preview.valid() ? preview.time() : 0.0f;

		// ---------------- Ruler ----------------
		ImVec2 ruler_p = ImGui::GetCursorScreenPos();
		float const tl_x = ruler_p.x + label_w;
		#define IAM_ED_T2X(t) (tl_x + ((t) - st->view_t0) / st->view_len * tl_w)
		#define IAM_ED_X2T(x) (st->view_t0 + ((x) - tl_x) / tl_w * st->view_len)
		{
			ImGui::InvisibleButton("##ruler_pad", ImVec2(label_w, ruler_h));
			ImGui::SameLine(0, 0);
			ImGui::InvisibleButton("##ruler", ImVec2(tl_w, ruler_h));
			bool ruler_active = ImGui::IsItemActive();
			if (ruler_active && preview.valid()) {
				if (ImGui::IsItemActivated() && preview.is_playing() && !preview.is_paused()) preview.pause();
				float t = IAM_ED_X2T(io.MousePos.x);
				t = ImClamp(st->snap && io.KeyShift ? snap_time(st, t) : t, 0.0f, clip->duration);
				preview.seek(t);
				preview.refresh();
			} else if (ruler_active && ImGui::IsItemActivated()) {
				preview = iam_play(clip_id, preview_id);
				preview.pause();
			}
			dl->AddRectFilled(ruler_p, ImVec2(tl_x + tl_w, ruler_p.y + ruler_h), col_bg);
			float step = nice_step(st->view_len * 70.0f / tl_w);
			float first = ImFloor(st->view_t0 / step) * step;
			dl->PushClipRect(ImVec2(tl_x, ruler_p.y), ImVec2(tl_x + tl_w, ruler_p.y + ruler_h), true);
			for (float t = first; t <= st->view_t0 + st->view_len; t += step) {
				float x = IAM_ED_T2X(t);
				dl->AddLine(ImVec2(x, ruler_p.y + ruler_h * 0.55f), ImVec2(x, ruler_p.y + ruler_h), col_grid);
				float hx = IAM_ED_T2X(t + step * 0.5f);
				dl->AddLine(ImVec2(hx, ruler_p.y + ruler_h * 0.8f), ImVec2(hx, ruler_p.y + ruler_h), col_grid);
				char lbl[32];
				snprintf(lbl, sizeof(lbl), step < 0.1f ? "%.2f" : (step < 1.0f ? "%.1f" : "%.0f"), t);
				dl->AddText(ImVec2(x + 3, ruler_p.y + 1), col_text, lbl);
			}
			// Labels
			for (int l = 0; l < clip->labels.Size; l++) {
				float lx = IAM_ED_T2X(clip->labels[l].time);
				char const* ln = iam_get_id_name(clip->labels[l].id);
				char lbuf[24];
				if (!ln) { snprintf(lbuf, sizeof(lbuf), "0x%08X", clip->labels[l].id); ln = lbuf; }
				dl->AddTriangleFilled(ImVec2(lx, ruler_p.y + ruler_h * 0.55f), ImVec2(lx + 5, ruler_p.y + ruler_h * 0.35f), ImVec2(lx, ruler_p.y + ruler_h * 0.15f), IM_COL32(230, 190, 90, 255));
				dl->AddLine(ImVec2(lx, ruler_p.y + ruler_h * 0.15f), ImVec2(lx, ruler_p.y + ruler_h), IM_COL32(230, 190, 90, 200));
				ImVec2 lt(lx + 7, ruler_p.y + ruler_h * 0.5f - ImGui::GetFontSize() * 0.5f);
				ImVec2 ls = ImGui::CalcTextSize(ln);
				dl->AddRectFilled(ImVec2(lt.x - 2, lt.y), ImVec2(lt.x + ls.x + 2, lt.y + ls.y), col_bg, 2.0f);  // readable over tick numbers
				dl->AddText(lt, IM_COL32(230, 190, 90, 255), ln);
			}
			// Clip end + playhead handle
			float end_x = IAM_ED_T2X(clip->duration);
			dl->AddLine(ImVec2(end_x, ruler_p.y), ImVec2(end_x, ruler_p.y + ruler_h), AgedCopper, 2.0f);
			float px = IAM_ED_T2X(play_t);
			dl->AddTriangleFilled(ImVec2(px - 6, ruler_p.y + ruler_h - 10), ImVec2(px + 6, ruler_p.y + ruler_h - 10), ImVec2(px, ruler_p.y + ruler_h), col_playhead);
			dl->PopClipRect();
			char info[64];
			snprintf(info, sizeof(info), "%d tracks", clip->iam_tracks.Size);
			dl->AddText(ImVec2(ruler_p.x + style.FramePadding.x, ruler_p.y + style.FramePadding.y), col_text, info);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Click/drag to scrub (Shift: snap)");
			}
		}

		// ---------------- Lanes ----------------
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::BeginChild("##lanes", ImVec2(0, lanes_h), 0, ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar(2);
		{
			ImDrawList* ldl = ImGui::GetWindowDrawList();
			ImVec2 lp = ImGui::GetCursorScreenPos();
			float const content_h = ImMax(row_h * (float)clip->iam_tracks.Size, lanes_h);
			ldl->AddRectFilled(lp, ImVec2(lp.x + label_w + tl_w, lp.y + content_h), col_bg);

			// Labels (one button per track for its context menu)
			int delete_track = -1;
			for (int t = 0; t < clip->iam_tracks.Size; t++) {
				iam_track& trk = clip->iam_tracks[t];
				ImGui::SetCursorScreenPos(ImVec2(lp.x, lp.y + row_h * t));
				ImGui::PushID(t);
				ImGui::InvisibleButton("##label", ImVec2(label_w, row_h));
				if (ImGui::BeginPopupContextItem("##track_ctx")) {
					char lbl[64];
					track_label(trk, lbl, sizeof(lbl));
					ImGui::TextDisabled("%s (%s)", lbl, type_name(trk.type));
					if (ImGui::MenuItem("Add key at playhead")) {
						push_undo(st, clip);
						keyframe kf;
						sample_track(trk, play_t, &kf);
						kf.channel = trk.channel; kf.type = trk.type; kf.time = snap_time(st, play_t);
						trk.keys.push_back(kf);
						st->selected.clear(); st->select(kf.uid);
						after_edit(st, clip, preview_id);
					}
					if (ImGui::MenuItem("Select all keys")) {
						if (!io.KeyCtrl) st->selected.clear();
						for (int k = 0; k < trk.keys.Size; k++) st->select(trk.keys[k].uid);
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Delete track")) delete_track = t;
					ImGui::EndPopup();
				}
				ImU32 lane_col = (t & 1) ? col_lane_alt : col_lane;
				float y = lp.y + row_h * t;
				ldl->AddRectFilled(ImVec2(lp.x, y), ImVec2(lp.x + label_w - 2, y + row_h - 1), lane_col);
				char lbl[64];
				track_label(trk, lbl, sizeof(lbl));
				float ty = y + (row_h - ImGui::GetFontSize()) * 0.5f;
				ldl->AddText(ImVec2(lp.x + style.FramePadding.x, ty), col_text, lbl);
				char val[64];
				ImVec4 swatch;
				float right = lp.x + label_w - style.FramePadding.x - 2;
				if (preview_color(preview_id, trk, &swatch)) {
					float sz = row_h - 8;
					ldl->AddRectFilled(ImVec2(right - sz, y + 4), ImVec2(right, y + 4 + sz), ImGui::ColorConvertFloat4ToU32(swatch), 2.0f);
					right -= sz + 4;
				} else {
					preview_value_text(preview_id, trk, val, sizeof(val));
					if (val[0]) {
						float vw = ImGui::CalcTextSize(val).x;
						ldl->AddText(ImVec2(right - vw, ty), ZimaBlue, val);
						right -= vw + 6;
					}
				}
				char const* tn = type_name(trk.type);
				float tw = ImGui::CalcTextSize(tn).x;
				if (right - tw > lp.x + style.FramePadding.x + ImGui::CalcTextSize(lbl).x + 6)
					ldl->AddText(ImVec2(right - tw, ty), IM_COL32(120, 125, 140, 255), tn);
				ImGui::PopID();
			}
			if (delete_track >= 0) {
				push_undo(st, clip);
				free_track_keys(clip->iam_tracks[delete_track]);
				clip->iam_tracks.erase(clip->iam_tracks.Data + delete_track);
				st->selected.clear();
				after_edit(st, clip, preview_id);
			}

			// Timeline canvas: one button for all mouse input
			ImGui::SetCursorScreenPos(ImVec2(tl_x, lp.y));
			ImGui::InvisibleButton("##canvas", ImVec2(tl_w, content_h), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
			bool const canvas_hovered = ImGui::IsItemHovered();
			bool const canvas_active = ImGui::IsItemActive();
			bool const canvas_activated = ImGui::IsItemActivated();
#if IMGUI_VERSION_NUM >= 18950
			if (canvas_hovered) {
				ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
				ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);
			}
#endif
			ImVec2 mouse = io.MousePos;
			float const mouse_t = IAM_ED_X2T(mouse.x);
			int const mouse_row = (int)ImFloor((mouse.y - lp.y) / row_h);

			// Zoom / pan
			if (canvas_hovered || ImGui::IsWindowHovered()) {
				if (io.MouseWheel != 0.0f && io.KeyCtrl) {
					float f = ImPow(1.2f, -io.MouseWheel);
					float nl = ImClamp(st->view_len * f, 0.02f, 3600.0f);
					st->view_t0 = mouse_t - (mouse_t - st->view_t0) * (nl / st->view_len);
					st->view_len = nl;
				} else if (io.MouseWheel != 0.0f && io.KeyShift) {
					st->view_t0 -= io.MouseWheel * st->view_len * 0.1f;
				} else if (io.MouseWheelH != 0.0f) {
					st->view_t0 -= io.MouseWheelH * st->view_len * 0.1f;
				} else if (io.MouseWheel != 0.0f) {
					ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseWheel * row_h * 2.0f);
				}
			}
			if (canvas_active && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
				st->view_t0 -= io.MouseDelta.x / tl_w * st->view_len;
			}

			// Hit test keys
			unsigned hovered_uid = 0;
			int hovered_track = -1;
			float const key_r = row_h * 0.32f;
			if (canvas_hovered && mouse_row >= 0 && mouse_row < clip->iam_tracks.Size) {
				iam_track& trk = clip->iam_tracks[mouse_row];
				float best = key_r + 3.0f;
				for (int k = 0; k < trk.keys.Size; k++) {
					float d = ImFabs(IAM_ED_T2X(trk.keys[k].time) - mouse.x);
					if (d <= best) { best = d; hovered_uid = trk.keys[k].uid; hovered_track = mouse_row; }
				}
			}

			// Mouse: left button
			if (canvas_activated && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				bool additive = io.KeyCtrl || io.KeyShift;
				if (hovered_uid) {
					if (additive && st->is_selected(hovered_uid)) st->deselect(hovered_uid);
					else {
						if (!additive && !st->is_selected(hovered_uid)) st->selected.clear();
						st->select(hovered_uid);
					}
					st->drag_keys = st->is_selected(hovered_uid);
					st->drag_moved = false;
					st->drag_origin_t = mouse_t;
					st->drag_orig_times.resize(st->selected.Size);
					for (int i = 0; i < st->selected.Size; i++) {
						keyframe* kf = find_key(clip, st->selected[i]);
						st->drag_orig_times[i] = kf ? kf->time : 0.0f;
					}
				} else {
					if (!additive) st->selected.clear();
					st->box_active = true;
					st->box_start = mouse;
				}
			}
			if (st->drag_keys) {
				if (canvas_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f)) {
					if (!st->drag_moved) { push_undo(st, clip); st->drag_moved = true; }
					float dt = mouse_t - st->drag_origin_t;
					for (int i = 0; i < st->selected.Size && i < st->drag_orig_times.Size; i++) {
						keyframe* kf = find_key(clip, st->selected[i]);
						if (kf) kf->time = snap_time(st, st->drag_orig_times[i] + dt);
					}
					after_edit(st, clip, preview_id);
					ImGui::SetTooltip("%.3fs", find_key(clip, st->selected.Size ? st->selected[0] : 0) ? find_key(clip, st->selected[0])->time : 0.0f);
				}
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) st->drag_keys = false;
			}
			if (st->box_active) {
				ImVec2 a = st->box_start, b = mouse;
				ImVec2 bmin(ImMin(a.x, b.x), ImMin(a.y, b.y)), bmax(ImMax(a.x, b.x), ImMax(a.y, b.y));
				bool dragged = ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f) || (bmax.x - bmin.x) > 3.0f || (bmax.y - bmin.y) > 3.0f;
				if (dragged) {
					ldl->AddRectFilled(bmin, bmax, col_box);
					ldl->AddRect(bmin, bmax, col_box_border);
				}
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
					if (dragged) {
						for (int t = 0; t < clip->iam_tracks.Size; t++) {
							float cy = lp.y + row_h * (t + 0.5f);
							if (cy < bmin.y || cy > bmax.y) continue;
							iam_track& trk = clip->iam_tracks[t];
							for (int k = 0; k < trk.keys.Size; k++) {
								float x = IAM_ED_T2X(trk.keys[k].time);
								if (x >= bmin.x && x <= bmax.x) st->select(trk.keys[k].uid);
							}
						}
					} else if (preview.valid() || iam_clip_exists(clip_id)) {
						// Plain click on empty space: move the playhead
						if (!preview.valid()) { preview = iam_play(clip_id, preview_id); }
						if (preview.is_playing() && !preview.is_paused()) preview.pause();
						preview.seek(ImClamp(mouse_t, 0.0f, clip->duration));
						preview.refresh();
					}
					st->box_active = false;
				}
			}
			// Double-click on a lane: add a key there (keeps the current curve)
			if (canvas_hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !hovered_uid && mouse_row >= 0 && mouse_row < clip->iam_tracks.Size) {
				push_undo(st, clip);
				iam_track& trk = clip->iam_tracks[mouse_row];
				float t = snap_time(st, mouse_t);
				keyframe kf;
				sample_track(trk, t, &kf);
				kf.channel = trk.channel; kf.type = trk.type; kf.time = t;
				trk.keys.push_back(kf);
				st->selected.clear(); st->select(kf.uid);
				st->box_active = false;
				after_edit(st, clip, preview_id);
			}
			// Right-click: key context menu
			if (canvas_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				if (hovered_uid && !st->is_selected(hovered_uid)) { st->selected.clear(); st->select(hovered_uid); }
				if (st->selected.Size > 0) ImGui::OpenPopup("##key_ctx");
			}
			if (ImGui::BeginPopup("##key_ctx")) {
				ImGui::TextDisabled("%d key%s", st->selected.Size, st->selected.Size == 1 ? "" : "s");
				if (ImGui::BeginMenu("Ease")) {
					for (int e = 0; e < k_ease_count; e++) {
						if (e == iam_ease_spring || e == iam_ease_custom || e == iam_ease_cubic_bezier || e == iam_ease_curve) continue;
						if (ImGui::MenuItem(k_ease_names[e])) {
							push_undo(st, clip);
							for (int i = 0; i < st->selected.Size; i++) {
								keyframe* kf = find_key(clip, st->selected[i]);
								if (kf) { kf->ease_type = e; kf->has_bezier = false; kf->is_spring = false; }
							}
							after_edit(st, clip, preview_id);
						}
					}
					ImGui::EndMenu();
				}
				if (ImGui::MenuItem("Delete", "Del")) { delete_selected(st, clip); after_edit(st, clip, preview_id); }
				ImGui::EndPopup();
			}

			// Draw lanes, segments, keys
			ldl->PushClipRect(ImVec2(tl_x, lp.y), ImVec2(tl_x + tl_w, lp.y + content_h), true);
			float step = nice_step(st->view_len * 70.0f / tl_w);
			for (float t = ImFloor(st->view_t0 / step) * step; t <= st->view_t0 + st->view_len; t += step) {
				float x = IAM_ED_T2X(t);
				ldl->AddLine(ImVec2(x, lp.y), ImVec2(x, lp.y + content_h), IM_COL32(50, 54, 66, 255));
			}
			for (int t = 0; t < clip->iam_tracks.Size; t++) {
				iam_track& trk = clip->iam_tracks[t];
				float y = lp.y + row_h * t;
				ldl->AddRectFilled(ImVec2(tl_x, y), ImVec2(tl_x + tl_w, y + row_h - 1), (t & 1) ? col_lane_alt : col_lane);
				ImU32 c_main = (t & 1) ? AgedCopper : ZimaBlue;
				ImU32 c_dim = (c_main & 0x00FFFFFF) | (70u << 24);
				ImU32 c_curve = (c_main & 0x00FFFFFF) | (200u << 24);
				for (int k = 0; k + 1 < trk.keys.Size; k++) {
					float x0 = IAM_ED_T2X(trk.keys[k].time), x1 = IAM_ED_T2X(trk.keys[k + 1].time);
					if (x1 - x0 < 1.0f) continue;
					ImVec2 a(x0, y + 3), b(x1, y + row_h - 4);
					ldl->AddRectFilled(a, b, c_dim, 2.0f);
					if (x1 - x0 > 12.0f) draw_ease_curve(ldl, ImVec2(a.x, a.y + 1), ImVec2(b.x, b.y - 1), trk.keys[k], trk.type, c_curve, 1.5f);
				}
				for (int k = 0; k < trk.keys.Size; k++) {
					keyframe const& kf = trk.keys[k];
					ImVec2 c(IAM_ED_T2X(kf.time), y + row_h * 0.5f);
					bool sel = st->is_selected(kf.uid);
					bool hov = (kf.uid == hovered_uid);
					float r = key_r * (hov ? 1.2f : 1.0f);
					ImU32 fill = sel ? col_sel : c_main;
					ldl->AddQuadFilled(ImVec2(c.x, c.y - r), ImVec2(c.x + r, c.y), ImVec2(c.x, c.y + r), ImVec2(c.x - r, c.y), fill);
					if (sel || hov) ldl->AddQuad(ImVec2(c.x, c.y - r - 2), ImVec2(c.x + r + 2, c.y), ImVec2(c.x, c.y + r + 2), ImVec2(c.x - r - 2, c.y), c_main, 1.5f);
					if (kf.is_spring) ldl->AddCircle(c, r * 0.45f, IM_COL32(30, 32, 40, 255), 0, 1.5f);
				}
			}
			float end_x = IAM_ED_T2X(clip->duration);
			ldl->AddLine(ImVec2(end_x, lp.y), ImVec2(end_x, lp.y + content_h), (AgedCopper & 0x00FFFFFF) | (140u << 24), 1.5f);
			float px = IAM_ED_T2X(play_t);
			ldl->AddLine(ImVec2(px, lp.y), ImVec2(px, lp.y + content_h), col_playhead, 1.5f);
			ldl->PopClipRect();

			if (hovered_uid && !st->drag_keys && !ImGui::IsPopupOpen("##key_ctx")) {
				keyframe* kf = find_key(clip, hovered_uid);
				if (kf) ImGui::SetTooltip("%.3fs  %s%s", kf->time, kf->is_spring ? "spring" : ease_name(kf->ease_type), kf->has_variation ? "  (variation)" : "");
			}
			if (clip->iam_tracks.Size == 0) {
				ldl->AddText(ImVec2(tl_x + 8, lp.y + 6), col_text, "No tracks. Use \"Add track\".");
			}
			ImGui::SetCursorScreenPos(ImVec2(lp.x, lp.y + content_h));
		}
		ImGui::EndChild();
		#undef IAM_ED_T2X
		#undef IAM_ED_X2T

		// ---------------- Shortcuts ----------------
		if (focused && !ImGui::IsAnyItemActive() && !io.WantTextInput) {
			if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
				if (st->selected.Size) { delete_selected(st, clip); after_edit(st, clip, preview_id); }
			}
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
				st->selected.clear();
				for (int t = 0; t < clip->iam_tracks.Size; t++)
					for (int k = 0; k < clip->iam_tracks[t].keys.Size; k++) st->select(clip->iam_tracks[t].keys[k].uid);
			}
			bool undo_key = io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z);
			bool redo_key =io.KeyCtrl && ((io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)) || ImGui::IsKeyPressed(ImGuiKey_Y));
			if (undo_key && st->undo.Size) { st->redo.push_back(take_snapshot(clip)); restore_snapshot(clip, st->undo.back()); IM_DELETE(st->undo.back()); st->undo.pop_back(); st->selected.clear(); refresh_preview(preview_id); st->modified = true; }
			if (redo_key && st->redo.Size) { st->undo.push_back(take_snapshot(clip)); restore_snapshot(clip, st->redo.back()); IM_DELETE(st->redo.back()); st->redo.pop_back(); st->selected.clear(); refresh_preview(preview_id); st->modified = true; }
			if (ImGui::IsKeyPressed(ImGuiKey_Space) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
				if (!preview.valid() || !preview.is_playing()) preview = iam_play(clip_id, preview_id);
				else if (preview.is_paused()) preview.resume();
				else preview.pause();
			}
		}

		// ---------------- Inspector ----------------
		ImGui::Separator();
		ImGui::BeginChild("##inspector", ImVec2(0, 0), 0, 0);
		{
			// Drop selected uids that no longer exist (undo, delete, reload)
			for (int i = st->selected.Size - 1; i >= 0; i--) if (!find_key(clip, st->selected[i])) st->selected.erase(st->selected.Data + i);

			if (st->selected.Size == 0) {
				ImGui::TextDisabled("Select a key to edit it. Double-click a lane to add one, drag keys to retime them.");
			} else if (st->selected.Size == 1) {
				int ti = -1;
				keyframe* kf = find_key(clip, st->selected[0], &ti);
				iam_track& trk = clip->iam_tracks[ti];
				bool activated = false, changed = false;
				char lbl[64];
				track_label(trk, lbl, sizeof(lbl));

				float const curve_w = ImGui::GetFontSize() * 16.0f;
				keyframe const before = *kf;
				ImGui::BeginGroup();
				ImGui::Text("%s", lbl);
				ImGui::SameLine();
				ImGui::TextDisabled("(%s)  key %d/%d", type_name(trk.type), (int)(kf - trk.keys.Data) + 1, trk.keys.Size);
				ImGui::PushItemWidth(ImMax(ImGui::GetContentRegionAvail().x - curve_w - ImGui::GetFontSize() * 6.0f, ImGui::GetFontSize() * 8.0f));
				float time = kf->time;
				if (ImGui::DragFloat("Time", &time, 0.005f, 0.0f, 3600.0f, "%.3fs")) { kf->time = time < 0 ? 0 : time; changed = true; }
				activated |= ImGui::IsItemActivated();
				changed |= edit_key_value(*kf, trk, &activated);
				if (trk.type == iam_chan_color) {
					int cs = kf->color_space;
					static char const* const cs_names[] = { "sRGB", "sRGB linear", "HSV", "OKLAB", "OKLCH" };
					if (ImGui::Combo("Blend space", &cs, cs_names, 5)) { activated = true; kf->color_space = cs; changed = true; }
				}
				if (trk.type == iam_chan_float) {
					bool spring = kf->is_spring;
					if (ImGui::Checkbox("Spring", &spring)) { activated = true; kf->is_spring = spring; changed = true; }
					if (kf->is_spring) {
						float sp[4] = { kf->spring.mass, kf->spring.stiffness, kf->spring.damping, kf->spring.initial_velocity };
						if (ImGui::DragFloat4("Mass/Stiff/Damp/V0", sp, 0.1f)) {
							kf->spring.mass = ImMax(sp[0], 0.001f); kf->spring.stiffness = ImMax(sp[1], 0.001f);
							kf->spring.damping = ImMax(sp[2], 0.0f); kf->spring.initial_velocity = sp[3];
							changed = true;
						}
						activated |= ImGui::IsItemActivated();
					}
				}
				if (kf->has_variation) ImGui::TextDisabled("This key has per-loop variation (kept, not editable here).");
				ImGui::PopItemWidth();
				ImGui::EndGroup();

				// Ease of the segment starting at this key: editable graph (spring keys: preview only)
				ImGui::SameLine();
				ImVec2 cp = ImGui::GetCursorScreenPos();
				float ch = ImMin(ImGui::GetContentRegionAvail().y, curve_w * 0.6f);
				if (!kf->is_spring && ch > ImGui::GetFrameHeightWithSpacing() * 3.0f) {
					iam_ease_desc d = { kf->ease_type, kf->bezier[0], kf->bezier[1], kf->bezier[2], kf->bezier[3] };
					if (kf->ease_type <= iam_ease_in_out_bounce) d.p0 = d.p1 = d.p2 = d.p3 = 0.0f;
					bool ease_changed = iam_ease_editor("Ease", &d, iam_v2(curve_w, ch - ImGui::GetFrameHeightWithSpacing() * 2.0f));
					if (ease_changed) {
						if (!st->ease_editing) activated = true;
						st->ease_editing = true;
						kf->ease_type = d.type;
						kf->bezier[0] = d.p0; kf->bezier[1] = d.p1; kf->bezier[2] = d.p2; kf->bezier[3] = d.p3;
						kf->has_bezier = (d.type == iam_ease_cubic_bezier);
						changed = true;
					}
					if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) st->ease_editing = false;
				} else if (ch > 20.0f) {
					ImDrawList* idl = ImGui::GetWindowDrawList();
					ImVec2 c1(cp.x + curve_w, cp.y + ch);
					idl->AddRectFilled(cp, c1, col_bg, 3.0f);
					float pad = 6.0f;
					ImVec2 a(cp.x + pad, cp.y + ch * 0.2f), b(c1.x - pad, c1.y - ch * 0.2f);
					idl->AddLine(ImVec2(a.x, b.y), ImVec2(b.x, b.y), col_grid);
					idl->AddLine(ImVec2(a.x, a.y), ImVec2(b.x, a.y), col_grid);
					idl->PushClipRect(cp, c1, true);
					draw_ease_curve(idl, a, b, *kf, trk.type, ZimaBlue, 2.0f);
					idl->PopClipRect();
					ImGui::Dummy(ImVec2(curve_w, ch));
				}

				if (activated) {
					keyframe const after = *kf;
					*kf = before;  // snapshot the state from before this frame's edit
					push_undo(st, clip);
					*kf = after;
				}
				if (changed) after_edit(st, clip, preview_id);
			} else {
				ImGui::Text("%d keys selected", st->selected.Size);
				int ease = -1;
				ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10.0f);
				if (ease_combo("Set ease", &ease) && ease >= 0) {
					push_undo(st, clip);
					for (int i = 0; i < st->selected.Size; i++) {
						keyframe* kf = find_key(clip, st->selected[i]);
						if (!kf) continue;
						kf->ease_type = ease;
						kf->is_spring = false;
						if (ease == iam_ease_cubic_bezier && !kf->has_bezier) { kf->has_bezier = true; kf->bezier[0] = 0.25f; kf->bezier[1] = 0.1f; kf->bezier[2] = 0.25f; kf->bezier[3] = 1.0f; }
						else if (ease != iam_ease_cubic_bezier) kf->has_bezier = false;
					}
					after_edit(st, clip, preview_id);
				}
				ImGui::SameLine();
				if (ImGui::Button("Delete")) { delete_selected(st, clip); after_edit(st, clip, preview_id); }
			}
		}
		ImGui::EndChild();

		ImGui::PopID();
		return st->modified;
	}
} // namespace iam_clip_editor_detail

static void iam_clip_editor_free_states() { iam_clip_editor_detail::free_states(); }

// ============================================================
// EASE EDITOR
// ============================================================
namespace iam_ease_editor_detail {
	using iam_clip_editor_detail::k_ease_names;
	using iam_clip_editor_detail::k_ease_count;

	// Default curve: a single ease-in-out segment
	static void default_curve(ImVector<ImVec2>& pts) {
		pts.resize(4);
		pts[0] = ImVec2(0, 0); pts[1] = ImVec2(0.42f, 0.0f); pts[2] = ImVec2(0.58f, 1.0f); pts[3] = ImVec2(1, 1);
	}

	static void set_curve(ImGuiID id, ImVector<ImVec2> const& pts) {
		ImVector<iam_vec2> conv;
		conv.resize(pts.Size);
		for (int i = 0; i < pts.Size; i++) conv[i] = IAM_VEC2_FROM_IM(pts[i]);
		iam_ease_curve_set(id, conv.Data, conv.Size);
	}

	static ImGuiID new_curve_id() {
		char name[32];
		for (int n = 1; ; n++) {
			snprintf(name, sizeof(name), "curve_%d", n);
			ImGuiID id = ImHashStr(name);
			if (!iam_ease_curve_exists(id)) { iam_set_id_name(id, name); return id; }
		}
	}

	// Split segment k of a bezier chain at parameter u (de Casteljau): the curve shape is unchanged
	static void split_segment(ImVector<ImVec2>& pts, int k, float u) {
		ImVec2 p0 = pts[k * 3], p1 = pts[k * 3 + 1], p2 = pts[k * 3 + 2], p3 = pts[k * 3 + 3];
		ImVec2 p01 = ImLerp(p0, p1, u), p12 = ImLerp(p1, p2, u), p23 = ImLerp(p2, p3, u);
		ImVec2 p012 = ImLerp(p01, p12, u), p123 = ImLerp(p12, p23, u), mid = ImLerp(p012, p123, u);
		pts[k * 3 + 1] = p01;
		pts[k * 3 + 2] = p012;
		ImVec2 ins[3] = { mid, p123, p23 };
		pts.insert(pts.Data + k * 3 + 3, ins[2]);
		pts.insert(pts.Data + k * 3 + 3, ins[1]);
		pts.insert(pts.Data + k * 3 + 3, ins[0]);
	}

	// Parameter u where segment k reaches x (bisection; x is monotonic per segment)
	static float solve_u(ImVector<ImVec2> const& pts, int k, float x) {
		float lo = 0.0f, hi = 1.0f;
		for (int i = 0; i < 30; i++) {
			float m = (lo + hi) * 0.5f;
			if (iam_detail::bezier_point(pts[k * 3], pts[k * 3 + 1], pts[k * 3 + 2], pts[k * 3 + 3], m).x < x) lo = m; else hi = m;
		}
		return (lo + hi) * 0.5f;
	}

	static bool editor(char const* label, iam_ease_desc* ez, ImVec2 size_in) {
		ImGuiIO& io = ImGui::GetIO();
		float const fs = ImGui::GetFontSize();
		bool changed = false;
		ImGui::PushID(label);
		ImGui::BeginGroup();

		// Type picker
		char const* cur_name = (ez->type >= 0 && ez->type < k_ease_count) ? k_ease_names[ez->type] : "?";
		float const w = size_in.x > 0.0f ? size_in.x : ImGui::GetContentRegionAvail().x;
		ImGui::SetNextItemWidth(ImMax(w - ImGui::CalcTextSize(label).x - ImGui::GetStyle().ItemInnerSpacing.x, fs * 6.0f));
		if (ImGui::BeginCombo(label, cur_name)) {
			for (int e = 0; e < k_ease_count; e++) {
				if (e == iam_ease_custom) continue;  // needs a registered slot
				if (ImGui::Selectable(k_ease_names[e], e == ez->type)) {
					if (e != ez->type) {
						iam_ease_desc d = { e, 0, 0, 0, 0 };
						if (e == iam_ease_cubic_bezier) { d.p0 = 0.25f; d.p1 = 0.1f; d.p2 = 0.25f; d.p3 = 1.0f; }
						if (e == iam_ease_steps) { d.p0 = 5.0f; d.p1 = 0.0f; }
						if (e == iam_ease_spring) { d.p0 = 1.0f; d.p1 = 120.0f; d.p2 = 20.0f; d.p3 = 0.0f; }
						if (e == iam_ease_curve) {
							ImGuiID cid = new_curve_id();
							ImVector<ImVec2> pts;
							default_curve(pts);
							set_curve(cid, pts);
							d = iam_ease_curve_desc(cid);
						}
						*ez = d;
						changed = true;
					}
				}
			}
			ImGui::EndCombo();
		}

		// Parameters
		ImGui::PushItemWidth(w);
		if (ez->type == iam_ease_steps) {
			int steps = (int)ez->p0, mode = (int)ez->p1;
			static char const* const modes[] = { "jump end", "jump start", "jump both" };
			if (ImGui::SliderInt("##steps", &steps, 1, 24, "%d steps")) { ez->p0 = (float)steps; changed = true; }
			if (ImGui::Combo("##stepmode", &mode, modes, 3)) { ez->p1 = (float)mode; changed = true; }
		} else if (ez->type == iam_ease_spring) {
			float sp[4] = { ez->p0, ez->p1, ez->p2, ez->p3 };
			if (ImGui::DragFloat4("##spring", sp, 0.1f, 0.0f, 0.0f, "%.2f")) { ez->p0 = ImMax(sp[0], 0.01f); ez->p1 = ImMax(sp[1], 0.01f); ez->p2 = ImMax(sp[2], 0.0f); ez->p3 = sp[3]; changed = true; }
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("mass, stiffness, damping, initial velocity");
		} else if (ez->type == iam_ease_cubic_bezier) {
			float b[4] = { ez->p0, ez->p1, ez->p2, ez->p3 };
			if (ImGui::DragFloat4("##bezier", b, 0.005f, -2.0f, 3.0f, "%.3f")) {
				ez->p0 = ImClamp(b[0], 0.0f, 1.0f); ez->p1 = b[1]; ez->p2 = ImClamp(b[2], 0.0f, 1.0f); ez->p3 = b[3];
				changed = true;
			}
		} else if (ez->type == iam_ease_curve) {
			ImGuiID cid = iam_detail::curve_id_from(ez->p0, ez->p1);
			char const* cname = iam_get_id_name(cid);
			if (cname) ImGui::TextDisabled("%s: double-click adds a point, right-click removes it", cname);
			else ImGui::TextDisabled("0x%08X: double-click adds a point, right-click removes it", cid);
		}
		ImGui::PopItemWidth();

		// Graph
		ImVec2 gsize(w, size_in.y > 0.0f ? size_in.y : ImMin(w * 0.6f, fs * 12.0f));
		ImVec2 gp = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##graph", gsize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		bool const hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		// y range: [0,1] plus any overshoot
		float y_min = 0.0f, y_max = 1.0f;
		ImVector<ImVec2> curve_pts;
		ImGuiID cid = 0;
		if (ez->type == iam_ease_curve) {
			cid = iam_detail::curve_id_from(ez->p0, ez->p1);
			iam_detail::ease_curve* c = iam_detail::find_curve(cid);
			if (c) curve_pts = c->pts;
			else { default_curve(curve_pts); set_curve(cid, curve_pts); }
			for (int i = 0; i < curve_pts.Size; i++) { y_min = ImMin(y_min, curve_pts[i].y); y_max = ImMax(y_max, curve_pts[i].y); }
		}
		for (int i = 0; i <= 48; i++) { float v = iam_detail::eval(*ez, i / 48.0f); y_min = ImMin(y_min, v); y_max = ImMax(y_max, v); }
		if (ez->type == iam_ease_cubic_bezier) { y_min = ImMin(y_min, ImMin(ez->p1, ez->p3)); y_max = ImMax(y_max, ImMax(ez->p1, ez->p3)); }
		float const pad = fs * 0.8f;
		ImVec2 g0(gp.x + pad, gp.y + pad), g1(gp.x + gsize.x - pad * 3.0f, gp.y + gsize.y - pad);
		float const yr = (y_max - y_min) > 1e-4f ? (y_max - y_min) : 1.0f;
		#define IAM_EE_X(vx_) (g0.x + (vx_) * (g1.x - g0.x))
		#define IAM_EE_Y(vy_) (g1.y - ((vy_) - y_min) / yr * (g1.y - g0.y))
		#define IAM_EE_P(pp_) ImVec2(IAM_EE_X((pp_).x), IAM_EE_Y((pp_).y))
		ImU32 const col_bg = IM_COL32(25, 27, 34, 255), col_grid = IM_COL32(55, 60, 74, 255);
		ImU32 const col_curve = ZimaBlue, col_handle = AgedCopper;
		dl->AddRectFilled(gp, ImVec2(gp.x + gsize.x, gp.y + gsize.y), col_bg, 4.0f);
		dl->AddLine(ImVec2(g0.x, IAM_EE_Y(0.0f)), ImVec2(g1.x, IAM_EE_Y(0.0f)), col_grid);
		dl->AddLine(ImVec2(g0.x, IAM_EE_Y(1.0f)), ImVec2(g1.x, IAM_EE_Y(1.0f)), col_grid);
		dl->AddLine(ImVec2(g0.x, g0.y), ImVec2(g0.x, g1.y), col_grid);
		dl->AddLine(ImVec2(g1.x, g0.y), ImVec2(g1.x, g1.y), col_grid);
		ImVec2 line[97];
		for (int i = 0; i <= 96; i++) { float t = i / 96.0f; line[i] = ImVec2(IAM_EE_X(t), IAM_EE_Y(iam_detail::eval(*ez, t))); }
		dl->AddPolyline(line, 97, col_curve, 0, 2.0f);

		// Interactive handles
		ImVec2 const mouse = io.MousePos;
		float const grab = fs * 0.6f;
		ImGuiStorage* store = ImGui::GetStateStorage();
		ImGuiID const drag_key = ImGui::GetID("##drag_index");
		int drag = store->GetInt(drag_key, -1);
		float const mx = ImClamp((mouse.x - g0.x) / (g1.x - g0.x), 0.0f, 1.0f);
		float const my = y_min + (g1.y - mouse.y) / (g1.y - g0.y) * yr;
		if (ez->type == iam_ease_cubic_bezier) {
			ImVec2 h[2] = { ImVec2(ez->p0, ez->p1), ImVec2(ez->p2, ez->p3) };
			ImVec2 a0 = IAM_EE_P(ImVec2(0, 0)), a1 = IAM_EE_P(ImVec2(1, 1));
			dl->AddLine(a0, IAM_EE_P(h[0]), col_handle, 1.5f);
			dl->AddLine(a1, IAM_EE_P(h[1]), col_handle, 1.5f);
			if (ImGui::IsItemActivated()) {
				drag = -1;
				for (int i = 0; i < 2; i++) { ImVec2 d = IAM_EE_P(h[i]); if (ImLengthSqr(ImVec2(d.x - mouse.x, d.y - mouse.y)) < grab * grab * 4) drag = i; }
			}
			if (active && drag >= 0) {
				if (drag == 0) { ez->p0 = mx; ez->p1 = my; } else { ez->p2 = mx; ez->p3 = my; }
				changed = true;
			}
			for (int i = 0; i < 2; i++) dl->AddCircleFilled(IAM_EE_P(h[i]), grab * 0.6f, (drag == i && active) ? IM_COL32_WHITE : col_handle);
		} else if (ez->type == iam_ease_curve && curve_pts.Size >= 4) {
			int const n = curve_pts.Size;
			for (int k = 0; k + 3 < n; k += 3) {
				dl->AddLine(IAM_EE_P(curve_pts[k]), IAM_EE_P(curve_pts[k + 1]), col_handle, 1.0f);
				dl->AddLine(IAM_EE_P(curve_pts[k + 3]), IAM_EE_P(curve_pts[k + 2]), col_handle, 1.0f);
			}
			int hover_i = -1;
			if (hovered || active) {
				float best = grab * grab * 4;
				for (int i = 0; i < n; i++) { ImVec2 d = IAM_EE_P(curve_pts[i]); float dd = ImLengthSqr(ImVec2(d.x - mouse.x, d.y - mouse.y)); if (dd < best) { best = dd; hover_i = i; } }
			}
			if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) drag = hover_i;
			bool edited = false;
			if (active && ImGui::IsMouseDown(ImGuiMouseButton_Left) && drag >= 0 && drag < n) {
				ImVector<ImVec2>& P = curve_pts;
				if (drag % 3 == 0) {  // anchor: x between neighbours (ends fixed at x 0 / 1), moves its handles along
					float nx = (drag == 0) ? 0.0f : (drag == n - 1) ? 1.0f : ImClamp(mx, P[drag - 3].x + 0.01f, P[drag + 3].x - 0.01f);
					ImVec2 delta(nx - P[drag].x, my - P[drag].y);
					P[drag] = ImVec2(nx, my);
					if (drag > 0) P[drag - 1] = ImVec2(P[drag - 1].x + delta.x, P[drag - 1].y + delta.y);
					if (drag < n - 1) P[drag + 1] = ImVec2(P[drag + 1].x + delta.x, P[drag + 1].y + delta.y);
				} else {  // handle: x within its segment so x stays monotonic
					int seg = drag / 3;
					P[drag] = ImVec2(ImClamp(mx, P[seg * 3].x, P[seg * 3 + 3].x), my);
				}
				for (int k = 0; k + 3 < n; k += 3) {  // keep handles inside their segment after anchor moves
					P[k + 1].x = ImClamp(P[k + 1].x, P[k].x, P[k + 3].x);
					P[k + 2].x = ImClamp(P[k + 2].x, P[k].x, P[k + 3].x);
				}
				edited = true;
			}
			if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hover_i < 0) {
				int seg = 0;
				while (seg * 3 + 3 < n - 1 && curve_pts[seg * 3 + 3].x < mx) seg++;
				split_segment(curve_pts, seg, solve_u(curve_pts, seg, mx));
				edited = true;
				drag = -1;
			}
			if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hover_i > 0 && hover_i < n - 1 && hover_i % 3 == 0) {
				curve_pts.erase(curve_pts.Data + hover_i - 1, curve_pts.Data + hover_i + 2);  // anchor + its two handles
				edited = true;
				drag = -1;
			}
			if (edited) { set_curve(cid, curve_pts); changed = true; }
			for (int i = 0; i < curve_pts.Size; i++) {
				bool anchor = (i % 3 == 0);
				ImU32 c = (i == drag && active) || i == hover_i ? IM_COL32_WHITE : (anchor ? col_curve : col_handle);
				if (anchor) dl->AddRectFilled(ImVec2(IAM_EE_X(curve_pts[i].x) - grab * 0.5f, IAM_EE_Y(curve_pts[i].y) - grab * 0.5f), ImVec2(IAM_EE_X(curve_pts[i].x) + grab * 0.5f, IAM_EE_Y(curve_pts[i].y) + grab * 0.5f), c);
				else dl->AddCircleFilled(IAM_EE_P(curve_pts[i]), grab * 0.45f, c);
			}
		}
		if (!active) drag = -1;
		store->SetInt(drag_key, drag);

		// Preview: a dot running along the curve, and the value on a side bar
		float const ph = ImFmod((float)ImGui::GetTime(), 1.8f) / 1.4f;
		float const pt = ImClamp(ph, 0.0f, 1.0f), pv = iam_detail::eval(*ez, pt);
		dl->AddCircleFilled(ImVec2(IAM_EE_X(pt), IAM_EE_Y(pv)), fs * 0.25f, IM_COL32(255, 255, 255, 200));
		float const bx = gp.x + gsize.x - pad * 1.5f;
		dl->AddLine(ImVec2(bx, IAM_EE_Y(0.0f)), ImVec2(bx, IAM_EE_Y(1.0f)), col_grid, 2.0f);
		dl->AddCircleFilled(ImVec2(bx, IAM_EE_Y(pv)), fs * 0.35f, col_handle);
		#undef IAM_EE_X
		#undef IAM_EE_Y
		#undef IAM_EE_P

		ImGui::EndGroup();
		ImGui::PopID();
		return changed;
	}
} // namespace iam_ease_editor_detail

bool iam_ease_editor(char const* label, iam_ease_desc* ez, iam_vec2 size) {
	if (!ez) return false;
	return iam_ease_editor_detail::editor(label, ez, IAM_VEC2_TO_IM(size));
}

bool iam_clip_editor(ImGuiID clip_id, ImGuiID preview_instance_id) {
	return iam_clip_editor_detail::editor_body(clip_id, preview_instance_id);
}

bool iam_show_clip_editor(ImGuiID clip_id, bool* p_open, ImGuiID preview_instance_id) {
	char title[96];
	char const* name = iam_get_id_name(clip_id);
	if (name) snprintf(title, sizeof(title), "Clip Editor - %s###iam_clip_editor_%08X", name, clip_id);
	else snprintf(title, sizeof(title), "Clip Editor - 0x%08X###iam_clip_editor_%08X", clip_id, clip_id);
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetFontSize() * 60.0f, ImGui::GetFontSize() * 26.0f), ImGuiCond_FirstUseEver);
	bool modified = false;
	if (ImGui::Begin(title, p_open)) modified = iam_clip_editor_detail::editor_body(clip_id, preview_instance_id);
	ImGui::End();
	return modified;
}

#undef IMGUI_STORAGE_PAIR

// ----------------------------------------------------
// Public API wrappers: convert iam_vec2/iam_vec4 (IAM_VEC2/IAM_VEC4) <-> ImVec2/ImVec4
// ----------------------------------------------------
iam_drag_feedback iam_drag_begin(ImGuiID id, iam_vec2 pos) { return iam_drag_begin_impl(id, IAM_VEC2_TO_IM(pos)); }
iam_drag_feedback iam_drag_update(ImGuiID id, iam_vec2 pos, float dt) { return iam_drag_update_impl(id, IAM_VEC2_TO_IM(pos), dt); }
iam_drag_feedback iam_drag_release(ImGuiID id, iam_vec2 pos, iam_drag_opts const& opts, float dt) { return iam_drag_release_impl(id, IAM_VEC2_TO_IM(pos), opts, dt); }
iam_vec2 iam_oscillate_vec2(ImGuiID id, iam_vec2 amplitude, iam_vec2 frequency, int wave_type, iam_vec2 phase, float dt) { return IAM_VEC2_FROM_IM(iam_oscillate_vec2_impl(id, IAM_VEC2_TO_IM(amplitude), IAM_VEC2_TO_IM(frequency), wave_type, IAM_VEC2_TO_IM(phase), dt)); }
iam_vec4 iam_oscillate_vec4(ImGuiID id, iam_vec4 amplitude, iam_vec4 frequency, int wave_type, iam_vec4 phase, float dt) { return IAM_VEC4_FROM_IM(iam_oscillate_vec4_impl(id, IAM_VEC4_TO_IM(amplitude), IAM_VEC4_TO_IM(frequency), wave_type, IAM_VEC4_TO_IM(phase), dt)); }
iam_vec4 iam_oscillate_color(ImGuiID id, iam_vec4 base_color, iam_vec4 amplitude, float frequency, int wave_type, float phase, int color_space, float dt) { return IAM_VEC4_FROM_IM(iam_oscillate_color_impl(id, IAM_VEC4_TO_IM(base_color), IAM_VEC4_TO_IM(amplitude), frequency, wave_type, phase, color_space, dt)); }
iam_vec2 iam_shake_vec2(ImGuiID id, iam_vec2 intensity, float frequency, float decay_time, float dt) { return IAM_VEC2_FROM_IM(iam_shake_vec2_impl(id, IAM_VEC2_TO_IM(intensity), frequency, decay_time, dt)); }
iam_vec4 iam_shake_vec4(ImGuiID id, iam_vec4 intensity, float frequency, float decay_time, float dt) { return IAM_VEC4_FROM_IM(iam_shake_vec4_impl(id, IAM_VEC4_TO_IM(intensity), frequency, decay_time, dt)); }
iam_vec4 iam_shake_color(ImGuiID id, iam_vec4 base_color, iam_vec4 intensity, float frequency, float decay_time, int color_space, float dt) { return IAM_VEC4_FROM_IM(iam_shake_color_impl(id, IAM_VEC4_TO_IM(base_color), IAM_VEC4_TO_IM(intensity), frequency, decay_time, color_space, dt)); }
iam_vec2 iam_wiggle_vec2(ImGuiID id, iam_vec2 amplitude, float frequency, float dt) { return IAM_VEC2_FROM_IM(iam_wiggle_vec2_impl(id, IAM_VEC2_TO_IM(amplitude), frequency, dt)); }
iam_vec4 iam_wiggle_vec4(ImGuiID id, iam_vec4 amplitude, float frequency, float dt) { return IAM_VEC4_FROM_IM(iam_wiggle_vec4_impl(id, IAM_VEC4_TO_IM(amplitude), frequency, dt)); }
iam_vec4 iam_wiggle_color(ImGuiID id, iam_vec4 base_color, iam_vec4 amplitude, float frequency, int color_space, float dt) { return IAM_VEC4_FROM_IM(iam_wiggle_color_impl(id, IAM_VEC4_TO_IM(base_color), IAM_VEC4_TO_IM(amplitude), frequency, color_space, dt)); }
iam_vec2 iam_tween_vec2(ImGuiID id, ImGuiID channel_id, iam_vec2 target, float dur, iam_ease_desc const& ez, int policy, float dt, iam_vec2 init_value) { return IAM_VEC2_FROM_IM(iam_tween_vec2_impl(id, channel_id, IAM_VEC2_TO_IM(target), dur, ez, policy, dt, IAM_VEC2_TO_IM(init_value))); }
iam_vec4 iam_tween_vec4(ImGuiID id, ImGuiID channel_id, iam_vec4 target, float dur, iam_ease_desc const& ez, int policy, float dt, iam_vec4 init_value) { return IAM_VEC4_FROM_IM(iam_tween_vec4_impl(id, channel_id, IAM_VEC4_TO_IM(target), dur, ez, policy, dt, IAM_VEC4_TO_IM(init_value))); }
iam_vec4 iam_tween_color(ImGuiID id, ImGuiID channel_id, iam_vec4 target_srgb, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt, iam_vec4 init_value) { return IAM_VEC4_FROM_IM(iam_tween_color_impl(id, channel_id, IAM_VEC4_TO_IM(target_srgb), dur, ez, policy, color_space, dt, IAM_VEC4_TO_IM(init_value))); }
iam_vec2 iam_anchor_size(int space) { return IAM_VEC2_FROM_IM(iam_anchor_size_impl(space)); }
iam_vec2 iam_tween_vec2_rel(ImGuiID id, ImGuiID channel_id, iam_vec2 percent, iam_vec2 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt) { return IAM_VEC2_FROM_IM(iam_tween_vec2_rel_impl(id, channel_id, IAM_VEC2_TO_IM(percent), IAM_VEC2_TO_IM(px_bias), dur, ez, policy, anchor_space, dt)); }
iam_vec4 iam_tween_vec4_rel(ImGuiID id, ImGuiID channel_id, iam_vec4 percent, iam_vec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt) { return IAM_VEC4_FROM_IM(iam_tween_vec4_rel_impl(id, channel_id, IAM_VEC4_TO_IM(percent), IAM_VEC4_TO_IM(px_bias), dur, ez, policy, anchor_space, dt)); }
iam_vec4 iam_tween_color_rel(ImGuiID id, ImGuiID channel_id, iam_vec4 percent, iam_vec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int color_space, int anchor_space, float dt) { return IAM_VEC4_FROM_IM(iam_tween_color_rel_impl(id, channel_id, IAM_VEC4_TO_IM(percent), IAM_VEC4_TO_IM(px_bias), dur, ez, policy, color_space, anchor_space, dt)); }
iam_vec2 iam_tween_vec2_resolved(ImGuiID id, ImGuiID channel_id, iam_vec2_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt) { return IAM_VEC2_FROM_IM(iam_tween_vec2_resolved_impl(id, channel_id, fn, user, dur, ez, policy, dt)); }
iam_vec4 iam_tween_vec4_resolved(ImGuiID id, ImGuiID channel_id, iam_vec4_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt) { return IAM_VEC4_FROM_IM(iam_tween_vec4_resolved_impl(id, channel_id, fn, user, dur, ez, policy, dt)); }
iam_vec4 iam_tween_color_resolved(ImGuiID id, ImGuiID channel_id, iam_color_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt) { return IAM_VEC4_FROM_IM(iam_tween_color_resolved_impl(id, channel_id, fn, user, dur, ez, policy, color_space, dt)); }
void iam_rebase_vec2(ImGuiID id, ImGuiID channel_id, iam_vec2 new_target, float dt) { iam_rebase_vec2_impl(id, channel_id, IAM_VEC2_TO_IM(new_target), dt); }
void iam_rebase_vec4(ImGuiID id, ImGuiID channel_id, iam_vec4 new_target, float dt) { iam_rebase_vec4_impl(id, channel_id, IAM_VEC4_TO_IM(new_target), dt); }
void iam_rebase_color(ImGuiID id, ImGuiID channel_id, iam_vec4 new_target, float dt) { iam_rebase_color_impl(id, channel_id, IAM_VEC4_TO_IM(new_target), dt); }
iam_vec4 iam_get_blended_color(iam_vec4 a_srgb, iam_vec4 b_srgb, float t, int color_space) { return IAM_VEC4_FROM_IM(iam_get_blended_color_impl(IAM_VEC4_TO_IM(a_srgb), IAM_VEC4_TO_IM(b_srgb), t, color_space)); }
iam_vec2 iam_tween_vec2_per_axis(ImGuiID id, ImGuiID channel_id, iam_vec2 target, float dur, iam_ease_per_axis const& ez, int policy, float dt) { return IAM_VEC2_FROM_IM(iam_tween_vec2_per_axis_impl(id, channel_id, IAM_VEC2_TO_IM(target), dur, ez, policy, dt)); }
iam_vec4 iam_tween_vec4_per_axis(ImGuiID id, ImGuiID channel_id, iam_vec4 target, float dur, iam_ease_per_axis const& ez, int policy, float dt) { return IAM_VEC4_FROM_IM(iam_tween_vec4_per_axis_impl(id, channel_id, IAM_VEC4_TO_IM(target), dur, ez, policy, dt)); }
iam_vec4 iam_tween_color_per_axis(ImGuiID id, ImGuiID channel_id, iam_vec4 target_srgb, float dur, iam_ease_per_axis const& ez, int policy, int color_space, float dt) { return IAM_VEC4_FROM_IM(iam_tween_color_per_axis_impl(id, channel_id, IAM_VEC4_TO_IM(target_srgb), dur, ez, policy, color_space, dt)); }
iam_vec2 iam_bezier_quadratic(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, float t) { return IAM_VEC2_FROM_IM(iam_bezier_quadratic_impl(IAM_VEC2_TO_IM(p0), IAM_VEC2_TO_IM(p1), IAM_VEC2_TO_IM(p2), t)); }
iam_vec2 iam_bezier_cubic(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, iam_vec2 p3, float t) { return IAM_VEC2_FROM_IM(iam_bezier_cubic_impl(IAM_VEC2_TO_IM(p0), IAM_VEC2_TO_IM(p1), IAM_VEC2_TO_IM(p2), IAM_VEC2_TO_IM(p3), t)); }
iam_vec2 iam_catmull_rom(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, iam_vec2 p3, float t, float tension) { return IAM_VEC2_FROM_IM(iam_catmull_rom_impl(IAM_VEC2_TO_IM(p0), IAM_VEC2_TO_IM(p1), IAM_VEC2_TO_IM(p2), IAM_VEC2_TO_IM(p3), t, tension)); }
iam_vec2 iam_bezier_quadratic_deriv(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, float t) { return IAM_VEC2_FROM_IM(iam_bezier_quadratic_deriv_impl(IAM_VEC2_TO_IM(p0), IAM_VEC2_TO_IM(p1), IAM_VEC2_TO_IM(p2), t)); }
iam_vec2 iam_bezier_cubic_deriv(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, iam_vec2 p3, float t) { return IAM_VEC2_FROM_IM(iam_bezier_cubic_deriv_impl(IAM_VEC2_TO_IM(p0), IAM_VEC2_TO_IM(p1), IAM_VEC2_TO_IM(p2), IAM_VEC2_TO_IM(p3), t)); }
iam_vec2 iam_catmull_rom_deriv(iam_vec2 p0, iam_vec2 p1, iam_vec2 p2, iam_vec2 p3, float t, float tension) { return IAM_VEC2_FROM_IM(iam_catmull_rom_deriv_impl(IAM_VEC2_TO_IM(p0), IAM_VEC2_TO_IM(p1), IAM_VEC2_TO_IM(p2), IAM_VEC2_TO_IM(p3), t, tension)); }
iam_vec2 iam_path_evaluate(ImGuiID path_id, float t) { return IAM_VEC2_FROM_IM(iam_path_evaluate_impl(path_id, t)); }
iam_vec2 iam_path_tangent(ImGuiID path_id, float t) { return IAM_VEC2_FROM_IM(iam_path_tangent_impl(path_id, t)); }
iam_vec2 iam_tween_path(ImGuiID id, ImGuiID channel_id, ImGuiID path_id, float dur, iam_ease_desc const& ez, int policy, float dt) { return IAM_VEC2_FROM_IM(iam_tween_path_impl(id, channel_id, path_id, dur, ez, policy, dt)); }
iam_vec2 iam_path_evaluate_at_distance(ImGuiID path_id, float distance) { return IAM_VEC2_FROM_IM(iam_path_evaluate_at_distance_impl(path_id, distance)); }
iam_vec2 iam_path_tangent_at_distance(ImGuiID path_id, float distance) { return IAM_VEC2_FROM_IM(iam_path_tangent_at_distance_impl(path_id, distance)); }
iam_vec2 iam_path_morph(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts) { return IAM_VEC2_FROM_IM(iam_path_morph_impl(path_a, path_b, t, blend, opts)); }
iam_vec2 iam_path_morph_tangent(ImGuiID path_a, ImGuiID path_b, float t, float blend, iam_morph_opts const& opts) { return IAM_VEC2_FROM_IM(iam_path_morph_tangent_impl(path_a, path_b, t, blend, opts)); }
iam_vec2 iam_tween_path_morph(ImGuiID id, ImGuiID channel_id, ImGuiID path_a, ImGuiID path_b, float target_blend, float dur, iam_ease_desc const& path_ease, iam_ease_desc const& morph_ease, int policy, float dt, iam_morph_opts const& opts) { return IAM_VEC2_FROM_IM(iam_tween_path_morph_impl(id, channel_id, path_a, path_b, target_blend, dur, path_ease, morph_ease, policy, dt, opts)); }
iam_vec2 iam_noise_channel_vec2(ImGuiID id, iam_vec2 frequency, iam_vec2 amplitude, iam_noise_opts const& opts, float dt) { return IAM_VEC2_FROM_IM(iam_noise_channel_vec2_impl(id, IAM_VEC2_TO_IM(frequency), IAM_VEC2_TO_IM(amplitude), opts, dt)); }
iam_vec4 iam_noise_channel_vec4(ImGuiID id, iam_vec4 frequency, iam_vec4 amplitude, iam_noise_opts const& opts, float dt) { return IAM_VEC4_FROM_IM(iam_noise_channel_vec4_impl(id, IAM_VEC4_TO_IM(frequency), IAM_VEC4_TO_IM(amplitude), opts, dt)); }
iam_vec4 iam_noise_channel_color(ImGuiID id, iam_vec4 base_color, iam_vec4 amplitude, float frequency, iam_noise_opts const& opts, int color_space, float dt) { return IAM_VEC4_FROM_IM(iam_noise_channel_color_impl(id, IAM_VEC4_TO_IM(base_color), IAM_VEC4_TO_IM(amplitude), frequency, opts, color_space, dt)); }
iam_vec2 iam_smooth_noise_vec2(ImGuiID id, iam_vec2 amplitude, float speed, float dt) { return IAM_VEC2_FROM_IM(iam_smooth_noise_vec2_impl(id, IAM_VEC2_TO_IM(amplitude), speed, dt)); }
iam_vec4 iam_smooth_noise_vec4(ImGuiID id, iam_vec4 amplitude, float speed, float dt) { return IAM_VEC4_FROM_IM(iam_smooth_noise_vec4_impl(id, IAM_VEC4_TO_IM(amplitude), speed, dt)); }
iam_vec4 iam_smooth_noise_color(ImGuiID id, iam_vec4 base_color, iam_vec4 amplitude, float speed, int color_space, float dt) { return IAM_VEC4_FROM_IM(iam_smooth_noise_color_impl(id, IAM_VEC4_TO_IM(base_color), IAM_VEC4_TO_IM(amplitude), speed, color_space, dt)); }
void iam_transform_quad(iam_vec2* quad, iam_vec2 center, float angle_rad, iam_vec2 translation) {
	ImVec2 q[4];
	for (int i = 0; i < 4; i++) q[i] = IAM_VEC2_TO_IM(quad[i]);
	iam_transform_quad_impl(q, IAM_VEC2_TO_IM(center), angle_rad, IAM_VEC2_TO_IM(translation));
	for (int i = 0; i < 4; i++) quad[i] = IAM_VEC2_FROM_IM(q[i]);
}
void iam_make_glyph_quad(iam_vec2* quad, iam_vec2 pos, float angle_rad, float glyph_width, float glyph_height, float baseline_offset) {
	ImVec2 q[4];
	iam_make_glyph_quad_impl(q, IAM_VEC2_TO_IM(pos), angle_rad, glyph_width, glyph_height, baseline_offset);
	for (int i = 0; i < 4; i++) quad[i] = IAM_VEC2_FROM_IM(q[i]);
}
bool iam_get_blended_vec2(ImGuiID instance_id, ImGuiID channel, iam_vec2* out) {
	ImVec2 v;
	if (!out || !iam_get_blended_vec2_impl(instance_id, channel, &v)) return false;
	*out = IAM_VEC2_FROM_IM(v);
	return true;
}
bool iam_get_blended_vec4(ImGuiID instance_id, ImGuiID channel, iam_vec4* out) {
	ImVec4 v;
	if (!out || !iam_get_blended_vec4_impl(instance_id, channel, &v)) return false;
	*out = IAM_VEC4_FROM_IM(v);
	return true;
}
