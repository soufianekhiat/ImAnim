// im_anim.cpp — Dear ImGui animation helpers implementation.
// Author: Soufiane KHIAT
// License: MIT
// Style: snake_case, tabs, east const.

#include "im_anim.h"
#include "imgui.h"
#include "imgui_internal.h"

// ----------------------------------------------------
// Internal: parameterized easing LUT cache (ImPool)
// ----------------------------------------------------
namespace iam_detail {

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
		if (t < 1/2.75f) return 7.5625f*t*t;
		else if (t < 2/2.75f) { t -= 1.5f/2.75f; return 7.5625f*t*t + 0.75f; }
		else if (t < 2.5f/2.75f){ t -= 2.25f/2.75f; return 7.5625f*t*t + 0.9375f; }
		else { t -= 2.625f/2.75f; return 7.5625f*t*t + 0.984375f; }
	}

	static float elastic_core(float t, float a, float p) {
		if (t == 0.f || t == 1.f) return t;
		float A = (a <= 0.f ? 1.f : a);
		float P = (p <= 0.f ? 0.3f : p);
		float s = (P / (2 * 3.1415926535f)) * asinf(1.0f / A);
		return -(A * ImPow(2.f, 10.f * (t - 1.f)) * ImSin((t - 1.f - s) * (2 * 3.1415926535f) / P));
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
					if (mode == 1)		y = ImFloor(x * n + 1e-6f) / (float)n;
					else if (mode == 2)	{ y = (ImFloor(x * n - 0.5f + 1e-6f) + 0.5f) / (float)n; if (y<0) y=0; if (y>1) y=1; }
					else				y = ImFloor(x * n + 1e-6f) / (float)n;
					break;
				}
				case iam_ease_in_elastic: {
					float a = (d.p0 <= 0.f ? 1.f : d.p0), p = (d.p1 <= 0.f ? 0.3f : d.p1);
					y = 1.f + elastic_core(1.f - x, a, p);
					break;
				}
				case iam_ease_out_elastic: {
					float a = (d.p0 <= 0.f ? 1.f : d.p0), p = (d.p1 <= 0.f ? 0.3f : d.p1);
					y = 1.f - elastic_core(x, a, p);
					break;
				}
				case iam_ease_in_out_elastic: {
					float a = (d.p0 <= 0.f ? 1.f : d.p0), p = (d.p1 <= 0.f ? 0.45f : d.p1);
					if (x < 0.5f) y = 0.5f * (1.f + elastic_core(1.f - 2.f*x, a, p));
					else y = 0.5f * (1.f - elastic_core(2.f*x - 1.f, a, p)) + 0.5f;
					break;
				}
				case iam_ease_in_back: {
					float s = (d.p0 == 0.f ? 1.70158f : d.p0);
					y = back_core(x, s);
					break;
				}
				case iam_ease_out_back: {
					float s = (d.p0 == 0.f ? 1.70158f : d.p0);
					y = 1.f - back_core(1.f - x, s);
					break;
				}
				case iam_ease_in_out_back: {
					float s = (d.p0 == 0.f ? 1.70158f * 1.525f : d.p0);
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
					y = spring_unit(x, (d.p0<=0.f?1.f:d.p0), (d.p1<=0.f?120.f:d.p1), (d.p2<=0.f?20.f:d.p2), d.p3);
					break;
				default: y = x; break;
			}
			if (y < 0.f) y = 0.f; if (y > 1.f) y = 1.f;
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

