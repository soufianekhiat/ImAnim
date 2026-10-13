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
struct xform_t { prop_t a, p, px, py, s, r, o, sk, sa, so, eo; int split; };

enum {
	SK_NONE, SK_GROUP, SK_PATH, SK_RECT, SK_ELLIPSE, SK_STAR,
	SK_FILL, SK_STROKE, SK_GFILL, SK_GSTROKE, SK_TRIM, SK_ROUND, SK_REPEATER
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
	int ty, ind, parent, hidden, td, tt, tp, matte, comp;
	float ip, op, st, sr;
	xform_t ks;
	int shapes, shapesn;       // children[]
	prop_t tm;
	float solid[3], sw, sh;
	int mask, maskn;
};
struct comp_t { int layer, layern; };
struct marker_t { int name; float t, dur; };

enum {
	W_EXPR = 1 << 0, W_EFFECTS = 1 << 1, W_TEXT = 1 << 2, W_IMAGE = 1 << 3, W_BLEND = 1 << 4, W_3D = 1 << 5,
	W_MERGE = 1 << 6, W_MODIFIER = 1 << 7, W_LUMA = 1 << 8, W_HIGHLIGHT = 1 << 9, W_AUTO_ORIENT = 1 << 10,
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
	ImVector<char> strings;
	ImVector<char> warnings;
	unsigned warn;
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
		if (D->get(node, "rx") >= 0 || D->get(node, "ry") >= 0) L->warn |= W_3D;
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
					s.kind = SK_PATH; s.p[0] = prop(D->get(c, "ks"));
				} else if (D->str_eq(ty, "rc")) {
					s.kind = SK_RECT; s.p[0] = prop(D->get(c, "p")); s.p[1] = prop(D->get(c, "s")); s.p[2] = prop(D->get(c, "r"));
				} else if (D->str_eq(ty, "el")) {
					s.kind = SK_ELLIPSE; s.p[0] = prop(D->get(c, "p")); s.p[1] = prop(D->get(c, "s"));
				} else if (D->str_eq(ty, "sr")) {
					s.kind = SK_STAR; s.mode = (int)D->num(c, "sy", 1.0);
					char const* const keys[7] = { "p", "or", "ir", "os", "is", "r", "pt" };
					for (int k = 0; k < 7; k++) s.p[k] = prop(D->get(c, keys[k]));
				} else if (D->str_eq(ty, "fl")) {
					s.kind = SK_FILL; s.p[0] = prop(D->get(c, "c")); s.p[1] = prop(D->get(c, "o")); s.rule = (int)D->num(c, "r", 1.0);
				} else if (D->str_eq(ty, "st")) {
					s.kind = SK_STROKE; s.p[0] = prop(D->get(c, "c")); s.p[1] = prop(D->get(c, "o")); s.p[2] = prop(D->get(c, "w"));
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
					if (D->num(D->get(c, "h"), "k", 0.0) != 0.0) L->warn |= W_HIGHLIGHT;
					if (stroke) { s.p[4] = prop(D->get(c, "w")); stroke_style(c, s); }
				} else if (D->str_eq(ty, "tm")) {
					s.kind = SK_TRIM; s.p[0] = prop(D->get(c, "s")); s.p[1] = prop(D->get(c, "e")); s.p[2] = prop(D->get(c, "o"));
					s.mode = (int)D->num(c, "m", 1.0);
				} else if (D->str_eq(ty, "rd")) {
					s.kind = SK_ROUND; s.p[0] = prop(D->get(c, "r"));
				} else if (D->str_eq(ty, "rp")) {
					s.kind = SK_REPEATER; s.p[0] = prop(D->get(c, "c")); s.p[1] = prop(D->get(c, "o"));
					s.mode = (int)D->num(c, "m", 1.0);
					s.tr = xform(D->get(c, "tr"));
				} else {
					L->warn |= D->str_eq(ty, "mm") ? W_MERGE : W_MODIFIER;
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
		comp_t cp = { L->layers.Size, 0 };
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
				if (D->num(c, "ddd", 0.0) != 0.0) L->warn |= W_3D;
				if (D->num(c, "bm", 0.0) != 0.0) L->warn |= W_BLEND;
				if (D->num(c, "ao", 0.0) != 0.0) L->warn |= W_AUTO_ORIENT;
				if (D->count(D->get(c, "ef")) > 0) L->warn |= W_EFFECTS;
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
					case 4: items(D->get(c, "shapes"), ly.shapes, ly.shapesn, nullptr); break;
					case 5: L->warn |= W_TEXT; break;
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
						mk.pt = prop(D->get(m, "pt"));
						mk.o = prop(D->get(m, "o"));
						if (mk.mode != 'a' && mk.mode != 's' && mk.mode != 'i' && mk.mode != 'n') L->warn |= W_MASK_MODE;
						L->masks.push_back(mk);
					}
				ly.maskn = L->masks.Size - ly.mask;
				L->layers.push_back(ly);
			}
		cp.layern = L->layers.Size - cp.layer;
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
};

