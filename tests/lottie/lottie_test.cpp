// Lottie player: fill rules, keyframes, strokes, trim, masks, mattes, precomps, gradients, robustness.
#include "im_anim_lottie.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <string>

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { g_fail++; printf("FAIL %d: ", __LINE__); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)

// Composites the triangles drawn since (vtx0, idx0) over point p: returns alpha, rgb in out
static float coverage(ImDrawList* dl, int idx0, ImVec2 p, float* rgb = nullptr) {
	float a = 0.0f, c[3] = { 0, 0, 0 };
	for (int i = idx0; i + 2 < dl->IdxBuffer.Size; i += 3) {
		ImDrawVert const& A = dl->VtxBuffer[dl->IdxBuffer[i]];
		ImDrawVert const& B = dl->VtxBuffer[dl->IdxBuffer[i + 1]];
		ImDrawVert const& C = dl->VtxBuffer[dl->IdxBuffer[i + 2]];
		float const d = (B.pos.y - C.pos.y) * (A.pos.x - C.pos.x) + (C.pos.x - B.pos.x) * (A.pos.y - C.pos.y);
		if (fabsf(d) < 1e-9f) continue;
		float const l1 = ((B.pos.y - C.pos.y) * (p.x - C.pos.x) + (C.pos.x - B.pos.x) * (p.y - C.pos.y)) / d;
		float const l2 = ((C.pos.y - A.pos.y) * (p.x - C.pos.x) + (A.pos.x - C.pos.x) * (p.y - C.pos.y)) / d;
		float const l3 = 1.0f - l1 - l2;
		if (l1 < -1e-5f || l2 < -1e-5f || l3 < -1e-5f) continue;
		ImVec4 const ca = ImGui::ColorConvertU32ToFloat4(A.col), cb = ImGui::ColorConvertU32ToFloat4(B.col), cc = ImGui::ColorConvertU32ToFloat4(C.col);
		float const ta = ca.w * l1 + cb.w * l2 + cc.w * l3;
		float const col[3] = { ca.x * l1 + cb.x * l2 + cc.x * l3, ca.y * l1 + cb.y * l2 + cc.y * l3, ca.z * l1 + cb.z * l2 + cc.z * l3 };
		for (int k = 0; k < 3; k++) c[k] = col[k] * ta + c[k] * (1.0f - ta);
		a = ta + a * (1.0f - ta);
	}
	if (rgb) for (int k = 0; k < 3; k++) rgb[k] = a > 0.0f ? c[k] / a : 0.0f;
	return a;
}

static std::string doc(std::string const& layers, std::string const& extra = "") {
	return "{\"v\":\"5.7.0\",\"fr\":30,\"ip\":0,\"op\":60,\"w\":200,\"h\":200," + extra + "\"layers\":[" + layers + "]}";
}
static std::string val(std::string const& v) { return "{\"a\":0,\"k\":" + v + "}"; }
static std::string layer(std::string const& shapes, std::string const& more = "") {
	return "{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{}," + more + "\"shapes\":[" + shapes + "]}";
}
static std::string group(std::string const& items) { return "{\"ty\":\"gr\",\"it\":[" + items + ",{\"ty\":\"tr\"}]}"; }
static std::string rect(float cx, float cy, float w, float h) {
	char b[256];
	snprintf(b, sizeof(b), "{\"ty\":\"rc\",\"p\":{\"a\":0,\"k\":[%g,%g]},\"s\":{\"a\":0,\"k\":[%g,%g]},\"r\":{\"a\":0,\"k\":0}}", cx, cy, w, h);
	return b;
}
static std::string fill(float r, float g, float b, int rule = 1) {
	char s[160];
	snprintf(s, sizeof(s), "{\"ty\":\"fl\",\"c\":{\"a\":0,\"k\":[%g,%g,%g,1]},\"o\":{\"a\":0,\"k\":100},\"r\":%d}", r, g, b, rule);
	return s;
}
static std::string line_path(float x0, float y0, float x1, float y1) {
	char b[256];
	snprintf(b, sizeof(b), "{\"ty\":\"sh\",\"ks\":{\"a\":0,\"k\":{\"c\":false,\"v\":[[%g,%g],[%g,%g]],\"i\":[[0,0],[0,0]],\"o\":[[0,0],[0,0]]}}}", x0, y0, x1, y1);
	return b;
}
static std::string stroke(float w, int cap) {
	char b[200];
	snprintf(b, sizeof(b), "{\"ty\":\"st\",\"c\":{\"a\":0,\"k\":[0,1,0,1]},\"o\":{\"a\":0,\"k\":100},\"w\":{\"a\":0,\"k\":%g},\"lc\":%d,\"lj\":1,\"ml\":4}", w, cap);
	return b;
}