static float eval_preset_internal(int type, float t) {
	if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
	switch (type) {
		case iam_ease_linear: return t;
		case iam_ease_in_quad: return t*t;
		case iam_ease_out_quad: { float u = 1.f - t; return 1.f - u*u; }
		case iam_ease_in_out_quad: return (t < 0.5f) ? 2.f*t*t : 1.f - ImPow(-2.f*t + 2.f, 2.f)/2.f;
		case iam_ease_in_cubic: return t*t*t;
		case iam_ease_out_cubic: { float u = 1.f - t; return 1.f - u*u*u; }
		case iam_ease_in_out_cubic: return (t < 0.5f) ? 4.f*t*t*t : 1.f - ImPow(-2.f*t + 2.f, 3.f)/2.f;
		case iam_ease_in_quart: return t*t*t*t;
		case iam_ease_out_quart: { float u = 1.f - t; return 1.f - u*u*u*u; }
		case iam_ease_in_out_quart: return (t < 0.5f) ? 8.f*t*t*t*t : 1.f - ImPow(-2.f*t + 2.f, 4.f)/2.f;
		case iam_ease_in_quint: return t*t*t*t*t;
		case iam_ease_out_quint: { float u = 1.f - t; return 1.f - u*u*u*u*u; }
		case iam_ease_in_out_quint: return (t < 0.5f) ? 16.f*t*t*t*t*t : 1.f - ImPow(-2.f*t + 2.f, 5.f)/2.f;
		case iam_ease_in_sine: return 1.f - ImCos((t * 3.1415926535f) / 2.f);
		case iam_ease_out_sine: return ImSin((t * 3.1415926535f) / 2.f);
		case iam_ease_in_out_sine: return -(ImCos(3.1415926535f * t) - 1.f) / 2.f;
		case iam_ease_in_expo: return (t == 0.f) ? 0.f : ImPow(2.f, 10.f * t - 10.f);
		case iam_ease_out_expo: return (t == 1.f) ? 1.f : 1.f - ImPow(2.f, -10.f * t);
		case iam_ease_in_out_expo:
			if (t == 0.f) return 0.f; if (t == 1.f) return 1.f;
			return (t < 0.5f) ? ImPow(2.f, 20.f*t - 10.f) / 2.f : (2.f - ImPow(2.f, -20.f*t + 10.f)) / 2.f;
		case iam_ease_in_circ: return 1.f - ImSqrt(1.f - t*t);
		case iam_ease_out_circ: return ImSqrt(1.f - (t - 1.f)*(t - 1.f));
		case iam_ease_in_out_circ:
			return (t < 0.5f) ? (1.f - ImSqrt(1.f - 4.f*t*t)) / 2.f
							 : (ImSqrt(1.f - (2.f*t - 2.f)*(2.f*t - 2.f)) + 1.f) / 2.f;
		default: return t;
	}
}

