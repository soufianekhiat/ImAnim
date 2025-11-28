// demo_im_anim.cpp — Demo window showcasing im_anim.h features.
// Author: Soufiane KHIAT
// License: MIT
// Integrate by calling ImAnimDemoWindow() inside your Dear ImGui frame.
// Inspired by Dear ImGui's demo structure.

#include "imgui.h"
#include "im_anim.h"
#include "imgui_internal.h"
#include <math.h>
#include <stdio.h>

// im_anim API is now in global namespace with iam_ prefix

// ============================================================
// HELPER: Get delta time with safety bounds
// ============================================================
static float GetSafeDeltaTime()
{
	float dt = ImGui::GetIO().DeltaTime;
	if (dt <= 0.0f) dt = 1.0f / 60.0f;
	if (dt > 0.1f) dt = 0.1f;
	return dt;
}

// HELPER: Use iam_eval_preset from im_anim API for easing evaluation

// ============================================================
// SECTION: Easing Functions
// ============================================================
static void ShowEasingDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped(
		"im_anim supports 30+ easing functions inspired by anime.js and CSS transitions. "
		"Each easing controls the rate of change during an animation.");

	ImGui::Spacing();

	// Easing preview
	static int selected_ease = iam_ease_out_cubic;
	static float preview_time = 0.0f;
	static bool preview_playing = false;

	const char* ease_names[] = {
		"iam_ease_linear",
		"iam_ease_in_quad", "iam_ease_out_quad", "iam_ease_in_out_quad",
		"iam_ease_in_cubic", "iam_ease_out_cubic", "iam_ease_in_out_cubic",
		"iam_ease_in_quart", "iam_ease_out_quart", "iam_ease_in_out_quart",
		"iam_ease_in_quint", "iam_ease_out_quint", "iam_ease_in_out_quint",
		"iam_ease_in_sine", "iam_ease_out_sine", "iam_ease_in_out_sine",
		"iam_ease_in_expo", "iam_ease_out_expo", "iam_ease_in_out_expo",
		"iam_ease_in_circ", "iam_ease_out_circ", "iam_ease_in_out_circ",
		"iam_ease_in_back", "iam_ease_out_back", "iam_ease_in_out_back",
		"iam_ease_in_elastic", "iam_ease_out_elastic", "iam_ease_in_out_elastic",
		"iam_ease_in_bounce", "iam_ease_out_bounce", "iam_ease_in_out_bounce",
	};

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Preset:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(350);
	ImGui::Combo("##iam_ease_preset", &selected_ease, ease_names, IM_ARRAYSIZE(ease_names));

	ImGui::SameLine();
	if (ImGui::Button(preview_playing ? "Reset" : "Play")) {
		preview_playing = !preview_playing;
		preview_time = 0.0f;
	}

	// Show parameter info for eases that have them
	if (selected_ease >= iam_ease_in_back && selected_ease <= iam_ease_in_out_back) {
		ImGui::TextDisabled("Parameters: overshoot (default: 1.70158)");
	} else if (selected_ease >= iam_ease_in_elastic && selected_ease <= iam_ease_in_out_elastic) {
		ImGui::TextDisabled("Parameters: amplitude (default: 1.0), period (default: 0.3)");
	}

	if (preview_playing) {
		preview_time += dt;
		if (preview_time > 2.0f) preview_time = 0.0f;
	}

	// Draw easing curve
	ImGui::Spacing();
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(300, 200);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Background
	draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(40, 40, 45, 255));
	draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(80, 80, 85, 255));

	// Grid lines
	for (int i = 1; i < 4; i++) {
		float x = canvas_pos.x + canvas_size.x * (i / 4.0f);
		float y = canvas_pos.y + canvas_size.y * (i / 4.0f);
		draw_list->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y), IM_COL32(60, 60, 65, 255));
		draw_list->AddLine(ImVec2(canvas_pos.x, y), ImVec2(canvas_pos.x + canvas_size.x, y), IM_COL32(60, 60, 65, 255));
	}

	// Draw curve using direct evaluation
	ImVec2 prev_pt(canvas_pos.x, canvas_pos.y + canvas_size.y);

	for (int i = 1; i <= 100; i++) {
		float t = i / 100.0f;
		float val = iam_eval_preset(selected_ease, t);

		// Clamp for display (elastic/back can overshoot)
		float display_val = val;
		if (display_val < -0.2f) display_val = -0.2f;
		if (display_val > 1.2f) display_val = 1.2f;

		ImVec2 pt;
		pt.x = canvas_pos.x + canvas_size.x * t;
		pt.y = canvas_pos.y + canvas_size.y * (1.0f - display_val);

		draw_list->AddLine(prev_pt, pt, IM_COL32(100, 180, 255, 255), 2.0f);
		prev_pt = pt;
	}

	// Animated ball
	if (preview_playing && preview_time <= 1.5f) {
		float t = preview_time / 1.5f;  // Normalize to 0-1 over 1.5 seconds
		if (t > 1.0f) t = 1.0f;

		float eased = iam_eval_preset(selected_ease, t);

		// Clamp for display
		float display_eased = eased;
		if (display_eased < -0.2f) display_eased = -0.2f;
		if (display_eased > 1.2f) display_eased = 1.2f;

		float ball_x = canvas_pos.x + canvas_size.x * t;
		float ball_y = canvas_pos.y + canvas_size.y * (1.0f - display_eased);
		draw_list->AddCircleFilled(ImVec2(ball_x, ball_y), 8.0f, IM_COL32(255, 100, 100, 255));

		// Also show horizontal position bar
		float bar_y = canvas_pos.y + canvas_size.y + 20;
		draw_list->AddRectFilled(ImVec2(canvas_pos.x, bar_y), ImVec2(canvas_pos.x + canvas_size.x, bar_y + 20),
			IM_COL32(40, 40, 45, 255));
		float bar_x = canvas_pos.x + canvas_size.x * ImClamp(eased, 0.0f, 1.0f);
		draw_list->AddCircleFilled(ImVec2(bar_x, bar_y + 10), 8.0f, IM_COL32(100, 255, 100, 255));
	}

	ImGui::Dummy(ImVec2(canvas_size.x, canvas_size.y + 40));

	// Custom easing section
	if (ImGui::TreeNode("Custom Bezier Curve")) {
		static float bezier[4] = { 0.25f, 0.1f, 0.25f, 1.0f };
		static float bezier_preview_time = 0.0f;
		static bool bezier_playing = false;

		ImGui::SliderFloat("x1", &bezier[0], 0.0f, 1.0f);
		ImGui::SliderFloat("y1", &bezier[1], -1.0f, 2.0f);
		ImGui::SliderFloat("x2", &bezier[2], 0.0f, 1.0f);
		ImGui::SliderFloat("y2", &bezier[3], -1.0f, 2.0f);

		if (ImGui::Button(bezier_playing ? "Reset##bezier" : "Play##bezier")) {
			bezier_playing = !bezier_playing;
			bezier_preview_time = 0.0f;
		}

		if (bezier_playing) {
			bezier_preview_time += dt;
			if (bezier_preview_time > 2.0f) bezier_preview_time = 0.0f;
		}

		// Draw bezier curve
		ImVec2 bezier_canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 bezier_canvas_size(250, 180);
		ImDrawList* bezier_draw_list = ImGui::GetWindowDrawList();

		bezier_draw_list->AddRectFilled(bezier_canvas_pos,
			ImVec2(bezier_canvas_pos.x + bezier_canvas_size.x, bezier_canvas_pos.y + bezier_canvas_size.y),
			IM_COL32(40, 40, 45, 255));
		bezier_draw_list->AddRect(bezier_canvas_pos,
			ImVec2(bezier_canvas_pos.x + bezier_canvas_size.x, bezier_canvas_pos.y + bezier_canvas_size.y),
			IM_COL32(80, 80, 85, 255));

		// Draw control points and handles
		ImVec2 p0(bezier_canvas_pos.x, bezier_canvas_pos.y + bezier_canvas_size.y);
		ImVec2 p1(bezier_canvas_pos.x + bezier[0] * bezier_canvas_size.x, bezier_canvas_pos.y + bezier_canvas_size.y * (1.0f - bezier[1]));
		ImVec2 p2(bezier_canvas_pos.x + bezier[2] * bezier_canvas_size.x, bezier_canvas_pos.y + bezier_canvas_size.y * (1.0f - bezier[3]));
		ImVec2 p3(bezier_canvas_pos.x + bezier_canvas_size.x, bezier_canvas_pos.y);

		// Draw control handles
		bezier_draw_list->AddLine(p0, p1, IM_COL32(255, 100, 100, 150), 1.0f);
		bezier_draw_list->AddLine(p3, p2, IM_COL32(100, 100, 255, 150), 1.0f);
		bezier_draw_list->AddCircleFilled(p1, 5.0f, IM_COL32(255, 100, 100, 255));
		bezier_draw_list->AddCircleFilled(p2, 5.0f, IM_COL32(100, 100, 255, 255));

		// Draw bezier curve
		bezier_draw_list->AddBezierCubic(p0, p1, p2, p3, IM_COL32(100, 255, 100, 255), 2.0f, 64);

		// Animated ball on curve
		if (bezier_playing && bezier_preview_time <= 1.5f) {
			float t = bezier_preview_time / 1.5f;
			if (t > 1.0f) t = 1.0f;

			// Evaluate cubic bezier for Y (the eased value)
			auto cubic_bezier_y = [](float x, float x1, float y1, float x2, float y2) {
				float t_guess = x;
				for (int i = 0; i < 5; ++i) {
					float mt = 1.f - t_guess;
					float bx = 3.f*mt*mt*t_guess*x1 + 3.f*mt*t_guess*t_guess*x2 + t_guess*t_guess*t_guess;
					float dx = 3.f*mt*mt*x1 + 6.f*mt*t_guess*(x2 - x1) + 3.f*t_guess*t_guess*(1.f - x2);
					if (dx != 0.f) t_guess = t_guess - (bx - x) / dx;
					if (t_guess < 0.f) t_guess = 0.f; if (t_guess > 1.f) t_guess = 1.f;
				}
				float mt = 1.f - t_guess;
				return 3.f*mt*mt*t_guess*y1 + 3.f*mt*t_guess*t_guess*y2 + t_guess*t_guess*t_guess;
			};

			float eased = cubic_bezier_y(t, bezier[0], bezier[1], bezier[2], bezier[3]);
			float ball_x = bezier_canvas_pos.x + bezier_canvas_size.x * t;
			float ball_y = bezier_canvas_pos.y + bezier_canvas_size.y * (1.0f - ImClamp(eased, -0.2f, 1.2f));
			bezier_draw_list->AddCircleFilled(ImVec2(ball_x, ball_y), 6.0f, IM_COL32(255, 255, 100, 255));
		}

		ImGui::Dummy(bezier_canvas_size);
		ImGui::TextDisabled("Usage: iam_ease_bezier(%.2f, %.2f, %.2f, %.2f)", bezier[0], bezier[1], bezier[2], bezier[3]);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Spring Physics")) {
		static float mass = 1.0f, stiffness = 120.0f, damping = 20.0f, v0 = 0.0f;
		static float spring_preview_time = 0.0f;
		static bool spring_playing = false;

		ImGui::SliderFloat("Mass", &mass, 0.1f, 5.0f);
		ImGui::SliderFloat("Stiffness", &stiffness, 10.0f, 500.0f);
		ImGui::SliderFloat("Damping", &damping, 1.0f, 50.0f);
		ImGui::SliderFloat("Initial Velocity", &v0, -10.0f, 10.0f);

		if (ImGui::Button(spring_playing ? "Reset##spring" : "Play##spring")) {
			spring_playing = !spring_playing;
			spring_preview_time = 0.0f;
		}

		if (spring_playing) {
			spring_preview_time += dt;
			if (spring_preview_time > 3.0f) spring_preview_time = 0.0f;
		}

		// Draw spring response curve
		ImVec2 spring_canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 spring_canvas_size(300, 180);
		ImDrawList* spring_draw_list = ImGui::GetWindowDrawList();

		spring_draw_list->AddRectFilled(spring_canvas_pos,
			ImVec2(spring_canvas_pos.x + spring_canvas_size.x, spring_canvas_pos.y + spring_canvas_size.y),
			IM_COL32(40, 40, 45, 255));
		spring_draw_list->AddRect(spring_canvas_pos,
			ImVec2(spring_canvas_pos.x + spring_canvas_size.x, spring_canvas_pos.y + spring_canvas_size.y),
			IM_COL32(80, 80, 85, 255));

		// Draw target line at y=1
		float target_y = spring_canvas_pos.y + spring_canvas_size.y * 0.2f;
		spring_draw_list->AddLine(ImVec2(spring_canvas_pos.x, target_y),
			ImVec2(spring_canvas_pos.x + spring_canvas_size.x, target_y), IM_COL32(100, 100, 100, 100), 1.0f);

		// Spring evaluation function
		auto eval_spring = [](float u, float m, float k, float c, float vel0) {
			float wn = sqrtf(k / m);
			float zeta = c / (2.f * sqrtf(k * m));
			if (zeta < 1.f) {
				float wdn = wn * sqrtf(1.f - zeta*zeta);
				float A = 1.f;
				float B = (zeta * wn * A + vel0) / wdn;
				float e = expf(-zeta * wn * u);
				return 1.f - e * (A * ImCos(wdn*u) + B * ImSin(wdn*u));
			} else if (zeta == 1.f) {
				float e = expf(-wn * u);
				return 1.f - e * (1.f + wn * u);
			} else {
				float wd = wn * sqrtf(zeta*zeta - 1.f);
				float e1 = expf(-(zeta * wn - wd) * u);
				float e2 = expf(-(zeta * wn + wd) * u);
				return 1.f - 0.5f*(e1 + e2);
			}
		};

		// Draw spring curve
		ImVec2 spring_prev_pt(spring_canvas_pos.x, spring_canvas_pos.y + spring_canvas_size.y);
		for (int i = 1; i <= 100; i++) {
			float t = (float)i / 100.0f;
			float val = eval_spring(t * 2.0f, mass, stiffness, damping, v0);  // 2 seconds worth

			float display_val = ImClamp(val, -0.2f, 1.4f);
			ImVec2 pt;
			pt.x = spring_canvas_pos.x + spring_canvas_size.x * t;
			pt.y = spring_canvas_pos.y + spring_canvas_size.y * (1.0f - display_val * 0.8f);

			spring_draw_list->AddLine(spring_prev_pt, pt, IM_COL32(100, 200, 255, 255), 2.0f);
			spring_prev_pt = pt;
		}

		// Animated ball
		if (spring_playing && spring_preview_time <= 2.0f) {
			float t = spring_preview_time / 2.0f;
			if (t > 1.0f) t = 1.0f;

			float val = eval_spring(t * 2.0f, mass, stiffness, damping, v0);
			float display_val = ImClamp(val, -0.2f, 1.4f);
			float ball_x = spring_canvas_pos.x + spring_canvas_size.x * t;
			float ball_y = spring_canvas_pos.y + spring_canvas_size.y * (1.0f - display_val * 0.8f);
			spring_draw_list->AddCircleFilled(ImVec2(ball_x, ball_y), 6.0f, IM_COL32(255, 100, 100, 255));
		}

		ImGui::Dummy(spring_canvas_size);
		ImGui::TextDisabled("Usage: iam_ease_spring_desc(%.1f, %.1f, %.1f, %.1f)", mass, stiffness, damping, v0);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Steps Easing")) {
		static int step_count = 5;
		static int step_mode = 0;  // 0=end, 1=start, 2=both
		static float steps_preview_time = 0.0f;
		static bool steps_playing = false;

		ImGui::SliderInt("Step Count", &step_count, 1, 12);
		const char* mode_names[] = { "Jump End (0)", "Jump Start (1)", "Jump Both (2)" };
		ImGui::Combo("Step Mode", &step_mode, mode_names, 3);

		ImGui::SameLine();
		if (ImGui::Button(steps_playing ? "Reset##steps" : "Play##steps")) {
			steps_playing = !steps_playing;
			steps_preview_time = 0.0f;
		}

		if (steps_playing) {
			steps_preview_time += dt;
			if (steps_preview_time > 2.0f) steps_preview_time = 0.0f;
		}

		// Draw steps curve
		ImVec2 steps_canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 steps_canvas_size(250, 150);
		ImDrawList* steps_draw_list = ImGui::GetWindowDrawList();

		steps_draw_list->AddRectFilled(steps_canvas_pos,
			ImVec2(steps_canvas_pos.x + steps_canvas_size.x, steps_canvas_pos.y + steps_canvas_size.y),
			IM_COL32(40, 40, 45, 255));
		steps_draw_list->AddRect(steps_canvas_pos,
			ImVec2(steps_canvas_pos.x + steps_canvas_size.x, steps_canvas_pos.y + steps_canvas_size.y),
			IM_COL32(80, 80, 85, 255));

		// Steps evaluation function
		auto eval_steps = [](float t, int steps, int mode) -> float {
			if (steps < 1) steps = 1;
			float s = (float)steps;
			if (mode == 1) {  // jump-start
				return floorf(t * s + 1.0f) / s;
			} else if (mode == 2) {  // jump-both
				return (floorf(t * s) + 1.0f) / (s + 1.0f);
			} else {  // jump-end (default)
				return floorf(t * s) / s;
			}
		};

		// Draw horizontal step lines
		for (int i = 0; i <= step_count; ++i) {
			float y = steps_canvas_pos.y + steps_canvas_size.y * (1.0f - (float)i / step_count);
			steps_draw_list->AddLine(
				ImVec2(steps_canvas_pos.x, y),
				ImVec2(steps_canvas_pos.x + steps_canvas_size.x, y),
				IM_COL32(60, 60, 65, 100), 1.0f);
		}

		// Draw step function
		float prev_val = eval_steps(0.0f, step_count, step_mode);
		for (int i = 1; i <= 100; i++) {
			float t = (float)i / 100.0f;
			float val = eval_steps(t, step_count, step_mode);

			float x0 = steps_canvas_pos.x + steps_canvas_size.x * ((i - 1) / 100.0f);
			float x1 = steps_canvas_pos.x + steps_canvas_size.x * t;
			float y0 = steps_canvas_pos.y + steps_canvas_size.y * (1.0f - prev_val);
			float y1 = steps_canvas_pos.y + steps_canvas_size.y * (1.0f - val);

			// Draw horizontal segment
			steps_draw_list->AddLine(ImVec2(x0, y0), ImVec2(x1, y0), IM_COL32(255, 180, 100, 255), 2.0f);
			// Draw vertical jump
			if (val != prev_val) {
				steps_draw_list->AddLine(ImVec2(x1, y0), ImVec2(x1, y1), IM_COL32(255, 180, 100, 100), 1.0f);
			}
			prev_val = val;
		}

		// Animated indicator
		if (steps_playing && steps_preview_time <= 1.5f) {
			float t = steps_preview_time / 1.5f;
			if (t > 1.0f) t = 1.0f;

			float val = eval_steps(t, step_count, step_mode);
			float ball_x = steps_canvas_pos.x + steps_canvas_size.x * t;
			float ball_y = steps_canvas_pos.y + steps_canvas_size.y * (1.0f - val);
			steps_draw_list->AddCircleFilled(ImVec2(ball_x, ball_y), 6.0f, IM_COL32(100, 255, 200, 255));
		}

		ImGui::Dummy(steps_canvas_size);
		ImGui::TextDisabled("Usage: iam_ease_steps_desc(%d, %d)", step_count, step_mode);
		ImGui::TreePop();
	}
}

