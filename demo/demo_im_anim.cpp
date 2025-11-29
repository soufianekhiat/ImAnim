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

// ============================================================
// HELPER: Open/Close all collapsing headers and tree nodes
// ============================================================
static int s_open_all = 0;  // 0 = none, 1 = open all, -1 = close all

static void ApplyOpenAll()
{
	if (s_open_all != 0)
		ImGui::SetNextItemOpen(s_open_all > 0, ImGuiCond_Always);
}

// HELPER: Use iam_eval_preset from im_anim API for easing evaluation

// ============================================================
// SECTION: Hero Animation - Dynamic Brand Showcase
// ============================================================

// Helper: Draw a rotated rectangle
static void DrawRotatedRect(ImDrawList* dl, ImVec2 ctr, ImVec2 size, float angle, ImU32 fill, ImU32 border) {
	float c = cosf(angle), s = sinf(angle);
	ImVec2 corners[4] = {
		ImVec2(-size.x * 0.5f, -size.y * 0.5f),
		ImVec2( size.x * 0.5f, -size.y * 0.5f),
		ImVec2( size.x * 0.5f,  size.y * 0.5f),
		ImVec2(-size.x * 0.5f,  size.y * 0.5f)
	};
	ImVec2 pts[4];
	for (int i = 0; i < 4; i++) {
		pts[i].x = ctr.x + corners[i].x * c - corners[i].y * s;
		pts[i].y = ctr.y + corners[i].x * s + corners[i].y * c;
	}
	dl->AddConvexPolyFilled(pts, 4, fill);
	if ((border & IM_COL32_A_MASK) > 0)
		dl->AddPolyline(pts, 4, border, ImDrawFlags_Closed, 1.5f);
}

