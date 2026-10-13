// Curve eases (iam_ease_curve_*), parametric eases in clip keys, curves in clip files, and the ease editor widget.
#include "im_anim.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <math.h>

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { g_fail++; printf("FAIL %d: ", __LINE__); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static bool near(float a, float b, float eps) { return fabsf(a - b) <= eps; }

static iam_ease_desc g_desc;
static ImVec2 g_group_min, g_group_max;
static float const GRAPH_H = 200.0f;
static bool g_changed = false;

static void frame() {
	ImGui::NewFrame();
	iam_update_begin_frame();
	iam_clip_update(ImGui::GetIO().DeltaTime);
	ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(ImVec2(500, 500));
	ImGui::Begin("w");
	g_changed |= iam_ease_editor("Ease", &g_desc, iam_v2(400, GRAPH_H));
	g_group_min = ImGui::GetItemRectMin(); g_group_max = ImGui::GetItemRectMax();
	ImGui::End();
	ImGui::Render();
}
static void frames(int n) { for (int i = 0; i < n; i++) frame(); }
static void mouse(ImVec2 p) { ImGui::GetIO().AddMousePosEvent(p.x, p.y); frame(); }
static void button(int b, bool down) { ImGui::GetIO().AddMouseButtonEvent(b, down); frame(); }
// Graph mapping (mirrors iam_ease_editor for a [0,1] value range)
static ImVec2 graph_pt(float x, float y) {
	float fs = ImGui::GetFontSize(), pad = fs * 0.8f;
	ImVec2 gp(g_group_min.x, g_group_max.y - GRAPH_H);
	ImVec2 g0(gp.x + pad, gp.y + pad), g1(gp.x + 400 - pad * 3.0f, gp.y + GRAPH_H - pad);
	return ImVec2(g0.x + x * (g1.x - g0.x), g1.y - y * (g1.y - g0.y));
}

int main() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(800, 600); io.DeltaTime = 1.0f / 60.0f; io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	// --- Curve evaluation
	iam_vec2 const line[] = { iam_v2(0, 0), iam_v2(1.0f / 3, 1.0f / 3), iam_v2(2.0f / 3, 2.0f / 3), iam_v2(1, 1) };
	iam_ease_curve_set(1, line, 4);
	iam_ease_desc lin_curve = iam_ease_curve_desc(1);
	float max_err = 0;
	for (int i = 0; i <= 20; i++) { float t = i / 20.0f; max_err = ImMax(max_err, fabsf(iam_eval_preset(iam_ease_linear, t) - t)); }
	CHECK(iam_ease_curve_exists(1) && iam_ease_curve_get(1, nullptr, 0) == 4, "curve defined (4 points)");
	// iam_eval_preset doesn't take descs: evaluate through a tween instead
	ImGui::NewFrame(); iam_update_begin_frame(); iam_tween_float(1, 1, 0.0f, 1.0f, lin_curve, iam_policy_crossfade, 0); ImGui::Render();
	float tw = 0;
	for (int f = 0; f < 30; f++) { ImGui::NewFrame(); iam_update_begin_frame(); tw = iam_tween_float(1, 1, 1.0f, 1.0f, lin_curve, iam_policy_crossfade, 0); ImGui::Render(); }
	CHECK(near(tw, 29.0f / 60.0f, 0.02f), "straight curve behaves like linear in a tween (%.3f)", tw);

	iam_vec2 const bump[] = { iam_v2(0, 0), iam_v2(0.2f, 0.0f), iam_v2(0.3f, 1.3f), iam_v2(0.5f, 1.2f), iam_v2(0.7f, 1.1f), iam_v2(0.8f, 1.0f), iam_v2(1, 1) };
	ImGuiID const BUMP = ImHashStr("bump");
	iam_ease_curve_set(BUMP, bump, 7);
	iam_ease_curve_set(BUMP, bump, 5);  // invalid count: ignored
	CHECK(iam_ease_curve_get(BUMP, nullptr, 0) == 7, "invalid point count ignored");

	// --- Clip keys: curve ease and parametric (steps) ease
	static float const steps4[4] = { 4.0f, 0.0f, 0.0f, 0.0f };
	ImGuiID const CLIP = ImHashStr("clip");
	iam_clip::begin(CLIP)
		.key_float(1, 0.0f, 0.0f, iam_ease_curve, iam_ease_curve_ref(BUMP)).key_float(1, 1.0f, 1.0f)
		.key_float(2, 0.0f, 0.0f, iam_ease_steps, steps4).key_float(2, 1.0f, 1.0f)
		.key_float(3, 0.0f, 0.0f, iam_ease_curve, iam_ease_curve_ref(ImHashStr("undefined"))).key_float(3, 1.0f, 1.0f)
		.end();
	iam_instance inst = iam_play(CLIP, ImHashStr("i"));
	inst.pause();
	inst.seek(0.5f); inst.refresh();
	float c = 0, st = 0, u = 0;
	inst.get_float(1, &c); inst.get_float(2, &st); inst.get_float(3, &u);
	CHECK(near(c, 1.2f, 0.01f), "curve ease in a clip key overshoots like the curve (%.3f)", c);
	inst.seek(0.3f); inst.refresh(); inst.get_float(2, &st);
	CHECK(near(st, 0.25f, 1e-4f), "steps params work in clip keys (%.3f)", st);
	CHECK(near(u, 0.5f, 1e-3f), "undefined curve falls back to linear (%.3f)", u);

	// --- Clip file v6 carries the curve
	CHECK(iam_clip_save(CLIP, "ease_test.iamclip") == iam_ok, "save");
	iam_ease_curve_set(BUMP, line, 4);  // change the curve, then load restores it
	ImGuiID loaded = 0;
	CHECK(iam_clip_load("ease_test.iamclip", &loaded) == iam_ok, "load");
	CHECK(iam_ease_curve_get(BUMP, nullptr, 0) == 7, "load restores the curve from the file");
	remove("ease_test.iamclip");

	// --- Ease editor: drag a cubic-bezier handle
	g_desc = iam_ease_bezier(0.25f, 0.1f, 0.25f, 1.0f);
	frames(2);
	ImVec2 h1 = graph_pt(0.25f, 0.1f);
	mouse(h1); button(0, true);
	for (int i = 1; i <= 5; i++) mouse(ImVec2(h1.x + (graph_pt(0.6f, 0.1f).x - h1.x) * i / 5.0f, h1.y));
	button(0, false);
	CHECK(g_changed && near(g_desc.p0, 0.6f, 0.02f) && near(g_desc.p1, 0.1f, 0.02f), "drag moves the first bezier handle (%.3f, %.3f)", g_desc.p0, g_desc.p1);

	// --- Ease editor: curve anchors (double-click adds, right-click removes)
	iam_vec2 const s_curve[] = { iam_v2(0, 0), iam_v2(0.4f, 0.0f), iam_v2(0.6f, 1.0f), iam_v2(1, 1) };
	ImGuiID const EDIT = ImHashStr("edit_curve");
	iam_ease_curve_set(EDIT, s_curve, 4);
	g_desc = iam_ease_curve_desc(EDIT);
	frames(30);  // let double-click timing expire
	ImVec2 empty = graph_pt(0.5f, 0.1f);  // away from the curve's points
	mouse(empty); button(0, true); button(0, false); button(0, true); button(0, false);
	CHECK(iam_ease_curve_get(EDIT, nullptr, 0) == 7, "double-click adds an anchor (%d points)", iam_ease_curve_get(EDIT, nullptr, 0));
	iam_vec2 pts[16];
	iam_ease_curve_get(EDIT, pts, 16);
	ImVec2 mid = IAM_VEC2_TO_IM(pts[3]);
	CHECK(near(mid.x, 0.5f, 0.01f) && near(mid.y, 0.5f, 0.02f), "new anchor lies on the curve (%.3f, %.3f)", mid.x, mid.y);
	frames(30);
	mouse(graph_pt(mid.x, mid.y)); button(1, true); button(1, false);
	CHECK(iam_ease_curve_get(EDIT, nullptr, 0) == 4, "right-click removes it (%d points)", iam_ease_curve_get(EDIT, nullptr, 0));

	ImGui::DestroyContext();
	printf("%s (%d failures)\n", g_fail ? "FAILED" : "ALL PASSED", g_fail);
	return g_fail ? 1 : 0;
}
