// Presence (iam_presence) and path drawing (iam_path_draw).
#include "im_anim.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <math.h>

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { g_fail++; printf("FAIL %d: ", __LINE__); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)

static void begin_frame() { ImGui::NewFrame(); iam_update_begin_frame(); ImGui::Begin("w"); }
static void end_frame() { ImGui::End(); ImGui::Render(); }

// x/y extent of the vertices added to dl since vtx_start
static void bounds(ImDrawList* dl, int vtx_start, ImVec2* mn, ImVec2* mx) {
	*mn = ImVec2(FLT_MAX, FLT_MAX); *mx = ImVec2(-FLT_MAX, -FLT_MAX);
	for (int i = vtx_start; i < dl->VtxBuffer.Size; i++) {
		ImVec2 p = dl->VtxBuffer[i].pos;
		mn->x = ImMin(mn->x, p.x); mn->y = ImMin(mn->y, p.y); mx->x = ImMax(mx->x, p.x); mx->y = ImMax(mx->y, p.y);
	}
}

int main() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(800, 600); io.DeltaTime = 1.0f / 60.0f; io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	float const dt = io.DeltaTime;
	iam_presence_opts opts;  // enter 0.2s, exit 0.15s

	// --- Enter: 0 -> 1 in ~12 frames
	float t = -1;
	int frames_to_full = -1;
	bool drawn = false;
	for (int f = 0; f < 30; f++) {
		begin_frame();
		drawn = iam_presence(1, true, &t, dt, opts);
		if (frames_to_full < 0 && t >= 1.0f) frames_to_full = f;
		end_frame();
	}
	CHECK(drawn && t == 1.0f, "visible: drawn, t = 1");
	CHECK(frames_to_full >= 11 && frames_to_full <= 13, "enter takes ~0.2s (%d frames)", frames_to_full);
	CHECK(!iam_presence_is_animating(1), "not animating once shown");

	// --- Exit: still drawn while animating out, then not drawn
	int last_drawn = -1;
	for (int f = 0; f < 30; f++) {
		begin_frame();
		if (iam_presence(1, false, &t, dt, opts)) last_drawn = f;
		end_frame();
	}
	CHECK(t == 0.0f, "hidden: t = 0");
	CHECK(last_drawn >= 7 && last_drawn <= 10, "exit keeps drawing ~0.15s (last drawn frame %d)", last_drawn);

	// --- Reverse mid-enter: continuous
	float prev = 0, max_jump = 0;
	for (int f = 0; f < 6; f++) { begin_frame(); iam_presence(2, true, &prev, dt, opts); end_frame(); }
	float at_reverse = prev, first_after = -1;
	for (int f = 0; f < 20; f++) {
		begin_frame(); iam_presence(2, false, &t, dt, opts); end_frame();
		if (f == 0) first_after = t;
		max_jump = ImMax(max_jump, prev - t); prev = t;  // only decreasing allowed
	}
	CHECK(at_reverse > 0.3f && at_reverse < 1.0f, "reversed mid-way (t=%.3f)", at_reverse);
	CHECK(first_after < at_reverse && at_reverse - first_after < 0.05f, "no jump when reversing (%.3f -> %.3f)", at_reverse, first_after);
	CHECK(max_jump >= 0.0f && t == 0.0f, "exit then reaches 0");

	// --- animate_first = false: shown immediately; hidden at first sight: not drawn
	iam_presence_opts no_first; no_first.animate_first = false;
	begin_frame(); bool d3 = iam_presence(3, true, &t, dt, no_first); end_frame();
	CHECK(d3 && t == 1.0f, "animate_first=false starts fully shown");
	begin_frame(); bool d4 = iam_presence(4, false, &t, dt, opts); end_frame();
	CHECK(!d4 && t == 0.0f, "first seen hidden: not drawn");

	// --- Path drawing: straight path (0,0) -> (100,0) -> (100,100), length 200
	iam_path::begin(ImHashStr("p"), iam_v2(0, 0)).line_to(iam_v2(100, 0)).line_to(iam_v2(100, 100)).end();
	begin_frame();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 mn, mx;
	int v0 = dl->VtxBuffer.Size;
	iam_path_draw(dl, ImHashStr("p"), 0.0f, 0.25f, IM_COL32_WHITE, 2.0f, iam_v2(10, 10));
	bounds(dl, v0, &mn, &mx);
	CHECK(dl->VtxBuffer.Size > v0, "draws vertices");
	CHECK(fabsf(mn.x - 10) < 2.5f && fabsf(mx.x - 60) < 2.5f && mx.y < 13.0f, "[0,0.25] covers the first 50px, offset applied (x %.1f..%.1f, y max %.1f)", mn.x, mx.x, mx.y);

	v0 = dl->VtxBuffer.Size;
	iam_path_draw(dl, ImHashStr("p"), 0.5f, 1.0f, IM_COL32_WHITE, 2.0f);
	bounds(dl, v0, &mn, &mx);
	CHECK(fabsf(mn.x - 100) < 2.5f && fabsf(mn.y) < 2.5f && fabsf(mx.y - 100) < 2.5f, "[0.5,1] is the vertical leg (x %.1f, y %.1f..%.1f)", mn.x, mn.y, mx.y);

	v0 = dl->VtxBuffer.Size;
	iam_path_draw(dl, ImHashStr("p"), 0.3f, 0.3f, IM_COL32_WHITE, 2.0f);
	CHECK(dl->VtxBuffer.Size == v0, "empty range draws nothing");

	v0 = dl->VtxBuffer.Size;
	iam_path_draw(dl, ImHashStr("p"), 0.9f, 0.1f, IM_COL32_WHITE, 2.0f);
	bounds(dl, v0, &mn, &mx);
	CHECK(mn.x < 2.5f && mx.y > 97.0f, "wrapping range draws both ends (x min %.1f, y max %.1f)", mn.x, mx.y);

	iam_path::begin(ImHashStr("q"), iam_v2(0, 50)).line_to(iam_v2(100, 50)).line_to(iam_v2(100, 150)).end();
	v0 = dl->VtxBuffer.Size;
	iam_path_morph_draw(dl, ImHashStr("p"), ImHashStr("q"), 0.5f, 0.0f, 0.25f, IM_COL32_WHITE, 2.0f);
	bounds(dl, v0, &mn, &mx);
	CHECK(fabsf(mn.y - 25) < 2.5f && fabsf(mx.y - 25) < 2.5f, "morph draw halfway between paths (y %.1f..%.1f)", mn.y, mx.y);
	end_frame();

	ImGui::DestroyContext();
	printf("%s (%d failures)\n", g_fail ? "FAILED" : "ALL PASSED", g_fail);
	return g_fail ? 1 : 0;
}
