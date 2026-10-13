// Reduced motion, velocity tracking, SVG path import and text scramble.
#include "im_anim.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { g_fail++; printf("FAIL %d: ", __LINE__); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)

static void begin_frame() { ImGui::NewFrame(); iam_update_begin_frame(); }
static void end_frame() { ImGui::Render(); }
static bool near(float a, float b, float eps) { return fabsf(a - b) <= eps; }

int main() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(800, 600); io.DeltaTime = 1.0f / 60.0f; io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	float const dt = io.DeltaTime;
	iam_ease_desc lin = iam_ease_preset(iam_ease_linear);

	// ---------------- Reduced motion ----------------
	iam_set_reduced_motion(true, 0.0f);
	CHECK(iam_get_reduced_motion(), "reduced motion on");
	begin_frame();
	iam_tween_float(1, 1, 0.0f, 1.0f, lin, iam_policy_crossfade, dt);
	end_frame();
	begin_frame();
	float tw = iam_tween_float(1, 1, 50.0f, 1.0f, lin, iam_policy_crossfade, dt);
	float sp = iam_spring_float(1, 2, 80.0f, iam_spring_default(), dt);
	float pt = -1; iam_presence(1, true, &pt, dt);
	float osc = iam_oscillate(1, 10.0f, 1.0f, iam_wave_sine, 0.25f, dt);
	end_frame();
	CHECK(tw == 50.0f, "tween completes instantly (%.2f)", tw);
	CHECK(sp == 80.0f, "spring jumps to target (%.2f)", sp);
	CHECK(pt == 1.0f, "presence shows instantly (%.2f)", pt);
	CHECK(osc == 0.0f, "oscillation stilled with procedural_scale 0 (%.3f)", osc);
	iam_set_reduced_motion(false);

	// procedural_scale 0.5 applies exactly once (also through nested color -> vec4 calls)
	float o1 = 0, o2 = 0; ImVec4 c1, c2;
	ImVec4 const base(0.5f, 0.5f, 0.5f, 1.0f), amp(0.2f, 0.2f, 0.2f, 0.0f);
	for (int f = 0; f < 10; f++) {
		begin_frame();
		o1 = iam_oscillate(10, 10.0f, 1.0f, iam_wave_sine, 0.1f, dt);
		c1 = iam_oscillate_color(11, base, amp, 1.0f, iam_wave_sine, 0.1f, iam_col_srgb, dt);
		iam_set_reduced_motion(true, 0.5f);
		o2 = iam_oscillate(20, 10.0f, 1.0f, iam_wave_sine, 0.1f, dt);
		c2 = iam_oscillate_color(21, base, amp, 1.0f, iam_wave_sine, 0.1f, iam_col_srgb, dt);
		iam_set_reduced_motion(false);
		end_frame();
	}
	CHECK(fabsf(o1) > 1.0f && near(o2, o1 * 0.5f, 1e-3f), "procedural_scale 0.5 halves oscillation (%.3f vs %.3f)", o2, o1);
	CHECK(fabsf(c1.x - base.x) > 0.02f && near(c2.x - base.x, (c1.x - base.x) * 0.5f, 1e-3f), "nested color oscillation scaled once (%.4f vs %.4f)", c2.x - base.x, c1.x - base.x);

	// ---------------- Velocity tracking ----------------
	float v = 0, val = 0;
	for (int f = 0; f < 60; f++) { begin_frame(); val += 2.0f; v = iam_track_velocity_float(1, val, dt); end_frame(); }
	CHECK(near(v, 120.0f, 1.0f), "tracks 2 units/frame = 120/s (%.2f)", v);
	ImVec2 v2;
	for (int f = 0; f < 60; f++) { begin_frame(); v2 = IAM_VEC2_TO_IM(iam_track_velocity_vec2(2, iam_v2(f * 1.0f, -f * 3.0f), dt)); end_frame(); }
	CHECK(near(v2.x, 60.0f, 1.0f) && near(v2.y, -180.0f, 2.0f), "vec2 velocity (%.1f, %.1f)", v2.x, v2.y);

	// ---------------- SVG ----------------
	CHECK(iam_path_from_svg(1, "M0 0 L100 0") && near(iam_path_length(1), 100.0f, 0.5f), "M/L line length 100 (%.2f)", iam_path_length(1));
	CHECK(iam_path_from_svg(2, "M0,0 H100 V100 Z") && near(iam_path_length(2), 341.42f, 1.0f), "H/V/Z closed triangle (%.2f)", iam_path_length(2));
	CHECK(iam_path_from_svg(3, "m10 10 l10 0 0 10 h-10z") && near(iam_path_length(3), 40.0f, 0.5f), "relative + implicit lineto square (%.2f)", iam_path_length(3));
	CHECK(iam_path_from_svg(4, "M0 0 A50 50 0 0 1 100 0") && near(iam_path_length(4), 157.08f, 1.5f), "arc half circle length (%.2f)", iam_path_length(4));
	ImVec2 mid = IAM_VEC2_TO_IM(iam_path_evaluate_at_distance(4, iam_path_length(4) * 0.5f));
	CHECK(near(mid.x, 50.0f, 1.0f) && near(mid.y, -50.0f, 1.0f), "arc passes through (50,-50) (%.1f, %.1f)", mid.x, mid.y);
	CHECK(iam_path_from_svg(5, "M0 0 C0 50 100 50 100 0 S200 -50 200 0") && near(IAM_VEC2_TO_IM(iam_path_evaluate(5, 1.0f)).x, 200.0f, 0.01f), "cubic + smooth cubic end point");
	CHECK(iam_path_from_svg(6, "M0 0 Q50 100 100 0 T200 0") && near(IAM_VEC2_TO_IM(iam_path_evaluate(6, 1.0f)).x, 200.0f, 0.01f), "quadratic + smooth quadratic end point");
	CHECK(iam_path_from_svg(7, "M0 0 L10 0", 2.0f, iam_v2(5, 5)) && near(IAM_VEC2_TO_IM(iam_path_evaluate(7, 1.0f)).x, 25.0f, 0.01f) && near(IAM_VEC2_TO_IM(iam_path_evaluate(7, 1.0f)).y, 5.0f, 0.01f), "scale and offset");
	CHECK(iam_path_from_svg(8, "M1e1 0L-1.5-2.5.5.5"), "number formats: exponent, packed signs and dots");
	CHECK(!iam_path_from_svg(9, "L10 10") && !iam_path_exists(9), "rejects data without M (builds nothing)");
	CHECK(!iam_path_from_svg(9, "M0 0 L10") && !iam_path_exists(9), "rejects missing coordinate");
	CHECK(!iam_path_from_svg(9, "") && !iam_path_from_svg(9, "M0 0 X5 5"), "rejects empty / unknown command");

	// ---------------- Text scramble ----------------
	char buf[128];
	char const* text = "Hello World";
	iam_text_scramble(buf, sizeof(buf), text, 1.0f);
	CHECK(strcmp(buf, text) == 0, "progress 1 = final text");
	int n = iam_text_scramble(buf, sizeof(buf), text, 0.0f);
	CHECK(n == (int)strlen(text) && buf[5] == ' ' && strcmp(buf, text) != 0, "progress 0: same length, space kept, scrambled (%s)", buf);
	iam_text_scramble(buf, sizeof(buf), text, 0.5f);
	CHECK(strncmp(buf, "Hello", 5) == 0, "progress 0.5 reveals left half (%s)", buf);
	iam_text_scramble(buf, sizeof(buf), "h\xC3\xA9llo", 1.0f);
	CHECK(strcmp(buf, "h\xC3\xA9llo") == 0, "UTF-8 kept at progress 1");
	iam_text_scramble(buf, 4, text, 1.0f);
	CHECK(strlen(buf) == 3, "respects out_size");
	iam_text_scramble_opts ro; ro.random_order = true; ro.seed = 7;
	int revealed = 0;
	iam_text_scramble(buf, sizeof(buf), "abcdefghijklmnopqrstuvwxyz", 0.5f, ro);
	for (int i = 0; i < 26; i++) revealed += buf[i] == 'a' + i;
	CHECK(revealed >= 7 && revealed <= 20, "random order reveals about half (%d/26)", revealed);

	// ---------------- Text outlines as paths ----------------
	ImGui::NewFrame(); iam_update_begin_frame(); ImGui::Begin("tp");
	ImGuiID const TXT = ImHashStr("txt");
	int nc = iam_text_to_paths(TXT, "Hi", 40.0f, iam_v2(100, 50));
	float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
	bool all_exist = nc > 0;
	for (int i = 0; i < nc; i++) {
		ImGuiID pid = iam_text_path_id(TXT, i);
		all_exist &= iam_path_exists(pid);
		for (int k = 0; k <= 10; k++) { ImVec2 q = IAM_VEC2_TO_IM(iam_path_evaluate(pid, k / 10.0f)); minx = ImMin(minx, q.x); maxx = ImMax(maxx, q.x); miny = ImMin(miny, q.y); maxy = ImMax(maxy, q.y); }
	}
	CHECK(nc >= 2 && all_exist, "\"Hi\" gives contours as paths (%d)", nc);
	CHECK(minx >= 100.0f && maxx < 160.0f && miny >= 50.0f && maxy <= 95.0f, "contours inside the text box (x %.1f..%.1f, y %.1f..%.1f)", minx, maxx, miny, maxy);
	int no = iam_text_to_paths(ImHashStr("o"), "O", 40.0f);
	CHECK(no >= 2, "\"O\" has an outer and an inner contour (%d)", no);
	int n2 = iam_text_to_paths(ImHashStr("two"), "I\nI", 40.0f);
	ImVec2 a0 = IAM_VEC2_TO_IM(iam_path_evaluate(iam_text_path_id(ImHashStr("two"), 0), 0.0f));
	ImVec2 a1 = IAM_VEC2_TO_IM(iam_path_evaluate(iam_text_path_id(ImHashStr("two"), n2 - 1), 0.0f));
	CHECK(n2 >= 2 && a1.y > a0.y + 30.0f, "'\n' starts a new line (%.1f -> %.1f)", a0.y, a1.y);
	ImDrawList* tdl = ImGui::GetWindowDrawList();
	int tv0 = tdl->VtxBuffer.Size;
	iam_text_paths_draw(tdl, TXT, nc, 0.5f, IM_COL32_WHITE, 2.0f);
	CHECK(tdl->VtxBuffer.Size > tv0, "iam_text_paths_draw draws");
	CHECK(iam_text_to_paths(ImHashStr("none"), "", 40.0f) == 0 && iam_text_to_paths(ImHashStr("none"), "x", 0.0f) == 0, "empty text / zero size: no paths");
	ImGui::End(); ImGui::Render();

	ImGui::DestroyContext();
	printf("%s (%d failures)\n", g_fail ? "FAILED" : "ALL PASSED", g_fail);
	return g_fail ? 1 : 0;
}
