// im_anim_lottie.cpp -- Lottie player for ImAnim (see im_anim_lottie.h).
//
// The polygon fill (non-zero / even-odd, anti-aliased trapezoid sweep), the path clip
// stack and the gradient paint are adapted from DearWidgets (dear_widgets_path.cpp, CC0).

#include "im_anim_lottie.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Optional GPU fill through ImPlatform's custom shaders (the D3D backends take the large
// constant buffer it needs; anything else keeps the CPU fill)
#ifdef IAM_LOTTIE_IMPLATFORM
#include "ImPlatform.h"
#if defined(IM_CURRENT_GFX) && IMPLATFORM_GFX_SUPPORT_CUSTOM_SHADER && \
	((IM_CURRENT_GFX == IM_GFX_DIRECTX10) || (IM_CURRENT_GFX == IM_GFX_DIRECTX11) || (IM_CURRENT_GFX == IM_GFX_DIRECTX12))
#define IAM_LOTTIE_GPU 1
#endif
#endif
#ifndef IAM_LOTTIE_GPU
#define IAM_LOTTIE_GPU 0
#endif
// Offscreen layers (blend modes, luma mattes) need render textures and immediate texture
// binding: DX10 / DX11
#if IAM_LOTTIE_GPU && ((IM_CURRENT_GFX == IM_GFX_DIRECTX10) || (IM_CURRENT_GFX == IM_GFX_DIRECTX11))
#define IAM_LOTTIE_OFFSCREEN 1
#else
#define IAM_LOTTIE_OFFSCREEN 0
#endif

namespace iam_lottie_detail {

// ----------------------------------------------------------------------------
// JSON (DOM over the source text; strings keep their escapes)
// ----------------------------------------------------------------------------

enum { J_NULL, J_FALSE, J_TRUE, J_NUM, J_STR, J_ARR, J_OBJ };
struct jnode { int type, next, child, key, key_len, str, str_len; double num; };

struct jdoc {
	char const* text;
	ImVector<jnode> n;

	int get(int obj, char const* key) const {
		if (obj < 0 || n[obj].type != J_OBJ) return -1;
		int const len = (int)strlen(key);
		for (int c = n[obj].child; c >= 0; c = n[c].next)
			if (n[c].key_len == len && memcmp(text + n[c].key, key, (size_t)len) == 0) return c;
		return -1;
	}
	int type(int i) const { return i < 0 ? -1 : n[i].type; }
	double num(int i, double def) const {
		if (i < 0) return def;
		switch (n[i].type) {
			case J_NUM: return n[i].num;
			case J_TRUE: return 1.0;
			case J_FALSE: return 0.0;
			case J_ARR: return n[i].child >= 0 ? num(n[i].child, def) : def;  // [v] for scalars
			default: return def;
		}
	}
	double num(int obj, char const* key, double def) const { return num(get(obj, key), def); }
	int count(int arr) const { int k = 0; if (arr >= 0) for (int c = n[arr].child; c >= 0; c = n[c].next) k++; return k; }
	bool str_eq(int i, char const* s) const {
		if (i < 0 || n[i].type != J_STR) return false;
		int const len = (int)strlen(s);
		return n[i].str_len == len && memcmp(text + n[i].str, s, (size_t)len) == 0;
	}
};

struct jparser {
	char const* base;
	char const* s;
	char const* end;
	jdoc* doc;

	void ws() { while (s < end && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')) ++s; }
	bool lit(char const* w) {
		size_t const len = strlen(w);
		if ((size_t)(end - s) < len || memcmp(s, w, len) != 0) return false;
		s += len;
		return true;
	}
	bool str(int& ofs, int& len) {
		if (s >= end || *s != '"') return false;
		char const* b = ++s;
		while (s < end && *s != '"') { if (*s == '\\') ++s; ++s; }
		if (s >= end) return false;
		ofs = (int)(b - base); len = (int)(s - b);
		++s;
		return true;
	}
	// Locale-independent (strtod would read "1.5" as 1 under a comma-decimal locale)
	bool number(double& out) {
		char const* p = s;
		double sign = 1.0;
		if (p < end && *p == '-') { sign = -1.0; ++p; }
		bool digits = false;
		double v = 0.0;
		while (p < end && *p >= '0' && *p <= '9') { v = v * 10.0 + (*p++ - '0'); digits = true; }
		if (p < end && *p == '.') {
			++p;
			double f = 0.1;
			while (p < end && *p >= '0' && *p <= '9') { v += (*p++ - '0') * f; f *= 0.1; digits = true; }
		}
		if (!digits) return false;
		if (p < end && (*p == 'e' || *p == 'E')) {
			++p;
			int es = 1, e = 0;
			if (p < end && (*p == '+' || *p == '-')) { if (*p == '-') es = -1; ++p; }
			while (p < end && *p >= '0' && *p <= '9') { if (e < 400) e = e * 10 + (*p - '0'); ++p; }
			v *= pow(10.0, (double)(es * e));
		}
		out = sign * v;
		s = p;
		return true;
	}
	int value(int depth) {
		if (depth > 256) return -1;  // untrusted input: bound the recursion
		ws();
		if (s >= end) return -1;
		int const idx = doc->n.Size;
		jnode nd = { J_NULL, -1, -1, 0, 0, 0, 0, 0.0 };
		doc->n.push_back(nd);
		char const c = *s;
		if (c == '{' || c == '[') {
			bool const obj = (c == '{');
			char const close = obj ? '}' : ']';
			doc->n[idx].type = obj ? J_OBJ : J_ARR;
			++s;
			ws();
			if (s < end && *s == close) { ++s; return idx; }
			int last = -1;
			for (;;) {
				int ko = 0, kl = 0;
				if (obj) {
					ws();
					if (!str(ko, kl)) return -1;
					ws();
					if (s >= end || *s != ':') return -1;
					++s;
				}
				int const ch = value(depth + 1);
				if (ch < 0) return -1;
				doc->n[ch].key = ko; doc->n[ch].key_len = kl;
				if (last < 0) doc->n[idx].child = ch; else doc->n[last].next = ch;
				last = ch;
				ws();
				if (s >= end) return -1;
				if (*s == ',') { ++s; continue; }
				if (*s == close) { ++s; return idx; }
				return -1;
			}
		}
		if (c == '"') {
			doc->n[idx].type = J_STR;
			return str(doc->n[idx].str, doc->n[idx].str_len) ? idx : -1;
		}
		if (lit("true")) { doc->n[idx].type = J_TRUE; return idx; }
		if (lit("false")) { doc->n[idx].type = J_FALSE; return idx; }
		if (lit("null")) return idx;
		double v;
		if (!number(v)) return -1;
		doc->n[idx].type = J_NUM;
		doc->n[idx].num = v;
		return idx;
	}
};

// ----------------------------------------------------------------------------
// Model (flat POD arrays, indices between them)
// ----------------------------------------------------------------------------

struct keyframe_t {
	float t;
	int s, sn, e, en;          // start / end values in floats[] (e is the old-format explicit end)
	int hold, nease, spatial;
	float ox[3], oy[3], ix[3], iy[3];
	float to[2], ti[2];
};
struct prop_t { int v, vn, k, kn; };  // static value floats[v, v+vn) or keys[k, k+kn); both 0 = absent
struct xform_t { prop_t a, p, px, py, pz, s, r, o, sk, sa, so, eo, rx, ry, orient; int split; };

enum {
	SK_NONE, SK_GROUP, SK_PATH, SK_RECT, SK_ELLIPSE, SK_STAR,
	SK_FILL, SK_STROKE, SK_GFILL, SK_GSTROKE, SK_TRIM, SK_ROUND, SK_REPEATER, SK_PUCKER, SK_ZIGZAG, SK_MERGE, SK_OFFSET, SK_TWIST
};
struct shape_t {
	int kind, hidden, dir, rule, cap, join, mode, gcount;
	float miter;
	prop_t p[7];
	int dash, dashn;           // dashes[]
	int child, childn;         // children[] (groups)
	xform_t tr;                // group / repeater transform
};
struct dash_t { int kind; prop_t v; };  // 0 dash, 1 gap, 2 offset
struct mask_t { char mode; int inv; prop_t pt, o; };
struct layer_t {
	int ty, ind, parent, hidden, td, tt, tp, matte, comp, ao, ddd, bm;
	float ip, op, st, sr;
	xform_t ks;
	int shapes, shapesn;       // children[]
	prop_t tm;
	float solid[3], sw, sh;
	int mask, maskn;
	int doc, docn;             // docs[] (text layers)
	int fx, fxn;               // effects[]
	float pw, ph;              // precomp layer size
	prop_t pe;                 // camera zoom
};
// Effect parameters in file order. 1 fill (v2 colour, v6 opacity 0..1), 2 tint (v0 black, v1 white,
// v2 amount %), 3 gaussian blur (v0 blurriness, v1 dimensions), 4 drop shadow (v0 colour, v1 opacity
// 0..255, v2 direction, v3 distance, v4 softness, v5 shadow only)
enum { FX_FILL = 1, FX_TINT, FX_BLUR, FX_SHADOW };
struct effect_t { int kind; prop_t v[8]; };
struct comp_t { int layer, layern, camera, has3d; };  // camera: layer index or -1
struct marker_t { int name; float t, dur; };
struct font_t { int name, family, style; float ascent; };  // names in strings[]; ascent in % of the size
struct glyph_t { unsigned cp; int family, style; float w; int shapes, shapesn; };  // outlines at size 100
struct textdoc_t {
	float t, size, tracking, lh, sw;
	float box[2], box_pos[2];  // paragraph text: box size (0 = point text) and top-left corner
	int text, font, justify, fill, stroke, stroke_over;
	float fc[3], sc[3];
};

enum {
	W_EXPR = 1 << 0, W_EFFECTS = 1 << 1, W_TEXT = 1 << 2, W_IMAGE = 1 << 3, W_BLEND = 1 << 4, W_3D = 1 << 5,
	W_MERGE = 1 << 6, W_MODIFIER = 1 << 7, W_LUMA = 1 << 8, W_HIGHLIGHT = 1 << 9, W_TEXT_ANIM = 1 << 10,
	W_MASK_MODE = 1 << 11, W_UNKNOWN_LAYER = 1 << 12
};

} // namespace iam_lottie_detail

using namespace iam_lottie_detail;

struct iam_lottie {
	float w, h, fr, ip, op;
	ImVector<float> floats;
	ImVector<keyframe_t> keys;
	ImVector<shape_t> shapes;
	ImVector<int> children;
	ImVector<dash_t> dashes;
	ImVector<mask_t> masks;
	ImVector<layer_t> layers;
	ImVector<comp_t> comps;        // [0] = root
	ImVector<marker_t> markers;
	ImVector<font_t> fonts;
	ImVector<glyph_t> glyphs;
	ImVector<textdoc_t> docs;
	ImVector<effect_t> effects;
	ImVector<char> strings;
	ImVector<char> warnings;
	unsigned warn;

	// Replay cache: a draw repeated with the same key (paused animation, idle widget) copies
	// the vertices it produced last time instead of evaluating and filling again
	struct draw_key { float frame, pos[2], size[2], uv[2], clip[4]; int gpu; };  // plain floats: compared bytewise
	struct draw_entry { draw_key key; unsigned stamp; bool valid; ImVector<ImDrawVert> vtx; ImVector<ImDrawIdx> idx; };
	enum { CACHE = 8 };
	mutable draw_entry cache[CACHE];
	mutable draw_key seen[CACHE];  // keys drawn once: stored when they come again
	mutable int seen_next;
	mutable unsigned stamp;
	mutable bool cache_enabled;
	bool frame_snap;
};

namespace iam_lottie_detail {

// ----------------------------------------------------------------------------
// Loading
// ----------------------------------------------------------------------------

struct builder {
	iam_lottie* L;
	jdoc const* D;
	ImVector<int> ref_layer, ref_str, ref_len;   // precomp layers waiting for their asset
	ImVector<int> comp_id, comp_id_len;
	int version[3];            // file "v"; older exports are upgraded like lottie-web does

	bool older_than(int a, int b, int c) const {
		if (version[0] != a) return version[0] < a;
		if (version[1] != b) return version[1] < b;
		return version[2] < c;
	}
	// Every value of a property (static, key starts and old-format key ends)
	template <typename F> void each_value(prop_t const& p, F f) {
		if (p.kn == 0) { if (p.vn) f(L->floats.Data + p.v, p.vn); return; }
		for (int i = 0; i < p.kn; i++) {
			keyframe_t const& k = L->keys[p.k + i];
			if (k.sn) f(L->floats.Data + k.s, k.sn);
			if (k.en) f(L->floats.Data + k.e, k.en);
		}
	}
	// Before 4.1.9 fill / stroke colours were 0..255
	prop_t color_prop(int node) {
		prop_t const p = prop(node);
		if (older_than(4, 1, 9)) each_value(p, [](float* v, int n) { for (int i = 0; i < n; i++) v[i] /= 255.0f; });
		return p;
	}
	// Before 4.4.18 the closed flag lived on the shape item ("closed") or mask ("cl")
	prop_t path_prop(int node, int owner, char const* closed_key) {
		prop_t const p = prop(node);
		if (older_than(4, 4, 18)) {
			float const closed = D->num(owner, closed_key, 0.0) != 0.0 ? 1.0f : 0.0f;
			each_value(p, [closed](float* v, int n) { if (n > 0) v[0] = closed; });
		}
		return p;
	}

	bool is_obj(int i) const { return D->type(i) == J_OBJ; }

	void flatten(int i) {
		switch (D->type(i)) {
			case J_NUM: case J_TRUE: case J_FALSE: L->floats.push_back((float)D->num(i, 0.0)); break;
			case J_ARR: for (int c = D->n[i].child; c >= 0; c = D->n[c].next) flatten(c); break;
			case J_OBJ: {
				// Bezier shape: [closed, n, (vx vy ix iy ox oy) * n]
				int const v = D->get(i, "v"), in = D->get(i, "i"), out = D->get(i, "o");
				if (v < 0) break;
				L->floats.push_back(D->num(i, "c", 0.0) != 0.0 ? 1.0f : 0.0f);
				L->floats.push_back((float)D->count(v));
				int ci = in >= 0 ? D->n[in].child : -1, co = out >= 0 ? D->n[out].child : -1;
				for (int cv = D->n[v].child; cv >= 0; cv = D->n[cv].next) {
					int const pts[3] = { cv, ci, co };
					for (int k = 0; k < 3; k++) {
						int const a = pts[k] >= 0 ? D->n[pts[k]].child : -1;
						int const b = a >= 0 ? D->n[a].next : -1;
						L->floats.push_back((float)D->num(a, 0.0));
						L->floats.push_back((float)D->num(b, 0.0));
					}
					if (ci >= 0) ci = D->n[ci].next;
					if (co >= 0) co = D->n[co].next;
				}
				break;
			}
			default: break;
		}
	}

	void read_ease(int node, float* x, float* y, int& n) {
		int const nx = D->get(node, "x"), ny = D->get(node, "y");
		int cnt = 1;
		if (D->type(nx) == J_ARR) cnt = ImClamp(D->count(nx), 1, 3);
		n = ImMax(n, cnt);
		for (int k = 0; k < 3; k++) {
			int ex = nx, ey = ny;
			if (D->type(nx) == J_ARR) { ex = D->n[nx].child; for (int j = 0; j < k && ex >= 0 && D->n[ex].next >= 0; j++) ex = D->n[ex].next; }
			if (D->type(ny) == J_ARR) { ey = D->n[ny].child; for (int j = 0; j < k && ey >= 0 && D->n[ey].next >= 0; j++) ey = D->n[ey].next; }
			x[k] = (float)D->num(ex, x[k]);
			y[k] = (float)D->num(ey, y[k]);
		}
	}

	prop_t prop(int node) {
		prop_t p = { L->floats.Size, 0, L->keys.Size, 0 };
		if (node < 0) return p;
		if (is_obj(node) && D->type(D->get(node, "x")) == J_STR) L->warn |= W_EXPR;
		int const k = is_obj(node) ? D->get(node, "k") : node;
		if (k < 0) return p;
		int const first = D->type(k) == J_ARR ? D->n[k].child : -1;
		if (first >= 0 && is_obj(first) && D->get(first, "t") >= 0) {
			for (int kf = first; kf >= 0; kf = D->n[kf].next) {
				keyframe_t key;
				memset(&key, 0, sizeof(key));
				key.t = (float)D->num(kf, "t", 0.0);
				key.s = L->floats.Size; flatten(D->get(kf, "s")); key.sn = L->floats.Size - key.s;
				key.e = L->floats.Size; flatten(D->get(kf, "e")); key.en = L->floats.Size - key.e;
				key.hold = D->num(kf, "h", 0.0) != 0.0;
				for (int d = 0; d < 3; d++) { key.ox[d] = 0.0f; key.oy[d] = 0.0f; key.ix[d] = 1.0f; key.iy[d] = 1.0f; }
				key.nease = 1;
				read_ease(D->get(kf, "o"), key.ox, key.oy, key.nease);
				read_ease(D->get(kf, "i"), key.ix, key.iy, key.nease);
				int const to = D->get(kf, "to"), ti = D->get(kf, "ti");
				if (to >= 0 && ti >= 0) {
					int const to1 = D->n[to].child, ti1 = D->n[ti].child;
					key.to[0] = (float)D->num(to1, 0.0); key.to[1] = (float)D->num(to1 >= 0 ? D->n[to1].next : -1, 0.0);
					key.ti[0] = (float)D->num(ti1, 0.0); key.ti[1] = (float)D->num(ti1 >= 0 ? D->n[ti1].next : -1, 0.0);
					key.spatial = key.to[0] != 0.0f || key.to[1] != 0.0f || key.ti[0] != 0.0f || key.ti[1] != 0.0f;
				}
				L->keys.push_back(key);
			}
			p.kn = L->keys.Size - p.k;
		} else {
			flatten(k);
			p.vn = L->floats.Size - p.v;
		}
		return p;
	}

	xform_t xform(int node) {
		xform_t x;
		memset(&x, 0, sizeof(x));
		x.a = prop(D->get(node, "a"));
		int const p = D->get(node, "p");
		if (is_obj(p) && D->num(p, "s", 0.0) != 0.0) {
			x.split = 1;
			x.px = prop(D->get(p, "x"));
			x.py = prop(D->get(p, "y"));
			x.pz = prop(D->get(p, "z"));
		} else {
			x.p = prop(p);
		}
		x.s = prop(D->get(node, "s"));
		x.r = prop(D->get(node, D->get(node, "r") >= 0 ? "r" : "rz"));
		x.o = prop(D->get(node, "o"));
		x.sk = prop(D->get(node, "sk"));
		x.sa = prop(D->get(node, "sa"));
		x.so = prop(D->get(node, "so"));
		x.eo = prop(D->get(node, "eo"));
		x.rx = prop(D->get(node, "rx"));
		x.ry = prop(D->get(node, "ry"));
		x.orient = prop(D->get(node, "or"));
		return x;
	}

	void dashes(int arr, shape_t& s) {
		s.dash = L->dashes.Size;
		if (D->type(arr) == J_ARR)
			for (int c = D->n[arr].child; c >= 0; c = D->n[c].next) {
				int const n = D->get(c, "n");
				dash_t d;
				d.kind = D->str_eq(n, "g") ? 1 : D->str_eq(n, "o") ? 2 : 0;
				d.v = prop(D->get(c, "v"));
				L->dashes.push_back(d);
			}
		s.dashn = L->dashes.Size - s.dash;
	}

	void stroke_style(int c, shape_t& s) {
		s.cap = (int)D->num(c, "lc", 2.0);
		s.join = (int)D->num(c, "lj", 2.0);
		s.miter = (float)D->num(c, "ml", 4.0);
		dashes(D->get(c, "d"), s);
	}

	// Parses a shape list; returns its range in children[]
	void items(int arr, int& ofs, int& cnt, int* tr_node) {
		ImVector<int> ids;
		if (D->type(arr) == J_ARR)
			for (int c = D->n[arr].child; c >= 0; c = D->n[c].next) {
				int const ty = D->get(c, "ty");
				if (D->str_eq(ty, "tr")) { if (tr_node) *tr_node = c; continue; }
				shape_t s;
				memset(&s, 0, sizeof(s));
				s.hidden = D->num(c, "hd", 0.0) != 0.0;
				s.dir = (int)D->num(c, "d", 1.0);
				if (D->str_eq(ty, "gr")) {
					s.kind = SK_GROUP;
					int tr = -1;
					items(D->get(c, "it"), s.child, s.childn, &tr);
					s.tr = xform(tr);
				} else if (D->str_eq(ty, "sh")) {
					s.kind = SK_PATH; s.p[0] = path_prop(D->get(c, "ks"), c, "closed");
				} else if (D->str_eq(ty, "rc")) {
					s.kind = SK_RECT; s.p[0] = prop(D->get(c, "p")); s.p[1] = prop(D->get(c, "s")); s.p[2] = prop(D->get(c, "r"));
				} else if (D->str_eq(ty, "el")) {
					s.kind = SK_ELLIPSE; s.p[0] = prop(D->get(c, "p")); s.p[1] = prop(D->get(c, "s"));
				} else if (D->str_eq(ty, "sr")) {
					s.kind = SK_STAR; s.mode = (int)D->num(c, "sy", 1.0);
					char const* const keys[7] = { "p", "or", "ir", "os", "is", "r", "pt" };
					for (int k = 0; k < 7; k++) s.p[k] = prop(D->get(c, keys[k]));
				} else if (D->str_eq(ty, "fl")) {
					s.kind = SK_FILL; s.p[0] = color_prop(D->get(c, "c")); s.p[1] = prop(D->get(c, "o")); s.rule = (int)D->num(c, "r", 1.0);
				} else if (D->str_eq(ty, "st")) {
					s.kind = SK_STROKE; s.p[0] = color_prop(D->get(c, "c")); s.p[1] = prop(D->get(c, "o")); s.p[2] = prop(D->get(c, "w"));
					stroke_style(c, s);
				} else if (D->str_eq(ty, "gf") || D->str_eq(ty, "gs")) {
					bool const stroke = D->str_eq(ty, "gs");
					s.kind = stroke ? SK_GSTROKE : SK_GFILL;
					s.p[0] = prop(D->get(c, "o")); s.p[1] = prop(D->get(c, "s")); s.p[2] = prop(D->get(c, "e"));
					int const g = D->get(c, "g");
					s.p[3] = prop(D->get(g, "k"));
					s.gcount = (int)D->num(g, "p", 0.0);
					s.mode = (int)D->num(c, "t", 1.0);
					s.rule = (int)D->num(c, "r", 1.0);
					s.p[5] = prop(D->get(c, "h"));  // radial highlight length (%) and angle
					s.p[6] = prop(D->get(c, "a"));
					if (stroke) { s.p[4] = prop(D->get(c, "w")); stroke_style(c, s); }
				} else if (D->str_eq(ty, "tm")) {
					s.kind = SK_TRIM; s.p[0] = prop(D->get(c, "s")); s.p[1] = prop(D->get(c, "e")); s.p[2] = prop(D->get(c, "o"));
					s.mode = (int)D->num(c, "m", 1.0);
				} else if (D->str_eq(ty, "rd")) {
					s.kind = SK_ROUND; s.p[0] = prop(D->get(c, "r"));
				} else if (D->str_eq(ty, "pb")) {
					s.kind = SK_PUCKER; s.p[0] = prop(D->get(c, "a"));
				} else if (D->str_eq(ty, "zz")) {
					s.kind = SK_ZIGZAG; s.p[0] = prop(D->get(c, "s")); s.p[1] = prop(D->get(c, "r")); s.p[2] = prop(D->get(c, "pt"));
				} else if (D->str_eq(ty, "rp")) {
					s.kind = SK_REPEATER; s.p[0] = prop(D->get(c, "c")); s.p[1] = prop(D->get(c, "o"));
					s.mode = (int)D->num(c, "m", 1.0);
					s.tr = xform(D->get(c, "tr"));
				} else if (D->str_eq(ty, "tw")) {
					s.kind = SK_TWIST; s.p[0] = prop(D->get(c, "a")); s.p[1] = prop(D->get(c, "c"));
				} else if (D->str_eq(ty, "op")) {
					s.kind = SK_OFFSET; s.p[0] = prop(D->get(c, "a"));
					s.join = (int)D->num(c, "lj", 1.0); s.miter = (float)D->num(c, "ml", 4.0);
				} else if (D->str_eq(ty, "mm")) {
					s.kind = SK_MERGE; s.mode = (int)D->num(c, "mm", 1.0);
				} else {
					L->warn |= W_MODIFIER;
					continue;
				}
				ids.push_back(L->shapes.Size);
				L->shapes.push_back(s);
			}
		ofs = L->children.Size;
		cnt = ids.Size;
		for (int i = 0; i < ids.Size; i++) L->children.push_back(ids[i]);
	}