static void ShowHeroAnimation()
{
	float dt = GetSafeDeltaTime();
	static float T = 0.0f;  // Global time
	T += dt;

	// === 8-second cycle with sequences ===
	const float CYCLE = 8.0f;
	float t = fmodf(T, CYCLE);

	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 cp = ImGui::GetCursorScreenPos();  // Canvas position
	ImVec2 cs = ImVec2(ImGui::GetContentRegionAvail().x, 300.0f);  // Canvas size
	ImVec2 cc = ImVec2(cp.x + cs.x * 0.5f, cp.y + cs.y * 0.5f);    // Canvas center

	// === BRAND COLORS ===
	const ImU32 C1 = IM_COL32(91, 194, 231, 255);   // Main cyan
	const ImU32 C2 = IM_COL32(204, 120, 88, 255);   // Accent coral
	const ImU32 C1a = IM_COL32(91, 194, 231, 80);
	const ImU32 C2a = IM_COL32(204, 120, 88, 80);

	// ========================================================
	// LAYER 0: ANIMATED GRADIENT BACKGROUND
	// ========================================================
	{
		float grad_shift = sinf(T * 0.8f) * 0.5f + 0.5f;
		float grad_shift2 = sinf(T * 0.5f + 1.0f) * 0.5f + 0.5f;

		// Four corners with animated colors
		ImU32 tl = IM_COL32((int)(12 + 20 * grad_shift), (int)(14 + 15 * grad_shift), (int)(28 + 25 * grad_shift), 255);
		ImU32 tr = IM_COL32((int)(8 + 30 * grad_shift2), (int)(12 + 20 * grad_shift2), (int)(22 + 35 * grad_shift2), 255);
		ImU32 bl = IM_COL32((int)(15 + 25 * grad_shift2), (int)(10 + 18 * grad_shift2), (int)(25 + 30 * grad_shift2), 255);
		ImU32 br = IM_COL32((int)(10 + 22 * grad_shift), (int)(15 + 12 * grad_shift), (int)(30 + 20 * grad_shift), 255);

		dl->AddRectFilledMultiColor(cp, ImVec2(cp.x + cs.x, cp.y + cs.y), tl, tr, br, bl);
	}

	// ========================================================
	// LAYER 1: BEZIER CURVES NETWORK (Background decoration)
	// ========================================================
	{
		// Multiple curves flowing across the canvas
		struct CurveDef { float ox, oy, ax, ay, speed, phase; };
		CurveDef curves[] = {
			{ 0.0f, 0.3f, 0.15f, 0.25f, 0.7f, 0.0f },
			{ 0.0f, 0.7f, 0.2f,  0.2f,  0.5f, 1.5f },
			{ 1.0f, 0.2f, -0.15f, 0.3f, 0.6f, 0.8f },
			{ 1.0f, 0.8f, -0.2f, 0.15f, 0.8f, 2.2f },
		};

		for (int ci = 0; ci < 4; ci++) {
			CurveDef& cv = curves[ci];
			float anim = fmodf(T * cv.speed + cv.phase, 3.0f) / 3.0f;

			// Dynamic bezier control points
			ImVec2 p0 = ImVec2(cp.x + cs.x * cv.ox, cp.y + cs.y * cv.oy);
			ImVec2 p3 = ImVec2(cp.x + cs.x * (1.0f - cv.ox), cp.y + cs.y * (1.0f - cv.oy + sinf(T + cv.phase) * 0.1f));
			ImVec2 p1 = ImVec2(p0.x + cs.x * (cv.ax + sinf(T * 0.3f + cv.phase) * 0.05f),
			                   p0.y + cs.y * (cv.ay * sinf(T * 0.5f + cv.phase)));
			ImVec2 p2 = ImVec2(p3.x - cs.x * (cv.ax + cosf(T * 0.4f + cv.phase) * 0.05f),
			                   p3.y - cs.y * (cv.ay * cosf(T * 0.6f + cv.phase)));

			// Draw curve
			ImVec2 prev = p0;
			for (int i = 1; i <= 30; i++) {
				float ct = (float)i / 30.0f;
				ImVec2 pt = iam_bezier_cubic(p0, p1, p2, p3, ct);
				int alpha = (int)(20 + 15 * sinf(ct * 3.14159f));
				dl->AddLine(prev, pt, (ci % 2 == 0) ? IM_COL32(91, 194, 231, alpha) : IM_COL32(204, 120, 88, alpha), 1.0f);
				prev = pt;
			}

			// Traveling dot on curve
			float dot_t = iam_eval_preset(iam_ease_in_out_sine, anim);
			ImVec2 dot_pos = iam_bezier_cubic(p0, p1, p2, p3, dot_t);
			dl->AddCircleFilled(dot_pos, 3.0f, (ci % 2 == 0) ? C1a : C2a);
		}
	}

	// ========================================================
	// LAYER 2: TRANSFORM SHOWCASE RECTANGLE (Rotation + Scale)
	// ========================================================
	{
		// One rectangle with full transform: position along a RIGHT STROPHOID curve
		// Strophoid parametric equations:
		//   raw_x(s) = (s² - 1) / (s² + 1)
		//   raw_y(s) = s * (s² - 1) / (s² + 1)
		// Rotated 90° so the curve goes left-to-right with loop in middle

		const float PI = 3.14159265f;
		float center_x = cp.x + cs.x * 0.5f;
		float center_y = cp.y + cs.y * 0.5f;
		float travel = cs.x - 120.0f;  // Horizontal span of the curve

		// Scale factor: raw_y spans [-1.2, 1.2] for s in [-2, 2], so 2.4 total
		// We want the curve to span 'travel' horizontally
		float curve_scale = travel / 2.4f;
		float loop_height = curve_scale * 0.25f;  // Height of the loop (reduced more)

		// Lambda to compute position on the RIGHT STROPHOID
		// s ∈ [-2, 2] traces the full curve with loop
		// Rotated 90° and flipped Y: x follows raw_y, y follows +raw_x (loop goes DOWN)
		auto strophoid_pos = [&](float t) -> ImVec2 {
			float s = -2.0f + 4.0f * t;  // Map t:[0,1] to s:[-2,2]
			float s2 = s * s;
			float denom = s2 + 1.0f;
			float factor = (s2 - 1.0f) / denom;

			float raw_x = factor;        // ranges: 0.6 → -1 → 0.6
			float raw_y = s * factor;    // ranges: -1.2 → 0 → 1.2

			// Rotate 90° + flip Y: new_x = raw_y, new_y = +raw_x (loop goes down)
			float x = center_x + raw_y * curve_scale;
			float y = center_y + raw_x * loop_height;
			return ImVec2(x, y);
		};

		// Lambda to compute tangent (derivative) for rotation
		// d(factor)/ds = 4s / (s²+1)²
		// d(s·factor)/ds = (s⁴ + 4s² - 1) / (s²+1)²
		auto strophoid_tangent = [&](float t) -> ImVec2 {
			float s = -2.0f + 4.0f * t;
			float s2 = s * s;
			float denom = s2 + 1.0f;
			float denom2 = denom * denom;

			// Derivatives of raw coordinates w.r.t. s
			float d_raw_x = 4.0f * s / denom2;
			float d_raw_y = (s2 * s2 + 4.0f * s2 - 1.0f) / denom2;

			// Rotated + flipped: dx/ds = d_raw_y, dy/ds = +d_raw_x
			// Chain rule: dx/dt = dx/ds * ds/dt, where ds/dt = 4
			float dx = d_raw_y * 4.0f * curve_scale;
			float dy = d_raw_x * 4.0f * loop_height;
			return ImVec2(dx, dy);
		};

		// Alias for compatibility with existing code
		auto loop_curve_pos = strophoid_pos;
		auto loop_curve_tangent = strophoid_tangent;

		// Animation: 4s travel + 0.5s pause
		float local_t = fmodf(T, 4.5f);
		float progress = ImClamp(local_t / 4.0f, 0.0f, 1.0f);

		// Apply bounce easing to the progress
		float eased = iam_eval_preset(iam_ease_out_bounce, progress);

		// Position on the continuous loop curve
		ImVec2 pos = loop_curve_pos(eased);
		ImVec2 tangent = loop_curve_tangent(eased);
		float path_angle = atan2f(tangent.y, tangent.x);

		// ROTATION: Follow tangent + extra spin with elastic easing
		float rot_eased = iam_eval_preset(iam_ease_out_elastic, progress);
		float rotation = path_angle + rot_eased * PI * 2.0f;

		// SCALE: Burlesque exaggerated effect - dramatic swings
		float scale_eased = iam_eval_preset(iam_ease_out_elastic, progress);
		float base_scale = 0.2f + 0.9f * scale_eased;  // Starts tiny, grows big
		// Strong pulse that peaks during the loop (around progress 0.3-0.7)
		float loop_intensity = 1.0f - powf(fabsf(progress - 0.5f) * 2.0f, 2.0f);
		float pulse = sinf(progress * PI * 6.0f) * 0.35f * loop_intensity;
		// Add a "squash and stretch" effect based on velocity
		float squash = sinf(progress * PI * 4.0f) * 0.2f * (1.0f - progress);
		float scale = base_scale + pulse + squash;

		// Draw the continuous loop curve path hint
		ImVec2 prev_curve_pt = loop_curve_pos(0.0f);
		for (int i = 1; i <= 80; i++) {
			float ct = (float)i / 80.0f;
			ImVec2 curve_pt = loop_curve_pos(ct);
			int alpha = 25 + (int)(15.0f * sinf(ct * PI * 4.0f));
			dl->AddLine(prev_curve_pt, curve_pt, IM_COL32(91, 194, 231, alpha), 2.0f);
			prev_curve_pt = curve_pt;
		}

		// Motion trail with rotation - using same continuous curve
		for (int tr = 6; tr >= 1; tr--) {
			float trail_progress = ImMax(0.0f, progress - tr * 0.04f);
			float trail_eased = iam_eval_preset(iam_ease_out_bounce, trail_progress);
			float trail_rot_eased = iam_eval_preset(iam_ease_out_elastic, trail_progress);

			ImVec2 trail_pos = loop_curve_pos(trail_eased);
			ImVec2 trail_tan = loop_curve_tangent(trail_eased);
			float trail_path_angle = atan2f(trail_tan.y, trail_tan.x);
			float trail_rot = trail_path_angle + trail_rot_eased * PI * 2.0f;
			float trail_scale = (0.3f + 0.7f * iam_eval_preset(iam_ease_out_back, trail_progress)) * (1.0f - tr * 0.08f);

			int alpha = 50 - tr * 7;
			DrawRotatedRect(dl, trail_pos, ImVec2(40.0f * trail_scale, 24.0f * trail_scale), trail_rot,
				IM_COL32(91, 194, 231, alpha), 0);
		}

		// Main rectangle
		ImVec2 rect_size = ImVec2(44.0f * scale, 26.0f * scale);
		DrawRotatedRect(dl, pos, rect_size, rotation, C1, IM_COL32(255, 255, 255, 180));

		// Inner detail to show rotation clearly
		DrawRotatedRect(dl, pos, ImVec2(rect_size.x * 0.4f, rect_size.y * 0.4f), rotation, C2, 0);
	}

	// ========================================================
	// LAYER 2b: ADDITIONAL PATH RECTANGLES
	// ========================================================
	{
		// Two more rectangles with simpler animations
		struct RectPath {
			ImVec2 p0, p1, p2, p3;
			int ease_type;
			float duration, delay;
			ImU32 color;
		};

		RectPath paths[2];

		// Path 1: Top curve (back easing)
		paths[0].p0 = ImVec2(cp.x + cs.x - 60, cp.y + cs.y * 0.25f);
		paths[0].p3 = ImVec2(cp.x + 60, cp.y + cs.y * 0.25f);
		paths[0].p1 = ImVec2(cp.x + cs.x * 0.7f, cp.y + 20);
		paths[0].p2 = ImVec2(cp.x + cs.x * 0.3f, cp.y + 20);
		paths[0].ease_type = iam_ease_out_back;
		paths[0].duration = 2.2f;
		paths[0].delay = 0.5f;
		paths[0].color = C2;

		// Path 2: Bottom curve (cubic)
		paths[1].p0 = ImVec2(cp.x + 60, cp.y + cs.y * 0.75f);
		paths[1].p3 = ImVec2(cp.x + cs.x - 60, cp.y + cs.y * 0.75f);
		paths[1].p1 = ImVec2(cp.x + cs.x * 0.3f, cp.y + cs.y - 20);
		paths[1].p2 = ImVec2(cp.x + cs.x * 0.7f, cp.y + cs.y - 20);
		paths[1].ease_type = iam_ease_out_cubic;
		paths[1].duration = 2.0f;
		paths[1].delay = 0.8f;
		paths[1].color = IM_COL32(140, 200, 180, 255);

		for (int pi = 0; pi < 2; pi++) {
			RectPath& rp = paths[pi];

			float local_t = fmodf(T - rp.delay, rp.duration + 1.2f);
			float progress = ImClamp(local_t / rp.duration, 0.0f, 1.0f);
			float eased = iam_eval_preset(rp.ease_type, progress);

			ImVec2 pos = iam_bezier_cubic(rp.p0, rp.p1, rp.p2, rp.p3, eased);
			ImVec2 tan = iam_bezier_cubic_deriv(rp.p0, rp.p1, rp.p2, rp.p3, eased);
			float angle = atan2f(tan.y, tan.x);

			// Path dots
			for (int i = 0; i < 15; i++) {
				float pt = (float)i / 15.0f;
				ImVec2 pp = iam_bezier_cubic(rp.p0, rp.p1, rp.p2, rp.p3, pt);
				dl->AddCircleFilled(pp, 1.0f, IM_COL32((rp.color >> 0) & 0xFF, (rp.color >> 8) & 0xFF, (rp.color >> 16) & 0xFF, 20));
			}

			// Trail
			for (int tr = 3; tr >= 1; tr--) {
				float trail_eased = ImMax(0.0f, eased - tr * 0.05f);
				ImVec2 trail_pos = iam_bezier_cubic(rp.p0, rp.p1, rp.p2, rp.p3, trail_eased);
				ImVec2 trail_tan = iam_bezier_cubic_deriv(rp.p0, rp.p1, rp.p2, rp.p3, trail_eased);
				float trail_angle = atan2f(trail_tan.y, trail_tan.x);
				ImU32 trail_col = IM_COL32((rp.color >> 0) & 0xFF, (rp.color >> 8) & 0xFF, (rp.color >> 16) & 0xFF, 35 - tr * 10);
				DrawRotatedRect(dl, trail_pos, ImVec2(22.0f - tr * 2.0f, 14.0f - tr), trail_angle, trail_col, 0);
			}

			// Main rectangle
			float rect_scale = 0.8f + 0.2f * iam_eval_preset(iam_ease_out_back, ImMin(progress * 2.5f, 1.0f));
			DrawRotatedRect(dl, pos, ImVec2(26.0f * rect_scale, 16.0f * rect_scale), angle, rp.color,
				IM_COL32(255, 255, 255, 100));
		}
	}

	// ========================================================
	// LAYER 3: CENTRAL LOGO WITH TRANSFORM ANIMATION
	// ========================================================
	{
		const char* logo = "ImAnim";
		float base_size = ImGui::GetFontSize();

		// Sequence timing within cycle
		// 0-1s: Build up (scale from 0)
		// 1-3s: Hold with subtle motion
		// 3-3.5s: Glitch/cut effect
		// 3.5-7s: Hold
		// 7-8s: Exit

		float logo_scale = 1.0f;
		float logo_alpha = 1.0f;
		float logo_y_offset = 0.0f;
		float logo_rotation = 0.0f;
		bool show_glitch = false;

		if (t < 1.0f) {
			// Enter: scale up with overshoot
			float enter_t = t / 1.0f;
			logo_scale = iam_eval_preset(iam_ease_out_back, enter_t) * 3.5f;
			logo_alpha = iam_eval_preset(iam_ease_out_cubic, enter_t);
			logo_y_offset = (1.0f - iam_eval_preset(iam_ease_out_expo, enter_t)) * 30.0f;
		}
		else if (t < 3.0f) {
			// Hold with breathing
			logo_scale = 3.5f + sinf(T * 2.5f) * 0.08f;
			logo_rotation = sinf(T * 1.5f) * 0.015f;
		}
		else if (t < 3.5f) {
			// Glitch cut
			show_glitch = true;
			logo_scale = 3.5f;
			logo_y_offset = sinf((t - 3.0f) * 60.0f) * 8.0f * (1.0f - (t - 3.0f) * 2.0f);
		}
		else if (t < 7.0f) {
			// Hold
			logo_scale = 3.5f + sinf(T * 2.0f) * 0.05f;
		}
		else {
			// Exit
			float exit_t = (t - 7.0f) / 1.0f;
			logo_scale = 3.5f * (1.0f - iam_eval_preset(iam_ease_in_back, exit_t));
			logo_alpha = 1.0f - iam_eval_preset(iam_ease_in_expo, exit_t);
		}

		if (logo_scale > 0.1f && logo_alpha > 0.01f) {
			float font_size = ImMax(1.0f, base_size * logo_scale);
			float spacing = font_size * 0.6f;
			float total_w = spacing * 5 + ImGui::CalcTextSize("m").x * logo_scale;
			float start_x = cc.x - total_w * 0.5f;
			float base_y = cc.y - font_size * 0.35f + logo_y_offset;

			// Glitch effect: horizontal slice displacement
			if (show_glitch) {
				float gt = (t - 3.0f) * 4.0f;
				for (int slice = 0; slice < 3; slice++) {
					float slice_offset = sinf(gt * 20.0f + slice * 2.0f) * 15.0f * (1.0f - gt);
					float slice_y = base_y + (slice - 1) * font_size * 0.3f;
					ImU32 slice_col = (slice % 2 == 0) ? IM_COL32(91, 194, 231, (int)(80 * (1.0f - gt)))
					                                   : IM_COL32(204, 120, 88, (int)(80 * (1.0f - gt)));
					dl->AddText(nullptr, font_size * 0.9f, ImVec2(start_x + slice_offset, slice_y), slice_col, logo);
				}
			}

			// Draw each character with stagger
			for (int i = 0; i < 6; i++) {
				char ch[2] = { logo[i], '\0' };

				// Per-character wave motion
				float char_wave = sinf(T * 3.0f + i * 0.5f) * 3.0f;
				float char_scale = 1.0f + sinf(T * 2.0f + i * 0.8f) * 0.05f;

				// Color: gradient across letters
				float hue_t = (float)i / 5.0f;
				int r = (int)(91 + (204 - 91) * hue_t);
				int g = (int)(194 + (120 - 194) * hue_t);
				int b = (int)(231 + (88 - 231) * hue_t);
				ImU32 char_col = IM_COL32(r, g, b, (int)(logo_alpha * 255));

				ImVec2 char_pos = ImVec2(start_x + i * spacing, base_y + char_wave);

				// Shadow
				dl->AddText(nullptr, font_size * char_scale, ImVec2(char_pos.x + 3, char_pos.y + 3),
					IM_COL32(0, 0, 0, (int)(logo_alpha * 100)), ch);

				// Glow
				dl->AddText(nullptr, font_size * char_scale + 2, ImVec2(char_pos.x - 1, char_pos.y - 1),
					IM_COL32(r, g, b, (int)(logo_alpha * 50)), ch);

				// Main
				dl->AddText(nullptr, font_size * char_scale, char_pos, char_col, ch);
			}

			// Underline with gradient
			float line_y = base_y + font_size + 8.0f;
			float line_progress = (t < 1.5f) ? iam_eval_preset(iam_ease_out_expo, (t - 0.5f) / 1.0f) : 1.0f;
			if (t > 7.0f) line_progress = 1.0f - iam_eval_preset(iam_ease_in_expo, (t - 7.0f) / 0.5f);
			line_progress = ImClamp(line_progress, 0.0f, 1.0f);

			float line_w = total_w * 0.8f * line_progress;
			if (line_w > 1.0f) {
				// Gradient line (approximate with segments)
				int segs = 20;
				for (int i = 0; i < segs; i++) {
					float seg_t = (float)i / segs;
					float x1 = cc.x - line_w * 0.5f + line_w * seg_t;
					float x2 = cc.x - line_w * 0.5f + line_w * (seg_t + 1.0f / segs);
					int sr = (int)(91 + (204 - 91) * seg_t);
					int sg = (int)(194 + (120 - 194) * seg_t);
					int sb = (int)(231 + (88 - 231) * seg_t);
					dl->AddLine(ImVec2(x1, line_y), ImVec2(x2, line_y), IM_COL32(sr, sg, sb, (int)(logo_alpha * 200)), 2.5f);
				}
			}
		}
	}

	// ========================================================
	// LAYER 4: FLOATING PARTICLES WITH EASING
	// ========================================================
	{
		struct Particle { float seed, speed, size, ease_type; };
		Particle particles[] = {
			{ 1.0f, 0.8f, 3.0f, (float)iam_ease_out_sine },
			{ 2.3f, 0.6f, 2.5f, (float)iam_ease_out_quad },
			{ 3.7f, 1.0f, 2.0f, (float)iam_ease_out_cubic },
			{ 4.1f, 0.7f, 3.5f, (float)iam_ease_out_bounce },
			{ 5.5f, 0.9f, 2.8f, (float)iam_ease_out_elastic },
			{ 6.2f, 0.5f, 2.2f, (float)iam_ease_in_out_sine },
			{ 7.8f, 0.75f, 3.2f, (float)iam_ease_out_back },
			{ 8.4f, 0.85f, 2.6f, (float)iam_ease_out_quart },
		};

		for (int i = 0; i < 8; i++) {
			Particle& p = particles[i];

			// Looping vertical motion
			float cycle_t = fmodf(T * p.speed + p.seed * 10.0f, 4.0f);
			float progress = ImClamp(cycle_t / 3.0f, 0.0f, 1.0f);
			float eased_y = iam_eval_preset((int)p.ease_type, progress);

			float px = cp.x + fmodf(p.seed * 137.0f, cs.x);
			float py = cp.y + cs.y - eased_y * (cs.y + 40.0f);

			// Alpha fade at edges
			float alpha = 1.0f;
			if (progress < 0.1f) alpha = progress / 0.1f;
			if (progress > 0.9f) alpha = (1.0f - progress) / 0.1f;

			ImU32 pcol = (i % 2 == 0)
				? IM_COL32(91, 194, 231, (int)(alpha * 150))
				: IM_COL32(204, 120, 88, (int)(alpha * 120));

			// Glow
			dl->AddCircleFilled(ImVec2(px, py), p.size * 2.5f, IM_COL32((pcol >> 0) & 0xFF, (pcol >> 8) & 0xFF, (pcol >> 16) & 0xFF, (int)(alpha * 30)));
			dl->AddCircleFilled(ImVec2(px, py), p.size, pcol);
		}
	}

	// ========================================================
	// LAYER 5: SUBTITLE TEXT ON WAVE
	// ========================================================
	{
		const char* subtitle = "Fluid Animation for Dear ImGui";
		int len = (int)strlen(subtitle);

		// Appear after logo settles
		float sub_alpha = 0.0f;
		if (t > 1.5f && t < 7.5f) {
			if (t < 2.5f) sub_alpha = (t - 1.5f) / 1.0f;
			else if (t > 6.5f) sub_alpha = (7.5f - t) / 1.0f;
			else sub_alpha = 1.0f;
		}

		if (sub_alpha > 0.01f) {
			float base_y = cc.y + 55.0f;
			float wave_amp = 6.0f;
			float total_w = ImGui::CalcTextSize(subtitle).x * 1.1f;
			float start_x = cc.x - total_w * 0.5f;

			float char_x = start_x;
			for (int i = 0; i < len; i++) {
				char ch[2] = { subtitle[i], '\0' };
				ImVec2 ch_size = ImGui::CalcTextSize(ch);

				// Wave motion
				float wave_y = sinf(T * 2.5f + i * 0.3f) * wave_amp;

				// Stagger reveal
				float reveal = ImClamp((sub_alpha * len * 1.5f - i) / 3.0f, 0.0f, 1.0f);
				float char_alpha = sub_alpha * reveal;
				float char_scale = 0.8f + 0.2f * iam_eval_preset(iam_ease_out_back, reveal);

				if (char_alpha > 0.01f) {
					ImU32 col = IM_COL32(180, 195, 210, (int)(char_alpha * 200));
					dl->AddText(nullptr, ImGui::GetFontSize() * char_scale,
						ImVec2(char_x, base_y + wave_y), col, ch);
				}

				char_x += ch_size.x * 1.05f;
			}
		}
	}

	// ========================================================
	// LAYER 6: CORNER ACCENTS & FRAME
	// ========================================================
	{
		float corner_progress = ImClamp((t - 2.0f) / 0.5f, 0.0f, 1.0f);
		if (t > 7.0f) corner_progress = ImMax(0.0f, 1.0f - (t - 7.0f) / 0.3f);

		float corner_len = 30.0f * iam_eval_preset(iam_ease_out_back, corner_progress);
		ImU32 corner_col = IM_COL32(91, 194, 231, (int)(corner_progress * 150));

		float m = 10.0f;  // Margin
		// Top-left
		dl->AddLine(ImVec2(cp.x + m, cp.y + m), ImVec2(cp.x + m + corner_len, cp.y + m), corner_col, 2.0f);
		dl->AddLine(ImVec2(cp.x + m, cp.y + m), ImVec2(cp.x + m, cp.y + m + corner_len), corner_col, 2.0f);
		// Top-right
		dl->AddLine(ImVec2(cp.x + cs.x - m, cp.y + m), ImVec2(cp.x + cs.x - m - corner_len, cp.y + m), corner_col, 2.0f);
		dl->AddLine(ImVec2(cp.x + cs.x - m, cp.y + m), ImVec2(cp.x + cs.x - m, cp.y + m + corner_len), corner_col, 2.0f);
		// Bottom-left
		dl->AddLine(ImVec2(cp.x + m, cp.y + cs.y - m), ImVec2(cp.x + m + corner_len, cp.y + cs.y - m), corner_col, 2.0f);
		dl->AddLine(ImVec2(cp.x + m, cp.y + cs.y - m), ImVec2(cp.x + m, cp.y + cs.y - m - corner_len), corner_col, 2.0f);
		// Bottom-right (accent color)
		ImU32 corner_col2 = IM_COL32(204, 120, 88, (int)(corner_progress * 150));
		dl->AddLine(ImVec2(cp.x + cs.x - m, cp.y + cs.y - m), ImVec2(cp.x + cs.x - m - corner_len, cp.y + cs.y - m), corner_col2, 2.0f);
		dl->AddLine(ImVec2(cp.x + cs.x - m, cp.y + cs.y - m), ImVec2(cp.x + cs.x - m, cp.y + cs.y - m - corner_len), corner_col2, 2.0f);
	}

	// Reserve space
	ImGui::Dummy(cs);
	ImGui::Spacing();
}

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
	if (ImGui::Button(preview_playing ? "Reset##EasePreview" : "Play##EasePreview")) {
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
	ApplyOpenAll();
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

	ApplyOpenAll();
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

	ApplyOpenAll();
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

	ApplyOpenAll();
	if (ImGui::TreeNode("Easing Gallery")) {
		ImGui::TextWrapped(
			"Visual grid showing all standard easing functions side-by-side. "
			"Red disc shows X (time), green disc shows Y (eased value).");

		static float gallery_time = 0.0f;
		static bool gallery_playing = true;
		static float gallery_duration = 1.5f;

		ImGui::Checkbox("Auto-play", &gallery_playing);
		ImGui::SameLine();
		if (ImGui::Button("Reset##gallery")) {
			gallery_time = 0.0f;
		}
		ImGui::SameLine();
		ImGui::SliderFloat("Duration##EaseGallery", &gallery_duration, 0.5f, 3.0f, "%.1fs");

		if (gallery_playing) {
			gallery_time += dt;
			if (gallery_time > gallery_duration + 0.5f) gallery_time = 0.0f;
		}

		float t = gallery_time / gallery_duration;
		if (t > 1.0f) t = 1.0f;

		// Easing info: name, enum value
		struct EaseInfo { const char* name; int type; };
		EaseInfo eases[] = {
			{ "Linear",       iam_ease_linear },
			{ "In Quad",      iam_ease_in_quad },
			{ "Out Quad",     iam_ease_out_quad },
			{ "InOut Quad",   iam_ease_in_out_quad },
			{ "In Cubic",     iam_ease_in_cubic },
			{ "Out Cubic",    iam_ease_out_cubic },
			{ "InOut Cubic",  iam_ease_in_out_cubic },
			{ "In Quart",     iam_ease_in_quart },
			{ "Out Quart",    iam_ease_out_quart },
			{ "InOut Quart",  iam_ease_in_out_quart },
			{ "In Quint",     iam_ease_in_quint },
			{ "Out Quint",    iam_ease_out_quint },
			{ "InOut Quint",  iam_ease_in_out_quint },
			{ "In Sine",      iam_ease_in_sine },
			{ "Out Sine",     iam_ease_out_sine },
			{ "InOut Sine",   iam_ease_in_out_sine },
			{ "In Expo",      iam_ease_in_expo },
			{ "Out Expo",     iam_ease_out_expo },
			{ "InOut Expo",   iam_ease_in_out_expo },
			{ "In Circ",      iam_ease_in_circ },
			{ "Out Circ",     iam_ease_out_circ },
			{ "InOut Circ",   iam_ease_in_out_circ },
			{ "In Back",      iam_ease_in_back },
			{ "Out Back",     iam_ease_out_back },
			{ "InOut Back",   iam_ease_in_out_back },
			{ "In Elastic",   iam_ease_in_elastic },
			{ "Out Elastic",  iam_ease_out_elastic },
			{ "InOut Elastic",iam_ease_in_out_elastic },
			{ "In Bounce",    iam_ease_in_bounce },
			{ "Out Bounce",   iam_ease_out_bounce },
			{ "InOut Bounce", iam_ease_in_out_bounce },
		};
		int num_eases = IM_ARRAYSIZE(eases);

		// Grid layout - larger cells
		ImVec2 cell_size(200, 160);
		int cols = (int)(ImGui::GetContentRegionAvail().x / (cell_size.x + 10));
		if (cols < 1) cols = 1;
		if (cols > 4) cols = 4;

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		for (int i = 0; i < num_eases; i++) {
			if (i % cols != 0) ImGui::SameLine();

			ImGui::BeginGroup();

			ImVec2 cell_pos = ImGui::GetCursorScreenPos();
			float margin = 12.0f;
			float label_h = 20.0f;
			float graph_x = cell_pos.x + margin;
			float graph_y = cell_pos.y + label_h;
			float graph_w = cell_size.x - margin * 2;
			float graph_h = cell_size.y - label_h - margin;

			// Cell background
			draw_list->AddRectFilled(cell_pos,
				ImVec2(cell_pos.x + cell_size.x, cell_pos.y + cell_size.y),
				IM_COL32(30, 30, 35, 255), 4.0f);
			draw_list->AddRect(cell_pos,
				ImVec2(cell_pos.x + cell_size.x, cell_pos.y + cell_size.y),
				IM_COL32(60, 60, 70, 255), 4.0f);

			// Graph background
			draw_list->AddRectFilled(ImVec2(graph_x, graph_y),
				ImVec2(graph_x + graph_w, graph_y + graph_h),
				IM_COL32(20, 20, 25, 255), 2.0f);

			// Grid lines
			for (int g = 1; g < 4; g++) {
				float gx = graph_x + graph_w * (g / 4.0f);
				float gy = graph_y + graph_h * (g / 4.0f);
				draw_list->AddLine(ImVec2(gx, graph_y), ImVec2(gx, graph_y + graph_h), IM_COL32(50, 50, 55, 100));
				draw_list->AddLine(ImVec2(graph_x, gy), ImVec2(graph_x + graph_w, gy), IM_COL32(50, 50, 55, 100));
			}

			// Y=0 and Y=1 reference lines
			float y0_line = graph_y + graph_h;
			float y1_line = graph_y;
			draw_list->AddLine(ImVec2(graph_x, y0_line), ImVec2(graph_x + graph_w, y0_line), IM_COL32(80, 80, 80, 150));
			draw_list->AddLine(ImVec2(graph_x, y1_line), ImVec2(graph_x + graph_w, y1_line), IM_COL32(80, 80, 80, 150));

			// Draw easing curve
			ImVec2 prev_pt(graph_x, graph_y + graph_h);
			for (int j = 1; j <= 60; j++) {
				float ct = j / 60.0f;
				float val = iam_eval_preset(eases[i].type, ct);
				val = ImClamp(val, -0.2f, 1.2f);

				ImVec2 pt;
				pt.x = graph_x + graph_w * ct;
				pt.y = graph_y + graph_h - graph_h * val;

				draw_list->AddLine(prev_pt, pt, IM_COL32(100, 180, 255, 255), 2.0f);
				prev_pt = pt;
			}

			// Animated indicators
			if (t <= 1.0f) {
				float eased = iam_eval_preset(eases[i].type, t);
				float eased_clamped = ImClamp(eased, -0.2f, 1.2f);

				float ball_x = graph_x + graph_w * t;
				float ball_y = graph_y + graph_h - graph_h * eased_clamped;

				// X axis indicator (red) - horizontal line with disc
				draw_list->AddLine(ImVec2(graph_x, ball_y), ImVec2(ball_x, ball_y), IM_COL32(255, 80, 80, 150), 1.0f);
				draw_list->AddCircleFilled(ImVec2(graph_x - 6, ball_y), 5.0f, IM_COL32(255, 80, 80, 255));

				// Y axis indicator (green) - vertical line with disc
				draw_list->AddLine(ImVec2(ball_x, graph_y + graph_h), ImVec2(ball_x, ball_y), IM_COL32(80, 255, 80, 150), 1.0f);
				draw_list->AddCircleFilled(ImVec2(ball_x, graph_y + graph_h + 6), 5.0f, IM_COL32(80, 255, 80, 255));

				// Ball on curve (yellow)
				draw_list->AddCircleFilled(ImVec2(ball_x, ball_y), 6.0f, IM_COL32(255, 220, 100, 255));
				draw_list->AddCircle(ImVec2(ball_x, ball_y), 6.0f, IM_COL32(255, 255, 255, 200), 0, 1.5f);
			}

			// Label at top
			ImVec2 text_size = ImGui::CalcTextSize(eases[i].name);
			ImVec2 text_pos = ImVec2(cell_pos.x + (cell_size.x - text_size.x) * 0.5f, cell_pos.y + 3);
			draw_list->AddText(text_pos, IM_COL32(220, 220, 220, 255), eases[i].name);

			ImGui::Dummy(cell_size);
			ImGui::EndGroup();
		}

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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
// SECTION: Per-Axis Easing
// ============================================================
static void ShowPerAxisEasingDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped(
		"Per-axis easing allows different easing functions for each axis of a vector or color. "
		"This enables effects like elastic bounce on one axis while smooth motion on another.");

	ImGui::Spacing();

	// Demo 1: Vec2 with different X and Y easing
	ApplyOpenAll();
	if (ImGui::TreeNode("Vec2 Per-Axis")) {
		static int ease_x = 2;   // Out Cubic
		static int ease_y = 10;  // Out Bounce
		static ImVec2 target_pos(300, 100);
		static bool toggle = false;

		const char* ease_names[] = {
			"Linear", "Out Quad", "Out Cubic", "Out Quart", "Out Quint",
			"Out Sine", "Out Expo", "Out Circ", "Out Back", "Out Elastic", "Out Bounce"
		};
		int ease_vals[] = {
			iam_ease_linear, iam_ease_out_quad, iam_ease_out_cubic, iam_ease_out_quart, iam_ease_out_quint,
			iam_ease_out_sine, iam_ease_out_expo, iam_ease_out_circ, iam_ease_out_back, iam_ease_out_elastic, iam_ease_out_bounce
		};

		ImGui::SetNextItemWidth(150);
		ImGui::Combo("X Easing##vec2", &ease_x, ease_names, IM_ARRAYSIZE(ease_names));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		ImGui::Combo("Y Easing##vec2", &ease_y, ease_names, IM_ARRAYSIZE(ease_names));

		if (ImGui::Button("Toggle Position##vec2")) {
			toggle = !toggle;
			target_pos = toggle ? ImVec2(400, 150) : ImVec2(50, 50);
		}

		// Draw area
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(500, 200);
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(30, 30, 40, 255));
		ImGui::Dummy(canvas_size);

		// Animate with per-axis easing
		iam_ease_per_axis per_axis(
			iam_ease_preset(ease_vals[ease_x]),
			iam_ease_preset(ease_vals[ease_y])
		);

		ImGuiID id = ImHashStr("per_axis_vec2_demo");
		ImVec2 pos = iam_tween_vec2_per_axis(id, 1, target_pos, 1.5f, per_axis, iam_policy_crossfade, dt);

		// Draw animated circle
		draw->AddCircleFilled(ImVec2(canvas_pos.x + pos.x, canvas_pos.y + pos.y), 15.0f, IM_COL32(100, 200, 255, 255));

		// Draw ghost targets
		draw->AddCircle(ImVec2(canvas_pos.x + 50, canvas_pos.y + 50), 18.0f, IM_COL32(100, 100, 100, 128), 0, 2.0f);
		draw->AddCircle(ImVec2(canvas_pos.x + 400, canvas_pos.y + 150), 18.0f, IM_COL32(100, 100, 100, 128), 0, 2.0f);

		ImGui::TextDisabled("Notice X uses %s, Y uses %s", ease_names[ease_x], ease_names[ease_y]);
		ImGui::TreePop();
	}

	// Demo 2: Color with per-channel easing
	ApplyOpenAll();
	if (ImGui::TreeNode("Color Per-Channel")) {
		static int ease_r = 2;  // Out Cubic
		static int ease_g = 5;  // Out Bounce
		static int ease_b = 4;  // Out Elastic
		static bool toggle_color = false;

		const char* ease_names[] = {
			"Linear", "Out Quad", "Out Cubic", "Out Back", "Out Elastic", "Out Bounce"
		};
		int ease_vals[] = {
			iam_ease_linear, iam_ease_out_quad, iam_ease_out_cubic, iam_ease_out_back, iam_ease_out_elastic, iam_ease_out_bounce
		};

		ImGui::SetNextItemWidth(120);
		ImGui::Combo("R Easing", &ease_r, ease_names, IM_ARRAYSIZE(ease_names));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120);
		ImGui::Combo("G Easing", &ease_g, ease_names, IM_ARRAYSIZE(ease_names));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120);
		ImGui::Combo("B Easing", &ease_b, ease_names, IM_ARRAYSIZE(ease_names));

		if (ImGui::Button("Toggle Color##peraxis")) {
			toggle_color = !toggle_color;
		}

		ImVec4 target_color = toggle_color ? ImVec4(1.0f, 0.8f, 0.0f, 1.0f) : ImVec4(0.2f, 0.4f, 1.0f, 1.0f);

		iam_ease_per_axis per_axis(
			iam_ease_preset(ease_vals[ease_r]),
			iam_ease_preset(ease_vals[ease_g]),
			iam_ease_preset(ease_vals[ease_b]),
			iam_ease_preset(iam_ease_linear) // Alpha stays linear
		);

		ImGuiID id = ImHashStr("per_axis_color_demo");
		ImVec4 color = iam_tween_color_per_axis(id, 1, target_color, 2.0f, per_axis, iam_policy_crossfade, iam_col_srgb, dt);

		ImGui::ColorButton("##color_result", color, 0, ImVec2(200, 60));

		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::Text("R: %.2f (ease: %s)", color.x, ease_names[ease_r]);
		ImGui::Text("G: %.2f (ease: %s)", color.y, ease_names[ease_g]);
		ImGui::Text("B: %.2f (ease: %s)", color.z, ease_names[ease_b]);
		ImGui::EndGroup();

		ImGui::TextDisabled("Each color channel animates with its own easing function.");
		ImGui::TreePop();
	}

	// Demo 3: Practical example - bounce landing effect
	ApplyOpenAll();
	if (ImGui::TreeNode("Bounce Landing Effect")) {
		static float drop_timer = 0.0f;
		static bool dropping = false;

		if (ImGui::Button("Drop!")) {
			dropping = true;
			drop_timer = 0.0f;
		}

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(400, 200);
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(30, 30, 40, 255));

		// Ground line
		float ground_y = canvas_pos.y + canvas_size.y - 30;
		draw->AddLine(ImVec2(canvas_pos.x, ground_y), ImVec2(canvas_pos.x + canvas_size.x, ground_y), IM_COL32(100, 100, 100, 255), 2.0f);
		ImGui::Dummy(canvas_size);

		// Animate: X moves linearly, Y bounces on landing
		ImVec2 start_pos(50, 20);
		ImVec2 end_pos(350, canvas_size.y - 50);

		iam_ease_per_axis per_axis(
			iam_ease_preset(iam_ease_linear),      // X: linear motion
			iam_ease_preset(iam_ease_out_bounce)   // Y: bounce on landing
		);

		ImVec2 target = dropping ? end_pos : start_pos;
		ImGuiID id = ImHashStr("bounce_landing_demo");
		ImVec2 pos = iam_tween_vec2_per_axis(id, 1, target, 1.2f, per_axis, iam_policy_crossfade, dt);

		// Draw ball
		draw->AddCircleFilled(ImVec2(canvas_pos.x + pos.x, canvas_pos.y + pos.y), 20.0f, IM_COL32(255, 100, 100, 255));

		// Reset after animation
		if (dropping) {
			drop_timer += dt;
			if (drop_timer > 2.0f) dropping = false;
		}

		ImGui::TextDisabled("X: linear motion, Y: bounce on landing - creates natural drop effect.");
		ImGui::TreePop();
	}
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
	if (ImGui::TreeNode("Playback Control")) {
		static ImGuiID inst_id = ImHashStr("control_inst");

		ImGui::Text("Controls:");
		if (ImGui::Button("Play##ClipPlayback")) iam_play(CLIP_COMPLEX, inst_id);
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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

	ApplyOpenAll();
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

	ApplyOpenAll();
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

	ApplyOpenAll();
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

	ApplyOpenAll();
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

	ApplyOpenAll();
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

	ApplyOpenAll();
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

	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
	if (ImGui::TreeNode("Practical: Pulsing Button")) {
		float pulse = iam_oscillate(ImGui::GetID("pulse_btn"), 0.1f, 2.0f, iam_wave_sine, 0.0f, dt);
		float scale = 1.0f + pulse;

		// Fixed height container to prevent layout shifts
		float max_scale = 1.1f;  // 1.0 + max amplitude
		float fixed_height = 40 * max_scale + ImGui::GetStyle().ItemSpacing.y;
		ImGui::BeginChild("##PulsingButtonContainer", ImVec2(0, fixed_height), false, ImGuiWindowFlags_NoScrollbar);

		ImGui::SetWindowFontScale(scale);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f + pulse * 0.5f, 0.5f, 0.8f, 1.0f));
		ImGui::Button("Click Me!", ImVec2(120 * scale, 40 * scale));
		ImGui::PopStyleColor();
		ImGui::SetWindowFontScale(1.0f);

		ImGui::SameLine();
		ImGui::TextDisabled("Button pulses continuously");

		ImGui::EndChild();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ApplyOpenAll();
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
	ImGui::SliderFloat("Duration##Scroll", &scroll_duration, 0.1f, 2.0f, "%.1f s");

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
	ImGui::SliderFloat("Duration##MotionPath", &path_duration, 0.5f, 5.0f);

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
// PATH MORPHING DEMO
// ============================================================
static void ShowPathMorphingDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped(
		"Path morphing allows smooth interpolation between two different paths, even if they have "
		"different numbers of control points. Useful for shape transitions and metamorphosis effects.");

	ImGui::Spacing();

	// Initialize paths
	static bool paths_initialized = false;
	static ImGuiID path_circle_id = ImHashStr("morph_circle_path");
	static ImGuiID path_star_id = ImHashStr("morph_star_path");
	static ImGuiID path_wave_id = ImHashStr("morph_wave_path");
	static ImGuiID path_heart_id = ImHashStr("morph_heart_path");

	if (!paths_initialized) {
		// Circle-like path (using bezier approximation)
		float cx = 200, cy = 100, r = 60;
		float k = 0.5522847498f; // bezier circle constant
		iam_path::begin(path_circle_id, ImVec2(cx + r, cy))
			.cubic_to(ImVec2(cx + r, cy + r * k), ImVec2(cx + r * k, cy + r), ImVec2(cx, cy + r))
			.cubic_to(ImVec2(cx - r * k, cy + r), ImVec2(cx - r, cy + r * k), ImVec2(cx - r, cy))
			.cubic_to(ImVec2(cx - r, cy - r * k), ImVec2(cx - r * k, cy - r), ImVec2(cx, cy - r))
			.cubic_to(ImVec2(cx + r * k, cy - r), ImVec2(cx + r, cy - r * k), ImVec2(cx + r, cy))
			.end();

		// Star-like path
		float sr = 70, sir = 30; // outer and inner radius
		ImVec2 star_points[10];
		for (int i = 0; i < 10; i++) {
			float angle = (float)i * IM_PI * 2.0f / 10.0f - IM_PI / 2.0f;
			float rad = (i % 2 == 0) ? sr : sir;
			star_points[i] = ImVec2(cx + rad * cosf(angle), cy + rad * sinf(angle));
		}
		iam_path::begin(path_star_id, star_points[0])
			.line_to(star_points[1]).line_to(star_points[2]).line_to(star_points[3])
			.line_to(star_points[4]).line_to(star_points[5]).line_to(star_points[6])
			.line_to(star_points[7]).line_to(star_points[8]).line_to(star_points[9])
			.line_to(star_points[0])
			.end();

		// Wave path
		iam_path::begin(path_wave_id, ImVec2(100, cy))
			.cubic_to(ImVec2(130, cy - 50), ImVec2(170, cy - 50), ImVec2(200, cy))
			.cubic_to(ImVec2(230, cy + 50), ImVec2(270, cy + 50), ImVec2(300, cy))
			.end();

		// Heart-like path - starts at bottom point, goes up right side, over top, down left side
		iam_path::begin(path_heart_id, ImVec2(cx, cy + 60))  // Bottom point
			.cubic_to(ImVec2(cx + 5, cy + 40), ImVec2(cx + 40, cy + 20), ImVec2(cx + 60, cy - 10))   // Right lower curve
			.cubic_to(ImVec2(cx + 75, cy - 35), ImVec2(cx + 55, cy - 55), ImVec2(cx + 30, cy - 55)) // Right lobe top
			.cubic_to(ImVec2(cx + 10, cy - 55), ImVec2(cx, cy - 40), ImVec2(cx, cy - 30))           // Right to center top
			.cubic_to(ImVec2(cx, cy - 40), ImVec2(cx - 10, cy - 55), ImVec2(cx - 30, cy - 55))      // Center to left lobe
			.cubic_to(ImVec2(cx - 55, cy - 55), ImVec2(cx - 75, cy - 35), ImVec2(cx - 60, cy - 10)) // Left lobe top
			.cubic_to(ImVec2(cx - 40, cy + 20), ImVec2(cx - 5, cy + 40), ImVec2(cx, cy + 60))       // Left lower curve back to point
			.end();

		paths_initialized = true;
	}

	// Demo 1: Manual blend slider
	ApplyOpenAll();
	if (ImGui::TreeNode("Manual Morph Control")) {
		static float blend = 0.0f;
		static int path_a_idx = 0;
		static int path_b_idx = 1;

		const char* path_names[] = { "Circle", "Star", "Wave", "Heart" };
		ImGuiID path_ids[] = { path_circle_id, path_star_id, path_wave_id, path_heart_id };

		ImGui::SetNextItemWidth(100);
		ImGui::Combo("Path A", &path_a_idx, path_names, IM_ARRAYSIZE(path_names));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::Combo("Path B", &path_b_idx, path_names, IM_ARRAYSIZE(path_names));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200);
		ImGui::SliderFloat("Blend", &blend, 0.0f, 1.0f);

		// Draw area
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(400, 200);
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(30, 30, 40, 255));
		ImGui::Dummy(canvas_size);

		// Draw morphed path
		ImGuiID pa = path_ids[path_a_idx];
		ImGuiID pb = path_ids[path_b_idx];

		iam_morph_opts opts;
		opts.samples = 100;

		ImVec2 prev_pt;
		for (int i = 0; i <= 100; i++) {
			float t = (float)i / 100.0f;
			ImVec2 pt = iam_path_morph(pa, pb, t, blend, opts);
			pt.x += canvas_pos.x;
			pt.y += canvas_pos.y;
			if (i > 0) {
				// Color interpolation based on blend
				ImU32 col = IM_COL32(
					(int)(100 + 155 * blend),
					(int)(200 - 100 * blend),
					(int)(255 - 155 * blend),
					255
				);
				draw->AddLine(prev_pt, pt, col, 3.0f);
			}
			prev_pt = pt;
		}

		ImGui::TextDisabled("Drag the blend slider to morph between shapes.");
		ImGui::TreePop();
	}

	// Demo 2: Animated morph
	ApplyOpenAll();
	if (ImGui::TreeNode("Animated Shape Morph")) {
		static float morph_timer = 0.0f;
		static bool animating = false;
		static int from_shape = 0;
		static int to_shape = 1;

		const char* path_names[] = { "Circle", "Star", "Wave", "Heart" };
		ImGuiID path_ids[] = { path_circle_id, path_star_id, path_wave_id, path_heart_id };

		ImGui::SetNextItemWidth(100);
		ImGui::Combo("From##anim", &from_shape, path_names, IM_ARRAYSIZE(path_names));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::Combo("To##anim", &to_shape, path_names, IM_ARRAYSIZE(path_names));
		ImGui::SameLine();

		if (ImGui::Button(animating ? "Reset" : "Morph!")) {
			if (animating) {
				animating = false;
				morph_timer = 0.0f;
			} else {
				animating = true;
				morph_timer = 0.0f;
			}
		}

		// Animate blend
		float duration = 2.0f;
		float blend = 0.0f;
		if (animating) {
			morph_timer += dt;
			float t = ImClamp(morph_timer / duration, 0.0f, 1.0f);
			blend = iam_eval_preset(iam_ease_in_out_cubic, t);
			if (morph_timer > duration + 0.5f) {
				animating = false;
				morph_timer = 0.0f;
			}
		}

		// Draw area
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(400, 200);
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(30, 30, 40, 255));
		ImGui::Dummy(canvas_size);

		// Draw morphed shape
		ImGuiID pa = path_ids[from_shape];
		ImGuiID pb = path_ids[to_shape];
		iam_morph_opts opts;
		opts.samples = 100;

		ImVec2 prev_pt;
		for (int i = 0; i <= 100; i++) {
			float t = (float)i / 100.0f;
			ImVec2 pt = iam_path_morph(pa, pb, t, blend, opts);
			pt.x += canvas_pos.x;
			pt.y += canvas_pos.y;
			if (i > 0) {
				draw->AddLine(prev_pt, pt, IM_COL32(100, 255, 150, 255), 3.0f);
			}
			prev_pt = pt;
		}

		ImGui::Text("Blend: %.2f", blend);
		ImGui::TextDisabled("Click 'Morph!' to animate the shape transition.");
		ImGui::TreePop();
	}

	// Demo 3: Object along morphing path
	ApplyOpenAll();
	if (ImGui::TreeNode("Object Along Morphing Path")) {
		static float path_t = 0.0f;
		static float path_blend = 0.0f;
		static bool animating_path = false;

		ImGui::SliderFloat("Path T", &path_t, 0.0f, 1.0f);
		ImGui::SliderFloat("Morph Blend", &path_blend, 0.0f, 1.0f);

		if (ImGui::Button(animating_path ? "Stop" : "Animate Along Path")) {
			animating_path = !animating_path;
			if (animating_path) path_t = 0.0f;
		}

		if (animating_path) {
			path_t += dt * 0.5f;
			if (path_t > 1.0f) path_t = 0.0f;
		}

		// Draw area
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(400, 200);
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(30, 30, 40, 255));
		ImGui::Dummy(canvas_size);

		iam_morph_opts opts;
		opts.samples = 100;

		// Draw morphed path
		ImVec2 prev_pt;
		for (int i = 0; i <= 100; i++) {
			float t = (float)i / 100.0f;
			ImVec2 pt = iam_path_morph(path_circle_id, path_star_id, t, path_blend, opts);
			pt.x += canvas_pos.x;
			pt.y += canvas_pos.y;
			if (i > 0) {
				draw->AddLine(prev_pt, pt, IM_COL32(80, 80, 100, 255), 2.0f);
			}
			prev_pt = pt;
		}

		// Draw object at current position
		ImVec2 obj_pos = iam_path_morph(path_circle_id, path_star_id, path_t, path_blend, opts);
		obj_pos.x += canvas_pos.x;
		obj_pos.y += canvas_pos.y;

		// Get tangent for rotation
		float angle = iam_path_morph_angle(path_circle_id, path_star_id, path_t, path_blend, opts);

		// Draw rotated triangle
		float size = 12.0f;
		ImVec2 p1(obj_pos.x + size * cosf(angle), obj_pos.y + size * sinf(angle));
		ImVec2 p2(obj_pos.x + size * cosf(angle + 2.5f), obj_pos.y + size * sinf(angle + 2.5f));
		ImVec2 p3(obj_pos.x + size * cosf(angle - 2.5f), obj_pos.y + size * sinf(angle - 2.5f));
		draw->AddTriangleFilled(p1, p2, p3, IM_COL32(255, 200, 100, 255));

		ImGui::TextDisabled("Object follows the morphed path with proper rotation.");
		ImGui::TreePop();
	}
}