// ============================================================
// SECTION: Custom Easing
// ============================================================

// Example custom easing functions
static float CustomEaseSmooth(float t) {
	// Attempt a smoothstep-like ease
	return t * t * (3.0f - 2.0f * t);
}

static float CustomEaseBouncy(float t) {
	// Custom bouncy effect
	float n = 7.5625f;
	float d = 2.75f;
	if (t < 1.0f / d) return n * t * t;
	if (t < 2.0f / d) { t -= 1.5f / d; return n * t * t + 0.75f; }
	if (t < 2.5f / d) { t -= 2.25f / d; return n * t * t + 0.9375f; }
	t -= 2.625f / d;
	return n * t * t + 0.984375f;
}

static float CustomEaseWobble(float t) {
	// Wobble with overshoot
	return t + sinf(t * 3.14159f * 3.0f) * (1.0f - t) * 0.3f;
}

static void ShowCustomEasingDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped(
		"Register your own easing functions using iam_register_custom_ease(). "
		"You get 16 slots (0-15) for custom easing callbacks.");

	// Register custom easings (safe to call every frame - just overwrites)
	static bool initialized = false;
	if (!initialized) {
		iam_register_custom_ease(0, CustomEaseSmooth);
		iam_register_custom_ease(1, CustomEaseBouncy);
		iam_register_custom_ease(2, CustomEaseWobble);
		initialized = true;
	}

	ImGui::Spacing();

	// Show registered slots
	ImGui::Text("Registered Custom Easings:");
	ImGui::BulletText("Slot 0: Smooth (smoothstep)");
	ImGui::BulletText("Slot 1: Bouncy (bounce variation)");
	ImGui::BulletText("Slot 2: Wobble (overshoot with sine)");

	ImGui::Spacing();
	ImGui::Separator();

	// Interactive demo
	static int selected_slot = 0;
	static bool playing = false;
	static float target = 0.0f;

	ImGui::Text("Test Custom Easing:");
	ImGui::RadioButton("Smooth##custom", &selected_slot, 0); ImGui::SameLine();
	ImGui::RadioButton("Bouncy##custom", &selected_slot, 1); ImGui::SameLine();
	ImGui::RadioButton("Wobble##custom", &selected_slot, 2);

	if (ImGui::Button(playing ? "Reset##custom" : "Play##custom")) {
		playing = !playing;
		target = playing ? 1.0f : 0.0f;
	}

	ImGuiID id = ImHashStr("custom_ease_demo");
	float value = iam_tween_float(id, ImHashStr("pos"), target, 1.0f,
		iam_ease_custom_fn(selected_slot), iam_policy_crossfade, dt);

	// Draw animated bar
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	float canvas_w = ImGui::GetContentRegionAvail().x;
	float canvas_h = 30.0f;
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	draw_list->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_w, canvas_pos.y + canvas_h),
		IM_COL32(40, 42, 48, 255), 4.0f);

	float bar_w = value * (canvas_w - 10.0f);
	draw_list->AddRectFilled(
		ImVec2(canvas_pos.x + 5, canvas_pos.y + 5),
		ImVec2(canvas_pos.x + 5 + bar_w, canvas_pos.y + canvas_h - 5),
		IM_COL32(100, 180, 255, 255), 3.0f);

	ImGui::Dummy(ImVec2(canvas_w, canvas_h));

	// Show code example
	ImGui::Spacing();
	ImGui::TextDisabled("Usage:");
	ImGui::TextDisabled("  iam_register_custom_ease(0, MyEaseFunc);");
	ImGui::TextDisabled("  iam_tween_float(id, ch, target, dur, iam_ease_custom_fn(0), policy, dt);");
}

