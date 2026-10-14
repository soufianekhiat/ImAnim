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
		unsigned vofs = 0;  // index values are relative to their command's vertex offset
		for (int k = 0; k < dl->CmdBuffer.Size; k++) if ((int)dl->CmdBuffer[k].IdxOffset <= i) vofs = dl->CmdBuffer[k].VtxOffset;
		ImDrawVert const& A = dl->VtxBuffer[vofs + dl->IdxBuffer[i]];
		ImDrawVert const& B = dl->VtxBuffer[vofs + dl->IdxBuffer[i + 1]];
		ImDrawVert const& C = dl->VtxBuffer[vofs + dl->IdxBuffer[i + 2]];
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
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset;
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
		// Key times are comp times: a layer's start time ("st") does not shift them (lottie-web)
		a = draw(doc("{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":60,\"st\":20," + lin + "\"shapes\":[" + el + "]}"), 0.5f);
		CHECK(cov_at(100, 100) > 0.99f, "start time does not offset key times");
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
		{
			// Inverted matte made of two overlapping shapes: their overlap stays hidden too
			std::string const matte2 = "{\"ty\":4,\"ind\":1,\"td\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{},\"shapes\":[" +
				group(rect(90, 100, 40, 40) + "," + fill(1, 1, 1)) + "," + group(rect(110, 100, 40, 40) + "," + fill(1, 1, 1)) + "]}";
			std::string inv = target;
			inv.replace(inv.find("\"tt\":1"), 6, "\"tt\":2");
			iam_lottie* m2 = draw(doc(matte2 + "," + inv), 0.0f);
			CHECK(cov_at(100, 103) < 0.01f && cov_at(80, 103) < 0.01f && cov_at(60, 103) > 0.99f, "inverted matte with overlapping shapes");
			iam_lottie_free(m2);
		}
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
		std::string const pb = "{\"ty\":\"pb\",\"a\":" + val("50") + "}";
		a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + pb + "," + fill(1, 1, 1)))), 0.0f);
		CHECK(cov_at(100, 100) > 0.99f && cov_at(100, 45) > 0.99f && cov_at(55, 55) < 0.01f, "bloat: corners pulled in, edges bulge");
		iam_lottie_free(a);
		for (int mode = 1; mode <= 5; mode++) {
			std::string const mm = "{\"ty\":\"mm\",\"mm\":" + std::to_string(mode) + "}";
			a = draw(doc(layer(group(rect(80, 100, 80, 80) + "," + rect(120, 100, 80, 80) + "," + mm + "," + fill(1, 1, 1)))), 0.0f);
			bool const only_a = cov_at(57, 103) > 0.99f, both = cov_at(97, 103) > 0.99f, only_b = cov_at(143, 103) > 0.99f;
			bool const want[6][3] = { {}, { true, true, true }, { true, true, true }, { true, false, false }, { false, true, false }, { true, false, true } };
			CHECK(only_a == want[mode][0] && both == want[mode][1] && only_b == want[mode][2] && strcmp(iam_lottie_warnings(a), "") == 0,
			      "merge paths mode %d (A %d, A&B %d, B %d)", mode, only_a, both, only_b);
			iam_lottie_free(a);
		}
		auto twist = [](float angle) { char b[120]; snprintf(b, sizeof(b), "{\"ty\":\"tw\",\"a\":{\"a\":0,\"k\":%g},\"c\":{\"a\":0,\"k\":[100,100]}}", angle); return std::string(b); };
		a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + twist(0) + "," + fill(1, 1, 1)))), 0.0f);
		CHECK(cov_at(122, 52) > 0.99f && strcmp(iam_lottie_warnings(a), "") == 0, "twist 0 leaves the shape");
		iam_lottie_free(a);
		a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + twist(90) + "," + fill(1, 1, 1)))), 0.0f);
		CHECK(cov_at(122, 52) < 0.01f && cov_at(122, 59) > 0.99f && cov_at(100, 100) > 0.99f, "twist bends the edges around the centre");
		iam_lottie_free(a);
		std::string const zz ="{\"ty\":\"zz\",\"s\":" + val("10") + ",\"r\":" + val("3") + ",\"pt\":" + val("1") + "}";
		a = draw(doc(layer(group(line_path(20, 100, 180, 100) + "," + zz + "," + stroke(2, 1)))), 0.0f);
		CHECK(cov_at(60, 90) > 0.9f && cov_at(100, 110) > 0.9f && cov_at(60, 100) < 0.01f, "zig-zag: alternating ridges");
		iam_lottie_free(a);
	}

	// ---------------- Auto-orient, mask / matte opacity ----------------
	{
		std::string const bar = group(rect(0, 0, 40, 6) + "," + fill(1, 1, 1));
		std::string const move = "\"ks\":{\"p\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[100,50],\"o\":{\"x\":0,\"y\":0},\"i\":{\"x\":1,\"y\":1}},{\"t\":30,\"s\":[100,150]}]}},";
		iam_lottie* a = draw(doc("{\"ty\":4,\"ind\":1,\"ao\":1,\"ip\":0,\"op\":60,\"st\":0," + move + "\"shapes\":[" + bar + "]}"), 0.5f);
		CHECK(cov_at(100, 115) > 0.99f && cov_at(115, 100) < 0.01f, "auto-orient turns the layer along its path");
		iam_lottie_free(a);
		a = draw(doc("{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":60,\"st\":0," + move + "\"shapes\":[" + bar + "]}"), 0.5f);
		CHECK(cov_at(115, 100) > 0.99f && cov_at(100, 115) < 0.01f, "without auto-orient");
		iam_lottie_free(a);
		std::string const sq = "{\"c\":true,\"v\":[[80,80],[120,80],[120,120],[80,120]],\"i\":[[0,0],[0,0],[0,0],[0,0]],\"o\":[[0,0],[0,0],[0,0],[0,0]]}";
		a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + fill(1, 0, 0)), "\"masksProperties\":[{\"mode\":\"a\",\"pt\":" + val(sq) + ",\"o\":" + val("50") + "}],")), 0.0f);
		CHECK(fabsf(cov_at(97, 103) - 0.5f) < 0.02f, "add mask opacity (%.2f)", cov_at(97, 103));
		iam_lottie_free(a);
		std::string const big = "{\"c\":true,\"v\":[[60,60],[140,60],[140,140],[60,140]],\"i\":[[0,0],[0,0],[0,0],[0,0]],\"o\":[[0,0],[0,0],[0,0],[0,0]]}";
		a = draw(doc(layer(group(rect(100, 100, 160, 160) + "," + fill(1, 0, 0)),
			"\"masksProperties\":[{\"mode\":\"a\",\"pt\":" + val(big) + ",\"o\":" + val("100") + "},{\"mode\":\"f\",\"pt\":" + val(sq) + ",\"o\":" + val("100") + "}],")), 0.0f);
		CHECK(cov_at(97, 103) < 0.01f && cov_at(70, 103) > 0.99f && cov_at(30, 103) < 0.01f && strcmp(iam_lottie_warnings(a), "") == 0, "difference mask: XOR with the add mask");
		iam_lottie_free(a);
		std::string const matte = "{\"ty\":4,\"ind\":1,\"td\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{\"o\":" + val("50") + "},\"shapes\":[" + group(rect(100, 100, 40, 40) + "," + fill(1, 1, 1)) + "]}";
		std::string const target = "{\"ty\":4,\"ind\":2,\"tt\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{},\"shapes\":[" + group(rect(100, 100, 100, 100) + "," + fill(0, 0, 1)) + "]}";
		a = draw(doc(matte + "," + target), 0.0f);
		CHECK(fabsf(cov_at(97, 103) - 0.5f) < 0.02f && cov_at(60, 100) < 0.01f, "matte layer opacity (%.2f)", cov_at(97, 103));
		iam_lottie_free(a);
	}

	// ---------------- Effects, offset path ----------------
	{
		std::string const fill_fx = "\"ef\":[{\"ty\":21,\"nm\":\"Fill\",\"mn\":\"ADBE Fill\",\"en\":1,\"ef\":[{\"ty\":10,\"v\":" + val("0") + "},{\"ty\":7,\"v\":" + val("0") +
			"},{\"ty\":2,\"v\":" + val("[0,0,1,1]") + "},{\"ty\":7,\"v\":" + val("0") + "},{\"ty\":0,\"v\":" + val("0") + "},{\"ty\":0,\"v\":" + val("0") + "},{\"ty\":0,\"v\":" + val("1") + "}]}],";
		float rgb[3];
		iam_lottie* a = draw(doc(layer(group(rect(100, 100, 60, 60) + "," + fill(1, 0, 0)), fill_fx)), 0.0f);
		cov_at(97, 103, rgb);
		CHECK(rgb[2] > 0.99f && rgb[0] < 0.01f && strcmp(iam_lottie_warnings(a), "") == 0, "Fill effect recolours the layer");
		iam_lottie_free(a);
		std::string const tint_fx = "\"ef\":[{\"ty\":20,\"nm\":\"Tint\",\"en\":1,\"ef\":[{\"ty\":2,\"v\":" + val("[0,0,0,1]") + "},{\"ty\":2,\"v\":" + val("[0,1,0,1]") + "},{\"ty\":0,\"v\":" + val("100") + "}]},"
			"{\"ty\":5,\"nm\":\"Bounce\",\"mn\":\"ADBE Slider Control\",\"ef\":[{\"ty\":0,\"v\":" + val("3") + "}]}],";
		a = draw(doc(layer(group(rect(100, 100, 60, 60) + "," + fill(1, 1, 1)), tint_fx)), 0.0f);
		cov_at(97, 103, rgb);
		CHECK(rgb[1] > 0.99f && rgb[0] < 0.01f && rgb[2] < 0.01f && strcmp(iam_lottie_warnings(a), "") == 0, "Tint maps white to the white colour; slider controls are silent");
		iam_lottie_free(a);
		std::string const blur = "\"ef\":[{\"ty\":26,\"nm\":\"Radial Wipe\",\"mn\":\"ADBE Radial Wipe\",\"ef\":[]}],";
		std::string const blur_doc = doc(layer(group(rect(100, 100, 60, 60) + "," + fill(1, 1, 1)), blur));
		a = iam_lottie_load_from_memory(blur_doc.data(), blur_doc.size());
		CHECK(a && strstr(iam_lottie_warnings(a), "effects"), "other effects are reported");
		iam_lottie_free(a);
		// Gaussian blur (CPU: soft edge ~2.5 sigma wide, sigma = 0.3 blurriness)
		std::string const blur_fx = "\"ef\":[{\"ty\":29,\"nm\":\"Gaussian Blur\",\"ef\":[{\"ty\":0,\"v\":" + val("20") + "},{\"ty\":7,\"v\":" + val("1") + "},{\"ty\":7,\"v\":" + val("0") + "}]}],";
		a = draw(doc(layer(group(rect(100, 100, 60, 60) + "," + fill(1, 1, 1)), blur_fx)), 0.0f);
		{
			float const e = cov_at(70, 103), in = cov_at(97, 103), out = cov_at(64, 103);
			CHECK(e > 0.35f && e < 0.65f && in > 0.99f && out > 0.01f && out < 0.35f && strcmp(iam_lottie_warnings(a), "") == 0, "blur softens the edge (%.2f %.2f %.2f)", out, e, in);
		}
		iam_lottie_free(a);
		// Drop shadow: direction 135 = down right, distance 20, black; the layer stays on top
		std::string const shadow_fx = "\"ef\":[{\"ty\":25,\"nm\":\"Drop Shadow\",\"ef\":[{\"ty\":2,\"v\":" + val("[0,0,0,1]") + "},{\"ty\":0,\"v\":" + val("255") +
			"},{\"ty\":1,\"v\":" + val("135") + "},{\"ty\":0,\"v\":" + val("20") + "},{\"ty\":0,\"v\":" + val("0") + "},{\"ty\":7,\"v\":" + val("0") + "}]}],";
		a = draw(doc(layer(group(rect(90, 90, 40, 40) + "," + fill(1, 0, 0)), shadow_fx)), 0.0f);
		{
			float s[3], c[3];
			float const cs = cov_at(119, 121, s), cc = cov_at(88, 92, c);
			CHECK(cs > 0.99f && s[0] < 0.01f && cc > 0.99f && c[0] > 0.99f && cov_at(127, 127) < 0.01f, "drop shadow behind the layer");
		}
		iam_lottie_free(a);
		auto radial = [](float h) {
			char b[400];
			snprintf(b, sizeof(b), "{\"ty\":\"gf\",\"o\":{\"a\":0,\"k\":100},\"r\":1,\"t\":2,\"s\":{\"a\":0,\"k\":[100,100]},\"e\":{\"a\":0,\"k\":[150,100]},"
			         "\"h\":{\"a\":0,\"k\":%g},\"a\":{\"a\":0,\"k\":0},\"g\":{\"p\":2,\"k\":{\"a\":0,\"k\":[0,1,1,1,1,0,0,0]}}}", h);
			return std::string(b);
		};
		float l[3], r[3];
		a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + radial(0)))), 0.0f);
		cov_at(76, 103, l); cov_at(124, 103, r);
		CHECK(fabsf(l[0] - r[0]) < 0.05f, "radial gradient without highlight is symmetric (%.2f %.2f)", l[0], r[0]);
		iam_lottie_free(a);
		a = draw(doc(layer(group(rect(100, 100, 100, 100) + "," + radial(50)))), 0.0f);
		cov_at(76, 103, l); cov_at(124, 103, r);
		CHECK(r[0] > l[0] + 0.3f && r[0] > 0.9f && strcmp(iam_lottie_warnings(a), "") == 0, "highlight moves the centre of the radial gradient (%.2f %.2f)", l[0], r[0]);
		iam_lottie_free(a);
		auto offset = [](float amount) { char b[120]; snprintf(b, sizeof(b), "{\"ty\":\"op\",\"a\":{\"a\":0,\"k\":%g},\"lj\":1,\"ml\":4}", amount); return std::string(b); };
		a = draw(doc(layer(group(rect(100, 100, 60, 60) + "," + offset(10) + "," + fill(1, 1, 1)))), 0.0f);
		CHECK(cov_at(65, 103) > 0.99f && cov_at(97, 103) > 0.99f && cov_at(55, 103) < 0.01f && cov_at(63, 63) > 0.99f, "offset path grows the shape (miter corners)");
		iam_lottie_free(a);
		a = draw(doc(layer(group(rect(100, 100, 60, 60) + "," + offset(-10) + "," + fill(1, 1, 1)))), 0.0f);
		CHECK(cov_at(75, 103) < 0.01f && cov_at(85, 103) > 0.99f && cov_at(97, 103) > 0.99f && cov_at(65, 103) < 0.01f, "offset path shrinks the shape");
		iam_lottie_free(a);
	}

	// ---------------- 3D layers and cameras ----------------
	{
		// Comp 200x200: default camera zoom 200 / 0.72; a 3D layer at z = zoom shows at half size
		float const zoom = 200.0f / 0.72f;
		auto layer3d = [](char const* ks, char const* extra) {
			return std::string("{\"ty\":4,\"ind\":1,\"ddd\":1,\"ip\":0,\"op\":60,\"st\":0,") + extra + "\"ks\":{" + ks + "},\"shapes\":[" +
			       group(rect(0, 0, 40, 40) + "," + fill(1, 1, 1)) + "]}";
		};
		iam_lottie* a = draw(doc(layer3d("\"p\":{\"a\":0,\"k\":[100,100,0]}", "")), 0.0f);
		CHECK(cov_at(118, 103) > 0.99f && cov_at(122, 103) < 0.01f && strcmp(iam_lottie_warnings(a), "") == 0, "3D layer at z = 0 matches 2D");
		iam_lottie_free(a);
		char ks[160];
		snprintf(ks, sizeof(ks), "\"p\":{\"a\":0,\"k\":[150,100,%g]}", zoom);
		a = draw(doc(layer3d(ks, "")), 0.0f);
		CHECK(cov_at(125, 103) > 0.99f && cov_at(133, 103) > 0.99f && cov_at(138, 103) < 0.01f && cov_at(150, 103) < 0.01f, "depth: half size, pulled toward the centre");
		iam_lottie_free(a);
		a = draw(doc(layer3d("\"p\":{\"a\":0,\"k\":[100,100,0]},\"ry\":{\"a\":0,\"k\":60}", "")), 0.0f);
		CHECK(cov_at(108, 103) > 0.99f && cov_at(115, 103) < 0.01f && cov_at(100, 117) > 0.99f, "Y rotation foreshortens the width");
		iam_lottie_free(a);
		a = draw(doc(layer3d("\"p\":{\"a\":0,\"k\":[100,100,0]},\"ry\":{\"a\":0,\"k\":90}", "")), 0.0f);
		CHECK(cov_at(97, 103) < 0.01f && cov_at(103, 103) < 0.01f, "edge-on layer disappears");
		iam_lottie_free(a);
		// Camera moved left: the scene moves right
		char cam[300];
		snprintf(cam, sizeof(cam), "{\"ty\":13,\"ind\":9,\"ip\":0,\"op\":60,\"st\":0,\"pe\":{\"a\":0,\"k\":%g},\"ks\":{\"p\":{\"a\":0,\"k\":[50,100,%g]}}},", zoom, -zoom);
		a = draw(doc(cam + layer3d("\"p\":{\"a\":0,\"k\":[100,100,0]}", "")), 0.0f);
		CHECK(cov_at(147, 103) > 0.99f && cov_at(97, 103) < 0.01f && strcmp(iam_lottie_warnings(a), "") == 0, "camera layer position");
		iam_lottie_free(a);
		// Cameras switch with their in / out points: the top one until frame 30, then the next
		{
			char two[700];
			snprintf(two, sizeof(two),
				"{\"ty\":13,\"ind\":9,\"ip\":0,\"op\":30,\"st\":0,\"pe\":{\"a\":0,\"k\":%g},\"ks\":{\"p\":{\"a\":0,\"k\":[50,100,%g]}}},"
				"{\"ty\":13,\"ind\":8,\"ip\":30,\"op\":60,\"st\":0,\"pe\":{\"a\":0,\"k\":%g},\"ks\":{\"p\":{\"a\":0,\"k\":[150,100,%g]}}},", zoom, -zoom, zoom, -zoom);
			a = draw(doc(two + layer3d("\"p\":{\"a\":0,\"k\":[100,100,0]}", "")), 0.5f);
			CHECK(cov_at(147, 103) > 0.99f && cov_at(53, 103) < 0.01f, "first camera before its out point");
			iam_lottie_free(a);
			a = draw(doc(two + layer3d("\"p\":{\"a\":0,\"k\":[100,100,0]}", "")), 1.5f);
			CHECK(cov_at(53, 103) > 0.99f && cov_at(147, 103) < 0.01f, "second camera after the switch");
			iam_lottie_free(a);
		}
		// Two-node camera aimed at the layer from the right: the layer stays centred, seen at an angle
		snprintf(cam, sizeof(cam), "{\"ty\":13,\"ind\":9,\"ip\":0,\"op\":60,\"st\":0,\"pe\":{\"a\":0,\"k\":%g},\"ks\":{\"p\":{\"a\":0,\"k\":[300,100,-200]},\"a\":{\"a\":0,\"k\":[100,100,0]}}},", zoom);
		a = draw(doc(cam + layer3d("\"p\":{\"a\":0,\"k\":[100,100,0]}", "")), 0.0f);
		CHECK(cov_at(100, 103) > 0.99f && cov_at(97, 103) > 0.99f && cov_at(103, 103) > 0.99f, "point of interest keeps the target centred");
		iam_lottie_free(a);
		// 3D parent: a 3D null shifts its 3D child
		std::string const null3d = "{\"ty\":3,\"ind\":5,\"ddd\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{\"p\":{\"a\":0,\"k\":[50,0,0]}}},";
		a = draw(doc(null3d + layer3d("\"p\":{\"a\":0,\"k\":[100,100,0]}", "\"parent\":5,")), 0.0f);
		CHECK(cov_at(147, 103) > 0.99f && cov_at(97, 103) < 0.01f, "3D parenting");
		iam_lottie_free(a);
		// Depth sort: the top layer (red) is farther than the bottom one (green), so green draws over it;
		// at equal depth the layer order holds
		auto colored3d = [](int ind, float z, char const* rgb) {
			char b[200];
			snprintf(b, sizeof(b), "{\"ty\":4,\"ind\":%d,\"ddd\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{\"p\":{\"a\":0,\"k\":[100,100,%g]}},\"shapes\":[", ind, z);
			return std::string(b) + group(rect(0, 0, 40, 40) + ",{\"ty\":\"fl\",\"c\":{\"a\":0,\"k\":[" + rgb + ",1]},\"o\":{\"a\":0,\"k\":100},\"r\":1}") + "]}";
		};
		float rgb[3];
		a = draw(doc(colored3d(1, 200.0f, "1,0,0") + "," + colored3d(2, 0.0f, "0,1,0")), 0.0f);
		cov_at(100, 100, rgb);
		CHECK(rgb[1] > 0.99f && rgb[0] < 0.01f, "3D depth sort: the nearer layer covers the farther one (rgb %.2f %.2f)", rgb[0], rgb[1]);
		iam_lottie_free(a);
		a = draw(doc(colored3d(1, 0.0f, "1,0,0") + "," + colored3d(2, 0.0f, "0,1,0")), 0.0f);
		cov_at(100, 100, rgb);
		CHECK(rgb[0] > 0.99f && rgb[1] < 0.01f, "3D depth sort: equal depth keeps the layer order");
		iam_lottie_free(a);
	}

	// ---------------- Text layers with embedded glyphs ----------------
	{
		// Glyphs at size 100: 'A' a 60x70 box, 'B' a triangle, U+00E9 a small box; advance 70
		auto glyph = [](char const* ch, char const* verts, int n) {
			std::string zeros;
			for (int i = 0; i < n; i++) zeros += i ? ",[0,0]" : "[0,0]";
			return std::string("{\"ch\":\"") + ch + "\",\"fFamily\":\"Test\",\"style\":\"Regular\",\"size\":100,\"w\":70,\"data\":{\"shapes\":[{\"ty\":\"gr\",\"it\":[{\"ty\":\"sh\",\"ks\":{\"a\":0,\"k\":{\"c\":true,\"v\":[" +
			       verts + "],\"i\":[" + zeros + "],\"o\":[" + zeros + "]}}},{\"ty\":\"tr\"}]}]}}";
		};
		std::string const fonts = "\"fonts\":{\"list\":[{\"fName\":\"TestSans\",\"fFamily\":\"Test\",\"fStyle\":\"Regular\",\"ascent\":70}]},\"chars\":[" +
			glyph("A", "[0,0],[60,0],[60,-70],[0,-70]", 4) + "," + glyph("B", "[0,0],[60,0],[30,-70]", 3) + "," + glyph("\\u00e9", "[0,0],[20,0],[20,-20],[0,-20]", 4) +
			",{\"ch\":\" \",\"fFamily\":\"Test\",\"style\":\"Regular\",\"size\":100,\"w\":30,\"data\":{}}],";
		auto text = [](char const* t, int j, float tr, float x) {
			char b[400];
			snprintf(b, sizeof(b), "{\"ty\":5,\"ind\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{\"p\":{\"a\":0,\"k\":[%g,100]}},\"t\":{\"d\":{\"k\":[{\"t\":0,\"s\":{\"s\":50,\"f\":\"TestSans\",\"t\":\"%s\",\"j\":%d,\"tr\":%g,\"lh\":60,\"fc\":[1,0,0]}}]},\"a\":[]}}", x, t, j, tr);
			return std::string(b);
		};
		iam_lottie* a = draw(doc(text("AB\\rA", 0, 0, 20), fonts), 0.0f);
		float rgb[3];
		CHECK(cov_at(35, 80, rgb) > 0.99f && rgb[0] > 0.99f && rgb[1] < 0.01f, "text: first glyph in the fill colour");
		CHECK(cov_at(52, 80) < 0.01f && cov_at(70, 97) > 0.99f && cov_at(57, 70) < 0.01f, "text: second glyph after the advance");
		CHECK(cov_at(35, 140) > 0.99f && cov_at(35, 110) < 0.01f, "text: \\r starts a line one line height down");
		CHECK(strcmp(iam_lottie_warnings(a), "") == 0, "text with glyphs: no warning (%s)", iam_lottie_warnings(a));
		iam_lottie_free(a);
		a = draw(doc(text("AB", 2, 0, 100), fonts), 0.0f);
		CHECK(cov_at(80, 80) > 0.99f && cov_at(62, 80) < 0.01f && cov_at(125, 97) > 0.99f, "text: centred");
		iam_lottie_free(a);
		a = draw(doc(text("AB", 1, 0, 100), fonts), 0.0f);
		CHECK(cov_at(45, 80) > 0.99f && cov_at(90, 97) > 0.99f && cov_at(105, 97) < 0.01f, "text: right-aligned");
		iam_lottie_free(a);
		a = draw(doc(text("AB", 0, 200, 20), fonts), 0.0f);
		CHECK(cov_at(60, 97) < 0.01f && cov_at(80, 97) > 0.99f, "text: tracking");
		iam_lottie_free(a);
		a = draw(doc(text("\\u00e9", 0, 0, 20), fonts), 0.0f);
		CHECK(cov_at(25, 97) > 0.99f && cov_at(35, 97) < 0.01f, "text: \\u escapes map to glyphs");
		iam_lottie_free(a);
		// Paragraph text, size 50 (A: 30 wide, advance 35; space 15) in a 90 wide box at (20, 20):
		// "A A A" wraps after "A A"; first baseline at the box top + ascent 70% of 50
		auto boxed = [](int j) {
			char b[400];
			snprintf(b, sizeof(b), "{\"ty\":5,\"ind\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{\"p\":{\"a\":0,\"k\":[20,20]}},\"t\":{\"d\":{\"k\":[{\"t\":0,\"s\":{\"s\":50,\"f\":\"TestSans\",\"t\":\"A A A\",\"j\":%d,\"lh\":60,\"sz\":[90,200],\"ps\":[0,0],\"fc\":[1,0,0]}}]},\"a\":[]}}", j);
			return std::string(b);
		};
		a = draw(doc(boxed(0), fonts), 0.0f);
		CHECK(cov_at(35, 40) > 0.99f && cov_at(85, 40) > 0.99f && cov_at(135, 40) < 0.01f && cov_at(35, 17) < 0.01f, "text box: first line \"A A\" below the box top");
		CHECK(cov_at(35, 100) > 0.99f && cov_at(85, 100) < 0.01f, "text box: the last word wraps to the next line");
		iam_lottie_free(a);
		a = draw(doc(boxed(1), fonts), 0.0f);
		CHECK(cov_at(90, 100) > 0.99f && cov_at(35, 100) < 0.01f, "text box: right-aligned to the box edge");
		iam_lottie_free(a);
	}

	// ---------------- Replay cache ----------------
	{
		std::string const json = doc(layer(group(rect(100, 100, 100, 100) + "," + rect(100, 100, 40, 40) + "," + fill(1, 0, 0, 2)) + "," + group(line_path(20, 20, 180, 30) + "," + stroke(6, 2))));
		iam_lottie* a = iam_lottie_load_from_memory(json.data(), json.size());
		ImVector<ImDrawVert> first;
		ImVector<ImDrawIdx> first_idx;
		bool same = true;
		for (int pass = 0; pass < 3; pass++) {
			int const v0 = g_dl->VtxBuffer.Size, i0 = g_dl->IdxBuffer.Size;
			unsigned const base = g_dl->_VtxCurrentIdx;
			iam_lottie_draw(g_dl, a, 0.25f, iam_v2(10, 10), iam_v2(150, 150));
			int const nv = g_dl->VtxBuffer.Size - v0, ni = g_dl->IdxBuffer.Size - i0;
			if (pass == 0) {
				first.resize(nv); memcpy(first.Data, g_dl->VtxBuffer.Data + v0, sizeof(ImDrawVert) * nv);
				first_idx.resize(ni); for (int j = 0; j < ni; j++) first_idx[j] = (ImDrawIdx)(g_dl->IdxBuffer[i0 + j] - base);
			} else {
				same = same && nv == first.Size && ni == first_idx.Size && memcmp(first.Data, g_dl->VtxBuffer.Data + v0, sizeof(ImDrawVert) * nv) == 0;
				for (int j = 0; same && j < ni; j++) same = (ImDrawIdx)(g_dl->IdxBuffer[i0 + j] - base) == first_idx[j];
			}
		}
		CHECK(same && first.Size > 0, "replayed draws match the evaluated one (%d vertices)", first.Size);
		g_idx0 = g_dl->IdxBuffer.Size;
		iam_lottie_draw(g_dl, a, 0.25f, iam_v2(10, 10), iam_v2(150, 150));
		CHECK(cov_at(10 + 150 * 0.5f, 10 + 150 * 0.3f) > 0.99f, "replayed draw covers the shape");
		{
			// A draw recorded across ImDrawList's 16-bit vertex offset wrap replays the same triangles
			auto abs_tris = [](ImDrawList* dl, int idx0, int vtx0, ImVector<int>& out) {
				out.resize(0);
				for (int c = 0; c < dl->CmdBuffer.Size; c++) {
					ImDrawCmd const& cmd = dl->CmdBuffer[c];
					int const end = c + 1 < dl->CmdBuffer.Size ? (int)dl->CmdBuffer[c + 1].IdxOffset : dl->IdxBuffer.Size;
					for (int j = ImMax((int)cmd.IdxOffset, idx0); j < end; j++) out.push_back((int)cmd.VtxOffset + (int)dl->IdxBuffer[j] - vtx0);
				}
			};
			iam_lottie_set_cache(a, true);
			ImVector<int> rec, rep;
			ImVector<ImDrawVert> rec_v;
			for (int pass = 0; pass < 3; pass++) {
				if (pass == 1) {
					// Push the vertex counter close to 65535 so this draw wraps
					int const k = 65536 - (int)g_dl->_VtxCurrentIdx - first.Size / 2;
					if (k > 0) { g_dl->PrimReserve(0, k); memset(g_dl->_VtxWritePtr, 0, sizeof(ImDrawVert) * k); g_dl->_VtxWritePtr += k; g_dl->_VtxCurrentIdx += k; }
				}
				unsigned const ofs_before = g_dl->_CmdHeader.VtxOffset;
				int const v0b = g_dl->VtxBuffer.Size, i0b = g_dl->IdxBuffer.Size;
				iam_lottie_draw(g_dl, a, 0.5f, iam_v2(10, 10), iam_v2(150, 150));
				if (pass == 1) { abs_tris(g_dl, i0b, v0b, rec); rec_v.resize(g_dl->VtxBuffer.Size - v0b); memcpy(rec_v.Data, g_dl->VtxBuffer.Data + v0b, sizeof(ImDrawVert) * rec_v.Size);
				                 CHECK(g_dl->_CmdHeader.VtxOffset != ofs_before, "test setup: the recorded draw wrapped the vertex offset"); }
				if (pass == 2) abs_tris(g_dl, i0b, v0b, rep);
			}
			bool ok = rec.Size == rep.Size && rec.Size > 0;
			for (int j = 0; ok && j < rec.Size; j++) ok = memcmp(&rec_v[rec[j]], &g_dl->VtxBuffer[g_dl->VtxBuffer.Size - rec_v.Size + rep[j]], sizeof(ImDrawVert)) == 0;
			CHECK(ok, "replay after a vertex offset wrap draws the same triangles (%d / %d indices)", rep.Size, rec.Size);
		}
		iam_lottie_set_cache(a, false);
		int const v0 = g_dl->VtxBuffer.Size;
		iam_lottie_draw(g_dl, a, 0.25f, iam_v2(10, 10), iam_v2(150, 150));
		CHECK(g_dl->VtxBuffer.Size - v0 == first.Size, "cache off: same output, evaluated");
		iam_lottie_free(a);
	}

	// ---------------- Frame snap ----------------
	{
		// Square moving from x = 50 to 150 over frames 0..30 (30 fps)
		std::string const m = doc("{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":60,\"st\":0,\"ks\":{\"p\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[50,100]},{\"t\":30,\"s\":[150,100]}]}},"
		                          "\"shapes\":[" + group(rect(0, 0, 20, 20) + "," + fill(1, 1, 1)) + "]}");
		iam_lottie* a = iam_lottie_load_from_memory(m.data(), m.size());
		iam_lottie_set_cache(a, false);
		auto draw_at = [&](float t, ImVector<ImDrawVert>& out) {
			int const v0 = g_dl->VtxBuffer.Size;
			iam_lottie_draw(g_dl, a, t, iam_v2(0, 0), iam_v2(200, 200));
			out.resize(g_dl->VtxBuffer.Size - v0);
			memcpy(out.Data, g_dl->VtxBuffer.Data + v0, sizeof(ImDrawVert) * out.Size);
		};
		auto same = [](ImVector<ImDrawVert> const& x, ImVector<ImDrawVert> const& y) { return x.Size == y.Size && x.Size > 0 && memcmp(x.Data, y.Data, sizeof(ImDrawVert) * x.Size) == 0; };
		ImVector<ImDrawVert> f10, f10b, f11;
		draw_at(10.2f / 30.0f, f10); draw_at(10.8f / 30.0f, f10b);
		CHECK(f10.Size > 0 && !same(f10, f10b), "no snap: in-between frames interpolate");
		iam_lottie_set_frame_snap(a, true);
		draw_at(10.0f / 30.0f, f10); draw_at(10.9f / 30.0f, f10b); draw_at(11.0f / 30.0f, f11);
		CHECK(same(f10, f10b) && !same(f10, f11), "snap: one image per file frame, the next frame differs");
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
		CHECK(a && strstr(iam_lottie_warnings(a), "text without embedded glyphs") && strstr(iam_lottie_warnings(a), "expressions"), "warnings: %s", a ? iam_lottie_warnings(a) : "");
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