// ============================================================
// TEXT ALONG MOTION PATHS DEMO
// ============================================================
static void ShowTextAlongPathDemo()
{
	ImGui::TextWrapped("Text can be animated along motion paths with proper character rotation and constant-speed placement.");

	static bool paths_initialized = false;
	static ImGuiID wave_path_id = ImHashStr("text_wave_path");
	static ImGuiID arc_path_id = ImHashStr("text_arc_path");
	static ImGuiID spiral_path_id = ImHashStr("text_spiral_path");

	// Initialize paths once
	if (!paths_initialized) {
		// Wave path (sine wave using quadratic beziers)
		iam_path::begin(wave_path_id, ImVec2(20, 60))
			.quadratic_to(ImVec2(80, 20), ImVec2(140, 60))
			.quadratic_to(ImVec2(200, 100), ImVec2(260, 60))
			.quadratic_to(ImVec2(320, 20), ImVec2(380, 60))
			.end();

		// Arc path (half circle using cubic bezier approximation)
		float r = 120.0f;
		float cx = 200.0f, cy = 100.0f;
		iam_path::begin(arc_path_id, ImVec2(cx - r, cy))
			.cubic_to(ImVec2(cx - r, cy - r * 0.55f), ImVec2(cx - r * 0.55f, cy - r), ImVec2(cx, cy - r))
			.cubic_to(ImVec2(cx + r * 0.55f, cy - r), ImVec2(cx + r, cy - r * 0.55f), ImVec2(cx + r, cy))
			.end();

		// Spiral path using catmull-rom
		iam_path::begin(spiral_path_id, ImVec2(200, 80))
			.catmull_to(ImVec2(280, 40))
			.catmull_to(ImVec2(340, 80))
			.catmull_to(ImVec2(280, 120))
			.catmull_to(ImVec2(200, 80))
			.catmull_to(ImVec2(140, 50))
			.catmull_to(ImVec2(60, 80))
			.end();

		// Build arc-length LUTs for accurate text placement
		iam_path_build_arc_lut(wave_path_id, 128);
		iam_path_build_arc_lut(arc_path_id, 128);
		iam_path_build_arc_lut(spiral_path_id, 128);

		paths_initialized = true;
	}

	// Animation controls
	static float animation_progress = 0.0f;
	static bool auto_animate = false;
	static float animation_speed = 0.5f;
	static int selected_align = iam_text_align_start;
	static float letter_spacing = 0.0f;
	static float font_scale = 1.0f;

	ImGui::Checkbox("Auto Animate", &auto_animate);
	ImGui::SameLine();
	ImGui::SliderFloat("Speed", &animation_speed, 0.1f, 2.0f);

	if (!auto_animate) {
		ImGui::SliderFloat("Progress", &animation_progress, 0.0f, 1.0f);
	} else {
		float dt = GetSafeDeltaTime();
		animation_progress += dt * animation_speed;
		if (animation_progress > 1.0f) animation_progress = 0.0f;
	}

	static const char* align_names[] = { "Start", "Center", "End" };
	ImGui::Combo("Alignment", &selected_align, align_names, IM_ARRAYSIZE(align_names));
	ImGui::SliderFloat("Letter Spacing", &letter_spacing, -2.0f, 10.0f);
	ImGui::SliderFloat("Font Scale", &font_scale, 0.5f, 2.0f);

	// Demo 1: Wave text
	ImGui::Separator();
	ImGui::Text("Wave Path:");
	{
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(400, 120);
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(20, 25, 35, 255));
		ImGui::Dummy(canvas_size);

		// Draw path
		for (float t = 0; t < 1.0f; t += 0.01f) {
			ImVec2 p1 = iam_path_evaluate(wave_path_id, t);
			ImVec2 p2 = iam_path_evaluate(wave_path_id, t + 0.01f);
			draw->AddLine(
				ImVec2(canvas_pos.x + p1.x, canvas_pos.y + p1.y),
				ImVec2(canvas_pos.x + p2.x, canvas_pos.y + p2.y),
				IM_COL32(60, 60, 80, 255), 1.0f);
		}

		// Draw text along path
		iam_text_path_opts opts;
		opts.origin = canvas_pos;
		opts.align = selected_align;
		opts.letter_spacing = letter_spacing;
		opts.font_scale = font_scale;
		opts.color = IM_COL32(100, 200, 255, 255);
		iam_text_path_animated(wave_path_id, "Hello World!", animation_progress, opts);
	}

	// Demo 2: Arc text
	ImGui::Text("Arc Path:");
	{
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(400, 120);
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(20, 25, 35, 255));
		ImGui::Dummy(canvas_size);

		// Draw path
		for (float t = 0; t < 1.0f; t += 0.01f) {
			ImVec2 p1 = iam_path_evaluate(arc_path_id, t);
			ImVec2 p2 = iam_path_evaluate(arc_path_id, t + 0.01f);
			draw->AddLine(
				ImVec2(canvas_pos.x + p1.x, canvas_pos.y + p1.y),
				ImVec2(canvas_pos.x + p2.x, canvas_pos.y + p2.y),
				IM_COL32(60, 60, 80, 255), 1.0f);
		}

		// Draw text along path
		iam_text_path_opts opts;
		opts.origin = canvas_pos;
		opts.align = iam_text_align_center;
		opts.letter_spacing = letter_spacing;
		opts.font_scale = font_scale;
		opts.color = IM_COL32(255, 200, 100, 255);
		iam_text_path_animated(arc_path_id, "CURVED TEXT", animation_progress, opts);
	}

	// Demo 3: Spiral text (static, not animated)
	ImGui::Text("Spiral Path (Static):");
	{
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(400, 140);
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(20, 25, 35, 255));
		ImGui::Dummy(canvas_size);

		// Draw path
		for (float t = 0; t < 1.0f; t += 0.01f) {
			ImVec2 p1 = iam_path_evaluate(spiral_path_id, t);
			ImVec2 p2 = iam_path_evaluate(spiral_path_id, t + 0.01f);
			draw->AddLine(
				ImVec2(canvas_pos.x + p1.x, canvas_pos.y + p1.y),
				ImVec2(canvas_pos.x + p2.x, canvas_pos.y + p2.y),
				IM_COL32(60, 60, 80, 255), 1.0f);
		}

		// Draw text along path (static)
		iam_text_path_opts opts;
		opts.origin = canvas_pos;
		opts.align = selected_align;
		opts.letter_spacing = letter_spacing;
		opts.font_scale = font_scale;
		opts.color = IM_COL32(100, 255, 150, 255);
		iam_text_path(spiral_path_id, "Following the winding path...", opts);
	}

	ImGui::TextDisabled("Text uses arc-length parameterization for constant character spacing.");
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
// TEXT STAGGER DEMO
// ============================================================
static void ShowTextStaggerDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped("Text stagger animates text character-by-character with various effects. "
		"Each character is animated individually with configurable delay and duration.");

	// Effect selector
	static int effect = iam_text_fx_fade;
	const char* effect_names[] = {
		"None", "Fade", "Scale", "Slide Up", "Slide Down",
		"Slide Left", "Slide Right", "Rotate", "Bounce", "Wave", "Typewriter"
	};
	ImGui::Combo("Effect", &effect, effect_names, IM_ARRAYSIZE(effect_names));

	// Parameters
	static float char_delay = 0.05f;
	static float char_duration = 0.3f;
	static float intensity = 20.0f;
	ImGui::SliderFloat("Char Delay", &char_delay, 0.01f, 0.2f, "%.2f s");
	ImGui::SliderFloat("Char Duration", &char_duration, 0.1f, 1.0f, "%.2f s");
	ImGui::SliderFloat("Intensity", &intensity, 5.0f, 50.0f, "%.0f");

	// Animation control
	static float progress = 0.0f;
	static bool playing = false;
	if (ImGui::Button(playing ? "Reset##TextStagger" : "Play##TextStagger")) {
		playing = !playing;
		progress = 0.0f;
	}
	ImGui::SameLine();
	ImGui::SliderFloat("Progress", &progress, 0.0f, 1.0f);

	if (playing) {
		progress += dt * 0.5f; // 2 seconds for full animation
		if (progress > 1.0f) {
			progress = 1.0f;
			playing = false;
		}
	}

	// Demo text
	const char* demo_text = "Hello, ImAnim!";

	ImGui::Separator();

	// Visual demo
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 80.0f);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(30, 30, 40, 255), 4.0f);

	iam_text_stagger_opts opts;
	opts.pos = ImVec2(canvas_pos.x + 20.0f, canvas_pos.y + canvas_size.y * 0.5f - 10.0f);
	opts.effect = effect;
	opts.char_delay = char_delay;
	opts.char_duration = char_duration;
	opts.effect_intensity = intensity;
	opts.color = IM_COL32(100, 200, 255, 255);

	iam_text_stagger(ImGui::GetID("stagger_demo"), demo_text, progress, opts);

	ImGui::Dummy(canvas_size);

	// Duration info
	float total_duration = iam_text_stagger_duration(demo_text, opts);
	ImGui::Text("Total Duration: %.2f s", total_duration);

	// Multiple effect comparison
	ApplyOpenAll();
	if (ImGui::TreeNode("Effect Comparison")) {
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 size(ImGui::GetContentRegionAvail().x, 300.0f);
		draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(25, 25, 35, 255), 4.0f);

		const char* texts[] = {"Fade In", "Scale Up", "Slide Up", "Bounce!", "Wave~"};
		int effects[] = {iam_text_fx_fade, iam_text_fx_scale, iam_text_fx_slide_up, iam_text_fx_bounce, iam_text_fx_wave};

		for (int i = 0; i < 5; i++) {
			iam_text_stagger_opts o;
			o.pos = ImVec2(pos.x + 20.0f, pos.y + 30.0f + i * 55.0f);
			o.effect = effects[i];
			o.char_delay = 0.04f;
			o.char_duration = 0.25f;
			o.color = IM_COL32(255 - i * 30, 150 + i * 20, 100 + i * 30, 255);

			char id_buf[32];
			snprintf(id_buf, sizeof(id_buf), "stagger_cmp_%d", i);
			iam_text_stagger(ImGui::GetID(id_buf), texts[i], progress, o);
		}

		ImGui::Dummy(size);
		ImGui::TreePop();
	}
}