static void build_warnings(iam_lottie* L) {
	static char const* const names[] = {
		"expressions", "effects", "text layers", "image layers", "blend modes", "3D layers",
		"merge paths", "path modifiers", "luma mattes", "gradient highlights", "auto-orient",
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

struct mat { float a, b, c, d, x, y; };  // x' = a x + c y + x, y' = b x + d y + y
static inline mat mat_make(float a, float b, float c, float d, float x, float y) { mat m = { a, b, c, d, x, y }; return m; }
static inline mat mul(mat const& A, mat const& B) {
	return mat_make(A.a * B.a + A.c * B.b, A.b * B.a + A.d * B.b, A.a * B.c + A.c * B.d, A.b * B.c + A.d * B.d,
	                A.a * B.x + A.c * B.y + A.x, A.b * B.x + A.d * B.y + A.y);
}
static inline ImVec2 apply(mat const& M, float x, float y) { return ImVec2(M.a * x + M.c * y + M.x, M.b * x + M.d * y + M.y); }
static inline mat rot(float deg) { float const r = deg * (IM_PI / 180.0f), c = ImCos(r), s = ImSin(r); return mat_make(c, s, -s, c, 0, 0); }
static inline float mat_scale(mat const& M) { return ImSqrt(ImFabs(M.a * M.d - M.b * M.c)); }

struct contour { int first, count; bool closed; };
struct geom {
	ImVector<ImVec2> pts;      // cubic chains: p0, (c1 c2 p) * n
	ImVector<contour> cs;
	void begin(ImVec2 p) { contour c = { pts.Size, 1, false }; cs.push_back(c); pts.push_back(p); }
	void cubic(ImVec2 c1, ImVec2 c2, ImVec2 p) { pts.push_back(c1); pts.push_back(c2); pts.push_back(p); cs.back().count += 3; }
	void line(ImVec2 p) { ImVec2 const a = pts.back(); cubic(a, p, p); }  // Lottie convention: handles on the vertices
	void append(geom const& g) {
		for (int i = 0; i < g.cs.Size; i++) { contour c = g.cs[i]; c.first += pts.Size; cs.push_back(c); }
		for (int i = 0; i < g.pts.Size; i++) pts.push_back(g.pts[i]);
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
	if (e - s <= 1e-6f) { g = out; return; }
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
	g = out;
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
	g = out;
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
		return p->type == 2 ? ImSqrt(dx * dx + dy * dy) * inv_r : (dx * dir.x + dy * dir.y) * inv_len2;
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

	// Convex polygon with per-vertex coverage, coloured by the gradient (t clamped into [lo, hi])
	void paint_poly(ImVector<pvert> const& poly, float lo, float hi) {
		if (poly.Size < 3) return;
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
		float const cell = 4.0f;
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
		dl->PrimReserve(6, 4);
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

static inline bool fill_inside(int winding, int even_odd) { return even_odd ? (winding & 1) != 0 : winding != 0; }

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
                          paint_t const* paint, clip_t const* clips, int nclips) {
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
			if (set == 0) { subj.Add(a); subj.Add(b); }
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

	ImVector<int>& active = S.active;
	ImVector<slab_edge>& slab = S.slab;
	ImVector<float>& sub_ys = S.sub_ys;
	ImVector<span>& prev_bottom = S.prev_bottom, &cur_top = S.cur_top, &cur_bottom = S.cur_bottom, &diff = S.diff;
	ImVector<open_span>& open = S.open, &next_open = S.next_open;
	active.resize(0); prev_bottom.resize(0); open.resize(0);
	float prev_bottom_y = -FLT_MAX;
	int next = 0;

	auto sort_slab = [](ImVector<slab_edge>& v) {
		if (v.Size > 24) {
			ImQsort(v.Data, (size_t)v.Size, sizeof(slab_edge), [](void const* a, void const* b) -> int {
				float const fa = ((slab_edge const*)a)->xa + ((slab_edge const*)a)->xb;
				float const fb = ((slab_edge const*)b)->xa + ((slab_edge const*)b)->xb;
				return fa < fb ? -1 : fa > fb ? 1 : 0; });
			return;
		}
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
		int wnd[MAX_CLIPS + 1] = {};
		auto inside_all = [&]() {
			for (int c = 0; c <= nclips; ++c) if (!fill_inside(wnd[c], rules[c])) return false;
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
		for (int k = 0; k < active.Size; ) {
			if (edges[active[k]].y1 <= ya) { active[k] = active.back(); active.pop_back(); }
			else ++k;
		}
		if (ya > clip.w + aa) { flush_all(ya); break; }  // everything below is clipped
		if (yb < clip.y - aa) { flush_all(ya); continue; }

		// Split the slab at every crossing (insertion sort by bottom x finds them)
		sub_ys.resize(0);
		sub_ys.push_back(ya);
		int crossings = 0;
		if (active.Size > 1 && yb - ya > 0.25f) {  // thinner slabs: crossing order is off by < 0.25 px anyway
			slab.resize(0);
			for (int k = 0; k < active.Size; ++k) {
				fill_edge const& e = edges[active[k]];
				slab_edge se = { edge_x_at(e, ya), edge_x_at(e, yb), 0, active[k], e.set };
				slab.push_back(se);
			}
			ImQsort(slab.Data, (size_t)slab.Size, sizeof(slab_edge), [](void const* a, void const* b) -> int {
				slab_edge const* ea = (slab_edge const*)a; slab_edge const* eb = (slab_edge const*)b;
				if (ea->xa != eb->xa) return ea->xa < eb->xa ? -1 : 1;
				return ea->xb < eb->xb ? -1 : ea->xb > eb->xb ? 1 : 0; });
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
	for (int i = 0; i < num; i++) {
		float2 p0 = segments[i].xy, p1 = segments[i].zw;
		float c = (p1.x - p0.x) * (P.y - p0.y) - (P.x - p0.x) * (p1.y - p0.y);
		if (p0.y <= P.y) { if (p1.y > P.y && c > 0.0) winding++; }
		else if (p1.y <= P.y && c < 0.0) winding--;
		float2 d = p1 - p0;
		float l2 = dot(d, d);
		float t = l2 > 1e-8 ? saturate(dot(P - p0, d) / l2) : 0.0;
		float2 q = P - (p0 + d * t);
		d2 = min(d2, dot(q, q));
	}
	bool inside = params.y > 0.5 ? (winding & 1) != 0 : winding != 0;
	// Inside is opaque so inner edges (stroke joins, overlaps) leave no seam
	float a = inside ? 1.0 : saturate(0.5 - sqrt(d2));
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
		f[0] = (float)bd.count; f[1] = even_odd ? 1.0f : 0.0f;
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
	ImVector<ImVec2> q;
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
	ImVector<ImVec2> d;
	d.resize(nseg);
	for (int i = 0; i < nseg; i++) {
		ImVec2 const a = q[i], b = q[(i + 1) % m];
		float const l = ImSqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
		d[i] = ImVec2((b.x - a.x) / l, (b.y - a.y) / l);
	}
	// Outline: the offset chain of each side (one loop when open, two when closed). The
	// straight link between offsets is the bevel; on the inner side the chain goes through
	// the vertex itself, so inner overlaps add up under non-zero instead of cancelling.
	ImVector<ImVec2> A, B;
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
	ImVector<ImVec2> rb;
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
	ImVector<ImVec2> piece;
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

	mat xform(xform_t const& x, float t, float* opacity) {
		float a[2], p[2], s[2];
		pv2(x.a, t, 0.0f, a);
		if (x.split) { p[0] = pv(x.px, t, 0, 0.0f); p[1] = pv(x.py, t, 0, 0.0f); }
		else pv2(x.p, t, 0.0f, p);
		pv2(x.s, t, 100.0f, s);
		float const r = pv(x.r, t, 0, 0.0f), sk = pv(x.sk, t, 0, 0.0f), sa = pv(x.sa, t, 0, 0.0f);
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
		c.segs = segs;
		c.even_odd = even_odd;
		c.bounds = ImRect(ImVec2(FLT_MAX, FLT_MAX), ImVec2(-FLT_MAX, -FLT_MAX));
		for (int i = 0; i < segs.Size; i++) c.bounds.Add(segs[i]);
	}
	void pop_clip() { if (overflow > 0) overflow--; else if (nclips > 0) nclips--; }

	// Region outside `segs` (positive outlines): a big rectangle minus them
	void complement(ImVector<ImVec2>& out, ImVector<ImVec2> const& segs) {
		ImVec4 const cr = dl->_CmdHeader.ClipRect;
		ImVec2 const r[4] = { ImVec2(cr.x - 4, cr.y - 4), ImVec2(cr.z + 4, cr.y - 4), ImVec2(cr.z + 4, cr.w + 4), ImVec2(cr.x - 4, cr.w + 4) };
		out.resize(0);
		add_poly(out, r, 4);
		for (int i = 0; i + 1 < segs.Size; i += 2) { out.push_back(segs[i + 1]); out.push_back(segs[i]); }
	}

	void fill(ImVector<ImVec2> const& segs, ImU32 col, int even_odd, paint_t const* paint) {
		if (collect) { for (int i = 0; i < segs.Size; i++) collect->push_back(segs[i]); return; }
#if IAM_LOTTIE_GPU
		if (!paint && nclips == 0 && gpu_fill(dl, segs.Data, segs.Size / 2, col, even_odd)) return;  // gradients and clips stay on the CPU
#endif
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
				ImVector<float> v;
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
					geom sub;
					collect_geom(sh.child, sh.childn, mul(M, xform(sh.tr, t, nullptr)), t, sub);
					depth--;
					out.append(sub);
					break;
				}
				case SK_TRIM:
					trim(out, pv(sh.p[0], t, 0, 0.0f) * 0.01f, pv(sh.p[1], t, 0, 100.0f) * 0.01f, pv(sh.p[2], t, 0, 0.0f) / 360.0f, sh.mode, tol);
					break;
				case SK_ROUND: round_corners(out, pv(sh.p[0], t, 0, 0.0f) * mat_scale(M)); break;
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
			float const r = scratch.Size > 0 ? scratch[0] : 0.0f, gr = scratch.Size > 1 ? scratch[1] : 0.0f, b = scratch.Size > 2 ? scratch[2] : 0.0f;
			float const a = scratch.Size > 3 ? scratch[3] : 1.0f;
			col = ImGui::ColorConvertFloat4ToU32(ImVec4(r, gr, b, ImSaturate(a) * ImSaturate(opacity)));
			if (collect) col |= IM_COL32_A_MASK;
		}
		ImVector<ImVec2> poly, segs;
		if (!stroke) {
			for (int c = 0; c < g.cs.Size; c++) {
				flatten_contour(g, g.cs[c], poly, tol);
				if (poly.Size < 3) continue;
				if (collect) { add_poly(segs, poly.Data, poly.Size); continue; }  // matte: union of positive outlines
				for (int i = 0; i < poly.Size; i++) { segs.push_back(poly[i]); segs.push_back(poly[(i + 1) % poly.Size]); }
			}
			fill(segs, col, sh.rule == 2, grad ? &pt : nullptr);
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
					geom g;
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

	mat layer_world(int li, float t, float* opacity) {
		layer_t const& ly = L->layers[li];
		mat W = xform(ly.ks, (t - ly.st) / ly.sr, opacity);
		int p = ly.parent;
		for (int guard = 0; p >= 0 && guard < 32; guard++) {
			layer_t const& pl = L->layers[p];
			W = mul(xform(pl.ks, (t - pl.st) / pl.sr, nullptr), W);  // parents pass their transform, not their opacity
			p = pl.parent;
		}
		return W;
	}

	int push_masks(layer_t const& ly, mat const& W, float lt) {
		int pushed = 0;
		ImVector<ImVec2> uni, segs, poly;
		auto flush = [&]() { if (uni.Size) { push_clip(uni, 0); pushed++; uni.resize(0); } };
		for (int i = 0; i < ly.maskn; i++) {
			mask_t const& mk = L->masks[ly.mask + i];
			if (mk.mode == 'n') continue;
			geom g;
			eval(L, mk.pt, lt, scratch);
			if (scratch.Size >= 2) {
				int const n = (int)scratch[1];
				if (n > 0 && scratch.Size >= 2 + 6 * n) add_vertices(g, W, scratch.Data + 2, n, true, false);
			}
			segs.resize(0);
			for (int c = 0; c < g.cs.Size; c++) { flatten_contour(g, g.cs[c], poly, tol); if (poly.Size >= 3) add_poly(segs, poly.Data, poly.Size); }
			bool const subtract = (mk.mode == 's') != (mk.inv != 0);
			if (mk.mode != 's' && mk.mode != 'i' && !mk.inv) { for (int k = 0; k < segs.Size; k++) uni.push_back(segs[k]); continue; }  // add (and other modes): union
			flush();
			if (subtract) { ImVector<ImVec2> comp; complement(comp, segs); push_clip(comp, 0); }
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
		float const a = alpha * op;
		float const lt = (t - ly.st) / ly.sr;
		int pushed = 0;
		if (a > 0.0f || collect) {
			if (ly.matte >= 0 && ly.matte != li) {
				ImVector<ImVec2> matte;
				ImVector<ImVec2>* const prev = collect;
				collect = &matte;
				layer_t const& ml = L->layers[ly.matte];
				if (t >= ml.ip && t < ml.op) render_layer(ly.matte, t, M, 1.0f);
				collect = prev;
				if (ly.tt == 2 || ly.tt == 4) { ImVector<ImVec2> comp; complement(comp, matte); push_clip(comp, 0); }
				else push_clip(matte, 0);
				pushed++;
			}
			pushed += push_masks(ly, W, lt);
			switch (ly.ty) {
				case 4: render_group(ly.shapes, ly.shapesn, W, a, lt); break;
				case 1: {
					ImVec2 const r[4] = { apply(W, 0, 0), apply(W, ly.sw, 0), apply(W, ly.sw, ly.sh), apply(W, 0, ly.sh) };
					ImVector<ImVec2> segs;
					add_poly(segs, r, 4);
					fill(segs, ImGui::ColorConvertFloat4ToU32(ImVec4(ly.solid[0], ly.solid[1], ly.solid[2], collect ? 1.0f : ImSaturate(a))), 0, nullptr);
					break;
				}
				case 0:
					if (ly.comp >= 0) render_comp(ly.comp, present(ly.tm) ? pv(ly.tm, lt, 0, 0.0f) * L->fr : lt, W, a);
					break;
				default: break;
			}
		}
		while (pushed-- > 0) pop_clip();
		depth--;
	}

	void render_comp(int ci, float t, mat const& M, float alpha) {
		comp_t const& cp = L->comps[ci];
		for (int i = cp.layern - 1; i >= 0; --i) {
			layer_t const& ly = L->layers[cp.layer + i];
			if (ly.td || ly.hidden || ly.ty == 3) continue;  // matte sources are drawn through the layer they matte
			if (t < ly.ip || t >= ly.op) continue;
			render_layer(cp.layer + i, t, M, alpha);
		}
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
	L->w = (float)doc.num(root, "w", 0.0);
	L->h = (float)doc.num(root, "h", 0.0);
	L->fr = (float)doc.num(root, "fr", 0.0);
	L->ip = (float)doc.num(root, "ip", 0.0);
	L->op = (float)doc.num(root, "op", 0.0);
	if (!(L->w > 0.0f) || !(L->h > 0.0f) || !(L->fr > 0.0f) || !(L->op > L->ip)) { IM_DELETE(L); return nullptr; }

	builder B;
	B.L = L;
	B.D = &doc;
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
	float const frame = ImClamp(anim->ip + time * anim->fr, anim->ip, anim->op - 1e-3f);
	float const k = ImMin(size.x / anim->w, size.y / anim->h);
	ctx C;
	C.dl = dl;
	C.L = anim;
	C.nclips = C.overflow = C.depth = 0;
	C.collect = nullptr;
	C.tol = 0.25f;
	mat const M = mat_make(k, 0, 0, k, pos.x + (size.x - anim->w * k) * 0.5f, pos.y + (size.y - anim->h * k) * 0.5f);
	C.render_comp(0, frame, M, 1.0f);
}

bool iam_lottie_gpu_active() {
#if IAM_LOTTIE_GPU
	return gpu_program() != nullptr;
#else
	return false;
#endif
}

void iam_lottie_set_gpu(bool enabled) { s_gpu_enabled = enabled; }