	void layers(int arr) {
		comp_t cp = { L->layers.Size, 0, -1, 0 };
		if (D->type(arr) == J_ARR)
			for (int c = D->n[arr].child; c >= 0; c = D->n[c].next) {
				layer_t ly;
				memset(&ly, 0, sizeof(ly));
				ly.ty = (int)D->num(c, "ty", -1.0);
				ly.ind = (int)D->num(c, "ind", -1.0);
				ly.parent = (int)D->num(c, "parent", -1.0);  // ind for now, resolved below
				ly.ip = (float)D->num(c, "ip", 0.0);
				ly.op = (float)D->num(c, "op", 0.0);
				ly.st = (float)D->num(c, "st", 0.0);
				ly.sr = (float)D->num(c, "sr", 1.0);
				if (ly.sr == 0.0f) ly.sr = 1.0f;
				ly.hidden = D->num(c, "hd", 0.0) != 0.0;
				ly.td = D->num(c, "td", 0.0) != 0.0;
				ly.tt = (int)D->num(c, "tt", 0.0);
				ly.tp = (int)D->num(c, "tp", -1.0);
				ly.matte = -1;
				ly.comp = -1;
				ly.ks = xform(D->get(c, "ks"));
				ly.tm = prop(D->get(c, "tm"));
				if (ly.tt == 3 || ly.tt == 4) L->warn |= W_LUMA;
				ly.ddd = D->num(c, "ddd", 0.0) != 0.0;
				ly.pw = (float)D->num(c, "w", 0.0);
				ly.ph = (float)D->num(c, "h", 0.0);
				ly.bm = (int)D->num(c, "bm", 0.0);
				if (ly.bm != 0) L->warn |= W_BLEND;
				ly.ao = D->num(c, "ao", 0.0) != 0.0;
				effects(D->get(c, "ef"), ly);
				switch (ly.ty) {
					case 0: {
						int const ref = D->get(c, "refId");
						if (D->type(ref) == J_STR) {
							ref_layer.push_back(L->layers.Size);
							ref_str.push_back(D->n[ref].str);
							ref_len.push_back(D->n[ref].str_len);
						}
						break;
					}
					case 1: {
						int const sc = D->get(c, "sc");
						unsigned rgb = 0;
						if (D->type(sc) == J_STR && D->n[sc].str_len >= 7)
							for (int h = 1; h < 7; h++) {
								char const ch = D->text[D->n[sc].str + h];
								rgb = rgb * 16 + (unsigned)(ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : 0);
							}
						ly.solid[0] = ((rgb >> 16) & 0xFF) / 255.0f;
						ly.solid[1] = ((rgb >> 8) & 0xFF) / 255.0f;
						ly.solid[2] = (rgb & 0xFF) / 255.0f;
						ly.sw = (float)D->num(c, "sw", 0.0);
						ly.sh = (float)D->num(c, "sh", 0.0);
						break;
					}
					case 2: L->warn |= W_IMAGE; break;
					case 3: break;
					case 13: ly.pe = prop(D->get(c, "pe")); break;  // camera
					case 4: items(D->get(c, "shapes"), ly.shapes, ly.shapesn, nullptr); break;
					case 5: text(D->get(c, "t"), ly); break;
					default: L->warn |= W_UNKNOWN_LAYER; break;
				}
				int const mp = D->get(c, "masksProperties");
				ly.mask = L->masks.Size;
				if (D->type(mp) == J_ARR)
					for (int m = D->n[mp].child; m >= 0; m = D->n[m].next) {
						mask_t mk;
						int const mode = D->get(m, "mode");
						mk.mode = (D->type(mode) == J_STR && D->n[mode].str_len > 0) ? D->text[D->n[mode].str] : 'a';
						mk.inv = D->num(m, "inv", 0.0) != 0.0;
						mk.pt = path_prop(D->get(m, "pt"), m, "cl");
						mk.o = prop(D->get(m, "o"));
						if (mk.mode != 'a' && mk.mode != 's' && mk.mode != 'i' && mk.mode != 'n' && mk.mode != 'f') L->warn |= W_MASK_MODE;
						L->masks.push_back(mk);
					}
				ly.maskn = L->masks.Size - ly.mask;
				L->layers.push_back(ly);
			}
		cp.layern = L->layers.Size - cp.layer;
		for (int i = cp.layer; i < cp.layer + cp.layern; i++) {
			if (L->layers[i].ty == 13 && cp.camera < 0) cp.camera = i;  // the top camera (the search for the active one starts there)
			if (L->layers[i].ddd || L->layers[i].ty == 13) cp.has3d = 1;
		}
		// Parents and mattes refer to layers of the same composition
		for (int i = cp.layer; i < cp.layer + cp.layern; i++) {
			layer_t& ly = L->layers[i];
			int const parent_ind = ly.parent;
			ly.parent = -1;
			for (int j = cp.layer; j < cp.layer + cp.layern; j++) {
				if (j != i && parent_ind >= 0 && L->layers[j].ind == parent_ind) ly.parent = j;
				if (j != i && ly.tp >= 0 && L->layers[j].ind == ly.tp) ly.matte = j;
			}
			if (ly.tt != 0 && ly.matte < 0 && i > cp.layer && L->layers[i - 1].td) ly.matte = i - 1;
			if (ly.tt == 0) ly.matte = -1;
		}
		L->comps.push_back(cp);
	}

	int add_string(char const* s, int len) {
		int const ofs = L->strings.Size;
		for (int i = 0; i < len; i++) L->strings.push_back(s[i]);
		L->strings.push_back(0);
		return ofs;
	}

	static unsigned hex4(char const* c) {
		unsigned v = 0;
		for (int k = 0; k < 4; k++) { char const h = c[k]; v = v * 16 + (unsigned)(h >= '0' && h <= '9' ? h - '0' : h >= 'a' && h <= 'f' ? h - 'a' + 10 : h >= 'A' && h <= 'F' ? h - 'A' + 10 : 0); }
		return v;
	}

	// JSON string node -> UTF-8 in strings[] (escapes resolved)
	int add_json_string(int node) {
		int const ofs = L->strings.Size;
		if (D->type(node) == J_STR) {
			char const* c = D->text + D->n[node].str;
			char const* const e = c + D->n[node].str_len;
			while (c < e) {
				if (*c != '\\' || c + 1 >= e) { L->strings.push_back(*c++); continue; }
				char const esc = c[1];
				c += 2;
				unsigned cp = (unsigned char)esc;  // \" \\ \/
				if (esc == 'n') cp = '\n';
				else if (esc == 'r') cp = '\r';
				else if (esc == 't') cp = '\t';
				else if (esc == 'b') cp = '\b';
				else if (esc == 'f') cp = '\f';
				else if (esc == 'u') {
					if (e - c < 4) break;
					cp = hex4(c);
					c += 4;
					if (cp >= 0xD800 && cp < 0xDC00 && e - c >= 6 && c[0] == '\\' && c[1] == 'u') {  // surrogate pair
						unsigned const lo = hex4(c + 2);
						if (lo >= 0xDC00 && lo < 0xE000) { cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00); c += 6; }
					}
				}
				char u[5];
				int const n = ImTextCharToUtf8(u, cp);
				for (int k = 0; k < n; k++) L->strings.push_back(u[k]);
			}
		}
		L->strings.push_back(0);
		return ofs;
	}

	void color3(int node, float* out) {
		for (int k = 0; k < 3; k++) out[k] = 0.0f;
		int c = D->type(node) == J_ARR ? D->n[node].child : -1;
		bool bytes = false;
		for (int k = 0; k < 3 && c >= 0; k++, c = D->n[c].next) { out[k] = (float)D->num(c, 0.0); if (out[k] > 1.0f) bytes = true; }
		if (bytes) for (int k = 0; k < 3; k++) out[k] /= 255.0f;  // some exporters write 0..255
	}

	// Layer effects: Fill and Tint recolour the layer; expression controls (sliders, ...)
	// draw nothing; anything else is reported
	void effects(int arr, layer_t& ly) {
		ly.fx = L->effects.Size;
		if (D->type(arr) == J_ARR)
			for (int e = D->n[arr].child; e >= 0; e = D->n[e].next) {
				if (D->num(e, "en", 1.0) == 0.0) continue;  // disabled
				int const ty = (int)D->num(e, "ty", 0.0);
				int const mn = D->get(e, "mn");
				if (ty == 5 && D->type(mn) == J_STR) {
					char const* m = D->text + D->n[mn].str;
					int const len = D->n[mn].str_len;
					if (len > 13 && memcmp(m, "ADBE ", 5) == 0 && memcmp(m + len - 8, " Control", 8) == 0) continue;
				}
				int params[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
				int np = 0;
				int const pe = D->get(e, "ef");
				if (D->type(pe) == J_ARR)
					for (int q = D->n[pe].child; q >= 0 && np < 8; q = D->n[q].next) params[np++] = D->get(q, "v");
				effect_t fx;
				memset(&fx, 0, sizeof(fx));
				fx.kind = ty == 21 && np >= 7 ? FX_FILL : ty == 20 && np >= 3 ? FX_TINT : ty == 29 && np >= 1 ? FX_BLUR : ty == 25 && np >= 5 ? FX_SHADOW : 0;
				if (fx.kind == 0) { L->warn |= W_EFFECTS; continue; }
				for (int q = 0; q < np; q++) fx.v[q] = prop(params[q]);
				L->effects.push_back(fx);
			}
		ly.fxn = L->effects.Size - ly.fx;
	}

	// Text layer: the document keyframes (text, font, size, colours); animators are not supported
	void text(int t, layer_t& ly) {
		ly.doc = L->docs.Size;
		int const dn = D->get(t, "d");
		int const k = D->get(dn, "k");
		bool const single = older_than(4, 4, 14) && D->get(dn, "t") >= 0;  // before 4.4.14: one document, not keyframes
		if (D->type(k) == J_ARR || single)
			for (int kf = single ? dn : D->n[k].child; kf >= 0; kf = single ? -1 : D->n[kf].next) {
				int const sd = single ? dn : D->get(kf, "s");
				textdoc_t d;
				memset(&d, 0, sizeof(d));
				d.t = (float)D->num(kf, "t", 0.0);
				d.size = (float)D->num(sd, "s", 10.0);
				d.tracking = (float)D->num(sd, "tr", 0.0);
				d.lh = (float)D->num(sd, "lh", d.size * 1.2);
				d.justify = (int)D->num(sd, "j", 0.0);
				auto pair = [&](char const* key, float* out) {
					int const arr = D->get(sd, key);
					int i = 0;
					for (int c = D->type(arr) == J_ARR ? D->n[arr].child : -1; c >= 0 && i < 2; c = D->n[c].next) out[i++] = (float)D->num(c, 0.0);
				};
				pair("sz", d.box);
				pair("ps", d.box_pos);
				d.text = add_json_string(D->get(sd, "t"));
				d.font = -1;
				int const f = D->get(sd, "f");
				for (int i = 0; i < L->fonts.Size; i++)
					if (D->type(f) == J_STR && strlen(L->strings.Data + L->fonts[i].name) == (size_t)D->n[f].str_len &&
					    memcmp(L->strings.Data + L->fonts[i].name, D->text + D->n[f].str, (size_t)D->n[f].str_len) == 0) d.font = i;
				d.fill = D->get(sd, "fc") >= 0;
				d.stroke = D->get(sd, "sc") >= 0 && D->num(sd, "sw", 0.0) > 0.0;
				color3(D->get(sd, "fc"), d.fc);
				color3(D->get(sd, "sc"), d.sc);
				d.sw = (float)D->num(sd, "sw", 0.0);
				d.stroke_over = D->num(sd, "of", 0.0) != 0.0;
				L->docs.push_back(d);
			}
		ly.docn = L->docs.Size - ly.doc;
		if (D->count(D->get(t, "a")) > 0) L->warn |= W_TEXT_ANIM;
	}

	void fonts_and_chars(int root) {
		int const list = D->get(D->get(root, "fonts"), "list");
		if (D->type(list) == J_ARR)
			for (int f = D->n[list].child; f >= 0; f = D->n[f].next) {
				font_t ft;
				ft.name = add_json_string(D->get(f, "fName"));
				ft.family = add_json_string(D->get(f, "fFamily"));
				ft.style = add_json_string(D->get(f, "fStyle"));
				ft.ascent = (float)D->num(f, "ascent", 75.0);
				L->fonts.push_back(ft);
			}
		int const chars = D->get(root, "chars");
		if (D->type(chars) == J_ARR)
			for (int c = D->n[chars].child; c >= 0; c = D->n[c].next) {
				glyph_t g;
				int const ch = add_json_string(D->get(c, "ch"));
				g.cp = 0;
				ImTextCharFromUtf8(&g.cp, L->strings.Data + ch, L->strings.Data + ch + strlen(L->strings.Data + ch));
				g.family = add_json_string(D->get(c, "fFamily"));
				g.style = add_json_string(D->get(c, "style"));
				g.w = (float)D->num(c, "w", 0.0);
				items(D->get(D->get(c, "data"), "shapes"), g.shapes, g.shapesn, nullptr);
				L->glyphs.push_back(g);
			}
	}
};

static void build_warnings(iam_lottie* L) {
	static char const* const names[] = {
		"expressions", "effects", "text without embedded glyphs", "image layers", "blend modes (GPU path only)", "3D layers",
		"merge paths", "path modifiers", "luma mattes (GPU path only, else alpha)", "gradient highlights", "text animators",
		"mask modes", "unknown layer types"
	};
	for (int i = 0; i < IM_ARRAYSIZE(names); i++)
		if (L->warn & (1u << i)) {
			if (L->warnings.Size) { L->warnings.push_back(','); L->warnings.push_back(' '); }
			for (char const* c = names[i]; *c; c++) L->warnings.push_back(*c);
		}
	L->warnings.push_back(0);
}

// ----------------------------------------------------------------------------
// Evaluation
// ----------------------------------------------------------------------------

static inline float bez1(float a, float b, float t) { float const u = 1.0f - t; return 3.0f * a * u * u * t + 3.0f * b * u * t * t + t * t * t; }

// CSS-style cubic-bezier(x1, y1, x2, y2) at x
static float ease_bezier(float x1, float y1, float x2, float y2, float x) {
	if (x <= 0.0f) return 0.0f;
	if (x >= 1.0f) return 1.0f;
	if (x1 == y1 && x2 == y2) return x;
	x1 = ImSaturate(x1); x2 = ImSaturate(x2);
	float t = x;
	for (int i = 0; i < 8; i++) {
		float const err = bez1(x1, x2, t) - x;
		if (fabsf(err) < 1e-6f) return bez1(y1, y2, t);
		float const u = 1.0f - t;
		float const d = 3.0f * x1 * u * u + 6.0f * (x2 - x1) * u * t + 3.0f * (1.0f - x2) * t * t;
		if (fabsf(d) < 1e-6f) break;
		t -= err / d;
		if (t < 0.0f || t > 1.0f) break;
	}
	float lo = 0.0f, hi = 1.0f;
	t = x;
	for (int i = 0; i < 30; i++) {
		if (bez1(x1, x2, t) < x) lo = t; else hi = t;
		t = (lo + hi) * 0.5f;
	}
	return bez1(y1, y2, t);
}

static void eval(iam_lottie const* L, prop_t const& p, float t, ImVector<float>& out) {
	if (p.kn == 0) {
		out.resize(p.vn);
		if (p.vn) memcpy(out.Data, L->floats.Data + p.v, sizeof(float) * (size_t)p.vn);
		return;
	}
	keyframe_t const* K = L->keys.Data + p.k;
	int const n = p.kn;
	int k = 0;
	while (k + 1 < n && K[k + 1].t <= t) k++;
	int a = K[k].s, an = K[k].sn;
	if (an == 0 && k > 0) { a = K[k - 1].en ? K[k - 1].e : K[k - 1].s; an = K[k - 1].en ? K[k - 1].en : K[k - 1].sn; }
	out.resize(an);
	if (an) memcpy(out.Data, L->floats.Data + a, sizeof(float) * (size_t)an);
	if (t < K[k].t || k + 1 >= n || K[k].hold || an == 0) return;
	int const b = K[k].en ? K[k].e : K[k + 1].s;
	int const bn = K[k].en ? K[k].en : K[k + 1].sn;
	if (bn != an) return;  // mismatched shapes / gradients: hold
	float const span = K[k + 1].t - K[k].t;
	float const u = span > 0.0f ? (t - K[k].t) / span : 1.0f;
	float const* A = L->floats.Data + a;
	float const* B = L->floats.Data + b;
	for (int d = 0; d < an; d++) {
		int const e = ImMin(d, K[k].nease - 1);
		float const w = ease_bezier(K[k].ox[e], K[k].oy[e], K[k].ix[e], K[k].iy[e], u);
		out[d] = A[d] + (B[d] - A[d]) * w;
	}
	if (K[k].spatial && an >= 2) {
		// Curved motion path: constant speed along the cubic (arc length lookup)
		ImVec2 const p0(A[0], A[1]), p3(B[0], B[1]);
		ImVec2 const p1(A[0] + K[k].to[0], A[1] + K[k].to[1]), p2(B[0] + K[k].ti[0], B[1] + K[k].ti[1]);
		float const w = ease_bezier(K[k].ox[0], K[k].oy[0], K[k].ix[0], K[k].iy[0], u);
		enum { N = 32 };
		float len[N + 1];
		len[0] = 0.0f;
		ImVec2 prev = p0;
		for (int i = 1; i <= N; i++) {
			ImVec2 const q = ImBezierCubicCalc(p0, p1, p2, p3, (float)i / N);
			len[i] = len[i - 1] + ImSqrt((q.x - prev.x) * (q.x - prev.x) + (q.y - prev.y) * (q.y - prev.y));
			prev = q;
		}
		float const target = w * len[N];
		int i = 1;
		while (i < N && len[i] < target) i++;
		float const seg = len[i] - len[i - 1];
		float const f = seg > 0.0f ? (target - len[i - 1]) / seg : 0.0f;
		ImVec2 const q = ImBezierCubicCalc(p0, p1, p2, p3, ((float)(i - 1) + ImClamp(f, 0.0f, 1.0f)) / N);
		out[0] = q.x; out[1] = q.y;
	}
}

static inline bool present(prop_t const& p) { return p.vn > 0 || p.kn > 0; }

// ----------------------------------------------------------------------------
// Geometry
// ----------------------------------------------------------------------------

// 2D transform of a layer plane, projective so 3D layers seen through a camera fit too:
// x' = (a x + c y + x) / w, y' = (b x + d y + y) / w, w = g x + h y + i (affine: g = h = 0, i = 1)
struct mat { float a, b, c, d, x, y, g, h, i; };
static inline mat mat_make(float a, float b, float c, float d, float x, float y) { mat m = { a, b, c, d, x, y, 0, 0, 1 }; return m; }
static inline mat mul(mat const& A, mat const& B) {
	mat r;
	r.a = A.a * B.a + A.c * B.b + A.x * B.g; r.c = A.a * B.c + A.c * B.d + A.x * B.h; r.x = A.a * B.x + A.c * B.y + A.x * B.i;
	r.b = A.b * B.a + A.d * B.b + A.y * B.g; r.d = A.b * B.c + A.d * B.d + A.y * B.h; r.y = A.b * B.x + A.d * B.y + A.y * B.i;
	r.g = A.g * B.a + A.h * B.b + A.i * B.g; r.h = A.g * B.c + A.h * B.d + A.i * B.h; r.i = A.g * B.x + A.h * B.y + A.i * B.i;
	return r;
}
static inline ImVec2 apply(mat const& M, float x, float y) {
	float w = M.g * x + M.h * y + M.i;
	if (w < 1e-3f) w = 1e-3f;  // behind the camera: pushed far away instead of mirrored
	return ImVec2((M.a * x + M.c * y + M.x) / w, (M.b * x + M.d * y + M.y) / w);
}
static inline mat rot(float deg) { float const r = deg * (IM_PI / 180.0f), c = ImCos(r), s = ImSin(r); return mat_make(c, s, -s, c, 0, 0); }
// Local scale at the plane origin (area scale = det / w^3)
static inline float mat_scale(mat const& M) {
	float const det = M.a * (M.d * M.i - M.y * M.h) - M.c * (M.b * M.i - M.y * M.g) + M.x * (M.b * M.h - M.d * M.g);
	float const w = ImMax(ImFabs(M.i), 1e-3f);
	return ImSqrt(ImFabs(det) / (w * w * w));
}

// 3D: 4x4 row-major, column vectors; After Effects axes (x right, y down, z into the screen)
struct m4 { float m[16]; };
static m4 m4_id() { m4 r; for (int k = 0; k < 16; k++) r.m[k] = (k % 5) == 0 ? 1.0f : 0.0f; return r; }
static m4 m4_mul(m4 const& A, m4 const& B) {
	m4 r;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++) r.m[i * 4 + j] = A.m[i * 4] * B.m[j] + A.m[i * 4 + 1] * B.m[4 + j] + A.m[i * 4 + 2] * B.m[8 + j] + A.m[i * 4 + 3] * B.m[12 + j];
	return r;
}
static m4 m4_translate(float x, float y, float z) { m4 r = m4_id(); r.m[3] = x; r.m[7] = y; r.m[11] = z; return r; }
static m4 m4_rot(int axis, float deg) {
	float const t = deg * (IM_PI / 180.0f), c = ImCos(t), s = ImSin(t);
	m4 r = m4_id();
	int const u = axis == 0 ? 1 : 0, v = axis == 2 ? 1 : 2;  // the plane the axis rotates
	r.m[u * 4 + u] = c; r.m[v * 4 + v] = c;
	if (axis == 1) { r.m[u * 4 + v] = s; r.m[v * 4 + u] = -s; }  // y: x' = c x + s z
	else { r.m[u * 4 + v] = -s; r.m[v * 4 + u] = s; }           // x: y' = c y - s z; z: x' = c x - s y
	return r;
}
static m4 m4_from_mat(mat const& M) {
	m4 r = m4_id();
	r.m[0] = M.a; r.m[1] = M.c; r.m[3] = M.x;
	r.m[4] = M.b; r.m[5] = M.d; r.m[7] = M.y;
	return r;
}
// Inverse of an affine 4x4 (bottom row 0 0 0 1)
static m4 m4_inverse_affine(m4 const& A) {
	float const* a = A.m;
	float const c00 = a[5] * a[10] - a[6] * a[9], c01 = a[6] * a[8] - a[4] * a[10], c02 = a[4] * a[9] - a[5] * a[8];
	float det = a[0] * c00 + a[1] * c01 + a[2] * c02;
	if (ImFabs(det) < 1e-12f) return m4_id();
	det = 1.0f / det;
	m4 r = m4_id();
	r.m[0] = c00 * det; r.m[1] = (a[2] * a[9] - a[1] * a[10]) * det; r.m[2] = (a[1] * a[6] - a[2] * a[5]) * det;
	r.m[4] = c01 * det; r.m[5] = (a[0] * a[10] - a[2] * a[8]) * det; r.m[6] = (a[2] * a[4] - a[0] * a[6]) * det;
	r.m[8] = c02 * det; r.m[9] = (a[1] * a[8] - a[0] * a[9]) * det; r.m[10] = (a[0] * a[5] - a[1] * a[4]) * det;
	for (int i = 0; i < 3; i++) r.m[i * 4 + 3] = -(r.m[i * 4] * a[3] + r.m[i * 4 + 1] * a[7] + r.m[i * 4 + 2] * a[11]);
	return r;
}

struct contour { int first, count; bool closed; };
// Fill rules: 0 non-zero, 1 even-odd, k >= 2: winding >= k - 1 (merge paths)
struct geom {
	ImVector<ImVec2> pts;      // cubic chains: p0, (c1 c2 p) * n
	ImVector<contour> cs;
	int rule = -1;             // set by merge paths, overrides the style's rule
	ImVector<ImVec2> soup;     // extra directed segments filled with the contours (offset path)
	void begin(ImVec2 p) { contour c = { pts.Size, 1, false }; cs.push_back(c); pts.push_back(p); }
	void cubic(ImVec2 c1, ImVec2 c2, ImVec2 p) { pts.push_back(c1); pts.push_back(c2); pts.push_back(p); cs.back().count += 3; }
	void line(ImVec2 p) { ImVec2 const a = pts.back(); cubic(a, p, p); }  // Lottie convention: handles on the vertices
	void clear() { pts.resize(0); cs.resize(0); soup.resize(0); rule = -1; }
	// Takes o's buffers (modifiers build a new geometry, then replace the old one without copying)
	void take(geom& o) { pts.swap(o.pts); cs.swap(o.cs); soup.swap(o.soup); rule = o.rule; }
	void append(geom const& g) {
		for (int i = 0; i < g.cs.Size; i++) { contour c = g.cs[i]; c.first += pts.Size; cs.push_back(c); }
		for (int i = 0; i < g.pts.Size; i++) pts.push_back(g.pts[i]);
		for (int i = 0; i < g.soup.Size; i++) soup.push_back(g.soup[i]);
	}
	void polyline(ImVec2 const* p, int n, bool closed) {
		if (n <= 0) return;
		begin(p[0]);
		for (int i = 1; i < n; i++) line(p[i]);
		if (closed && n > 1) line(p[0]);
		cs.back().closed = closed;
	}
};

// Builds a contour from Lottie vertices (in / out tangents relative to their vertex)
static void add_vertices(geom& g, mat const& M, float const* v, int n, bool closed, bool reverse) {
	if (n <= 0) return;
	// v: (vx vy ix iy ox oy) per vertex
	auto V = [&](int k) { k = reverse ? (n - 1 - k) : k; return v + 6 * k; };
	auto in_of = [&](float const* q) { return reverse ? ImVec2(q[0] + q[4], q[1] + q[5]) : ImVec2(q[0] + q[2], q[1] + q[3]); };
	auto out_of = [&](float const* q) { return reverse ? ImVec2(q[0] + q[2], q[1] + q[3]) : ImVec2(q[0] + q[4], q[1] + q[5]); };
	float const* q0 = V(0);
	g.begin(apply(M, q0[0], q0[1]));
	int const segs = closed ? n : n - 1;
	for (int k = 0; k < segs; k++) {
		float const* a = V(k);
		float const* b = V((k + 1) % n);
		ImVec2 const o = out_of(a), i = in_of(b);
		g.cubic(apply(M, o.x, o.y), apply(M, i.x, i.y), apply(M, b[0], b[1]));
	}
	g.cs.back().closed = closed;
}

static void flatten_contour(geom const& g, contour const& c, ImVector<ImVec2>& out, float tol) {
	out.resize(0);
	ImVec2 const* p = g.pts.Data + c.first;
	out.push_back(p[0]);
	for (int k = 0; k + 3 < c.count; k += 3) {
		ImVec2 const p0 = p[k], p1 = p[k + 1], p2 = p[k + 2], p3 = p[k + 3];
		bool const straight = (p1.x == p0.x && p1.y == p0.y && p2.x == p3.x && p2.y == p3.y);
		int n = 1;
		if (!straight) {
			// Segment count from the second-difference bound (Wang's formula)
			float const ddx = ImMax(ImFabs(p0.x - 2 * p1.x + p2.x), ImFabs(p1.x - 2 * p2.x + p3.x));
			float const ddy = ImMax(ImFabs(p0.y - 2 * p1.y + p2.y), ImFabs(p1.y - 2 * p2.y + p3.y));
			n = ImClamp((int)ImCeil(ImSqrt(ImSqrt(ddx * ddx + ddy * ddy) * 0.75f / tol)), 1, 512);
		}
		for (int i = 1; i <= n; i++) out.push_back(n == 1 ? p3 : ImBezierCubicCalc(p0, p1, p2, p3, (float)i / (float)n));
	}
	if (c.closed && out.Size > 1 && ImFabs(out[0].x - out.back().x) < 1e-5f && ImFabs(out[0].y - out.back().y) < 1e-5f) out.pop_back();
}

static float poly_length(ImVector<ImVec2> const& p, bool closed) {
	float l = 0.0f;
	int const n = closed ? p.Size : p.Size - 1;
	for (int i = 0; i < n; i++) { ImVec2 const a = p[i], b = p[(i + 1) % p.Size]; l += ImSqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)); }
	return l;
}

