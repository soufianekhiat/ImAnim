// Spring channels (iam_spring_*) and tween state queries (iam_tween_is_active/progress/velocity_*).
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
	iam_spring_params sp = iam_spring_default();

	// --- Spring converges and settles
	float x = 0;
	int settle_frame = -1;
	for (int f = 0; f < 240; f++) {
		begin_frame();
		x = iam_spring_float(1, 1, 100.0f, sp, dt);
		if (settle_frame < 0 && !iam_tween_is_active(1, 1)) settle_frame = f;
		end_frame();
	}
	CHECK(x == 100.0f, "spring settles exactly on target (x=%.4f)", x);
	CHECK(settle_frame > 10 && settle_frame < 120, "settles in a plausible time (frame %d)", settle_frame);
	CHECK(iam_tween_velocity_float(1, 1) == 0.0f, "velocity 0 when settled");

	// --- Called twice per frame: simulated once
	float a = 0, b = 0;
	begin_frame(); a = iam_spring_float(1, 2, 50.0f, sp, dt); b = iam_spring_float(1, 2, 50.0f, sp, dt); end_frame();
	begin_frame(); a = iam_spring_float(1, 2, 50.0f, sp, dt); b = iam_spring_float(1, 2, 50.0f, sp, dt); end_frame();
	CHECK(a == b && a > 0.0f && a < 50.0f, "two calls per frame give the same value (%.4f %.4f)", a, b);

	// --- Retarget mid-motion keeps velocity
	float prev = 0, vel_before = 0;
	for (int f = 0; f < 10; f++) { begin_frame(); prev = iam_spring_float(2, 1, 100.0f, sp, dt); end_frame(); }
	vel_before = iam_tween_velocity_float(2, 1);
	begin_frame(); iam_spring_float(2, 1, -100.0f, sp, dt); end_frame();
	CHECK(vel_before > 50.0f, "moving fast toward +100 before retarget (v=%.1f)", vel_before);
	float vel_after = iam_tween_velocity_float(2, 1);
	float accel = -sp.stiffness * (prev + 100.0f) - sp.damping * vel_before;  // at retarget time
	float expected = vel_before + accel * dt;
	CHECK(fabsf(vel_after - expected) < fabsf(accel * dt) * 0.25f, "velocity changes by accel*dt only, no reset (%.1f -> %.1f, expected ~%.1f)", vel_before, vel_after, expected);

	// --- vec2 / color springs
	iam_vec2 v2 = iam_v2(0, 0); iam_vec4 c = iam_v4(0, 0, 0, 1);
	for (int f = 0; f < 300; f++) {
		begin_frame();
		v2 = iam_spring_vec2(3, 1, iam_v2(10, -20), sp, dt);
		c = iam_spring_color(3, 2, iam_v4(1, 0.5f, 0, 1), sp, iam_col_oklab, dt, iam_v4(0, 0, 1, 1));
		end_frame();
	}
	ImVec2 V2 = IAM_VEC2_TO_IM(v2); ImVec4 C = IAM_VEC4_TO_IM(c);
	CHECK(fabsf(V2.x - 10) < 1e-3f && fabsf(V2.y + 20) < 1e-3f, "vec2 spring reaches target (%.3f, %.3f)", V2.x, V2.y);
	CHECK(fabsf(C.x - 1) < 2e-3f && fabsf(C.y - 0.5f) < 2e-3f && fabsf(C.z) < 2e-3f, "color spring reaches target (%.3f %.3f %.3f)", C.x, C.y, C.z);

	// --- Time scale 0 freezes springs
	iam_set_global_time_scale(0.0f);
	begin_frame(); float frozen0 = iam_spring_float(4, 1, 10.0f, sp, dt); end_frame();
	begin_frame(); float frozen1 = iam_spring_float(4, 1, 10.0f, sp, dt); end_frame();
	iam_set_global_time_scale(1.0f);
	CHECK(frozen0 == 0.0f && frozen1 == 0.0f, "time scale 0 freezes the spring");

	// --- Tween queries
	iam_ease_desc lin = iam_ease_preset(iam_ease_linear);
	begin_frame(); iam_tween_float(5, 1, 0.0f, 1.0f, lin, iam_policy_crossfade, dt); end_frame();
	CHECK(!iam_tween_is_active(5, 1) && iam_tween_progress(5, 1) == 1.0f, "idle tween: inactive, progress 1");
	float p_prev = -1, last_v = 0; bool monotonic = true;
	int frames_active = 0;
	for (int f = 0; f < 80; f++) {
		begin_frame();
		iam_tween_float(5, 1, 60.0f, 1.0f, lin, iam_policy_crossfade, dt, 0.0f);
		if (iam_tween_is_active(5, 1)) {
			frames_active++;
			float p = iam_tween_progress(5, 1);
			monotonic &= p >= p_prev; p_prev = p;
			last_v = iam_tween_velocity_float(5, 1);
		}
		end_frame();
	}
	CHECK(frames_active >= 55 && frames_active <= 62, "tween active for ~1s (%d frames)", frames_active);
	CHECK(monotonic && p_prev > 0.9f, "progress rises monotonically to ~1 (%.3f)", p_prev);
	CHECK(fabsf(last_v - 60.0f) < 0.5f, "linear tween velocity = 60/s (%.3f)", last_v);
	CHECK(!iam_tween_is_active(5, 1) && iam_tween_velocity_float(5, 1) == 0.0f, "finished tween: inactive, velocity 0");

	// vec2 tween velocity with out_cubic at start = 3 * delta / dur
	begin_frame(); iam_tween_vec2(6, 1, iam_v2(0, 0), 2.0f, iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt); end_frame();
	begin_frame(); iam_tween_vec2(6, 1, iam_v2(20, -40), 2.0f, iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt); end_frame();
	ImVec2 tv = IAM_VEC2_TO_IM(iam_tween_velocity_vec2(6, 1));
	CHECK(tv.x > 20.0f && tv.x < 30.0f && tv.y < -40.0f && tv.y > -60.0f, "out_cubic vec2 velocity near 3*delta/dur at start (%.2f, %.2f)", tv.x, tv.y);

	CHECK(!iam_tween_is_active(999, 999) && iam_tween_progress(999, 999) == 1.0f && iam_tween_velocity_float(999, 999) == 0.0f, "unknown channel: inactive/1/0");

	ImGui::DestroyContext();
	printf("%s (%d failures)\n", g_fail ? "FAILED" : "ALL PASSED", g_fail);
	return g_fail ? 1 : 0;
}