// ============================================================
// NOISE CHANNELS DEMO
// ============================================================
static void ShowNoiseChannelsDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped("Noise channels provide organic, natural-looking movement using Perlin, Simplex, "
		"or other noise algorithms. Great for idle animations and procedural effects.");

	// Noise type selector
	static int noise_type = iam_noise_perlin;
	ImGui::Combo("Noise Type", &noise_type, "Perlin\0Simplex\0Value\0Worley\0");

	// Noise parameters
	static int octaves = 4;
	static float persistence = 0.5f;
	static float lacunarity = 2.0f;
	ImGui::SliderInt("Octaves", &octaves, 1, 8);
	ImGui::SliderFloat("Persistence", &persistence, 0.1f, 1.0f);
	ImGui::SliderFloat("Lacunarity", &lacunarity, 1.0f, 4.0f);

	static float frequency = 1.0f;
	static float amplitude = 40.0f;
	ImGui::SliderFloat("Frequency", &frequency, 0.1f, 5.0f, "%.1f Hz");
	ImGui::SliderFloat("Amplitude", &amplitude, 10.0f, 100.0f, "%.0f px");

	// 2D Noise visualization
	ApplyOpenAll();
	if (ImGui::TreeNodeEx("2D Noise Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(200, 200);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Draw noise texture preview
		iam_noise_opts opts;
		opts.type = noise_type;
		opts.octaves = octaves;
		opts.persistence = persistence;
		opts.lacunarity = lacunarity;

		static float time_offset = 0.0f;
		time_offset += dt * 0.5f;

		const int res = 50;
		float cell_w = canvas_size.x / res;
		float cell_h = canvas_size.y / res;

		for (int y = 0; y < res; y++) {
			for (int x = 0; x < res; x++) {
				float nx = x * 0.1f + time_offset;
				float ny = y * 0.1f;
				float n = iam_noise(nx, ny, opts);
				n = (n + 1.0f) * 0.5f; // Map from [-1,1] to [0,1]
				ImU8 c = (ImU8)(n * 255);
				ImVec2 p0(canvas_pos.x + x * cell_w, canvas_pos.y + y * cell_h);
				ImVec2 p1(p0.x + cell_w, p0.y + cell_h);
				draw_list->AddRectFilled(p0, p1, IM_COL32(c, c, c, 255));
			}
		}

		draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(100, 100, 100, 255));

		ImGui::Dummy(canvas_size);
		ImGui::TreePop();
	}

	// Animated noise channel demo
	ApplyOpenAll();
	if (ImGui::TreeNodeEx("Animated Noise Channel", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 120.0f);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(30, 30, 40, 255), 4.0f);

		// Center line
		float center_y = canvas_pos.y + canvas_size.y * 0.5f;
		draw_list->AddLine(ImVec2(canvas_pos.x, center_y), ImVec2(canvas_pos.x + canvas_size.x, center_y),
			IM_COL32(80, 80, 80, 100));

		iam_noise_opts opts;
		opts.type = noise_type;
		opts.octaves = octaves;
		opts.persistence = persistence;
		opts.lacunarity = lacunarity;

		// Draw 4 balls with noise-based movement
		ImU32 colors[] = {
			IM_COL32(255, 100, 100, 255),
			IM_COL32(100, 255, 100, 255),
			IM_COL32(100, 100, 255, 255),
			IM_COL32(255, 255, 100, 255)
		};

		for (int i = 0; i < 4; i++) {
			float x = canvas_pos.x + 50.0f + i * (canvas_size.x - 100.0f) / 3.0f;
			char id_buf[32];
			snprintf(id_buf, sizeof(id_buf), "noise_demo_%d", i);

			opts.seed = i * 12345;
			float offset = iam_noise_channel(ImGui::GetID(id_buf), frequency, amplitude, opts, dt);

			draw_list->AddCircleFilled(ImVec2(x, center_y + offset), 12.0f, colors[i]);
			draw_list->AddCircle(ImVec2(x, center_y + offset), 12.0f, IM_COL32(255, 255, 255, 100), 0, 2.0f);
		}

		ImGui::Dummy(canvas_size);
		ImGui::TreePop();
	}

	// 2D noise movement
	ApplyOpenAll();
	if (ImGui::TreeNode("2D Noise Movement")) {
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(200.0f, 200.0f);
		ImVec2 center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(30, 30, 40, 255), 4.0f);

		ImVec2 offset = iam_smooth_noise_vec2(ImGui::GetID("smooth_2d"), ImVec2(amplitude, amplitude), frequency, dt);
		draw_list->AddCircleFilled(ImVec2(center.x + offset.x, center.y + offset.y), 15.0f, IM_COL32(100, 200, 255, 255));

		// Draw trail
		draw_list->AddCircle(center, 3.0f, IM_COL32(100, 100, 100, 150));

		ImGui::Dummy(canvas_size);
		ImGui::TreePop();
	}
}