// Appends the part of polyline p between lengths a and b (a < b) to out
static void poly_extract(ImVector<ImVec2> const& p, bool closed, float a, float b, ImVector<ImVec2>& out) {
	int const n = closed ? p.Size : p.Size - 1;
	float acc = 0.0f;
	for (int i = 0; i < n; i++) {
		ImVec2 const s = p[i], e = p[(i + 1) % p.Size];
		float const l = ImSqrt((e.x - s.x) * (e.x - s.x) + (e.y - s.y) * (e.y - s.y));
		float const l0 = acc, l1 = acc + l;
		acc = l1;
		if (l1 < a || l0 > b || l <= 0.0f) continue;
		float const t0 = ImSaturate((a - l0) / l), t1 = ImSaturate((b - l0) / l);
		ImVec2 const q0(s.x + (e.x - s.x) * t0, s.y + (e.y - s.y) * t0), q1(s.x + (e.x - s.x) * t1, s.y + (e.y - s.y) * t1);
		if (out.Size == 0 || out.back().x != q0.x || out.back().y != q0.y) out.push_back(q0);
		out.push_back(q1);
	}
}

// Trim paths: s, e in [0, 1], o in turns; mode 1 = each path, 2 = all paths as one
static void trim(geom& g, float s, float e, float o, int mode, float tol) {
	s = ImSaturate(s); e = ImSaturate(e);
	if (s > e) { float const t = s; s = e; e = t; }
	if (e - s >= 1.0f) return;
	geom out;
	if (e - s <= 1e-6f) { out.rule = g.rule; out.soup.swap(g.soup); g.take(out); return; }
	float start = s + o;
	start -= ImFloor(start);
	float const end = start + (e - s);
	ImVector<ImVec2> poly, piece;
	if (mode == 2) {
		float total = 0.0f;
		for (int i = 0; i < g.cs.Size; i++) { flatten_contour(g, g.cs[i], poly, tol); total += poly_length(poly, g.cs[i].closed); }
		float ofs = 0.0f;
		for (int i = 0; i < g.cs.Size; i++) {
			flatten_contour(g, g.cs[i], poly, tol);
			float const len = poly_length(poly, g.cs[i].closed);
			float const ranges[2][2] = { { start * total, ImMin(end, 1.0f) * total }, { 0.0f, (end - 1.0f) * total } };
			for (int r = 0; r < 2; r++) {
				float const a = ImMax(ranges[r][0], ofs) - ofs, b = ImMin(ranges[r][1], ofs + len) - ofs;
				if (b <= a) continue;
				piece.resize(0);
				poly_extract(poly, g.cs[i].closed, a, b, piece);
				out.polyline(piece.Data, piece.Size, false);
			}
			ofs += len;
		}
	} else {
		for (int i = 0; i < g.cs.Size; i++) {
			bool const closed = g.cs[i].closed;
			flatten_contour(g, g.cs[i], poly, tol);
			float const len = poly_length(poly, closed);
			piece.resize(0);
			poly_extract(poly, closed, start * len, ImMin(end, 1.0f) * len, piece);
			if (end > 1.0f) {
				if (!closed) { out.polyline(piece.Data, piece.Size, false); piece.resize(0); }
				poly_extract(poly, closed, 0.0f, (end - 1.0f) * len, piece);  // a closed path continues across its start
			}
			out.polyline(piece.Data, piece.Size, false);
		}
	}
	out.rule = g.rule; out.soup.swap(g.soup); g.take(out);
}

// Contour as vertices with absolute handles (in = handle before the vertex, out = after)
static int contour_vertices(geom const& g, contour const& c, ImVector<ImVec2>& v, ImVector<ImVec2>& vin, ImVector<ImVec2>& vout) {
	ImVec2 const* p = g.pts.Data + c.first;
	int const nseg = (c.count - 1) / 3;
	int const n = c.closed ? nseg : nseg + 1;
	v.resize(0); vin.resize(0); vout.resize(0);
	for (int k = 0; k < n; k++) {
		v.push_back(p[3 * k]);
		vin.push_back(k > 0 ? p[3 * k - 1] : (c.closed && nseg > 0 ? p[c.count - 2] : p[0]));
		vout.push_back(k < nseg ? p[3 * k + 1] : p[3 * k]);
	}
	return n;
}

static void add_contour_vertices(geom& out, ImVector<ImVec2> const& v, ImVector<ImVec2> const& vin, ImVector<ImVec2> const& vout, bool closed) {
	if (v.Size == 0) return;
	out.begin(v[0]);
	for (int k = 0; k + 1 < v.Size; k++) out.cubic(vout[k], vin[k + 1], v[k + 1]);
	if (closed) out.cubic(vout.back(), vin[0], v[0]);
	out.cs.back().closed = closed;
}

// Pucker (amount < 0) / bloat (> 0), in percent: vertices move toward the centre of their
// path, handles away from it
static void pucker_bloat(geom& g, float amount) {
	if (amount == 0.0f) return;
	float const f = amount * 0.01f;
	geom out;
	ImVector<ImVec2> v, vin, vout;
	for (int ci = 0; ci < g.cs.Size; ci++) {
		int const n = contour_vertices(g, g.cs[ci], v, vin, vout);
		ImVec2 c(0, 0);
		for (int k = 0; k < n; k++) { c.x += v[k].x / n; c.y += v[k].y / n; }
		for (int k = 0; k < n; k++) {
			v[k] = ImVec2(v[k].x + (c.x - v[k].x) * f, v[k].y + (c.y - v[k].y) * f);
			vin[k] = ImVec2(vin[k].x - (c.x - vin[k].x) * f, vin[k].y - (c.y - vin[k].y) * f);
			vout[k] = ImVec2(vout[k].x - (c.x - vout[k].x) * f, vout[k].y - (c.y - vout[k].y) * f);
		}
		add_contour_vertices(out, v, vin, vout, g.cs[ci].closed);
	}
	out.rule = g.rule; out.soup.swap(g.soup); g.take(out);
}

// Zig-zag: `ridges` points per segment (vertex to vertex), alternately pushed `size` to each
// side of the path; smooth = 2 rounds the peaks
static void zigzag(geom& g, float size, int ridges, bool smooth, float tol) {
	if (size == 0.0f || ridges < 0) return;
	geom out;
	ImVector<ImVec2> seg, pts, nrm, v, vin, vout;
	for (int ci = 0; ci < g.cs.Size; ci++) {
		contour const& c = g.cs[ci];
		ImVec2 const* p = g.pts.Data + c.first;
		pts.resize(0); nrm.resize(0);
		for (int k = 0; k + 3 < c.count; k += 3) {
			// One segment, flattened, sampled at ridges + 1 equal arc-length steps
			geom one;
			one.begin(p[k]);
			one.cubic(p[k + 1], p[k + 2], p[k + 3]);
			flatten_contour(one, one.cs[0], seg, tol);
			float const len = poly_length(seg, false);
			int const steps = ridges + 1;
			int j = 0;
			float acc = 0.0f;
			for (int st = 0; st < steps; st++) {
				float const target = len * st / steps;
				while (j + 1 < seg.Size - 1 && acc + ImSqrt((seg[j + 1].x - seg[j].x) * (seg[j + 1].x - seg[j].x) + (seg[j + 1].y - seg[j].y) * (seg[j + 1].y - seg[j].y)) < target) {
					acc += ImSqrt((seg[j + 1].x - seg[j].x) * (seg[j + 1].x - seg[j].x) + (seg[j + 1].y - seg[j].y) * (seg[j + 1].y - seg[j].y));
					j++;
				}
				ImVec2 const a = seg[j], b = seg[ImMin(j + 1, seg.Size - 1)];
				float const l = ImSqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
				float const f = l > 0.0f ? ImSaturate((target - acc) / l) : 0.0f;
				pts.push_back(ImVec2(a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f));
				nrm.push_back(l > 0.0f ? ImVec2(-(b.y - a.y) / l, (b.x - a.x) / l) : ImVec2(0, 0));
			}
		}
		if (!c.closed && c.count >= 4) {
			ImVec2 const a = p[c.count - 2], b = p[c.count - 1];
			float const l = ImSqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
			pts.push_back(b);
			nrm.push_back(l > 0.0f ? ImVec2(-(b.y - a.y) / l, (b.x - a.x) / l) : (nrm.Size ? nrm.back() : ImVec2(0, 0)));
		}
		if (pts.Size < 2) continue;
		v.resize(pts.Size); vin.resize(pts.Size); vout.resize(pts.Size);
		for (int k = 0; k < pts.Size; k++) {
			float const side = (k % 2) ? -size : size;
			v[k] = ImVec2(pts[k].x + nrm[k].x * side, pts[k].y + nrm[k].y * side);
		}
		for (int k = 0; k < pts.Size; k++) {
			vin[k] = vout[k] = v[k];
			if (!smooth) continue;
			// Handles along the path direction, a third of the way to the neighbours
			ImVec2 const prev = v[(k - 1 + pts.Size) % pts.Size], next = v[(k + 1) % pts.Size];
			ImVec2 const tdir(nrm[k].y, -nrm[k].x);
			float const lp = ImSqrt((v[k].x - prev.x) * (v[k].x - prev.x) + (v[k].y - prev.y) * (v[k].y - prev.y)) * 0.36f;
			float const ln = ImSqrt((next.x - v[k].x) * (next.x - v[k].x) + (next.y - v[k].y) * (next.y - v[k].y)) * 0.36f;
			vin[k] = ImVec2(v[k].x - tdir.x * lp, v[k].y - tdir.y * lp);
			vout[k] = ImVec2(v[k].x + tdir.x * ln, v[k].y + tdir.y * ln);
		}
		add_contour_vertices(out, v, vin, vout, c.closed);
	}
	out.rule = g.rule; out.soup.swap(g.soup); g.take(out);
}

// Reverses contour ci if needed so it fills with winding +1 (positive = true) or -1
static void orient_contour(geom& g, int ci, bool positive, float tol, ImVector<ImVec2>& poly) {
	flatten_contour(g, g.cs[ci], poly, tol);
	float area = 0.0f;
	for (int i = 0; i < poly.Size; i++) { ImVec2 const a = poly[i], b = poly[(i + 1) % poly.Size]; area += a.x * b.y - b.x * a.y; }
	if ((area <= 0.0f) == positive) return;  // y down: negative shoelace area = winding +1
	contour const& c = g.cs[ci];
	for (int i = 0, j = c.count - 1; i < j; i++, j--) { ImVec2 const t = g.pts[c.first + i]; g.pts[c.first + i] = g.pts[c.first + j]; g.pts[c.first + j] = t; }
}

static void stroke_polyline(ImVector<ImVec2>& segs, ImVec2 const* in, int n_in, bool closed, float hw, int cap, int join, float miter, float tol);

// Offset path: the shape grown (amount > 0) or shrunk by |amount|, as the shape plus or
// minus a stroke of that half width (joins: 1 miter, 2 round, 3 bevel)
static void offset_path(geom& g, float amount, int join, float miter, float tol) {
	if (amount == 0.0f || g.cs.Size == 0) return;
	ImVector<ImVec2> poly;
	for (int ci = 0; ci < g.cs.Size; ci++) {
		orient_contour(g, ci, amount < 0.0f, tol, poly);  // shortcut: holes become solid, upgrade if files offset outlines with holes
		flatten_contour(g, g.cs[ci], poly, tol);
		stroke_polyline(g.soup, poly.Data, poly.Size, true, ImFabs(amount), 1, join, miter, tol);  // pieces fill with winding -1
	}
	g.rule = amount > 0.0f ? 0 : 2;  // union under non-zero; shrink: winding >= 1 keeps the shape outside the stroke
}

// Twist: no renderer defines it (lottie-web, Skottie, ThorVG and rlottie skip it); this
// follows After Effects' description, turning the path more at the centre than at the edge:
// each flattened point rotates by angle * (1 - d / R) around the centre, R the farthest point
static void twist(geom& g, float angle_deg, ImVec2 center, float tol) {
	if (angle_deg == 0.0f || g.cs.Size == 0) return;
	ImVector<ImVec2> poly, all;
	ImVector<int> counts;
	ImVector<bool> closed;
	float R = 0.0f;
	for (int ci = 0; ci < g.cs.Size; ci++) {
		flatten_contour(g, g.cs[ci], poly, tol * 0.5f);  // finer: straight edges bend
		// Long straight segments have no inner points to bend: subdivide them
		ImVector<ImVec2> fine;
		int const n = g.cs[ci].closed ? poly.Size : poly.Size - 1;
		for (int i = 0; i < n; i++) {
			ImVec2 const a = poly[i], b = poly[(i + 1) % poly.Size];
			int const steps = ImClamp((int)(ImSqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)) / 4.0f), 1, 256);
			for (int k = 0; k < steps; k++) fine.push_back(ImVec2(a.x + (b.x - a.x) * k / steps, a.y + (b.y - a.y) * k / steps));
		}
		if (!g.cs[ci].closed && poly.Size) fine.push_back(poly.back());
		for (int i = 0; i < fine.Size; i++) {
			all.push_back(fine[i]);
			float const dx = fine[i].x - center.x, dy = fine[i].y - center.y;
			R = ImMax(R, ImSqrt(dx * dx + dy * dy));
		}
		counts.push_back(fine.Size);
		closed.push_back(g.cs[ci].closed);
	}
	if (R <= 0.0f) return;
	for (int i = 0; i < all.Size; i++) {
		float const dx = all[i].x - center.x, dy = all[i].y - center.y;
		float const a = angle_deg * (IM_PI / 180.0f) * (1.0f - ImSqrt(dx * dx + dy * dy) / R);
		float const c = ImCos(a), s = ImSin(a);
		all[i] = ImVec2(center.x + dx * c - dy * s, center.y + dx * s + dy * c);
	}
	geom out;
	int ofs = 0;
	for (int ci = 0; ci < counts.Size; ci++) { out.polyline(all.Data + ofs, counts[ci], closed[ci]); ofs += counts[ci]; }
	out.rule = g.rule; out.soup.swap(g.soup);
	g = out;
}

// Merge paths (geometry so far in the group): 1 merge keeps the paths as they are; 2 add,
// 3 subtract, 4 intersect, 5 exclude orient the contours and pick a fill rule
static void merge_paths(geom& g, int mode, float tol) {
	if (mode < 2 || mode > 5 || g.cs.Size == 0) return;
	ImVector<ImVec2> poly;
	for (int ci = 0; ci < g.cs.Size; ci++) orient_contour(g, ci, !(mode == 3 && ci > 0), tol, poly);  // subtract: the first path minus the others
	g.rule = mode == 2 ? 0 : mode == 3 ? 2 : mode == 4 ? g.cs.Size + 1 : 1;
}

// Round corners: sharp vertices (both handles on the vertex) become arcs of radius r
static void round_corners(geom& g, float r) {
	if (r <= 0.0f) return;
	geom out;
	ImVector<ImVec2> v, vin, vout;
	for (int ci = 0; ci < g.cs.Size; ci++) {
		contour const& c = g.cs[ci];
		ImVec2 const* p = g.pts.Data + c.first;
		int const nseg = (c.count - 1) / 3;
		int const n = c.closed ? nseg : nseg + 1;
		if (n < 2) { geom one; one.pts.resize(0); for (int i = 0; i < c.count; i++) one.pts.push_back(p[i]); contour cc = { 0, c.count, c.closed }; one.cs.push_back(cc); out.append(one); continue; }
		v.resize(0); vin.resize(0); vout.resize(0);
		for (int k = 0; k < n; k++) {
			ImVec2 const pv = p[3 * k];
			ImVec2 const pin = (k > 0) ? p[3 * k - 1] : (c.closed ? p[c.count - 2] : pv);
			ImVec2 const pout = (k < nseg) ? p[3 * k + 1] : pv;
			bool const sharp = pin.x == pv.x && pin.y == pv.y && pout.x == pv.x && pout.y == pv.y;
			bool const end = !c.closed && (k == 0 || k == n - 1);
			if (!sharp || end) { v.push_back(pv); vin.push_back(pin); vout.push_back(pout); continue; }
			ImVec2 const prev = p[3 * ((k - 1 + n) % n)], next = p[3 * ((k + 1) % n)];
			float const k_arc = 0.5519f;
			ImVec2 const ends[2] = { prev, next };
			ImVec2 q[2];
			for (int s = 0; s < 2; s++) {
				float const dx = ends[s].x - pv.x, dy = ends[s].y - pv.y, d = ImSqrt(dx * dx + dy * dy);
				float const f = d > 0.0f ? ImMin(d * 0.5f, r) / d : 0.0f;
				q[s] = ImVec2(pv.x + dx * f, pv.y + dy * f);
			}
			v.push_back(q[0]); vin.push_back(q[0]); vout.push_back(ImVec2(q[0].x + (pv.x - q[0].x) * k_arc, q[0].y + (pv.y - q[0].y) * k_arc));
			v.push_back(q[1]); vin.push_back(ImVec2(q[1].x + (pv.x - q[1].x) * k_arc, q[1].y + (pv.y - q[1].y) * k_arc)); vout.push_back(q[1]);
		}
		out.begin(v[0]);
		for (int k = 0; k + 1 < v.Size; k++) out.cubic(vout[k], vin[k + 1], v[k + 1]);
		if (c.closed) out.cubic(vout.back(), vin[0], v[0]);
		out.cs.back().closed = c.closed;
	}
	out.rule = g.rule; out.soup.swap(g.soup); g.take(out);
}

// ----------------------------------------------------------------------------
// Polygon fill (adapted from DearWidgets DW_FillSegments)
// ----------------------------------------------------------------------------
// Trapezoid decomposition: the y range is cut into slabs at every vertex (and at every
// edge crossing, so self-intersecting outlines are exact); inside a slab the edges do
// not cross, so sorting them by x and walking the winding number gives the covered
// spans. Anti-aliasing is a 1 px alpha fringe along every boundary piece.

enum { MAX_CLIPS = 4, MAX_STOPS = 16 };

struct clip_t { ImVector<ImVec2> segs; int even_odd; ImRect bounds; };

struct paint_t {
	int type;                  // 1 linear, 2 radial
	ImVec2 p0, p1;
	ImVec2 focal;              // radial: highlight point (p0 when none)
	int n;
	float off[MAX_STOPS];
	float rgba[MAX_STOPS][4];  // straight alpha
};

struct fill_edge { float x0, y0, x1, y1; int dir, set; };
struct slab_edge { float xa, xb; int dir, id, set; };
struct open_span { int L, R; float y0; };
struct span { float l, r; };
struct pvert { ImVec2 p; float cov; };

static inline float edge_x_at(fill_edge const& e, float y) { return e.x0 + (e.x1 - e.x0) * ((y - e.y0) / (e.y1 - e.y0)); }

struct paint_eval {
	paint_t const* p;
	ImVec2 dir;
	float inv_len2, inv_r;
	void init(paint_t const* paint) {
		p = paint;
		dir = ImVec2(p->p1.x - p->p0.x, p->p1.y - p->p0.y);
		float const l2 = dir.x * dir.x + dir.y * dir.y;
		inv_len2 = l2 > 0.0f ? 1.0f / l2 : 0.0f;
		inv_r = l2 > 0.0f ? 1.0f / ImSqrt(l2) : 0.0f;
	}
	float T(ImVec2 q) const {
		float const dx = q.x - p->p0.x, dy = q.y - p->p0.y;
		if (p->type != 2) return (dx * dir.x + dy * dir.y) * inv_len2;
		if (p->focal.x == p->p0.x && p->focal.y == p->p0.y) return ImSqrt(dx * dx + dy * dy) * inv_r;
		// Focal radial (SVG fx/fy): distance from the focal point over the distance from it
		// to the circle along the same ray
		float const qx = q.x - p->focal.x, qy = q.y - p->focal.y, ql = ImSqrt(qx * qx + qy * qy);
		if (ql <= 1e-6f) return 0.0f;
		float const r = inv_r > 0.0f ? 1.0f / inv_r : 0.0f;
		float const ux = qx / ql, uy = qy / ql, fx = p->focal.x - p->p0.x, fy = p->focal.y - p->p0.y;
		float const b = ux * fx + uy * fy, c = fx * fx + fy * fy - r * r;
		float const L = -b + ImSqrt(ImMax(b * b - c, 0.0f));
		return L > 1e-6f ? ql / L : 1.0f;
	}
	// Premultiplied RGBA at gradient parameter t (padded)
	void color(float t, float out[4]) const {
		t = ImSaturate(t);
		int const n = p->n;
		int i = 0;
		while (i < n && p->off[i] <= t) ++i;
		float const* a = p->rgba[ImClamp(i - 1, 0, n - 1)];
		float const* b = p->rgba[ImClamp(i, 0, n - 1)];
		float u = 0.0f;
		if (i > 0 && i < n) { float const span = p->off[i] - p->off[i - 1]; u = span > 0.0f ? (t - p->off[i - 1]) / span : 1.0f; }
		float pa[4] = { a[0] * a[3], a[1] * a[3], a[2] * a[3], a[3] }, pb[4] = { b[0] * b[3], b[1] * b[3], b[2] * b[3], b[3] };
		for (int k = 0; k < 4; ++k) out[k] = pa[k] + (pb[k] - pa[k]) * u;
	}
	ImU32 color_u32(float t, float coverage) const {
		float c[4];
		color(t, c);
		float const al = c[3] * coverage;
		if (al <= 0.0f) return 0;
		float const inv_a = c[3] > 0.0f ? 1.0f / c[3] : 0.0f;  // straight alpha for ImDrawList
		return IM_COL32((int)(ImSaturate(c[0] * inv_a) * 255.0f + 0.5f), (int)(ImSaturate(c[1] * inv_a) * 255.0f + 0.5f),
		                (int)(ImSaturate(c[2] * inv_a) * 255.0f + 0.5f), (int)(ImSaturate(al) * 255.0f + 0.5f));
	}
};

