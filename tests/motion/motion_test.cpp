// Layout animation (iam_layout), scroll-linked progress/triggers (iam_scroll_*) and inertia (iam_inertia_*).
#include "im_anim.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <math.h>

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { g_fail++; printf("FAIL %d: ", __LINE__); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)

static void begin_frame() { ImGui::NewFrame(); iam_update_begin_frame(); }
static void end_frame() { ImGui::Render(); }

int main() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(800, 600); io.DeltaTime = 1.0f / 60.0f; io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	float const dt = io.DeltaTime;

	// ---------------- Layout ----------------
	ImVec2 pos, size;
	begin_frame();
	bool anim = iam_layout(1, ImVec2(10, 20), ImVec2(100, 30), &pos, &size, dt);
	end_frame();
	CHECK(!anim && pos.x == 10 && pos.y == 20 && size.x == 100, "first sight snaps to the target");
	int frames = 0;
	float prev_y = pos.y; bool monotonic = true;
	for (int f = 0; f < 120; f++) {
		begin_frame();
		anim = iam_layout(1, ImVec2(10, 120), ImVec2(100, 60), &pos, &size, dt);
		end_frame();
		if (anim) frames++;
		monotonic &= pos.y >= prev_y - 1e-3f; prev_y = pos.y;
		if (f == 2) CHECK(pos.y > 20 && pos.y < 120 && size.y > 30 && size.y < 60, "moves toward the new slot (y %.1f, h %.1f)", pos.y, size.y);
	}
	CHECK(!anim && pos.y == 120 && size.y == 60, "settles on the new slot");
	CHECK(frames > 10 && frames < 60 && monotonic, "glides without overshoot (%d frames)", frames);
	iam_layout_opts no_size; no_size.animate_size = false;
	begin_frame(); iam_layout(1, ImVec2(10, 0), ImVec2(50, 10), &pos, &size, dt, no_size); end_frame();
	CHECK(size.x == 50 && size.y == 10 && pos.y > 0, "animate_size=false: size follows immediately, position animates");

	// ---------------- Scroll ----------------
	// 300px tall window, 20 items of 50px (+ spacing)
	float sy = 0;
	int events10 = 0, events0 = 0; bool vis10 = false;
	float prog10_a = -1, prog10_b = -1, win_prog = -1;
	for (int f = 0; f < 6; f++) {
		begin_frame();
		ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(ImVec2(300, 300));
		if (f == 3) ImGui::SetNextWindowScroll(ImVec2(0, 520));
		ImGui::Begin("scroll", nullptr, ImGuiWindowFlags_NoTitleBar);
		for (int i = 0; i < 20; i++) {
			ImGui::PushID(i);
			ImGui::Dummy(ImVec2(100, 50));
			if (i == 0) { int e = iam_scroll_trigger(ImGui::GetID("t"), 0.5f); if (f == 0) events0 |= e << 4; else events0 |= e; }
			if (i == 10) {
				int e = iam_scroll_trigger(ImGui::GetID("t"), 0.5f, &vis10);
				if (f == 0) CHECK(e == iam_scroll_none && !vis10, "item 10 hidden at the start, no event");
				events10 |= e;
				if (f == 1) prog10_a = iam_scroll_item_progress();
				if (f == 5) prog10_b = iam_scroll_item_progress();
			}
			ImGui::PopID();
		}
		sy = ImGui::GetScrollY();
		win_prog = iam_scroll_window_progress();
		ImGui::End();
		end_frame();
	}
	CHECK(fabsf(sy - 520) < 1.0f, "window scrolled to 520 (%.0f)", sy);
	CHECK((events0 >> 4) == iam_scroll_entered, "item 0 'entered' on first sight");
	CHECK((events0 & iam_scroll_left) != 0, "item 0 'left' after scrolling");
	CHECK((events10 & iam_scroll_entered) != 0 && vis10, "item 10 'entered' after scrolling");
	CHECK(prog10_a == 0.0f && prog10_b > 0.2f && prog10_b < 0.8f, "item progress 0 below the view, mid-way once in view (%.2f -> %.2f)", prog10_a, prog10_b);
	CHECK(win_prog > 0.3f && win_prog < 0.8f, "window progress (%.2f)", win_prog);

	// ---------------- Inertia ----------------
	iam_inertia_opts opts;
	float x = 0;
	// Drag at 600 px/s for 0.2s, release, coast
	for (int f = 0; f < 12; f++) { begin_frame(); x = iam_inertia_float(1, x + 10.0f, true, dt, opts); end_frame(); }
	float at_release = x;
	CHECK(fabsf(at_release - 120.0f) < 1e-3f, "follows while held (%.1f)", at_release);
	int coast_frames = 0;
	for (int f = 0; f < 240; f++) { begin_frame(); x = iam_inertia_float(1, x, false, dt, opts); end_frame(); if (iam_inertia_is_moving(1)) coast_frames++; }
	float travel = x - at_release;
	CHECK(fabsf(travel - 600.0f * opts.decay_time) < 20.0f, "coasts ~ velocity * decay_time (%.1f, expected %.1f)", travel, 600.0f * opts.decay_time);
	CHECK(coast_frames > 20 && !iam_inertia_is_moving(1), "stops after coasting (%d frames)", coast_frames);

	// Bounds: fling past max -> bounces back to max
	iam_inertia_opts b = opts; b.use_bounds = true; b.bounds_min = iam_v2(0, 0); b.bounds_max = iam_v2(150, 0);
	x = 0;
	for (int f = 0; f < 12; f++) { begin_frame(); x = iam_inertia_float(2, x + 10.0f, true, dt, b); end_frame(); }
	float peak = x;
	for (int f = 0; f < 300; f++) { begin_frame(); x = iam_inertia_float(2, x, false, dt, b); end_frame(); peak = ImMax(peak, x); }
	CHECK(peak > 150.0f && x == 150.0f, "overshoots bounds then settles on max (peak %.1f, end %.1f)", peak, x);
	// Rubber band while held past the bound
	x = 150;
	for (int f = 0; f < 10; f++) { begin_frame(); x = iam_inertia_float(2, x + 10.0f, true, dt, b); end_frame(); }
	CHECK(fabsf(x - (150.0f + 100.0f * b.rubber_band)) < 0.5f, "rubber band past max while held (%.1f)", x);
	for (int f = 0; f < 300; f++) { begin_frame(); x = iam_inertia_float(2, x, false, dt, b); end_frame(); }

	// Snap grid: lands on the grid point nearest the projected stop
	iam_inertia_opts s = opts; s.snap_grid = iam_v2(100, 100);
	iam_vec2 p = iam_v2(0, 0);
	for (int f = 0; f < 12; f++) { begin_frame(); ImVec2 q = IAM_VEC2_TO_IM(p); p = iam_inertia_vec2(3, ImVec2(q.x + 10.0f, q.y + 2.0f), true, dt, s); end_frame(); }
	for (int f = 0; f < 300; f++) { begin_frame(); p = iam_inertia_vec2(3, p, false, dt, s); end_frame(); }
	ImVec2 P = IAM_VEC2_TO_IM(p);
	// projected: x = 120 + 600*0.325 = 315 -> 300 ; y = 24 + 120*0.325 = 63 -> 100
	CHECK(P.x == 300.0f && P.y == 100.0f, "snaps to the grid point nearest the projection (%.1f, %.1f)", P.x, P.y);

	// Idle follows programmatic changes; iam_inertia_set stops motion
	begin_frame(); x = iam_inertia_float(1, 42.0f, false, dt, opts); end_frame();
	CHECK(x == 42.0f, "idle: follows programmatic value");
	iam_inertia_set(3, iam_v2(5, 5));
	begin_frame(); P = IAM_VEC2_TO_IM(iam_inertia_vec2(3, iam_v2(5, 5), false, dt, s)); end_frame();
	CHECK(P.x == 5.0f && !iam_inertia_is_moving(3), "iam_inertia_set teleports and stops");

	ImGui::DestroyContext();
	printf("%s (%d failures)\n", g_fail ? "FAILED" : "ALL PASSED", g_fail);
	return g_fail ? 1 : 0;
}