// ============================================================
// SECTION: Basic Tweens
// ============================================================
static void ShowBasicTweensDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped(
		"Tweens smoothly interpolate values over time. Each tween is identified by a unique (id, channel) pair. "
		"Call the tween function every frame with your target value - the library handles the animation.");

	ImGui::Spacing();
	ImGui::Separator();

	// Float tween
	if (ImGui::TreeNode("Float Tween")) {
		static float target = 50.0f;
		ImGui::SliderFloat("Target", &target, 0.0f, 100.0f);

		ImGuiID id = ImHashStr("float_demo");
		float value = iam_tween_float(id, 0, target, 1.0f, iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		ImGui::ProgressBar(value / 100.0f, ImVec2(-1, 0), "");
		ImGui::SameLine();
		ImGui::Text("%.1f", value);

		ImGui::TextDisabled("iam_tween_float(id, channel, %.1f, 1.0f, ease_out_cubic, crossfade, dt)", target);
		ImGui::TreePop();
	}

	// Vec2 tween
	if (ImGui::TreeNode("Vec2 Tween")) {
		static ImVec2 target(150.0f, 80.0f);
		ImGui::SliderFloat2("Target", &target.x, 0.0f, 280.0f);

		ImGuiID id = ImHashStr("vec2_demo");
		ImVec2 value = iam_tween_vec2(id, 0, target, 1.0f, iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Draw a dot at the animated position
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300, 150);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(40, 40, 45, 255));
		draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(80, 80, 85, 255));

		// Clamp position to canvas
		float draw_x = ImClamp(value.x, 0.0f, canvas_size.x - 10.0f);
		float draw_y = ImClamp(value.y, 0.0f, canvas_size.y - 10.0f);
		draw_list->AddCircleFilled(ImVec2(canvas_pos.x + draw_x + 10, canvas_pos.y + draw_y + 10), 10.0f,
			IM_COL32(100, 200, 255, 255));
		ImGui::Dummy(canvas_size);

		ImGui::Text("Position: (%.1f, %.1f)", value.x, value.y);
		ImGui::TreePop();
	}

	// Int tween
	if (ImGui::TreeNode("Int Tween")) {
		static int target = 50;
		ImGui::SliderInt("Target", &target, 0, 100);

		ImGuiID id = ImHashStr("int_demo");
		int value = iam_tween_int(id, 0, target, 1.5f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

		ImGui::Text("Value: %d", value);
		ImGui::TextDisabled("Useful for step-based animations, frame indices, etc.");
		ImGui::TreePop();
	}

	// Vec4 tween
	if (ImGui::TreeNode("Vec4 Tween")) {
		static ImVec4 target(1.0f, 0.5f, 0.2f, 1.0f);
		ImGui::ColorEdit4("Target", &target.x);

		ImGuiID id = ImHashStr("vec4_demo");
		ImVec4 value = iam_tween_vec4(id, 0, target, 1.0f, iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		ImGui::ColorButton("Animated", value, 0, ImVec2(100, 30));
		ImGui::SameLine();
		ImGui::Text("(%.2f, %.2f, %.2f, %.2f)", value.x, value.y, value.z, value.w);
		ImGui::TreePop();
	}

	// Multi-property animation
	if (ImGui::TreeNodeEx("Multi-Property Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::TextDisabled("Animate multiple properties on the same object with different timings");
		ImGui::Spacing();

		static bool expanded = false;
		if (ImGui::Button(expanded ? "Collapse" : "Expand")) expanded = !expanded;
		ImGui::SameLine();

		ImGuiID id = ImHashStr("multi_prop_demo");

		// Animate multiple properties
		float scale = iam_tween_float(id, ImHashStr("scale"), expanded ? 1.2f : 1.0f, 0.4f,
			iam_ease_spring_desc(1.0f, 180.f, 15.f, 0.f), iam_policy_crossfade, dt);
		float rotation = iam_tween_float(id, ImHashStr("rotation"), expanded ? 45.0f : 0.0f, 0.5f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);
		float alpha = iam_tween_float(id, ImHashStr("alpha"), expanded ? 1.0f : 0.7f, 0.3f,
			iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);
		ImVec4 color = iam_tween_color(id, ImHashStr("color"),
			expanded ? ImVec4(0.3f, 0.8f, 0.5f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
			0.6f, iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, iam_col_oklab, dt);

		// Draw animated shape
		ImVec2 center = ImGui::GetCursorScreenPos();
		center.x += 80;
		center.y += 60;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Draw rotated and scaled rectangle
		float size = 40.0f * scale;
		float rad = rotation * 3.14159f / 180.0f;
		ImVec2 corners[4];
		for (int i = 0; i < 4; i++) {
			float angle = rad + (float)i * 3.14159f * 0.5f + 3.14159f * 0.25f;
			corners[i].x = center.x + ImCos(angle) * size * 0.707f;
			corners[i].y = center.y + ImSin(angle) * size * 0.707f;
		}

		ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, alpha));
		draw_list->AddQuadFilled(corners[0], corners[1], corners[2], corners[3], col);
		draw_list->AddQuad(corners[0], corners[1], corners[2], corners[3], IM_COL32(255, 255, 255, (int)(alpha * 100)), 2.0f);

		ImGui::Dummy(ImVec2(160, 120));

		ImGui::Text("Scale: %.2f  Rotation: %.1f  Alpha: %.2f", scale, rotation, alpha);
		ImGui::TreePop();
	}

	// Staggered wave animation
	if (ImGui::TreeNode("Staggered Wave Animation")) {
		ImGui::TextDisabled("Multiple items with offset timing create a wave effect");
		ImGui::Spacing();

		static bool wave_active = false;
		static float wave_time = 0.0f;

		if (ImGui::Button(wave_active ? "Reset" : "Start Wave")) {
			wave_active = !wave_active;
			wave_time = 0.0f;
		}

		if (wave_active) wave_time += dt;
		if (wave_time > 3.0f) wave_time = 0.0f;

		ImVec2 origin = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(origin, ImVec2(origin.x + 400, origin.y + 80), IM_COL32(40, 40, 45, 255));

		const int num_dots = 12;
		for (int i = 0; i < num_dots; i++) {
			ImGuiID id = ImGui::GetID(i + 100);
			float stagger_delay = (float)i * 0.08f;
			float local_time = wave_active ? ImMax(0.0f, wave_time - stagger_delay) : 0.0f;
			float normalized_t = ImClamp(local_time / 0.6f, 0.0f, 1.0f);

			// Animate Y position with bounce
			float y_offset = iam_tween_float(id, ImHashStr("wave_y"),
				wave_active ? (normalized_t > 0.0f ? -25.0f : 0.0f) : 0.0f,
				0.5f, iam_ease_preset(iam_ease_out_bounce), iam_policy_crossfade, dt);

			// Animate scale (slower for smoother effect)
			float dot_scale = iam_tween_float(id, ImHashStr("wave_scale"),
				wave_active ? (normalized_t > 0.0f ? 1.3f : 1.0f) : 1.0f,
				0.8f, iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

			// Animate color
			ImVec4 dot_color = iam_tween_color(id, ImHashStr("wave_color"),
				wave_active ? ImVec4(0.3f, 0.7f + (float)i * 0.02f, 1.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
				0.4f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, iam_col_oklab, dt);

			float x = origin.x + 20 + (float)i * 32;
			float y = origin.y + 50 + y_offset;
			float radius = 10.0f * dot_scale;

			draw_list->AddCircleFilled(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(dot_color));
		}

		ImGui::Dummy(ImVec2(400, 80));
		ImGui::TreePop();
	}

	// Spring physics comparison
	if (ImGui::TreeNode("Spring Physics Comparison")) {
		ImGui::TextDisabled("Compare different spring parameters - adjust stiffness and damping");
		ImGui::Spacing();

		static float spring_stiffness = 180.0f;
		static float spring_damping = 15.0f;
		static bool spring_triggered = false;

		ImGui::SliderFloat("Stiffness", &spring_stiffness, 50.0f, 400.0f);
		ImGui::SliderFloat("Damping", &spring_damping, 5.0f, 40.0f);
		if (ImGui::Button("Trigger Spring")) spring_triggered = !spring_triggered;

		ImGui::Spacing();

		ImVec2 origin = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(origin, ImVec2(origin.x + 300, origin.y + 180), IM_COL32(40, 40, 45, 255));

		struct SpringConfig {
			const char* name;
			float stiffness;
			float damping;
			ImU32 color;
		};
		SpringConfig configs[] = {
			{"Bouncy", 120.0f, 8.0f, IM_COL32(255, 100, 100, 255)},
			{"Smooth", 200.0f, 25.0f, IM_COL32(100, 255, 100, 255)},
			{"Stiff", 300.0f, 30.0f, IM_COL32(100, 100, 255, 255)},
			{"Custom", spring_stiffness, spring_damping, IM_COL32(255, 255, 100, 255)}
		};

		float const vis_width = 300.0f;
		float const text_width = 100.0f;
		for (int i = 0; i < 4; i++) {
			ImGuiID id = ImGui::GetID(i + 200);
			float x_pos = iam_tween_float(id, ImHashStr("spring_x"),
				spring_triggered ? (vis_width - 20.0f) : 20.0f,
				1.5f, iam_ease_spring_desc(1.0f, configs[i].stiffness, configs[i].damping, 0.0f),
				iam_policy_crossfade, dt);

			float y = origin.y + 25 + (float)i * 38;
			draw_list->AddCircleFilled(ImVec2(origin.x + x_pos, y), 12.0f, configs[i].color);
			draw_list->AddText(ImVec2(origin.x + vis_width + 10, y - 8), IM_COL32(200, 200, 200, 255), configs[i].name);
		}

		ImGui::Dummy(ImVec2(vis_width + text_width, 180));
		ImGui::TreePop();
	}

	// Smooth counter animation
	if (ImGui::TreeNode("Animated Counter")) {
		ImGui::TextDisabled("Smooth number counting animation using int tweens");
		ImGui::Spacing();

		static int counter_target = 0;
		if (ImGui::Button("+100")) counter_target += 100;
		ImGui::SameLine();
		if (ImGui::Button("+1000")) counter_target += 1000;
		ImGui::SameLine();
		if (ImGui::Button("Reset")) counter_target = 0;

		ImGuiID id = ImHashStr("counter_demo");
		int animated_value = iam_tween_int(id, 0, counter_target, 0.8f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
		ImGui::SetWindowFontScale(2.0f);
		ImGui::Text("%d", animated_value);
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopFont();

		ImGui::TextDisabled("Target: %d", counter_target);
		ImGui::TreePop();
	}
}

// ============================================================
// SECTION: Color Tweens
// ============================================================
static void ShowColorTweensDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped(
		"Color tweening supports multiple color spaces for perceptually pleasing transitions. "
		"OKLAB produces the most visually uniform interpolation.");

	ImGui::Spacing();

	static ImVec4 color_a(1.0f, 0.0f, 0.0f, 1.0f);  // Red
	static ImVec4 color_b(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
	static bool toggle = false;

	ImGui::ColorEdit4("Color A", &color_a.x, ImGuiColorEditFlags_NoInputs);
	ImGui::SameLine();
	ImGui::ColorEdit4("Color B", &color_b.x, ImGuiColorEditFlags_NoInputs);
	ImGui::SameLine();
	if (ImGui::Button("Toggle")) toggle = !toggle;

	ImVec4 target = toggle ? color_b : color_a;

	ImGui::Spacing();

	const char* space_names[] = { "sRGB", "Linear sRGB", "HSV", "OKLAB", "OKLCH" };
	int spaces[] = { iam_col_srgb, iam_col_srgb_linear, iam_col_hsv, iam_col_oklab, iam_col_oklch };

	for (int i = 0; i < 5; i++) {
		ImGuiID id = ImHashStr("color_space_demo");
		ImVec4 value = iam_tween_color(id, (ImGuiID)i, target, 1.5f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, spaces[i], dt);

		ImGui::ColorButton(space_names[i], value, 0, ImVec2(120, 40));
		ImGui::SameLine();
		ImGui::Text("%s", space_names[i]);
	}

	ImGui::Spacing();
	ImGui::TextDisabled("OKLAB/OKLCH avoid muddy middle colors. OKLCH uses cylindrical coords (hue interpolation).");
}

// ============================================================
// SECTION: Tween Policies
// ============================================================
static void ShowPoliciesDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped(
		"Policies control how tweens behave when the target changes mid-animation:");

	ImGui::BulletText("Crossfade: Smoothly blend into new target (default)");
	ImGui::BulletText("Cut: Instantly snap to new target");
	ImGui::BulletText("Queue: Finish current animation, then start new one");

	ImGui::Spacing();
	ImGui::Separator();

	static float target = 0.0f;
	if (ImGui::Button("Target = 0")) target = 0.0f;
	ImGui::SameLine();
	if (ImGui::Button("Target = 50")) target = 50.0f;
	ImGui::SameLine();
	if (ImGui::Button("Target = 100")) target = 100.0f;

	ImGui::Spacing();

	auto ez = iam_ease_preset(iam_ease_out_cubic);

	// Crossfade
	{
		ImGuiID id = ImHashStr("policy_crossfade");
		float value = iam_tween_float(id, 0, target, 1.5f, ez, iam_policy_crossfade, dt);
		ImGui::ProgressBar(value / 100.0f, ImVec2(250, 0));
		ImGui::SameLine();
		ImGui::Text("Crossfade: %.1f", value);
	}

	// Cut
	{
		ImGuiID id = ImHashStr("policy_cut");
		float value = iam_tween_float(id, 0, target, 1.5f, ez, iam_policy_cut, dt);
		ImGui::ProgressBar(value / 100.0f, ImVec2(250, 0));
		ImGui::SameLine();
		ImGui::Text("Cut: %.1f", value);
	}

	// Queue
	{
		ImGuiID id = ImHashStr("policy_queue");
		float value = iam_tween_float(id, 0, target, 1.5f, ez, iam_policy_queue, dt);
		ImGui::ProgressBar(value / 100.0f, ImVec2(250, 0));
		ImGui::SameLine();
		ImGui::Text("Queue: %.1f", value);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Visual policy comparison - each policy in its own horizontal lane
	if (ImGui::TreeNode("Visual Comparison")) {
		ImGui::TextWrapped("Each row shows the same animation with different policies. "
			"Click buttons rapidly to see differences:");
		ImGui::BulletText("Cut (green): Jumps instantly to target");
		ImGui::BulletText("Crossfade (red): Smoothly transitions, interrupts on new target");
		ImGui::BulletText("Queue (blue): Finishes current animation before starting next");
		ImGui::Spacing();

		// Target X positions (horizontal movement)
		static float target_x_positions[] = {30.0f, 120.0f, 220.0f, 320.0f};
		static int visual_target_idx = 0;

		if (ImGui::Button("A (Left)")) visual_target_idx = 0;
		ImGui::SameLine();
		if (ImGui::Button("B")) visual_target_idx = 1;
		ImGui::SameLine();
		if (ImGui::Button("C")) visual_target_idx = 2;
		ImGui::SameLine();
		if (ImGui::Button("D (Right)")) visual_target_idx = 3;

		float target_x = target_x_positions[visual_target_idx];

		ImVec2 origin = ImGui::GetCursorScreenPos();
		float const vis_width = 360.0f;
		float const text_width = 80.0f;
		ImVec2 canvas_size(vis_width, 130);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Background
		draw_list->AddRectFilled(origin, ImVec2(origin.x + vis_width, origin.y + canvas_size.y),
			IM_COL32(40, 40, 45, 255));
		draw_list->AddRect(origin, ImVec2(origin.x + vis_width, origin.y + canvas_size.y),
			IM_COL32(80, 80, 85, 255));

		// Draw vertical lines at target positions
		for (int i = 0; i < 4; ++i) {
			float x = origin.x + target_x_positions[i];
			ImU32 col = (i == visual_target_idx) ? IM_COL32(255, 255, 100, 100) : IM_COL32(80, 80, 80, 100);
			draw_list->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + canvas_size.y), col, 1.0f);
		}

		// Lane labels (on right side)
		float lane_height = 40.0f;
		float y_cut = origin.y + 20.0f;
		float y_crossfade = origin.y + 20.0f + lane_height;
		float y_queue = origin.y + 20.0f + lane_height * 2;

		draw_list->AddText(ImVec2(origin.x + vis_width + 10, y_cut - 4), IM_COL32(100, 255, 100, 180), "Cut");
		draw_list->AddText(ImVec2(origin.x + vis_width + 10, y_crossfade - 4), IM_COL32(255, 100, 100, 180), "Crossfade");
		draw_list->AddText(ImVec2(origin.x + vis_width + 10, y_queue - 4), IM_COL32(100, 100, 255, 180), "Queue");

		auto ez_vis = iam_ease_preset(iam_ease_out_cubic);

		// Cut square (green) - top lane - should jump instantly
		{
			ImGuiID id = ImHashStr("policy_visual_cut");
			float x = iam_tween_float(id, 0, target_x, 0.8f, ez_vis, iam_policy_cut, dt);
			draw_list->AddRectFilled(
				ImVec2(origin.x + x - 14, y_cut - 14),
				ImVec2(origin.x + x + 14, y_cut + 14),
				IM_COL32(100, 255, 100, 255));
		}

		// Crossfade square (red) - middle lane - smooth transition
		{
			ImGuiID id = ImHashStr("policy_visual_crossfade");
			float x = iam_tween_float(id, 0, target_x, 0.8f, ez_vis, iam_policy_crossfade, dt);
			draw_list->AddRectFilled(
				ImVec2(origin.x + x - 14, y_crossfade - 14),
				ImVec2(origin.x + x + 14, y_crossfade + 14),
				IM_COL32(255, 100, 100, 255));
		}

		// Queue square (blue) - bottom lane - finishes before starting next
		{
			ImGuiID id = ImHashStr("policy_visual_queue");
			float x = iam_tween_float(id, 0, target_x, 0.8f, ez_vis, iam_policy_queue, dt);
			draw_list->AddRectFilled(
				ImVec2(origin.x + x - 14, y_queue - 14),
				ImVec2(origin.x + x + 14, y_queue + 14),
				IM_COL32(100, 100, 255, 255));
		}

		ImGui::Dummy(ImVec2(vis_width + text_width, canvas_size.y));
		ImGui::TreePop();
	}
}

// ============================================================
// SECTION: Interactive Widgets
// ============================================================
static void ShowWidgetsDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped(
		"Combining tweens with ImGui widgets creates polished UI interactions.");

	ImGui::Spacing();
	ImGui::Separator();

	// Animated buttons - using fixed layout to prevent movement
	if (ImGui::TreeNodeEx("Animated Buttons", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::TextDisabled("Hover over buttons to see animation effects");
		ImGui::Spacing();

		// Use a child window with fixed size to prevent layout shifts
		ImVec2 buttons_area(400, 60);
		ImGui::BeginChild("##buttons_area", buttons_area, false, ImGuiWindowFlags_NoScrollbar);

		for (int i = 0; i < 3; i++) {
			char label[32];
			snprintf(label, sizeof(label), "Button %d", i + 1);

			ImGuiID id = ImGui::GetID(label);
			ImVec2 size(110, 35);

			// Fixed position for each button
			float btn_x = i * 125.0f;
			ImGui::SetCursorPos(ImVec2(btn_x, 10));

			// Invisible button for interaction at fixed position
			bool clicked = ImGui::InvisibleButton(label, size);
			bool hovered = ImGui::IsItemHovered();
			bool active = ImGui::IsItemActive();

			// Animate properties
			auto ez = iam_ease_preset(iam_ease_out_cubic);
			float alpha = iam_tween_float(id, ImHashStr("alpha"), hovered ? 1.0f : 0.6f, 0.4f, ez, iam_policy_crossfade, dt);
			float scale = iam_tween_float(id, ImHashStr("scale"), hovered ? 1.08f : 1.0f, 0.3f,
				iam_ease_spring_desc(1.0f, 200.f, 15.f, 0.f), iam_policy_crossfade, dt);
			ImVec2 offset = iam_tween_vec2(id, ImHashStr("offset"), active ? ImVec2(0, 2) : ImVec2(0, 0), 0.15f, ez, iam_policy_crossfade, dt);
			ImVec4 color = iam_tween_color(id, ImHashStr("color"),
				hovered ? ImVec4(0.3f, 0.6f, 1.0f, 1.0f) : ImVec4(0.2f, 0.2f, 0.25f, 1.0f),
				0.4f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, iam_col_oklab, dt);

			// Calculate scaled button size (scale from center)
			ImVec2 scaled_size(size.x * scale, size.y * scale);
			ImVec2 size_diff((size.x - scaled_size.x) * 0.5f, (size.y - scaled_size.y) * 0.5f);

			// Draw at fixed position with offset
			ImGui::SetCursorPos(ImVec2(btn_x + offset.x + size_diff.x, 10 + offset.y + size_diff.y));
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
			ImGui::PushStyleColor(ImGuiCol_Button, color);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
			ImGui::PushID(i + 1000);
			ImGui::Button(label, scaled_size);
			ImGui::PopID();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar();

			(void)clicked;
		}

		ImGui::EndChild();
		ImGui::TreePop();
	}

	// Animated toggle
	if (ImGui::TreeNode("Animated Toggle")) {
		static bool toggle_state = false;

		ImGuiID id = ImHashStr("toggle_demo");
		ImVec2 toggle_size(60, 30);

		// Draw toggle background
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Animate background color
		ImVec4 bg_color = iam_tween_color(id, ImHashStr("bg"),
			toggle_state ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.35f, 1.0f),
			0.4f, iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, iam_col_oklab, dt);

		// Animate knob position
		float knob_x = iam_tween_float(id, ImHashStr("knob"),
			toggle_state ? toggle_size.x - 15.0f - 4.0f : 4.0f,
			0.5f, iam_ease_spring_desc(1.0f, 180.f, 18.f, 0.f), iam_policy_crossfade, dt);

		draw_list->AddRectFilled(pos, ImVec2(pos.x + toggle_size.x, pos.y + toggle_size.y),
			ImGui::ColorConvertFloat4ToU32(bg_color), toggle_size.y * 0.5f);
		draw_list->AddCircleFilled(ImVec2(pos.x + knob_x + 11, pos.y + toggle_size.y * 0.5f), 11.0f,
			IM_COL32(255, 255, 255, 255));

		// Invisible button for interaction
		if (ImGui::InvisibleButton("##toggle", toggle_size)) {
			toggle_state = !toggle_state;
		}

		ImGui::SameLine();
		ImGui::Text(toggle_state ? "ON" : "OFF");
		ImGui::TreePop();
	}

	// Hover card - larger
	if (ImGui::TreeNode("Hover Card")) {
		ImGuiID id = ImHashStr("card_demo");

		ImVec2 card_size(280, 140);
		ImVec2 pos = ImGui::GetCursorScreenPos();

		ImGui::InvisibleButton("##card", card_size);
		bool hovered = ImGui::IsItemHovered();

		// Animate elevation/shadow
		float elevation = iam_tween_float(id, ImHashStr("elevation"), hovered ? 16.0f : 4.0f, 0.4f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);
		float y_offset = iam_tween_float(id, ImHashStr("lift"), hovered ? -6.0f : 0.0f, 0.4f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Shadow
		ImVec2 shadow_pos(pos.x + elevation * 0.3f, pos.y + y_offset + elevation);
		draw_list->AddRectFilled(shadow_pos, ImVec2(shadow_pos.x + card_size.x, shadow_pos.y + card_size.y),
			IM_COL32(0, 0, 0, (int)(40 + elevation * 3)), 12.0f);

		// Card
		ImVec2 card_pos(pos.x, pos.y + y_offset);
		draw_list->AddRectFilled(card_pos, ImVec2(card_pos.x + card_size.x, card_pos.y + card_size.y),
			IM_COL32(60, 60, 70, 255), 12.0f);
		draw_list->AddRect(card_pos, ImVec2(card_pos.x + card_size.x, card_pos.y + card_size.y),
			IM_COL32(80, 80, 90, 255), 12.0f);

		// Text
		draw_list->AddText(ImVec2(card_pos.x + 20, card_pos.y + 20), IM_COL32(255, 255, 255, 255), "Hover Card");
		draw_list->AddText(ImVec2(card_pos.x + 20, card_pos.y + 50), IM_COL32(180, 180, 180, 255), "Hover to see lift effect");
		draw_list->AddText(ImVec2(card_pos.x + 20, card_pos.y + 80), IM_COL32(140, 140, 140, 255), "Shadow grows on hover");

		ImGui::TreePop();
	}
}

// ============================================================
// SECTION: Clip System
// ============================================================

// Clip IDs
static const ImGuiID CLIP_FADE_IN = 0x1001;
static const ImGuiID CLIP_BOUNCE = 0x1002;
static const ImGuiID CLIP_COLOR_CYCLE = 0x1003;
static const ImGuiID CLIP_COMPLEX = 0x1004;
static const ImGuiID CLIP_DELAYED = 0x1005;
static const ImGuiID CLIP_WITH_CALLBACKS = 0x1006;
static const ImGuiID CLIP_INT_ANIM = 0x1007;
static const ImGuiID CLIP_SEQUENTIAL = 0x1008;
static const ImGuiID CLIP_PARALLEL = 0x1009;
static const ImGuiID CLIP_STAGGER = 0x100A;
static const ImGuiID CLIP_STAGGER_LIST = 0x100B;
static const ImGuiID CLIP_STAGGER_GRID = 0x100C;
static const ImGuiID CLIP_STAGGER_CARDS = 0x100D;

// Channel IDs for clips
static const ImGuiID CLIP_CH_ALPHA = 0x2001;
static const ImGuiID CLIP_CH_SCALE = 0x2002;
static const ImGuiID CLIP_CH_OFFSET = 0x2003;
static const ImGuiID CLIP_CH_COLOR = 0x2004;
static const ImGuiID CLIP_CH_ROTATION = 0x2005;
static const ImGuiID CLIP_CH_COUNTER = 0x2006;
static const ImGuiID CLIP_CH_POS_X = 0x2007;
static const ImGuiID CLIP_CH_POS_Y = 0x2008;

// Callback state for demo
static int s_callback_begin_count = 0;
static int s_callback_update_count = 0;
static int s_callback_complete_count = 0;

static bool s_clips_initialized = false;

static void InitDemoClips()
{
	if (s_clips_initialized) return;
	s_clips_initialized = true;

	// Clip 1: Multi-keyframe fade with pulse
	iam_clip::begin(CLIP_FADE_IN)
		.key_float(CLIP_CH_ALPHA, 0.0f, 0.0f, iam_ease_out_cubic)
		.key_float(CLIP_CH_ALPHA, 0.5f, 1.0f, iam_ease_out_cubic)
		.key_float(CLIP_CH_ALPHA, 0.8f, 0.6f, iam_ease_in_out_sine)
		.key_float(CLIP_CH_ALPHA, 1.2f, 1.0f, iam_ease_out_cubic)
		.key_float(CLIP_CH_SCALE, 0.0f, 0.5f, iam_ease_out_back)
		.key_float(CLIP_CH_SCALE, 0.4f, 1.1f, iam_ease_out_back)
		.key_float(CLIP_CH_SCALE, 0.7f, 0.95f, iam_ease_in_out_sine)
		.key_float(CLIP_CH_SCALE, 1.0f, 1.0f, iam_ease_out_cubic)
		.end();

	// Clip 2: Bounce with multiple keyframes
	iam_spring_params spring = { 1.0f, 180.0f, 22.0f, 0.0f };  // Higher damping to prevent excessive scale overshoot
	iam_clip::begin(CLIP_BOUNCE)
		.key_vec2(CLIP_CH_OFFSET, 0.0f, ImVec2(0, -50), iam_ease_linear)
		.key_float(CLIP_CH_SCALE, 0.0f, 0.6f, iam_ease_linear)
		.key_float(CLIP_CH_ALPHA, 0.0f, 0.3f, iam_ease_linear)
		.key_vec2(CLIP_CH_OFFSET, 0.3f, ImVec2(0, 10), iam_ease_out_quad)
		.key_float(CLIP_CH_ALPHA, 0.3f, 1.0f, iam_ease_out_quad)
		.key_vec2(CLIP_CH_OFFSET, 0.5f, ImVec2(0, -15), iam_ease_out_quad)
		.key_vec2(CLIP_CH_OFFSET, 0.7f, ImVec2(0, 5), iam_ease_out_quad)
		.key_vec2(CLIP_CH_OFFSET, 0.9f, ImVec2(0, 0), iam_ease_out_bounce)
		.key_float_spring(CLIP_CH_SCALE, 0.3f, 1.0f, spring)
		.end();

	// Clip 3: Color cycle (looping)
	iam_clip::begin(CLIP_COLOR_CYCLE)
		.key_vec4(CLIP_CH_COLOR, 0.0f, ImVec4(1.0f, 0.3f, 0.3f, 1.0f), iam_ease_in_out_sine)
		.key_vec4(CLIP_CH_COLOR, 1.5f, ImVec4(1.0f, 1.0f, 0.3f, 1.0f), iam_ease_in_out_sine)
		.key_vec4(CLIP_CH_COLOR, 3.0f, ImVec4(0.3f, 1.0f, 0.3f, 1.0f), iam_ease_in_out_sine)
		.key_vec4(CLIP_CH_COLOR, 4.5f, ImVec4(0.3f, 1.0f, 1.0f, 1.0f), iam_ease_in_out_sine)
		.key_vec4(CLIP_CH_COLOR, 6.0f, ImVec4(0.3f, 0.3f, 1.0f, 1.0f), iam_ease_in_out_sine)
		.key_vec4(CLIP_CH_COLOR, 7.5f, ImVec4(1.0f, 0.3f, 1.0f, 1.0f), iam_ease_in_out_sine)
		.key_vec4(CLIP_CH_COLOR, 9.0f, ImVec4(1.0f, 0.3f, 0.3f, 1.0f), iam_ease_in_out_sine)
		.set_loop(true, iam_dir_normal, -1)
		.end();

	// Clip 4: Complex multi-channel animation (slower)
	iam_clip::begin(CLIP_COMPLEX)
		.key_float(CLIP_CH_ALPHA, 0.0f, 0.0f, iam_ease_out_cubic)
		.key_float(CLIP_CH_ALPHA, 0.8f, 1.0f, iam_ease_out_cubic)
		.key_float(CLIP_CH_SCALE, 0.0f, 0.3f, iam_ease_out_back)
		.key_float(CLIP_CH_SCALE, 0.6f, 1.15f, iam_ease_out_back)
		.key_float(CLIP_CH_SCALE, 1.2f, 1.0f, iam_ease_in_out_sine)
		.key_vec2(CLIP_CH_OFFSET, 0.0f, ImVec2(-80, 0), iam_ease_out_cubic)
		.key_vec2(CLIP_CH_OFFSET, 0.5f, ImVec2(10, 0), iam_ease_out_cubic)
		.key_vec2(CLIP_CH_OFFSET, 1.0f, ImVec2(0, 0), iam_ease_out_cubic)
		.key_vec4(CLIP_CH_COLOR, 0.0f, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), iam_ease_linear)
		.key_vec4(CLIP_CH_COLOR, 0.6f, ImVec4(1.0f, 0.8f, 0.3f, 1.0f), iam_ease_out_cubic)
		.key_vec4(CLIP_CH_COLOR, 1.5f, ImVec4(0.3f, 0.7f, 1.0f, 1.0f), iam_ease_out_cubic)
		.end();

	// Clip 5: Animation with delay
	iam_clip::begin(CLIP_DELAYED)
		.key_float(CLIP_CH_ALPHA, 0.0f, 0.0f, iam_ease_out_cubic)
		.key_float(CLIP_CH_ALPHA, 0.5f, 1.0f, iam_ease_out_cubic)
		.key_float(CLIP_CH_SCALE, 0.0f, 0.5f, iam_ease_out_back)
		.key_float(CLIP_CH_SCALE, 0.5f, 1.0f, iam_ease_out_back)
		.set_delay(1.0f)  // 1 second delay before animation starts
		.end();

	// Clip 6: Animation with callbacks
	iam_clip::begin(CLIP_WITH_CALLBACKS)
		.key_float(CLIP_CH_SCALE, 0.0f, 0.5f, iam_ease_out_cubic)
		.key_float(CLIP_CH_SCALE, 0.5f, 1.2f, iam_ease_out_back)
		.key_float(CLIP_CH_SCALE, 1.0f, 1.0f, iam_ease_in_out_sine)
		.on_begin([](ImGuiID, void*) { s_callback_begin_count++; })
		.on_update([](ImGuiID, void*) { s_callback_update_count++; })
		.on_complete([](ImGuiID, void*) { s_callback_complete_count++; })
		.end();

	// Clip 7: Integer keyframes (counter animation)
	iam_clip::begin(CLIP_INT_ANIM)
		.key_int(CLIP_CH_COUNTER, 0.0f, 0, iam_ease_linear)
		.key_int(CLIP_CH_COUNTER, 2.0f, 100, iam_ease_out_cubic)
		.end();

	// Clip 8: Sequential timeline (animations play one after another)
	// Total duration: 0.5 + 0.5 + 0.5 = 1.5s
	iam_clip::begin(CLIP_SEQUENTIAL)
		.seq_begin()
			// First: move right
			.key_float(CLIP_CH_POS_X, 0.0f, 0.0f, iam_ease_out_cubic)
			.key_float(CLIP_CH_POS_X, 0.5f, 100.0f, iam_ease_out_cubic)
		.seq_end()
		.seq_begin()
			// Then: move down
			.key_float(CLIP_CH_POS_Y, 0.0f, 0.0f, iam_ease_out_cubic)
			.key_float(CLIP_CH_POS_Y, 0.5f, 50.0f, iam_ease_out_cubic)
		.seq_end()
		.seq_begin()
			// Finally: scale up
			.key_float(CLIP_CH_SCALE, 0.0f, 1.0f, iam_ease_out_back)
			.key_float(CLIP_CH_SCALE, 0.5f, 1.5f, iam_ease_out_back)
		.seq_end()
		.end();

	// Clip 9: Parallel timeline (all animations start at the same time)
	// Total duration: max(0.6, 0.6, 0.6) = 0.6s
	iam_clip::begin(CLIP_PARALLEL)
		.par_begin()
			// All at once: move, scale, and fade
			.key_float(CLIP_CH_POS_X, 0.0f, 0.0f, iam_ease_out_cubic)
			.key_float(CLIP_CH_POS_X, 0.6f, 100.0f, iam_ease_out_cubic)
			.key_float(CLIP_CH_POS_Y, 0.0f, 0.0f, iam_ease_out_cubic)
			.key_float(CLIP_CH_POS_Y, 0.6f, 50.0f, iam_ease_out_cubic)
			.key_float(CLIP_CH_SCALE, 0.0f, 0.5f, iam_ease_out_elastic)
			.key_float(CLIP_CH_SCALE, 0.6f, 1.2f, iam_ease_out_elastic)
			.key_float(CLIP_CH_ALPHA, 0.0f, 0.0f, iam_ease_out_quad)
			.key_float(CLIP_CH_ALPHA, 0.6f, 1.0f, iam_ease_out_quad)
		.par_end()
		.end();

	// Clip 10: Stagger animation - cascading wave effect
	iam_clip::begin(CLIP_STAGGER)
		// Pop in from below with scale
		.key_float(CLIP_CH_POS_Y, 0.0f, 40.0f, iam_ease_out_back)
		.key_float(CLIP_CH_POS_Y, 0.5f, 0.0f, iam_ease_out_back)
		.key_float(CLIP_CH_SCALE, 0.0f, 0.0f, iam_ease_out_elastic)
		.key_float(CLIP_CH_SCALE, 0.6f, 1.0f, iam_ease_out_elastic)
		.key_float(CLIP_CH_ALPHA, 0.0f, 0.0f, iam_ease_out_quad)
		.key_float(CLIP_CH_ALPHA, 0.3f, 1.0f, iam_ease_out_quad)
		.set_stagger(12, 0.06f, 0.0f)  // 12 items, 60ms delay for smooth wave
		.end();

	// Clip 11: Stagger list items - slide in from left
	iam_clip::begin(CLIP_STAGGER_LIST)
		.key_float(CLIP_CH_POS_X, 0.0f, -50.0f, iam_ease_out_cubic)
		.key_float(CLIP_CH_POS_X, 0.4f, 0.0f, iam_ease_out_cubic)
		.key_float(CLIP_CH_ALPHA, 0.0f, 0.0f, iam_ease_out_quad)
		.key_float(CLIP_CH_ALPHA, 0.3f, 1.0f, iam_ease_out_quad)
		.set_stagger(6, 0.08f, 0.0f)
		.end();

	// Clip 12: Stagger grid - scale in with rotation feel
	iam_clip::begin(CLIP_STAGGER_GRID)
		.key_float(CLIP_CH_SCALE, 0.0f, 0.0f, iam_ease_out_back)
		.key_float(CLIP_CH_SCALE, 0.5f, 1.0f, iam_ease_out_back)
		.key_float(CLIP_CH_ALPHA, 0.0f, 0.0f, iam_ease_out_quad)
		.key_float(CLIP_CH_ALPHA, 0.25f, 1.0f, iam_ease_out_quad)
		.key_float(CLIP_CH_ROTATION, 0.0f, -15.0f, iam_ease_out_cubic)
		.key_float(CLIP_CH_ROTATION, 0.5f, 0.0f, iam_ease_out_cubic)
		.set_stagger(16, 0.04f, 0.0f)
		.end();

	// Clip 13: Stagger cards - drop from top with bounce
	iam_clip::begin(CLIP_STAGGER_CARDS)
		.key_float(CLIP_CH_POS_Y, 0.0f, -80.0f, iam_ease_out_bounce)
		.key_float(CLIP_CH_POS_Y, 0.6f, 0.0f, iam_ease_out_bounce)
		.key_float(CLIP_CH_ALPHA, 0.0f, 0.0f, iam_ease_out_quad)
		.key_float(CLIP_CH_ALPHA, 0.2f, 1.0f, iam_ease_out_quad)
		.key_float(CLIP_CH_SCALE, 0.0f, 0.8f, iam_ease_out_cubic)
		.key_float(CLIP_CH_SCALE, 0.4f, 1.0f, iam_ease_out_cubic)
		.set_stagger(5, 0.12f, 0.0f)
		.end();
}

static void ShowClipSystemDemo()
{
	float dt = GetSafeDeltaTime();
	InitDemoClips();

	ImGui::TextWrapped(
		"The Clip system provides timeline-based animations with multiple keyframes. "
		"Define clips once, then play them on instances with full playback control.");

	ImGui::Spacing();
	ImGui::Separator();

	// Basic clip playback
	if (ImGui::TreeNodeEx("Basic Playback", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Fade In with scale
		{
			static ImGuiID inst_id = ImHashStr("fade_inst");
			if (ImGui::Button("Play Fade+Scale")) {
				iam_play(CLIP_FADE_IN, inst_id);
			}
			ImGui::SameLine();

			iam_instance inst = iam_get_instance(inst_id);
			float alpha = 1.0f;
			float scale = 1.0f;
			if (inst.valid()) {
				inst.get_float(CLIP_CH_ALPHA, &alpha);
				inst.get_float(CLIP_CH_SCALE, &scale);
			}
			// Clamp scale to valid range for SetWindowFontScale
			if (scale < 0.1f) scale = 0.1f;
			if (scale > 10.0f) scale = 10.0f;

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
			ImGui::SetWindowFontScale(scale);
			ImGui::Text("Fading Text (a:%.2f s:%.2f)", alpha, scale);
			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopStyleVar();
		}

		ImGui::Spacing();

		// Bounce
		{
			static ImGuiID inst_id = ImHashStr("bounce_inst");
			if (ImGui::Button("Play Bounce")) {
				iam_play(CLIP_BOUNCE, inst_id);
			}
			ImGui::SameLine();

			iam_instance inst = iam_get_instance(inst_id);
			ImVec2 offset(0, 0);
			float scale = 1.0f;
			float alpha = 1.0f;
			if (inst.valid()) {
				inst.get_vec2(CLIP_CH_OFFSET, &offset);
				inst.get_float(CLIP_CH_SCALE, &scale);
				inst.get_float(CLIP_CH_ALPHA, &alpha);
			}
			// Clamp scale to valid range for SetWindowFontScale
			if (scale < 0.1f) scale = 0.1f;
			if (scale > 10.0f) scale = 10.0f;

			ImVec2 cur = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(cur.x + offset.x, cur.y + offset.y));
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
			ImGui::SetWindowFontScale(scale);
			ImGui::Text("Bouncing!");
			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopStyleVar();
		}

		ImGui::Spacing();

		// Complex
		{
			static ImGuiID inst_id = ImHashStr("complex_inst");
			if (ImGui::Button("Play Complex")) {
				iam_play(CLIP_COMPLEX, inst_id);
			}
			ImGui::SameLine();

			iam_instance inst = iam_get_instance(inst_id);
			float alpha = 1.0f;
			float scale = 1.0f;
			ImVec2 offset(0, 0);
			ImVec4 color(1, 1, 1, 1);
			if (inst.valid()) {
				inst.get_float(CLIP_CH_ALPHA, &alpha);
				inst.get_float(CLIP_CH_SCALE, &scale);
				inst.get_vec2(CLIP_CH_OFFSET, &offset);
				inst.get_vec4(CLIP_CH_COLOR, &color);
			}
			// Clamp scale to valid range for SetWindowFontScale
			if (scale < 0.1f) scale = 0.1f;
			if (scale > 10.0f) scale = 10.0f;

			ImVec2 cur = ImGui::GetCursorPos();
			ImGui::SetCursorPos(ImVec2(cur.x + offset.x, cur.y + offset.y));
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
			ImGui::SetWindowFontScale(scale);
			ImGui::TextColored(color, "Multi-channel Animation");
			ImGui::SetWindowFontScale(1.0f);
			ImGui::PopStyleVar();
		}

		ImGui::TreePop();
	}

	// Looping animations
	if (ImGui::TreeNode("Looping Animations")) {
		static ImGuiID inst_id = ImHashStr("loop_inst");
		static bool playing = false;

		if (!playing) {
			if (ImGui::Button("Start Color Cycle")) {
				iam_play(CLIP_COLOR_CYCLE, inst_id);
				playing = true;
			}
		} else {
			if (ImGui::Button("Stop")) {
				iam_instance inst = iam_get_instance(inst_id);
				if (inst.valid()) inst.stop();
				playing = false;
			}
		}

		ImGui::SameLine();

		iam_instance inst = iam_get_instance(inst_id);
		ImVec4 color(1, 1, 1, 1);
		float time = 0.0f;
		if (inst.valid()) {
			inst.get_vec4(CLIP_CH_COLOR, &color);
			time = inst.time();
		}

		// Draw as a larger square
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 square_size(100, 100);
		draw_list->AddRectFilled(pos, ImVec2(pos.x + square_size.x, pos.y + square_size.y),
			ImGui::ColorConvertFloat4ToU32(color), 8.0f);
		ImGui::Dummy(square_size);

		ImGui::SameLine();
		ImGui::Text("Time: %.2fs / 9.0s", time);

		ImGui::TreePop();
	}

	// Playback control
	if (ImGui::TreeNode("Playback Control")) {
		static ImGuiID inst_id = ImHashStr("control_inst");

		ImGui::Text("Controls:");
		if (ImGui::Button("Play")) iam_play(CLIP_COMPLEX, inst_id);
		ImGui::SameLine();

		iam_instance inst = iam_get_instance(inst_id);

		if (ImGui::Button("Pause")) { if (inst.valid()) inst.pause(); }
		ImGui::SameLine();
		if (ImGui::Button("Resume")) { if (inst.valid()) inst.resume(); }
		ImGui::SameLine();
		if (ImGui::Button("Stop")) { if (inst.valid()) inst.stop(); }

		// Seek slider
		float time = inst.valid() ? inst.time() : 0.0f;
		float duration = inst.valid() ? inst.duration() : 1.5f;
		if (ImGui::SliderFloat("Seek", &time, 0.0f, duration, "%.2f s")) {
			if (inst.valid()) inst.seek(time);
		}

		// Time scale (applied continuously so it works for new plays and during playback)
		static float time_scale = 1.0f;
		ImGui::SliderFloat("Time Scale", &time_scale, 0.1f, 3.0f);
		if (inst.valid()) inst.set_time_scale(time_scale);

		// Status
		ImGui::Text("Status: %s  Duration: %.2fs",
			inst.valid() ? (inst.is_playing() ? (inst.is_paused() ? "Paused" : "Playing") : "Stopped") : "Not started",
			duration);

		// Show current values
		if (inst.valid()) {
			float alpha = 0, scale = 0;
			ImVec2 offset(0, 0);
			ImVec4 color(0, 0, 0, 0);
			inst.get_float(CLIP_CH_ALPHA, &alpha);
			inst.get_float(CLIP_CH_SCALE, &scale);
			inst.get_vec2(CLIP_CH_OFFSET, &offset);
			inst.get_vec4(CLIP_CH_COLOR, &color);
			ImGui::Text("Values: alpha=%.2f scale=%.2f offset=(%.1f,%.1f)", alpha, scale, offset.x, offset.y);
		}

		ImGui::TreePop();
	}

	// Delayed playback
	if (ImGui::TreeNode("Delayed Playback")) {
		ImGui::TextWrapped("set_delay() adds a delay before the animation starts playing.");

		static ImGuiID inst_id = ImHashStr("delayed_inst");
		static float elapsed_since_play = 0.0f;
		static bool was_playing = false;

		if (ImGui::Button("Play (1s Delay)")) {
			iam_play(CLIP_DELAYED, inst_id);
			elapsed_since_play = 0.0f;
			was_playing = true;
		}

		iam_instance inst = iam_get_instance(inst_id);
		if (was_playing && inst.valid()) {
			elapsed_since_play += dt;
		}
		if (inst.valid() && !inst.is_playing()) {
			was_playing = false;
		}

		ImGui::SameLine();
		float alpha = 1.0f, scale = 1.0f;
		if (inst.valid()) {
			inst.get_float(CLIP_CH_ALPHA, &alpha);
			inst.get_float(CLIP_CH_SCALE, &scale);
		}
		if (scale < 0.1f) scale = 0.1f;
		if (scale > 10.0f) scale = 10.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
		ImGui::SetWindowFontScale(scale);
		ImGui::Text("Delayed Text");
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleVar();

		if (was_playing) {
			ImGui::Text("Elapsed: %.2fs (delay: 1.0s, anim starts after delay)", elapsed_since_play);
		}

		ImGui::TreePop();
	}

	// Callbacks demo
	if (ImGui::TreeNode("Callbacks")) {
		ImGui::TextWrapped(
			"on_begin(), on_update(), and on_complete() let you hook into animation lifecycle events.");

		static ImGuiID inst_id = ImHashStr("callback_inst");

		if (ImGui::Button("Play with Callbacks")) {
			iam_play(CLIP_WITH_CALLBACKS, inst_id);
		}

		ImGui::SameLine();
		if (ImGui::Button("Reset Counters")) {
			s_callback_begin_count = 0;
			s_callback_update_count = 0;
			s_callback_complete_count = 0;
		}

		iam_instance inst = iam_get_instance(inst_id);
		float scale = 1.0f;
		if (inst.valid()) {
			inst.get_float(CLIP_CH_SCALE, &scale);
		}
		if (scale < 0.1f) scale = 0.1f;
		if (scale > 10.0f) scale = 10.0f;

		ImGui::SameLine();
		ImGui::SetWindowFontScale(scale);
		ImGui::Text("Scaling");
		ImGui::SetWindowFontScale(1.0f);

		ImGui::Text("on_begin called:    %d times", s_callback_begin_count);
		ImGui::Text("on_update called:   %d times", s_callback_update_count);
		ImGui::Text("on_complete called: %d times", s_callback_complete_count);

		ImGui::TreePop();
	}

	// Integer keyframes demo
	if (ImGui::TreeNode("Integer Keyframes")) {
		ImGui::TextWrapped(
			"key_int() animates integer values (useful for counters, frame indices, etc.).");

		static ImGuiID inst_id = ImHashStr("int_inst");

		if (ImGui::Button("Count to 100")) {
			iam_play(CLIP_INT_ANIM, inst_id);
		}

		iam_instance inst = iam_get_instance(inst_id);
		int counter = 0;
		if (inst.valid()) {
			inst.get_int(CLIP_CH_COUNTER, &counter);
		}

		ImGui::SameLine();
		ImGui::Text("Counter: %d", counter);

		// Progress bar
		ImGui::ProgressBar(counter / 100.0f, ImVec2(-1, 0), "");

		ImGui::TreePop();
	}

	// Sequential Timeline demo
	if (ImGui::TreeNode("Sequential Timeline (seq_begin/end)")) {
		ImGui::TextWrapped(
			"seq_begin()/seq_end() groups keyframes that play in sequence. "
			"Each group starts after the previous one completes.");

		static ImGuiID inst_id = ImHashStr("seq_inst");
		if (ImGui::Button("Play Sequential")) {
			iam_play(CLIP_SEQUENTIAL, inst_id);
		}

		iam_instance inst = iam_get_instance(inst_id);
		float pos_x = 0.0f, pos_y = 0.0f, scale = 1.0f;
		if (inst.valid()) {
			inst.get_float(CLIP_CH_POS_X, &pos_x);
			inst.get_float(CLIP_CH_POS_Y, &pos_y);
			inst.get_float(CLIP_CH_SCALE, &scale);
		}
		if (scale < 0.1f) scale = 0.1f;
		if (scale > 10.0f) scale = 10.0f;

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(200, 100);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(40, 40, 45, 255));

		// Draw animated square
		float sq_size = 20.0f * scale;
		ImVec2 sq_pos(canvas_pos.x + 20 + pos_x, canvas_pos.y + 20 + pos_y);
		draw_list->AddRectFilled(sq_pos, ImVec2(sq_pos.x + sq_size, sq_pos.y + sq_size),
			IM_COL32(100, 200, 255, 255), 4.0f);

		ImGui::Dummy(canvas_size);
		ImGui::Text("Step 1: Move right | Step 2: Move down | Step 3: Scale up");
		ImGui::Text("X: %.1f  Y: %.1f  Scale: %.2f", pos_x, pos_y, scale);

		ImGui::TreePop();
	}

	// Parallel Timeline demo
	if (ImGui::TreeNode("Parallel Timeline (par_begin/end)")) {
		ImGui::TextWrapped(
			"par_begin()/par_end() groups keyframes that play simultaneously. "
			"All animations in the group start at the same time.");

		static ImGuiID inst_id = ImHashStr("par_inst");
		if (ImGui::Button("Play Parallel")) {
			iam_play(CLIP_PARALLEL, inst_id);
		}

		iam_instance inst = iam_get_instance(inst_id);
		float pos_x = 0.0f, pos_y = 0.0f, scale = 1.0f, alpha = 1.0f;
		if (inst.valid()) {
			inst.get_float(CLIP_CH_POS_X, &pos_x);
			inst.get_float(CLIP_CH_POS_Y, &pos_y);
			inst.get_float(CLIP_CH_SCALE, &scale);
			inst.get_float(CLIP_CH_ALPHA, &alpha);
		}
		if (scale < 0.1f) scale = 0.1f;
		if (scale > 10.0f) scale = 10.0f;

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(200, 100);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(40, 40, 45, 255));

		// Draw animated circle
		float radius = 15.0f * scale;
		ImVec2 circ_pos(canvas_pos.x + 30 + pos_x, canvas_pos.y + 30 + pos_y);
		int a = (int)(alpha * 255);
		draw_list->AddCircleFilled(circ_pos, radius, IM_COL32(255, 150, 100, a));

		ImGui::Dummy(canvas_size);
		ImGui::Text("All at once: Move + Scale + Fade");
		ImGui::Text("X: %.1f  Y: %.1f  Scale: %.2f  Alpha: %.2f", pos_x, pos_y, scale, alpha);

		ImGui::TreePop();
	}

	// Stagger demo
	if (ImGui::TreeNode("Stagger Animation")) {
		ImGui::TextWrapped(
			"set_stagger() applies progressive delays for animating multiple items. "
			"Each element pops in with a cascading wave effect.");

		static const int NUM_ITEMS = 12;
		static ImGuiID inst_ids[NUM_ITEMS];
		static bool initialized = false;
		if (!initialized) {
			for (int i = 0; i < NUM_ITEMS; i++) {
				char buf[32];
				snprintf(buf, sizeof(buf), "stagger_dot_%d", i);
				inst_ids[i] = ImHashStr(buf);
			}
			initialized = true;
		}

		if (ImGui::Button("Play Wave")) {
			for (int i = 0; i < NUM_ITEMS; i++) {
				iam_play_stagger(CLIP_STAGGER, inst_ids[i], i);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			for (int i = 0; i < NUM_ITEMS; i++) {
				iam_instance inst = iam_get_instance(inst_ids[i]);
				if (inst.valid()) inst.destroy();
			}
		}

		ImGui::Spacing();

		// Draw cascading circles with rainbow colors
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		float const canvas_w = 400.0f;
		float const canvas_h = 80.0f;
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + canvas_w, canvas_pos.y + canvas_h),
			IM_COL32(25, 25, 30, 255), 8.0f);

		float spacing = canvas_w / (NUM_ITEMS + 1);
		float base_y = canvas_pos.y + canvas_h * 0.5f;

		for (int i = 0; i < NUM_ITEMS; i++) {
			iam_instance inst = iam_get_instance(inst_ids[i]);
			float alpha = 0.0f, pos_y = 40.0f, scale = 0.0f;
			if (inst.valid()) {
				inst.get_float(CLIP_CH_ALPHA, &alpha);
				inst.get_float(CLIP_CH_POS_Y, &pos_y);
				inst.get_float(CLIP_CH_SCALE, &scale);
			}

			float x = canvas_pos.x + spacing * (i + 1);
			float y = base_y + pos_y;
			float radius = 12.0f * scale;

			// Rainbow hue based on index
			float hue = (float)i / NUM_ITEMS;
			ImVec4 col_hsv(hue, 0.8f, 0.9f, alpha);
			ImVec4 col_rgb;
			ImGui::ColorConvertHSVtoRGB(col_hsv.x, col_hsv.y, col_hsv.z, col_rgb.x, col_rgb.y, col_rgb.z);
			col_rgb.w = alpha;

			ImU32 col = ImGui::ColorConvertFloat4ToU32(col_rgb);

			if (radius > 0.5f) {
				// Glow effect
				draw_list->AddCircleFilled(ImVec2(x, y), radius * 1.5f,
					IM_COL32((int)(col_rgb.x*255), (int)(col_rgb.y*255), (int)(col_rgb.z*255), (int)(alpha * 40)));
				// Main circle
				draw_list->AddCircleFilled(ImVec2(x, y), radius, col);
				// Highlight
				draw_list->AddCircleFilled(ImVec2(x - radius*0.3f, y - radius*0.3f), radius * 0.25f,
					IM_COL32(255, 255, 255, (int)(alpha * 150)));
			}
		}

		ImGui::Dummy(ImVec2(canvas_w, canvas_h));

		ImGui::TreePop();
	}

	// Stagger List demo
	if (ImGui::TreeNode("Stagger: List Slide-In")) {
		ImGui::TextWrapped(
			"Classic list animation with items sliding in from the left.");

		static const int NUM_LIST_ITEMS = 6;
		static ImGuiID list_inst_ids[NUM_LIST_ITEMS];
		static bool list_initialized = false;
		if (!list_initialized) {
			for (int i = 0; i < NUM_LIST_ITEMS; i++) {
				char buf[32];
				snprintf(buf, sizeof(buf), "stagger_list_%d", i);
				list_inst_ids[i] = ImHashStr(buf);
			}
			list_initialized = true;
		}

		if (ImGui::Button("Play List")) {
			for (int i = 0; i < NUM_LIST_ITEMS; i++) {
				iam_play_stagger(CLIP_STAGGER_LIST, list_inst_ids[i], i);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset##list")) {
			for (int i = 0; i < NUM_LIST_ITEMS; i++) {
				iam_instance inst = iam_get_instance(list_inst_ids[i]);
				if (inst.valid()) inst.destroy();
			}
		}

		ImGui::Spacing();

		float const frame_h = ImGui::GetFrameHeight();
		float const item_spacing = 4.0f;
		float const padding = 8.0f;
		float const list_canvas_w = 250.0f;
		float const list_canvas_h = padding * 2 + NUM_LIST_ITEMS * frame_h + (NUM_LIST_ITEMS - 1) * item_spacing;

		ImVec2 list_canvas_pos = ImGui::GetCursorScreenPos();
		ImDrawList* list_draw_list = ImGui::GetWindowDrawList();

		list_draw_list->AddRectFilled(list_canvas_pos,
			ImVec2(list_canvas_pos.x + list_canvas_w, list_canvas_pos.y + list_canvas_h),
			IM_COL32(30, 32, 38, 255), 6.0f);

		const char* list_labels[] = { "Dashboard", "Projects", "Tasks", "Calendar", "Settings", "Help" };
		for (int i = 0; i < NUM_LIST_ITEMS; i++) {
			iam_instance inst = iam_get_instance(list_inst_ids[i]);
			float alpha = 0.0f, pos_x = -50.0f;
			if (inst.valid()) {
				inst.get_float(CLIP_CH_ALPHA, &alpha);
				inst.get_float(CLIP_CH_POS_X, &pos_x);
			}

			float y = list_canvas_pos.y + padding + i * (frame_h + item_spacing);
			int a = (int)(alpha * 255);

			// Draw list item background
			list_draw_list->AddRectFilled(
				ImVec2(list_canvas_pos.x + padding + pos_x, y),
				ImVec2(list_canvas_pos.x + list_canvas_w - padding + pos_x, y + frame_h),
				IM_COL32(50, 55, 65, a), 4.0f);

			// Draw icon placeholder
			list_draw_list->AddCircleFilled(
				ImVec2(list_canvas_pos.x + padding + 14 + pos_x, y + frame_h * 0.5f),
				6.0f, IM_COL32(100, 140, 200, a));

			// Draw label
			float text_y = y + (frame_h - ImGui::GetFontSize()) * 0.5f;
			list_draw_list->AddText(ImVec2(list_canvas_pos.x + padding + 28 + pos_x, text_y),
				IM_COL32(220, 220, 230, a), list_labels[i]);
		}

		ImGui::Dummy(ImVec2(list_canvas_w, list_canvas_h));
		ImGui::TreePop();
	}

	// Stagger Grid demo
	if (ImGui::TreeNode("Stagger: Grid Reveal")) {
		ImGui::TextWrapped(
			"Grid items appearing with scale and subtle rotation.");

		static const int GRID_COLS = 4;
		static const int GRID_ROWS = 4;
		static const int NUM_GRID_ITEMS = GRID_COLS * GRID_ROWS;
		static ImGuiID grid_inst_ids[NUM_GRID_ITEMS];
		static bool grid_initialized = false;
		if (!grid_initialized) {
			for (int i = 0; i < NUM_GRID_ITEMS; i++) {
				char buf[32];
				snprintf(buf, sizeof(buf), "stagger_grid_%d", i);
				grid_inst_ids[i] = ImHashStr(buf);
			}
			grid_initialized = true;
		}

		if (ImGui::Button("Play Grid")) {
			for (int i = 0; i < NUM_GRID_ITEMS; i++) {
				iam_play_stagger(CLIP_STAGGER_GRID, grid_inst_ids[i], i);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset##grid")) {
			for (int i = 0; i < NUM_GRID_ITEMS; i++) {
				iam_instance inst = iam_get_instance(grid_inst_ids[i]);
				if (inst.valid()) inst.destroy();
			}
		}

		ImGui::Spacing();

		ImVec2 grid_canvas_pos = ImGui::GetCursorScreenPos();
		float const cell_size = 45.0f;
		float const grid_spacing = 8.0f;
		float const grid_canvas_w = GRID_COLS * (cell_size + grid_spacing) + grid_spacing;
		float const grid_canvas_h = GRID_ROWS * (cell_size + grid_spacing) + grid_spacing;
		ImDrawList* grid_draw_list = ImGui::GetWindowDrawList();

		grid_draw_list->AddRectFilled(grid_canvas_pos,
			ImVec2(grid_canvas_pos.x + grid_canvas_w, grid_canvas_pos.y + grid_canvas_h),
			IM_COL32(25, 28, 35, 255), 8.0f);

		for (int row = 0; row < GRID_ROWS; row++) {
			for (int col = 0; col < GRID_COLS; col++) {
				int idx = row * GRID_COLS + col;
				iam_instance inst = iam_get_instance(grid_inst_ids[idx]);
				float alpha = 0.0f, scale = 0.0f, rotation = 0.0f;
				if (inst.valid()) {
					inst.get_float(CLIP_CH_ALPHA, &alpha);
					inst.get_float(CLIP_CH_SCALE, &scale);
					inst.get_float(CLIP_CH_ROTATION, &rotation);
				}

				float cx = grid_canvas_pos.x + grid_spacing + col * (cell_size + grid_spacing) + cell_size * 0.5f;
				float cy = grid_canvas_pos.y + grid_spacing + row * (cell_size + grid_spacing) + cell_size * 0.5f;

				int a = (int)(alpha * 255);
				float half = cell_size * 0.5f * scale;

				// Pastel colors based on position
				float hue = (float)(row * GRID_COLS + col) / NUM_GRID_ITEMS;
				ImVec4 col_hsv(hue, 0.5f, 0.85f, alpha);
				ImVec4 col_rgb;
				ImGui::ColorConvertHSVtoRGB(col_hsv.x, col_hsv.y, col_hsv.z, col_rgb.x, col_rgb.y, col_rgb.z);

				if (scale > 0.01f) {
					// Draw rotated rounded rect
					float rad = rotation * 3.14159f / 180.0f;
					ImVec2 corners[4];
					float corner_angles[4] = { -0.785f, 0.785f, 2.356f, 3.927f }; // 45, 135, 225, 315 degrees
					for (int c = 0; c < 4; c++) {
						float ca = corner_angles[c] + rad;
						float dist = half * 1.414f;
						corners[c] = ImVec2(cx + ImCos(ca) * dist, cy + ImSin(ca) * dist);
					}
					grid_draw_list->AddQuadFilled(corners[0], corners[1], corners[2], corners[3],
						IM_COL32((int)(col_rgb.x*255), (int)(col_rgb.y*255), (int)(col_rgb.z*255), a));
				}
			}
		}

		ImGui::Dummy(ImVec2(grid_canvas_w, grid_canvas_h));
		ImGui::TreePop();
	}

	// Stagger Cards demo
	if (ImGui::TreeNode("Stagger: Dropping Cards")) {
		ImGui::TextWrapped(
			"Cards dropping in from above with a bounce effect.");

		static const int NUM_CARDS = 5;
		static ImGuiID card_inst_ids[NUM_CARDS];
		static bool cards_initialized = false;
		if (!cards_initialized) {
			for (int i = 0; i < NUM_CARDS; i++) {
				char buf[32];
				snprintf(buf, sizeof(buf), "stagger_card_%d", i);
				card_inst_ids[i] = ImHashStr(buf);
			}
			cards_initialized = true;
		}

		if (ImGui::Button("Drop Cards")) {
			for (int i = 0; i < NUM_CARDS; i++) {
				iam_play_stagger(CLIP_STAGGER_CARDS, card_inst_ids[i], i);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset##cards")) {
			for (int i = 0; i < NUM_CARDS; i++) {
				iam_instance inst = iam_get_instance(card_inst_ids[i]);
				if (inst.valid()) inst.destroy();
			}
		}

		ImGui::Spacing();

		ImVec2 cards_canvas_pos = ImGui::GetCursorScreenPos();
		float const card_w = 70.0f;
		float const card_h = 90.0f;
		float const card_spacing = 12.0f;
		float const cards_canvas_w = NUM_CARDS * (card_w + card_spacing) + card_spacing;
		float const cards_canvas_h = card_h + 100.0f; // Extra space for drop animation
		ImDrawList* cards_draw_list = ImGui::GetWindowDrawList();

		cards_draw_list->AddRectFilled(cards_canvas_pos,
			ImVec2(cards_canvas_pos.x + cards_canvas_w, cards_canvas_pos.y + cards_canvas_h),
			IM_COL32(20, 25, 35, 255), 8.0f);

		// Card suits for visual interest
		const char* suits[] = { "A", "K", "Q", "J", "10" };
		ImU32 card_colors[] = {
			IM_COL32(220, 60, 60, 255),   // Red
			IM_COL32(40, 40, 50, 255),    // Black
			IM_COL32(220, 60, 60, 255),   // Red
			IM_COL32(40, 40, 50, 255),    // Black
			IM_COL32(220, 60, 60, 255),   // Red
		};

		for (int i = 0; i < NUM_CARDS; i++) {
			iam_instance inst = iam_get_instance(card_inst_ids[i]);
			float alpha = 0.0f, pos_y = -80.0f, scale = 0.8f;
			if (inst.valid()) {
				inst.get_float(CLIP_CH_ALPHA, &alpha);
				inst.get_float(CLIP_CH_POS_Y, &pos_y);
				inst.get_float(CLIP_CH_SCALE, &scale);
			}

			float x = cards_canvas_pos.x + card_spacing + i * (card_w + card_spacing);
			float y = cards_canvas_pos.y + 80.0f + pos_y;

			float scaled_w = card_w * scale;
			float scaled_h = card_h * scale;
			float offset_x = (card_w - scaled_w) * 0.5f;
			float offset_y = (card_h - scaled_h) * 0.5f;

			int a = (int)(alpha * 255);

			if (alpha > 0.01f) {
				// Card shadow
				cards_draw_list->AddRectFilled(
					ImVec2(x + offset_x + 3, y + offset_y + 3),
					ImVec2(x + offset_x + scaled_w + 3, y + offset_y + scaled_h + 3),
					IM_COL32(0, 0, 0, a / 3), 6.0f);

				// Card background
				cards_draw_list->AddRectFilled(
					ImVec2(x + offset_x, y + offset_y),
					ImVec2(x + offset_x + scaled_w, y + offset_y + scaled_h),
					IM_COL32(250, 250, 245, a), 6.0f);

				// Card border
				cards_draw_list->AddRect(
					ImVec2(x + offset_x, y + offset_y),
					ImVec2(x + offset_x + scaled_w, y + offset_y + scaled_h),
					IM_COL32(180, 180, 175, a), 6.0f, 0, 1.5f);

				// Suit text
				ImU32 text_col = (card_colors[i] & 0x00FFFFFF) | ((a & 0xFF) << 24);
				cards_draw_list->AddText(
					ImVec2(x + offset_x + 8, y + offset_y + 6),
					text_col, suits[i]);
			}
		}

		ImGui::Dummy(ImVec2(cards_canvas_w, cards_canvas_h));
		ImGui::TreePop();
	}

}

// ============================================================
// SECTION: Resize-Aware Helpers
// ============================================================
static void ShowResizeHelpersDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped(
		"When windows resize or dock, absolute positions become invalid. "
		"The resize-aware helpers use relative coordinates that adapt to container size changes.");

	ImGui::Spacing();

	if (ImGui::TreeNode("Relative Positioning")) {
		ImGui::TextWrapped("Position as percentage of container + pixel offset:");

		static ImVec2 percent(0.5f, 0.5f);
		static ImVec2 px_bias(0, 0);

		ImGui::SliderFloat2("Percent", &percent.x, 0.0f, 1.0f);
		ImGui::SliderFloat2("Pixel Bias", &px_bias.x, -50.0f, 50.0f);

		// Draw indicator in a fixed-size canvas
		ImVec2 origin = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(400, 200);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(origin, ImVec2(origin.x + canvas_size.x, origin.y + canvas_size.y), IM_COL32(40, 40, 45, 255));
		draw_list->AddRect(origin, ImVec2(origin.x + canvas_size.x, origin.y + canvas_size.y), IM_COL32(80, 80, 85, 255));

		// Calculate target position based on percentage of canvas
		ImVec2 target_pos(canvas_size.x * percent.x + px_bias.x, canvas_size.y * percent.y + px_bias.y);

		ImGuiID id = ImHashStr("rel_pos_demo");
		ImVec2 pos = iam_tween_vec2(id, 0, target_pos, 0.5f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Clamp position to stay within canvas
		float radius = 12.0f;
		float draw_x = ImClamp(pos.x, radius, canvas_size.x - radius);
		float draw_y = ImClamp(pos.y, radius, canvas_size.y - radius);

		draw_list->AddCircleFilled(ImVec2(origin.x + draw_x, origin.y + draw_y), radius, IM_COL32(100, 200, 255, 255));

		ImGui::Dummy(canvas_size);
		ImGui::Text("Position: (%.1f, %.1f)", pos.x, pos.y);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Anchor Spaces Showcase")) {
		ImGui::TextWrapped("Each anchor space measures from a different reference:");

		ImGui::Spacing();

		// Window Content anchor
		{
			ImGui::Text("window_content: GetContentRegionAvail()");
			ImVec2 content_size = ImGui::GetContentRegionAvail();
			content_size.y = 60;
			ImVec2 origin = ImGui::GetCursorScreenPos();

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(origin, ImVec2(origin.x + content_size.x, origin.y + content_size.y), IM_COL32(40, 50, 40, 255));
			draw_list->AddRect(origin, ImVec2(origin.x + content_size.x, origin.y + content_size.y), IM_COL32(80, 120, 80, 255));

			ImGuiID id = ImHashStr("anchor_content");
			ImVec2 pos = iam_tween_vec2_rel(id, 0, ImVec2(0.5f, 0.5f), ImVec2(0, 0), 0.5f,
				iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, iam_anchor_window_content, dt);

			// Draw dot clamped to this region
			float draw_x = ImClamp(pos.x, 10.0f, content_size.x - 10.0f);
			float draw_y = ImClamp(pos.y, 10.0f, content_size.y - 10.0f);
			draw_list->AddCircleFilled(ImVec2(origin.x + draw_x, origin.y + draw_y), 8.0f, IM_COL32(100, 255, 100, 255));
			draw_list->AddText(ImVec2(origin.x + 5, origin.y + 5), IM_COL32(180, 255, 180, 255), "Content Region");

			ImGui::Dummy(content_size);
		}

		ImGui::Spacing();

		// Window anchor
		{
			ImGui::Text("window: GetWindowSize()");
			ImVec2 win_size = ImGui::GetWindowSize();
			ImVec2 display_size(ImMin(win_size.x - 20, 400.0f), 60);
			ImVec2 origin = ImGui::GetCursorScreenPos();

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(origin, ImVec2(origin.x + display_size.x, origin.y + display_size.y), IM_COL32(40, 40, 50, 255));
			draw_list->AddRect(origin, ImVec2(origin.x + display_size.x, origin.y + display_size.y), IM_COL32(80, 80, 120, 255));

			ImGuiID id = ImHashStr("anchor_window");
			ImVec2 pos = iam_tween_vec2_rel(id, 0, ImVec2(0.5f, 0.5f), ImVec2(0, 0), 0.5f,
				iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, iam_anchor_window, dt);

			// Scale position to display size
			float scale_x = display_size.x / win_size.x;
			float scale_y = display_size.y / win_size.y;
			float draw_x = ImClamp(pos.x * scale_x, 10.0f, display_size.x - 10.0f);
			float draw_y = ImClamp(pos.y * scale_y, 10.0f, display_size.y - 10.0f);
			draw_list->AddCircleFilled(ImVec2(origin.x + draw_x, origin.y + draw_y), 8.0f, IM_COL32(100, 100, 255, 255));
			draw_list->AddText(ImVec2(origin.x + 5, origin.y + 5), IM_COL32(180, 180, 255, 255), "Window Size (scaled preview)");

			ImGui::Dummy(display_size);
			ImGui::Text("Actual window size: (%.0f, %.0f), Center pos: (%.1f, %.1f)", win_size.x, win_size.y, pos.x, pos.y);
		}

		ImGui::Spacing();

		// Viewport anchor
		{
			ImGui::Text("viewport: GetWindowViewport()->Size");
			ImVec2 vp_size;
#ifdef IMGUI_HAS_VIEWPORT
			vp_size = ImGui::GetWindowViewport()->Size;
#else
			vp_size = ImGui::GetIO().DisplaySize;
#endif
			ImVec2 display_size(ImMin(vp_size.x * 0.3f, 400.0f), 60);
			ImVec2 origin = ImGui::GetCursorScreenPos();

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(origin, ImVec2(origin.x + display_size.x, origin.y + display_size.y), IM_COL32(50, 40, 40, 255));
			draw_list->AddRect(origin, ImVec2(origin.x + display_size.x, origin.y + display_size.y), IM_COL32(120, 80, 80, 255));

			ImGuiID id = ImHashStr("anchor_viewport");
			ImVec2 pos = iam_tween_vec2_rel(id, 0, ImVec2(0.5f, 0.5f), ImVec2(0, 0), 0.5f,
				iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, iam_anchor_viewport, dt);

			// Scale position to display size
			float scale_x = display_size.x / vp_size.x;
			float scale_y = display_size.y / vp_size.y;
			float draw_x = ImClamp(pos.x * scale_x, 10.0f, display_size.x - 10.0f);
			float draw_y = ImClamp(pos.y * scale_y, 10.0f, display_size.y - 10.0f);
			draw_list->AddCircleFilled(ImVec2(origin.x + draw_x, origin.y + draw_y), 8.0f, IM_COL32(255, 100, 100, 255));
			draw_list->AddText(ImVec2(origin.x + 5, origin.y + 5), IM_COL32(255, 180, 180, 255), "Viewport Size (scaled preview)");

			ImGui::Dummy(display_size);
			ImGui::Text("Actual viewport size: (%.0f, %.0f), Center pos: (%.1f, %.1f)", vp_size.x, vp_size.y, pos.x, pos.y);
		}

		ImGui::Spacing();

		// Last item anchor
		{
			ImGui::Text("last_item: GetItemRectSize()");
			ImGui::Button("Reference Button", ImVec2(200, 40));
			ImVec2 item_size = ImGui::GetItemRectSize();

			ImVec2 origin = ImGui::GetCursorScreenPos();
			ImVec2 display_size(200, 40);

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(origin, ImVec2(origin.x + display_size.x, origin.y + display_size.y), IM_COL32(50, 50, 40, 255));
			draw_list->AddRect(origin, ImVec2(origin.x + display_size.x, origin.y + display_size.y), IM_COL32(120, 120, 80, 255));

			ImGuiID id = ImHashStr("anchor_item");
			ImVec2 pos = iam_tween_vec2_rel(id, 0, ImVec2(0.5f, 0.5f), ImVec2(0, 0), 0.5f,
				iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, iam_anchor_last_item, dt);

			// Scale position to display size
			float scale_x = display_size.x / item_size.x;
			float scale_y = display_size.y / item_size.y;
			float draw_x = ImClamp(pos.x * scale_x, 10.0f, display_size.x - 10.0f);
			float draw_y = ImClamp(pos.y * scale_y, 10.0f, display_size.y - 10.0f);
			draw_list->AddCircleFilled(ImVec2(origin.x + draw_x, origin.y + draw_y), 8.0f, IM_COL32(255, 255, 100, 255));
			draw_list->AddText(ImVec2(origin.x + 5, origin.y + 5), IM_COL32(255, 255, 180, 255), "Last Item Size");

			ImGui::Dummy(display_size);
			ImGui::Text("Button size: (%.0f, %.0f), Center pos: (%.1f, %.1f)", item_size.x, item_size.y, pos.x, pos.y);
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Resolver Callback")) {
		ImGui::TextWrapped(
			"iam_tween_vec2_resolved() uses a callback to compute the target position dynamically. "
			"Useful when the target depends on runtime state.");

		// Resolver function that calculates position based on time
		static float resolver_angle = 0.0f;
		resolver_angle += dt * 1.5f;

		// Store resolver data for the callback
		struct ResolverData {
			ImVec2 center;
			float radius;
			float angle;
		};
		static ResolverData rd;

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300, 150);
		rd.center = ImVec2(canvas_size.x * 0.5f, canvas_size.y * 0.5f);
		rd.radius = 50.0f;
		rd.angle = resolver_angle;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(40, 40, 45, 255));
		draw_list->AddRect(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(80, 80, 85, 255));

		// Draw orbit path
		draw_list->AddCircle(
			ImVec2(canvas_pos.x + rd.center.x, canvas_pos.y + rd.center.y),
			rd.radius, IM_COL32(60, 60, 80, 255), 32, 1.0f);

		// Resolver callback
		auto resolver = [](void* user) -> ImVec2 {
			ResolverData* data = (ResolverData*)user;
			return ImVec2(
				data->center.x + ImCos(data->angle) * data->radius,
				data->center.y + ImSin(data->angle) * data->radius
			);
		};

		ImGuiID id = ImHashStr("resolver_demo");
		ImVec2 pos = iam_tween_vec2_resolved(id, 0, resolver, &rd, 0.3f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Draw the animated dot (smoothly following the orbit)
		draw_list->AddCircleFilled(
			ImVec2(canvas_pos.x + pos.x, canvas_pos.y + pos.y),
			10.0f, IM_COL32(100, 200, 255, 255));

		// Draw the instant target position (without smoothing)
		ImVec2 instant = resolver(&rd);
		draw_list->AddCircle(
			ImVec2(canvas_pos.x + instant.x, canvas_pos.y + instant.y),
			12.0f, IM_COL32(255, 100, 100, 150), 12, 2.0f);

		ImGui::Dummy(canvas_size);
		ImGui::TextDisabled("Blue: smoothed position, Red circle: instant target");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Rebase Animation")) {
		ImGui::TextWrapped(
			"iam_rebase_vec2() allows changing the target of an in-progress animation "
			"without snapping or restarting. Useful for drag operations.");

		static ImVec2 target(150, 75);
		static bool dragging = false;

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300, 150);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(40, 45, 40, 255));
		draw_list->AddRect(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(80, 100, 80, 255));

		ImGui::InvisibleButton("rebase_canvas", canvas_size);
		bool hovered = ImGui::IsItemHovered();
		bool clicked = ImGui::IsItemClicked();

		if (clicked) {
			ImVec2 mouse = ImGui::GetMousePos();
			target = ImVec2(mouse.x - canvas_pos.x, mouse.y - canvas_pos.y);

			// Rebase the animation to the new target
			ImGuiID id = ImHashStr("rebase_demo");
			iam_rebase_vec2(id, 0, target, dt);
			dragging = true;
		}
		if (dragging && ImGui::IsMouseDown(0) && hovered) {
			ImVec2 mouse = ImGui::GetMousePos();
			target = ImVec2(mouse.x - canvas_pos.x, mouse.y - canvas_pos.y);
			ImGuiID id = ImHashStr("rebase_demo");
			iam_rebase_vec2(id, 0, target, dt);
		}
		if (ImGui::IsMouseReleased(0)) {
			dragging = false;
		}

		ImGuiID id = ImHashStr("rebase_demo");
		ImVec2 pos = iam_tween_vec2(id, 0, target, 0.4f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Draw target crosshair
		draw_list->AddLine(
			ImVec2(canvas_pos.x + target.x - 10, canvas_pos.y + target.y),
			ImVec2(canvas_pos.x + target.x + 10, canvas_pos.y + target.y),
			IM_COL32(255, 100, 100, 200), 1.0f);
		draw_list->AddLine(
			ImVec2(canvas_pos.x + target.x, canvas_pos.y + target.y - 10),
			ImVec2(canvas_pos.x + target.x, canvas_pos.y + target.y + 10),
			IM_COL32(255, 100, 100, 200), 1.0f);

		// Draw animated circle
		draw_list->AddCircleFilled(
			ImVec2(canvas_pos.x + pos.x, canvas_pos.y + pos.y),
			15.0f, IM_COL32(100, 255, 150, 255));

		ImGui::TextDisabled("Click anywhere to rebase the target");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Anchor Size Query")) {
		ImGui::TextWrapped(
			"anchor_size() returns the dimensions of each anchor space. "
			"Useful for manual calculations.");

		ImVec2 content = iam_anchor_size(iam_anchor_window_content);
		ImVec2 window = iam_anchor_size(iam_anchor_window);
		ImVec2 viewport = iam_anchor_size(iam_anchor_viewport);
		ImVec2 last_item = iam_anchor_size(iam_anchor_last_item);

		ImGui::Text("Content Region: %.0f x %.0f", content.x, content.y);
		ImGui::Text("Window Size:    %.0f x %.0f", window.x, window.y);
		ImGui::Text("Viewport Size:  %.0f x %.0f", viewport.x, viewport.y);
		ImGui::Text("Last Item Size: %.0f x %.0f", last_item.x, last_item.y);

		ImGui::TreePop();
	}
}

// ============================================================
// SECTION: Layering System
// ============================================================

// Layering clip IDs and channels
static const ImGuiID LAYER_CLIP_A = 0x3001;
static const ImGuiID LAYER_CLIP_B = 0x3002;
static const ImGuiID LAYER_CLIP_C = 0x3003;
static const ImGuiID LAYER_CH_X = 0x3101;

static bool s_layer_clips_initialized = false;

static void InitLayerClips()
{
	if (s_layer_clips_initialized) return;
	s_layer_clips_initialized = true;

	// Animation A: moves right slowly
	iam_clip::begin(LAYER_CLIP_A)
		.key_float(LAYER_CH_X, 0.0f, 0.0f, iam_ease_in_out_sine)
		.key_float(LAYER_CH_X, 2.0f, 200.0f, iam_ease_in_out_sine)
		.set_loop(true, iam_dir_alternate)
		.end();

	// Animation B: moves left faster
	iam_clip::begin(LAYER_CLIP_B)
		.key_float(LAYER_CH_X, 0.0f, 200.0f, iam_ease_in_out_cubic)
		.key_float(LAYER_CH_X, 1.5f, 0.0f, iam_ease_in_out_cubic)
		.set_loop(true, iam_dir_alternate)
		.end();

	// Animation C: bouncy center pulse
	iam_clip::begin(LAYER_CLIP_C)
		.key_float(LAYER_CH_X, 0.0f, 100.0f, iam_ease_out_elastic)
		.key_float(LAYER_CH_X, 0.8f, 50.0f, iam_ease_in_out_quad)
		.key_float(LAYER_CH_X, 1.6f, 150.0f, iam_ease_in_out_quad)
		.key_float(LAYER_CH_X, 2.4f, 100.0f, iam_ease_out_bounce)
		.set_loop(true, iam_dir_normal)
		.end();
}

static void ShowLayeringDemo()
{
	float dt = GetSafeDeltaTime();
	(void)dt;
	InitLayerClips();

	ImGui::TextWrapped(
		"The layering system allows blending multiple animation instances together. "
		"Use layer_begin/layer_add/layer_end to combine animations with weights.");

	ImGui::Spacing();

	if (ImGui::TreeNode("Basic Layer Blending (3 Layers)")) {
		ImGui::TextWrapped(
			"Three animations move dots with different patterns. "
			"Adjust the weight sliders to blend between them.");

		static ImGuiID inst_a = ImHashStr("layer_inst_a");
		static ImGuiID inst_b = ImHashStr("layer_inst_b");
		static ImGuiID inst_c = ImHashStr("layer_inst_c");
		static float weight_a = 0.33f;
		static float weight_b = 0.33f;
		static float weight_c = 0.34f;
		static bool playing = false;

		if (!playing) {
			if (ImGui::Button("Start Animations")) {
				iam_play(LAYER_CLIP_A, inst_a);
				iam_play(LAYER_CLIP_B, inst_b);
				iam_play(LAYER_CLIP_C, inst_c);
				playing = true;
			}
		} else {
			if (ImGui::Button("Stop")) {
				iam_instance a = iam_get_instance(inst_a);
				iam_instance b = iam_get_instance(inst_b);
				iam_instance c = iam_get_instance(inst_c);
				if (a.valid()) a.stop();
				if (b.valid()) b.stop();
				if (c.valid()) c.stop();
				playing = false;
			}
		}

		ImGui::SliderFloat("Weight A", &weight_a, 0.0f, 1.0f);
		ImGui::SliderFloat("Weight B", &weight_b, 0.0f, 1.0f);
		ImGui::SliderFloat("Weight C", &weight_c, 0.0f, 1.0f);

		iam_instance a = iam_get_instance(inst_a);
		iam_instance b = iam_get_instance(inst_b);
		iam_instance c = iam_get_instance(inst_c);

		// Get individual values for visualization
		float x_a = 100.0f, x_b = 100.0f, x_c = 100.0f;
		if (a.valid()) a.get_float(LAYER_CH_X, &x_a);
		if (b.valid()) b.get_float(LAYER_CH_X, &x_b);
		if (c.valid()) c.get_float(LAYER_CH_X, &x_c);

		// Use the layering API to blend animations
		static ImGuiID composite_id = ImHashStr("layer_composite");
		iam_layer_begin(composite_id);
		if (a.valid()) iam_layer_add(a, weight_a);
		if (b.valid()) iam_layer_add(b, weight_b);
		if (c.valid()) iam_layer_add(c, weight_c);
		iam_layer_end(composite_id);

		float blended_x = 100.0f;
		iam_get_blended_float(composite_id, LAYER_CH_X, &blended_x);

		// Draw visualization
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		float const vis_width = 250.0f;
		float const text_width = 120.0f;
		float const row_height = 35.0f;
		ImVec2 canvas_size(vis_width, row_height * 4 + 20.0f);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + vis_width, canvas_pos.y + canvas_size.y),
			IM_COL32(40, 40, 45, 255));
		draw_list->AddRect(canvas_pos,
			ImVec2(canvas_pos.x + vis_width, canvas_pos.y + canvas_size.y),
			IM_COL32(80, 80, 85, 255));

		// Draw individual positions (faded) with labels on right
		float y_row = canvas_pos.y + row_height * 0.5f + 10.0f;
		draw_list->AddCircleFilled(ImVec2(canvas_pos.x + 25 + x_a * 0.5f, y_row), 8.0f, IM_COL32(255, 100, 100, 100));
		draw_list->AddText(ImVec2(canvas_pos.x + vis_width + 10, y_row - 6), IM_COL32(255, 100, 100, 200), "A (right)");

		y_row = canvas_pos.y + row_height * 1.5f + 10.0f;
		draw_list->AddCircleFilled(ImVec2(canvas_pos.x + 25 + x_b * 0.5f, y_row), 8.0f, IM_COL32(100, 100, 255, 100));
		draw_list->AddText(ImVec2(canvas_pos.x + vis_width + 10, y_row - 6), IM_COL32(100, 100, 255, 200), "B (left)");

		y_row = canvas_pos.y + row_height * 2.5f + 10.0f;
		draw_list->AddCircleFilled(ImVec2(canvas_pos.x + 25 + x_c * 0.5f, y_row), 8.0f, IM_COL32(255, 200, 100, 100));
		draw_list->AddText(ImVec2(canvas_pos.x + vis_width + 10, y_row - 6), IM_COL32(255, 200, 100, 200), "C (bouncy)");

		// Draw blended position (solid)
		y_row = canvas_pos.y + row_height * 3.5f + 10.0f;
		draw_list->AddCircleFilled(ImVec2(canvas_pos.x + 25 + blended_x * 0.5f, y_row), 10.0f, IM_COL32(100, 255, 100, 255));
		draw_list->AddText(ImVec2(canvas_pos.x + vis_width + 10, y_row - 6), IM_COL32(100, 255, 100, 255), "Blended");

		ImGui::Dummy(ImVec2(vis_width + text_width, canvas_size.y));
		// Calculate normalized weights for display
		float total = weight_a + weight_b + weight_c;
		if (total < 0.001f) total = 1.0f;
		ImGui::Text("Weights: A=%.0f%% B=%.0f%% C=%.0f%%", (weight_a/total) * 100, (weight_b/total) * 100, (weight_c/total) * 100);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Instance Weights")) {
		ImGui::TextWrapped(
			"set_weight() on an instance controls its contribution when used with the layering API.");

		static ImGuiID inst_id = ImHashStr("weight_inst");
		static float weight = 1.0f;

		if (ImGui::Button("Play##weight")) {
			iam_play(LAYER_CLIP_A, inst_id);
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		ImGui::SliderFloat("iam_instanceWeight", &weight, 0.0f, 2.0f);

		iam_instance inst = iam_get_instance(inst_id);
		if (inst.valid()) {
			inst.set_weight(weight);
		}

		float x = 0.0f;
		if (inst.valid()) inst.get_float(LAYER_CH_X, &x);

		// Apply weight to the position for visualization
		float weighted_x = x * weight;

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300, 50);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(40, 40, 45, 255));

		// Original position
		draw_list->AddCircle(
			ImVec2(canvas_pos.x + 50 + x * 0.5f, canvas_pos.y + 25),
			10.0f, IM_COL32(255, 255, 255, 100), 12, 1.0f);

		// Weighted position
		draw_list->AddCircleFilled(
			ImVec2(canvas_pos.x + 50 + weighted_x * 0.5f, canvas_pos.y + 25),
			8.0f, IM_COL32(255, 200, 100, 255));

		ImGui::Dummy(canvas_size);
		ImGui::Text("Original: %.1f, Weighted (x%.1f): %.1f", x, weight, weighted_x);

		ImGui::TreePop();
	}
}