// Clip a convex polygon to f(p) >= 0 for an affine f (Sutherland-Hodgman, one plane)
template <typename F>
static void clip_poly(ImVector<pvert> const& in, ImVector<pvert>& out, F f) {
	out.resize(0);
	int const n = in.Size;
	for (int i = 0; i < n; ++i) {
		pvert const& A = in[i];
		pvert const& B = in[(i + 1) % n];
		float const fa = f(A.p), fb = f(B.p);
		if (fa >= 0.0f) out.push_back(A);
		if ((fa >= 0.0f) != (fb >= 0.0f)) {
			float const t = fa / (fa - fb);
			pvert v = { ImVec2(A.p.x + (B.p.x - A.p.x) * t, A.p.y + (B.p.y - A.p.y) * t), A.cov + (B.cov - A.cov) * t };
			out.push_back(v);
		}
	}
}

struct fill_writer {
	ImDrawList* dl;
	ImU32 col, col_trans, col_half;
	float aa;
	ImVec2 uv;
	paint_eval const* paint;
	ImVector<pvert> pa, pb, quad, row;
	int reserved;  // quad vertices reserved ahead and not yet written (one PrimReserve per 64 quads)

	void flush() { if (reserved) dl->PrimUnreserve(reserved / 4 * 6, reserved); reserved = 0; }

	// Convex polygon with per-vertex coverage, coloured by the gradient (t clamped into [lo, hi])
	void paint_poly(ImVector<pvert> const& poly, float lo, float hi) {
		if (poly.Size < 3) return;
		flush();
		dl->PrimReserve((poly.Size - 2) * 3, poly.Size);
		ImDrawIdx const base = (ImDrawIdx)dl->_VtxCurrentIdx;
		for (int i = 0; i < poly.Size; ++i) {
			float t = paint->T(poly[i].p);
			if (lo > -FLT_MAX && hi < FLT_MAX) t = ImClamp(t, lo + (hi - lo) * 1e-4f, hi - (hi - lo) * 1e-4f);
			dl->PrimWriteVtx(poly[i].p, uv, paint->color_u32(t, poly[i].cov));
		}
		for (int i = 1; i + 1 < poly.Size; ++i) { dl->PrimWriteIdx(base); dl->PrimWriteIdx((ImDrawIdx)(base + i)); dl->PrimWriteIdx((ImDrawIdx)(base + i + 1)); }
	}

	void paint_quad(ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d, float ka, float kb, float kc, float kd) {
		quad.resize(0);
		pvert const v[4] = { { a, ka }, { b, kb }, { c, kc }, { d, kd } };
		for (int i = 0; i < 4; i++) quad.push_back(v[i]);
		paint_eval const* pe = paint;
		if (paint->p->type == 1) {
			// Linear: exact, sliced at every stop so the GPU interpolation matches the gradient
			float tmin = FLT_MAX, tmax = -FLT_MAX;
			for (int i = 0; i < 4; ++i) { float const t = pe->T(quad[i].p); tmin = ImMin(tmin, t); tmax = ImMax(tmax, t); }
			float cuts[MAX_STOPS + 2];
			int nc = 0;
			for (int i = 0; i < pe->p->n; i++) { float const o = pe->p->off[i]; if (o > tmin && o < tmax) cuts[nc++] = o; }
			float lo = -FLT_MAX;
			for (int ci = 0; ci <= nc; ++ci) {
				float const hi = ci < nc ? cuts[ci] : FLT_MAX;
				if (hi <= lo) continue;
				clip_poly(quad, pa, [&](ImVec2 p) { return pe->T(p) - lo; });
				clip_poly(pa, pb, [&](ImVec2 p) { return hi - pe->T(p); });
				paint_poly(pb, lo, hi);
				lo = hi;
			}
			return;
		}
		// Radial: not linear in position; evaluate on a small grid
		// Cells scale with the radius: across one, t changes by at most ~1/20 (colours interpolate linearly inside)
		float const cell = ImClamp(pe->inv_r > 0.0f ? 0.05f / pe->inv_r : 4.0f, 4.0f, 32.0f);
		float x0 = FLT_MAX, y0 = FLT_MAX, x1 = -FLT_MAX, y1 = -FLT_MAX;
		for (int i = 0; i < 4; ++i) { x0 = ImMin(x0, quad[i].p.x); y0 = ImMin(y0, quad[i].p.y); x1 = ImMax(x1, quad[i].p.x); y1 = ImMax(y1, quad[i].p.y); }
		for (float gy = ImFloor(y0 / cell) * cell; gy < y1; gy += cell) {
			clip_poly(quad, pa, [&](ImVec2 p) { return p.y - gy; });
			clip_poly(pa, row, [&](ImVec2 p) { return gy + cell - p.y; });
			if (row.Size < 3) continue;
			for (float gx = ImFloor(x0 / cell) * cell; gx < x1; gx += cell) {
				clip_poly(row, pa, [&](ImVec2 p) { return p.x - gx; });
				clip_poly(pa, pb, [&](ImVec2 p) { return gx + cell - p.x; });
				paint_poly(pb, -FLT_MAX, FLT_MAX);
			}
		}
	}

	void Quad(ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d, ImU32 ca, ImU32 cb, ImU32 cc, ImU32 cd) {
		if (paint) {
			// Coverage per vertex is carried in the alpha the solid path would use
			float const full = (float)((col >> IM_COL32_A_SHIFT) & 0xFF);
			auto k = [&](ImU32 v) { return full > 0.0f ? (float)((v >> IM_COL32_A_SHIFT) & 0xFF) / full : 0.0f; };
			paint_quad(a, b, c, d, k(ca), k(cb), k(cc), k(cd));
			return;
		}
		if (reserved < 4) { flush(); dl->PrimReserve(6 * 64, 4 * 64); reserved = 4 * 64; }
		reserved -= 4;
		ImDrawIdx const base = (ImDrawIdx)dl->_VtxCurrentIdx;
		dl->PrimWriteVtx(a, uv, ca); dl->PrimWriteVtx(b, uv, cb); dl->PrimWriteVtx(c, uv, cc); dl->PrimWriteVtx(d, uv, cd);
		dl->PrimWriteIdx(base); dl->PrimWriteIdx((ImDrawIdx)(base + 1)); dl->PrimWriteIdx((ImDrawIdx)(base + 2));
		dl->PrimWriteIdx(base); dl->PrimWriteIdx((ImDrawIdx)(base + 2)); dl->PrimWriteIdx((ImDrawIdx)(base + 3));
	}

	// Fringe along a boundary piece p->q with outward unit normal n: opaque half inside
	// (covered by the shrunk trapezoid), transparent half outside
	void Fringe(ImVec2 p, ImVec2 q, ImVec2 n) {
		if (aa <= 0.0f) return;
		float const h = aa * 0.5f;
		Quad(ImVec2(p.x - n.x * h, p.y - n.y * h), ImVec2(q.x - n.x * h, q.y - n.y * h),
		     ImVec2(q.x + n.x * h, q.y + n.y * h), ImVec2(p.x + n.x * h, p.y + n.y * h), col, col, col_trans, col_trans);
	}
	// Outer-half fringe for unshrunk (near-horizontal) boundaries: never overlaps the fill
	void FringeOuter(ImVec2 p, ImVec2 q, ImVec2 n) {
		if (aa <= 0.0f) return;
		float const h = aa * 0.5f;
		Quad(p, q, ImVec2(q.x + n.x * h, q.y + n.y * h), ImVec2(p.x + n.x * h, p.y + n.y * h), col_half, col_half, col_trans, col_trans);
	}
};

static inline bool fill_inside(int winding, int rule) { return rule == 0 ? winding != 0 : rule == 1 ? (winding & 1) != 0 : winding >= rule - 1; }

// Appends the parts of `a` that are not covered by `b` (both sorted, disjoint)
static void span_difference(ImVector<span> const& a, ImVector<span> const& b, ImVector<span>& out) {
	out.resize(0);
	int j = 0;
	for (int i = 0; i < a.Size; ++i) {
		float l = a[i].l;
		float const r = a[i].r;
		while (j < b.Size && b[j].r <= l) ++j;
		int k = j;
		while (l < r) {
			if (k >= b.Size || b[k].l >= r) { span s = { l, r }; out.push_back(s); break; }
			if (b[k].l > l) { span s = { l, b[k].l }; out.push_back(s); }
			l = ImMax(l, b[k].r);
			++k;
		}
	}
}

struct fill_scratch {
	ImVector<fill_edge> edges;
	ImVector<float> ys, sub_ys;
	ImVector<int> active;
	ImVector<slab_edge> slab;
	ImVector<span> prev_bottom, cur_top, cur_bottom, diff;
	ImVector<open_span> open, next_open;
	fill_writer w;
};

// Fills a soup of directed segments (pairs of points), intersected with the active clips
static void fill_segments(ImDrawList* dl, ImVec2 const* seg_pts, int seg_count, ImU32 col, int even_odd, float aa,
                          paint_t const* paint, clip_t const* clips, int nclips, ImVector<ImVec2> const* uni = nullptr) {
	paint_eval pe;
	if (paint) { pe.init(paint); col = IM_COL32_WHITE; }
	if (!dl || !seg_pts || seg_count <= 0 || (col & IM_COL32_A_MASK) == 0) return;

	// Scratch kept across calls: a file draws hundreds of small fills per frame
	static fill_scratch S;
	ImVector<fill_edge>& edges = S.edges;
	ImVector<float>& ys = S.ys;
	edges.resize(0);
	ys.resize(0);
	ImRect subj(ImVec2(FLT_MAX, FLT_MAX), ImVec2(-FLT_MAX, -FLT_MAX));
	auto add_edges = [&](ImVec2 const* pts, int count, int set) {
		for (int i = 0; i < count; ++i) {
			ImVec2 const a = pts[i * 2], b = pts[i * 2 + 1];
			if (!(a.x - a.x == 0.0f) || !(a.y - a.y == 0.0f) || !(b.x - b.x == 0.0f) || !(b.y - b.y == 0.0f)) continue;  // NaN / inf
			if (set == 0 || set == MAX_CLIPS + 1) { subj.Add(a); subj.Add(b); }
			if (a.y == b.y) continue;  // horizontal edges never change the winding
			fill_edge e;
			if (a.y < b.y) { e.x0 = a.x; e.y0 = a.y; e.x1 = b.x; e.y1 = b.y; e.dir = +1; }
			else           { e.x0 = b.x; e.y0 = b.y; e.x1 = a.x; e.y1 = a.y; e.dir = -1; }
			e.set = set;
			edges.push_back(e);
			ys.push_back(e.y0);
			ys.push_back(e.y1);
		}
	};
	add_edges(seg_pts, seg_count, 0);
	if (uni) add_edges(uni->Data, uni->Size / 2, MAX_CLIPS + 1);  // union set: non-zero, OR'ed with the shape
	if (edges.Size == 0) return;
	int rules[MAX_CLIPS + 1];
	rules[0] = even_odd;
	for (int c = 0; c < nclips; ++c) {
		if (!clips[c].bounds.Overlaps(subj)) return;  // a clip that misses the shape empties it
		rules[c + 1] = clips[c].even_odd;
		add_edges(clips[c].segs.Data, clips[c].segs.Size / 2, c + 1);
	}

	ImVec4 const clip = dl->_CmdHeader.ClipRect;
	ImQsort(ys.Data, (size_t)ys.Size, sizeof(float), [](void const* a, void const* b) -> int {
		float const fa = *(float const*)a, fb = *(float const*)b; return fa < fb ? -1 : fa > fb ? 1 : 0; });
	{
		// Exact dedupe: every endpoint must be a slab boundary
		int w = 0;
		for (int i = 0; i < ys.Size; ++i) if (w == 0 || ys[i] != ys[w - 1]) ys[w++] = ys[i];
		ys.resize(w);
	}
	ImQsort(edges.Data, (size_t)edges.Size, sizeof(fill_edge), [](void const* a, void const* b) -> int {
		float const fa = ((fill_edge const*)a)->y0, fb = ((fill_edge const*)b)->y0; return fa < fb ? -1 : fa > fb ? 1 : 0; });

	fill_writer& w = S.w;
	w.dl = dl;
	w.col = col;
	w.col_trans = col & ~IM_COL32_A_MASK;
	w.col_half = w.col_trans | ((((col >> IM_COL32_A_SHIFT) & 0xFF) / 2) << IM_COL32_A_SHIFT);
	w.aa = aa;
	w.uv = dl->_Data->TexUvWhitePixel;
	w.paint = paint ? &pe : nullptr;
	w.reserved = 0;

	ImVector<int>& active = S.active;
	ImVector<slab_edge>& slab = S.slab;
	ImVector<float>& sub_ys = S.sub_ys;
	ImVector<span>& prev_bottom = S.prev_bottom, &cur_top = S.cur_top, &cur_bottom = S.cur_bottom, &diff = S.diff;
	ImVector<open_span>& open = S.open, &next_open = S.next_open;
	active.resize(0); prev_bottom.resize(0); open.resize(0);
	float prev_bottom_y = -FLT_MAX;
	int next = 0;

	// Insertion sort: the active edges arrive in the previous slab's order, nearly sorted
	auto sort_slab = [](ImVector<slab_edge>& v) {
		for (int i = 1; i < v.Size; ++i) {
			slab_edge const t = v[i];
			float const key = t.xa + t.xb;
			int j = i - 1;
			while (j >= 0 && v[j].xa + v[j].xb > key) { v[j + 1] = v[j]; --j; }
			v[j + 1] = t;
		}
	};

	// Trapezoid of a span bounded by edges L and R between y0 and y1, shrunk by half the
	// fringe along each steep edge, plus its side fringes
	auto flush_span = [&](open_span const& sp, float y1) {
		float const y0 = sp.y0;
		if (y1 - y0 <= 0.0f) return;
		fill_edge const& EL = edges[sp.L];
		fill_edge const& ER = edges[sp.R];
		float const La = edge_x_at(EL, y0), Lb = edge_x_at(EL, y1);
		float const Ra = edge_x_at(ER, y0), Rb = edge_x_at(ER, y1);
		float const h = aa * 0.5f;
		auto edge_nx = [](fill_edge const& e) -> float {
			float const dx = e.x1 - e.x0, dy = e.y1 - e.y0, len = ImSqrt(dx * dx + dy * dy);
			return len > 0.0f ? dy / len : 1.0f;
		};
		float const nxL = edge_nx(EL), nxR = edge_nx(ER);
		bool const steepL = nxL >= 0.35f, steepR = nxR >= 0.35f;
		float const shL = (aa > 0.0f && steepL) ? h / nxL : 0.0f;
		float const shR = (aa > 0.0f && steepR) ? h / nxR : 0.0f;
		float lta = La + shL, ltb = Lb + shL, rta = Ra - shR, rtb = Rb - shR;
		if (rta < lta) { float const c = (La + Ra) * 0.5f; lta = rta = c; }
		if (rtb < ltb) { float const c = (Lb + Rb) * 0.5f; ltb = rtb = c; }
		w.Quad(ImVec2(lta, y0), ImVec2(rta, y0), ImVec2(rtb, y1), ImVec2(ltb, y1), col, col, col, col);
		if (aa > 0.0f) {
			ImVec2 const lp(La, y0), lq(Lb, y1);
			float const ldx = lq.x - lp.x, ldy = lq.y - lp.y, ll = ImSqrt(ldx * ldx + ldy * ldy);
			if (ll > 0.0f) { ImVec2 const n(-ldy / ll, ldx / ll); if (steepL) w.Fringe(lp, lq, n); else w.FringeOuter(lp, lq, n); }
			ImVec2 const rp(Ra, y0), rq(Rb, y1);
			float const rdx = rq.x - rp.x, rdy = rq.y - rp.y, rl = ImSqrt(rdx * rdx + rdy * rdy);
			if (rl > 0.0f) { ImVec2 const n(rdy / rl, -rdx / rl); if (steepR) w.Fringe(rp, rq, n); else w.FringeOuter(rp, rq, n); }
		}
	};
	auto flush_all = [&](float y1) {
		for (int i = 0; i < open.Size; ++i) flush_span(open[i], y1);
		open.resize(0);
	};

	// One crossing-free slab [ya, yb]: continues or opens spans, records top / bottom intervals
	auto emit_slab = [&](float ya, float yb) {
		slab.resize(0);
		for (int k = 0; k < active.Size; ++k) {
			fill_edge const& e = edges[active[k]];
			slab_edge s = { edge_x_at(e, ya), edge_x_at(e, yb), e.dir, active[k], e.set };
			slab.push_back(s);
		}
		sort_slab(slab);
		{
			// Coincident edges must cancel or add as one edge, whatever the sort order
			int wi = 0;
			for (int k = 0; k < slab.Size; ++k) {
				if (wi > 0 && slab[wi - 1].xa == slab[k].xa && slab[wi - 1].xb == slab[k].xb && slab[wi - 1].set == slab[k].set)
					slab[wi - 1].dir += slab[k].dir;
				else
					slab[wi++] = slab[k];
			}
			slab.resize(wi);
			wi = 0;
			for (int k = 0; k < slab.Size; ++k) if (slab[k].dir != 0) slab[wi++] = slab[k];
			slab.resize(wi);
		}
		cur_top.resize(0);
		cur_bottom.resize(0);
		next_open.resize(0);
		int open_cursor = 0;
		int wnd[MAX_CLIPS + 2] = {};
		auto inside_all = [&]() {
			if (!fill_inside(wnd[0], rules[0]) && wnd[MAX_CLIPS + 1] == 0) return false;
			for (int c = 1; c <= nclips; ++c) if (!fill_inside(wnd[c], rules[c])) return false;
			return true;
		};
		for (int k = 0; k < slab.Size; ) {
			bool const was_in = inside_all();
			wnd[slab[k].set] += slab[k].dir;
			if (!was_in && inside_all()) {
				slab_edge const& L = slab[k];
				int m = k + 1;
				for (; m < slab.Size; ++m) {
					wnd[slab[m].set] += slab[m].dir;
					if (!inside_all()) break;
				}
				if (m >= slab.Size) break;  // malformed (open) input
				slab_edge const& R = slab[m];
				if (R.xa <= L.xa && R.xb <= L.xb) { k = m + 1; continue; }
				open_span sp = { L.id, R.id, ya };
				for (int o = open_cursor; o < open.Size; ++o)
					if (open[o].L == L.id && open[o].R == R.id) { sp.y0 = open[o].y0; open[o].L = -1; open_cursor = o + 1; break; }
				next_open.push_back(sp);
				span const st = { L.xa, R.xa }, sb = { L.xb, R.xb };
				if (st.r > st.l) cur_top.push_back(st);
				if (sb.r > sb.l) cur_bottom.push_back(sb);
				k = m + 1;
				continue;
			}
			++k;
		}
		for (int o = 0; o < open.Size; ++o) if (open[o].L >= 0) flush_span(open[o], ya);
		open.swap(next_open);

		if (aa > 0.0f) {
			bool const contiguous = ImFabs(prev_bottom_y - ya) < 1e-4f;
			static ImVector<span> const empty;
			span_difference(cur_top, contiguous ? prev_bottom : empty, diff);
			for (int i = 0; i < diff.Size; ++i) w.FringeOuter(ImVec2(diff[i].l, ya), ImVec2(diff[i].r, ya), ImVec2(0.0f, -1.0f));
			if (contiguous) {
				span_difference(prev_bottom, cur_top, diff);
				for (int i = 0; i < diff.Size; ++i) w.FringeOuter(ImVec2(diff[i].l, ya), ImVec2(diff[i].r, ya), ImVec2(0.0f, 1.0f));
			} else {
				for (int i = 0; i < prev_bottom.Size; ++i)
					w.FringeOuter(ImVec2(prev_bottom[i].l, prev_bottom_y), ImVec2(prev_bottom[i].r, prev_bottom_y), ImVec2(0.0f, 1.0f));
			}
		}
		prev_bottom.swap(cur_bottom);
		prev_bottom_y = yb;
	};

	for (int yi = 0; yi + 1 < ys.Size; ++yi) {
		float const ya = ys[yi], yb = ys[yi + 1];
		while (next < edges.Size && edges[next].y0 <= ya) active.push_back(next++);
		{
			// Drop finished edges, keeping the order (it carries over from slab to slab)
			int w2 = 0;
			for (int k = 0; k < active.Size; ++k) if (edges[active[k]].y1 > ya) active[w2++] = active[k];
			active.resize(w2);
		}
		if (ya > clip.w + aa) { flush_all(ya); break; }  // everything below is clipped
		if (yb < clip.y - aa) { flush_all(ya); continue; }

		// Split the slab at every crossing (insertion sort by bottom x finds them)
		sub_ys.resize(0);
		sub_ys.push_back(ya);
		int crossings = 0;
		if (active.Size > 1) {
			slab.resize(0);
			for (int k = 0; k < active.Size; ++k) {
				fill_edge const& e = edges[active[k]];
				slab_edge se = { edge_x_at(e, ya), edge_x_at(e, yb), 0, active[k], e.set };
				slab.push_back(se);
			}
			// By top x then bottom x; linear on the order kept from the previous slab
			for (int i = 1; i < slab.Size; ++i) {
				slab_edge const t = slab[i];
				int j = i - 1;
				while (j >= 0 && (slab[j].xa > t.xa || (slab[j].xa == t.xa && slab[j].xb > t.xb))) { slab[j + 1] = slab[j]; --j; }
				slab[j + 1] = t;
			}
		}
		if (active.Size > 1 && yb - ya > 0.25f) {  // thinner slabs: crossing order is off by < 0.25 px anyway
			int const max_exact = 16;
			for (int i = 1; i < slab.Size; ++i) {
				slab_edge const t = slab[i];
				int j = i - 1;
				float const eps = 1e-5f + 1e-6f * ImFabs(t.xb);
				while (j >= 0 && slab[j].xb > t.xb + eps) {
					if (crossings < max_exact) {
						float const da = slab[j].xa - t.xa, db = slab[j].xb - t.xb;
						float const yc = ya + (yb - ya) * (da / (da - db));
						if (yc > ya + 1e-4f && yc < yb - 1e-4f) sub_ys.push_back(yc);
					}
					++crossings;
					slab[j + 1] = slab[j];
					--j;
				}
				slab[j + 1] = t;
			}
			// Heavily self-intersecting: quarter-pixel sub-slabs instead of exact crossings
			if (crossings > max_exact) {
				sub_ys.resize(1);
				int const n = ImMax(2, ImMin((int)ImCeil((yb - ya) * 4.0f), crossings + 1));
				for (int k = 1; k < n; ++k) sub_ys.push_back(ya + (yb - ya) * (float)k / (float)n);
			}
		}
		if (active.Size > 1)
			for (int k = 0; k < slab.Size; ++k) active[k] = slab[k].id;  // carry the order to the next slab
		sub_ys.push_back(yb);
		if (sub_ys.Size > 2)
			ImQsort(sub_ys.Data, (size_t)sub_ys.Size, sizeof(float), [](void const* a, void const* b) -> int {
				float const fa = *(float const*)a, fb = *(float const*)b; return fa < fb ? -1 : fa > fb ? 1 : 0; });
		for (int s = 0; s + 1 < sub_ys.Size; ++s)
			if (sub_ys[s + 1] - sub_ys[s] > 1e-5f) emit_slab(sub_ys[s], sub_ys[s + 1]);
	}
	flush_all(prev_bottom_y);
	if (aa > 0.0f)
		for (int i = 0; i < prev_bottom.Size; ++i)
			w.FringeOuter(ImVec2(prev_bottom[i].l, prev_bottom_y), ImVec2(prev_bottom[i].r, prev_bottom_y), ImVec2(0.0f, 1.0f));
	w.flush();
}