// ============================================================
// STYLE INTERPOLATION DEMO
// ============================================================
static void ShowStyleInterpolationDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped("Style interpolation smoothly transitions between different ImGui themes. "
		"Colors, padding, spacing, and rounding are all blended. Colors use perceptually uniform color spaces.");

	// Register custom styles with varied parameters
	static bool styles_registered = false;
	static ImGuiID style_compact = ImHashStr("style_compact");
	static ImGuiID style_spacious = ImHashStr("style_spacious");
	static ImGuiID style_rounded = ImHashStr("style_rounded");

	if (!styles_registered) {
		// Compact dark style - tight spacing, sharp corners, small padding
		ImGuiStyle compact;
		ImGui::StyleColorsDark(&compact);
		compact.WindowPadding = ImVec2(4, 4);
		compact.FramePadding = ImVec2(4, 2);
		compact.CellPadding = ImVec2(2, 2);
		compact.ItemSpacing = ImVec2(4, 2);
		compact.ItemInnerSpacing = ImVec2(2, 2);
		compact.IndentSpacing = 12.0f;
		compact.ScrollbarSize = 10.0f;
		compact.GrabMinSize = 8.0f;
		compact.WindowRounding = 0.0f;
		compact.ChildRounding = 0.0f;
		compact.FrameRounding = 0.0f;
		compact.PopupRounding = 0.0f;
		compact.ScrollbarRounding = 0.0f;
		compact.GrabRounding = 0.0f;
		compact.TabRounding = 0.0f;
		compact.WindowBorderSize = 1.0f;
		compact.ChildBorderSize = 1.0f;
		compact.FrameBorderSize = 0.0f;
		compact.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
		compact.Colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.0f);
		compact.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
		compact.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
		compact.Colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.50f, 1.0f);
		compact.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
		compact.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.0f);
		compact.Colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.45f, 1.0f);
		compact.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
		compact.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
		compact.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.32f, 1.0f);
		compact.Colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.50f, 0.55f, 1.0f);
		compact.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.65f, 0.65f, 0.70f, 1.0f);
		compact.Colors[ImGuiCol_CheckMark] = ImVec4(0.70f, 0.70f, 0.75f, 1.0f);
		compact.Colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.35f, 1.0f);
		compact.Colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
		iam_style_register(style_compact, compact);

		// Spacious light style - generous spacing, subtle borders
		ImGuiStyle spacious;
		ImGui::StyleColorsLight(&spacious);
		spacious.WindowPadding = ImVec2(16, 16);
		spacious.FramePadding = ImVec2(12, 6);
		spacious.CellPadding = ImVec2(8, 6);
		spacious.ItemSpacing = ImVec2(12, 8);
		spacious.ItemInnerSpacing = ImVec2(8, 6);
		spacious.IndentSpacing = 24.0f;
		spacious.ScrollbarSize = 16.0f;
		spacious.GrabMinSize = 14.0f;
		spacious.WindowRounding = 4.0f;
		spacious.ChildRounding = 4.0f;
		spacious.FrameRounding = 4.0f;
		spacious.PopupRounding = 4.0f;
		spacious.ScrollbarRounding = 4.0f;
		spacious.GrabRounding = 4.0f;
		spacious.TabRounding = 4.0f;
		spacious.WindowBorderSize = 0.0f;
		spacious.ChildBorderSize = 0.0f;
		spacious.FrameBorderSize = 1.0f;
		spacious.Colors[ImGuiCol_WindowBg] = ImVec4(0.96f, 0.96f, 0.98f, 1.0f);
		spacious.Colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
		spacious.Colors[ImGuiCol_Button] = ImVec4(0.85f, 0.85f, 0.88f, 1.0f);
		spacious.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.78f, 0.78f, 0.82f, 1.0f);
		spacious.Colors[ImGuiCol_ButtonActive] = ImVec4(0.70f, 0.70f, 0.75f, 1.0f);
		spacious.Colors[ImGuiCol_Header] = ImVec4(0.88f, 0.88f, 0.92f, 1.0f);
		spacious.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.80f, 0.80f, 0.85f, 1.0f);
		spacious.Colors[ImGuiCol_HeaderActive] = ImVec4(0.72f, 0.72f, 0.78f, 1.0f);
		spacious.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
		spacious.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.95f, 0.95f, 0.98f, 1.0f);
		spacious.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.90f, 0.95f, 1.0f);
		spacious.Colors[ImGuiCol_SliderGrab] = ImVec4(0.55f, 0.55f, 0.60f, 1.0f);
		spacious.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.40f, 0.45f, 1.0f);
		spacious.Colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.25f, 0.30f, 1.0f);
		spacious.Colors[ImGuiCol_Text] = ImVec4(0.15f, 0.15f, 0.20f, 1.0f);
		spacious.Colors[ImGuiCol_Separator] = ImVec4(0.80f, 0.80f, 0.85f, 1.0f);
		spacious.Colors[ImGuiCol_Border] = ImVec4(0.75f, 0.75f, 0.80f, 1.0f);
		iam_style_register(style_spacious, spacious);

		// Rounded colorful style - pill shapes, vibrant colors
		ImGuiStyle rounded;
		ImGui::StyleColorsDark(&rounded);
		rounded.WindowPadding = ImVec2(12, 12);
		rounded.FramePadding = ImVec2(10, 5);
		rounded.CellPadding = ImVec2(6, 4);
		rounded.ItemSpacing = ImVec2(10, 6);
		rounded.ItemInnerSpacing = ImVec2(6, 4);
		rounded.IndentSpacing = 20.0f;
		rounded.ScrollbarSize = 14.0f;
		rounded.GrabMinSize = 12.0f;
		rounded.WindowRounding = 12.0f;
		rounded.ChildRounding = 12.0f;
		rounded.FrameRounding = 12.0f;
		rounded.PopupRounding = 12.0f;
		rounded.ScrollbarRounding = 12.0f;
		rounded.GrabRounding = 12.0f;
		rounded.TabRounding = 12.0f;
		rounded.WindowBorderSize = 0.0f;
		rounded.ChildBorderSize = 0.0f;
		rounded.FrameBorderSize = 0.0f;
		rounded.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.08f, 0.18f, 1.0f);
		rounded.Colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.10f, 0.22f, 1.0f);
		rounded.Colors[ImGuiCol_Button] = ImVec4(0.45f, 0.25f, 0.70f, 1.0f);
		rounded.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.55f, 0.35f, 0.80f, 1.0f);
		rounded.Colors[ImGuiCol_ButtonActive] = ImVec4(0.65f, 0.45f, 0.90f, 1.0f);
		rounded.Colors[ImGuiCol_Header] = ImVec4(0.40f, 0.22f, 0.60f, 1.0f);
		rounded.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.50f, 0.30f, 0.70f, 1.0f);
		rounded.Colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.40f, 0.80f, 1.0f);
		rounded.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.14f, 0.30f, 1.0f);
		rounded.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.20f, 0.40f, 1.0f);
		rounded.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.25f, 0.50f, 1.0f);
		rounded.Colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.45f, 0.95f, 1.0f);
		rounded.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.85f, 0.60f, 1.00f, 1.0f);
		rounded.Colors[ImGuiCol_CheckMark] = ImVec4(0.85f, 0.55f, 1.00f, 1.0f);
		rounded.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.92f, 1.00f, 1.0f);
		rounded.Colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.35f, 0.70f, 1.0f);
		rounded.Colors[ImGuiCol_Border] = ImVec4(0.45f, 0.30f, 0.65f, 1.0f);
		iam_style_register(style_rounded, rounded);

		styles_registered = true;
	}

	// Style selector
	static int from_style = 0;
	static int to_style = 2;
	const char* style_names[] = {"Compact Dark", "Spacious Light", "Rounded Colorful"};
	ImGuiID style_ids[] = {style_compact, style_spacious, style_rounded};

	ImGui::Combo("From Style", &from_style, style_names, 3);
	ImGui::Combo("To Style", &to_style, style_names, 3);

	// Color space selector (matches iam_color_space enum order)
	static int color_space = iam_col_oklab;
	ImGui::Combo("Color Space", &color_space, "sRGB\0sRGB Linear\0HSV\0OKLAB\0OKLCH\0");

	// Blend control
	static float blend_t = 0.0f;
	static bool animating = false;
	static float anim_dir = 1.0f;

	if (ImGui::Button("Animate")) {
		animating = true;
	}
	ImGui::SameLine();
	ImGui::SliderFloat("Blend", &blend_t, 0.0f, 1.0f);

	if (animating) {
		blend_t += dt * 0.5f * anim_dir;
		if (blend_t >= 1.0f) { blend_t = 1.0f; anim_dir = -1.0f; }
		if (blend_t <= 0.0f) { blend_t = 0.0f; anim_dir = 1.0f; animating = false; }
	}

	// Apply blended style to a child region
	ImGui::Separator();
	ImGui::Text("Preview (blended style applied to child window):");

	// Get blended style
	ImGuiStyle blended;
	iam_style_blend_to(style_ids[from_style], style_ids[to_style], blend_t, &blended, color_space);

	// Apply all style vars
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, blended.WindowPadding);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, blended.FramePadding);
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, blended.CellPadding);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, blended.ItemSpacing);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, blended.ItemInnerSpacing);
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, blended.IndentSpacing);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, blended.ScrollbarSize);
	ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, blended.GrabMinSize);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, blended.ChildRounding);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, blended.FrameRounding);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, blended.ScrollbarRounding);
	ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, blended.GrabRounding);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, blended.ChildBorderSize);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, blended.FrameBorderSize);

	// Apply all colors
	ImGui::PushStyleColor(ImGuiCol_ChildBg, blended.Colors[ImGuiCol_ChildBg]);
	ImGui::PushStyleColor(ImGuiCol_Button, blended.Colors[ImGuiCol_Button]);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, blended.Colors[ImGuiCol_ButtonHovered]);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, blended.Colors[ImGuiCol_ButtonActive]);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, blended.Colors[ImGuiCol_FrameBg]);
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, blended.Colors[ImGuiCol_FrameBgHovered]);
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, blended.Colors[ImGuiCol_FrameBgActive]);
	ImGui::PushStyleColor(ImGuiCol_Text, blended.Colors[ImGuiCol_Text]);
	ImGui::PushStyleColor(ImGuiCol_Header, blended.Colors[ImGuiCol_Header]);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, blended.Colors[ImGuiCol_HeaderHovered]);
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, blended.Colors[ImGuiCol_HeaderActive]);
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, blended.Colors[ImGuiCol_SliderGrab]);
	ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, blended.Colors[ImGuiCol_SliderGrabActive]);
	ImGui::PushStyleColor(ImGuiCol_CheckMark, blended.Colors[ImGuiCol_CheckMark]);
	ImGui::PushStyleColor(ImGuiCol_Separator, blended.Colors[ImGuiCol_Separator]);
	ImGui::PushStyleColor(ImGuiCol_Border, blended.Colors[ImGuiCol_Border]);

	ImGui::BeginChild("StylePreview", ImVec2(0, 280), ImGuiChildFlags_Borders);

	// Row 1: Buttons
	ImGui::Text("Buttons");
	ImGui::Button("Primary");
	ImGui::SameLine();
	ImGui::Button("Secondary");
	ImGui::SameLine();
	ImGui::SmallButton("Small");

	ImGui::Separator();

	// Row 2: Checkboxes and Radio
	ImGui::Text("Toggles");
	static bool check1 = true, check2 = false, check3 = true;
	ImGui::Checkbox("Option A", &check1);
	ImGui::SameLine();
	ImGui::Checkbox("Option B", &check2);
	ImGui::SameLine();
	ImGui::Checkbox("Option C", &check3);

	static int radio_val = 0;
	ImGui::RadioButton("Choice 1", &radio_val, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Choice 2", &radio_val, 1);
	ImGui::SameLine();
	ImGui::RadioButton("Choice 3", &radio_val, 2);

	ImGui::Separator();

	// Row 3: Sliders and Drags
	ImGui::Text("Sliders & Inputs");
	static float slider_val = 0.5f;
	static int int_val = 50;
	static float drag_val = 25.0f;
	ImGui::SliderFloat("Float Slider", &slider_val, 0.0f, 1.0f);
	ImGui::SliderInt("Int Slider", &int_val, 0, 100);
	ImGui::DragFloat("Drag Float", &drag_val, 0.5f, 0.0f, 100.0f);

	ImGui::Separator();

	// Row 4: Text input and Combo
	ImGui::Text("Text & Selection");
	static char text_buf[64] = "Sample text";
	ImGui::InputText("Text Input", text_buf, sizeof(text_buf));
	static int combo_val = 1;
	ImGui::Combo("Combo Box", &combo_val, "Item A\0Item B\0Item C\0Item D\0");

	ImGui::Separator();

	// Row 5: Collapsing header
	if (ImGui::CollapsingHeader("Collapsible Section")) {
		ImGui::Text("Content inside collapsing header");
		ImGui::BulletText("Bullet point 1");
		ImGui::BulletText("Bullet point 2");
	}

	ImGui::EndChild();

	ImGui::PopStyleColor(16);
	ImGui::PopStyleVar(14);

	// Show current interpolated values
	ApplyOpenAll();
	if (ImGui::TreeNode("Interpolated Values")) {
		ImGui::Text("Rounding: Frame=%.1f, Child=%.1f, Grab=%.1f",
			blended.FrameRounding, blended.ChildRounding, blended.GrabRounding);
		ImGui::Text("Padding: Frame=(%.0f,%.0f), Item=(%.0f,%.0f)",
			blended.FramePadding.x, blended.FramePadding.y,
			blended.ItemSpacing.x, blended.ItemSpacing.y);
		ImGui::Text("Borders: Frame=%.0f, Child=%.0f",
			blended.FrameBorderSize, blended.ChildBorderSize);
		ImGui::TreePop();
	}
}