// ============================================================
// SECTION: ImDrawList Animations
// ============================================================
static void ShowDrawListDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped(
		"Custom drawing with ImDrawList can be animated using tweens for smooth, "
		"professional visual effects.");

	ImGui::Spacing();
	ImGui::Separator();

	// Rotating 3D Cube
	if (ImGui::TreeNodeEx("3D Rotating Cube", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::TextDisabled("Wireframe cube with animated rotation");

		static float angle_x = 0.0f;
		static float angle_y = 0.0f;
		static float angle_z = 0.0f;
		static bool auto_rotate = true;

		ImGui::Checkbox("Auto Rotate", &auto_rotate);
		if (!auto_rotate) {
			ImGui::SliderFloat("X Rotation", &angle_x, 0.0f, 6.28f);
			ImGui::SliderFloat("Y Rotation", &angle_y, 0.0f, 6.28f);
			ImGui::SliderFloat("Z Rotation", &angle_z, 0.0f, 6.28f);
		} else {
			angle_x += dt * 0.7f;
			angle_y += dt * 1.1f;
			angle_z += dt * 0.3f;
		}

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(250, 200);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Background
		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(20, 20, 30, 255));

		ImVec2 center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);
		float cube_size = 60.0f;

		// Define cube vertices (8 corners)
		float vertices[8][3] = {
			{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},  // Back face
			{-1, -1,  1}, {1, -1,  1}, {1, 1,  1}, {-1, 1,  1}   // Front face
		};

		// Rotate and project vertices
		ImVec2 projected[8];
		for (int i = 0; i < 8; ++i) {
			float x = vertices[i][0], y = vertices[i][1], z = vertices[i][2];

			// Rotate around X
			float y1 = y * ImCos(angle_x) - z * ImSin(angle_x);
			float z1 = y * ImSin(angle_x) + z * ImCos(angle_x);
			y = y1; z = z1;

			// Rotate around Y
			float x1 = x * ImCos(angle_y) + z * ImSin(angle_y);
			z1 = -x * ImSin(angle_y) + z * ImCos(angle_y);
			x = x1; z = z1;

			// Rotate around Z
			x1 = x * ImCos(angle_z) - y * ImSin(angle_z);
			y1 = x * ImSin(angle_z) + y * ImCos(angle_z);
			x = x1; y = y1;

			// Simple perspective projection
			float perspective = 3.0f / (3.0f + z);
			projected[i] = ImVec2(center.x + x * cube_size * perspective,
								  center.y + y * cube_size * perspective);
		}

		// Draw edges with depth-based color
		int edges[12][2] = {
			{0,1},{1,2},{2,3},{3,0},  // Back face
			{4,5},{5,6},{6,7},{7,4},  // Front face
			{0,4},{1,5},{2,6},{3,7}   // Connecting edges
		};

		for (int i = 0; i < 12; ++i) {
			int v0 = edges[i][0], v1 = edges[i][1];
			// Color based on average Z depth
			float z0 = vertices[v0][2], z1 = vertices[v1][2];
			float avg_z = (z0 + z1) * 0.5f;
			int brightness = (int)(180 + avg_z * 50);
			if (brightness < 80) brightness = 80;
			if (brightness > 255) brightness = 255;
			draw_list->AddLine(projected[v0], projected[v1],
				IM_COL32(brightness, brightness/2, brightness, 255), 2.0f);
		}

		// Draw vertices as small dots
		for (int i = 0; i < 8; ++i) {
			float z = vertices[i][2];
			int brightness = (int)(200 + z * 40);
			if (brightness < 100) brightness = 100;
			if (brightness > 255) brightness = 255;
			draw_list->AddCircleFilled(projected[i], 4.0f, IM_COL32(100, brightness, 255, 255));
		}

		ImGui::Dummy(canvas_size);
		ImGui::TreePop();
	}

	ImGui::Spacing();

	// Pulsing Rings
	if (ImGui::TreeNode("Pulsing Rings")) {
		ImGui::TextDisabled("Animated expanding rings with easing");

		static float ring_time = 0.0f;
		ring_time += dt;

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(250, 150);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(15, 15, 25, 255));

		ImVec2 center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

		// Draw multiple rings at different phases
		for (int i = 0; i < 4; ++i) {
			float phase = ImFmod(ring_time + i * 0.5f, 2.0f);
			float t = phase / 2.0f;
			float eased = iam_eval_preset(iam_ease_out_cubic, t);
			float radius = 10.0f + eased * 60.0f;
			int alpha = (int)((1.0f - t) * 200);
			if (alpha < 0) alpha = 0;

			ImU32 col = IM_COL32(100, 150, 255, alpha);
			draw_list->AddCircle(center, radius, col, 0, 2.0f);
		}

		// Center dot
		draw_list->AddCircleFilled(center, 6.0f, IM_COL32(100, 200, 255, 255));

		ImGui::Dummy(canvas_size);
		ImGui::TreePop();
	}

	ImGui::Spacing();

	// Orbiting Particles
	if (ImGui::TreeNode("Orbiting Particles")) {
		ImGui::TextDisabled("Particles following elliptical paths");

		static float orbit_time = 0.0f;
		orbit_time += dt;

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(280, 160);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(10, 15, 25, 255));

		ImVec2 center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

		// Draw orbit paths (ellipses)
		for (int orbit = 0; orbit < 3; ++orbit) {
			float rx = 40.0f + orbit * 30.0f;
			float ry = 25.0f + orbit * 18.0f;
			draw_list->AddEllipse(center, ImVec2(rx, ry), IM_COL32(60, 60, 80, 100), 0.0f, 32, 1.0f);
		}

		// Draw orbiting particles
		int num_particles = 8;
		for (int i = 0; i < num_particles; ++i) {
			int orbit = i % 3;
			float rx = 40.0f + orbit * 30.0f;
			float ry = 25.0f + orbit * 18.0f;
			float speed = 1.0f - orbit * 0.2f;
			float angle = orbit_time * speed + i * 0.8f;

			float px = center.x + ImCos(angle) * rx;
			float py = center.y + ImSin(angle) * ry;

			// Color based on orbit
			ImU32 colors[] = {
				IM_COL32(255, 100, 100, 255),
				IM_COL32(100, 255, 100, 255),
				IM_COL32(100, 100, 255, 255)
			};

			// Draw trail
			for (int t = 1; t <= 5; ++t) {
				float trail_angle = angle - t * 0.08f;
				float tx = center.x + ImCos(trail_angle) * rx;
				float ty = center.y + ImSin(trail_angle) * ry;
				int alpha = 150 - t * 25;
				draw_list->AddCircleFilled(ImVec2(tx, ty), 3.0f - t * 0.4f,
					(colors[orbit] & 0x00FFFFFF) | ((alpha & 0xFF) << 24));
			}

			draw_list->AddCircleFilled(ImVec2(px, py), 5.0f, colors[orbit]);
		}

		// Center "sun"
		draw_list->AddCircleFilled(center, 12.0f, IM_COL32(255, 200, 100, 255));
		draw_list->AddCircleFilled(center, 8.0f, IM_COL32(255, 255, 200, 255));

		ImGui::Dummy(canvas_size);
		ImGui::TreePop();
	}

	ImGui::Spacing();

	// Bouncing Ball with Trail
	if (ImGui::TreeNode("Bouncing Ball")) {
		ImGui::TextDisabled("Ball bouncing with motion trail");

		static float ball_time = 0.0f;
		ball_time += dt;

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300, 120);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(25, 20, 30, 255));

		// Animate horizontal position (slower, smoother)
		float cycle = ImFmod(ball_time * 0.4f, 1.0f);
		float x_t = cycle * 2.0f;
		if (x_t > 1.0f) x_t = 2.0f - x_t;
		float ball_x = canvas_pos.x + 30.0f + x_t * (canvas_size.x - 60.0f);

		// Bounce animation (vertical) - use sine for smooth arc
		float bounce_cycle = ImFmod(ball_time * 0.8f, 1.0f);
		float bounce_eased = ImSin(bounce_cycle * 3.14159f);  // Simple sine arc
		float ball_y = canvas_pos.y + canvas_size.y - 20.0f - bounce_eased * 70.0f;

		// Draw shadow
		float shadow_scale = 0.3f + 0.7f * (1.0f - bounce_eased);
		draw_list->AddEllipseFilled(ImVec2(ball_x, canvas_pos.y + canvas_size.y - 10.0f),
			ImVec2(15.0f * shadow_scale, 5.0f * shadow_scale),
			IM_COL32(0, 0, 0, (int)(100 * shadow_scale)));

		// Draw trail (fewer, more spaced out)
		for (int t = 1; t <= 5; ++t) {
			float trail_time = ball_time - t * 0.04f;
			float trail_cycle = ImFmod(trail_time * 0.4f, 1.0f);
			float trail_x_t = trail_cycle * 2.0f;
			if (trail_x_t > 1.0f) trail_x_t = 2.0f - trail_x_t;
			float trail_x = canvas_pos.x + 30.0f + trail_x_t * (canvas_size.x - 60.0f);

			float trail_bounce_cycle = ImFmod(trail_time * 0.8f, 1.0f);
			float trail_bounce = ImSin(trail_bounce_cycle * 3.14159f);
			float trail_y = canvas_pos.y + canvas_size.y - 20.0f - trail_bounce * 70.0f;

			int alpha = 100 - t * 18;
			if (alpha > 0) {
				draw_list->AddCircleFilled(ImVec2(trail_x, trail_y), 10.0f - t * 1.5f,
					IM_COL32(255, 150, 50, alpha));
			}
		}

		// Draw ball
		draw_list->AddCircleFilled(ImVec2(ball_x, ball_y), 12.0f, IM_COL32(255, 180, 80, 255));
		draw_list->AddCircleFilled(ImVec2(ball_x - 3, ball_y - 3), 4.0f, IM_COL32(255, 255, 200, 200));

		// Ground line
		draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + canvas_size.y - 5),
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y - 5),
			IM_COL32(80, 80, 100, 255), 2.0f);

		ImGui::Dummy(canvas_size);
		ImGui::TreePop();
	}

	ImGui::Spacing();

	// Morphing Shape (animated interpolation between shapes)
	if (ImGui::TreeNode("Morphing Shape")) {
		ImGui::TextDisabled("Smooth interpolation between different shapes");

		static float morph_time = 0.0f;
		static int morph_ease = iam_ease_out_elastic;
		morph_time += dt * 0.3f;

		const char* ease_names[] = {
			"Linear", "InOutCubic", "OutBack", "OutElastic", "OutBounce", "InOutSine"
		};
		int ease_types[] = {
			iam_ease_linear, iam_ease_in_out_cubic, iam_ease_out_back,
			iam_ease_out_elastic, iam_ease_out_bounce, iam_ease_in_out_sine
		};
		static int ease_idx = 3;  // OutElastic by default
		ImGui::SetNextItemWidth(120);
		if (ImGui::Combo("Easing", &ease_idx, ease_names, IM_ARRAYSIZE(ease_names))) {
			morph_ease = ease_types[ease_idx];
		}

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(220, 180);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(20, 20, 35, 255));

		ImVec2 center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

		// Cycle through shapes: circle -> triangle -> square -> pentagon -> circle
		float cycle = ImFmod(morph_time, 4.0f);
		int shape_from = (int)cycle;
		int shape_to = (shape_from + 1) % 4;
		float t = cycle - shape_from;
		float eased_t = iam_eval_preset(morph_ease, t);

		// Generate points for each shape
		auto get_shape_point = [](int shape, int point_idx, int num_points, float radius) -> ImVec2 {
			if (shape == 0) {  // Circle
				float angle = (float)point_idx / num_points * 2.0f * IM_PI - IM_PI * 0.5f;
				return ImVec2(ImCos(angle) * radius, ImSin(angle) * radius);
			} else {
				int sides = shape + 2;  // 3, 4, 5 sides
				float angle = (float)(point_idx % sides) / sides * 2.0f * IM_PI - IM_PI * 0.5f;
				return ImVec2(ImCos(angle) * radius, ImSin(angle) * radius);
			}
		};

		// Draw morphing polygon
		const int num_points = 60;
		const float radius = 60.0f;
		ImVec2 points[num_points];

		for (int i = 0; i < num_points; ++i) {
			float angle_norm = (float)i / num_points;

			// Calculate position on each shape
			auto shape_pos = [&](int shape) -> ImVec2 {
				if (shape == 0) {  // Circle
					float a = angle_norm * 2.0f * IM_PI - IM_PI * 0.5f;
					return ImVec2(ImCos(a), ImSin(a));
				} else {
					int sides = shape + 2;
					float segment = 1.0f / sides;
					int seg_idx = (int)(angle_norm / segment);
					float seg_t = ImFmod(angle_norm, segment) / segment;
					float a0 = (float)seg_idx / sides * 2.0f * IM_PI - IM_PI * 0.5f;
					float a1 = (float)(seg_idx + 1) / sides * 2.0f * IM_PI - IM_PI * 0.5f;
					return ImVec2(
						ImCos(a0) * (1 - seg_t) + ImCos(a1) * seg_t,
						ImSin(a0) * (1 - seg_t) + ImSin(a1) * seg_t
					);
				}
			};

			ImVec2 p0 = shape_pos(shape_from);
			ImVec2 p1 = shape_pos(shape_to);

			points[i] = ImVec2(
				center.x + (p0.x * (1 - eased_t) + p1.x * eased_t) * radius,
				center.y + (p0.y * (1 - eased_t) + p1.y * eased_t) * radius
			);
		}

		// Color based on current shape
		ImU32 colors[] = {
			IM_COL32(255, 100, 150, 255), // Circle: pink
			IM_COL32(100, 255, 150, 255), // Triangle: green
			IM_COL32(100, 150, 255, 255), // Square: blue
			IM_COL32(255, 200, 100, 255)  // Pentagon: orange
		};
		ImU32 col_from = colors[shape_from];
		ImU32 col_to = colors[shape_to];

		// Blend colors
		int r = (int)((col_from & 0xFF) * (1 - eased_t) + (col_to & 0xFF) * eased_t);
		int g = (int)(((col_from >> 8) & 0xFF) * (1 - eased_t) + ((col_to >> 8) & 0xFF) * eased_t);
		int b = (int)(((col_from >> 16) & 0xFF) * (1 - eased_t) + ((col_to >> 16) & 0xFF) * eased_t);
		ImU32 blended = IM_COL32(r, g, b, 255);

		draw_list->AddConvexPolyFilled(points, num_points, (blended & 0x00FFFFFF) | 0x40000000);
		draw_list->AddPolyline(points, num_points, blended, ImDrawFlags_Closed, 2.5f);

		const char* shape_names[] = {"Circle", "Triangle", "Square", "Pentagon"};
		ImGui::Dummy(canvas_size);
		ImGui::Text("%s -> %s (%.0f%%)", shape_names[shape_from], shape_names[shape_to], eased_t * 100);

		ImGui::TreePop();
	}
}