// ----------------------------------------------------------------------------
// GPU fill (adapted from DearWidgets' winding-number stroke shader)
// ----------------------------------------------------------------------------
// One quad per horizontal band; the pixel shader casts a horizontal ray through the
// band's segments for the winding number and fades the outside half pixel by distance.

static bool s_gpu_enabled = true;

#if IAM_LOTTIE_GPU
static char const s_gpu_hlsl[] = R"(
cbuffer vertexBuffer : register(b0) { float4x4 ProjMtx; };
cbuffer lottieBuffer : register(b1) { float4 params; float4 fillColor; float4 bounds; float4 segments[1024]; };
struct VS_INPUT { float2 pos : POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };
PS_INPUT main_vs(VS_INPUT input) {
	PS_INPUT o;
	o.pos = mul(ProjMtx, float4(input.pos.xy, 0.f, 1.f));
	o.col = input.col;
	o.uv = input.uv;
	return o;
}
float4 main_ps(PS_INPUT input) : SV_Target {
	float2 P = lerp(bounds.xy, bounds.zw, input.uv.xy);
	int num = (int)params.x;
	int winding = 0;
	float d2 = 1e10;
	int across = 0;  // winding change when crossing the nearest segment
	for (int i = 0; i < num; i++) {
		float2 p0 = segments[i].xy, p1 = segments[i].zw;
		float c = (p1.x - p0.x) * (P.y - p0.y) - (P.x - p0.x) * (p1.y - p0.y);
		if (p0.y <= P.y) { if (p1.y > P.y && c > 0.0) winding++; }
		else if (p1.y <= P.y && c < 0.0) winding--;
		float2 d = p1 - p0;
		float l2 = dot(d, d);
		float t = l2 > 1e-8 ? saturate(dot(P - p0, d) / l2) : 0.0;
		float2 q = P - (p0 + d * t);
		float e2 = dot(q, q);
		if (e2 < d2) { d2 = e2; across = c > 0.0 ? 1 : -1; }
	}
	int rule = (int)params.y;
	winding = -winding;  // the ray goes to +x: opposite sign to the CPU sweep, which counts from the left
	bool inside = rule == 0 ? winding != 0 : rule == 1 ? (winding & 1) != 0 : winding >= rule - 1;
	int w2 = winding + across;
	bool inside2 = rule == 0 ? w2 != 0 : rule == 1 ? (w2 & 1) != 0 : w2 >= rule - 1;
	// Inside is opaque so inner edges (stroke joins, overlaps) leave no seam; outside fades
	// only next to an edge whose other side is filled
	float a = inside ? 1.0 : (inside2 ? saturate(0.5 - sqrt(d2)) : 0.0);
	if (a <= 0.0) discard;
	return float4(fillColor.rgb, fillColor.a * a);
}
)";

enum { GPU_MAX_SEGS = 1024, GPU_BAND_SEGS = 256 };
static ImPlatform_ShaderProgram s_gpu_program = nullptr;
static bool s_gpu_tried = false;

static ImPlatform_ShaderProgram gpu_program() {
	if (!s_gpu_enabled) return nullptr;
	if (!s_gpu_tried) {
		s_gpu_tried = true;  // a failed compile is not retried every frame
		ImPlatform_ShaderDesc d;
		memset(&d, 0, sizeof(d));
		d.format = ImPlatform_ShaderFormat_HLSL;
		d.source_code = s_gpu_hlsl;
		d.stage = ImPlatform_ShaderStage_Vertex;
		d.entry_point = "main_vs";
		ImPlatform_Shader const vs = ImPlatform_CreateShader(&d);
		d.stage = ImPlatform_ShaderStage_Fragment;
		d.entry_point = "main_ps";
		ImPlatform_Shader const ps = ImPlatform_CreateShader(&d);
		if (vs && ps) s_gpu_program = ImPlatform_CreateShaderProgram(vs, ps);
	}
	return s_gpu_program;
}

// Callback data (copied by ImDrawList): program handle, then the constant buffer
static void gpu_upload(ImDrawList const*, ImDrawCmd const* cmd) {
	char const* d = (char const*)cmd->UserCallbackData;
	ImPlatform_ShaderProgram prog;
	memcpy(&prog, d, sizeof(prog));
	ImPlatform_SetShaderUniform(prog, "lottieBuffer", d + 16, (unsigned int)cmd->UserCallbackDataSize - 16);
}

struct gpu_band { float y0, y1; int first, count; };

// Splits [y0, y1] until each band holds few enough segments; false if it cannot
static bool gpu_split(ImVec2 const* segs, ImVector<int> const& in, float y0, float y1, int depth, ImVector<gpu_band>& bands, ImVector<int>& idx) {
	int const first = idx.Size;
	for (int i = 0; i < in.Size; i++) {
		ImVec2 const a = segs[in[i] * 2], b = segs[in[i] * 2 + 1];
		if (ImMax(a.y, b.y) >= y0 - 1.0f && ImMin(a.y, b.y) <= y1 + 1.0f) idx.push_back(in[i]);  // +-1: the AA reach
	}
	int const count = idx.Size - first;
	if (count > GPU_BAND_SEGS && y1 - y0 > 2.0f && depth < 16) {
		ImVector<int> mine;
		mine.resize(count);
		memcpy(mine.Data, idx.Data + first, sizeof(int) * (size_t)count);
		idx.resize(first);
		float const mid = (y0 + y1) * 0.5f;
		return gpu_split(segs, mine, y0, mid, depth + 1, bands, idx) && gpu_split(segs, mine, mid, y1, depth + 1, bands, idx);
	}
	if (count > GPU_MAX_SEGS) return false;
	if (count > 0) { gpu_band b = { y0, y1, first, count }; bands.push_back(b); }
	return true;
}

static bool gpu_fill(ImDrawList* dl, ImVec2 const* segs, int n, ImU32 col, int even_odd) {
	ImPlatform_ShaderProgram const prog = gpu_program();
	if (!prog || n <= 0) return false;
	ImRect b(ImVec2(FLT_MAX, FLT_MAX), ImVec2(-FLT_MAX, -FLT_MAX));
	for (int i = 0; i < n * 2; i++) b.Add(segs[i]);
	b.Expand(1.0f);
	ImVec4 const cr = dl->_CmdHeader.ClipRect;
	b.ClipWithFull(ImRect(cr.x, cr.y, cr.z, cr.w));
	if (b.Min.x >= b.Max.x || b.Min.y >= b.Max.y) return true;  // nothing visible
	static ImVector<int> all, idx;
	static ImVector<gpu_band> bands;
	all.resize(n);
	for (int i = 0; i < n; i++) all[i] = i;
	idx.resize(0);
	bands.resize(0);
	if (!gpu_split(segs, all, b.Min.y, b.Max.y, 0, bands, idx)) return false;
	ImVec4 const c = ImGui::ColorConvertU32ToFloat4(col);
	static ImVector<char> buf;
	for (int k = 0; k < bands.Size; k++) {
		gpu_band const& bd = bands[k];
		buf.resize(16 + 48 + bd.count * 16);
		memset(buf.Data, 0, 64);
		memcpy(buf.Data, &prog, sizeof(prog));
		float* f = (float*)(void*)(buf.Data + 16);
		f[0] = (float)bd.count; f[1] = (float)even_odd;
		f[4] = c.x; f[5] = c.y; f[6] = c.z; f[7] = c.w;
		f[8] = b.Min.x; f[9] = bd.y0; f[10] = b.Max.x; f[11] = bd.y1;
		for (int i = 0; i < bd.count; i++) {
			ImVec2 const p0 = segs[idx[bd.first + i] * 2], p1 = segs[idx[bd.first + i] * 2 + 1];
			f[12 + i * 4] = p0.x; f[13 + i * 4] = p0.y; f[14 + i * 4] = p1.x; f[15 + i * 4] = p1.y;
		}
		dl->AddCallback(gpu_upload, buf.Data, (size_t)buf.Size);
		ImPlatform_BeginCustomShader(dl, prog);
		dl->AddImageQuad(ImGui::GetIO().Fonts->TexRef, ImVec2(b.Min.x, bd.y0), ImVec2(b.Max.x, bd.y0), ImVec2(b.Max.x, bd.y1), ImVec2(b.Min.x, bd.y1),
		                 ImVec2(0, 0), ImVec2(1, 0), ImVec2(1, 1), ImVec2(0, 1), IM_COL32_WHITE);
		ImPlatform_EndCustomShader(dl);
	}
	return true;
}
#endif

#if IAM_LOTTIE_OFFSCREEN
// Offscreen layer composite: t0 = the layer drawn into a backbuffer-sized texture
// (premultiplied), t1 = the matte (luma mattes) or a copy of the backbuffer (blend modes)
static char const s_comp_hlsl[] = R"(
cbuffer vertexBuffer : register(b0) { float4x4 ProjMtx; };
// params: x mode (-1 / -2 luma, -3 / -4 alpha matte (inverted), 1..17 blend, 0 normal), y opacity,
// z blur sigma in pixels along params2.xy, w pass (0 composite, 1 blur into a texture, 2 shadow)
// params2.zw: shadow offset in pixels; color: shadow colour
cbuffer compositeBuffer : register(b1) { float4 params; float4 params2; float4 color; };
Texture2D layerTex : register(t0);
Texture2D extraTex : register(t1);
struct VS_INPUT { float2 pos : POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };
PS_INPUT main_vs(VS_INPUT input) {
	PS_INPUT o;
	o.pos = mul(ProjMtx, float4(input.pos.xy, 0.f, 1.f));
	o.col = input.col;
	o.uv = input.uv;
	return o;
}
float lum(float3 c) { return dot(c, float3(0.3, 0.59, 0.11)); }
float3 clip_color(float3 c) {
	float l = lum(c), n = min(c.r, min(c.g, c.b)), x = max(c.r, max(c.g, c.b));
	if (n < 0.0) c = l + (c - l) * l / (l - n);
	if (x > 1.0) c = l + (c - l) * (1.0 - l) / (x - l);
	return c;
}
float3 set_lum(float3 c, float l) { return clip_color(c + (l - lum(c))); }
float sat(float3 c) { return max(c.r, max(c.g, c.b)) - min(c.r, min(c.g, c.b)); }
float3 set_sat(float3 c, float s) {
	float mx = max(c.r, max(c.g, c.b)), mn = min(c.r, min(c.g, c.b));
	return mx > mn ? (c - mn) * s / (mx - mn) : float3(0, 0, 0);
}
float3 soft_light(float3 b, float3 s) {
	float3 d = b <= 0.25 ? ((16.0 * b - 12.0) * b + 4.0) * b : sqrt(b);
	return s <= 0.5 ? b - (1.0 - 2.0 * s) * b * (1.0 - b) : b + (2.0 * s - 1.0) * (d - b);
}
float3 hard_light(float3 b, float3 s) { return s <= 0.5 ? b * 2.0 * s : 1.0 - (1.0 - b) * (1.0 - (2.0 * s - 1.0)); }
// W3C compositing formulas, Lottie / After Effects mode numbers
float3 blend(int m, float3 b, float3 s) {
	if (m == 1) return b * s;
	if (m == 2) return b + s - b * s;
	if (m == 3) return hard_light(s, b);
	if (m == 4) return min(b, s);
	if (m == 5) return max(b, s);
	if (m == 6) return b <= 0.0 ? 0.0 : (s >= 1.0 ? 1.0 : min(1.0, b / (1.0 - s)));
	if (m == 7) return b >= 1.0 ? 1.0 : (s <= 0.0 ? 0.0 : 1.0 - min(1.0, (1.0 - b) / s));
	if (m == 8) return hard_light(b, s);
	if (m == 9) return soft_light(b, s);
	if (m == 10) return abs(b - s);
	if (m == 11) return b + s - 2.0 * b * s;
	if (m == 12) return set_lum(set_sat(s, sat(b)), lum(b));
	if (m == 13) return set_lum(set_sat(b, sat(s)), lum(b));
	if (m == 14) return set_lum(s, lum(b));
	if (m == 15) return set_lum(b, lum(s));
	if (m == 16) return min(1.0, b + s);
	if (m == 17) return (b + s) >= 1.0 ? 1.0 : 0.0;
	return s;
}
float4 fetch(int2 p) {
	float sigma = params.z;
	if (sigma <= 0.0) return layerTex.Load(int3(p, 0));
	// Gaussian along params2.xy, at most 64 taps per side (wider blurs stride)
	int r = (int)ceil(sigma * 3.0);
	int stride = max(1, (r + 63) / 64);
	float4 acc = 0;
	float wsum = 0;
	for (int k = -r; k <= r; k += stride) {
		float w = exp(-0.5 * (k * k) / (sigma * sigma));
		acc += w * layerTex.Load(int3(p + int2(params2.xy * k), 0));
		wsum += w;
	}
	return acc / wsum;
}
float4 main_ps(PS_INPUT input) : SV_Target {
	int2 p = int2(input.pos.xy);
	int3 px = int3(p, 0);
	int pk = (int)params.w;  // pass kind ("pass" is an HLSL keyword)
	if (pk == 2) {
		float sa = fetch(p - int2(params2.zw)).a * color.a;   // shadow: blurred alpha, tinted
		if (sa <= 0.0) discard;
		return float4(color.rgb, sa);
	}
	float4 src = fetch(p);
	if (pk == 1) {
		if (src.a <= 0.0) discard;
		return float4(src.rgb / src.a, src.a);   // stored premultiplied again by the blend state
	}
	if (src.a <= 0.0) discard;
	float3 s = src.rgb / src.a;
	int mode = (int)params.x;
	float a = src.a * params.y;
	if (mode < 0) {
		float4 m = extraTex.Load(px);
		float l = mode >= -2 ? dot(m.rgb, float3(0.299, 0.587, 0.114)) : m.a;   // premultiplied: luma times alpha
		a *= (mode == -1 || mode == -3) ? l : 1.0 - l;
		return float4(s, a);
	}
	if (mode == 0) return float4(s, a);
	float3 b = extraTex.Load(px).rgb;
	return float4(saturate(blend(mode, b, s)), a);
}
)";

static ImPlatform_ShaderProgram s_comp_program = nullptr;
static bool s_comp_tried = false;
static ImTextureID s_rt[3] = { ImTextureID(), ImTextureID(), ImTextureID() };  // layer, matte, blur scratch
static ImTextureID s_bg = ImTextureID();
static unsigned s_rt_w = 0, s_rt_h = 0;

static ImPlatform_ShaderProgram comp_program() {
	if (!s_gpu_enabled) return nullptr;
	if (!s_comp_tried) {
		s_comp_tried = true;
		ImPlatform_ShaderDesc d;
		memset(&d, 0, sizeof(d));
		d.format = ImPlatform_ShaderFormat_HLSL;
		d.source_code = s_comp_hlsl;
		d.stage = ImPlatform_ShaderStage_Vertex;
		d.entry_point = "main_vs";
		ImPlatform_Shader const vs = ImPlatform_CreateShader(&d);
		d.stage = ImPlatform_ShaderStage_Fragment;
		d.entry_point = "main_ps";
		ImPlatform_Shader const ps = ImPlatform_CreateShader(&d);
		if (vs && ps) s_comp_program = ImPlatform_CreateShaderProgram(vs, ps);
	}
	return s_comp_program;
}

// Backbuffer-sized targets (recreated when the backbuffer size changes)
static bool offscreen_targets() {
	unsigned w = 0, h = 0;
	ImPlatform_GetBackbufferSize(&w, &h);
	if (w == 0 || h == 0) return false;
	if (w != s_rt_w || h != s_rt_h || !s_rt[0]) {
		for (int i = 0; i < 3; i++) if (s_rt[i]) ImPlatform_DestroyTexture(s_rt[i]);
		if (s_bg) ImPlatform_DestroyTexture(s_bg);
		ImPlatform_TextureDesc desc = ImPlatform_TextureDesc_Default(w, h);
		desc.format = ImPlatform_PixelFormat_RGBA8;
		for (int i = 0; i < 3; i++) s_rt[i] = ImPlatform_CreateRenderTexture(&desc);
		ImVector<unsigned char> zeros;
		zeros.resize((int)(w * h * 4));
		memset(zeros.Data, 0, (size_t)zeros.Size);
		s_bg = ImPlatform_CreateTexture(zeros.Data, &desc);  // CopyBackbuffer target: same size and format
		s_rt_w = w; s_rt_h = h;
	}
	return s_rt[0] && s_rt[1] && s_rt[2] && s_bg;
}

static void cb_begin_rt(ImDrawList const*, ImDrawCmd const* cmd) { ImTextureID t; memcpy(&t, cmd->UserCallbackData, sizeof(t)); ImPlatform_BeginRenderToTexture(t); }
static void cb_end_rt(ImDrawList const*, ImDrawCmd const*) { ImPlatform_EndRenderToTexture(); }
static void cb_copy_bg(ImDrawList const*, ImDrawCmd const*) { ImPlatform_CopyBackbuffer(s_bg); }
struct comp_cb { ImPlatform_ShaderProgram prog; ImTextureID extra; float params[12]; };
static void cb_composite(ImDrawList const*, ImDrawCmd const* cmd) {
	comp_cb c;
	memcpy(&c, cmd->UserCallbackData, sizeof(c));
	ImPlatform_SetShaderUniform(c.prog, "compositeBuffer", c.params, sizeof(c.params));
	ImPlatform_SetShaderTexture(c.prog, "extraTex", 1, c.extra);
}

static void offscreen_begin(ImDrawList* dl, int i) { dl->AddCallback(cb_begin_rt, &s_rt[i], sizeof(ImTextureID)); }
static void offscreen_end(ImDrawList* dl) { dl->AddCallback(cb_end_rt, nullptr); dl->AddCallback(ImDrawCallback_ResetRenderState, nullptr); }
// One shader pass over the clip rect reading target `src` (t0), into target `dst` (-1 = screen).
// pass 0 composite (mode, extra), 1 blur into a texture, 2 shadow; blur `sigma` px along dir
static void offscreen_pass(ImDrawList* dl, int src, int dst, int pass, int mode, ImTextureID extra, float sigma, ImVec2 dir,
                           ImVec2 offset = ImVec2(0, 0), float const* rgba = nullptr) {
	ImPlatform_ShaderProgram const prog = comp_program();
	if (dst >= 0) offscreen_begin(dl, dst);
	if (pass == 0 && mode > 0) dl->AddCallback(cb_copy_bg, nullptr);
	comp_cb c = { prog, extra ? extra : s_rt[1], { (float)mode, 1.0f, sigma, (float)pass, dir.x, dir.y, offset.x, offset.y,
	              rgba ? rgba[0] : 0.0f, rgba ? rgba[1] : 0.0f, rgba ? rgba[2] : 0.0f, rgba ? rgba[3] : 0.0f } };
	dl->AddCallback(cb_composite, &c, sizeof(c));
	ImPlatform_BeginCustomShader(dl, prog);
	ImVec4 const cr = dl->_CmdHeader.ClipRect;
	dl->AddImageQuad(ImTextureRef(s_rt[src]), ImVec2(cr.x, cr.y), ImVec2(cr.z, cr.y), ImVec2(cr.z, cr.w), ImVec2(cr.x, cr.w));
	ImPlatform_EndCustomShader(dl);
	if (dst >= 0) offscreen_end(dl);
}
#endif

// ----------------------------------------------------------------------------
// Fast paths: Dear ImGui's own convex fill and thick polyline, for the common simple cases
// ----------------------------------------------------------------------------

// Convex and simple (turns one way, and x / y each change direction at most twice)
static bool convex_simple(ImVec2 const* p, int n) {
	if (n < 3) return false;
	int sign = 0, xflips = 0, yflips = 0, xs = 0, ys = 0, xs0 = 0, ys0 = 0;
	ImVec2 prev(0, 0);
	bool have_prev = false;
	for (int i = 0; i <= n; i++) {
		ImVec2 const e(p[(i + 1) % n].x - p[i % n].x, p[(i + 1) % n].y - p[i % n].y);
		if (e.x == 0.0f && e.y == 0.0f) continue;
		if (have_prev) {
			float const cr = prev.x * e.y - prev.y * e.x;
			if (ImFabs(cr) > 1e-6f * (ImFabs(prev.x) + ImFabs(prev.y)) * (ImFabs(e.x) + ImFabs(e.y))) {
				int const sg = cr > 0.0f ? 1 : -1;
				if (sign == 0) sign = sg; else if (sg != sign) return false;
			}
		}
		if (i < n) {
			int const sx = e.x > 0.0f ? 1 : e.x < 0.0f ? -1 : 0, sy = e.y > 0.0f ? 1 : e.y < 0.0f ? -1 : 0;
			if (sx) { if (xs && sx != xs) xflips++; if (!xs0) xs0 = sx; xs = sx; }
			if (sy) { if (ys && sy != ys) yflips++; if (!ys0) ys0 = sy; ys = sy; }
		}
		prev = e;
		have_prev = true;
	}
	if (xs && xs0 && xs != xs0) xflips++;  // wrap around
	if (ys && ys0 && ys != ys0) yflips++;
	return sign != 0 && xflips <= 2 && yflips <= 2;
}

// No turn sharper than ~60 degrees (where Dear ImGui's mitred thick lines differ from Lottie joins)
static bool smooth_polyline(ImVec2 const* p, int n, bool closed) {
	int const m = closed ? n : n - 2;
	for (int i = 0; i < m; i++) {
		ImVec2 const a = p[i], b = p[(i + 1) % n], c = p[(i + 2) % n];
		ImVec2 const d0(b.x - a.x, b.y - a.y), d1(c.x - b.x, c.y - b.y);
		float const l = ImSqrt((d0.x * d0.x + d0.y * d0.y) * (d1.x * d1.x + d1.y * d1.y));
		if (l > 0.0f && (d0.x * d1.x + d0.y * d1.y) < 0.5f * l) return false;
	}
	return true;
}

// ----------------------------------------------------------------------------
// Stroke outlines (pieces filled once with non-zero, so overlaps never darken)
// ----------------------------------------------------------------------------

// Appends a polygon with positive orientation so every piece adds to the non-zero union
static void add_poly(ImVector<ImVec2>& segs, ImVec2 const* p, int n, bool negative = false) {
	float area = 0.0f;
	for (int i = 0; i < n; i++) { ImVec2 const a = p[i], b = p[(i + 1) % n]; area += a.x * b.y - b.x * a.y; }
	if (ImFabs(area) < 1e-6f) return;
	bool const fwd = (area > 0.0f) != negative;
	for (int i = 0; i < n; i++) {
		ImVec2 const a = p[i], b = p[(i + 1) % n];
		segs.push_back(fwd ? a : b);
		segs.push_back(fwd ? b : a);
	}
}

static int arc_steps(float sweep, float r, float tol) {
	if (r <= tol) return 1;
	return ImClamp((int)ImCeil(ImFabs(sweep) / (2.0f * acosf(1.0f - tol / r))), 1, 128);
}

static void add_circle(ImVector<ImVec2>& segs, ImVec2 c, float r, float tol) {
	ImVec2 pts[130];
	int const n = ImMax(8, arc_steps(2.0f * IM_PI, r, tol));
	for (int i = 0; i < n; i++) { float const a = 2.0f * IM_PI * i / n; pts[i] = ImVec2(c.x + ImCos(a) * r, c.y + ImSin(a) * r); }
	add_poly(segs, pts, n);
}