// ============================================================
// DRAG FEEDBACK DEMO
// ============================================================
static void ShowDragFeedbackDemo()
{
	float dt = GetSafeDeltaTime();

	ImGui::TextWrapped("Drag feedback provides animated visual response during drag operations. "
		"Features include grid snapping, snap points, overshoot, and velocity tracking.");

	ImGui::Spacing();

	// Snap grid demo
	ApplyOpenAll();
	if (ImGui::TreeNodeEx("Grid Snapping", ImGuiTreeNodeFlags_DefaultOpen)) {
		static ImVec2 drag_pos(100, 60);
		static bool dragging = false;

		static float grid_size = 50.0f;
		static float snap_duration = 0.3f;
		static float overshoot = 0.5f;
		static int ease_type = iam_ease_out_back;

		ImGui::SliderFloat("Grid Size", &grid_size, 20.0f, 100.0f);
		ImGui::SliderFloat("Snap Duration", &snap_duration, 0.1f, 0.8f);
		ImGui::SliderFloat("Overshoot", &overshoot, 0.0f, 2.0f);

		const char* ease_names[] = { "Out Cubic", "Out Back", "Out Elastic", "Out Bounce" };
		int ease_types[] = { iam_ease_out_cubic, iam_ease_out_back, iam_ease_out_elastic, iam_ease_out_bounce };
		static int ease_idx = 1;
		if (ImGui::Combo("Easing", &ease_idx, ease_names, IM_ARRAYSIZE(ease_names))) {
			ease_type = ease_types[ease_idx];
		}

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300, 200);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Background
		draw_list->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(30, 30, 40, 255), 4.0f);

		// Draw grid
		for (float x = 0; x <= canvas_size.x; x += grid_size) {
			draw_list->AddLine(
				ImVec2(canvas_pos.x + x, canvas_pos.y),
				ImVec2(canvas_pos.x + x, canvas_pos.y + canvas_size.y),
				IM_COL32(60, 60, 70, 150));
		}
		for (float y = 0; y <= canvas_size.y; y += grid_size) {
			draw_list->AddLine(
				ImVec2(canvas_pos.x, canvas_pos.y + y),
				ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + y),
				IM_COL32(60, 60, 70, 150));
		}

		// Handle dragging
		ImGui::InvisibleButton("drag_canvas", canvas_size);
		ImGuiID drag_id = ImGui::GetID("grid_drag");
		iam_drag_feedback feedback;

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
			ImVec2 mouse_pos = ImGui::GetMousePos();
			ImVec2 relative_pos(mouse_pos.x - canvas_pos.x, mouse_pos.y - canvas_pos.y);

			if (!dragging) {
				iam_drag_begin(drag_id, relative_pos);
				dragging = true;
			}
			feedback = iam_drag_update(drag_id, relative_pos, dt);
			drag_pos = feedback.position;
		} else if (dragging) {
			iam_drag_opts opts;
			opts.snap_grid = ImVec2(grid_size, grid_size);
			opts.snap_duration = snap_duration;
			opts.overshoot = overshoot;
			opts.ease_type = ease_type;

			feedback = iam_drag_release(drag_id, drag_pos, opts, dt);
			drag_pos = feedback.position;

			if (!feedback.is_snapping) {
				dragging = false;
			}
		} else {
			// Continue snapping animation if active
			iam_drag_opts opts;
			opts.snap_grid = ImVec2(grid_size, grid_size);
			opts.snap_duration = snap_duration;
			opts.overshoot = overshoot;
			opts.ease_type = ease_type;

			feedback = iam_drag_release(drag_id, drag_pos, opts, dt);
			drag_pos = feedback.position;
		}

		// Draw draggable object
		ImVec2 obj_pos(canvas_pos.x + drag_pos.x, canvas_pos.y + drag_pos.y);
		ImU32 obj_color = dragging ? IM_COL32(255, 200, 100, 255) : IM_COL32(100, 200, 255, 255);
		draw_list->AddCircleFilled(obj_pos, 15.0f, obj_color);
		draw_list->AddCircle(obj_pos, 15.0f, IM_COL32(255, 255, 255, 150), 0, 2.0f);

		ImGui::TextDisabled("Drag the circle and release to see it snap to grid");

		ImGui::TreePop();
	}

	// Snap points demo
	ApplyOpenAll();
	if (ImGui::TreeNode("Snap Points")) {
		static ImVec2 drag_pos2(150, 100);
		static bool dragging2 = false;

		static ImVec2 snap_points[] = {
			ImVec2(50, 50), ImVec2(150, 50), ImVec2(250, 50),
			ImVec2(50, 100), ImVec2(150, 100), ImVec2(250, 100),
			ImVec2(50, 150), ImVec2(150, 150), ImVec2(250, 150)
		};

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300, 200);
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		draw_list->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(30, 30, 40, 255), 4.0f);

		// Draw snap points
		for (int i = 0; i < IM_ARRAYSIZE(snap_points); i++) {
			ImVec2 pt(canvas_pos.x + snap_points[i].x, canvas_pos.y + snap_points[i].y);
			draw_list->AddCircleFilled(pt, 6.0f, IM_COL32(80, 80, 100, 255));
			draw_list->AddCircle(pt, 6.0f, IM_COL32(120, 120, 140, 255));
		}

		// Handle dragging
		ImGui::InvisibleButton("snap_canvas", canvas_size);
		ImGuiID drag_id = ImGui::GetID("points_drag");
		iam_drag_feedback feedback;

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
			ImVec2 mouse_pos = ImGui::GetMousePos();
			ImVec2 relative_pos(mouse_pos.x - canvas_pos.x, mouse_pos.y - canvas_pos.y);

			if (!dragging2) {
				iam_drag_begin(drag_id, relative_pos);
				dragging2 = true;
			}
			feedback = iam_drag_update(drag_id, relative_pos, dt);
			drag_pos2 = feedback.position;
		} else if (dragging2) {
			iam_drag_opts opts;
			opts.snap_points = snap_points;
			opts.snap_points_count = IM_ARRAYSIZE(snap_points);
			opts.snap_duration = 0.25f;
			opts.overshoot = 0.3f;
			opts.ease_type = iam_ease_out_back;

			feedback = iam_drag_release(drag_id, drag_pos2, opts, dt);
			drag_pos2 = feedback.position;

			if (!feedback.is_snapping) {
				dragging2 = false;
			}
		} else {
			iam_drag_opts opts;
			opts.snap_points = snap_points;
			opts.snap_points_count = IM_ARRAYSIZE(snap_points);
			opts.snap_duration = 0.25f;
			opts.overshoot = 0.3f;
			opts.ease_type = iam_ease_out_back;

			feedback = iam_drag_release(drag_id, drag_pos2, opts, dt);
			drag_pos2 = feedback.position;
		}

		// Draw draggable object
		ImVec2 obj_pos(canvas_pos.x + drag_pos2.x, canvas_pos.y + drag_pos2.y);
		ImU32 obj_color = dragging2 ? IM_COL32(255, 200, 100, 255) : IM_COL32(200, 100, 255, 255);
		draw_list->AddCircleFilled(obj_pos, 12.0f, obj_color);

		ImGui::TextDisabled("Drag to snap to nearest point");

		ImGui::TreePop();
	}
}