// ============================================================
// OSCILLATORS DEMO
// ============================================================
static void ShowOscillatorsDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped("Oscillators provide continuous periodic animations without managing state. "
		"Four wave types available: sine, triangle, sawtooth, and square.");

	// Wave type selector
	static int wave_type = iam_wave_sine;
	ImGui::Combo("Wave Type", &wave_type, "Sine\0Triangle\0Sawtooth\0Square\0");

	static float frequency = 1.0f;
	static float amplitude = 50.0f;
	ImGui::SliderFloat("Frequency", &frequency, 0.1f, 5.0f, "%.1f Hz");
	ImGui::SliderFloat("Amplitude", &amplitude, 10.0f, 100.0f, "%.0f px");

	// Visual demo - oscillating circles
	ImGui::Separator();
	ImGui::Text("Visual Demo (4 circles with different phases):");

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 120.0f);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Background
	draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(30, 30, 40, 255), 4.0f);

	// Center line
	float center_y = canvas_pos.y + canvas_size.y * 0.5f;
	draw_list->AddLine(ImVec2(canvas_pos.x, center_y), ImVec2(canvas_pos.x + canvas_size.x, center_y),
		IM_COL32(100, 100, 100, 100), 1.0f);

	// Draw 4 circles with different phases
	ImU32 colors[] = {
		IM_COL32(255, 100, 100, 255),
		IM_COL32(100, 255, 100, 255),
		IM_COL32(100, 100, 255, 255),
		IM_COL32(255, 255, 100, 255)
	};

	for (int i = 0; i < 4; i++) {
		float phase = i * 0.25f;
		float x = canvas_pos.x + 50.0f + i * (canvas_size.x - 100.0f) / 3.0f;
		char id_buf[32];
		snprintf(id_buf, sizeof(id_buf), "osc_demo_%d", i);
		float offset_y = iam_oscillate(ImGui::GetID(id_buf),
			amplitude, frequency, wave_type, phase, dt);
		draw_list->AddCircleFilled(ImVec2(x, center_y + offset_y), 12.0f, colors[i]);
		draw_list->AddCircle(ImVec2(x, center_y + offset_y), 12.0f, IM_COL32(255,255,255,100), 0, 2.0f);
	}

	ImGui::Dummy(canvas_size);

	// Vec2 oscillation demo
	if (ImGui::TreeNode("2D Oscillation (Lissajous)")) {
		static ImVec2 freq_2d(1.0f, 2.0f);
		static ImVec2 amp_2d(40.0f, 40.0f);
		ImGui::SliderFloat2("Frequency X/Y", &freq_2d.x, 0.5f, 4.0f, "%.1f");
		ImGui::SliderFloat2("Amplitude X/Y", &amp_2d.x, 10.0f, 60.0f, "%.0f");

		ImVec2 canvas_pos2 = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size2(200.0f, 200.0f);
		ImVec2 center(canvas_pos2.x + canvas_size2.x * 0.5f, canvas_pos2.y + canvas_size2.y * 0.5f);

		draw_list->AddRectFilled(canvas_pos2, ImVec2(canvas_pos2.x + canvas_size2.x, canvas_pos2.y + canvas_size2.y),
			IM_COL32(30, 30, 40, 255), 4.0f);

		ImVec2 offset = iam_oscillate_vec2(ImGui::GetID("lissajous"), amp_2d, freq_2d, iam_wave_sine, ImVec2(0, 0), dt);
		draw_list->AddCircleFilled(ImVec2(center.x + offset.x, center.y + offset.y), 10.0f, IM_COL32(100, 200, 255, 255));

		ImGui::Dummy(canvas_size2);
		ImGui::TreePop();
	}

	// Practical UI example
	if (ImGui::TreeNode("Practical: Pulsing Button")) {
		float pulse = iam_oscillate(ImGui::GetID("pulse_btn"), 0.1f, 2.0f, iam_wave_sine, 0.0f, dt);
		float scale = 1.0f + pulse;

		ImGui::SetWindowFontScale(scale);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f + pulse * 0.5f, 0.5f, 0.8f, 1.0f));
		ImGui::Button("Click Me!", ImVec2(120 * scale, 40 * scale));
		ImGui::PopStyleColor();
		ImGui::SetWindowFontScale(1.0f);

		ImGui::SameLine();
		ImGui::TextDisabled("Button pulses continuously");
		ImGui::TreePop();
	}
}