// cap: 1 butt, 2 round, 3 square; join: 1 miter, 2 round, 3 bevel
static void stroke_polyline(ImVector<ImVec2>& segs, ImVec2 const* in, int n_in, bool closed, float hw, int cap, int join, float miter, float tol) {
	// Scratch kept across calls (not re-entered)
	static ImVector<ImVec2> q, d, A, B, rb;
	q.resize(0); A.resize(0); B.resize(0); rb.resize(0);
	for (int i = 0; i < n_in; i++)
		if (q.Size == 0 || ImFabs(in[i].x - q.back().x) > 1e-4f || ImFabs(in[i].y - q.back().y) > 1e-4f) q.push_back(in[i]);
	if (closed && q.Size > 1 && ImFabs(q[0].x - q.back().x) <= 1e-4f && ImFabs(q[0].y - q.back().y) <= 1e-4f) q.pop_back();
	int const m = q.Size;
	if (m == 0 || hw <= 0.0f) return;
	if (m == 1) {
		// Zero-length: a dot for round / square caps
		if (cap == 2) add_circle(segs, q[0], hw, tol);
		else if (cap == 3) { ImVec2 const r[4] = { ImVec2(q[0].x - hw, q[0].y - hw), ImVec2(q[0].x + hw, q[0].y - hw), ImVec2(q[0].x + hw, q[0].y + hw), ImVec2(q[0].x - hw, q[0].y + hw) }; add_poly(segs, r, 4); }
		return;
	}
	if (m == 2) closed = false;
	int const nseg = closed ? m : m - 1;
	d.resize(nseg);
	for (int i = 0; i < nseg; i++) {
		ImVec2 const a = q[i], b = q[(i + 1) % m];
		float const l = ImSqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
		d[i] = ImVec2((b.x - a.x) / l, (b.y - a.y) / l);
	}
	// Outline: the offset chain of each side (one loop when open, two when closed). The
	// straight link between offsets is the bevel; on the inner side the chain goes through
	// the vertex itself, so inner overlaps add up under non-zero instead of cancelling.
	for (int i = 0; i < nseg; i++) {
		ImVec2 const a = q[i], b = q[(i + 1) % m], nn(-d[i].y * hw, d[i].x * hw);
		if (i > 0 || closed) {
			ImVec2 const d0 = d[(i - 1 + nseg) % nseg];
			float const cr = d0.x * d[i].y - d0.y * d[i].x;
			if (cr > 0.0f) A.push_back(a); else if (cr < 0.0f) B.push_back(a);  // turning toward +n: +n is inner
		}
		A.push_back(ImVec2(a.x + nn.x, a.y + nn.y)); A.push_back(ImVec2(b.x + nn.x, b.y + nn.y));
		B.push_back(ImVec2(a.x - nn.x, a.y - nn.y)); B.push_back(ImVec2(b.x - nn.x, b.y - nn.y));
	}
	if (!closed && cap == 3) {
		ImVec2 const d0 = d[0], d1 = d[nseg - 1];
		A[0].x -= d0.x * hw; A[0].y -= d0.y * hw; B[0].x -= d0.x * hw; B[0].y -= d0.y * hw;
		A.back().x += d1.x * hw; A.back().y += d1.y * hw; B.back().x += d1.x * hw; B.back().y += d1.y * hw;
	}
	for (int i = B.Size - 1; i >= 0; i--) rb.push_back(B[i]);
	if (!closed) { for (int i = 0; i < rb.Size; i++) A.push_back(rb[i]); rb.resize(0); }
	float area = 0.0f;
	ImVector<ImVec2>* const loops[2] = { &A, &rb };
	for (int l = 0; l < 2; l++) {
		ImVector<ImVec2> const& p = *loops[l];
		for (int i = 0; i < p.Size; i++) { ImVec2 const a = p[i], b = p[(i + 1) % p.Size]; area += a.x * b.y - b.x * a.y; }
	}
	for (int l = 0; l < 2; l++) {
		ImVector<ImVec2> const& p = *loops[l];
		for (int i = 0; i < p.Size; i++) {
			ImVec2 const a = p[i], b = p[(i + 1) % p.Size];
			segs.push_back(area >= 0.0f ? a : b);  // positive like add_poly, so join and cap pieces add up
			segs.push_back(area >= 0.0f ? b : a);
		}
	}
	// Round / miter joins: extra pieces on the outer side, only where they leave the bevel
	for (int i = closed ? 0 : 1; join != 3 && i < (closed ? m : m - 1); i++) {
		ImVec2 const v = q[i], d0 = d[(i - 1 + nseg) % nseg], d1 = d[i % nseg];
		float const cr = d0.x * d1.y - d0.y * d1.x;
		if (cr == 0.0f && d0.x * d1.x + d0.y * d1.y > 0.0f) continue;
		float const s = cr > 0.0f ? -1.0f : 1.0f;  // outer side
		ImVec2 const n0(-d0.y * s, d0.x * s), n1(-d1.y * s, d1.x * s);
		ImVec2 const P0(v.x + n0.x * hw, v.y + n0.y * hw), P1(v.x + n1.x * hw, v.y + n1.y * hw);
		if (join == 2) {
			float const a0 = ImAtan2(n0.y, n0.x);
			float sweep = ImAtan2(n1.y, n1.x) - a0;
			while (sweep > IM_PI) sweep -= 2.0f * IM_PI;
			while (sweep < -IM_PI) sweep += 2.0f * IM_PI;
			int const steps = arc_steps(sweep, hw, tol);
			if (steps < 2) continue;  // the bevel is within tolerance
			ImVec2 fan[131];
			fan[0] = v;
			for (int k = 0; k <= steps; k++) { float const a = a0 + sweep * k / steps; fan[k + 1] = ImVec2(v.x + ImCos(a) * hw, v.y + ImSin(a) * hw); }
			add_poly(segs, fan, steps + 2);
		} else {
			float mx = n0.x + n1.x, my = n0.y + n1.y;
			float const ml = ImSqrt(mx * mx + my * my);
			if (ml <= 1e-6f) continue;
			mx /= ml; my /= ml;
			float const cosh = mx * n0.x + my * n0.y;
			if (cosh <= 1e-4f || 1.0f / cosh > miter || hw * (1.0f / cosh - cosh) < tol) continue;
			ImVec2 const tip(v.x + mx * hw / cosh, v.y + my * hw / cosh);
			ImVec2 const quad[4] = { v, P0, tip, P1 };
			add_poly(segs, quad, 4);
		}
	}
	if (!closed && cap == 2) { add_circle(segs, q[0], hw, tol); add_circle(segs, q[m - 1], hw, tol); }
}

// Splits a polyline into dashes; calls stroke for each piece
static void dash_polyline(ImVector<ImVec2>& segs, ImVector<ImVec2> const& p, bool closed, float const* pattern, int count, float offset,
                          float hw, int cap, int join, float miter, float tol) {
	float total = 0.0f;
	for (int i = 0; i < count; i++) total += ImMax(pattern[i], 0.0f);
	if (total <= 0.0f || poly_length(p, closed) / total > 10000.0f) {  // degenerate pattern: solid
		stroke_polyline(segs, p.Data, p.Size, closed, hw, cap, join, miter, tol);
		return;
	}
	float phase = ImFmod(offset, total);
	if (phase < 0.0f) phase += total;
	int idx = 0;
	while (phase >= ImMax(pattern[idx], 0.0f)) { phase -= ImMax(pattern[idx], 0.0f); idx = (idx + 1) % count; }
	float left = ImMax(pattern[idx], 0.0f) - phase;
	bool on = (idx % 2) == 0;
	static ImVector<ImVec2> piece;
	piece.resize(0);
	if (on) piece.push_back(p[0]);
	int const n = closed ? p.Size : p.Size - 1;
	for (int i = 0; i < n; i++) {
		ImVec2 a = p[i];
		ImVec2 const b = p[(i + 1) % p.Size];
		float l = ImSqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
		while (l > 0.0f) {
			if (left >= l) {
				left -= l;
				if (on) piece.push_back(b);
				break;
			}
			float const t = left / l;
			a = ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
			l -= left;
			if (on) { piece.push_back(a); stroke_polyline(segs, piece.Data, piece.Size, false, hw, cap, join, miter, tol); piece.resize(0); }
			else piece.push_back(a);
			on = !on;
			idx = (idx + 1) % count;
			left = ImMax(pattern[idx], 0.0f);
			if (!on) piece.resize(0);
		}
	}
	if (on && piece.Size > 1) stroke_polyline(segs, piece.Data, piece.Size, false, hw, cap, join, miter, tol);
}

// ----------------------------------------------------------------------------
// Rendering
// ----------------------------------------------------------------------------

struct ctx {
	ImDrawList* dl;
	iam_lottie const* L;
	clip_t clips[MAX_CLIPS];
	int nclips, overflow, depth;
	ImVector<ImVec2>* collect;  // matte capture: fills / strokes append their outline here
	ImVector<float> scratch;
	struct active_fx { int first, count; float t; };
	active_fx fx_stack[16];    // layers being drawn that carry effects, outermost first
	int nfx;
	int cur_comp;              // composition being drawn, and its size (3D projection centre)
	float cur_w, cur_h;
	bool offscreen;            // drawing into an offscreen layer (no nesting)
	int skip_matte;            // layer whose matte is applied by the composite
	int gpu_fx_layer;          // layer whose blur / shadow the GPU draws
	// CPU blur / drop shadow: a soft edge (fringe width in pixels) and a recoloured, offset pass
	float soft;
	bool shadow;
	ImVec2 shadow_ofs;
	float shadow_rgba[4];

	// Colour through the active Fill / Tint effects, innermost layer first
	void fx_color(float* rgb) {
		if (collect) return;
		for (int s = nfx - 1; s >= 0; s--)
			for (int i = 0; i < fx_stack[s].count; i++) {
				effect_t const& fx = L->effects[fx_stack[s].first + i];
				float const t = fx_stack[s].t;
				if (fx.kind != FX_FILL && fx.kind != FX_TINT) continue;
				float c0[3], c1[3];
				eval(L, fx.v[fx.kind == FX_FILL ? 2 : 0], t, scratch);
				for (int k = 0; k < 3; k++) c0[k] = k < scratch.Size ? scratch[k] : 0.0f;
				float target[3];
				float amount;
				if (fx.kind == FX_FILL) {
					for (int k = 0; k < 3; k++) target[k] = c0[k];
					amount = pv(fx.v[6], t, 0, 1.0f);
				} else {
					eval(L, fx.v[1], t, scratch);
					for (int k = 0; k < 3; k++) c1[k] = k < scratch.Size ? scratch[k] : 1.0f;
					float const luma = rgb[0] * 0.299f + rgb[1] * 0.587f + rgb[2] * 0.114f;
					for (int k = 0; k < 3; k++) target[k] = c0[k] + (c1[k] - c0[k]) * luma;
					amount = pv(fx.v[2], t, 0, 100.0f) * 0.01f;
				}
				amount = ImSaturate(amount);
				for (int k = 0; k < 3; k++) rgb[k] += (target[k] - rgb[k]) * amount;
			}
	}
	float tol;

	float pv(prop_t const& p, float t, int i, float def) {
		if (!present(p)) return def;
		eval(L, p, t, scratch);
		return i < scratch.Size ? scratch[i] : def;
	}
	void pv2(prop_t const& p, float t, float def, float out[2]) {
		out[0] = out[1] = def;
		if (!present(p)) return;
		eval(L, p, t, scratch);
		if (scratch.Size > 0) out[0] = scratch[0];
		if (scratch.Size > 1) out[1] = scratch[1];
	}

	void position(xform_t const& x, float t, float p[2]) {
		if (x.split) { p[0] = pv(x.px, t, 0, 0.0f); p[1] = pv(x.py, t, 0, 0.0f); }
		else pv2(x.p, t, 0.0f, p);
	}

	// auto_orient: also rotate along the motion path (layer "ao")
	mat xform(xform_t const& x, float t, float* opacity, bool auto_orient = false) {
		float a[2], p[2], s[2];
		pv2(x.a, t, 0.0f, a);
		position(x, t, p);
		pv2(x.s, t, 100.0f, s);
		float r = pv(x.r, t, 0, 0.0f);
		float const sk = pv(x.sk, t, 0, 0.0f), sa = pv(x.sa, t, 0, 0.0f);
		if (auto_orient) {
			// Tangent from a small step ahead (behind at the very end of the motion)
			float q[2];
			position(x, t + 0.05f, q);
			float dx = q[0] - p[0], dy = q[1] - p[1];
			if (dx * dx + dy * dy < 1e-10f) { position(x, t - 0.05f, q); dx = p[0] - q[0]; dy = p[1] - q[1]; }
			if (dx * dx + dy * dy >= 1e-10f) r += ImAtan2(dy, dx) * (180.0f / IM_PI);
		}
		if (opacity) *opacity = pv(x.o, t, 0, 100.0f) * 0.01f;
		mat m = mat_make(s[0] * 0.01f, 0, 0, s[1] * 0.01f, -a[0] * s[0] * 0.01f, -a[1] * s[1] * 0.01f);
		if (sk != 0.0f) m = mul(mul(mul(rot(-sa), mat_make(1, 0, tanf(-sk * (IM_PI / 180.0f)), 1, 0, 0)), rot(sa)), m);
		m = mul(rot(r), m);
		m.x += p[0]; m.y += p[1];
		return m;
	}

	void push_clip(ImVector<ImVec2> const& segs, int even_odd) {
		if (nclips >= MAX_CLIPS) { overflow++; return; }  // shortcut: deeper clips are ignored, raise MAX_CLIPS if files need more
		clip_t& c = clips[nclips++];
		c.segs.resize(segs.Size);
		if (segs.Size) memcpy(c.segs.Data, segs.Data, (size_t)segs.Size * sizeof(ImVec2));
		c.even_odd = even_odd;
		c.bounds = ImRect(ImVec2(FLT_MAX, FLT_MAX), ImVec2(-FLT_MAX, -FLT_MAX));
		for (int i = 0; i < segs.Size; i++) c.bounds.Add(segs[i]);
	}
	void pop_clip() { if (overflow > 0) overflow--; else if (nclips > 0) nclips--; }

	// Region outside `segs` (positive outlines, winding -1 each, possibly overlapping): a big
	// rectangle winding +1 plus the outlines as they are, filled where winding >= 1 (rule 2), so
	// a point is outside only when no outline covers it
	void complement(ImVector<ImVec2>& out, ImVector<ImVec2> const& segs) {
		ImVec4 const cr = dl->_CmdHeader.ClipRect;
		ImVec2 const r[4] = { ImVec2(cr.x - 4, cr.y - 4), ImVec2(cr.z + 4, cr.y - 4), ImVec2(cr.z + 4, cr.w + 4), ImVec2(cr.x - 4, cr.w + 4) };
		out.resize(0);
		add_poly(out, r, 4, true);
		for (int i = 0; i < segs.Size; i++) out.push_back(segs[i]);
	}

	// Soft edge `width` wide (blur, shadow softness): the shape drawn grown / shrunk at K
	// distances across the edge, each layer's opacity chosen so the stack composites to a
	// linear ramp. Grown = shape OR a stroke of its outline; shrunk = shape minus that stroke.
	void soft_fill(ImVector<ImVec2> const& segs, ImU32 col, int even_odd, paint_t const* paint, float width) {
		// Outline contours from the segment soup (fills and stroke outlines are chained loops)
		static ImVector<ImVec2> pts, stroke, comp;  // soft_fill is not re-entered
		static ImVector<int> counts;
		static ImVector<bool> closed;
		pts.resize(0); counts.resize(0); closed.resize(0);
		for (int i = 0; i < segs.Size; i += 2) {
			if (i == 0 || segs[i].x != segs[i - 1].x || segs[i].y != segs[i - 1].y) {
				if (counts.Size) closed.back() = pts.Size > 0 && pts.back().x == pts[pts.Size - counts.back()].x && pts.back().y == pts[pts.Size - counts.back()].y;
				counts.push_back(0); closed.push_back(false);
				pts.push_back(segs[i]); counts.back()++;
			}
			pts.push_back(segs[i + 1]); counts.back()++;
		}
		if (counts.Size) closed.back() = pts.back().x == pts[pts.Size - counts.back()].x && pts.back().y == pts[pts.Size - counts.back()].y;
		float const h = width * 0.5f;
		int const K = ImClamp((int)(width / 2.0f + 0.5f), 2, 12);
		float const base_a = ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
		for (int j = 0; j < K; j++) {
			float const d = h - (j + 0.5f) * (width / K);   // outermost first
			float const a = 1.0f / (K - j);                  // so that the stack ends fully covered
			stroke.resize(0);
			int ofs = 0;
			for (int c = 0; c < counts.Size; c++) {
				if (counts[c] >= 2) stroke_polyline(stroke, pts.Data + ofs, counts[c], closed[c], ImFabs(d), 2, 2, 4.0f, tol);
				ofs += counts[c];
			}
			paint_t pj;
			if (paint) { pj = *paint; for (int k = 0; k < pj.n; k++) pj.rgba[k][3] *= a; }
			ImU32 const cj = (col & ~IM_COL32_A_MASK) | ((ImU32)(ImSaturate(base_a * a) * 255.0f + 0.5f) << IM_COL32_A_SHIFT);
			if (d >= 0.0f) {
				fill_segments(dl, segs.Data, segs.Size / 2, cj, even_odd, 1.0f, paint ? &pj : nullptr, clips, nclips, &stroke);
			} else {
				complement(comp, stroke);
				push_clip(comp, 2);
				fill_segments(dl, segs.Data, segs.Size / 2, cj, even_odd, 1.0f, paint ? &pj : nullptr, clips, nclips);
				pop_clip();
			}
		}
	}

	void fill(ImVector<ImVec2> const& segs, ImU32 col, int even_odd, paint_t const* paint) {
		if (collect) { for (int i = 0; i < segs.Size; i++) collect->push_back(segs[i]); return; }
		if (shadow) {
			// Shadow pass: the same coverage, offset, in the shadow colour
			static ImVector<ImVec2> moved;
			moved.resize(segs.Size);
			for (int i = 0; i < segs.Size; i++) moved[i] = ImVec2(segs[i].x + shadow_ofs.x, segs[i].y + shadow_ofs.y);
			float const a = (paint ? 1.0f : ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f) * shadow_rgba[3];
			ImU32 const sc = ImGui::ColorConvertFloat4ToU32(ImVec4(shadow_rgba[0], shadow_rgba[1], shadow_rgba[2], a));
			if (soft > 2.0f) soft_fill(moved, sc, even_odd, nullptr, soft);
			else fill_segments(dl, moved.Data, moved.Size / 2, sc, even_odd, 1.0f, nullptr, clips, nclips);
			return;
		}
#if IAM_LOTTIE_GPU
		if (!paint && nclips == 0 && soft <= 1.0f && gpu_fill(dl, segs.Data, segs.Size / 2, col, even_odd)) return;  // gradients, clips and blur stay on the CPU
#endif
		if (soft > 2.0f) { soft_fill(segs, col, even_odd, paint, soft); return; }
		fill_segments(dl, segs.Data, segs.Size / 2, col, even_odd, 1.0f, paint, clips, nclips);
	}

	void build_shape(shape_t const& sh, mat const& M, float t, geom& g) {
		float const k = 0.5519f;
		bool const rev = sh.dir == 3;
		switch (sh.kind) {
			case SK_PATH: {
				eval(L, sh.p[0], t, scratch);
				if (scratch.Size < 2) return;
				int const n = (int)scratch[1];
				if (n <= 0 || scratch.Size < 2 + 6 * n) return;
				add_vertices(g, M, scratch.Data + 2, n, scratch[0] != 0.0f, false);
				break;
			}
			case SK_RECT: {
				float p[2], s[2];
				pv2(sh.p[0], t, 0.0f, p);
				pv2(sh.p[1], t, 0.0f, s);
				float const hw = s[0] * 0.5f, hh = s[1] * 0.5f;
				float const r = ImMin(pv(sh.p[2], t, 0, 0.0f), ImMin(ImFabs(hw), ImFabs(hh)));
				float const L0 = p[0] - hw, R0 = p[0] + hw, T0 = p[1] - hh, B0 = p[1] + hh;
				if (r <= 0.0f) {
					float const v[24] = { R0, T0, 0, 0, 0, 0, R0, B0, 0, 0, 0, 0, L0, B0, 0, 0, 0, 0, L0, T0, 0, 0, 0, 0 };
					add_vertices(g, M, v, 4, true, rev);
				} else {
					float const c = k * r;
					float const v[48] = {
						R0, T0 + r, 0, -c, 0, 0,   R0, B0 - r, 0, 0, 0, c,
						R0 - r, B0, c, 0, 0, 0,    L0 + r, B0, 0, 0, -c, 0,
						L0, B0 - r, 0, c, 0, 0,    L0, T0 + r, 0, 0, 0, -c,
						L0 + r, T0, -c, 0, 0, 0,   R0 - r, T0, 0, 0, c, 0 };
					add_vertices(g, M, v, 8, true, rev);
				}
				break;
			}
			case SK_ELLIPSE: {
				float p[2], s[2];
				pv2(sh.p[0], t, 0.0f, p);
				pv2(sh.p[1], t, 0.0f, s);
				float const rx = s[0] * 0.5f, ry = s[1] * 0.5f, cx = k * rx, cy = k * ry;
				float const v[24] = {
					p[0], p[1] - ry, -cx, 0, cx, 0,     p[0] + rx, p[1], 0, -cy, 0, cy,
					p[0], p[1] + ry, cx, 0, -cx, 0,     p[0] - rx, p[1], 0, cy, 0, -cy };
				add_vertices(g, M, v, 4, true, rev);
				break;
			}
			case SK_STAR: {
				float p[2];
				pv2(sh.p[0], t, 0.0f, p);
				float const orad = pv(sh.p[1], t, 0, 0.0f), irad = pv(sh.p[2], t, 0, 0.0f);
				float const oround = pv(sh.p[3], t, 0, 0.0f) * 0.01f, iround = pv(sh.p[4], t, 0, 0.0f) * 0.01f;
				float const rotation = pv(sh.p[5], t, 0, 0.0f) * (IM_PI / 180.0f);
				int const pts = ImClamp((int)ImFloor(pv(sh.p[6], t, 0, 5.0f)), 1, 1000);
				bool const star = sh.mode != 2;
				int const n = star ? pts * 2 : pts;
				float const dir = rev ? -1.0f : 1.0f;
				float const step = 2.0f * IM_PI / n;
				static ImVector<float> v;
				v.resize(n * 6);
				float ang = -IM_PI * 0.5f + rotation;
				for (int i = 0; i < n; i++) {
					bool const outer = !star || (i % 2) == 0;
					float const rad = outer ? orad : irad, round = outer ? oround : iround;
					float const perim = star ? 2.0f * IM_PI * rad / (n * 2) : 2.0f * IM_PI * rad / (n * 4);
					float const x = rad * ImCos(ang), y = rad * ImSin(ang);
					float const len = ImSqrt(x * x + y * y);
					float const ox = len > 0.0f ? y / len : 0.0f, oy = len > 0.0f ? -x / len : 0.0f;
					float const h = perim * round * dir;
					float* q = &v[i * 6];
					q[0] = x + p[0]; q[1] = y + p[1];
					q[2] = ox * h; q[3] = oy * h;      // in
					q[4] = -ox * h; q[5] = -oy * h;    // out
					ang += step * dir;
				}
				add_vertices(g, M, v.Data, n, true, false);
				break;
			}
			default: break;
		}
	}

	// Geometry of items [0, upto) of a shape list, in screen space, with modifiers applied
	void collect_geom(int ofs, int upto, mat const& M, float t, geom& out) {
		for (int i = 0; i < upto; i++) {
			shape_t const& sh = L->shapes[L->children[ofs + i]];
			if (sh.hidden) continue;
			switch (sh.kind) {
				case SK_PATH: case SK_RECT: case SK_ELLIPSE: case SK_STAR: build_shape(sh, M, t, out); break;
				case SK_GROUP: {
					if (depth > 64) break;
					depth++;
					// Pool by depth: nested collections are always deeper than the one using them
					static geom s_sub[66];
					geom& sub = s_sub[depth];
					sub.clear();
					collect_geom(sh.child, sh.childn, mul(M, xform(sh.tr, t, nullptr)), t, sub);
					depth--;
					out.append(sub);
					break;
				}
				case SK_TRIM:
					trim(out, pv(sh.p[0], t, 0, 0.0f) * 0.01f, pv(sh.p[1], t, 0, 100.0f) * 0.01f, pv(sh.p[2], t, 0, 0.0f) / 360.0f, sh.mode, tol);
					break;
				case SK_ROUND: round_corners(out, pv(sh.p[0], t, 0, 0.0f) * mat_scale(M)); break;
				case SK_PUCKER: pucker_bloat(out, pv(sh.p[0], t, 0, 0.0f)); break;
				case SK_MERGE: merge_paths(out, sh.mode, tol); break;
				case SK_TWIST: {
					float c[2];
					pv2(sh.p[1], t, 0.0f, c);
					twist(out, pv(sh.p[0], t, 0, 0.0f), apply(M, c[0], c[1]), tol);
					break;
				}
				case SK_OFFSET: offset_path(out, pv(sh.p[0], t, 0, 0.0f) * mat_scale(M), sh.join, sh.miter, tol); break;  // shortcut: strokes follow the original paths  // shortcut: strokes still follow the original contours
				case SK_ZIGZAG:
					zigzag(out, pv(sh.p[0], t, 0, 0.0f) * mat_scale(M), ImClamp((int)pv(sh.p[1], t, 0, 0.0f), 0, 1000), pv(sh.p[2], t, 0, 1.0f) >= 1.5f, tol);
					break;
				default: break;  // shortcut: a repeater only repeats what it styles, not geometry seen by later styles
			}
		}
	}