// ============================================================
// GRADIENT KEYFRAMES DEMO
// ============================================================
static void ShowGradientKeyframesDemo()
{
	ImGui::TextWrapped("Gradient keyframes allow you to interpolate between multi-stop color gradients, "
		"not just single colors. Great for animated backgrounds, health bars, and color themes.");

	float dt = GetSafeDeltaTime();

	// Demo 1: Basic gradient interpolation
	ApplyOpenAll();
	if (ImGui::TreeNode("Basic Gradient Interpolation")) {
		static float blend = 0.5f;
		ImGui::SliderFloat("Blend##GradientBasic", &blend, 0.0f, 1.0f);

		// Create two gradients
		iam_gradient grad_a;
		grad_a.add(0.0f, ImVec4(1.0f, 0.0f, 0.0f, 1.0f))  // Red
			  .add(0.5f, ImVec4(1.0f, 1.0f, 0.0f, 1.0f))  // Yellow
			  .add(1.0f, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green

		iam_gradient grad_b;
		grad_b.add(0.0f, ImVec4(0.0f, 0.5f, 1.0f, 1.0f))  // Blue
			  .add(0.5f, ImVec4(0.5f, 0.0f, 1.0f, 1.0f))  // Purple
			  .add(1.0f, ImVec4(1.0f, 0.0f, 0.5f, 1.0f)); // Pink

		iam_gradient result = iam_gradient_lerp(grad_a, grad_b, blend);

		// Draw gradient bar
		ImVec2 bar_pos = ImGui::GetCursorScreenPos();
		ImVec2 bar_size(300.0f, 30.0f);
		ImDrawList* draw = ImGui::GetWindowDrawList();

		int segments = 50;
		for (int i = 0; i < segments; i++) {
			float t0 = (float)i / segments;
			float t1 = (float)(i + 1) / segments;
			ImVec4 c0 = result.sample(t0);
			ImVec4 c1 = result.sample(t1);
			ImU32 col0 = ImGui::ColorConvertFloat4ToU32(c0);
			ImU32 col1 = ImGui::ColorConvertFloat4ToU32(c1);
			draw->AddRectFilledMultiColor(
				ImVec2(bar_pos.x + t0 * bar_size.x, bar_pos.y),
				ImVec2(bar_pos.x + t1 * bar_size.x, bar_pos.y + bar_size.y),
				col0, col1, col1, col0);
		}
		ImGui::Dummy(bar_size);

		ImGui::TextDisabled("Top gradient: Red -> Yellow -> Green");
		ImGui::TextDisabled("Bottom gradient: Blue -> Purple -> Pink");
		ImGui::TreePop();
	}

	// Demo 2: Animated gradient tween
	ApplyOpenAll();
	if (ImGui::TreeNode("Animated Gradient Tween")) {
		static int target_idx = 0;
		static const char* gradient_names[] = { "Sunset", "Ocean", "Forest", "Neon" };

		// Define gradient presets
		iam_gradient presets[4];

		// Sunset
		presets[0].add(0.0f, ImVec4(1.0f, 0.3f, 0.0f, 1.0f))
				  .add(0.5f, ImVec4(1.0f, 0.6f, 0.2f, 1.0f))
				  .add(1.0f, ImVec4(0.4f, 0.1f, 0.3f, 1.0f));

		// Ocean
		presets[1].add(0.0f, ImVec4(0.0f, 0.3f, 0.6f, 1.0f))
				  .add(0.5f, ImVec4(0.0f, 0.6f, 0.8f, 1.0f))
				  .add(1.0f, ImVec4(0.0f, 0.9f, 0.9f, 1.0f));

		// Forest
		presets[2].add(0.0f, ImVec4(0.1f, 0.3f, 0.1f, 1.0f))
				  .add(0.5f, ImVec4(0.2f, 0.6f, 0.2f, 1.0f))
				  .add(1.0f, ImVec4(0.5f, 0.8f, 0.3f, 1.0f));

		// Neon
		presets[3].add(0.0f, ImVec4(1.0f, 0.0f, 1.0f, 1.0f))
				  .add(0.33f, ImVec4(0.0f, 1.0f, 1.0f, 1.0f))
				  .add(0.66f, ImVec4(1.0f, 1.0f, 0.0f, 1.0f))
				  .add(1.0f, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));

		for (int i = 0; i < 4; i++) {
			if (ImGui::RadioButton(gradient_names[i], target_idx == i)) {
				target_idx = i;
			}
			if (i < 3) ImGui::SameLine();
		}

		iam_gradient current = iam_tween_gradient(
			ImGui::GetID("gradient_tween"),
			ImGui::GetID("ch_gradient"),
			presets[target_idx],
			0.8f,
			iam_ease_preset(iam_ease_out_cubic),
			iam_policy_crossfade,
			iam_col_oklab,
			dt
		);

		// Draw animated gradient bar
		ImVec2 bar_pos = ImGui::GetCursorScreenPos();
		ImVec2 bar_size(300.0f, 40.0f);
		ImDrawList* draw = ImGui::GetWindowDrawList();

		int segments = 60;
		for (int i = 0; i < segments; i++) {
			float t0 = (float)i / segments;
			float t1 = (float)(i + 1) / segments;
			ImVec4 c0 = current.sample(t0);
			ImVec4 c1 = current.sample(t1);
			ImU32 col0 = ImGui::ColorConvertFloat4ToU32(c0);
			ImU32 col1 = ImGui::ColorConvertFloat4ToU32(c1);
			draw->AddRectFilledMultiColor(
				ImVec2(bar_pos.x + t0 * bar_size.x, bar_pos.y),
				ImVec2(bar_pos.x + t1 * bar_size.x, bar_pos.y + bar_size.y),
				col0, col1, col1, col0);
		}
		ImGui::Dummy(bar_size);

		ImGui::TextDisabled("Click presets to see smooth gradient transitions.");
		ImGui::TreePop();
	}

	// Demo 3: Health bar with gradient
	ApplyOpenAll();
	if (ImGui::TreeNode("Health Bar with Gradient")) {
		static float health = 0.75f;
		ImGui::SliderFloat("Health", &health, 0.0f, 1.0f);

		// Gradient from red (low) to yellow (mid) to green (high)
		iam_gradient health_gradient;
		health_gradient.add(0.0f, ImVec4(0.8f, 0.1f, 0.1f, 1.0f))   // Red (critical)
					   .add(0.25f, ImVec4(0.9f, 0.4f, 0.1f, 1.0f))  // Orange (low)
					   .add(0.5f, ImVec4(0.9f, 0.9f, 0.2f, 1.0f))   // Yellow (medium)
					   .add(0.75f, ImVec4(0.4f, 0.8f, 0.3f, 1.0f))  // Light green
					   .add(1.0f, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));  // Green (full)

		// Draw health bar
		ImVec2 bar_pos = ImGui::GetCursorScreenPos();
		ImVec2 bar_size(250.0f, 25.0f);
		ImDrawList* draw = ImGui::GetWindowDrawList();

		// Background
		draw->AddRectFilled(bar_pos, ImVec2(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y),
			IM_COL32(40, 40, 40, 255), 4.0f);

		// Filled portion with gradient
		int segments = 30;
		float fill_width = bar_size.x * health;
		for (int i = 0; i < segments; i++) {
			float t0 = (float)i / segments;
			float t1 = (float)(i + 1) / segments;
			if (t1 * bar_size.x > fill_width) break;

			float sample_t = t0 * health;  // Sample gradient based on fill position
			ImVec4 col = health_gradient.sample(sample_t + (1.0f - health) * 0.5f);
			ImU32 c = ImGui::ColorConvertFloat4ToU32(col);
			draw->AddRectFilled(
				ImVec2(bar_pos.x + t0 * bar_size.x, bar_pos.y),
				ImVec2(bar_pos.x + ImMin(t1 * bar_size.x, fill_width), bar_pos.y + bar_size.y),
				c, 4.0f);
		}

		// Border
		draw->AddRect(bar_pos, ImVec2(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y),
			IM_COL32(100, 100, 100, 255), 4.0f);

		ImGui::Dummy(bar_size);
		ImGui::TextDisabled("Health bar color changes based on value.");
		ImGui::TreePop();
	}
}