// ============================================================
// SHAKE/WIGGLE DEMO
// ============================================================
static void ShowShakeWiggleDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped("Shake provides decaying random motion (for error feedback, impacts). "
		"Wiggle provides continuous smooth random movement (for idle animations, organic feel).");

	// Shake demo
	if (ImGui::TreeNodeEx("Shake (Decaying)", ImGuiTreeNodeFlags_DefaultOpen)) {
		static float shake_intensity = 10.0f;
		static float shake_frequency = 20.0f;
		static float shake_decay = 0.5f;

		ImGui::SliderFloat("Intensity", &shake_intensity, 1.0f, 30.0f, "%.0f px");
		ImGui::SliderFloat("Frequency", &shake_frequency, 5.0f, 50.0f, "%.0f Hz");
		ImGui::SliderFloat("Decay Time", &shake_decay, 0.1f, 2.0f, "%.1f s");

		ImGuiID shake_id = ImGui::GetID("shake_demo");
		if (ImGui::Button("Trigger Shake!")) {
			iam_trigger_shake(shake_id);
		}

		ImVec2 offset = iam_shake_vec2(shake_id, ImVec2(shake_intensity, shake_intensity),
			shake_frequency, shake_decay, dt);

		// Visual
		ImVec2 box_pos = ImGui::GetCursorScreenPos();
		ImVec2 box_size(100.0f, 60.0f);
		ImVec2 center(box_pos.x + 150.0f + offset.x, box_pos.y + 40.0f + offset.y);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(
			ImVec2(center.x - box_size.x * 0.5f, center.y - box_size.y * 0.5f),
			ImVec2(center.x + box_size.x * 0.5f, center.y + box_size.y * 0.5f),
			IM_COL32(255, 100, 100, 255), 8.0f);
		draw_list->AddText(ImVec2(center.x - 25, center.y - 8), IM_COL32(255, 255, 255, 255), "SHAKE");

		ImGui::Dummy(ImVec2(300, 100));
		ImGui::TreePop();
	}

	// Wiggle demo
	if (ImGui::TreeNode("Wiggle (Continuous)")) {
		static float wiggle_amplitude = 5.0f;
		static float wiggle_frequency = 3.0f;

		ImGui::SliderFloat("Amplitude##wiggle", &wiggle_amplitude, 1.0f, 20.0f, "%.0f px");
		ImGui::SliderFloat("Frequency##wiggle", &wiggle_frequency, 0.5f, 10.0f, "%.1f Hz");

		ImVec2 offset = iam_wiggle_vec2(ImGui::GetID("wiggle_demo"),
			ImVec2(wiggle_amplitude, wiggle_amplitude), wiggle_frequency, dt);

		// Visual - floating icon
		ImVec2 icon_pos = ImGui::GetCursorScreenPos();
		ImVec2 center(icon_pos.x + 150.0f + offset.x, icon_pos.y + 40.0f + offset.y);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddCircleFilled(center, 30.0f, IM_COL32(100, 200, 100, 255));
		draw_list->AddCircle(center, 30.0f, IM_COL32(255, 255, 255, 150), 0, 2.0f);
		draw_list->AddText(ImVec2(center.x - 10, center.y - 8), IM_COL32(255, 255, 255, 255), ":)");

		ImGui::Dummy(ImVec2(300, 100));
		ImGui::SameLine();
		ImGui::TextDisabled("Continuous organic movement");
		ImGui::TreePop();
	}

	// Practical example
	if (ImGui::TreeNode("Practical: Error Feedback")) {
		static char input_buf[64] = "";
		static bool show_error = false;
		ImGuiID error_shake_id = ImGui::GetID("error_shake");

		float shake_offset = iam_shake(error_shake_id, 8.0f, 30.0f, 0.3f, dt);

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + shake_offset);
		ImGui::PushItemWidth(200);

		if (show_error) {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
		}

		ImGui::InputText("##email", input_buf, sizeof(input_buf));

		if (show_error) {
			ImGui::PopStyleColor(2);
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();
		if (ImGui::Button("Validate")) {
			show_error = (strlen(input_buf) == 0 || strchr(input_buf, '@') == nullptr);
			if (show_error) {
				iam_trigger_shake(error_shake_id);
			}
		}

		if (show_error) {
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Invalid email format!");
		}
		ImGui::TreePop();
	}
}