static ImDrawList* g_dl;
static int g_idx0;
// Draws the animation at `time` (composition at 0,0, native size) into a fresh draw list section
static iam_lottie* draw(std::string const& json, float time) {
	iam_lottie* a = iam_lottie_load_from_memory(json.data(), json.size());
	g_idx0 = g_dl->IdxBuffer.Size;
	if (a) iam_lottie_draw(g_dl, a, time, iam_v2(0, 0));
	return a;
}
static float cov_at(float x, float y, float* rgb = nullptr) { return coverage(g_dl, g_idx0, ImVec2(x, y), rgb); }

int main() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(800, 600); io.DeltaTime = 1.0f / 60.0f; io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	ImGui::NewFrame();
	g_dl = ImGui::GetForegroundDrawList();

	// ---------------- Fill rules ("O": two rects with the same direction) ----------------
	{
		std::string const nz = doc(layer(group(rect(100, 100, 100, 100) + "," + rect(100, 100, 40, 40) + "," + fill(1, 0, 0, 1))));
		std::string const eo = doc(layer(group(rect(100, 100, 100, 100) + "," + rect(100, 100, 40, 40) + "," + fill(1, 0, 0, 2))));
		iam_lottie* a = draw(nz, 0.0f);
		CHECK(a != nullptr, "load");
		CHECK(cov_at(100, 100) > 0.99f && cov_at(65, 100) > 0.99f && cov_at(20, 20) < 0.01f, "non-zero fills the centre (%.2f) and ring", cov_at(100, 100));
		CHECK(cov_at(50.0f, 97) > 0.3f && cov_at(50.0f, 97) < 0.7f, "anti-aliased edge (%.2f)", cov_at(50.0f, 97));
		iam_lottie_free(a);
		a = draw(eo, 0.0f);
		CHECK(cov_at(100, 100) < 0.01f && cov_at(65, 100) > 0.99f && cov_at(20, 20) < 0.01f, "even-odd leaves a hole (%.2f) and fills the ring (%.2f)", cov_at(100, 100), cov_at(65, 100));
		float rgb[3];
		cov_at(65, 100, rgb);
		CHECK(rgb[0] > 0.99f && rgb[1] < 0.01f, "fill colour");
		CHECK(iam_lottie_duration(a) == 2.0f && iam_lottie_frame_rate(a) == 30.0f && IAM_VEC2_TO_IM(iam_lottie_size(a)).x == 200.0f, "duration / fps / size");
		CHECK(strcmp(iam_lottie_warnings(a), "") == 0, "no warnings");
		iam_lottie_free(a);
	}

	// ---------------- Keyframes: linear, eased, hold, curved motion ----------------
	{
		std::string const el = group("{\"ty\":\"el\",\"p\":" + val("[0,0]") + ",\"s\":" + val("[20,20]") + "}," + fill(1, 1, 1));
		std::string const lin = "\"ks\":{\"p\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[50,100],\"o\":{\"x\":0,\"y\":0},\"i\":{\"x\":1,\"y\":1}},{\"t\":30,\"s\":[150,100]}]}},";
		std::string json = doc("{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":60,\"st\":0," + lin + "\"shapes\":[" + el + "]}");
		iam_lottie* a = draw(json, 0.5f);
		CHECK(cov_at(100, 100) > 0.99f && cov_at(50, 100) < 0.01f, "linear key: frame 15 at x=100");
		iam_lottie_free(a);
		a = draw(json, 1.5f);
		CHECK(cov_at(150, 100) > 0.99f, "after the last key: holds the last value");
		iam_lottie_free(a);
		std::string hold = lin;
		hold.replace(hold.find("\"o\":"), 0, "\"h\":1,");
		a = draw(doc("{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":60,\"st\":0," + hold + "\"shapes\":[" + el + "]}"), 0.5f);
		CHECK(cov_at(50, 100) > 0.99f && cov_at(100, 100) < 0.01f, "hold key stays at the start");
		iam_lottie_free(a);
		// ease-in (0.9,0 -> 1,1): at the middle it has barely moved
		std::string ease = lin;
		ease.replace(ease.find("\"x\":0,\"y\":0"), 11, "\"x\":0.9,\"y\":0");
		a = draw(doc("{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":60,\"st\":0," + ease + "\"shapes\":[" + el + "]}"), 0.5f);
		CHECK(cov_at(70, 100) > 0.99f && cov_at(100, 100) < 0.01f, "bezier ease");
		iam_lottie_free(a);
		// spatial tangents: arc up through y=50 (to/ti pull the path above the chord)
		std::string arc = "\"ks\":{\"p\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[50,100],\"to\":[0,-66.67],\"ti\":[0,-66.67],\"o\":{\"x\":0,\"y\":0},\"i\":{\"x\":1,\"y\":1}},{\"t\":30,\"s\":[150,100]}]}},";
		a = draw(doc("{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":60,\"st\":0," + arc + "\"shapes\":[" + el + "]}"), 0.5f);
		CHECK(cov_at(100, 50) > 0.9f && cov_at(100, 100) < 0.01f, "curved motion path (%.2f)", cov_at(100, 50));
		iam_lottie_free(a);
	}

	// ---------------- Strokes, caps, dashes, trim ----------------
	{
		iam_lottie* a = draw(doc(layer(group(line_path(20, 100, 180, 100) + "," + stroke(10, 1)))), 0.0f);
		CHECK(cov_at(100, 100) > 0.99f && cov_at(100, 108) < 0.01f && cov_at(15, 100) < 0.01f, "butt stroke");
		iam_lottie_free(a);
		a = draw(doc(layer(group(line_path(20, 100, 180, 100) + "," + stroke(10, 2)))), 0.0f);
		CHECK(cov_at(17, 100) > 0.99f, "round cap");
		CHECK(cov_at(100, 100) > 0.99f && cov_at(100, 100) < 1.01f, "overlapping pieces are filled once (%.3f)", cov_at(100, 100));
		iam_lottie_free(a);
		a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + stroke(10, 1)))), 0.0f);
		CHECK(cov_at(52, 100) > 0.99f && cov_at(60, 100) < 0.01f && cov_at(100, 100) < 0.01f && cov_at(100, 47) > 0.99f, "closed stroke is a ring");
		iam_lottie_free(a);
		a = draw(doc(layer(group("{\"ty\":\"el\",\"p\":" + val("[100,100]") + ",\"s\":" + val("[100,100]") + "}," + stroke(10, 1)))), 0.0f);
		{
			int holes = 0;
			for (int i = 0; i < 720; i++) {
				float const an = i * 0.5f * 3.14159265f / 180.0f;
				for (float r = 46.5f; r <= 53.5f; r += 1.75f) if (cov_at(100 + cosf(an) * r, 100 + sinf(an) * r) < 0.99f) holes++;
			}
			CHECK(holes == 0, "curved stroke has no holes or notches (%d)", holes);
		}
		iam_lottie_free(a);
		auto corner = [](int join) {
			std::string s = stroke(20, 1);
			s.replace(s.find("\"lj\":1"), 6, join == 1 ? "\"lj\":1" : "\"lj\":3");
			return "{\"ty\":\"sh\",\"ks\":{\"a\":0,\"k\":{\"c\":false,\"v\":[[50,150],[150,150],[150,50]],\"i\":[[0,0],[0,0],[0,0]],\"o\":[[0,0],[0,0],[0,0]]}}}," + s;
		};
		a = draw(doc(layer(group(corner(1)))), 0.0f);
		CHECK(cov_at(143, 143) > 0.99f && cov_at(158, 158) > 0.99f && cov_at(100, 143) > 0.99f, "miter join: inner corner and outer tip filled");
		CHECK(cov_at(145, 145) < 1.01f, "inner overlap filled once");
		iam_lottie_free(a);
		a = draw(doc(layer(group(corner(3)))), 0.0f);
		CHECK(cov_at(143, 143) > 0.99f && cov_at(158, 158) < 0.01f && cov_at(153, 153) > 0.99f && cov_at(151, 151) > 0.99f, "bevel join cuts the tip, no notch");
		iam_lottie_free(a);
		a = draw(doc(layer(group("{\"ty\":\"sh\",\"ks\":{\"a\":0,\"k\":{\"c\":false,\"v\":[[20,100],[180,100],[20,106]],\"i\":[[0,0],[0,0],[0,0]],\"o\":[[0,0],[0,0],[0,0]]}}}," + stroke(20, 1)))), 0.0f);
		CHECK(cov_at(100, 103) > 0.99f && cov_at(170, 103) > 0.99f && cov_at(100, 92) > 0.99f && cov_at(100, 112) > 0.99f && cov_at(100, 116) < 0.01f, "hairpin turn stays filled");
		iam_lottie_free(a);
		std::string const trim ="{\"ty\":\"tm\",\"s\":" + val("0") + ",\"e\":" + val("50") + ",\"o\":" + val("0") + ",\"m\":1}";
		a = draw(doc(layer(group(line_path(20, 100, 180, 100) + "," + trim + "," + stroke(10, 1)))), 0.0f);
		CHECK(cov_at(60, 100) > 0.99f && cov_at(140, 100) < 0.01f, "trim paths keep the first half");
		iam_lottie_free(a);
		std::string dashed = stroke(10, 1);
		dashed.insert(dashed.size() - 1, ",\"d\":[{\"n\":\"d\",\"v\":" + val("20") + "},{\"n\":\"g\",\"v\":" + val("20") + "}]");
		a = draw(doc(layer(group(line_path(20, 100, 180, 100) + "," + dashed))), 0.0f);
		CHECK(cov_at(30, 100) > 0.99f && cov_at(50, 100) < 0.01f && cov_at(70, 100) > 0.99f, "dashes");
		iam_lottie_free(a);
	}

	// ---------------- Masks and mattes ----------------
	{
		std::string const sq = "{\"c\":true,\"v\":[[80,80],[120,80],[120,120],[80,120]],\"i\":[[0,0],[0,0],[0,0],[0,0]],\"o\":[[0,0],[0,0],[0,0],[0,0]]}";
		std::string const mask = "\"masksProperties\":[{\"mode\":\"s\",\"inv\":false,\"pt\":" + val(sq) + ",\"o\":" + val("100") + "}],";
		iam_lottie* a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + fill(1, 0, 0)), mask)), 0.0f);
		CHECK(cov_at(100, 100) < 0.01f && cov_at(60, 100) > 0.99f, "subtract mask");
		iam_lottie_free(a);
		std::string madd = mask;
		madd.replace(madd.find("\"s\""), 3, "\"a\"");
		a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + fill(1, 0, 0)), madd)), 0.0f);
		CHECK(cov_at(100, 100) > 0.99f && cov_at(60, 100) < 0.01f, "add mask");
		iam_lottie_free(a);
		// Track matte: layer 1 (td) is the matte of layer 2
		std::string const matte = "{\"ty\":4,\"ind\":1,\"td\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{},\"shapes\":[" + group(rect(100, 100, 40, 40) + "," + fill(1, 1, 1)) + "]}";
		std::string target = "{\"ty\":4,\"ind\":2,\"tt\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{},\"shapes\":[" + group(rect(100, 100, 100, 100) + "," + fill(0, 0, 1)) + "]}";
		a = draw(doc(matte + "," + target), 0.0f);
		CHECK(cov_at(100, 100) > 0.99f && cov_at(60, 100) < 0.01f, "alpha matte");
		float rgb[3];
		cov_at(100, 100, rgb);
		CHECK(rgb[2] > 0.99f && rgb[0] < 0.01f, "matte layer itself is not drawn");
		iam_lottie_free(a);
		target.replace(target.find("\"tt\":1"), 6, "\"tt\":2");
		a = draw(doc(matte + "," + target), 0.0f);
		CHECK(cov_at(100, 100) < 0.01f && cov_at(60, 100) > 0.99f, "inverted alpha matte");
		iam_lottie_free(a);
	}

	// ---------------- Parenting, precomps, solids, time remap ----------------
	{
		std::string const null_layer = "{\"ty\":3,\"ind\":5,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{\"p\":" + val("[50,0]") + "}}";
		std::string const child = "{\"ty\":4,\"ind\":1,\"parent\":5,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{},\"shapes\":[" + group(rect(50, 100, 20, 20) + "," + fill(1, 1, 1)) + "]}";
		iam_lottie* a = draw(doc(child + "," + null_layer), 0.0f);
		CHECK(cov_at(100, 100) > 0.99f && cov_at(50, 100) < 0.01f, "parent transform moves the child");
		iam_lottie_free(a);
		std::string const assets = "\"assets\":[{\"id\":\"comp_0\",\"layers\":[" + layer(group(rect(10, 10, 20, 20) + "," + fill(0, 1, 0))) + "]}],";
		std::string const pre = "{\"ty\":0,\"ind\":1,\"refId\":\"comp_0\",\"w\":200,\"h\":200,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{\"p\":" + val("[150,150]") + "}}";
		a = draw(doc(pre, assets), 0.0f);
		CHECK(cov_at(160, 160) > 0.99f && cov_at(10, 10) < 0.01f, "precomp layer");
		iam_lottie_free(a);
		std::string const solid = "{\"ty\":1,\"ind\":1,\"sc\":\"#0000ff\",\"sw\":50,\"sh\":50,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{}}";
		a = draw(doc(solid), 0.0f);
		float rgb[3];
		CHECK(cov_at(25, 25, rgb) > 0.99f && rgb[2] > 0.99f && cov_at(75, 75) < 0.01f, "solid layer");
		iam_lottie_free(a);
		// Layer in/out points
		std::string late = layer(group(rect(100, 100, 50, 50) + "," + fill(1, 1, 1)));
		late.replace(late.find("\"ip\":0"), 6, "\"ip\":30");
		a = draw(doc(late), 0.5f);
		CHECK(cov_at(100, 100) < 0.01f, "layer hidden before its in point");
		iam_lottie_free(a);
		a = draw(doc(late), 1.5f);
		CHECK(cov_at(100, 100) > 0.99f, "layer visible after its in point");
		iam_lottie_free(a);
	}

	// ---------------- Gradient, star, repeater, round corners ----------------
	{
		std::string const gf = "{\"ty\":\"gf\",\"o\":" + val("100") + ",\"r\":1,\"t\":1,\"s\":" + val("[50,0]") + ",\"e\":" + val("[150,0]") +
		                       ",\"g\":{\"p\":2,\"k\":" + val("[0,1,0,0,1,0,0,1]") + "}}";
		iam_lottie* a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + gf))), 0.0f);
		float l[3], r[3];
		cov_at(55, 100, l);
		cov_at(145, 100, r);
		CHECK(l[0] > 0.9f && l[2] < 0.1f && r[2] > 0.9f && r[0] < 0.1f, "linear gradient red -> blue (%.2f %.2f)", l[0], r[2]);
		iam_lottie_free(a);
		std::string const star = "{\"ty\":\"sr\",\"sy\":1,\"p\":" + val("[100,100]") + ",\"or\":" + val("50") + ",\"ir\":" + val("20") + ",\"os\":" + val("0") +
		                         ",\"is\":" + val("0") + ",\"r\":" + val("0") + ",\"pt\":" + val("5") + "}";
		a = draw(doc(layer(group(star + "," + fill(1, 1, 1)))), 0.0f);
		CHECK(cov_at(100, 100) > 0.99f && cov_at(100, 55) > 0.99f && cov_at(70, 60) < 0.01f, "star (tip up)");
		iam_lottie_free(a);
		std::string const rep = "{\"ty\":\"rp\",\"c\":" + val("3") + ",\"o\":" + val("0") + ",\"m\":1,\"tr\":{\"ty\":\"tr\",\"p\":" + val("[50,0]") +
		                        ",\"a\":" + val("[0,0]") + ",\"s\":" + val("[100,100]") + ",\"r\":" + val("0") + ",\"so\":" + val("100") + ",\"eo\":" + val("100") + "}}";
		a = draw(doc(layer(group(rect(30, 100, 20, 20) + "," + fill(1, 1, 1) + "," + rep))), 0.0f);
		CHECK(cov_at(30, 100) > 0.99f && cov_at(80, 100) > 0.99f && cov_at(130, 100) > 0.99f && cov_at(180, 100) < 0.01f, "repeater: 3 copies");
		iam_lottie_free(a);
		std::string const rd = "{\"ty\":\"rd\",\"r\":" + val("20") + "}";
		a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + rd + "," + fill(1, 1, 1)))), 0.0f);
		CHECK(cov_at(52, 52) < 0.01f && cov_at(100, 52) > 0.99f, "round corners");
		iam_lottie_free(a);
	}

	// ---------------- Markers, warnings, robustness ----------------
	{
		std::string const m = doc(layer(group(rect(100, 100, 10, 10) + "," + fill(1, 1, 1))), "\"markers\":[{\"tm\":30,\"cm\":\"loop\",\"dr\":15}],");
		iam_lottie* a = iam_lottie_load_from_memory(m.data(), m.size());
		float st = 0, du = 0;
		CHECK(iam_lottie_marker(a, "loop", &st, &du) && st == 1.0f && du == 0.5f && !iam_lottie_marker(a, "nope", nullptr, nullptr), "markers");
		iam_lottie_free(a);
		std::string const w = doc("{\"ty\":5,\"ind\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{\"o\":{\"a\":0,\"k\":100,\"x\":\"value * 2\"}}}");
		a = iam_lottie_load_from_memory(w.data(), w.size());
		CHECK(a && strstr(iam_lottie_warnings(a), "text layers") && strstr(iam_lottie_warnings(a), "expressions"), "warnings: %s", a ? iam_lottie_warnings(a) : "");
		iam_lottie_free(a);

		char const* bad[] = { "", "{", "[1,2]", "{\"w\":1}", "{\"w\":100,\"h\":100,\"fr\":0,\"ip\":0,\"op\":10,\"layers\":[]}", "{\"a\":tru}", "{\"a\":1,}", "\"x" };
		bool all_null = true;
		for (int i = 0; i < IM_ARRAYSIZE(bad); i++) if (iam_lottie_load_from_memory(bad[i], strlen(bad[i]))) all_null = false;
		CHECK(all_null, "malformed / invalid documents are rejected");
		std::string deep(5000, '[');
		CHECK(iam_lottie_load_from_memory(deep.data(), deep.size()) == nullptr, "deep nesting rejected");
		// Every truncation of a valid document: no crash, and the full one loads
		std::string const full = doc(layer(group(rect(100, 100, 100, 100) + "," + rect(100, 100, 40, 40) + "," + line_path(0, 0, 50, 50) + "," + stroke(4, 2) + "," + fill(1, 0, 0, 2))), "\"markers\":[{\"tm\":3,\"cm\":\"a\",\"dr\":1}],");
		int loaded = 0;
		for (size_t n = 0; n <= full.size(); n++) {
			iam_lottie* t = iam_lottie_load_from_memory(full.data(), n);
			if (t) { loaded++; iam_lottie_draw(g_dl, t, 0.3f, iam_v2(0, 0)); iam_lottie_free(t); }
		}
		CHECK(loaded == 1, "only the complete document loads (%d)", loaded);
		iam_lottie_draw(g_dl, nullptr, 0.0f, iam_v2(0, 0));
		iam_lottie_free(nullptr);
		CHECK(iam_lottie_load("does/not/exist.json") == nullptr, "missing file");
	}

	ImGui::EndFrame();
	ImGui::DestroyContext();
	printf("%s (%d failures)\n", g_fail ? "FAILED" : "PASSED", g_fail);
	return g_fail ? 1 : 0;
}