	void gradient(shape_t const& sh, mat const& M, float t, float alpha, paint_t& pt) {
		float s[2], e[2];
		pv2(sh.p[1], t, 0.0f, s);
		pv2(sh.p[2], t, 0.0f, e);
		pt.type = sh.mode == 2 ? 2 : 1;
		pt.p0 = apply(M, s[0], s[1]);
		pt.p1 = apply(M, e[0], e[1]);
		pt.focal = pt.p0;
		float const h = ImClamp(pv(sh.p[5], t, 0, 0.0f) * 0.01f, -0.99f, 0.99f);
		if (pt.type == 2 && h != 0.0f) {
			// Highlight: along the start -> end direction turned by the angle, at h of the radius (lottie-web)
			float const ang = ImAtan2(e[1] - s[1], e[0] - s[0]) + pv(sh.p[6], t, 0, 0.0f) * (IM_PI / 180.0f);
			float const dist = ImSqrt((e[0] - s[0]) * (e[0] - s[0]) + (e[1] - s[1]) * (e[1] - s[1])) * h;
			pt.focal = apply(M, s[0] + ImCos(ang) * dist, s[1] + ImSin(ang) * dist);
		}
		eval(L, sh.p[3], t, scratch);
		int const n = ImMin(ImMin(sh.gcount, (int)MAX_STOPS), scratch.Size / 4);
		pt.n = ImMax(n, 1);
		int const alpha_ofs = sh.gcount * 4, na = (scratch.Size - alpha_ofs) / 2;
		for (int i = 0; i < pt.n; i++) {
			float const* c = n > 0 ? &scratch[i * 4] : nullptr;
			pt.off[i] = c ? c[0] : 0.0f;
			pt.rgba[i][0] = c ? c[1] : 1.0f; pt.rgba[i][1] = c ? c[2] : 1.0f; pt.rgba[i][2] = c ? c[3] : 1.0f;
			float a = 1.0f;
			if (na > 0) {
				// shortcut: alpha stops are sampled at the colour stops only
				float const* as = &scratch[alpha_ofs];
				int j = 0;
				while (j < na && as[j * 2] < pt.off[i]) j++;
				if (j == 0) a = as[1];
				else if (j >= na) a = as[(na - 1) * 2 + 1];
				else { float const o0 = as[(j - 1) * 2], o1 = as[j * 2], f = o1 > o0 ? (pt.off[i] - o0) / (o1 - o0) : 0.0f; a = as[(j - 1) * 2 + 1] + (as[j * 2 + 1] - as[(j - 1) * 2 + 1]) * f; }
			}
			pt.rgba[i][3] = ImSaturate(a) * alpha;
		}
		for (int i = 0; i < pt.n; i++) fx_color(pt.rgba[i]);
	}

	void fill_geom(geom const& g, ImU32 col) {
		static ImVector<ImVec2> poly, segs;  // fill_geom is not re-entered
		segs.resize(0);
		for (int c = 0; c < g.cs.Size; c++) {
			flatten_contour(g, g.cs[c], poly, tol);
			if (poly.Size < 3) continue;
			if (collect) { add_poly(segs, poly.Data, poly.Size); continue; }
			for (int i = 0; i < poly.Size; i++) { segs.push_back(poly[i]); segs.push_back(poly[(i + 1) % poly.Size]); }
		}
		fill(segs, col, 0, nullptr);
	}

	glyph_t const* find_glyph(unsigned cp, int font) const {
		glyph_t const* any = nullptr;
		for (int i = 0; i < L->glyphs.Size; i++) {
			glyph_t const& g = L->glyphs[i];
			if (g.cp != cp) continue;
			if (font < 0 || (strcmp(L->strings.Data + g.family, L->strings.Data + L->fonts[font].family) == 0 &&
			                 strcmp(L->strings.Data + g.style, L->strings.Data + L->fonts[font].style) == 0)) return &g;
			if (!any) any = &g;
		}
		return any;
	}

	// Text layer: lines laid out from the embedded glyph outlines; paragraph text wraps at
	// spaces to its box width (lottie-web: first baseline at the box top + the font ascent)
	void draw_text(layer_t const& ly, mat const& W, float alpha, float lt) {
		if (ly.docn == 0 || L->glyphs.Size == 0) return;
		int di = 0;
		for (int i = 0; i < ly.docn; i++) if (L->docs[ly.doc + i].t <= lt) di = i;
		textdoc_t const& d = L->docs[ly.doc + di];
		float const scale = d.size * 0.01f, track = d.tracking * 0.001f * d.size;
		static geom g;  // draw_text is not re-entered
		g.clear();
		auto advance = [&](unsigned cp) { glyph_t const* gl = find_glyph(cp, d.font); return (gl ? gl->w * scale : 0.0f) + track; };
		bool const boxed = d.box[0] > 0.0f;
		// Lines: hard breaks, then (paragraph text) greedy wrapping at the last space that fits
		static ImVector<char const*> lines;  // begin, end pairs
		lines.resize(0);
		for (char const* line = L->strings.Data + d.text; ; ) {
			char const* e = line;
			while (*e && *e != '\r' && *e != '\n' && *e != 3) e++;
			char const* ls = line;
			char const* sp = nullptr;
			float w = 0.0f;
			for (char const* c = line; boxed && c < e; ) {
				unsigned cp = 0;
				int const n = ImTextCharFromUtf8(&cp, c, e);
				float const aw = advance(cp);
				if (cp != ' ' && c > ls && w + aw > d.box[0]) {
					char const* const brk = sp ? sp : c;
					lines.push_back(ls); lines.push_back(brk);
					ls = c = sp ? sp + 1 : c;
					sp = nullptr; w = 0.0f;
					continue;
				}
				if (cp == ' ') sp = c;
				w += aw;
				c += n;
			}
			lines.push_back(ls); lines.push_back(e);
			if (!*e) break;
			line = e + 1;
		}
		float const ascent = d.font >= 0 ? L->fonts[d.font].ascent * d.size * 0.01f : d.size * 0.75f;
		float y = boxed ? d.box_pos[1] + ascent : 0.0f;
		for (int li = 0; li < lines.Size; li += 2, y += d.lh) {
			char const* b = lines[li];
			char const* e = lines[li + 1];
			while (boxed && e > b && e[-1] == ' ') e--;  // wrapped lines align without their trailing spaces
			float width = 0.0f;
			for (char const* c = b; c < e; ) { unsigned cp = 0; c += ImTextCharFromUtf8(&cp, c, e); width += advance(cp); }
			float x = boxed ? d.box_pos[0] + (d.justify == 1 ? d.box[0] - width : d.justify == 2 ? (d.box[0] - width) * 0.5f : 0.0f)
			                : (d.justify == 1 ? -width : d.justify == 2 ? -width * 0.5f : 0.0f);
			for (char const* c = b; c < e; ) {
				unsigned cp = 0;
				c += ImTextCharFromUtf8(&cp, c, e);
				glyph_t const* gl = find_glyph(cp, d.font);
				if (!gl) continue;
				collect_geom(gl->shapes, gl->shapesn, mul(W, mat_make(scale, 0, 0, scale, x, y)), lt, g);
				x += gl->w * scale + track;
			}
		}
		float const a = collect ? 1.0f : ImSaturate(alpha);
		float fcol[3] = { d.fc[0], d.fc[1], d.fc[2] }, scol[3] = { d.sc[0], d.sc[1], d.sc[2] };
		fx_color(fcol);
		fx_color(scol);
		ImU32 const fc = ImGui::ColorConvertFloat4ToU32(ImVec4(fcol[0], fcol[1], fcol[2], a));
		ImU32 const sc = ImGui::ColorConvertFloat4ToU32(ImVec4(scol[0], scol[1], scol[2], a));
		static ImVector<ImVec2> poly, segs;
		float const hw = d.sw * mat_scale(W) * 0.5f;
		for (int pass = 0; pass < 2; pass++) {
			bool const stroke_pass = (pass == 0) != (d.stroke_over != 0);  // "of": stroke over fill
			if (stroke_pass && d.stroke) {
				segs.resize(0);
				for (int c = 0; c < g.cs.Size; c++) { flatten_contour(g, g.cs[c], poly, tol); stroke_polyline(segs, poly.Data, poly.Size, g.cs[c].closed, hw, 1, 1, 4.0f, tol); }
				fill(segs, sc, 0, nullptr);
			} else if (!stroke_pass && d.fill) {
				fill_geom(g, fc);
			}
		}
	}

	void draw_style(shape_t const& sh, geom const& g, mat const& M, float alpha, float t) {
		if (g.cs.Size == 0) return;
		bool const stroke = sh.kind == SK_STROKE || sh.kind == SK_GSTROKE;
		bool const grad = sh.kind == SK_GFILL || sh.kind == SK_GSTROKE;
		float const opacity = pv(sh.p[grad ? 0 : 1], t, 0, 100.0f) * 0.01f * alpha;
		if (opacity <= 0.0f && !collect) return;
		ImU32 col = 0;
		paint_t pt;
		if (grad) gradient(sh, M, t, opacity, pt);
		else {
			eval(L, sh.p[0], t, scratch);
			float rgb[3] = { scratch.Size > 0 ? scratch[0] : 0.0f, scratch.Size > 1 ? scratch[1] : 0.0f, scratch.Size > 2 ? scratch[2] : 0.0f };
			float const a = scratch.Size > 3 ? scratch[3] : 1.0f;
			fx_color(rgb);
			col = ImGui::ColorConvertFloat4ToU32(ImVec4(rgb[0], rgb[1], rgb[2], ImSaturate(a) * ImSaturate(opacity)));
			if (collect) col |= IM_COL32_A_MASK;
		}
		static ImVector<ImVec2> poly, segs;  // draw_style is not re-entered
		poly.resize(0); segs.resize(0);
		bool const fast = !collect && !grad && nclips == 0 && soft <= 1.0f && !shadow;
		if (!stroke && fast && g.cs.Size == 1 && g.soup.Size == 0) {
			flatten_contour(g, g.cs[0], poly, tol);
			if (convex_simple(poly.Data, poly.Size)) {
				// One convex contour: every fill rule gives the same coverage
				float area = 0.0f;
				for (int i = 0; i < poly.Size; i++) { ImVec2 const a = poly[i], b = poly[(i + 1) % poly.Size]; area += a.x * b.y - b.x * a.y; }
				if (area < 0.0f) for (int i = 0, j = poly.Size - 1; i < j; i++, j--) { ImVec2 const tmp = poly[i]; poly[i] = poly[j]; poly[j] = tmp; }  // ImGui's AA wants clockwise
				dl->AddConvexPolyFilled(poly.Data, poly.Size, col);
				return;
			}
		}
		if (!stroke) {
			for (int c = 0; c < g.cs.Size; c++) {
				flatten_contour(g, g.cs[c], poly, tol);
				if (poly.Size < 3) continue;
				if (collect) { add_poly(segs, poly.Data, poly.Size); continue; }  // matte: union of positive outlines
				for (int i = 0; i < poly.Size; i++) { segs.push_back(poly[i]); segs.push_back(poly[(i + 1) % poly.Size]); }
			}
			for (int i = 0; i < g.soup.Size; i++) segs.push_back(g.soup[i]);
			fill(segs, col, g.rule >= 0 ? g.rule : sh.rule == 2 ? 1 : 0, grad ? &pt : nullptr);
			return;
		}
		float const scale = mat_scale(M);
		float const hw = pv(sh.p[grad ? 4 : 2], t, 0, 1.0f) * scale * 0.5f;
		if (hw <= 0.0f) return;
		float pattern[16];
		int np = 0;
		float dash_offset = 0.0f;
		for (int i = 0; i < sh.dashn; i++) {
			dash_t const& d = L->dashes[sh.dash + i];
			float const v = pv(d.v, t, 0, 0.0f) * scale;
			if (d.kind == 2) dash_offset = v;
			else if (np < 16) pattern[np++] = v;
		}
		if (np % 2 == 1 && np < 8) { for (int i = 0; i < np; i++) pattern[np + i] = pattern[i]; np *= 2; }  // odd patterns repeat twice
		if (fast && np == 0 && (col >> IM_COL32_A_SHIFT) == 0xFF && hw >= 0.5f) {
			// Opaque, undashed, no sharp turn: Dear ImGui's thick polyline (shared joints, no overlap)
			ImVector<ImVec2>& lines = segs;  // reused as a flat list of the flattened contours
			static ImVector<int> counts;
			counts.resize(0);
			bool ok = true;
			for (int c = 0; c < g.cs.Size && ok; c++) {
				flatten_contour(g, g.cs[c], poly, tol);
				ok = poly.Size >= 2 && smooth_polyline(poly.Data, poly.Size, g.cs[c].closed);
				for (int i = 0; i < poly.Size; i++) lines.push_back(poly[i]);
				counts.push_back(poly.Size);
			}
			if (ok) {
				int ofs = 0;
				for (int c = 0; c < g.cs.Size; c++) {
					ImVec2* pts = lines.Data + ofs;
					int const n = counts[c];
					bool const closed = g.cs[c].closed && n > 2;
					if (!closed && sh.cap == 3) {
						// Square caps: extend both ends by half the width
						for (int e = 0; e < 2; e++) {
							ImVec2& q = pts[e == 0 ? 0 : n - 1];
							ImVec2 const r = pts[e == 0 ? 1 : n - 2];
							float const dx = q.x - r.x, dy = q.y - r.y, l = ImSqrt(dx * dx + dy * dy);
							if (l > 0.0f) { q.x += dx / l * hw; q.y += dy / l * hw; }
						}
					}
#if IMGUI_VERSION_NUM < 19276  // AddPolyline(): 'thickness' and 'flags' were swapped in Dear ImGui 1.92.8
					dl->AddPolyline(pts, n, col, closed ? ImDrawFlags_Closed : ImDrawFlags_None, hw * 2.0f);
#else
					dl->AddPolyline(pts, n, col, hw * 2.0f, closed ? ImDrawFlags_Closed : ImDrawFlags_None);
#endif
					if (!closed && sh.cap == 2) { dl->AddCircleFilled(pts[0], hw, col); dl->AddCircleFilled(pts[n - 1], hw, col); }
					ofs += n;
				}
				return;
			}
			segs.resize(0);
		}
		for (int c = 0; c < g.cs.Size; c++) {
			flatten_contour(g, g.cs[c], poly, tol);
			if (np > 0) dash_polyline(segs, poly, g.cs[c].closed, pattern, np, dash_offset, hw, sh.cap, sh.join, sh.miter, tol);
			else stroke_polyline(segs, poly.Data, poly.Size, g.cs[c].closed, hw, sh.cap, sh.join, sh.miter, tol);
		}
		fill(segs, col, 0, grad ? &pt : nullptr);
	}

	void render_group(int ofs, int cnt, mat const& M, float alpha, float t) {
		if (depth > 64) return;
		depth++;
		for (int i = cnt - 1; i >= 0; --i) {
			shape_t const& sh = L->shapes[L->children[ofs + i]];
			if (sh.hidden) continue;
			switch (sh.kind) {
				case SK_FILL: case SK_STROKE: case SK_GFILL: case SK_GSTROKE: {
					static geom g;  // only alive within this style (styles do not draw groups)
					g.clear();
					collect_geom(ofs, i, M, t, g);
					draw_style(sh, g, M, alpha, t);
					break;
				}
				case SK_GROUP: {
					float op = 1.0f;
					mat const G = mul(M, xform(sh.tr, t, &op));
					render_group(sh.child, sh.childn, G, alpha * op, t);
					break;
				}
				case SK_REPEATER: {
					int const copies = ImClamp((int)pv(sh.p[0], t, 0, 1.0f), 0, 256);
					float const offset = pv(sh.p[1], t, 0, 0.0f);
					float a[2], p[2], s[2];
					pv2(sh.tr.a, t, 0.0f, a);
					pv2(sh.tr.p, t, 0.0f, p);
					pv2(sh.tr.s, t, 100.0f, s);
					float const r = pv(sh.tr.r, t, 0, 0.0f);
					float const so = pv(sh.tr.so, t, 0, 100.0f) * 0.01f, eo = pv(sh.tr.eo, t, 0, 100.0f) * 0.01f;
					for (int j = 0; j < copies; j++) {
						int const c = sh.mode == 2 ? copies - 1 - j : j;  // 1 = later copies above
						float const kk = c + offset;
						mat R = mat_make(powf(s[0] * 0.01f, kk), 0, 0, powf(s[1] * 0.01f, kk), 0, 0);
						R = mul(rot(r * kk), R);
						R = mul(mat_make(1, 0, 0, 1, a[0] + p[0] * kk, a[1] + p[1] * kk), mul(R, mat_make(1, 0, 0, 1, -a[0], -a[1])));
						float const f = copies > 1 ? (float)c / (copies - 1) : 0.0f;
						render_group(ofs, i, mul(M, R), alpha * (so + (eo - so) * f), t);
					}
					depth--;
					return;  // the items before the repeater are drawn by its copies
				}
				default: break;
			}
		}
		depth--;
	}

	void pv3(prop_t const& p, float t, float def, float out[3]) {
		out[0] = out[1] = out[2] = def;
		if (!present(p)) return;
		eval(L, p, t, scratch);
		for (int k = 0; k < 3 && k < scratch.Size; k++) out[k] = scratch[k];
		if (scratch.Size == 2) out[2] = def == 100.0f ? 100.0f : 0.0f;  // 2D value on a 3D layer
	}

	// 3D layer transform (lottie-web's order, in After Effects axes):
	// T(p) Rx(or) Ry(or) Rz(or) Rx(rx) Ry(ry) Rz(rz) skew S T(-a)
	m4 xform3(xform_t const& x, float t, float* opacity, bool camera = false) {
		float a[3], p[3], s[3], o[3];
		pv3(x.a, t, 0.0f, a);
		if (x.split) { p[0] = pv(x.px, t, 0, 0.0f); p[1] = pv(x.py, t, 0, 0.0f); p[2] = pv(x.pz, t, 0, 0.0f); }
		else pv3(x.p, t, 0.0f, p);
		pv3(x.s, t, 100.0f, s);
		pv3(x.orient, t, 0.0f, o);
		if (opacity) *opacity = pv(x.o, t, 0, 100.0f) * 0.01f;
		m4 m = m4_translate(p[0], p[1], p[2]);
		if (camera && present(x.a)) {
			// A camera's anchor is its point of interest: aim +z at it, keeping y down
			float f[3] = { a[0] - p[0], a[1] - p[1], a[2] - p[2] };
			float const fl = ImSqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
			if (fl > 1e-6f) {
				for (int k = 0; k < 3; k++) f[k] /= fl;
				float xa[3] = { f[2], 0.0f, -f[0] };  // cross((0,1,0), f)
				float const xl = ImSqrt(xa[0] * xa[0] + xa[2] * xa[2]);
				if (xl > 1e-6f) { xa[0] /= xl; xa[2] /= xl; } else { xa[0] = 1.0f; xa[2] = 0.0f; }
				float const ya[3] = { f[1] * xa[2] - f[2] * xa[1], f[2] * xa[0] - f[0] * xa[2], f[0] * xa[1] - f[1] * xa[0] };
				m4 r = m4_id();
				for (int k = 0; k < 3; k++) { r.m[k * 4] = xa[k]; r.m[k * 4 + 1] = ya[k]; r.m[k * 4 + 2] = f[k]; }
				m = m4_mul(m, r);
			}
		}
		m = m4_mul(m, m4_rot(0, o[0])); m = m4_mul(m, m4_rot(1, o[1])); m = m4_mul(m, m4_rot(2, o[2]));
		m = m4_mul(m, m4_rot(0, pv(x.rx, t, 0, 0.0f))); m = m4_mul(m, m4_rot(1, pv(x.ry, t, 0, 0.0f))); m = m4_mul(m, m4_rot(2, pv(x.r, t, 0, 0.0f)));
		if (camera) return m;
		float const sk = pv(x.sk, t, 0, 0.0f);
		if (sk != 0.0f) {
			float const sa = pv(x.sa, t, 0, 0.0f);
			m = m4_mul(m, m4_from_mat(mul(mul(rot(-sa), mat_make(1, 0, tanf(-sk * (IM_PI / 180.0f)), 1, 0, 0)), rot(sa))));
		}
		m4 sc = m4_id();
		sc.m[0] = s[0] * 0.01f; sc.m[5] = s[1] * 0.01f; sc.m[10] = s[2] * 0.01f;
		m = m4_mul(m, sc);
		return m4_mul(m, m4_translate(-a[0], -a[1], -a[2]));
	}

	m4 layer_world3(int li, float t, float* opacity) {
		layer_t const& ly = L->layers[li];
		float const lt = t;  // properties use the comp time (lottie-web: the start time offsets both frame and keys)
		m4 W = ly.ddd || ly.ty == 13 ? xform3(ly.ks, lt, opacity, ly.ty == 13) : m4_from_mat(xform(ly.ks, lt, opacity));
		int p = ly.parent;
		for (int guard = 0; p >= 0 && guard < 32; guard++) {
			layer_t const& pl = L->layers[p];
			float const plt = t;
			W = m4_mul(pl.ddd ? xform3(pl.ks, plt, nullptr) : m4_from_mat(xform(pl.ks, plt, nullptr)), W);
			p = pl.parent;
		}
		return W;
	}

	// 3D layer in a comp of size cw x ch: its plane through the comp camera (or After
	// Effects' default 50 mm camera) as a projective 2D transform
	// World to camera space of the current comp (camera looking down +z) and its zoom
	m4 comp_view(float t, float* zoom) {
		comp_t const& cp = L->comps[cur_comp];
		*zoom = cur_w / 0.72f;  // 50 mm preset: horizontal view angle ~39.6 degrees
		int cam = -1;  // the topmost camera active at t; none: the default camera
		for (int i = cp.camera; i >= 0 && i < cp.layer + cp.layern && cam < 0; i++)
			if (L->layers[i].ty == 13 && t >= L->layers[i].ip && t < L->layers[i].op) cam = i;
		if (cam < 0) return m4_translate(-cur_w * 0.5f, -cur_h * 0.5f, *zoom);
		*zoom = pv(L->layers[cam].pe, t, 0, *zoom);
		return m4_inverse_affine(layer_world3(cam, t, nullptr));
	}

	mat layer_projected(int li, float t, float* opacity) {
		float const cx = cur_w * 0.5f, cy = cur_h * 0.5f;
		float zoom;
		m4 const view = comp_view(t, &zoom);
		m4 const K = m4_mul(view, layer_world3(li, t, opacity));
		mat H;
		H.a = zoom * K.m[0] + cx * K.m[8]; H.c = zoom * K.m[1] + cx * K.m[9]; H.x = zoom * K.m[3] + cx * K.m[11];
		H.b = zoom * K.m[4] + cy * K.m[8]; H.d = zoom * K.m[5] + cy * K.m[9]; H.y = zoom * K.m[7] + cy * K.m[11];
		H.g = K.m[8]; H.h = K.m[9]; H.i = K.m[11];
		return H;
	}

	mat layer_world(int li, float t, float* opacity) {
		layer_t const& ly = L->layers[li];
		if (ly.ddd && L->comps[cur_comp].has3d) return layer_projected(li, t, opacity);
		mat W = xform(ly.ks, t, opacity, ly.ao != 0);
		int p = ly.parent;
		for (int guard = 0; p >= 0 && guard < 32; guard++) {
			layer_t const& pl = L->layers[p];
			W = mul(xform(pl.ks, t, nullptr, pl.ao != 0), W);  // parents pass their transform, not their opacity
			p = pl.parent;
		}
		return W;
	}

	int push_masks(layer_t const& ly, mat const& W, float lt) {
		int pushed = 0;
		static ImVector<ImVec2> uni, segs, poly, comp;  // push_masks is not re-entered
		static geom g;
		uni.resize(0);
		bool uni_eo = false;  // a difference mask in the group: even-odd gives the XOR with the masks before it
		auto flush = [&]() { if (uni.Size) { push_clip(uni, uni_eo ? 1 : 0); pushed++; uni.resize(0); } uni_eo = false; };
		for (int i = 0; i < ly.maskn; i++) {
			mask_t const& mk = L->masks[ly.mask + i];
			if (mk.mode == 'n') continue;
			g.clear();
			eval(L, mk.pt, lt, scratch);
			if (scratch.Size >= 2) {
				int const n = (int)scratch[1];
				if (n > 0 && scratch.Size >= 2 + 6 * n) add_vertices(g, W, scratch.Data + 2, n, true, false);
			}
			segs.resize(0);
			for (int c = 0; c < g.cs.Size; c++) { flatten_contour(g, g.cs[c], poly, tol); if (poly.Size >= 3) add_poly(segs, poly.Data, poly.Size); }
			bool const subtract = (mk.mode == 's') != (mk.inv != 0);
			if (mk.mode != 's' && mk.mode != 'i' && !mk.inv) {  // add (and lighten / darken): union; difference: XOR
				if (mk.mode == 'f') uni_eo = true;
				for (int k = 0; k < segs.Size; k++) uni.push_back(segs[k]);
				continue;
			}
			flush();
			if (subtract) { complement(comp, segs); push_clip(comp, 2); }
			else push_clip(segs, 0);
			pushed++;
		}
		flush();
		return pushed;
	}