static float eval(iam_ease_desc const& d, float t) {
	switch (d.type) {
		case iam_ease_linear:
		case iam_ease_in_quad: case iam_ease_out_quad: case iam_ease_in_out_quad:
		case iam_ease_in_cubic: case iam_ease_out_cubic: case iam_ease_in_out_cubic:
		case iam_ease_in_quart: case iam_ease_out_quart: case iam_ease_in_out_quart:
		case iam_ease_in_quint: case iam_ease_out_quint: case iam_ease_in_out_quint:
		case iam_ease_in_sine:  case iam_ease_out_sine:  case iam_ease_in_out_sine:
		case iam_ease_in_expo:  case iam_ease_out_expo:  case iam_ease_in_out_expo:
		case iam_ease_in_circ:  case iam_ease_out_circ:  case iam_ease_in_out_circ:
			return eval_preset_internal(d.type, t);
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

struct float_chan {
	float	current, start, target;
	float	dur, t;
	iam_ease_desc ez;
	int		policy;
	unsigned last_seen_frame;
	unsigned has_pending;
	float	pending_target;
	float_chan() {
		current=0; start=0; target=0; dur=1e-6f; t=1.f;
		ez = { iam_ease_out_cubic, 0,0,0,0 };
		policy = iam_policy_crossfade;
		last_seen_frame = 0; has_pending = 0; pending_target = 0;
	}
	void set(float trg, float d, iam_ease_desc const& e, int pol) { start=current; target=trg; dur=(d<=1e-6f?1e-6f:d); t=0; ez=e; policy=pol; }
	void tick(float dt) { if (t>=1.f){ current=target; return; } if (dt>0) t += dt/dur; float k = eval(ez, t); current = start + (target - start) * k; }
};

struct vec2_chan {
	ImVec2	current, start, target;
	float	dur, t;
	iam_ease_desc ez;
	int		policy;
	unsigned last_seen_frame;
	unsigned has_pending;
	ImVec2	pending_target;
	vec2_chan() {
		current=ImVec2(0,0); start=current; target=current; dur=1e-6f; t=1.f;
		ez = { iam_ease_out_cubic, 0,0,0,0 };
		policy = iam_policy_crossfade;
		last_seen_frame = 0; has_pending = 0; pending_target = ImVec2(0,0);
	}
	void set(ImVec2 trg, float d, iam_ease_desc const& e, int pol) { start=current; target=trg; dur=(d<=1e-6f?1e-6f:d); t=0; ez=e; policy=pol; }
	void tick(float dt) {
		if (t>=1.f){ current=target; return; } if (dt>0) t += dt/dur; float k = eval(ez, t);
		current.x = start.x + (target.x - start.x) * k;
		current.y = start.y + (target.y - start.y) * k;
	}
};

struct vec4_chan {
	ImVec4	current, start, target;
	float	dur, t;
	iam_ease_desc ez;
	int		policy;
	unsigned last_seen_frame;
	unsigned has_pending;
	ImVec4	pending_target;
	vec4_chan() {
		current=ImVec4(1,1,1,1); start=current; target=current; dur=1e-6f; t=1.f;
		ez = { iam_ease_out_cubic, 0,0,0,0 };
		policy = iam_policy_crossfade;
		last_seen_frame = 0; has_pending = 0; pending_target = ImVec4(0,0,0,0);
	}
	void set(ImVec4 trg, float d, iam_ease_desc const& e, int pol) { start=current; target=trg; dur=(d<=1e-6f?1e-6f:d); t=0; ez=e; policy=pol; }
	void tick(float dt) {
		if (t>=1.f){ current=target; return; } if (dt>0) t += dt/dur; float k = eval(ez, t);
		current.x = start.x + (target.x - start.x) * k;
		current.y = start.y + (target.y - start.y) * k;
		current.z = start.z + (target.z - start.z) * k;
		current.w = start.w + (target.w - start.w) * k;
	}
};

struct int_chan {
	int		current, start, target;
	float	dur, t;
	iam_ease_desc ez;
	int		policy;
	unsigned last_seen_frame;
	unsigned has_pending;
	int		pending_target;
	int_chan() {
		current=0; start=0; target=0; dur=1e-6f; t=1.f;
		ez = { iam_ease_out_cubic, 0,0,0,0 };
		policy = iam_policy_crossfade;
		last_seen_frame = 0; has_pending = 0; pending_target = 0;
	}
	void set(int trg, float d, iam_ease_desc const& e, int pol) { start=current; target=trg; dur=(d<=1e-6f?1e-6f:d); t=0; ez=e; policy=pol; }
	void tick(float dt) {
		if (t>=1.f){ current=target; return; } if (dt>0) t += dt/dur; float k = eval(ez, t);
		float v = (float)start + ((float)target - (float)start) * k;
		current = (int)ImFloor(v + 0.5f);
	}
};

struct color_chan {
	ImVec4	current, start, target;
	float	dur, t;
	iam_ease_desc ez;
	int		policy;
	int		space;
	unsigned last_seen_frame;
	color_chan() {
		current=ImVec4(1,1,1,1); start=current; target=current; dur=1e-6f; t=1.f;
		ez = { iam_ease_out_cubic, 0,0,0,0 };
		policy = iam_policy_crossfade;
		space = iam_col_srgb_linear;
		last_seen_frame = 0;
	}
	void set(ImVec4 trg, float d, iam_ease_desc const& e, int pol, int sp) { start=current; target=trg; dur=(d<=1e-6f?1e-6f:d); t=0; ez=e; policy=pol; space=sp; }
	void tick(float dt) {
		if (t>=1.f){ current=target; return; } if (dt>0) t += dt/dur; float k = eval(ez, t);
		current = color::lerp_color(start, target, k, space);
	}
};

// Per-type pools
template<typename T>
struct pool_t {
	ImPool<T> pool;
	unsigned frame = 0;
	void begin() { ++frame; }
	T* get(ImGuiID key) { T* c = pool.GetOrAddByKey(key); c->last_seen_frame = frame; return c; }
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

} // namespace iam_detail

// ----------------------------------------------------
// Public API implementations
// ----------------------------------------------------

void iam_update_begin_frame() {
	iam_detail::g_float.begin();
	iam_detail::g_vec2.begin();
	iam_detail::g_vec4.begin();
	iam_detail::g_int.begin();
	iam_detail::g_color.begin();
}

void iam_gc(unsigned int max_age_frames) {
	iam_detail::g_float.gc(max_age_frames);
	iam_detail::g_vec2.gc(max_age_frames);
	iam_detail::g_vec4.gc(max_age_frames);
	iam_detail::g_int.gc(max_age_frames);
	iam_detail::g_color.gc(max_age_frames);
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

float iam_eval_preset(int type, float t) {
	return iam_detail::eval_preset_internal(type, t);
}

float iam_tween_float(ImGuiID id, ImGuiID channel_id, float target, float dur, iam_ease_desc const& ez, int policy, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	iam_detail::float_chan* c = iam_detail::g_float.get(key);
	bool const change = (c->policy!=policy) || (c->ez.type!=ez.type) ||
	                    (c->ez.p0!=ez.p0) || (c->ez.p1!=ez.p1) || (c->ez.p2!=ez.p2) || (c->ez.p3!=ez.p3) ||
	                    (fabsf(c->target - target) > 1e-6f) || (c->t >= 1.0f);
	if (change) {
		if (policy == iam_policy_queue && c->t < 1.0f && !c->has_pending) { c->pending_target = target; c->has_pending = 1; }
		else if (policy == iam_policy_cut) { c->current = c->start = c->target = target; c->t = 1.0f; c->dur = 1e-6f; c->ez = ez; c->policy = policy; }
		else { if (c->t < 1.0f && dt > 0) c->tick(dt); c->set(target, dur, ez, policy); c->tick(dt); }
	} else { c->tick(dt); }
	if (c->t >= 1.0f && c->has_pending) { c->set(c->pending_target, dur, ez, policy); c->has_pending = 0; }
	return c->current;
}

ImVec2 iam_tween_vec2(ImGuiID id, ImGuiID channel_id, ImVec2 target, float dur, iam_ease_desc const& ez, int policy, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	iam_detail::vec2_chan* c = iam_detail::g_vec2.get(key);
	bool const change = (c->policy!=policy) || (c->ez.type!=ez.type) ||
	                    (c->ez.p0!=ez.p0) || (c->ez.p1!=ez.p1) || (c->ez.p2!=ez.p2) || (c->ez.p3!=ez.p3) ||
	                    (fabsf(c->target.x - target.x) + fabsf(c->target.y - target.y) > 1e-6f) || (c->t >= 1.0f);
	if (change) {
		if (policy == iam_policy_queue && c->t < 1.0f && !c->has_pending) { c->pending_target = target; c->has_pending = 1; }
		else if (policy == iam_policy_cut) { c->current = c->start = c->target = target; c->t = 1.0f; c->dur = 1e-6f; c->ez = ez; c->policy = policy; }
		else { if (c->t < 1.0f && dt > 0) c->tick(dt); c->set(target, dur, ez, policy); c->tick(dt); }
	} else { c->tick(dt); }
	if (c->t >= 1.0f && c->has_pending) { c->set(c->pending_target, dur, ez, policy); c->has_pending = 0; }
	return c->current;
}

ImVec4 iam_tween_vec4(ImGuiID id, ImGuiID channel_id, ImVec4 target, float dur, iam_ease_desc const& ez, int policy, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	iam_detail::vec4_chan* c = iam_detail::g_vec4.get(key);
	bool const change = (c->policy!=policy) || (c->ez.type!=ez.type) ||
	                    (c->ez.p0!=ez.p0) || (c->ez.p1!=ez.p1) || (c->ez.p2!=ez.p2) || (c->ez.p3!=ez.p3) ||
	                    (fabsf(c->target.x-target.x)+fabsf(c->target.y-target.y)+fabsf(c->target.z-target.z)+fabsf(c->target.w-target.w) > 1e-6f) || (c->t >= 1.0f);
	if (change) {
		if (policy == iam_policy_queue && c->t < 1.0f && !c->has_pending) { c->pending_target = target; c->has_pending = 1; }
		else if (policy == iam_policy_cut) { c->current = c->start = c->target = target; c->t = 1.0f; c->dur = 1e-6f; c->ez = ez; c->policy = policy; }
		else { if (c->t < 1.0f && dt > 0) c->tick(dt); c->set(target, dur, ez, policy); c->tick(dt); }
	} else { c->tick(dt); }
	if (c->t >= 1.0f && c->has_pending) { c->set(c->pending_target, dur, ez, policy); c->has_pending = 0; }
	return c->current;
}

int iam_tween_int(ImGuiID id, ImGuiID channel_id, int target, float dur, iam_ease_desc const& ez, int policy, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	iam_detail::int_chan* c = iam_detail::g_int.get(key);
	bool const change = (c->policy!=policy) || (c->ez.type!=ez.type) ||
	                    (c->ez.p0!=ez.p0) || (c->ez.p1!=ez.p1) || (c->ez.p2!=ez.p2) || (c->ez.p3!=ez.p3) ||
	                    (c->target != target) || (c->t >= 1.0f);
	if (change) {
		if (policy == iam_policy_queue && c->t < 1.0f && !c->has_pending) { c->pending_target = target; c->has_pending = 1; }
		else if (policy == iam_policy_cut) { c->current = c->start = c->target = target; c->t = 1.0f; c->dur = 1e-6f; c->ez = ez; c->policy = policy; }
		else { if (c->t < 1.0f && dt > 0) c->tick(dt); c->set(target, dur, ez, policy); c->tick(dt); }
	} else { c->tick(dt); }
	if (c->t >= 1.0f && c->has_pending) { c->set(c->pending_target, dur, ez, policy); c->has_pending = 0; }
	return c->current;
}

ImVec4 iam_tween_color(ImGuiID id, ImGuiID channel_id, ImVec4 target_srgb, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	iam_detail::color_chan* c = iam_detail::g_color.get(key);
	bool const change = (c->policy!=policy) || (c->space != color_space) || (c->ez.type!=ez.type) ||
	                    (c->ez.p0!=ez.p0) || (c->ez.p1!=ez.p1) || (c->ez.p2!=ez.p2) || (c->ez.p3!=ez.p3) ||
	                    (fabsf(c->target.x-target_srgb.x)+fabsf(c->target.y-target_srgb.y)+fabsf(c->target.z-target_srgb.z)+fabsf(c->target.w-target_srgb.w) > 1e-6f) || (c->t >= 1.0f);
	if (change) {
		if (policy == iam_policy_cut) { c->current = c->start = c->target = target_srgb; c->t = 1.0f; c->dur = 1e-6f; c->ez = ez; c->policy = policy; c->space = color_space; }
		else { if (c->t < 1.0f && dt > 0) c->tick(dt); c->set(target_srgb, dur, ez, policy, color_space); c->tick(dt); }
	} else { c->tick(dt); }
	return c->current;
}

ImVec2 iam_anchor_size(int space) {
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

ImVec2 iam_tween_vec2_rel(ImGuiID id, ImGuiID channel_id, ImVec2 percent, ImVec2 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt) {
	ImVec2 base = iam_anchor_size(anchor_space);
	ImVec2 target = ImVec2(base.x*percent.x + px_bias.x, base.y*percent.y + px_bias.y);
	return iam_tween_vec2(id, channel_id, target, dur, ez, policy, dt);
}

ImVec2 iam_tween_vec2_resolved(ImGuiID id, ImGuiID channel_id, iam_vec2_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt) {
	ImVec2 target = fn ? fn(user) : ImVec2(0,0);
	return iam_tween_vec2(id, channel_id, target, dur, ez, policy, dt);
}

void iam_rebase_vec2(ImGuiID id, ImGuiID channel_id, ImVec2 new_target, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	int idx = iam_detail::g_vec2.pool.Map.GetInt(key, -1);
	if (idx == -1) return;
	iam_detail::vec2_chan* c = iam_detail::g_vec2.pool.GetByIndex(idx);
	if (c->t < 1.0f && dt > 0) c->tick(dt);
	float remain = (1.0f - (c->t < 1.0f ? c->t : 1.0f)) * c->dur;
	c->start = c->current;
	c->target = new_target;
	c->t = 0.0f;
	c->dur = (remain <= 1e-6f ? 1e-6f : remain);
}

// Additional resolved/rel/rebase functions for all types

float iam_tween_float_rel(ImGuiID id, ImGuiID channel_id, float percent, float px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, int axis, float dt) {
	ImVec2 base = iam_anchor_size(anchor_space);
	float target = (axis == 0 ? base.x : base.y) * percent + px_bias;
	return iam_tween_float(id, channel_id, target, dur, ez, policy, dt);
}

ImVec4 iam_tween_vec4_rel(ImGuiID id, ImGuiID channel_id, ImVec4 percent, ImVec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int anchor_space, float dt) {
	ImVec2 base = iam_anchor_size(anchor_space);
	ImVec4 target = ImVec4(base.x*percent.x + px_bias.x, base.y*percent.y + px_bias.y, percent.z + px_bias.z, percent.w + px_bias.w);
	return iam_tween_vec4(id, channel_id, target, dur, ez, policy, dt);
}

ImVec4 iam_tween_color_rel(ImGuiID id, ImGuiID channel_id, ImVec4 percent, ImVec4 px_bias, float dur, iam_ease_desc const& ez, int policy, int color_space, int anchor_space, float dt) {
	// For color, percent/px_bias represent color component modifiers (not spatial anchors)
	(void)anchor_space; // Color doesn't use spatial anchors
	ImVec4 target = ImVec4(percent.x + px_bias.x, percent.y + px_bias.y, percent.z + px_bias.z, percent.w + px_bias.w);
	return iam_tween_color(id, channel_id, target, dur, ez, policy, color_space, dt);
}

float iam_tween_float_resolved(ImGuiID id, ImGuiID channel_id, iam_float_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt) {
	float target = fn ? fn(user) : 0.0f;
	return iam_tween_float(id, channel_id, target, dur, ez, policy, dt);
}

ImVec4 iam_tween_vec4_resolved(ImGuiID id, ImGuiID channel_id, iam_vec4_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, float dt) {
	ImVec4 target = fn ? fn(user) : ImVec4(0,0,0,0);
	return iam_tween_vec4(id, channel_id, target, dur, ez, policy, dt);
}

ImVec4 iam_tween_color_resolved(ImGuiID id, ImGuiID channel_id, iam_color_resolver fn, void* user, float dur, iam_ease_desc const& ez, int policy, int color_space, float dt) {
	ImVec4 target = fn ? fn(user) : ImVec4(0,0,0,1);
	return iam_tween_color(id, channel_id, target, dur, ez, policy, color_space, dt);
}

void iam_rebase_float(ImGuiID id, ImGuiID channel_id, float new_target, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	int idx = iam_detail::g_float.pool.Map.GetInt(key, -1);
	if (idx == -1) return;
	iam_detail::float_chan* c = iam_detail::g_float.pool.GetByIndex(idx);
	if (c->t < 1.0f && dt > 0) c->tick(dt);
	float remain = (1.0f - (c->t < 1.0f ? c->t : 1.0f)) * c->dur;
	c->start = c->current;
	c->target = new_target;
	c->t = 0.0f;
	c->dur = (remain <= 1e-6f ? 1e-6f : remain);
}

void iam_rebase_vec4(ImGuiID id, ImGuiID channel_id, ImVec4 new_target, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	int idx = iam_detail::g_vec4.pool.Map.GetInt(key, -1);
	if (idx == -1) return;
	iam_detail::vec4_chan* c = iam_detail::g_vec4.pool.GetByIndex(idx);
	if (c->t < 1.0f && dt > 0) c->tick(dt);
	float remain = (1.0f - (c->t < 1.0f ? c->t : 1.0f)) * c->dur;
	c->start = c->current;
	c->target = new_target;
	c->t = 0.0f;
	c->dur = (remain <= 1e-6f ? 1e-6f : remain);
}

void iam_rebase_color(ImGuiID id, ImGuiID channel_id, ImVec4 new_target, float dt) {
	ImGuiID key = iam_detail::make_key(id, channel_id);
	int idx = iam_detail::g_color.pool.Map.GetInt(key, -1);
	if (idx == -1) return;
	iam_detail::color_chan* c = iam_detail::g_color.pool.GetByIndex(idx);
	if (c->t < 1.0f && dt > 0) c->tick(dt);
	float remain = (1.0f - (c->t < 1.0f ? c->t : 1.0f)) * c->dur;
	c->start = c->current;
	c->target = new_target;
	c->t = 0.0f;
	c->dur = (remain <= 1e-6f ? 1e-6f : remain);
}

// ============================================================
// CLIP-BASED ANIMATION SYSTEM IMPLEMENTATION
// ============================================================

namespace iam_clip_detail {

// keyframe data - using plain floats to avoid union constructor issues
struct keyframe {
	ImGuiID		channel;
	float		time;
	int			type;		// iam_channel_type
	int			ease_type;	// iam_ease_type
	float		bezier[4];
	bool		has_bezier;
	bool		is_spring;
	iam_spring_params spring;
	float		value[4];	// f=value[0], v2=(value[0],value[1]), v4=(value[0..3]), i=*(int*)&value[0]

	keyframe() : channel(0), time(0), type(0), ease_type(iam_ease_linear), has_bezier(false), is_spring(false) {
		bezier[0] = bezier[1] = bezier[2] = bezier[3] = 0;
		spring = { 1.0f, 120.0f, 20.0f, 0.0f };
		value[0] = value[1] = value[2] = value[3] = 0;
	}

	void set_float(float f) { value[0] = f; }
	float get_float() const { return value[0]; }
	void set_vec2(ImVec2 v) { value[0] = v.x; value[1] = v.y; }
	ImVec2 get_vec2() const { return ImVec2(value[0], value[1]); }
	void set_vec4(ImVec4 v) { value[0] = v.x; value[1] = v.y; value[2] = v.z; value[3] = v.w; }
	ImVec4 get_vec4() const { return ImVec4(value[0], value[1], value[2], value[3]); }
	void set_int(int i) { *(int*)&value[0] = i; }
	int get_int() const { return *(int*)&value[0]; }
};

// iam_track: sorted keyframes for a single channel
struct iam_track {
	ImGuiID				channel;
	int					type;
	ImVector<keyframe>	keys;

	iam_track() : channel(0), type(0) {}
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

	// Callbacks
	iam_clip_callback		cb_begin;
	iam_clip_callback		cb_update;
	iam_clip_callback		cb_complete;
	void*					cb_begin_user;
	void*					cb_update_user;
	void*					cb_complete_user;

	// Build-time state
	ImVector<iam_clip_detail::keyframe>	build_keys;

	iam_clip_data() : id(0), delay(0), duration(0), loop_count(0), direction(iam_dir_normal),
		cb_begin(nullptr), cb_update(nullptr), cb_complete(nullptr),
		cb_begin_user(nullptr), cb_update_user(nullptr), cb_complete_user(nullptr) {}
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
	// vec2/vec4 stored as float pairs/quads in separate arrays
	struct vec2_entry { ImGuiID ch; ImVec2 v; };
	struct vec4_entry { ImGuiID ch; ImVec4 v; };
	ImVector<vec2_entry> values_vec2;
	ImVector<vec4_entry> values_vec4;

	iam_instance_data() : inst_id(0), clip_id(0), time(0), time_scale(1.0f), weight(1.0f),
		delay_left(0), playing(false), paused(false), begin_called(false), dir_sign(1), loops_left(0), last_seen_frame(0) {}
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
	if (has_bezier && ease_type == iam_ease_cubic_bezier) {
		iam_ease_desc d = { ease_type, bezier[0], bezier[1], bezier[2], bezier[3] };
		return iam_detail::eval(d, t);
	}
	return iam_detail::eval_preset_internal(ease_type, t);
}

// Evaluate spring
static float eval_clip_spring(float u, iam_spring_params const& sp) {
	return iam_detail::ease_lut_pool_singleton().spring_unit(u, sp.mass, sp.stiffness, sp.damping, sp.initial_velocity);
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

	switch (trk.type) {
		case iam_chan_float: {
			float a = k0->get_float(), b = k1->get_float();
			float v = a + (b - a) * w;
			inst->values_float.SetFloat(trk.channel, v);
			break;
		}
		case iam_chan_vec2: {
			ImVec2 a = k0->get_vec2(), b = k1->get_vec2();
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
			int v = (int)(a + (int)((float)(b - a) * w + 0.5f));
			inst->values_int.SetInt(trk.channel, v);
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
	clip->duration = 0;
	clip->delay = 0;
	clip->loop_count = 0;
	clip->direction = iam_dir_normal;

	return iam_clip(clip_id);
}

iam_clip& iam_clip::key_float(ImGuiID channel, float time, float value, int ease_type, float const* bezier4) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = time;
	k.type = iam_chan_float;
	k.ease_type = ease_type;
	k.set_float(value);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (time > clip->duration) clip->duration = time;
	return *this;
}

iam_clip& iam_clip::key_vec2(ImGuiID channel, float time, ImVec2 value, int ease_type, float const* bezier4) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = time;
	k.type = iam_chan_vec2;
	k.ease_type = ease_type;
	k.set_vec2(value);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (time > clip->duration) clip->duration = time;
	return *this;
}

iam_clip& iam_clip::key_vec4(ImGuiID channel, float time, ImVec4 value, int ease_type, float const* bezier4) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = time;
	k.type = iam_chan_vec4;
	k.ease_type = ease_type;
	k.set_vec4(value);
	if (bezier4) {
		k.has_bezier = true;
		k.bezier[0] = bezier4[0]; k.bezier[1] = bezier4[1];
		k.bezier[2] = bezier4[2]; k.bezier[3] = bezier4[3];
	}
	clip->build_keys.push_back(k);
	if (time > clip->duration) clip->duration = time;
	return *this;
}

iam_clip& iam_clip::key_int(ImGuiID channel, float time, int value, int ease_type) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = time;
	k.type = iam_chan_int;
	k.ease_type = ease_type;
	k.set_int(value);
	clip->build_keys.push_back(k);
	if (time > clip->duration) clip->duration = time;
	return *this;
}

iam_clip& iam_clip::key_float_spring(ImGuiID channel, float time, float target, iam_spring_params const& spring) {
	iam_clip_data* clip = get_clip_data(m_clip_id);
	if (!clip) return *this;
	iam_clip_detail::keyframe k;
	k.channel = channel;
	k.time = time;
	k.type = iam_chan_float;
	k.ease_type = iam_ease_spring;
	k.is_spring = true;
	k.spring = spring;
	k.set_float(target);
	clip->build_keys.push_back(k);
	if (time > clip->duration) clip->duration = time;
	return *this;
}

iam_clip& iam_clip::seq_begin() { return *this; }  // Placeholder for future timeline features
iam_clip& iam_clip::seq_end() { return *this; }
iam_clip& iam_clip::par_begin() { return *this; }
iam_clip& iam_clip::par_end() { return *this; }

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
	(void)count; (void)each_delay; (void)from_center_bias;
	// Placeholder for future stagger implementation
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
		}

		trk->keys.push_back(k);
	}

	// Clear build data
	clip->build_keys.clear();
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
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) inst->paused = true;
}