// ============================================================
// SCROLL ANIMATION DEMO
// ============================================================
static void ShowScrollDemo()
{
	ImGui::TextWrapped("Smooth animated scrolling within ImGui windows. "
		"Use iam_scroll_to_y() for custom positions or convenience functions for common cases.");

	// Create a child window to demonstrate scrolling
	ImGui::Text("Scroll Controls:");

	static float scroll_duration = 0.5f;
	ImGui::SliderFloat("Duration", &scroll_duration, 0.1f, 2.0f, "%.1f s");

	if (ImGui::Button("Scroll to Top")) {
		// Will be called inside the child window
	}
	bool scroll_top = ImGui::IsItemClicked();

	ImGui::SameLine();
	if (ImGui::Button("Scroll to Middle")) {
	}
	bool scroll_middle = ImGui::IsItemClicked();

	ImGui::SameLine();
	if (ImGui::Button("Scroll to Bottom")) {
	}
	bool scroll_bottom = ImGui::IsItemClicked();

	ImGui::Separator();

	// Scrollable child window
	ImGui::BeginChild("ScrollDemoChild", ImVec2(0, 300), ImGuiChildFlags_Borders);

	// Apply scroll commands inside the child window
	if (scroll_top) {
		iam_scroll_to_top(scroll_duration);
	}
	if (scroll_middle) {
		iam_scroll_to_y(500.0f, scroll_duration);  // Roughly middle
	}
	if (scroll_bottom) {
		iam_scroll_to_bottom(scroll_duration);
	}

	// Content - many items to scroll through
	for (int i = 0; i < 50; i++) {
		bool is_special = (i == 0 || i == 24 || i == 49);
		if (is_special) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
		}

		if (i == 0) ImGui::Text(">>> TOP - Item %d <<<", i);
		else if (i == 24) ImGui::Text(">>> MIDDLE - Item %d <<<", i);
		else if (i == 49) ImGui::Text(">>> BOTTOM - Item %d <<<", i);
		else ImGui::Text("Item %d - Some content here", i);

		if (is_special) {
			ImGui::PopStyleColor();
		}

		// Add scroll-to buttons for specific items
		if (i == 10 || i == 30) {
			ImGui::SameLine();
			char btn_label[32];
			snprintf(btn_label, sizeof(btn_label), "Scroll Here##%d", i);
			if (ImGui::SmallButton(btn_label)) {
				iam_scroll_to_y(ImGui::GetCursorPosY() - 50.0f, scroll_duration);
			}
		}
	}

	ImGui::EndChild();

	ImGui::TextDisabled("Current scroll Y: %.0f", ImGui::GetScrollY());
}

