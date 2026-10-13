// Exercises the public API through iam_vec2/iam_vec4 only; prints values so the default (ImVec2)
// and custom-type builds can be diffed.
#include "im_anim.h"
#include "imgui_internal.h"
#include <stdio.h>

static ImVec2 I2(iam_vec2 v) { return IAM_VEC2_TO_IM(v); }
static ImVec4 I4(iam_vec4 v) { return IAM_VEC4_TO_IM(v); }
static void P2(const char* n, iam_vec2 v) { ImVec2 x = I2(v); printf("%-22s %.4f %.4f\n", n, x.x, x.y); }
static void P4(const char* n, iam_vec4 v) { ImVec4 x = I4(v); printf("%-22s %.4f %.4f %.4f %.4f\n", n, x.x, x.y, x.z, x.w); }
static iam_vec2 resolver2(void*) { return iam_v2(7, 8); }

int main() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(800, 600);
	io.DeltaTime = 1.0f / 60.0f;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	io.IniFilename = nullptr;

	iam_clip::begin(ImHashStr("clip"))
		.key_vec2(1, 0.0f, iam_v2(0, 0))
		.key_vec2(1, 1.0f, iam_v2(100, 50), iam_ease_out_cubic)
		.key_vec4(2, 0.0f, iam_v4(0, 0, 0, 0))
		.key_vec4(2, 1.0f, iam_v4(1, 2, 3, 4))
		.key_color(3, 0.0f, iam_v4(1, 0, 0, 1))
		.key_color(3, 1.0f, iam_v4(0, 0, 1, 1))
		.key_vec2_var(4, 0.0f, iam_v2(10, 10), iam_varv2_inc(5, 5))
		.key_vec2_var(4, 1.0f, iam_v2(20, 20), iam_varv2_inc(5, 5))
		.end();
	iam_path::begin(ImHashStr("path"), iam_v2(0, 0)).line_to(iam_v2(100, 0)).quadratic_to(iam_v2(150, 0), iam_v2(150, 50)).cubic_to(iam_v2(150, 80), iam_v2(120, 100), iam_v2(100, 100)).end();

	iam_instance inst;
	iam_drag_opts dopts; dopts.snap_grid = iam_v2(32, 32); dopts.snap_duration = 0.1f;
	for (int frame = 0; frame < 45; frame++) {
		ImGui::NewFrame();
		iam_update_begin_frame();
		iam_clip_update(io.DeltaTime);
		if (frame == 0) inst = iam_play(ImHashStr("clip"), ImHashStr("inst"));
		ImGui::Begin("w");
		iam_vec2 a = iam_tween_vec2(1, 1, iam_v2(100, 200), 0.5f, iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, io.DeltaTime);
		iam_vec4 b = iam_tween_vec4(1, 2, iam_v4(1, 2, 3, 4), 0.5f, iam_ease_preset(iam_ease_linear), iam_policy_crossfade, io.DeltaTime);
		iam_vec4 c = iam_tween_color(1, 3, iam_v4(0, 1, 0, 1), 0.5f, iam_ease_preset(iam_ease_linear), iam_policy_crossfade, iam_col_oklab, io.DeltaTime, iam_v4(1, 0, 0, 1));
		iam_vec2 r = iam_tween_vec2_resolved(1, 4, resolver2, nullptr, 0.3f, iam_ease_preset(iam_ease_linear), iam_policy_crossfade, io.DeltaTime);
		iam_vec2 o = iam_oscillate_vec2(1, iam_v2(10, 5), iam_v2(1, 2), iam_wave_sine, iam_v2(0, 0), io.DeltaTime);
		iam_vec2 pt = iam_tween_path(1, 5, ImHashStr("path"), 0.6f, iam_ease_preset(iam_ease_linear), iam_policy_crossfade, io.DeltaTime);
		if (frame == 0) iam_drag_begin(9, iam_v2(10, 10));
		if (frame == 5) iam_drag_update(9, iam_v2(50, 45), io.DeltaTime);
		iam_drag_feedback fb = frame >= 10 ? iam_drag_release(9, iam_v2(50, 45), dopts, io.DeltaTime) : iam_drag_feedback();
		if (frame % 11 == 0 || frame == 44) {
			printf("-- frame %d\n", frame);
			P2("tween_vec2", a); P4("tween_vec4", b); P4("tween_color", c); P2("resolved", r); P2("oscillate", o); P2("tween_path", pt);
			iam_vec2 cv2 = iam_v2(-1, -1); iam_vec4 cv4 = iam_v4(-1, -1, -1, -1), cc = iam_v4(-1, -1, -1, -1); iam_vec2 cvar = iam_v2(-1, -1);
			bool ok = inst.get_vec2(1, &cv2) & inst.get_vec4(2, &cv4) & inst.get_color(3, &cc) & inst.get_vec2(4, &cvar);
			printf("%-22s %d\n", "clip get ok", ok ? 1 : 0);
			P2("clip vec2", cv2); P4("clip vec4", cv4); P4("clip color", cc); P2("clip vec2 var", cvar);
			iam_vec2 missing = iam_v2(-1, -1); bool mok = inst.get_vec2(99, &missing);
			printf("%-22s %d\n", "missing ok", mok ? 1 : 0); P2("missing value", missing);
			if (frame >= 10) { P2("drag pos", fb.position); P2("drag offset", fb.offset); }
		}
		ImGui::End();
		ImGui::Render();
	}

	iam_transform t(iam_v2(10, 20), 0.5f, iam_v2(2, 3));
	P2("xf apply", t.apply(iam_v2(1, 1)));
	iam_transform ti = t.inverse(); P2("xf inv pos", ti.position); P2("xf inv scale", ti.scale);
	iam_transform tc = t * ti; P2("xf combo pos", tc.position);
	iam_transform tl = iam_transform_lerp(t, iam_transform(), 0.25f); P2("xf lerp pos", tl.position); P2("xf lerp scale", tl.scale);
	float m[6]; iam_transform_to_matrix(t, m); iam_transform tm = iam_transform_from_matrix(m[0], m[1], m[3], m[4], m[2], m[5]);
	P2("xf mat pos", tm.position); P2("xf mat scale", tm.scale);
	iam_gradient g = iam_gradient::three_color(iam_v4(1, 0, 0, 1), iam_v4(0, 1, 0, 1), iam_v4(0, 0, 1, 1));
	P4("gradient 0.3", g.sample(0.3f)); P4("gradient lerp", iam_gradient_lerp(g, iam_gradient::solid(iam_v4(1, 1, 1, 1)), 0.5f).sample(0.75f));
	iam_vec2 q[4] = { iam_v2(0, 0), iam_v2(1, 0), iam_v2(1, 1), iam_v2(0, 1) };
	iam_transform_quad(q, iam_v2(0.5f, 0.5f), 1.0f, iam_v2(3, 4)); P2("quad0", q[0]); P2("quad2", q[2]);
	iam_make_glyph_quad(q, iam_v2(5, 5), 0.3f, 8, 12, 2); P2("glyph0", q[0]); P2("glyph3", q[3]);
	P2("bezier_cubic", iam_bezier_cubic(iam_v2(0, 0), iam_v2(0, 1), iam_v2(1, 1), iam_v2(1, 0), 0.3f));
	P2("path eval", iam_path_evaluate(ImHashStr("path"), 0.7f)); P2("path tangent", iam_path_tangent(ImHashStr("path"), 0.7f));
	P4("blended color", iam_get_blended_color(iam_v4(1, 0, 0, 1), iam_v4(0, 0, 1, 1), 0.5f, iam_col_oklch));
	ImGui::DestroyContext();
	return 0;
}