void iam_instance::resume() {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) inst->paused = false;
}

void iam_instance::stop() {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) { inst->playing = false; inst->time = 0; }
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
}

void iam_instance::set_time_scale(float scale) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) inst->time_scale = scale;
}

void iam_instance::set_weight(float weight) {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (inst) inst->weight = weight;
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
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst || !out) return false;
	*out = inst->values_float.GetFloat(channel, 0.0f);
	return true;
}

bool iam_instance::get_vec2(ImGuiID channel, ImVec2* out) const {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst || !out) return false;
	for (int i = 0; i < inst->values_vec2.Size; ++i) {
		if (inst->values_vec2[i].ch == channel) {
			*out = inst->values_vec2[i].v;
			return true;
		}
	}
	*out = ImVec2(0, 0);
	return false;
}

bool iam_instance::get_vec4(ImGuiID channel, ImVec4* out) const {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst || !out) return false;
	for (int i = 0; i < inst->values_vec4.Size; ++i) {
		if (inst->values_vec4[i].ch == channel) {
			*out = inst->values_vec4[i].v;
			return true;
		}
	}
	*out = ImVec4(0, 0, 0, 0);
	return false;
}

bool iam_instance::get_int(ImGuiID channel, int* out) const {
	iam_instance_data* inst = get_instance_data(m_inst_id);
	if (!inst || !out) return false;
	*out = inst->values_int.GetInt(channel, 0);
	return true;
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

void iam_clip_shutdown() {
	using namespace iam_clip_detail;
	g_clip_sys.clips.clear();
	g_clip_sys.instances.clear();
	g_clip_sys.clip_map.Clear();
	g_clip_sys.inst_map.Clear();
	g_clip_sys.initialized = false;
}

void iam_clip_update(float dt) {
	using namespace iam_clip_detail;
	g_clip_sys.frame_counter++;

	// Safety: clamp dt to reasonable range
	if (dt < 0.0f) dt = 0.0f;
	if (dt > 1.0f) dt = 1.0f;

	for (int i = 0; i < g_clip_sys.instances.Size; ++i) {
		iam_instance_data* inst = &g_clip_sys.instances[i];
		iam_clip_data* clip = find_clip(inst->clip_id);
		if (!inst->playing || inst->paused || !clip) continue;

		// Use local copy of dt for this instance to avoid affecting other instances
		float inst_dt = dt;

		// Handle delay
		if (inst->delay_left > 0.0f) {
			inst->delay_left -= inst_dt;
			if (inst->delay_left > 0.0f) continue;
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

		float dur = clip->duration;
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
		} else if (clip->direction == iam_dir_reverse) {
			while (t < 0.0f && loop_iters < MAX_LOOP_ITERS) {
				if (clip->loop_count == 0 && inst->loops_left == 0) { done = true; break; }
				if (inst->loops_left > 0) inst->loops_left--;
				t += dur;
				loop_iters++;
			}
			while (t > dur && loop_iters < MAX_LOOP_ITERS) { t -= dur; loop_iters++; }
		} else { // normal
			while (t > dur && loop_iters < MAX_LOOP_ITERS) {
				if (clip->loop_count == 0 && inst->loops_left == 0) { done = true; break; }
				if (inst->loops_left > 0) inst->loops_left--;
				t -= dur;
				loop_iters++;
			}
			while (t < 0.0f && loop_iters < MAX_LOOP_ITERS) { t += dur; loop_iters++; }
		}
		// Safety clamp
		if (t < 0.0f) t = 0.0f;
		if (t > dur) t = dur;

		if (done) {
			inst->playing = false;
			inst->time = (inst->dir_sign > 0) ? dur : 0.0f;
			// Evaluate final frame
			for (int tr = 0; tr < clip->iam_tracks.Size; ++tr) {
				eval_iam_track(clip->iam_tracks[tr], inst->time, inst);
			}
			if (clip->cb_complete)
				clip->cb_complete(inst->inst_id, clip->cb_complete_user);
			continue;
		}

		inst->time = t;

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
	inst->loops_left = clip->loop_count;
	inst->last_seen_frame = g_clip_sys.frame_counter;

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

// Layering support (placeholder)
static struct {
	ImGuiID instance_id;
	float acc_weight;
} g_layer_state = { 0, 0 };

void iam_layer_begin(ImGuiID instance_id) {
	g_layer_state.instance_id = instance_id;
	g_layer_state.acc_weight = 0;
}

void iam_layer_add(iam_instance inst, float weight) {
	(void)inst; (void)weight;
	g_layer_state.acc_weight += weight;
}

void iam_layer_end(ImGuiID instance_id) {
	(void)instance_id;
}

// Persistence (placeholder)
iam_result iam_clip_save(ImGuiID clip_id, char const* path) {
	(void)clip_id; (void)path;
	return iam_err_not_found;  // Not implemented yet
}

iam_result iam_clip_load(char const* path, ImGuiID* out_clip_id) {
	(void)path; (void)out_clip_id;
	return iam_err_not_found;  // Not implemented yet
}