// ============================================================
// TRANSFORM INTERPOLATION DEMO
// ============================================================
static void ShowTransformInterpolationDemo()
{
	ImGui::TextWrapped("Transform interpolation allows you to blend 2D transforms (position, rotation, scale) "
		"with proper shortest-path rotation. Great for UI elements, sprites, and complex animations.");

	float dt = GetSafeDeltaTime();

	// Demo 1: Basic transform interpolation
	ApplyOpenAll();
	if (ImGui::TreeNode("Basic Transform Blend")) {
		static float blend = 0.5f;
		ImGui::SliderFloat("Blend##TransformBasic", &blend, 0.0f, 1.0f);

		iam_transform t_a;
		t_a.position = ImVec2(50.0f, 50.0f);
		t_a.rotation = 0.0f;
		t_a.scale = ImVec2(1.0f, 1.0f);

		iam_transform t_b;
		t_b.position = ImVec2(200.0f, 80.0f);
		t_b.rotation = 1.57f;  // 90 degrees
		t_b.scale = ImVec2(1.5f, 0.5f);

		iam_transform result = iam_transform_lerp(t_a, t_b, blend);

		// Draw canvas
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300.0f, 150.0f);
		ImDrawList* draw = ImGui::GetWindowDrawList();

		draw->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(30, 30, 40, 255));

		// Draw transformed rectangle
		float hw = 30.0f * result.scale.x;
		float hh = 20.0f * result.scale.y;
		float cos_r = cosf(result.rotation);
		float sin_r = sinf(result.rotation);

		ImVec2 center(canvas_pos.x + result.position.x, canvas_pos.y + result.position.y);
		ImVec2 corners[4] = {
			ImVec2(-hw, -hh), ImVec2(hw, -hh), ImVec2(hw, hh), ImVec2(-hw, hh)
		};

		ImVec2 transformed[4];
		for (int i = 0; i < 4; i++) {
			transformed[i].x = center.x + corners[i].x * cos_r - corners[i].y * sin_r;
			transformed[i].y = center.y + corners[i].x * sin_r + corners[i].y * cos_r;
		}

		draw->AddQuadFilled(transformed[0], transformed[1], transformed[2], transformed[3],
			IM_COL32(100, 150, 255, 200));
		draw->AddQuad(transformed[0], transformed[1], transformed[2], transformed[3],
			IM_COL32(150, 200, 255, 255), 2.0f);

		ImGui::Dummy(canvas_size);
		ImGui::TextDisabled("Blending position, rotation (90 deg), and non-uniform scale.");
		ImGui::TreePop();
	}

	// Demo 2: Animated transform tween
	ApplyOpenAll();
	if (ImGui::TreeNode("Animated Transform Tween")) {
		static int target_idx = 0;
		static const char* pose_names[] = { "Center", "Top-Left", "Bottom-Right", "Spinning" };

		iam_transform poses[4];

		// Center (default)
		poses[0].position = ImVec2(150.0f, 75.0f);
		poses[0].rotation = 0.0f;
		poses[0].scale = ImVec2(1.0f, 1.0f);

		// Top-left
		poses[1].position = ImVec2(50.0f, 30.0f);
		poses[1].rotation = -0.3f;
		poses[1].scale = ImVec2(0.7f, 0.7f);

		// Bottom-right
		poses[2].position = ImVec2(250.0f, 120.0f);
		poses[2].rotation = 0.5f;
		poses[2].scale = ImVec2(1.3f, 1.3f);

		// Spinning (rotated 180 degrees)
		poses[3].position = ImVec2(150.0f, 75.0f);
		poses[3].rotation = 3.14159f;
		poses[3].scale = ImVec2(1.0f, 1.0f);

		for (int i = 0; i < 4; i++) {
			if (ImGui::RadioButton(pose_names[i], target_idx == i)) {
				target_idx = i;
			}
			if (i < 3) ImGui::SameLine();
		}

		iam_transform current = iam_tween_transform(
			ImGui::GetID("transform_tween"),
			ImGui::GetID("ch_transform"),
			poses[target_idx],
			0.6f,
			iam_ease_preset(iam_ease_out_back),
			iam_policy_crossfade,
			iam_rotation_shortest,
			dt
		);

		// Draw canvas
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300.0f, 150.0f);
		ImDrawList* draw = ImGui::GetWindowDrawList();

		draw->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(30, 30, 40, 255));

		// Draw animated rectangle
		float hw = 25.0f * current.scale.x;
		float hh = 25.0f * current.scale.y;
		float cos_r = cosf(current.rotation);
		float sin_r = sinf(current.rotation);

		ImVec2 center(canvas_pos.x + current.position.x, canvas_pos.y + current.position.y);
		ImVec2 corners[4] = {
			ImVec2(-hw, -hh), ImVec2(hw, -hh), ImVec2(hw, hh), ImVec2(-hw, hh)
		};

		ImVec2 transformed[4];
		for (int i = 0; i < 4; i++) {
			transformed[i].x = center.x + corners[i].x * cos_r - corners[i].y * sin_r;
			transformed[i].y = center.y + corners[i].x * sin_r + corners[i].y * cos_r;
		}

		draw->AddQuadFilled(transformed[0], transformed[1], transformed[2], transformed[3],
			IM_COL32(255, 150, 100, 200));
		draw->AddQuad(transformed[0], transformed[1], transformed[2], transformed[3],
			IM_COL32(255, 200, 150, 255), 2.0f);

		// Draw direction indicator
		ImVec2 arrow_end(center.x + 20.0f * cos_r, center.y + 20.0f * sin_r);
		draw->AddLine(center, arrow_end, IM_COL32(255, 255, 255, 255), 2.0f);

		ImGui::Dummy(canvas_size);
		ImGui::TextDisabled("Uses iam_rotation_shortest (default).");
		ImGui::TreePop();
	}

	// Demo 3: Rotation Modes
	ApplyOpenAll();
	if (ImGui::TreeNode("Rotation Modes")) {
		static int rotation_mode = iam_rotation_shortest;
		static float target_angle = 0.0f;

		ImGui::Text("Rotation Mode:");
		ImGui::RadioButton("Shortest##RotMode", &rotation_mode, iam_rotation_shortest);
		ImGui::SameLine();
		ImGui::RadioButton("Longest##RotMode", &rotation_mode, iam_rotation_longest);
		ImGui::SameLine();
		ImGui::RadioButton("Clockwise##RotMode", &rotation_mode, iam_rotation_cw);
		ImGui::RadioButton("Counter-CW##RotMode", &rotation_mode, iam_rotation_ccw);
		ImGui::SameLine();
		ImGui::RadioButton("Direct##RotMode", &rotation_mode, iam_rotation_direct);

		ImGui::Separator();
		ImGui::Text("Target Angle:");
		if (ImGui::Button("0 deg")) target_angle = 0.0f;
		ImGui::SameLine();
		if (ImGui::Button("90 deg")) target_angle = 1.5708f;
		ImGui::SameLine();
		if (ImGui::Button("180 deg")) target_angle = 3.14159f;
		ImGui::SameLine();
		if (ImGui::Button("270 deg")) target_angle = 4.7124f;
		ImGui::SameLine();
		if (ImGui::Button("360 deg")) target_angle = 6.28318f;

		iam_transform target;
		target.position = ImVec2(150.0f, 75.0f);
		target.rotation = target_angle;
		target.scale = ImVec2(1.0f, 1.0f);

		iam_transform current = iam_tween_transform(
			ImGui::GetID("rotation_mode_demo"),
			ImGui::GetID("ch_rot_mode"),
			target,
			1.0f,
			iam_ease_preset(iam_ease_out_cubic),
			iam_policy_crossfade,
			rotation_mode,
			dt
		);

		// Draw canvas
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300.0f, 150.0f);
		ImDrawList* draw = ImGui::GetWindowDrawList();

		draw->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(30, 30, 40, 255));

		// Draw animated rectangle
		float hw = 40.0f;
		float hh = 25.0f;
		float cos_r = cosf(current.rotation);
		float sin_r = sinf(current.rotation);

		ImVec2 center(canvas_pos.x + current.position.x, canvas_pos.y + current.position.y);
		ImVec2 corners[4] = {
			ImVec2(-hw, -hh), ImVec2(hw, -hh), ImVec2(hw, hh), ImVec2(-hw, hh)
		};

		ImVec2 transformed[4];
		for (int i = 0; i < 4; i++) {
			transformed[i].x = center.x + corners[i].x * cos_r - corners[i].y * sin_r;
			transformed[i].y = center.y + corners[i].x * sin_r + corners[i].y * cos_r;
		}

		draw->AddQuadFilled(transformed[0], transformed[1], transformed[2], transformed[3],
			IM_COL32(100, 200, 150, 200));
		draw->AddQuad(transformed[0], transformed[1], transformed[2], transformed[3],
			IM_COL32(150, 255, 200, 255), 2.0f);

		// Draw direction indicator
		ImVec2 arrow_end(center.x + 30.0f * cos_r, center.y + 30.0f * sin_r);
		draw->AddLine(center, arrow_end, IM_COL32(255, 255, 255, 255), 2.0f);
		draw->AddCircleFilled(arrow_end, 4.0f, IM_COL32(255, 255, 255, 255));

		ImGui::Dummy(canvas_size);

		// Display current angle
		float deg = current.rotation * 57.2958f;
		ImGui::Text("Current: %.1f deg (%.2f rad)", deg, current.rotation);

		ImGui::TextDisabled("Shortest: takes the short way (<180 deg)");
		ImGui::TextDisabled("Longest: takes the long way (>180 deg)");
		ImGui::TextDisabled("CW/CCW: always rotates in one direction");
		ImGui::TextDisabled("Direct: raw lerp (can spin multiple times)");
		ImGui::TreePop();
	}

	// Demo 3: Transform composition
	ApplyOpenAll();
	if (ImGui::TreeNode("Transform Composition")) {
		static float time = 0.0f;
		time += dt;

		// Parent transform (orbiting)
		iam_transform parent;
		parent.position = ImVec2(150.0f, 75.0f);
		parent.rotation = time * 0.5f;
		parent.scale = ImVec2(1.0f, 1.0f);

		// Child transform (relative to parent)
		iam_transform child;
		child.position = ImVec2(50.0f, 0.0f);  // Offset from parent
		child.rotation = time * 2.0f;          // Spinning faster
		child.scale = ImVec2(0.5f, 0.5f);

		// Compose transforms
		iam_transform composed = parent * child;

		// Draw canvas
		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
		ImVec2 canvas_size(300.0f, 150.0f);
		ImDrawList* draw = ImGui::GetWindowDrawList();

		draw->AddRectFilled(canvas_pos,
			ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(30, 30, 40, 255));

		// Draw parent (larger square)
		{
			float hw = 20.0f;
			float hh = 20.0f;
			float cos_r = cosf(parent.rotation);
			float sin_r = sinf(parent.rotation);
			ImVec2 center(canvas_pos.x + parent.position.x, canvas_pos.y + parent.position.y);
			ImVec2 corners[4] = {
				ImVec2(-hw, -hh), ImVec2(hw, -hh), ImVec2(hw, hh), ImVec2(-hw, hh)
			};
			ImVec2 transformed[4];
			for (int i = 0; i < 4; i++) {
				transformed[i].x = center.x + corners[i].x * cos_r - corners[i].y * sin_r;
				transformed[i].y = center.y + corners[i].x * sin_r + corners[i].y * cos_r;
			}
			draw->AddQuadFilled(transformed[0], transformed[1], transformed[2], transformed[3],
				IM_COL32(100, 100, 200, 150));
		}

		// Draw child (smaller square, orbiting)
		{
			float hw = 10.0f * composed.scale.x;
			float hh = 10.0f * composed.scale.y;
			float cos_r = cosf(composed.rotation);
			float sin_r = sinf(composed.rotation);
			ImVec2 center(canvas_pos.x + composed.position.x, canvas_pos.y + composed.position.y);
			ImVec2 corners[4] = {
				ImVec2(-hw, -hh), ImVec2(hw, -hh), ImVec2(hw, hh), ImVec2(-hw, hh)
			};
			ImVec2 transformed[4];
			for (int i = 0; i < 4; i++) {
				transformed[i].x = center.x + corners[i].x * cos_r - corners[i].y * sin_r;
				transformed[i].y = center.y + corners[i].x * sin_r + corners[i].y * cos_r;
			}
			draw->AddQuadFilled(transformed[0], transformed[1], transformed[2], transformed[3],
				IM_COL32(255, 200, 100, 200));
		}

		// Draw connection line
		ImVec2 parent_center(canvas_pos.x + parent.position.x, canvas_pos.y + parent.position.y);
		ImVec2 child_center(canvas_pos.x + composed.position.x, canvas_pos.y + composed.position.y);
		draw->AddLine(parent_center, child_center, IM_COL32(150, 150, 150, 150), 1.0f);

		ImGui::Dummy(canvas_size);
		ImGui::TextDisabled("Blue = parent, Orange = child (orbiting with own spin).");
		ImGui::TreePop();
	}
}

// ============================================================
// ANIMATION INSPECTOR DEMO
// ============================================================
static void ShowAnimationInspectorDemo()
{
	ImGui::TextWrapped("The Unified Inspector provides a complete debug view of all active animations. "
		"Use the 'Show Debug Window' checkbox at the top of this demo to open it.");

	ImGui::Separator();
	ImGui::Text("Inspector Tabs:");
	ImGui::BulletText("Stats - Time scale, tween counts, clip stats, custom easing slots");
	ImGui::BulletText("Clips - Active instances with playback controls and scrubbing");
	ImGui::BulletText("Paths - Registered motion paths with segment info");
	ImGui::BulletText("Noise - Active noise channels with interactive preview");
	ImGui::BulletText("Styles - Registered styles and active style tweens");
	ImGui::BulletText("Performance - Profiler with per-section timing breakdown");

	ImGui::Separator();
	ImGui::TextDisabled("Tip: Use iam_profiler_begin/end() to instrument your code.");
}

// ============================================================
// MAIN DEMO WINDOW
// ============================================================
void ImAnimDemoWindow()
{
	// Start profiler frame
	iam_profiler_begin_frame();

	// Update animation systems
	iam_profiler_begin("iam_update_begin_frame");
	iam_update_begin_frame();
	iam_profiler_end();

	iam_profiler_begin("iam_clip_update");
	iam_clip_update(GetSafeDeltaTime());
	iam_profiler_end();

	ImGui::SetNextWindowSize(ImVec2(650, 750), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Anim Demo")) {
		ImGui::End();
		iam_profiler_end_frame();
		return;
	}

	ImGui::Text("Anim %s", "1.0");
	ImGui::TextDisabled("Animation helpers for Dear ImGui");

	// Debug window toggle
	static bool show_debug_window = false;
	ImGui::Checkbox("Show Debug Window", &show_debug_window);
	ImGui::SameLine();
	ImGui::TextDisabled("(time scale, stats, profiler)");

	// Open/Close all sections (uses global s_open_all)
	if (ImGui::Button("Open All")) {
		s_open_all = 1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Close All")) {
		s_open_all = -1;
	}

	ImGui::Separator();

	// Begin scrollable child for all demo content
	ImGui::BeginChild("DemoContent", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None);

	// ========================================
	// HERO ANIMATION (Showcase)
	// ========================================
	iam_profiler_begin("Hero Animation");
	ShowHeroAnimation();
	iam_profiler_end();

	ImGui::Separator();
	ImGui::Spacing();

	// ========================================
	// 1. EASING & TWEENS
	// ========================================
	ApplyOpenAll();
	if (ImGui::CollapsingHeader("Easing & Tweens", ImGuiTreeNodeFlags_DefaultOpen)) {
		iam_profiler_begin("Easing & Tweens");

		ApplyOpenAll();
		if (ImGui::TreeNode("Easing Functions")) {
			ShowEasingDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Custom Easing")) {
			ShowCustomEasingDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Basic Tweens")) {
			ShowBasicTweensDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Color Tweens")) {
			ShowColorTweensDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Per-Axis Easing")) {
			ShowPerAxisEasingDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Tween Policies")) {
			ShowPoliciesDemo();
			ImGui::TreePop();
		}

		iam_profiler_end();
	}

	// ========================================
	// 2. INTERACTIVE WIDGETS
	// ========================================
	ApplyOpenAll();
	if (ImGui::CollapsingHeader("Interactive Widgets", ImGuiTreeNodeFlags_DefaultOpen)) {
		iam_profiler_begin("Interactive Widgets");
		ShowWidgetsDemo();
		iam_profiler_end();
	}

	// ========================================
	// 3. CLIP-BASED ANIMATIONS
	// ========================================
	ApplyOpenAll();
	if (ImGui::CollapsingHeader("Clip-Based Animations")) {
		iam_profiler_begin("Clip-Based Animations");

		ApplyOpenAll();
		if (ImGui::TreeNode("Clip System")) {
			ShowClipSystemDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Timeline Markers")) {
			ShowTimelineMarkersDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Animation Chaining")) {
			ShowAnimationChainingDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Layering System")) {
			ShowLayeringDemo();
			ImGui::TreePop();
		}

		iam_profiler_end();
	}

	// ========================================
	// 4. PROCEDURAL ANIMATIONS
	// ========================================
	ApplyOpenAll();
	if (ImGui::CollapsingHeader("Procedural Animations")) {
		iam_profiler_begin("Procedural Animations");

		ApplyOpenAll();
		if (ImGui::TreeNode("Oscillators")) {
			ShowOscillatorsDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Shake & Wiggle")) {
			ShowShakeWiggleDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Noise Channels")) {
			ShowNoiseChannelsDemo();
			ImGui::TreePop();
		}

		iam_profiler_end();
	}

	// ========================================
	// 5. MOTION PATHS
	// ========================================
	ApplyOpenAll();
	if (ImGui::CollapsingHeader("Motion Paths")) {
		iam_profiler_begin("Motion Paths");

		ApplyOpenAll();
		if (ImGui::TreeNode("Path Basics")) {
			ShowMotionPathsDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Path Morphing")) {
			ShowPathMorphingDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Text Along Paths")) {
			ShowTextAlongPathDemo();
			ImGui::TreePop();
		}

		iam_profiler_end();
	}

	// ========================================
	// 6. ADVANCED INTERPOLATION
	// ========================================
	ApplyOpenAll();
	if (ImGui::CollapsingHeader("Advanced Interpolation")) {
		iam_profiler_begin("Advanced Interpolation");

		ApplyOpenAll();
		if (ImGui::TreeNode("Gradient Keyframes")) {
			ShowGradientKeyframesDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Transform Interpolation")) {
			ShowTransformInterpolationDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Style Interpolation")) {
			ShowStyleInterpolationDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Text Stagger")) {
			ShowTextStaggerDemo();
			ImGui::TreePop();
		}

		iam_profiler_end();
	}

	// ========================================
	// 7. UTILITIES
	// ========================================
	ApplyOpenAll();
	if (ImGui::CollapsingHeader("Utilities")) {
		iam_profiler_begin("Utilities");

		ApplyOpenAll();
		if (ImGui::TreeNode("ImDrawList Animations")) {
			ShowDrawListDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Resize-Aware Helpers")) {
			ShowResizeHelpersDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Scroll Animation")) {
			ShowScrollDemo();
			ImGui::TreePop();
		}

		ApplyOpenAll();
		if (ImGui::TreeNode("Drag Feedback")) {
			ShowDragFeedbackDemo();
			ImGui::TreePop();
		}

		iam_profiler_end();
	}

	// ========================================
	// 8. DEBUG TOOLS
	// ========================================
	ApplyOpenAll();
	if (ImGui::CollapsingHeader("Debug Tools")) {
		iam_profiler_begin("Debug Tools");
		ShowAnimationInspectorDemo();
		iam_profiler_end();
	}

	// Reset open/close all state after processing all headers
	s_open_all = 0;

	// Footer (inside child)
	ImGui::Separator();
	ImGui::TextDisabled("FPS: %.1f (dt: %.3f ms)", ImGui::GetIO().Framerate, ImGui::GetIO().DeltaTime * 1000.0f);

	ImGui::EndChild();  // End scrollable content

	ImGui::End();

	// Show unified inspector if enabled
	if (show_debug_window) {
		iam_profiler_begin("Unified Inspector");
		iam_show_unified_inspector(&show_debug_window);
		iam_profiler_end();
	}

	// End profiler frame
	iam_profiler_end_frame();
}