	void render_layer(int li, float t, mat const& M, float alpha) {
		layer_t const& ly = L->layers[li];
		if (depth > 32) return;
		depth++;
		float op = 1.0f;
		mat const W = mul(M, layer_world(li, t, &op));
		float const lt = t;  // properties use the comp time (lottie-web: the start time offsets both frame and keys)
		float a = alpha * op;
		// Clips are geometric; opacity is approximated: a single add mask's opacity and an
		// alpha matte layer's own opacity scale the layer
		if (ly.maskn == 1 && L->masks[ly.mask].mode == 'a' && !L->masks[ly.mask].inv) a *= ImSaturate(pv(L->masks[ly.mask].o, lt, 0, 100.0f) * 0.01f);
		if (ly.matte >= 0 && ly.matte != li && li != skip_matte && (ly.tt == 1 || ly.tt == 3)) {
			layer_t const& ml = L->layers[ly.matte];
			a *= ImSaturate(pv(ml.ks.o, t, 0, 100.0f) * 0.01f);
		}
		int pushed = 0;
#if IAM_LOTTIE_OFFSCREEN
		// Track mattes and blend modes: draw the layer (and the matte) into textures, composite
		// (exact matte opacity, gradients and luma, unlike the CPU's geometric clip)
		bool const luma = ly.matte >= 0 && ly.matte != li && ly.tt >= 1 && ly.tt <= 4;
		int blur_fx = -1, shadow_fx2 = -1;
		for (int i = 0; i < ly.fxn; i++) {
			if (L->effects[ly.fx + i].kind == FX_BLUR) blur_fx = ly.fx + i;
			if (L->effects[ly.fx + i].kind == FX_SHADOW) shadow_fx2 = ly.fx + i;
		}
		if (!collect && !offscreen && a > 0.0f && (luma || (ly.bm > 0 && ly.bm <= 17) || blur_fx >= 0 || shadow_fx2 >= 0) && comp_program() && offscreen_targets()) {
			offscreen = true;
			if (luma) {
				layer_t const& ml = L->layers[ly.matte];
				offscreen_begin(dl, 1);
				if (t >= ml.ip && t < ml.op) render_layer(ly.matte, t, M, 1.0f);
				offscreen_end(dl);
			}
			int const prev_skip = skip_matte, prev_fx = gpu_fx_layer;
			if (luma) skip_matte = li;  // the texture is the matte
			gpu_fx_layer = li;          // blur and shadow come from the passes below
			offscreen_begin(dl, 0);
			render_layer(li, t, M, alpha);
			offscreen_end(dl);
			skip_matte = prev_skip;
			gpu_fx_layer = prev_fx;
			offscreen = false;
			float const scale = mat_scale(W), fbs = ImGui::GetIO().DisplayFramebufferScale.x;
			if (shadow_fx2 >= 0) {
				// Drop shadow: the layer's alpha blurred (two passes), tinted and offset, under the layer
				effect_t const& fx = L->effects[shadow_fx2];
				float rgba[4];
				eval(L, fx.v[0], lt, scratch);
				for (int k = 0; k < 3; k++) rgba[k] = k < scratch.Size ? scratch[k] : 0.0f;
				rgba[3] = ImSaturate(pv(fx.v[1], lt, 0, 255.0f) / 255.0f);
				float const ang = (pv(fx.v[2], lt, 0, 135.0f) - 90.0f) * (IM_PI / 180.0f), dist = pv(fx.v[3], lt, 0, 5.0f) * scale * fbs;
				float const sigma = pv(fx.v[4], lt, 0, 0.0f) * 0.25f * scale * fbs;  // lottie-web: sigma = softness / 4
				offscreen_pass(dl, 0, 2, 1, 0, s_rt[1], sigma, ImVec2(1, 0));
				offscreen_pass(dl, 2, -1, 2, 0, s_rt[1], sigma, ImVec2(0, 1), ImVec2(ImCos(ang) * dist, ImSin(ang) * dist), rgba);
				if (pv(fx.v[5], lt, 0, 0.0f) != 0.0f) { depth--; return; }  // shadow only
			}
			static int const matte_mode[5] = { 0, -3, -4, -1, -2 };  // tt 1 alpha, 2 inverted alpha, 3 luma, 4 inverted luma
			int const mode = luma ? matte_mode[ly.tt] : (ly.bm > 0 && ly.bm <= 17 ? ly.bm : 0);
			ImTextureID const extra = luma ? s_rt[1] : s_bg;
			if (blur_fx >= 0) {
				// Gaussian blur: horizontal pass into the scratch texture, vertical in the composite
				effect_t const& fx = L->effects[blur_fx];
				float const sigma = pv(fx.v[0], lt, 0, 0.0f) * 0.3f * scale * fbs;  // lottie-web: sigma = 0.3 blurriness
				int const dims = (int)pv(fx.v[1], lt, 0, 1.0f);                     // 1 both, 2 horizontal, 3 vertical
				offscreen_pass(dl, 0, 2, 1, 0, s_rt[1], dims == 3 ? 0.0f : sigma, ImVec2(1, 0));
				offscreen_pass(dl, 2, -1, 0, mode, extra, dims == 2 ? 0.0f : sigma, ImVec2(0, 1));
			} else {
				offscreen_pass(dl, 0, -1, 0, mode, extra, 0.0f, ImVec2(0, 0));
			}
			depth--;
			return;
		}
#endif
		if (a > 0.0f || collect) {
			if (ly.matte >= 0 && ly.matte != li && li != skip_matte) {
				// Pool by depth: the matte layer renders deeper than the layer it clips
				static ImVector<ImVec2> s_matte[34], s_comp[34];
				ImVector<ImVec2>& matte = s_matte[depth];
				matte.resize(0);
				ImVector<ImVec2>* const prev = collect;
				collect = &matte;
				layer_t const& ml = L->layers[ly.matte];
				if (t >= ml.ip && t < ml.op) render_layer(ly.matte, t, M, 1.0f);
				collect = prev;
				if (ly.tt == 2 || ly.tt == 4) { complement(s_comp[depth], matte); push_clip(s_comp[depth], 2); }
				else push_clip(matte, 0);
				pushed++;
			}
			pushed += push_masks(ly, W, lt);
			bool const fx_pushed = ly.fxn > 0 && nfx < 16;
			if (fx_pushed) { active_fx f = { ly.fx, ly.fxn, lt }; fx_stack[nfx++] = f; }
			// Blur and drop shadow (CPU approximation): a Gaussian of sigma becomes a linear edge
			// ~2.5 sigma wide; the shadow is the content drawn first, offset and recoloured
			float const prev_soft = soft;
			float const scale = mat_scale(W);
			int shadow_fx = -1;
			for (int i = 0; i < ly.fxn && !collect && li != gpu_fx_layer; i++) {
				effect_t const& fx = L->effects[ly.fx + i];
				if (fx.kind == FX_BLUR) soft = ImMax(soft, pv(fx.v[0], lt, 0, 0.0f) * 0.3f * 2.5f * scale);  // lottie-web: sigma = 0.3 blurriness
				if (fx.kind == FX_SHADOW) shadow_fx = ly.fx + i;
			}
			bool draw_content = true;
			for (int pass = shadow_fx >= 0 ? 0 : 1; pass < 2; pass++) {
				if (pass == 0) {
					effect_t const& fx = L->effects[shadow_fx];
					eval(L, fx.v[0], lt, scratch);
					for (int k = 0; k < 3; k++) shadow_rgba[k] = k < scratch.Size ? scratch[k] : 0.0f;
					shadow_rgba[3] = ImSaturate(pv(fx.v[1], lt, 0, 255.0f) / 255.0f * ImSaturate(a));
					float const ang = (pv(fx.v[2], lt, 0, 135.0f) - 90.0f) * (IM_PI / 180.0f), dist = pv(fx.v[3], lt, 0, 5.0f) * scale;
					shadow_ofs = ImVec2(ImCos(ang) * dist, ImSin(ang) * dist);
					soft = ImMax(prev_soft, pv(fx.v[4], lt, 0, 0.0f) * 0.25f * 2.5f * scale);  // lottie-web: sigma = softness / 4
					shadow = true;
					draw_content = pv(fx.v[5], lt, 0, 0.0f) == 0.0f;
				} else {
					shadow = false;
					if (!draw_content) break;
					soft = prev_soft;
					for (int i = 0; i < ly.fxn && !collect && li != gpu_fx_layer; i++)
						if (L->effects[ly.fx + i].kind == FX_BLUR) soft = ImMax(soft, pv(L->effects[ly.fx + i].v[0], lt, 0, 0.0f) * 0.3f * 2.5f * scale);
				}
			switch (ly.ty) {
				case 4: render_group(ly.shapes, ly.shapesn, W, a, lt); break;
				case 1: {
					ImVec2 const r[4] = { apply(W, 0, 0), apply(W, ly.sw, 0), apply(W, ly.sw, ly.sh), apply(W, 0, ly.sh) };
					ImVector<ImVec2> segs;
					add_poly(segs, r, 4);
					float rgb[3] = { ly.solid[0], ly.solid[1], ly.solid[2] };
					fx_color(rgb);
					fill(segs, ImGui::ColorConvertFloat4ToU32(ImVec4(rgb[0], rgb[1], rgb[2], collect ? 1.0f : ImSaturate(a))), 0, nullptr);
					break;
				}
				case 5: draw_text(ly, W, a, lt); break;
				case 0:
					if (ly.comp >= 0) render_comp(ly.comp, present(ly.tm) ? pv(ly.tm, t, 0, 0.0f) * L->fr : (t - ly.st) / ly.sr, W, a, ly.pw > 0.0f ? ly.pw : cur_w, ly.ph > 0.0f ? ly.ph : cur_h);
					break;
				default: break;
			}
			}
			shadow = false;
			soft = prev_soft;
			if (fx_pushed) nfx--;
		}
		while (pushed-- > 0) pop_clip();
		depth--;
	}

	void render_comp(int ci, float t, mat const& M, float alpha, float cw, float ch) {
		comp_t const& cp = L->comps[ci];
		int const prev_comp = cur_comp;
		float const prev_w = cur_w, prev_h = cur_h;
		cur_comp = ci; cur_w = cw; cur_h = ch;
		// Consecutive 3D layers draw far to near (After Effects' depth sort by anchor point
		// distance along the camera axis; ties keep the layer order, 2D layers break the runs)
		static ImVector<ImVec2> s_run[34];  // (camera z, layer), by depth: precomps nest deeper
		ImVector<ImVec2>& run = s_run[ImMin(depth, 33)];
		run.resize(0);
		float zoom = 0.0f;
		m4 const view = cp.has3d ? comp_view(t, &zoom) : m4_id();
		auto flush = [&]() {
			for (int i = 1; i < run.Size; ++i) {  // stable insertion sort, farthest first
				ImVec2 const v = run[i];
				int j = i - 1;
				while (j >= 0 && run[j].x < v.x) { run[j + 1] = run[j]; --j; }
				run[j + 1] = v;
			}
			for (int i = 0; i < run.Size; ++i) render_layer((int)run[i].y, t, M, alpha);
			run.resize(0);
		};
		for (int i = cp.layern - 1; i >= 0; --i) {
			layer_t const& ly = L->layers[cp.layer + i];
			if (ly.td || ly.hidden || ly.ty == 3 || ly.ty == 13) continue;  // matte sources are drawn through the layer they matte
			if (t < ly.ip || t >= ly.op) continue;
			if (cp.has3d && ly.ddd) {
				float an[3];
				pv3(ly.ks.a, t, 0.0f, an);
				m4 const K = m4_mul(view, layer_world3(cp.layer + i, t, nullptr));
				run.push_back(ImVec2(K.m[8] * an[0] + K.m[9] * an[1] + K.m[10] * an[2] + K.m[11], (float)(cp.layer + i)));
				continue;
			}
			flush();
			render_layer(cp.layer + i, t, M, alpha);
		}
		flush();
		cur_comp = prev_comp; cur_w = prev_w; cur_h = prev_h;
	}
};

} // namespace iam_lottie_detail

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

iam_lottie* iam_lottie_load_from_memory(char const* json, size_t size) {
	if (!json || size == 0 || size > 0x7FFFFFFF) return nullptr;
	jdoc doc;
	doc.text = json;
	jparser ps = { json, json, json + size, &doc };
	int const root = ps.value(0);
	if (root < 0 || doc.type(root) != J_OBJ) return nullptr;

	iam_lottie* L = IM_NEW(iam_lottie)();
	L->warn = 0;
	L->cache_enabled = true;
	L->frame_snap = false;
	memset(L->seen, 0xFF, sizeof(L->seen));  // no key matches NaN frames
	L->w = (float)doc.num(root, "w", 0.0);
	L->h = (float)doc.num(root, "h", 0.0);
	L->fr = (float)doc.num(root, "fr", 0.0);
	L->ip = (float)doc.num(root, "ip", 0.0);
	L->op = (float)doc.num(root, "op", 0.0);
	if (!(L->w > 0.0f) || !(L->h > 0.0f) || !(L->fr > 0.0f) || !(L->op > L->ip)) { IM_DELETE(L); return nullptr; }

	builder B;
	B.L = L;
	B.D = &doc;
	{
		// "5.7.0" -> 5 7 0; no version reads as current
		B.version[0] = 100; B.version[1] = B.version[2] = 0;
		int const v = doc.get(root, "v");
		if (doc.type(v) == J_STR) {
			char const* c = json + doc.n[v].str;
			char const* const e = c + doc.n[v].str_len;
			for (int k = 0; k < 3; k++) {
				int n = 0;
				while (c < e && *c >= '0' && *c <= '9') n = n * 10 + (*c++ - '0');
				B.version[k] = n;
				if (c < e && *c == '.') c++;
			}
		}
	}
	B.fonts_and_chars(root);  // before the layers: text documents refer to the fonts
	B.layers(doc.get(root, "layers"));
	int const assets = doc.get(root, "assets");
	if (doc.type(assets) == J_ARR)
		for (int a = doc.n[assets].child; a >= 0; a = doc.n[a].next) {
			int const lys = doc.get(a, "layers"), id = doc.get(a, "id");
			if (doc.type(lys) != J_ARR || doc.type(id) != J_STR) { if (doc.get(a, "p") >= 0) L->warn |= W_IMAGE; continue; }
			B.comp_id.push_back(doc.n[id].str);
			B.comp_id_len.push_back(doc.n[id].str_len);
			B.layers(lys);
		}
	for (int i = 0; i < B.ref_layer.Size; i++)
		for (int c = 0; c < B.comp_id.Size; c++)
			if (B.comp_id_len[c] == B.ref_len[i] && memcmp(json + B.comp_id[c], json + B.ref_str[i], (size_t)B.ref_len[i]) == 0)
				L->layers[B.ref_layer[i]].comp = c + 1;
	int const markers = doc.get(root, "markers");
	if (doc.type(markers) == J_ARR)
		for (int m = doc.n[markers].child; m >= 0; m = doc.n[m].next) {
			int const cm = doc.get(m, "cm");
			marker_t mk;
			mk.name = B.add_string(doc.type(cm) == J_STR ? json + doc.n[cm].str : "", doc.type(cm) == J_STR ? doc.n[cm].str_len : 0);
			mk.t = (float)doc.num(m, "tm", 0.0);
			mk.dur = (float)doc.num(m, "dr", 0.0);
			L->markers.push_back(mk);
		}
	for (int i = 0; i < L->layers.Size; i++)
		if (L->layers[i].ty == 5 && L->glyphs.Size == 0) L->warn |= W_TEXT;  // system fonts only: nothing to draw
	build_warnings(L);
	return L;
}

iam_lottie* iam_lottie_load(char const* path) {
	FILE* f = path ? fopen(path, "rb") : nullptr;
	if (!f) return nullptr;
	ImVector<char> buf;
	char chunk[16384];
	size_t n;
	while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0) {
		int const at = buf.Size;
		buf.resize(at + (int)n);
		memcpy(buf.Data + at, chunk, n);
	}
	fclose(f);
	return iam_lottie_load_from_memory(buf.Data, (size_t)buf.Size);
}

void iam_lottie_free(iam_lottie* anim) { if (anim) IM_DELETE(anim); }

float iam_lottie_duration(iam_lottie const* anim) { return anim ? (anim->op - anim->ip) / anim->fr : 0.0f; }
float iam_lottie_frame_rate(iam_lottie const* anim) { return anim ? anim->fr : 0.0f; }
iam_vec2 iam_lottie_size(iam_lottie const* anim) { return anim ? iam_v2(anim->w, anim->h) : iam_v2(0, 0); }
char const* iam_lottie_warnings(iam_lottie const* anim) { return anim ? anim->warnings.Data : ""; }

bool iam_lottie_marker(iam_lottie const* anim, char const* name, float* out_start, float* out_duration) {
	if (!anim || !name) return false;
	for (int i = 0; i < anim->markers.Size; i++)
		if (strcmp(anim->strings.Data + anim->markers[i].name, name) == 0) {
			if (out_start) *out_start = (anim->markers[i].t - anim->ip) / anim->fr;
			if (out_duration) *out_duration = anim->markers[i].dur / anim->fr;
			return true;
		}
	return false;
}

void iam_lottie_draw(ImDrawList* dl, iam_lottie const* anim, float time, iam_vec2 pos_in, iam_vec2 size_in) {
	if (!dl || !anim) return;
	ImVec2 const pos = IAM_VEC2_TO_IM(pos_in);
	ImVec2 size = IAM_VEC2_TO_IM(size_in);
	if (size.x <= 0.0f || size.y <= 0.0f) size = ImVec2(anim->w, anim->h);
	float frame = ImClamp(anim->ip + time * anim->fr, anim->ip, anim->op - 1e-3f);
	if (anim->frame_snap) frame = ImMax(ImFloor(frame + 1e-3f), anim->ip);  // epsilon: t = n / fr lands on frame n despite rounding
	float const k = ImMin(size.x / anim->w, size.y / anim->h);

	// Replay cache
	iam_lottie::draw_key key;
	memset(&key, 0, sizeof(key));  // compared bytewise
	key.frame = frame;
	key.pos[0] = pos.x; key.pos[1] = pos.y; key.size[0] = size.x; key.size[1] = size.y;
	key.uv[0] = dl->_Data->TexUvWhitePixel.x; key.uv[1] = dl->_Data->TexUvWhitePixel.y;
	key.clip[0] = dl->_CmdHeader.ClipRect.x; key.clip[1] = dl->_CmdHeader.ClipRect.y; key.clip[2] = dl->_CmdHeader.ClipRect.z; key.clip[3] = dl->_CmdHeader.ClipRect.w;
	key.gpu = (s_gpu_enabled ? 1 : 0) | (ImGui::GetCurrentContext() ? (int)(ImGui::GetIO().DisplayFramebufferScale.x * 100.0f) << 1 : 0);
	if (anim->cache_enabled) {
		for (int i = 0; i < iam_lottie::CACHE; i++) {
			iam_lottie::draw_entry& e = anim->cache[i];
			if (!e.valid || memcmp(&e.key, &key, sizeof(key)) != 0) continue;
			e.stamp = ++anim->stamp;
			dl->PrimReserve(e.idx.Size, e.vtx.Size);
			memcpy(dl->_VtxWritePtr, e.vtx.Data, sizeof(ImDrawVert) * (size_t)e.vtx.Size);
			ImDrawIdx const base = (ImDrawIdx)dl->_VtxCurrentIdx;
			for (int j = 0; j < e.idx.Size; j++) dl->_IdxWritePtr[j] = (ImDrawIdx)(base + e.idx[j]);
			dl->_VtxWritePtr += e.vtx.Size;
			dl->_IdxWritePtr += e.idx.Size;
			dl->_VtxCurrentIdx += (unsigned)e.vtx.Size;
			return;
		}
	}
	bool repeat = false;
	for (int i = 0; i < iam_lottie::CACHE && anim->cache_enabled; i++) if (memcmp(&anim->seen[i], &key, sizeof(key)) == 0) repeat = true;
	int const cmd0 = dl->CmdBuffer.Size, vtx0 = dl->VtxBuffer.Size, idx0 = dl->IdxBuffer.Size;
	unsigned const cur0 = dl->_VtxCurrentIdx, vofs0 = dl->_CmdHeader.VtxOffset;

	static ctx C;  // its clip and scratch buffers are reused across draws
	C.dl = dl;
	C.L = anim;
	C.nclips = C.overflow = C.depth = C.nfx = 0;
	C.soft = 0.0f;
	C.shadow = false;
	C.offscreen = false;
	C.skip_matte = -1;
	C.gpu_fx_layer = -1;
	C.collect = nullptr;
	C.tol = 0.25f;
	mat const M = mat_make(k, 0, 0, k, pos.x + (size.x - anim->w * k) * 0.5f, pos.y + (size.y - anim->h * k) * 0.5f);
	C.cur_comp = 0; C.cur_w = anim->w; C.cur_h = anim->h;
	C.render_comp(0, frame, M, 1.0f, anim->w, anim->h);

	if (!anim->cache_enabled) return;
	if (!repeat && !anim->frame_snap) { anim->seen[anim->seen_next] = key; anim->seen_next = (anim->seen_next + 1) % iam_lottie::CACHE; return; }
	// Second time with this key (first with frame snap: frames repeat): keep the output if it is plain geometry (no callback, texture
	// or clip change; new commands only from ImDrawList's vertex offset wrap) that fits one
	// replay (16-bit indices: < 65536 vertices)
	int const nv = dl->VtxBuffer.Size - vtx0, ni = dl->IdxBuffer.Size - idx0;
	if (nv <= 0 || (sizeof(ImDrawIdx) == 2 && nv >= 65536)) return;
	ImDrawCmd const& start_cmd = dl->CmdBuffer[cmd0 - 1];
	for (int c = cmd0; c < dl->CmdBuffer.Size; c++) {
		ImDrawCmd const& cmd = dl->CmdBuffer[c];
		if (cmd.UserCallback || cmd.TexRef.GetTexID() != start_cmd.TexRef.GetTexID() || memcmp(&cmd.ClipRect, &start_cmd.ClipRect, sizeof(ImVec4)) != 0) return;
	}
	(void)vofs0; (void)cur0;
	int slot = 0;
	for (int i = 1; i < iam_lottie::CACHE; i++) if (!anim->cache[i].valid || (anim->cache[slot].valid && anim->cache[i].stamp < anim->cache[slot].stamp)) slot = i;
	iam_lottie::draw_entry& e = anim->cache[slot];
	e.key = key;
	e.valid = true;
	e.stamp = ++anim->stamp;
	e.vtx.resize(nv);
	memcpy(e.vtx.Data, dl->VtxBuffer.Data + vtx0, sizeof(ImDrawVert) * (size_t)nv);
	e.idx.resize(ni);
	// Index values are relative to their command's vertex offset: make them relative to our first vertex
	for (int c = cmd0 - 1; c < dl->CmdBuffer.Size; c++) {
		ImDrawCmd const& cmd = dl->CmdBuffer[c];
		int const b = ImMax((int)cmd.IdxOffset, idx0), e2 = (c + 1 < dl->CmdBuffer.Size) ? (int)dl->CmdBuffer[c + 1].IdxOffset : dl->IdxBuffer.Size;
		int const end = c == dl->CmdBuffer.Size - 1 ? dl->IdxBuffer.Size : e2;
		for (int j = b; j < end; j++) e.idx[j - idx0] = (ImDrawIdx)((int)cmd.VtxOffset + (int)dl->IdxBuffer[j] - vtx0);
	}
}

void iam_lottie_set_frame_snap(iam_lottie* anim, bool enabled) { if (anim) anim->frame_snap = enabled; }

void iam_lottie_set_cache(iam_lottie* anim, bool enabled) {
	if (!anim) return;
	anim->cache_enabled = enabled;
	for (int i = 0; i < iam_lottie::CACHE; i++) { anim->cache[i].valid = false; anim->cache[i].vtx.clear(); anim->cache[i].idx.clear(); }
	memset(anim->seen, 0xFF, sizeof(anim->seen));
}

bool iam_lottie_gpu_active() {
#if IAM_LOTTIE_GPU
	return gpu_program() != nullptr;
#else
	return false;
#endif
}

void iam_lottie_set_gpu(bool enabled) { s_gpu_enabled = enabled; }