// ============================================================
// MOTION PATHS DEMO
// ============================================================
static void ShowMotionPathsDemo()
{
	ImGui::TextWrapped("Motion paths allow animating positions along bezier curves and Catmull-Rom splines.");

	static bool paths_initialized = false;
	static ImGuiID bezier_path_id = ImHashStr("bezier_demo_path");
	static ImGuiID catmull_path_id = ImHashStr("catmull_demo_path");
	static ImGuiID complex_path_id = ImHashStr("complex_demo_path");

	// Initialize paths once
	if (!paths_initialized) {
		// Quadratic bezier path
		iam_path::begin(bezier_path_id, ImVec2(50, 100))
			.quadratic_to(ImVec2(150, 20), ImVec2(250, 100))
			.quadratic_to(ImVec2(350, 180), ImVec2(450, 100))
			.end();

		// Catmull-Rom spline path
		iam_path::begin(catmull_path_id, ImVec2(50, 50))
			.catmull_to(ImVec2(150, 120))
			.catmull_to(ImVec2(250, 30))
			.catmull_to(ImVec2(350, 100))
			.catmull_to(ImVec2(450, 50))
			.end();

		// Complex cubic bezier path
		iam_path::begin(complex_path_id, ImVec2(50, 80))
			.cubic_to(ImVec2(100, 10), ImVec2(150, 150), ImVec2(200, 80))
			.cubic_to(ImVec2(250, 10), ImVec2(300, 150), ImVec2(350, 80))
			.line_to(ImVec2(450, 80))
			.end();

		paths_initialized = true;
	}

	static float path_duration = 2.0f;
	static int selected_ease = iam_ease_in_out_cubic;
	ImGui::SliderFloat("Duration", &path_duration, 0.5f, 5.0f);

	static const char* ease_names[] = { "Linear", "In Quad", "Out Quad", "InOut Quad",
		"In Cubic", "Out Cubic", "InOut Cubic", "In Quart", "Out Quart", "InOut Quart" };
	ImGui::Combo("Easing", &selected_ease, ease_names, IM_ARRAYSIZE(ease_names));

	// Animation state: elapsed time for each path (-1 = not playing)
	static float path_elapsed[3] = { -1.0f, -1.0f, -1.0f };

	if (ImGui::Button("Play Bezier")) path_elapsed[0] = 0.0f;
	ImGui::SameLine();
	if (ImGui::Button("Play Catmull-Rom")) path_elapsed[1] = 0.0f;
	ImGui::SameLine();
	if (ImGui::Button("Play Complex")) path_elapsed[2] = 0.0f;

	// Draw area
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(500, 180);
	ImDrawList* draw = ImGui::GetWindowDrawList();
	draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(30, 30, 40, 255));
	ImGui::Dummy(canvas_size);

	float dt = GetSafeDeltaTime();

	// Draw paths and animate
	auto draw_path = [&](ImGuiID path_id, ImU32 path_color, float& elapsed, int idx) {
		// Draw the path
		for (float t = 0; t < 1.0f; t += 0.01f) {
			ImVec2 p1 = iam_path_evaluate(path_id, t);
			ImVec2 p2 = iam_path_evaluate(path_id, t + 0.01f);
			draw->AddLine(
				ImVec2(canvas_pos.x + p1.x, canvas_pos.y + p1.y + idx * 60),
				ImVec2(canvas_pos.x + p2.x, canvas_pos.y + p2.y + idx * 60),
				path_color, 2.0f);
		}

		// Animate object along path using timer + easing
		if (elapsed >= 0.0f) {
			elapsed += dt;
			float t = elapsed / path_duration;
			if (t > 1.0f) {
				t = 1.0f;
				elapsed = -1.0f;  // Stop animation
			}
			// Apply easing
			float eased_t = iam_eval_preset(selected_ease, t);
			ImVec2 pos = iam_path_evaluate(path_id, eased_t);
			draw->AddCircleFilled(
				ImVec2(canvas_pos.x + pos.x, canvas_pos.y + pos.y + idx * 60),
				8.0f, IM_COL32(255, 255, 255, 255));
		}
	};

	draw_path(bezier_path_id, IM_COL32(100, 200, 255, 255), path_elapsed[0], 0);
	draw_path(catmull_path_id, IM_COL32(100, 255, 100, 255), path_elapsed[1], 1);
	draw_path(complex_path_id, IM_COL32(255, 150, 100, 255), path_elapsed[2], 2);

	// Labels
	draw->AddText(ImVec2(canvas_pos.x + 5, canvas_pos.y + 5), IM_COL32(100, 200, 255, 255), "Quadratic Bezier");
	draw->AddText(ImVec2(canvas_pos.x + 5, canvas_pos.y + 65), IM_COL32(100, 255, 100, 255), "Catmull-Rom");
	draw->AddText(ImVec2(canvas_pos.x + 5, canvas_pos.y + 125), IM_COL32(255, 150, 100, 255), "Cubic Bezier + Line");

	ImGui::TextDisabled("Paths can mix bezier curves, Catmull-Rom splines, and lines.");
}

// ============================================================
// TIMELINE MARKERS DEMO
// ============================================================
static void ShowTimelineMarkersDemo()
{
	ImGui::TextWrapped("Timeline markers trigger callbacks at specific times during clip playback.");

	static bool clip_initialized = false;
	static ImGuiID marker_clip_id = ImHashStr("marker_demo_clip");
	static ImVector<const char*> marker_log;
	static float marker_log_time = 0;

	// Marker callback
	static auto marker_callback = [](ImGuiID inst_id, ImGuiID marker_id, float marker_time, void* user_data) {
		char* msg = new char[64];
		snprintf(msg, 64, "Marker at %.2fs", marker_time);
		marker_log.push_back(msg);
		marker_log_time = 3.0f;  // Show for 3 seconds
	};

	// Initialize clip with markers
	if (!clip_initialized) {
		iam_clip::begin(marker_clip_id)
			.key_float(ImHashStr("progress"), 0.0f, 0.0f, iam_ease_linear)
			.key_float(ImHashStr("progress"), 3.0f, 1.0f, iam_ease_linear)
			.marker(0.5f, marker_callback)
			.marker(1.0f, marker_callback)
			.marker(1.5f, marker_callback)
			.marker(2.0f, marker_callback)
			.marker(2.5f, marker_callback)
			.end();
		clip_initialized = true;
	}

	static iam_instance marker_inst;

	if (ImGui::Button("Play Clip with Markers")) {
		// Clear log
		for (int i = 0; i < marker_log.Size; i++) delete[] marker_log[i];
		marker_log.clear();
		marker_inst = iam_play(marker_clip_id, ImHashStr("marker_inst"));
	}

	// Progress bar
	float progress = 0.0f;
	if (marker_inst.valid()) {
		marker_inst.get_float(ImHashStr("progress"), &progress);
	}
	ImGui::ProgressBar(progress, ImVec2(-1, 0), "");

	// Show markers on timeline
	ImVec2 bar_pos = ImGui::GetItemRectMin();
	ImVec2 bar_size = ImGui::GetItemRectSize();
	ImDrawList* draw = ImGui::GetWindowDrawList();
	float marker_times[] = { 0.5f, 1.0f, 1.5f, 2.0f, 2.5f };
	for (int i = 0; i < 5; i++) {
		float t = marker_times[i] / 3.0f;  // Normalize to 0-1
		float x = bar_pos.x + bar_size.x * t;
		draw->AddLine(ImVec2(x, bar_pos.y), ImVec2(x, bar_pos.y + bar_size.y), IM_COL32(255, 200, 100, 255), 2.0f);
	}

	// Marker log
	ImGui::Text("Marker Events:");
	marker_log_time -= GetSafeDeltaTime();
	if (marker_log_time > 0) {
		for (int i = 0; i < marker_log.Size; i++) {
			ImGui::BulletText("%s", marker_log[i]);
		}
	}

	ImGui::TextDisabled("Orange lines show marker positions on the timeline.");
}

// ============================================================
// ANIMATION CHAINING DEMO
// ============================================================
static void ShowAnimationChainingDemo()
{
	ImGui::TextWrapped("Animation chaining allows clips to automatically trigger another clip when they complete.");

	static bool clips_initialized = false;
	static ImGuiID clip_a = ImHashStr("chain_clip_a");
	static ImGuiID clip_b = ImHashStr("chain_clip_b");
	static ImGuiID clip_c = ImHashStr("chain_clip_c");

	// Initialize clips
	if (!clips_initialized) {
		// Clip A: Move right
		iam_clip::begin(clip_a)
			.key_float(ImHashStr("x"), 0.0f, 50.0f, iam_ease_out_cubic)
			.key_float(ImHashStr("x"), 0.5f, 200.0f, iam_ease_out_cubic)
			.key_vec4(ImHashStr("color"), 0.0f, ImVec4(1, 0.3f, 0.3f, 1), iam_ease_linear)
			.key_vec4(ImHashStr("color"), 0.5f, ImVec4(1, 0.3f, 0.3f, 1), iam_ease_linear)
			.end();

		// Clip B: Move down
		iam_clip::begin(clip_b)
			.key_float(ImHashStr("y"), 0.0f, 30.0f, iam_ease_out_cubic)
			.key_float(ImHashStr("y"), 0.5f, 100.0f, iam_ease_out_cubic)
			.key_vec4(ImHashStr("color"), 0.0f, ImVec4(0.3f, 1, 0.3f, 1), iam_ease_linear)
			.key_vec4(ImHashStr("color"), 0.5f, ImVec4(0.3f, 1, 0.3f, 1), iam_ease_linear)
			.end();

		// Clip C: Move diagonally back
		iam_clip::begin(clip_c)
			.key_float(ImHashStr("x"), 0.0f, 200.0f, iam_ease_out_cubic)
			.key_float(ImHashStr("x"), 0.5f, 50.0f, iam_ease_out_cubic)
			.key_float(ImHashStr("y"), 0.0f, 100.0f, iam_ease_out_cubic)
			.key_float(ImHashStr("y"), 0.5f, 30.0f, iam_ease_out_cubic)
			.key_vec4(ImHashStr("color"), 0.0f, ImVec4(0.3f, 0.3f, 1, 1), iam_ease_linear)
			.key_vec4(ImHashStr("color"), 0.5f, ImVec4(0.3f, 0.3f, 1, 1), iam_ease_linear)
			.end();

		clips_initialized = true;
	}

	static float chain_delay = 0.1f;
	ImGui::SliderFloat("Delay Between Clips", &chain_delay, 0.0f, 0.5f);

	static bool b_chain_set = false;

	if (ImGui::Button("Play A -> B -> C (Chained)")) {
		// Destroy any existing instances to start fresh
		iam_instance old_a = iam_get_instance(ImHashStr("chain_inst_a"));
		iam_instance old_b = iam_get_instance(ImHashStr("chain_inst_b"));
		iam_instance old_c = iam_get_instance(ImHashStr("chain_inst_c"));
		if (old_a.valid()) old_a.destroy();
		if (old_b.valid()) old_b.destroy();
		if (old_c.valid()) old_c.destroy();
		b_chain_set = false;

		// Start clip A with chain to B
		iam_instance inst_a = iam_play(clip_a, ImHashStr("chain_inst_a"));
		inst_a.then(clip_b, ImHashStr("chain_inst_b")).then_delay(chain_delay);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(with .then())");

	// Get instances
	iam_instance inst_a = iam_get_instance(ImHashStr("chain_inst_a"));
	iam_instance inst_b = iam_get_instance(ImHashStr("chain_inst_b"));
	iam_instance inst_c = iam_get_instance(ImHashStr("chain_inst_c"));

	// Set up B -> C chain when B starts (can't do this upfront since B doesn't exist yet)
	if (inst_b.valid() && inst_b.is_playing() && !b_chain_set) {
		inst_b.then(clip_c, ImHashStr("chain_inst_c")).then_delay(chain_delay);
		b_chain_set = true;
	}

	// Draw animated object
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(300, 150);
	ImDrawList* draw = ImGui::GetWindowDrawList();
	draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(30, 30, 40, 255));

	// Determine which instance is active and get values from it
	float x = 50.0f, y = 30.0f;
	ImVec4 color(0.5f, 0.5f, 0.5f, 1.0f);

	// Only read from the currently active (or most recently finished) instance
	if (inst_c.valid()) {
		inst_c.get_float(ImHashStr("x"), &x);
		inst_c.get_float(ImHashStr("y"), &y);
		inst_c.get_vec4(ImHashStr("color"), &color);
	} else if (inst_b.valid()) {
		// Get x from A's final value
		if (inst_a.valid()) inst_a.get_float(ImHashStr("x"), &x);
		inst_b.get_float(ImHashStr("y"), &y);
		inst_b.get_vec4(ImHashStr("color"), &color);
	} else if (inst_a.valid()) {
		inst_a.get_float(ImHashStr("x"), &x);
		inst_a.get_vec4(ImHashStr("color"), &color);
	}

	ImU32 obj_color = ImGui::ColorConvertFloat4ToU32(color);
	draw->AddCircleFilled(ImVec2(canvas_pos.x + x, canvas_pos.y + y), 15.0f, obj_color);

	ImGui::Dummy(canvas_size);

	// Status
	ImGui::Text("Instance Status:");
	ImGui::BulletText("A: %s", inst_a.valid() ? (inst_a.is_playing() ? "Playing" : "Done") : "Not started");
	ImGui::BulletText("B: %s", inst_b.valid() ? (inst_b.is_playing() ? "Playing" : "Done") : "Not started");
	ImGui::BulletText("C: %s", inst_c.valid() ? (inst_c.is_playing() ? "Playing" : "Done") : "Not started");
}

// ============================================================
// MAIN DEMO WINDOW
// ============================================================
void ImAnimDemoWindow()
{
	// Update animation systems
	iam_update_begin_frame();
	iam_clip_update(GetSafeDeltaTime());

	ImGui::SetNextWindowSize(ImVec2(650, 750), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Anim Demo")) {
		ImGui::End();
		return;
	}

	ImGui::Text("Anim %s", "1.0");
	ImGui::TextDisabled("Animation helpers for Dear ImGui");

	// Debug window toggle
	static bool show_debug_window = false;
	ImGui::Checkbox("Show Debug Window", &show_debug_window);
	ImGui::SameLine();
	ImGui::TextDisabled("(time scale, stats)");

	ImGui::Separator();

	// Main sections as collapsing headers (like imgui_demo.cpp)
	if (ImGui::CollapsingHeader("Easing Functions")) {
		ShowEasingDemo();
	}

	if (ImGui::CollapsingHeader("Custom Easing")) {
		ShowCustomEasingDemo();
	}

	if (ImGui::CollapsingHeader("Basic Tweens", ImGuiTreeNodeFlags_DefaultOpen)) {
		ShowBasicTweensDemo();
	}

	if (ImGui::CollapsingHeader("Color Tweens")) {
		ShowColorTweensDemo();
	}

	if (ImGui::CollapsingHeader("Tween Policies")) {
		ShowPoliciesDemo();
	}

	if (ImGui::CollapsingHeader("Interactive Widgets", ImGuiTreeNodeFlags_DefaultOpen)) {
		ShowWidgetsDemo();
	}

	if (ImGui::CollapsingHeader("Clip System")) {
		ShowClipSystemDemo();
	}

	if (ImGui::CollapsingHeader("Layering System")) {
		ShowLayeringDemo();
	}

	if (ImGui::CollapsingHeader("Resize-Aware Helpers")) {
		ShowResizeHelpersDemo();
	}

	if (ImGui::CollapsingHeader("ImDrawList Animations")) {
		ShowDrawListDemo();
	}

	if (ImGui::CollapsingHeader("Oscillators")) {
		ShowOscillatorsDemo();
	}

	if (ImGui::CollapsingHeader("Shake & Wiggle")) {
		ShowShakeWiggleDemo();
	}

	if (ImGui::CollapsingHeader("Scroll Animation")) {
		ShowScrollDemo();
	}

	if (ImGui::CollapsingHeader("Motion Paths")) {
		ShowMotionPathsDemo();
	}

	if (ImGui::CollapsingHeader("Timeline Markers")) {
		ShowTimelineMarkersDemo();
	}

	if (ImGui::CollapsingHeader("Animation Chaining")) {
		ShowAnimationChainingDemo();
	}

	// Footer
	ImGui::Separator();
	ImGui::TextDisabled("FPS: %.1f (dt: %.3f ms)", ImGui::GetIO().Framerate, ImGui::GetIO().DeltaTime * 1000.0f);

	ImGui::End();

	// Show debug window if enabled
	if (show_debug_window) {
		iam_show_debug_window(&show_debug_window);
	}
}
