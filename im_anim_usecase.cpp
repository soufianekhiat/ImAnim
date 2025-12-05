// im_anim_usecase.cpp — Mockup UI usecases showcasing im_anim.h in real-world scenarios.
// Author: Soufiane KHIAT
// License: MIT
//
// OPTIONAL: This file is not required to use ImAnim.
// It provides interactive mockups demonstrating practical animation patterns.
// Include it in your project only if you want the usecase showcase.
//
// Usage: Call ImAnimUsecaseWindow() inside your Dear ImGui frame.

#include "imgui.h"
#include "im_anim.h"
#include "imgui_internal.h"
#include <math.h>
#include <stdio.h>

// ============================================================
// HELPER: Get delta time with safety bounds
// ============================================================
static float GetUsecaseDeltaTime()
{
	float dt = ImGui::GetIO().DeltaTime;
	if (dt <= 0.0f) dt = 1.0f / 60.0f;
	if (dt > 0.1f) dt = 0.1f;
	return dt;
}

// ============================================================
// USECASE 1: Animated Button with Hover/Press States
// ============================================================
static void ShowUsecase_AnimatedButton()
{
	ImGui::TextWrapped(
		"Buttons that respond with smooth animations on hover, press, and release. "
		"Demonstrates crossfade policy for seamless state transitions.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Button states
	static bool btn1_hovered = false;
	static bool btn1_pressed = false;
	static bool btn2_hovered = false;
	static bool btn2_pressed = false;
	static bool btn3_hovered = false;
	static bool btn3_pressed = false;

	ImVec2 cursor = ImGui::GetCursorScreenPos();
	float spacing = 20.0f;

	// --- Primary Button (Scale + Color) ---
	{
		ImGuiID id = ImGui::GetID("btn_primary");
		ImVec2 btn_size(140, 44);
		ImVec2 btn_pos = cursor;

		// Invisible button for interaction
		ImGui::SetCursorScreenPos(btn_pos);
		ImGui::InvisibleButton("##btn_primary", btn_size);
		btn1_hovered = ImGui::IsItemHovered();
		btn1_pressed = ImGui::IsItemActive();

		// Determine target scale
		float target_scale = 1.0f;
		if (btn1_pressed) target_scale = 0.95f;
		else if (btn1_hovered) target_scale = 1.05f;

		// Animate scale
		float scale = iam_tween_float(id, ImHashStr("scale"), target_scale, 0.15f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		// Animate color
		ImVec4 base_color(0.2f, 0.5f, 0.9f, 1.0f);
		ImVec4 hover_color(0.3f, 0.6f, 1.0f, 1.0f);
		ImVec4 press_color(0.15f, 0.4f, 0.7f, 1.0f);
		ImVec4 target_color = btn1_pressed ? press_color : (btn1_hovered ? hover_color : base_color);
		ImVec4 color = iam_tween_color(id, ImHashStr("color"), target_color, 0.2f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, iam_col_oklab, dt);

		// Draw scaled button
		ImVec2 center(btn_pos.x + btn_size.x * 0.5f, btn_pos.y + btn_size.y * 0.5f);
		ImVec2 half_size(btn_size.x * 0.5f * scale, btn_size.y * 0.5f * scale);
		dl->AddRectFilled(
			ImVec2(center.x - half_size.x, center.y - half_size.y),
			ImVec2(center.x + half_size.x, center.y + half_size.y),
			ImGui::ColorConvertFloat4ToU32(color), 8.0f);

		// Text
		const char* label = "Primary";
		ImVec2 text_size = ImGui::CalcTextSize(label);
		dl->AddText(ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f),
			IM_COL32_WHITE, label);

		cursor.x += btn_size.x + spacing;
	}

	// --- Ghost Button (Border animation) ---
	{
		ImGuiID id = ImGui::GetID("btn_ghost");
		ImVec2 btn_size(140, 44);
		ImVec2 btn_pos = cursor;

		ImGui::SetCursorScreenPos(btn_pos);
		ImGui::InvisibleButton("##btn_ghost", btn_size);
		btn2_hovered = ImGui::IsItemHovered();
		btn2_pressed = ImGui::IsItemActive();

		// Animate border thickness
		float target_border = btn2_hovered ? 2.5f : 1.5f;
		float border = iam_tween_float(id, ImHashStr("border"), target_border, 0.2f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Animate fill alpha
		float target_fill = btn2_pressed ? 0.15f : (btn2_hovered ? 0.08f : 0.0f);
		float fill_alpha = iam_tween_float(id, ImHashStr("fill"), target_fill, 0.15f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Draw
		ImU32 border_color = IM_COL32(91, 194, 231, 255);
		ImU32 fill_color = IM_COL32(91, 194, 231, (int)(fill_alpha * 255));
		dl->AddRectFilled(btn_pos, ImVec2(btn_pos.x + btn_size.x, btn_pos.y + btn_size.y),
			fill_color, 8.0f);
		dl->AddRect(btn_pos, ImVec2(btn_pos.x + btn_size.x, btn_pos.y + btn_size.y),
			border_color, 8.0f, 0, border);

		const char* label = "Ghost";
		ImVec2 text_size = ImGui::CalcTextSize(label);
		ImVec2 center(btn_pos.x + btn_size.x * 0.5f, btn_pos.y + btn_size.y * 0.5f);
		dl->AddText(ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f),
			border_color, label);

		cursor.x += btn_size.x + spacing;
	}

	// --- Icon Button with Rotation ---
	{
		ImGuiID id = ImGui::GetID("btn_icon");
		ImVec2 btn_size(44, 44);
		ImVec2 btn_pos = cursor;

		ImGui::SetCursorScreenPos(btn_pos);
		ImGui::InvisibleButton("##btn_icon", btn_size);
		btn3_hovered = ImGui::IsItemHovered();
		btn3_pressed = ImGui::IsItemActive();

		// Animate rotation on hover
		float target_rotation = btn3_hovered ? 90.0f : 0.0f;
		float rotation = iam_tween_float(id, ImHashStr("rotation"), target_rotation, 0.3f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		// Animate background
		float target_bg = btn3_hovered ? 0.15f : 0.0f;
		float bg_alpha = iam_tween_float(id, ImHashStr("bg"), target_bg, 0.2f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Draw background
		ImVec2 center(btn_pos.x + btn_size.x * 0.5f, btn_pos.y + btn_size.y * 0.5f);
		dl->AddCircleFilled(center, btn_size.x * 0.5f, IM_COL32(255, 255, 255, (int)(bg_alpha * 255)));

		// Draw rotated "+" icon
		float rad = rotation * 3.14159f / 180.0f;
		float icon_size = 10.0f;
		float c = ImCos(rad), s = ImSin(rad);

		ImVec2 h1(center.x - icon_size * c, center.y - icon_size * s);
		ImVec2 h2(center.x + icon_size * c, center.y + icon_size * s);
		ImVec2 v1(center.x - icon_size * (-s), center.y - icon_size * c);
		ImVec2 v2(center.x + icon_size * (-s), center.y + icon_size * c);

		ImU32 icon_color = btn3_pressed ? IM_COL32(200, 200, 200, 255) : IM_COL32(255, 255, 255, 255);
		dl->AddLine(h1, h2, icon_color, 2.5f);
		dl->AddLine(v1, v2, icon_color, 2.5f);
	}

	ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, cursor.y + 60));
}

// ============================================================
// USECASE 2: Notification Toast System
// ============================================================
static void ShowUsecase_ToastNotifications()
{
	ImGui::TextWrapped(
		"Toast notifications that slide in, hold, then fade out. "
		"Uses clip-based animation with stagger for multiple toasts.");

	float dt = GetUsecaseDeltaTime();

	// Toast state
	static float toast_timers[3] = { 0.0f, 0.0f, 0.0f };
	static bool toast_active[3] = { false, false, false };
	static int toast_type[3] = { 0, 1, 2 }; // 0=success, 1=warning, 2=error

	// Buttons to trigger toasts
	if (ImGui::Button("Success Toast")) { toast_active[0] = true; toast_timers[0] = 0.0f; }
	ImGui::SameLine();
	if (ImGui::Button("Warning Toast")) { toast_active[1] = true; toast_timers[1] = 0.0f; }
	ImGui::SameLine();
	if (ImGui::Button("Error Toast")) { toast_active[2] = true; toast_timers[2] = 0.0f; }

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 180);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Background
	dl->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(20, 20, 25, 255), 4.0f);

	const char* messages[3] = { "Operation completed successfully!", "Warning: Check your settings", "Error: Something went wrong" };
	ImVec4 colors[3] = {
		ImVec4(0.2f, 0.8f, 0.4f, 1.0f),  // Success green
		ImVec4(0.9f, 0.7f, 0.2f, 1.0f),  // Warning yellow
		ImVec4(0.9f, 0.3f, 0.3f, 1.0f)   // Error red
	};

	// Draw toasts
	float toast_height = 50.0f;
	float toast_spacing = 8.0f;
	float toast_padding = 32.0f;  // Padding on both sides

	for (int i = 0; i < 3; i++)
	{
		if (!toast_active[i]) continue;

		toast_timers[i] += dt;
		float t = toast_timers[i];

		// Toast lifecycle: 0-0.3s slide in, 0.3-2.3s hold, 2.3-2.6s fade out
		float slide_progress = 0.0f;
		float alpha = 1.0f;

		if (t < 0.3f)
		{
			// Slide in from right
			slide_progress = iam_eval_preset(iam_ease_out_back, t / 0.3f);
		}
		else if (t < 2.3f)
		{
			// Hold
			slide_progress = 1.0f;
		}
		else if (t < 2.6f)
		{
			// Fade out and slide up
			float fade_t = (t - 2.3f) / 0.3f;
			slide_progress = 1.0f;
			alpha = 1.0f - iam_eval_preset(iam_ease_in_quad, fade_t);
		}
		else
		{
			toast_active[i] = false;
			continue;
		}

		// Calculate width based on text size
		ImVec2 text_size = ImGui::CalcTextSize(messages[toast_type[i]]);
		float toast_width = text_size.x + toast_padding;

		float base_x = canvas_pos.x + canvas_size.x - toast_width - 16.0f;
		float base_y = canvas_pos.y + 16.0f + i * (toast_height + toast_spacing);

		float x = base_x + (1.0f - slide_progress) * (toast_width + 32.0f);
		float y = base_y;

		// Draw toast
		ImVec4 col = colors[toast_type[i]];
		col.w = alpha;
		ImU32 bg_color = IM_COL32(40, 40, 50, (int)(alpha * 230));
		ImU32 accent_color = ImGui::ColorConvertFloat4ToU32(col);

		dl->AddRectFilled(ImVec2(x, y), ImVec2(x + toast_width, y + toast_height), bg_color, 6.0f);
		dl->AddRectFilled(ImVec2(x, y), ImVec2(x + 4.0f, y + toast_height), accent_color, 6.0f, ImDrawFlags_RoundCornersLeft);

		// Text
		ImU32 text_color = IM_COL32(255, 255, 255, (int)(alpha * 255));
		dl->AddText(ImVec2(x + 16.0f, y + (toast_height - ImGui::GetTextLineHeight()) * 0.5f),
			text_color, messages[toast_type[i]]);
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE 3: Sidebar Navigation with Slide Animation
// ============================================================
static void ShowUsecase_SidebarNavigation()
{
	ImGui::TextWrapped(
		"Collapsible sidebar with smooth slide animation. "
		"Menu items stagger their entrance for a polished feel.");

	float dt = GetUsecaseDeltaTime();
	static bool sidebar_open = true;
	static float sidebar_toggle_time = 0.0f;

	if (ImGui::Button(sidebar_open ? "Close Sidebar" : "Open Sidebar"))
	{
		sidebar_open = !sidebar_open;
		sidebar_toggle_time = 0.0f;
	}

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 200);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Background (main content area)
	dl->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(30, 32, 40, 255), 4.0f);

	// Animate sidebar width
	ImGuiID id = ImGui::GetID("sidebar");
	float target_width = sidebar_open ? 180.0f : 0.0f;
	float sidebar_width = iam_tween_float(id, ImHashStr("width"), target_width, 0.35f,
		iam_ease_preset(iam_ease_out_expo), iam_policy_crossfade, dt);

	// Draw sidebar
	if (sidebar_width > 1.0f)
	{
		dl->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + sidebar_width, canvas_pos.y + canvas_size.y),
			IM_COL32(45, 48, 60, 255), 4.0f, ImDrawFlags_RoundCornersLeft);

		// Menu items with stagger
		const char* menu_items[5] = { "Dashboard", "Analytics", "Projects", "Settings", "Help" };
		float item_height = 36.0f;
		float stagger_delay = 0.05f;

		for (int i = 0; i < 5; i++)
		{
			// Calculate stagger offset for each item
			float item_delay = i * stagger_delay;
			float item_progress = 0.0f;

			if (sidebar_open)
			{
				// Items slide in with stagger
				float t = ImClamp((sidebar_width / 180.0f - item_delay * 2.0f) * 3.0f, 0.0f, 1.0f);
				item_progress = iam_eval_preset(iam_ease_out_cubic, t);
			}
			else
			{
				item_progress = sidebar_width / 180.0f;
			}

			float item_x = canvas_pos.x + 12.0f + (1.0f - item_progress) * (-50.0f);
			float item_y = canvas_pos.y + 16.0f + i * item_height;
			int alpha = (int)(item_progress * 255);

			dl->AddText(ImVec2(item_x, item_y + (item_height - ImGui::GetTextLineHeight()) * 0.5f),
				IM_COL32(200, 200, 210, alpha), menu_items[i]);
		}
	}

	// Content area indicator
	float content_x = canvas_pos.x + sidebar_width + 20.0f;
	dl->AddText(ImVec2(content_x, canvas_pos.y + canvas_size.y * 0.5f - 8.0f),
		IM_COL32(100, 100, 110, 255), "Main Content Area");

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE 4: Loading Spinner Variations
// ============================================================
static void ShowUsecase_LoadingSpinners()
{
	ImGui::TextWrapped(
		"Various loading spinner styles using oscillators and tweens. "
		"Demonstrates continuous animations with different waveforms.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 cursor = ImGui::GetCursorScreenPos();

	float spinner_size = 40.0f;
	float spacing = 80.0f;

	// --- Spinner 1: Classic rotating dots ---
	{
		ImGuiID id = ImGui::GetID("spinner1");
		ImVec2 center(cursor.x + spinner_size, cursor.y + spinner_size);

		for (int i = 0; i < 8; i++)
		{
			float angle = (float)i / 8.0f * 3.14159f * 2.0f;
			float phase = iam_oscillate(id + i, 1.0f, 1.5f, iam_wave_sine, (float)i / 8.0f, dt);
			float dot_alpha = 0.3f + (phase * 0.5f + 0.5f) * 0.7f;
			float dot_size = 3.0f + (phase * 0.5f + 0.5f) * 2.0f;

			float x = center.x + ImCos(angle) * (spinner_size * 0.6f);
			float y = center.y + ImSin(angle) * (spinner_size * 0.6f);

			dl->AddCircleFilled(ImVec2(x, y), dot_size, IM_COL32(91, 194, 231, (int)(dot_alpha * 255)));
		}
		dl->AddText(ImVec2(cursor.x, cursor.y + spinner_size * 2 + 8), IM_COL32(150, 150, 150, 255), "Pulse");
		cursor.x += spacing;
	}

	// --- Spinner 2: Arc spinner ---
	{
		ImGuiID id = ImGui::GetID("spinner2");
		ImVec2 center(cursor.x + spinner_size, cursor.y + spinner_size);

		// Continuous rotation using oscillator as time source
		static float rotation = 0.0f;
		rotation += dt * 4.0f;

		float arc_start = rotation;
		float arc_length = 1.5f + ImSin(rotation * 0.5f) * 0.5f;

		// Draw arc as line segments
		int segments = 20;
		for (int i = 0; i < segments; i++)
		{
			float t = (float)i / segments;
			float a1 = arc_start + t * arc_length;
			float a2 = arc_start + (t + 1.0f / segments) * arc_length;
			float alpha = t;

			ImVec2 p1(center.x + ImCos(a1) * spinner_size * 0.6f, center.y + ImSin(a1) * spinner_size * 0.6f);
			ImVec2 p2(center.x + ImCos(a2) * spinner_size * 0.6f, center.y + ImSin(a2) * spinner_size * 0.6f);

			dl->AddLine(p1, p2, IM_COL32(91, 194, 231, (int)(alpha * 255)), 3.0f);
		}
		dl->AddText(ImVec2(cursor.x, cursor.y + spinner_size * 2 + 8), IM_COL32(150, 150, 150, 255), "Arc");
		cursor.x += spacing;
	}

	// --- Spinner 3: Bouncing dots ---
	{
		ImGuiID id = ImGui::GetID("spinner3");
		float base_y = cursor.y + spinner_size;

		for (int i = 0; i < 3; i++)
		{
			float bounce = iam_oscillate(id + i, 15.0f, 2.0f, iam_wave_sine, (float)i * 0.15f, dt);
			float x = cursor.x + spinner_size * 0.5f + i * 16.0f;
			float y = base_y - fabsf(bounce);

			dl->AddCircleFilled(ImVec2(x, y), 5.0f, IM_COL32(204, 120, 88, 255));
		}
		dl->AddText(ImVec2(cursor.x, cursor.y + spinner_size * 2 + 8), IM_COL32(150, 150, 150, 255), "Bounce");
		cursor.x += spacing;
	}

	// --- Spinner 4: Morphing square ---
	{
		ImGuiID id = ImGui::GetID("spinner4");
		ImVec2 center(cursor.x + spinner_size, cursor.y + spinner_size);

		static float morph_time = 0.0f;
		morph_time += dt;

		float morph = (ImSin(morph_time * 2.0f) * 0.5f + 0.5f);
		float rotation = morph_time * 1.5f;
		float size = spinner_size * 0.5f;

		// Interpolate between square and circle (approximate with rounded rect)
		float rounding = morph * size;

		float c = ImCos(rotation), s = ImSin(rotation);
		ImVec2 corners[4];
		for (int i = 0; i < 4; i++)
		{
			float angle = (float)i * 3.14159f * 0.5f + 3.14159f * 0.25f;
			float x = ImCos(angle) * size;
			float y = ImSin(angle) * size;
			corners[i] = ImVec2(center.x + x * c - y * s, center.y + x * s + y * c);
		}

		// Draw with varying alpha for glow effect
		for (int layer = 2; layer >= 0; layer--)
		{
			float scale = 1.0f + layer * 0.1f;
			int alpha = 255 / (layer + 1);
			ImVec2 scaled_corners[4];
			for (int i = 0; i < 4; i++)
			{
				scaled_corners[i] = ImVec2(
					center.x + (corners[i].x - center.x) * scale,
					center.y + (corners[i].y - center.y) * scale);
			}
			dl->AddConvexPolyFilled(scaled_corners, 4, IM_COL32(91, 194, 231, alpha));
		}
		dl->AddText(ImVec2(cursor.x, cursor.y + spinner_size * 2 + 8), IM_COL32(150, 150, 150, 255), "Morph");
	}

	ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, cursor.y + spinner_size * 2 + 30));
}

// ============================================================
// USECASE 5: Card Hover Effects
// ============================================================
static void ShowUsecase_CardHover()
{
	ImGui::TextWrapped(
		"Card components with lift, shadow, and content reveal on hover. "
		"Shows coordinated multi-property animations.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 start_pos = ImGui::GetCursorScreenPos();

	ImVec2 card_size(160, 120);
	float spacing = 20.0f;

	static bool card_hovered[3] = { false, false, false };
	const char* titles[3] = { "Analytics", "Reports", "Settings" };
	const char* descriptions[3] = { "View insights", "Export data", "Configure" };

	for (int i = 0; i < 3; i++)
	{
		ImGuiID id = ImGui::GetID(titles[i]);
		ImVec2 card_pos(start_pos.x + i * (card_size.x + spacing), start_pos.y);

		// Invisible button for interaction
		ImGui::SetCursorScreenPos(card_pos);
		ImGui::InvisibleButton(titles[i], card_size);
		card_hovered[i] = ImGui::IsItemHovered();

		// Animate lift (Y offset)
		float target_lift = card_hovered[i] ? -8.0f : 0.0f;
		float lift = iam_tween_float(id, ImHashStr("lift"), target_lift, 0.25f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Animate shadow
		float target_shadow = card_hovered[i] ? 20.0f : 8.0f;
		float shadow_blur = iam_tween_float(id, ImHashStr("shadow"), target_shadow, 0.25f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Animate border color
		ImVec4 base_border(0.3f, 0.3f, 0.35f, 1.0f);
		ImVec4 hover_border(0.35f, 0.76f, 0.9f, 1.0f);
		ImVec4 target_border = card_hovered[i] ? hover_border : base_border;
		ImVec4 border_color = iam_tween_color(id, ImHashStr("border"), target_border, 0.2f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, iam_col_oklab, dt);

		// Animate description reveal
		float target_desc_alpha = card_hovered[i] ? 1.0f : 0.0f;
		float desc_alpha = iam_tween_float(id, ImHashStr("desc"), target_desc_alpha, 0.2f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Apply lift
		ImVec2 drawn_pos(card_pos.x, card_pos.y + lift);

		// Draw shadow (multiple layers for blur effect)
		for (int s = 3; s >= 0; s--)
		{
			float offset = shadow_blur * 0.2f * (s + 1);
			int alpha = 15 - s * 3;
			dl->AddRectFilled(
				ImVec2(drawn_pos.x + offset * 0.5f, drawn_pos.y + offset),
				ImVec2(drawn_pos.x + card_size.x + offset * 0.5f, drawn_pos.y + card_size.y + offset),
				IM_COL32(0, 0, 0, alpha), 8.0f);
		}

		// Draw card background
		dl->AddRectFilled(drawn_pos, ImVec2(drawn_pos.x + card_size.x, drawn_pos.y + card_size.y),
			IM_COL32(45, 48, 58, 255), 8.0f);
		dl->AddRect(drawn_pos, ImVec2(drawn_pos.x + card_size.x, drawn_pos.y + card_size.y),
			ImGui::ColorConvertFloat4ToU32(border_color), 8.0f, 0, 1.5f);

		// Draw title
		ImVec2 title_pos(drawn_pos.x + 16.0f, drawn_pos.y + 20.0f);
		dl->AddText(title_pos, IM_COL32(220, 220, 230, 255), titles[i]);

		// Draw description with animated alpha
		ImVec2 desc_pos(drawn_pos.x + 16.0f, drawn_pos.y + 50.0f);
		dl->AddText(desc_pos, IM_COL32(140, 140, 150, (int)(desc_alpha * 255)), descriptions[i]);

		// Draw icon placeholder
		ImVec2 icon_center(drawn_pos.x + card_size.x - 30.0f, drawn_pos.y + 30.0f);
		dl->AddCircleFilled(icon_center, 12.0f, IM_COL32(91, 194, 231, 100 + (int)(card_hovered[i] ? 100 : 0)));
	}

	ImGui::SetCursorScreenPos(ImVec2(start_pos.x, start_pos.y + card_size.y + 20));
}

// ============================================================
// USECASE 6: Tab Transition Animation
// ============================================================
static void ShowUsecase_TabTransitions()
{
	ImGui::TextWrapped(
		"Tab switching with smooth underline animation and content crossfade. "
		"The indicator follows the active tab with spring physics.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static int active_tab = 0;
	const char* tab_names[4] = { "Overview", "Details", "History", "Settings" };
	float tab_widths[4];

	ImVec2 tabs_pos = ImGui::GetCursorScreenPos();
	float tab_height = 36.0f;

	// Calculate tab widths
	float total_width = 0.0f;
	for (int i = 0; i < 4; i++)
	{
		tab_widths[i] = ImGui::CalcTextSize(tab_names[i]).x + 32.0f;
		total_width += tab_widths[i];
	}

	// Draw tab background
	dl->AddRectFilled(tabs_pos, ImVec2(tabs_pos.x + total_width, tabs_pos.y + tab_height),
		IM_COL32(35, 38, 48, 255), 4.0f, ImDrawFlags_RoundCornersTop);

	// Calculate target indicator position
	float target_x = tabs_pos.x;
	for (int i = 0; i < active_tab; i++)
		target_x += tab_widths[i];
	float target_width = tab_widths[active_tab];

	// Animate indicator with spring
	ImGuiID id = ImGui::GetID("tab_indicator");
	float indicator_x = iam_tween_float(id, ImHashStr("x"), target_x, 0.3f,
		iam_ease_spring_desc(1.0f, 180.0f, 18.0f, 0.0f), iam_policy_crossfade, dt);
	float indicator_width = iam_tween_float(id, ImHashStr("w"), target_width, 0.25f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	// Draw tabs
	float x = tabs_pos.x;
	for (int i = 0; i < 4; i++)
	{
		ImVec2 tab_min(x, tabs_pos.y);
		ImVec2 tab_max(x + tab_widths[i], tabs_pos.y + tab_height);

		// Invisible button
		ImGui::SetCursorScreenPos(tab_min);
		char btn_id[32];
		snprintf(btn_id, sizeof(btn_id), "##tab%d", i);
		if (ImGui::InvisibleButton(btn_id, ImVec2(tab_widths[i], tab_height)))
			active_tab = i;

		bool hovered = ImGui::IsItemHovered();

		// Text color animation
		ImGuiID tab_id = ImGui::GetID(tab_names[i]);
		float target_alpha = (i == active_tab) ? 1.0f : (hovered ? 0.8f : 0.5f);
		float text_alpha = iam_tween_float(tab_id, ImHashStr("alpha"), target_alpha, 0.15f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Draw text
		ImVec2 text_size = ImGui::CalcTextSize(tab_names[i]);
		ImVec2 text_pos(x + (tab_widths[i] - text_size.x) * 0.5f, tabs_pos.y + (tab_height - text_size.y) * 0.5f);
		dl->AddText(text_pos, IM_COL32(255, 255, 255, (int)(text_alpha * 255)), tab_names[i]);

		x += tab_widths[i];
	}

	// Draw animated indicator
	float indicator_y = tabs_pos.y + tab_height - 3.0f;
	dl->AddRectFilled(ImVec2(indicator_x + 8.0f, indicator_y),
		ImVec2(indicator_x + indicator_width - 8.0f, indicator_y + 3.0f),
		IM_COL32(91, 194, 231, 255), 2.0f);

	// Content area with fade
	ImVec2 content_pos(tabs_pos.x, tabs_pos.y + tab_height + 8.0f);
	ImVec2 content_size(total_width, 80.0f);

	dl->AddRectFilled(content_pos, ImVec2(content_pos.x + content_size.x, content_pos.y + content_size.y),
		IM_COL32(30, 32, 40, 255), 4.0f);

	// Animate content alpha
	float content_alpha = iam_tween_float(id, ImHashStr("content"), 1.0f, 0.2f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	const char* contents[4] = {
		"Overview: Quick summary of your data",
		"Details: In-depth analysis and metrics",
		"History: Past records and changes",
		"Settings: Configure your preferences"
	};
	dl->AddText(ImVec2(content_pos.x + 16.0f, content_pos.y + 30.0f),
		IM_COL32(180, 180, 190, (int)(content_alpha * 255)), contents[active_tab]);

	ImGui::SetCursorScreenPos(ImVec2(tabs_pos.x, content_pos.y + content_size.y + 16.0f));
}

// ============================================================
// USECASE 7: Progress Bar with Segments
// ============================================================
static void ShowUsecase_ProgressBar()
{
	ImGui::TextWrapped(
		"Animated progress bar with segmented fill and glow effects. "
		"Uses color interpolation in OKLAB for smooth gradients.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static float target_progress = 0.65f;
	ImGui::SliderFloat("Progress", &target_progress, 0.0f, 1.0f);

	ImGuiID id = ImGui::GetID("progress_bar");

	// Animate progress value
	float progress = iam_tween_float(id, ImHashStr("value"), target_progress, 0.5f,
		iam_ease_preset(iam_ease_out_expo), iam_policy_crossfade, dt);

	ImVec2 bar_pos = ImGui::GetCursorScreenPos();
	ImVec2 bar_size(ImGui::GetContentRegionAvail().x - 20.0f, 24.0f);

	// Background
	dl->AddRectFilled(bar_pos, ImVec2(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y),
		IM_COL32(30, 32, 40, 255), 6.0f);

	// Filled portion with gradient
	float filled_width = bar_size.x * progress;
	if (filled_width > 2.0f)
	{
		// Gradient from cyan to green based on progress
		ImVec4 start_col(0.2f, 0.6f, 0.9f, 1.0f);
		ImVec4 end_col(0.2f, 0.9f, 0.5f, 1.0f);
		ImVec4 fill_color = iam_get_blended_color(start_col, end_col, progress, iam_col_oklab);

		dl->AddRectFilled(bar_pos, ImVec2(bar_pos.x + filled_width, bar_pos.y + bar_size.y),
			ImGui::ColorConvertFloat4ToU32(fill_color), 6.0f, ImDrawFlags_RoundCornersLeft);

		// Glow effect at the edge
		float glow_x = bar_pos.x + filled_width - 4.0f;
		for (int i = 0; i < 4; i++)
		{
			float alpha = 0.3f * (1.0f - i * 0.25f);
			float offset = i * 4.0f;
			dl->AddRectFilled(
				ImVec2(glow_x - offset, bar_pos.y),
				ImVec2(glow_x + 4.0f, bar_pos.y + bar_size.y),
				IM_COL32(255, 255, 255, (int)(alpha * 255 * (1.0f - i * 0.2f))), 4.0f);
		}

		// Animated shimmer
		static float shimmer_time = 0.0f;
		shimmer_time += dt;
		float shimmer_pos = ImFmod(shimmer_time * 0.5f, 1.0f) * filled_width;
		float shimmer_width = 60.0f;

		if (shimmer_pos < filled_width)
		{
			float shimmer_alpha = 0.15f * ImSin(shimmer_pos / filled_width * 3.14159f);
			dl->AddRectFilledMultiColor(
				ImVec2(bar_pos.x + shimmer_pos, bar_pos.y),
				ImVec2(bar_pos.x + shimmer_pos + shimmer_width, bar_pos.y + bar_size.y),
				IM_COL32(255, 255, 255, 0),
				IM_COL32(255, 255, 255, (int)(shimmer_alpha * 255)),
				IM_COL32(255, 255, 255, (int)(shimmer_alpha * 255)),
				IM_COL32(255, 255, 255, 0));
		}
	}

	// Percentage text
	char percent_text[16];
	snprintf(percent_text, sizeof(percent_text), "%.0f%%", progress * 100.0f);
	ImVec2 text_size = ImGui::CalcTextSize(percent_text);
	ImVec2 text_pos(bar_pos.x + (bar_size.x - text_size.x) * 0.5f,
		bar_pos.y + (bar_size.y - text_size.y) * 0.5f);
	dl->AddText(text_pos, IM_COL32(255, 255, 255, 200), percent_text);

	ImGui::Dummy(ImVec2(bar_size.x, bar_size.y + 16.0f));
}

// ============================================================
// USECASE 8: Modal Dialog Animation
// ============================================================
static void ShowUsecase_ModalDialog()
{
	ImGui::TextWrapped(
		"Modal dialog with backdrop fade and content scale-in animation. "
		"Demonstrates layered animations with different timing.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static bool modal_open = false;
	static float modal_time = 0.0f;

	if (ImGui::Button("Open Modal"))
	{
		modal_open = true;
		modal_time = 0.0f;
	}

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 200);

	// Draw canvas background (represents the app)
	dl->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	// Fake app content
	dl->AddText(ImVec2(canvas_pos.x + 20.0f, canvas_pos.y + 20.0f),
		IM_COL32(100, 100, 110, 255), "Application Content");
	for (int i = 0; i < 3; i++)
	{
		dl->AddRectFilled(
			ImVec2(canvas_pos.x + 20.0f, canvas_pos.y + 50.0f + i * 40.0f),
			ImVec2(canvas_pos.x + canvas_size.x * 0.7f, canvas_pos.y + 80.0f + i * 40.0f),
			IM_COL32(45, 48, 58, 255), 4.0f);
	}

	if (modal_open)
	{
		modal_time += dt;
		ImGuiID id = ImGui::GetID("modal");

		// Animate backdrop
		float target_backdrop = 1.0f;
		float backdrop = iam_tween_float(id, ImHashStr("backdrop"), target_backdrop, 0.25f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Animate modal scale and opacity
		float target_scale = 1.0f;
		float scale = iam_tween_float(id, ImHashStr("scale"), target_scale, 0.3f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		float target_opacity = 1.0f;
		float opacity = iam_tween_float(id, ImHashStr("opacity"), target_opacity, 0.2f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Draw backdrop
		dl->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
			IM_COL32(0, 0, 0, (int)(backdrop * 150)), 4.0f);

		// Calculate modal size based on content
		const char* title = "Confirm Action";
		const char* line1 = "Are you sure you want to proceed?";
		const char* line2 = "This action cannot be undone.";

		ImVec2 title_size = ImGui::CalcTextSize(title);
		ImVec2 line1_size = ImGui::CalcTextSize(line1);
		ImVec2 line2_size = ImGui::CalcTextSize(line2);

		float content_width = ImMax(ImMax(title_size.x, line1_size.x), line2_size.x) + 48.0f;
		float modal_width = ImMax(content_width, 300.0f);
		float modal_height = 160.0f;

		// Draw modal
		ImVec2 modal_size(modal_width, modal_height);
		ImVec2 modal_center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);
		ImVec2 scaled_half(modal_size.x * 0.5f * scale, modal_size.y * 0.5f * scale);

		ImVec2 modal_min(modal_center.x - scaled_half.x, modal_center.y - scaled_half.y);
		ImVec2 modal_max(modal_center.x + scaled_half.x, modal_center.y + scaled_half.y);

		// Shadow
		for (int s = 3; s >= 0; s--)
		{
			float offset = 4.0f * (s + 1) * scale;
			dl->AddRectFilled(
				ImVec2(modal_min.x + offset * 0.3f, modal_min.y + offset),
				ImVec2(modal_max.x + offset * 0.3f, modal_max.y + offset),
				IM_COL32(0, 0, 0, (int)(15 * opacity)), 8.0f);
		}

		// Modal background
		dl->AddRectFilled(modal_min, modal_max, IM_COL32(50, 53, 65, (int)(255 * opacity)), 8.0f);

		// Modal content
		int text_alpha = (int)(255 * opacity);
		dl->AddText(ImVec2(modal_min.x + 24.0f * scale, modal_min.y + 20.0f * scale),
			IM_COL32(255, 255, 255, text_alpha), title);
		dl->AddText(ImVec2(modal_min.x + 24.0f * scale, modal_min.y + 52.0f * scale),
			IM_COL32(180, 180, 190, text_alpha), line1);
		dl->AddText(ImVec2(modal_min.x + 24.0f * scale, modal_min.y + 75.0f * scale),
			IM_COL32(140, 140, 150, text_alpha), line2);

		// Close button area
		ImVec2 close_btn_min(modal_min.x + 24.0f * scale, modal_max.y - 50.0f * scale);
		ImVec2 close_btn_max(modal_min.x + 110.0f * scale, modal_max.y - 18.0f * scale);
		dl->AddRectFilled(close_btn_min, close_btn_max, IM_COL32(91, 194, 231, (int)(200 * opacity)), 6.0f);
		ImVec2 btn_text_size = ImGui::CalcTextSize("Close");
		float btn_text_x = close_btn_min.x + ((close_btn_max.x - close_btn_min.x) - btn_text_size.x) * 0.5f;
		float btn_text_y = close_btn_min.y + ((close_btn_max.y - close_btn_min.y) - btn_text_size.y) * 0.5f;
		dl->AddText(ImVec2(btn_text_x, btn_text_y), IM_COL32(255, 255, 255, text_alpha), "Close");

		// Handle close
		ImGui::SetCursorScreenPos(close_btn_min);
		if (ImGui::InvisibleButton("##close_modal", ImVec2(close_btn_max.x - close_btn_min.x, close_btn_max.y - close_btn_min.y)))
			modal_open = false;
	}
	else
	{
		// Reset animations when closed
		ImGuiID id = ImGui::GetID("modal");
		iam_tween_float(id, ImHashStr("backdrop"), 0.0f, 0.2f, iam_ease_preset(iam_ease_in_cubic), iam_policy_crossfade, dt);
		iam_tween_float(id, ImHashStr("scale"), 0.9f, 0.2f, iam_ease_preset(iam_ease_in_cubic), iam_policy_crossfade, dt);
		iam_tween_float(id, ImHashStr("opacity"), 0.0f, 0.15f, iam_ease_preset(iam_ease_in_cubic), iam_policy_crossfade, dt);
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE 9: Accordion/Collapsible Sections
// ============================================================
static void ShowUsecase_Accordion()
{
	ImGui::TextWrapped(
		"Accordion sections that expand and collapse with smooth height animation. "
		"Arrow icons rotate to indicate state.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static bool sections_open[3] = { true, false, false };
	const char* section_titles[3] = { "General Settings", "Advanced Options", "About" };
	const char* section_contents[3] = {
		"Configure your basic preferences here.",
		"Expert settings for power users.",
		"Version 1.0.0 - Created with ImAnim"
	};
	float content_heights[3] = { 40.0f, 40.0f, 40.0f };

	ImVec2 start_pos = ImGui::GetCursorScreenPos();
	float section_width = ImGui::GetContentRegionAvail().x - 10.0f;
	float header_height = 40.0f;
	float current_y = start_pos.y;

	for (int i = 0; i < 3; i++)
	{
		ImGuiID id = ImGui::GetID(section_titles[i]);

		// Animate content height
		float target_height = sections_open[i] ? content_heights[i] : 0.0f;
		float animated_height = iam_tween_float(id, ImHashStr("height"), target_height, 0.3f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Animate arrow rotation
		float target_rotation = sections_open[i] ? 90.0f : 0.0f;
		float arrow_rotation = iam_tween_float(id, ImHashStr("arrow"), target_rotation, 0.25f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Header
		ImVec2 header_min(start_pos.x, current_y);
		ImVec2 header_max(start_pos.x + section_width, current_y + header_height);

		// Header background
		bool header_hovered = false;
		ImGui::SetCursorScreenPos(header_min);
		char btn_id[32];
		snprintf(btn_id, sizeof(btn_id), "##section%d", i);
		if (ImGui::InvisibleButton(btn_id, ImVec2(section_width, header_height)))
			sections_open[i] = !sections_open[i];
		header_hovered = ImGui::IsItemHovered();

		ImU32 header_color = header_hovered ? IM_COL32(55, 58, 70, 255) : IM_COL32(45, 48, 60, 255);
		dl->AddRectFilled(header_min, header_max, header_color, 4.0f,
			animated_height > 1.0f ? ImDrawFlags_RoundCornersTop : ImDrawFlags_RoundCornersAll);

		// Arrow icon
		ImVec2 arrow_center(header_min.x + 20.0f, header_min.y + header_height * 0.5f);
		float rad = arrow_rotation * 3.14159f / 180.0f;
		float arrow_size = 6.0f;
		ImVec2 arrow_points[3];
		for (int j = 0; j < 3; j++)
		{
			float angle = (j * 120.0f + 30.0f) * 3.14159f / 180.0f + rad;
			arrow_points[j] = ImVec2(
				arrow_center.x + ImCos(angle) * arrow_size,
				arrow_center.y + ImSin(angle) * arrow_size);
		}
		dl->AddTriangleFilled(arrow_points[0], arrow_points[1], arrow_points[2], IM_COL32(150, 150, 160, 255));

		// Title
		dl->AddText(ImVec2(header_min.x + 40.0f, header_min.y + (header_height - ImGui::GetTextLineHeight()) * 0.5f),
			IM_COL32(220, 220, 230, 255), section_titles[i]);

		current_y += header_height;

		// Content area
		if (animated_height > 1.0f)
		{
			ImVec2 content_min(start_pos.x, current_y);
			ImVec2 content_max(start_pos.x + section_width, current_y + animated_height);

			// Clip content
			dl->PushClipRect(content_min, content_max, true);

			dl->AddRectFilled(content_min, ImVec2(content_max.x, content_min.y + content_heights[i]),
				IM_COL32(35, 38, 48, 255), 4.0f, ImDrawFlags_RoundCornersBottom);

			// Content text
			float content_alpha = ImClamp(animated_height / content_heights[i], 0.0f, 1.0f);
			dl->AddText(ImVec2(content_min.x + 16.0f, content_min.y + 10.0f),
				IM_COL32(160, 160, 170, (int)(255 * content_alpha)), section_contents[i]);

			dl->PopClipRect();

			current_y += animated_height;
		}

		current_y += 4.0f; // spacing between sections
	}

	ImGui::SetCursorScreenPos(ImVec2(start_pos.x, current_y + 8.0f));
}

// ============================================================
// USECASE 10: Floating Action Button (FAB) Menu
// ============================================================
static void ShowUsecase_FABMenu()
{
	ImGui::TextWrapped(
		"Floating action button that expands into a radial arc menu. "
		"Uses staggered animations for child buttons.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static bool fab_open = false;

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 180);

	// Background
	dl->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	// FAB position (bottom-right corner)
	ImVec2 fab_center(canvas_pos.x + canvas_size.x - 50.0f, canvas_pos.y + canvas_size.y - 50.0f);
	float fab_radius = 28.0f;

	ImGuiID id = ImGui::GetID("fab");

	// Animate FAB rotation
	float target_rotation = fab_open ? 45.0f : 0.0f;
	float fab_rotation = iam_tween_float(id, ImHashStr("rotation"), target_rotation, 0.25f,
		iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

	// Child button positions (arc going up-left from bottom-right)
	const int child_count = 4;
	const char* child_icons[4] = { "A", "B", "C", "D" };
	ImVec4 child_colors[4] = {
		ImVec4(0.9f, 0.4f, 0.4f, 1.0f),
		ImVec4(0.4f, 0.8f, 0.4f, 1.0f),
		ImVec4(0.4f, 0.6f, 0.9f, 1.0f),
		ImVec4(0.9f, 0.7f, 0.3f, 1.0f)
	};
	float child_radius = 22.0f;
	float arc_radius = 80.0f;

	// Arc from 180 degrees (left) to 270 degrees (up) - upper-left quadrant
	float start_angle = 3.14159f;       // 180 degrees (pointing left)
	float end_angle = 3.14159f * 1.5f;  // 270 degrees (pointing up)

	// Track hover state for each child
	static bool child_hovered[4] = { false, false, false, false };

	// Draw child buttons (arc pattern)
	for (int i = 0; i < child_count; i++)
	{
		// Calculate angle for this button
		float angle_t = (float)i / (float)(child_count - 1);
		float angle = start_angle + (end_angle - start_angle) * angle_t;

		// Staggered animation
		float stagger_delay = (float)i * 0.05f;
		float target_dist = fab_open ? arc_radius : 0.0f;
		float target_alpha = fab_open ? 1.0f : 0.0f;
		float target_scale = fab_open ? 1.0f : 0.5f;

		ImGuiID child_id = id + i + 1;
		float dist = iam_tween_float(child_id, ImHashStr("dist"), target_dist, 0.3f - stagger_delay,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);
		float alpha = iam_tween_float(child_id, ImHashStr("alpha"), target_alpha, 0.2f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);
		float base_scale = iam_tween_float(child_id, ImHashStr("scale"), target_scale, 0.25f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		if (alpha > 0.01f)
		{
			// Child buttons positioned in arc
			ImVec2 child_center(
				fab_center.x + ImCos(angle) * dist,
				fab_center.y + ImSin(angle) * dist
			);

			// Check hover on child button
			ImGui::SetCursorScreenPos(ImVec2(child_center.x - child_radius, child_center.y - child_radius));
			char child_btn_id[32];
			snprintf(child_btn_id, sizeof(child_btn_id), "##fab_child_%d", i);
			ImGui::InvisibleButton(child_btn_id, ImVec2(child_radius * 2, child_radius * 2));
			child_hovered[i] = ImGui::IsItemHovered() && fab_open;

			// Animate hover scale
			float hover_target = child_hovered[i] ? 1.2f : 1.0f;
			float hover_scale = iam_tween_float(child_id, ImHashStr("hover"), hover_target, 0.15f,
				iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

			float scale = base_scale * hover_scale;

			// Shadow (larger when hovered)
			float shadow_alpha = child_hovered[i] ? 50.0f : 30.0f;
			dl->AddCircleFilled(ImVec2(child_center.x + 2, child_center.y + 3),
				child_radius * scale, IM_COL32(0, 0, 0, (int)(shadow_alpha * alpha)));

			// Glow effect on hover
			if (child_hovered[i])
			{
				float glow_anim = iam_tween_float(child_id, ImHashStr("glow"), 1.0f, 0.15f,
					iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);
				ImVec4 glow_col = child_colors[i];
				dl->AddCircleFilled(child_center, child_radius * scale + 4 * glow_anim,
					IM_COL32((int)(glow_col.x * 255), (int)(glow_col.y * 255), (int)(glow_col.z * 255), (int)(60 * alpha * glow_anim)));
			}
			else
			{
				iam_tween_float(child_id, ImHashStr("glow"), 0.0f, 0.15f,
					iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);
			}

			// Button
			ImVec4 col = child_colors[i];
			col.w = alpha;
			dl->AddCircleFilled(child_center, child_radius * scale,
				ImGui::ColorConvertFloat4ToU32(col));

			// Icon
			ImVec2 text_size = ImGui::CalcTextSize(child_icons[i]);
			dl->AddText(ImVec2(child_center.x - text_size.x * 0.5f, child_center.y - text_size.y * 0.5f),
				IM_COL32(255, 255, 255, (int)(255 * alpha)), child_icons[i]);
		}
	}

	// Main FAB button
	ImGui::SetCursorScreenPos(ImVec2(fab_center.x - fab_radius, fab_center.y - fab_radius));
	if (ImGui::InvisibleButton("##fab", ImVec2(fab_radius * 2, fab_radius * 2)))
		fab_open = !fab_open;

	bool fab_hovered = ImGui::IsItemHovered();

	// FAB hover animation
	float target_fab_scale = fab_hovered ? 1.1f : 1.0f;
	float fab_scale = iam_tween_float(id, ImHashStr("scale"), target_fab_scale, 0.15f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	// Shadow
	dl->AddCircleFilled(ImVec2(fab_center.x + 2, fab_center.y + 4),
		fab_radius * fab_scale, IM_COL32(0, 0, 0, 40));

	// FAB background
	dl->AddCircleFilled(fab_center, fab_radius * fab_scale, IM_COL32(91, 194, 231, 255));

	// Rotated plus icon
	float rad = fab_rotation * 3.14159f / 180.0f;
	float icon_size = 12.0f;
	float c = ImCos(rad), s = ImSin(rad);

	ImVec2 h1(fab_center.x - icon_size * c, fab_center.y - icon_size * s);
	ImVec2 h2(fab_center.x + icon_size * c, fab_center.y + icon_size * s);
	ImVec2 v1(fab_center.x + icon_size * s, fab_center.y - icon_size * c);
	ImVec2 v2(fab_center.x - icon_size * s, fab_center.y + icon_size * c);

	dl->AddLine(h1, h2, IM_COL32_WHITE, 3.0f);
	dl->AddLine(v1, v2, IM_COL32_WHITE, 3.0f);

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE 11: Animated Counter / Statistics
// ============================================================
static void ShowUsecase_AnimatedCounter()
{
	ImGui::TextWrapped(
		"Statistics that count up when they come into view. "
		"Common in landing pages and dashboards.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static bool triggered = false;
	static float trigger_time = 0.0f;

	if (ImGui::Button(triggered ? "Reset" : "Animate"))
	{
		triggered = !triggered;
		trigger_time = 0.0f;
	}

	if (triggered) trigger_time += dt;

	ImVec2 start_pos = ImGui::GetCursorScreenPos();
	float card_width = 140.0f;
	float card_height = 100.0f;
	float spacing = 16.0f;

	struct StatData { const char* label; int value; const char* suffix; float delay; };
	StatData stats[4] = {
		{ "Users", 12847, "", 0.0f },
		{ "Revenue", 94, "K", 0.1f },
		{ "Growth", 127, "%", 0.2f },
		{ "Rating", 49, "/5", 0.3f }
	};

	for (int i = 0; i < 4; i++)
	{
		ImVec2 card_pos(start_pos.x + i * (card_width + spacing), start_pos.y);
		ImGuiID id = ImGui::GetID(stats[i].label);

		// Calculate animated value
		float progress = 0.0f;
		if (triggered)
		{
			float t = trigger_time - stats[i].delay;
			if (t > 0) progress = ImClamp(t / 1.2f, 0.0f, 1.0f);
			progress = iam_eval_preset(iam_ease_out_expo, progress);
		}

		int display_value = (int)(stats[i].value * progress);

		// Animate card scale on trigger
		float target_scale = triggered ? 1.0f : 0.9f;
		float anim_t = ImClamp((trigger_time - stats[i].delay) / 0.4f, 0.0f, 1.0f);
		float scale = 0.9f + 0.1f * iam_eval_preset(iam_ease_out_back, anim_t);
		if (!triggered) scale = 0.9f;

		// Draw card
		ImVec2 center(card_pos.x + card_width * 0.5f, card_pos.y + card_height * 0.5f);
		ImVec2 half(card_width * 0.5f * scale, card_height * 0.5f * scale);

		dl->AddRectFilled(ImVec2(center.x - half.x, center.y - half.y),
			ImVec2(center.x + half.x, center.y + half.y),
			IM_COL32(40, 44, 55, 255), 8.0f);

		// Value text
		char value_text[32];
		if (strcmp(stats[i].suffix, "/5") == 0)
			snprintf(value_text, sizeof(value_text), "%.1f%s", display_value / 10.0f, stats[i].suffix);
		else
			snprintf(value_text, sizeof(value_text), "%d%s", display_value, stats[i].suffix);

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Use default font
		ImVec2 value_size = ImGui::CalcTextSize(value_text);
		dl->AddText(ImVec2(center.x - value_size.x * 0.5f, center.y - 20.0f),
			IM_COL32(91, 194, 231, 255), value_text);
		ImGui::PopFont();

		// Label
		ImVec2 label_size = ImGui::CalcTextSize(stats[i].label);
		dl->AddText(ImVec2(center.x - label_size.x * 0.5f, center.y + 10.0f),
			IM_COL32(140, 140, 150, 255), stats[i].label);
	}

	ImGui::SetCursorScreenPos(ImVec2(start_pos.x, start_pos.y + card_height + 16));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE 12: Typewriter Text Effect
// ============================================================
static void ShowUsecase_TypewriterText()
{
	ImGui::TextWrapped(
		"Text that types out character by character with a blinking cursor. "
		"Classic effect for hero sections and intros.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static float type_time = 0.0f;
	static int current_phrase = 0;
	static bool deleting = false;

	const char* phrases[] = {
		"Welcome to ImAnim",
		"Beautiful Animations",
		"Made Simple",
		"For Dear ImGui"
	};
	const int phrase_count = 4;

	type_time += dt;

	// Typing speed
	float chars_per_second = deleting ? 30.0f : 12.0f;
	float pause_time = 2.0f;

	const char* current = phrases[current_phrase];
	int total_chars = (int)strlen(current);

	// Calculate visible characters
	int visible_chars;
	if (deleting)
	{
		visible_chars = total_chars - (int)(type_time * chars_per_second);
		if (visible_chars <= 0)
		{
			visible_chars = 0;
			deleting = false;
			current_phrase = (current_phrase + 1) % phrase_count;
			type_time = 0.0f;
		}
	}
	else
	{
		visible_chars = (int)(type_time * chars_per_second);
		if (visible_chars >= total_chars)
		{
			visible_chars = total_chars;
			if (type_time > total_chars / chars_per_second + pause_time)
			{
				deleting = true;
				type_time = 0.0f;
			}
		}
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 60);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(20, 22, 30, 255), 4.0f);

	// Draw text
	char display_text[256];
	strncpy(display_text, current, visible_chars);
	display_text[visible_chars] = '\0';

	ImVec2 text_pos(pos.x + 20.0f, pos.y + (canvas_size.y - ImGui::GetTextLineHeight()) * 0.5f);
	dl->AddText(text_pos, IM_COL32(220, 220, 230, 255), display_text);

	// Blinking cursor
	float cursor_blink = ImSin(type_time * 6.0f) > 0 ? 1.0f : 0.0f;
	if (!deleting && visible_chars < total_chars) cursor_blink = 1.0f; // Solid while typing

	ImVec2 text_size = ImGui::CalcTextSize(display_text);
	float cursor_x = text_pos.x + text_size.x + 2.0f;
	float cursor_height = ImGui::GetTextLineHeight();
	dl->AddRectFilled(ImVec2(cursor_x, text_pos.y),
		ImVec2(cursor_x + 2.0f, text_pos.y + cursor_height),
		IM_COL32(91, 194, 231, (int)(255 * cursor_blink)));

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE 13: Skeleton Loading Placeholder
// ============================================================
static void ShowUsecase_SkeletonLoading()
{
	ImGui::TextWrapped(
		"Skeleton loading placeholders with shimmer effect. "
		"Shows perceived performance while content loads.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static float shimmer_time = 0.0f;
	static bool is_loading = true;
	static float load_progress = 0.0f;
	shimmer_time += dt;

	// Auto-load after 3 seconds
	if (is_loading)
	{
		load_progress += dt;
		if (load_progress > 3.0f)
			is_loading = false;
	}

	if (ImGui::Button("Reset to Loading"))
	{
		shimmer_time = 0.0f;
		is_loading = true;
		load_progress = 0.0f;
	}
	ImGui::SameLine();
	if (ImGui::Button(is_loading ? "Skip Loading" : "Show Loaded"))
	{
		is_loading = false;
		load_progress = 3.0f;
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 160);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	float padding = 16.0f;
	ImVec2 avatar_pos(pos.x + padding, pos.y + padding);

	if (is_loading)
	{
		// Helper lambda to draw skeleton element with shimmer
		auto DrawSkeleton = [&](ImVec2 elem_pos, ImVec2 elem_size, float rounding) {
			// Base color
			dl->AddRectFilled(elem_pos, ImVec2(elem_pos.x + elem_size.x, elem_pos.y + elem_size.y),
				IM_COL32(45, 48, 58, 255), rounding);

			// Shimmer effect
			float shimmer_width = 80.0f;
			float shimmer_speed = 1.5f;
			float cycle = ImFmod(shimmer_time * shimmer_speed, 2.0f);
			float shimmer_x = elem_pos.x - shimmer_width + cycle * (elem_size.x + shimmer_width * 2);

			// Clip shimmer to element bounds
			dl->PushClipRect(elem_pos, ImVec2(elem_pos.x + elem_size.x, elem_pos.y + elem_size.y), true);

			// Draw gradient shimmer
			for (int j = 0; j < 3; j++)
			{
				float offset = j * shimmer_width * 0.3f;
				float alpha = 0.15f * (1.0f - fabsf(j - 1) * 0.5f);
				dl->AddRectFilledMultiColor(
					ImVec2(shimmer_x + offset, elem_pos.y),
					ImVec2(shimmer_x + offset + shimmer_width * 0.3f, elem_pos.y + elem_size.y),
					IM_COL32(255, 255, 255, 0),
					IM_COL32(255, 255, 255, (int)(alpha * 255)),
					IM_COL32(255, 255, 255, (int)(alpha * 255)),
					IM_COL32(255, 255, 255, 0));
			}

			dl->PopClipRect();
		};

		// Draw skeleton card layout

		// Avatar circle (skeleton)
		dl->AddCircleFilled(ImVec2(avatar_pos.x + 24, avatar_pos.y + 24), 24,
			IM_COL32(45, 48, 58, 255));
		// Shimmer on avatar
		float shimmer_width = 80.0f;
		float shimmer_speed = 1.5f;
		float cycle = ImFmod(shimmer_time * shimmer_speed, 2.0f);
		dl->PushClipRect(avatar_pos, ImVec2(avatar_pos.x + 48, avatar_pos.y + 48), true);
		float shimmer_x = avatar_pos.x - shimmer_width + cycle * (48 + shimmer_width * 2);
		dl->AddRectFilledMultiColor(
			ImVec2(shimmer_x, avatar_pos.y),
			ImVec2(shimmer_x + shimmer_width, avatar_pos.y + 48),
			IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 30),
			IM_COL32(255, 255, 255, 30), IM_COL32(255, 255, 255, 0));
		dl->PopClipRect();

		// Name placeholder
		DrawSkeleton(ImVec2(pos.x + padding + 60, pos.y + padding + 4), ImVec2(120, 16), 4.0f);

		// Subtitle placeholder
		DrawSkeleton(ImVec2(pos.x + padding + 60, pos.y + padding + 28), ImVec2(80, 12), 4.0f);

		// Content lines
		float line_y = pos.y + padding + 64;
		DrawSkeleton(ImVec2(pos.x + padding, line_y), ImVec2(canvas_size.x - padding * 2, 14), 4.0f);
		DrawSkeleton(ImVec2(pos.x + padding, line_y + 22), ImVec2(canvas_size.x - padding * 2 - 40, 14), 4.0f);
		DrawSkeleton(ImVec2(pos.x + padding, line_y + 44), ImVec2(canvas_size.x - padding * 2 - 80, 14), 4.0f);
	}
	else
	{
		// Draw loaded content

		// Avatar (colored circle with initials)
		dl->AddCircleFilled(ImVec2(avatar_pos.x + 24, avatar_pos.y + 24), 24,
			IM_COL32(91, 194, 231, 255));
		dl->AddText(ImVec2(avatar_pos.x + 14, avatar_pos.y + 16), IM_COL32_WHITE, "JD");

		// Name
		dl->AddText(ImVec2(pos.x + padding + 60, pos.y + padding + 4),
			IM_COL32(220, 220, 230, 255), "John Doe");

		// Subtitle
		dl->AddText(ImVec2(pos.x + padding + 60, pos.y + padding + 28),
			IM_COL32(140, 140, 150, 255), "Software Engineer");

		// Content lines
		float line_y = pos.y + padding + 64;
		dl->AddText(ImVec2(pos.x + padding, line_y),
			IM_COL32(180, 180, 190, 255), "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
		dl->AddText(ImVec2(pos.x + padding, line_y + 22),
			IM_COL32(180, 180, 190, 255), "Sed do eiusmod tempor incididunt ut labore.");
		dl->AddText(ImVec2(pos.x + padding, line_y + 44),
			IM_COL32(180, 180, 190, 255), "Ut enim ad minim veniam.");
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE 14: Avatar Stack with Hover Expansion
// ============================================================
static void ShowUsecase_AvatarStack()
{
	ImGui::TextWrapped(
		"Stacked avatars that expand on hover to reveal all members. "
		"Common pattern for showing team members or participants.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static bool stack_hovered = false;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 80);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	// Avatar data
	const int avatar_count = 5;
	ImU32 avatar_colors[5] = {
		IM_COL32(91, 194, 231, 255),
		IM_COL32(204, 120, 88, 255),
		IM_COL32(130, 200, 130, 255),
		IM_COL32(200, 180, 100, 255),
		IM_COL32(180, 130, 200, 255)
	};
	const char* initials[5] = { "JD", "AS", "MK", "RB", "TW" };

	float avatar_radius = 20.0f;
	float collapsed_overlap = 28.0f;  // More overlap when collapsed
	float expanded_spacing = avatar_radius * 2.2f;

	// Check hover on stack area
	float stack_width_collapsed = avatar_radius * 2 + (avatar_count - 1) * (avatar_radius * 2 - collapsed_overlap);
	float stack_width_expanded = avatar_count * expanded_spacing;

	ImVec2 stack_pos(pos.x + 20.0f, pos.y + canvas_size.y * 0.5f);

	ImGui::SetCursorScreenPos(ImVec2(stack_pos.x - avatar_radius, stack_pos.y - avatar_radius));
	ImGui::InvisibleButton("##avatar_stack", ImVec2(stack_width_expanded, avatar_radius * 2));
	stack_hovered = ImGui::IsItemHovered();

	ImGuiID id = ImGui::GetID("avatar_stack");

	// Animate expansion
	float target_expand = stack_hovered ? 1.0f : 0.0f;
	float expand = iam_tween_float(id, ImHashStr("expand"), target_expand, 0.3f,
		iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

	// Draw avatars (reverse order so first is on top when collapsed)
	for (int i = avatar_count - 1; i >= 0; i--)
	{
		float collapsed_x = stack_pos.x + i * (avatar_radius * 2 - collapsed_overlap);
		float expanded_x = stack_pos.x + i * expanded_spacing;
		float x = collapsed_x + (expanded_x - collapsed_x) * expand;

		// Individual hover animation
		ImGuiID av_id = id + i + 1;
		float target_lift = 0.0f;

		// Check individual hover when expanded
		if (expand > 0.5f)
		{
			float dx = ImGui::GetMousePos().x - x;
			float dy = ImGui::GetMousePos().y - stack_pos.y;
			if (dx * dx + dy * dy < avatar_radius * avatar_radius * 1.5f)
				target_lift = -8.0f;
		}

		float lift = iam_tween_float(av_id, ImHashStr("lift"), target_lift, 0.15f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Scale animation
		float target_scale = (target_lift < 0) ? 1.15f : 1.0f;
		float scale = iam_tween_float(av_id, ImHashStr("scale"), target_scale, 0.15f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		ImVec2 av_center(x, stack_pos.y + lift);

		// Shadow
		dl->AddCircleFilled(ImVec2(av_center.x + 2, av_center.y + 3),
			avatar_radius * scale, IM_COL32(0, 0, 0, 30));

		// Avatar circle
		dl->AddCircleFilled(av_center, avatar_radius * scale, avatar_colors[i]);
		dl->AddCircle(av_center, avatar_radius * scale, IM_COL32(255, 255, 255, 50), 0, 2.0f);

		// Initials
		ImVec2 text_size = ImGui::CalcTextSize(initials[i]);
		dl->AddText(ImVec2(av_center.x - text_size.x * 0.5f, av_center.y - text_size.y * 0.5f),
			IM_COL32_WHITE, initials[i]);
	}

	// "+N more" badge when collapsed
	if (expand < 0.5f)
	{
		float badge_alpha = 1.0f - expand * 2.0f;
		float badge_x = stack_pos.x + stack_width_collapsed + 10.0f;
		char more_text[16];
		snprintf(more_text, sizeof(more_text), "+%d", avatar_count);
		dl->AddText(ImVec2(badge_x, stack_pos.y - ImGui::GetTextLineHeight() * 0.5f),
			IM_COL32(140, 140, 150, (int)(255 * badge_alpha)), more_text);
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE 15: Ripple Effect (Material Design)
// ============================================================
static void ShowUsecase_RippleEffect()
{
	ImGui::TextWrapped(
		"Material Design-style ripple effect on click. "
		"Provides tactile feedback for interactive elements.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Ripple state
	static ImVec2 ripple_origin;
	static float ripple_time = -1.0f;
	static bool ripple_active = false;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 btn_size(200, 50);

	// Button background
	dl->AddRectFilled(pos, ImVec2(pos.x + btn_size.x, pos.y + btn_size.y),
		IM_COL32(91, 194, 231, 255), 8.0f);

	// Invisible button for interaction
	ImGui::SetCursorScreenPos(pos);
	if (ImGui::InvisibleButton("##ripple_btn", btn_size))
	{
		ripple_origin = ImGui::GetMousePos();
		ripple_time = 0.0f;
		ripple_active = true;
	}
	bool hovered = ImGui::IsItemHovered();

	// Update ripple
	if (ripple_active)
	{
		ripple_time += dt;

		// Calculate max radius (corner to corner)
		float max_dist = 0.0f;
		ImVec2 corners[4] = {
			pos,
			ImVec2(pos.x + btn_size.x, pos.y),
			ImVec2(pos.x + btn_size.x, pos.y + btn_size.y),
			ImVec2(pos.x, pos.y + btn_size.y)
		};
		for (int i = 0; i < 4; i++)
		{
			float dx = corners[i].x - ripple_origin.x;
			float dy = corners[i].y - ripple_origin.y;
			float d = sqrtf(dx * dx + dy * dy);
			if (d > max_dist) max_dist = d;
		}

		float duration = 0.6f;
		float progress = ripple_time / duration;

		if (progress < 1.0f)
		{
			float ease_progress = iam_eval_preset(iam_ease_out_cubic, progress);
			float radius = max_dist * ease_progress;
			float alpha = 0.3f * (1.0f - progress);

			// Clip to button bounds
			dl->PushClipRect(pos, ImVec2(pos.x + btn_size.x, pos.y + btn_size.y), true);
			dl->AddCircleFilled(ripple_origin, radius, IM_COL32(255, 255, 255, (int)(alpha * 255)));
			dl->PopClipRect();
		}
		else
		{
			ripple_active = false;
		}
	}

	// Hover highlight
	if (hovered && !ripple_active)
	{
		dl->AddRectFilled(pos, ImVec2(pos.x + btn_size.x, pos.y + btn_size.y),
			IM_COL32(255, 255, 255, 20), 8.0f);
	}

	// Text
	const char* label = "Ripple";
	ImVec2 text_size = ImGui::CalcTextSize(label);
	dl->AddText(ImVec2(pos.x + (btn_size.x - text_size.x) * 0.5f, pos.y + (btn_size.y - text_size.y) * 0.5f),
		IM_COL32_WHITE, label);

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + btn_size.y + 16));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE 16: Hamburger Menu Morph
// ============================================================
static void ShowUsecase_HamburgerMorph()
{
	ImGui::TextWrapped(
		"Hamburger icon that morphs into an X when clicked. "
		"Classic navigation pattern with smooth line animations.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static bool menu_open = false;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 btn_size(50, 50);

	// Button background
	ImGui::SetCursorScreenPos(pos);
	if (ImGui::InvisibleButton("##hamburger", btn_size))
		menu_open = !menu_open;

	bool hovered = ImGui::IsItemHovered();

	ImGuiID id = ImGui::GetID("hamburger");

	// Animate morph progress (slower for visibility)
	float target_morph = menu_open ? 1.0f : 0.0f;
	float morph = iam_tween_float(id, ImHashStr("morph"), target_morph, 0.6f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	// Animate hover scale
	float target_scale = hovered ? 1.1f : 1.0f;
	float scale = iam_tween_float(id, ImHashStr("scale"), target_scale, 0.25f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	// Background circle
	ImVec2 center(pos.x + btn_size.x * 0.5f, pos.y + btn_size.y * 0.5f);
	float bg_alpha = hovered ? 0.15f : 0.0f;
	dl->AddCircleFilled(center, btn_size.x * 0.5f * scale, IM_COL32(255, 255, 255, (int)(bg_alpha * 255)));

	// Line properties
	float line_width = 20.0f * scale;
	float line_height = 2.5f;
	float line_gap = 6.0f;

	// Calculate line positions and rotations
	// Top line: moves down and rotates to form top of X
	// Middle line: fades out
	// Bottom line: moves up and rotates to form bottom of X

	float top_y = center.y - line_gap;
	float mid_y = center.y;
	float bot_y = center.y + line_gap;

	// Morphed positions
	float morph_top_y = center.y;
	float morph_bot_y = center.y;

	float current_top_y = top_y + (morph_top_y - top_y) * morph;
	float current_bot_y = bot_y + (morph_bot_y - bot_y) * morph;

	// Rotations (in radians)
	float top_rotation = morph * 0.785f;  // 45 degrees
	float bot_rotation = morph * -0.785f; // -45 degrees

	// Middle line alpha
	float mid_alpha = 1.0f - morph;

	ImU32 line_color = IM_COL32(220, 220, 230, 255);

	// Draw lines
	auto DrawRotatedLine = [&](float cy, float rotation, float alpha) {
		float half_w = line_width * 0.5f;
		float c = ImCos(rotation), s = ImSin(rotation);

		ImVec2 p1(center.x - half_w * c, cy - half_w * s);
		ImVec2 p2(center.x + half_w * c, cy + half_w * s);

		dl->AddLine(p1, p2, IM_COL32(220, 220, 230, (int)(255 * alpha)), line_height);
	};

	DrawRotatedLine(current_top_y, top_rotation, 1.0f);
	DrawRotatedLine(mid_y, 0.0f, mid_alpha);
	DrawRotatedLine(current_bot_y, bot_rotation, 1.0f);

	// Label
	ImGui::SetCursorScreenPos(ImVec2(pos.x + btn_size.x + 16, pos.y + (btn_size.y - ImGui::GetTextLineHeight()) * 0.5f));
	ImGui::Text(menu_open ? "Close Menu" : "Open Menu");

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + btn_size.y + 16));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE 17: Swipe Cards (Tinder-style)
// ============================================================
static void ShowUsecase_SwipeCards()
{
	ImGui::TextWrapped(
		"Stacked cards that can be swiped left or right. "
		"Popular in dating apps and decision-making interfaces.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static int current_card = 0;
	static float swipe_x = 0.0f;
	static float swipe_rotation = 0.0f;
	static bool swiping = false;

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 230);

	// Background
	dl->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(20, 22, 30, 255), 4.0f);

	// Card data
	const char* card_titles[] = { "Project Alpha", "Design Beta", "Code Gamma", "Launch Delta" };
	const char* card_descs[] = { "Innovation awaits", "Creative vision", "Build the future", "Ship it!" };
	ImU32 card_colors[] = {
		IM_COL32(91, 194, 231, 255),
		IM_COL32(204, 120, 88, 255),
		IM_COL32(130, 200, 130, 255),
		IM_COL32(200, 180, 100, 255)
	};
	const int total_cards = 4;

	ImVec2 card_size(280, 180);
	ImVec2 stack_center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

	// Swipe buttons - positioned within canvas bounds
	float btn_y = canvas_pos.y + canvas_size.y - 45;
	ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x + 30, btn_y));
	if (ImGui::Button("< Reject") && !swiping && current_card < total_cards)
	{
		swiping = true;
		swipe_x = 0.0f;
	}
	ImGui::SameLine();
	ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x + canvas_size.x - 110, btn_y));
	if (ImGui::Button("Accept >") && !swiping && current_card < total_cards)
	{
		swiping = true;
		swipe_x = 0.01f; // Small positive to indicate right swipe
	}

	// Update swipe animation
	ImGuiID id = ImGui::GetID("swipe_cards");
	if (swiping)
	{
		float target_x = (swipe_x >= 0) ? 400.0f : -400.0f;
		swipe_x = iam_tween_float(id, ImHashStr("swipe_x"), target_x, 0.8f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);
		swipe_rotation = swipe_x * 0.04f;

		if (fabsf(swipe_x) > 350.0f)
		{
			current_card++;
			swiping = false;
			swipe_x = 0.0f;
			swipe_rotation = 0.0f;
			// Reset tween
			iam_tween_float(id, ImHashStr("swipe_x"), 0.0f, 0.01f,
				iam_ease_preset(iam_ease_linear), iam_policy_cut, dt);
		}
	}

	// Draw cards (back to front)
	for (int i = ImMin(current_card + 2, total_cards - 1); i >= current_card; i--)
	{
		if (i >= total_cards) continue;

		int stack_index = i - current_card;
		float scale = 1.0f - stack_index * 0.05f;
		float y_offset = stack_index * 8.0f;

		ImVec2 card_center = stack_center;
		card_center.y += y_offset;

		float rotation = 0.0f;
		float x_offset = 0.0f;

		// Apply swipe to top card only
		if (i == current_card && swiping)
		{
			x_offset = swipe_x;
			rotation = swipe_rotation;
		}

		card_center.x += x_offset;

		// Calculate rotated corners
		ImVec2 half(card_size.x * 0.5f * scale, card_size.y * 0.5f * scale);
		float c = ImCos(rotation * 3.14159f / 180.0f);
		float s = ImSin(rotation * 3.14159f / 180.0f);

		ImVec2 corners[4];
		ImVec2 offsets[4] = {
			ImVec2(-half.x, -half.y), ImVec2(half.x, -half.y),
			ImVec2(half.x, half.y), ImVec2(-half.x, half.y)
		};
		for (int j = 0; j < 4; j++)
		{
			corners[j].x = card_center.x + offsets[j].x * c - offsets[j].y * s;
			corners[j].y = card_center.y + offsets[j].x * s + offsets[j].y * c;
		}

		// Shadow
		ImVec2 shadow_offset(4 + fabsf(x_offset) * 0.02f, 6 + fabsf(x_offset) * 0.02f);
		ImVec2 shadow_corners[4];
		for (int j = 0; j < 4; j++)
		{
			shadow_corners[j].x = corners[j].x + shadow_offset.x;
			shadow_corners[j].y = corners[j].y + shadow_offset.y;
		}
		dl->AddConvexPolyFilled(shadow_corners, 4, IM_COL32(0, 0, 0, 40));

		// Card
		dl->AddConvexPolyFilled(corners, 4, IM_COL32(50, 54, 65, 255));

		// Color accent bar
		ImVec2 accent_corners[4] = { corners[0], corners[1],
			ImVec2(corners[1].x + (corners[2].x - corners[1].x) * 0.15f, corners[1].y + (corners[2].y - corners[1].y) * 0.15f),
			ImVec2(corners[0].x + (corners[3].x - corners[0].x) * 0.15f, corners[0].y + (corners[3].y - corners[0].y) * 0.15f) };
		dl->AddConvexPolyFilled(accent_corners, 4, card_colors[i % total_cards]);

		// Title and description
		if (i < total_cards)
		{
			ImVec2 title_size = ImGui::CalcTextSize(card_titles[i]);
			ImVec2 desc_size = ImGui::CalcTextSize(card_descs[i]);
			dl->AddText(ImVec2(card_center.x - title_size.x * 0.5f, card_center.y - 10), IM_COL32(220, 220, 230, 255), card_titles[i]);
			dl->AddText(ImVec2(card_center.x - desc_size.x * 0.5f, card_center.y + 10), IM_COL32(150, 150, 160, 255), card_descs[i]);
		}

		// Swipe indicators on top card
		if (i == current_card && swiping)
		{
			if (swipe_x > 50)
			{
				dl->AddText(ImVec2(card_center.x - 30, card_center.y - 40),
					IM_COL32(100, 200, 100, (int)(ImMin(swipe_x / 100.0f, 1.0f) * 255)), "ACCEPT");
			}
			else if (swipe_x < -50)
			{
				dl->AddText(ImVec2(card_center.x - 30, card_center.y - 40),
					IM_COL32(200, 100, 100, (int)(ImMin(-swipe_x / 100.0f, 1.0f) * 255)), "REJECT");
			}
		}
	}

	// Reset button
	if (current_card >= total_cards)
	{
		ImGui::SetCursorScreenPos(ImVec2(stack_center.x - 40, stack_center.y - 10));
		if (ImGui::Button("Reset Cards"))
			current_card = 0;
	}

	ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x, canvas_pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE 18: Gradient Background Animation
// ============================================================
static void ShowUsecase_GradientAnimation()
{
	ImGui::TextWrapped(
		"Animated gradient backgrounds that shift colors smoothly. "
		"Creates atmosphere and visual interest in hero sections.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static float gradient_time = 0.0f;
	gradient_time += dt * 0.5f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 size(ImGui::GetContentRegionAvail().x, 120);

	// Animate colors through hue cycle using OKLAB
	float hue1 = ImFmod(gradient_time * 0.1f, 1.0f);
	float hue2 = ImFmod(gradient_time * 0.1f + 0.3f, 1.0f);
	float hue3 = ImFmod(gradient_time * 0.1f + 0.6f, 1.0f);

	// Convert hue to sRGB via OKLCH
	auto HueToColor = [](float h, float c, float l) -> ImU32 {
		// Simple HSL-ish approximation for demo
		float r = 0.5f + 0.5f * ImCos((h + 0.0f / 3.0f) * 6.28318f);
		float g = 0.5f + 0.5f * ImCos((h + 1.0f / 3.0f) * 6.28318f);
		float b = 0.5f + 0.5f * ImCos((h + 2.0f / 3.0f) * 6.28318f);
		r = l + (r - 0.5f) * c;
		g = l + (g - 0.5f) * c;
		b = l + (b - 0.5f) * c;
		return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
	};

	ImU32 col1 = HueToColor(hue1, 0.6f, 0.3f);
	ImU32 col2 = HueToColor(hue2, 0.6f, 0.25f);
	ImU32 col3 = HueToColor(hue3, 0.6f, 0.35f);

	// Draw gradient with multiple color stops (approximated with rects)
	int segments = 20;
	float segment_width = size.x / segments;

	for (int i = 0; i < segments; i++)
	{
		float t1 = (float)i / segments;
		float t2 = (float)(i + 1) / segments;

		// Blend between colors based on position and time
		auto BlendColors = [](ImU32 a, ImU32 b, float t) -> ImU32 {
			int ra = (a >> 0) & 0xFF, ga = (a >> 8) & 0xFF, ba = (a >> 16) & 0xFF;
			int rb = (b >> 0) & 0xFF, gb = (b >> 8) & 0xFF, bb = (b >> 16) & 0xFF;
			int r = (int)(ra + (rb - ra) * t);
			int g = (int)(ga + (gb - ga) * t);
			int b_ch = (int)(ba + (bb - ba) * t);
			return IM_COL32(r, g, b_ch, 255);
		};

		// Create wave effect
		float wave = ImSin(t1 * 3.14159f * 2.0f + gradient_time * 2.0f) * 0.5f + 0.5f;

		ImU32 left_col = BlendColors(BlendColors(col1, col2, t1), col3, wave);
		ImU32 right_col = BlendColors(BlendColors(col1, col2, t2), col3, wave);

		dl->AddRectFilledMultiColor(
			ImVec2(pos.x + i * segment_width, pos.y),
			ImVec2(pos.x + (i + 1) * segment_width, pos.y + size.y),
			left_col, right_col, right_col, left_col);
	}

	// Overlay text
	const char* text = "Animated Gradient";
	ImVec2 text_size = ImGui::CalcTextSize(text);
	dl->AddText(ImVec2(pos.x + (size.x - text_size.x) * 0.5f, pos.y + (size.y - text_size.y) * 0.5f),
		IM_COL32(255, 255, 255, 200), text);

	ImGui::Dummy(size);
}

// ============================================================
// USECASE 19: Tooltip Animation
// ============================================================
static void ShowUsecase_TooltipAnimation()
{
	ImGui::TextWrapped(
		"Smooth tooltip animations that slide in with a slight bounce. "
		"Enhances UX by providing context without jarring popups.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 100);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	// Interactive elements
	struct TooltipItem {
		const char* label;
		const char* tooltip;
		float x_offset;
	};
	TooltipItem items[] = {
		{ "[?]", "This is a help icon with more information", 0.2f },
		{ "[i]", "Information tooltip with details", 0.5f },
		{ "[!]", "Warning: Important notice here", 0.8f }
	};

	static int hovered_item = -1;
	static float tooltip_time = 0.0f;

	int new_hovered = -1;
	float hover_radius = 18.0f;

	for (int i = 0; i < 3; i++)
	{
		// Calculate text size and center position
		ImVec2 text_size = ImGui::CalcTextSize(items[i].label);
		ImVec2 item_center(pos.x + canvas_size.x * items[i].x_offset, pos.y + canvas_size.y * 0.5f);
		ImVec2 item_pos(item_center.x - text_size.x * 0.5f, item_center.y - text_size.y * 0.5f);

		// Check hover using circular hit region
		ImVec2 mouse = ImGui::GetMousePos();
		float dx = mouse.x - item_center.x;
		float dy = mouse.y - item_center.y;
		float dist = sqrtf(dx * dx + dy * dy);
		bool item_hovered = (dist < hover_radius);
		if (item_hovered) new_hovered = i;

		// Draw hover disc background
		if (item_hovered)
		{
			dl->AddCircleFilled(item_center, hover_radius, IM_COL32(91, 194, 231, 40));
			dl->AddCircle(item_center, hover_radius, IM_COL32(91, 194, 231, 100), 0, 1.5f);
		}

		// Draw item text
		dl->AddText(item_pos, item_hovered ? IM_COL32(91, 194, 231, 255) : IM_COL32(150, 150, 160, 255),
			items[i].label);
	}

	// Update tooltip state
	if (new_hovered != hovered_item)
	{
		hovered_item = new_hovered;
		tooltip_time = 0.0f;
	}
	else if (hovered_item >= 0)
	{
		tooltip_time += dt;
	}

	// Draw tooltip
	if (hovered_item >= 0)
	{
		ImGuiID id = ImGui::GetID("tooltip");

		// Animate with delay - smooth fade without bouncing/flickering (accessibility)
		float delay = 0.15f;
		float anim_t = ImClamp((tooltip_time - delay) / 0.2f, 0.0f, 1.0f);
		float ease_t = iam_eval_preset(iam_ease_out_cubic, anim_t);  // Smooth ease without overshoot

		if (tooltip_time > delay)
		{
			ImVec2 anchor(pos.x + canvas_size.x * items[hovered_item].x_offset, pos.y + canvas_size.y * 0.5f - hover_radius);

			const char* tip_text = items[hovered_item].tooltip;
			ImVec2 text_size = ImGui::CalcTextSize(tip_text);
			ImVec2 padding(12, 8);
			ImVec2 tip_size(text_size.x + padding.x * 2, text_size.y + padding.y * 2);

			// Position above with animation
			float y_offset = -tip_size.y - 10 + (1.0f - ease_t) * 10.0f;
			ImVec2 tip_pos(anchor.x - tip_size.x * 0.5f, anchor.y + y_offset);

			// Clamp to canvas
			if (tip_pos.x < pos.x) tip_pos.x = pos.x;
			if (tip_pos.x + tip_size.x > pos.x + canvas_size.x)
				tip_pos.x = pos.x + canvas_size.x - tip_size.x;

			int alpha = (int)(255 * ease_t);

			// Shadow
			dl->AddRectFilled(ImVec2(tip_pos.x + 2, tip_pos.y + 3),
				ImVec2(tip_pos.x + tip_size.x + 2, tip_pos.y + tip_size.y + 3),
				IM_COL32(0, 0, 0, alpha / 4), 6.0f);

			// Background
			dl->AddRectFilled(tip_pos, ImVec2(tip_pos.x + tip_size.x, tip_pos.y + tip_size.y),
				IM_COL32(50, 54, 65, alpha), 6.0f);

			// Arrow
			ImVec2 arrow_tip(anchor.x, tip_pos.y + tip_size.y + 6);
			ImVec2 arrow_left(anchor.x - 6, tip_pos.y + tip_size.y);
			ImVec2 arrow_right(anchor.x + 6, tip_pos.y + tip_size.y);
			dl->AddTriangleFilled(arrow_left, arrow_right, arrow_tip, IM_COL32(50, 54, 65, alpha));

			// Text
			dl->AddText(ImVec2(tip_pos.x + padding.x, tip_pos.y + padding.y),
				IM_COL32(220, 220, 230, alpha), tip_text);
		}
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE 20: Pulse Notification Badge
// ============================================================
static void ShowUsecase_PulseBadge()
{
	ImGui::TextWrapped(
		"Notification badges that pulse to draw attention. "
		"Common for unread messages, alerts, and status indicators.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static float pulse_time = 0.0f;
	static int badge_counts[3] = { 3, 99, 0 };
	pulse_time += dt;

	if (ImGui::Button("Reset Badges"))
	{
		pulse_time = 0.0f;
		badge_counts[0] = 3;
		badge_counts[1] = 99;
		badge_counts[2] = 0;
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Notification"))
	{
		badge_counts[0]++;
		badge_counts[1]++;
		pulse_time = 0.0f; // Restart pulse animation
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 80);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	// Draw example icons with badges
	struct BadgeItem { ImVec2 offset; int* count; bool pulse; };
	BadgeItem items[] = {
		{ ImVec2(0.2f, 0.5f), &badge_counts[0], true },
		{ ImVec2(0.5f, 0.5f), &badge_counts[1], true },
		{ ImVec2(0.8f, 0.5f), &badge_counts[2], false }  // Just a dot
	};

	for (int i = 0; i < 3; i++)
	{
		ImVec2 icon_center(pos.x + canvas_size.x * items[i].offset.x, pos.y + canvas_size.y * items[i].offset.y);
		float icon_size = 24.0f;

		// Draw icon placeholder (bell shape approximation)
		dl->AddCircleFilled(icon_center, icon_size, IM_COL32(60, 65, 80, 255));
		dl->AddCircleFilled(ImVec2(icon_center.x, icon_center.y - 4), icon_size * 0.6f, IM_COL32(80, 85, 100, 255));

		// Badge position
		ImVec2 badge_center(icon_center.x + icon_size * 0.7f, icon_center.y - icon_size * 0.5f);

		// Pulse animation - much more visible with scale and multiple rings
		float pulse = 0.0f;
		float pulse2 = 0.0f;
		float badge_scale = 1.0f;
		if (items[i].pulse && *items[i].count > 0)
		{
			float phase = pulse_time * 3.0f + i * 0.5f;
			pulse = ImFmod(phase, 1.0f);  // Repeating 0-1 cycle
			pulse2 = ImFmod(phase + 0.5f, 1.0f);  // Second ring offset
			// Add subtle scale bounce
			float scale_phase = pulse_time * 4.0f + i * 0.3f;
			badge_scale = 1.0f + ImSin(scale_phase) * 0.15f;
		}

		// Draw expanding pulse rings (multiple for visibility)
		if (pulse > 0.01f && *items[i].count > 0)
		{
			// First ring
			float ring_radius = 10.0f + pulse * 20.0f;
			int ring_alpha = (int)((1.0f - pulse) * 180);
			dl->AddCircle(badge_center, ring_radius, IM_COL32(231, 76, 60, ring_alpha), 0, 2.5f);

			// Second ring (offset phase)
			float ring_radius2 = 10.0f + pulse2 * 20.0f;
			int ring_alpha2 = (int)((1.0f - pulse2) * 180);
			dl->AddCircle(badge_center, ring_radius2, IM_COL32(231, 76, 60, ring_alpha2), 0, 2.5f);
		}

		// Draw badge with scale
		float badge_radius = 10.0f * badge_scale;
		dl->AddCircleFilled(badge_center, badge_radius, IM_COL32(231, 76, 60, 255));

		// Badge text
		int count = *items[i].count;
		if (count > 0)
		{
			char count_text[8];
			if (count > 99)
				snprintf(count_text, sizeof(count_text), "99+");
			else
				snprintf(count_text, sizeof(count_text), "%d", count);

			ImVec2 text_size = ImGui::CalcTextSize(count_text);
			dl->AddText(ImVec2(badge_center.x - text_size.x * 0.5f, badge_center.y - text_size.y * 0.5f),
				IM_COL32_WHITE, count_text);
		}
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE 21: Flip Card
// ============================================================
static void ShowUsecase_FlipCard()
{
	ImGui::TextWrapped(
		"Card that flips to reveal content on the back. "
		"Uses perspective simulation for a 3D-like effect.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static bool flipped = false;

	if (ImGui::Button(flipped ? "Flip to Front" : "Flip to Back"))
		flipped = !flipped;

	ImGuiID id = ImGui::GetID("flip_card");

	// Animate flip (0 = front, 1 = back)
	float target_flip = flipped ? 1.0f : 0.0f;
	float flip_progress = iam_tween_float(id, ImHashStr("flip"), target_flip, 0.6f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 card_size(240, 150);

	// Calculate perspective scaling (card appears to rotate around Y axis)
	float angle = flip_progress * 3.14159f;
	float scale_x = fabsf(ImCos(angle));
	bool show_back = flip_progress > 0.5f;

	// Center the card
	ImVec2 card_center(pos.x + card_size.x * 0.5f, pos.y + card_size.y * 0.5f);
	float half_width = card_size.x * 0.5f * scale_x;
	float half_height = card_size.y * 0.5f;

	ImVec2 card_min(card_center.x - half_width, card_center.y - half_height);
	ImVec2 card_max(card_center.x + half_width, card_center.y + half_height);

	// Only draw if card has some width
	if (scale_x > 0.05f)
	{
		// Shadow
		dl->AddRectFilled(
			ImVec2(card_min.x + 4, card_min.y + 6),
			ImVec2(card_max.x + 4, card_max.y + 6),
			IM_COL32(0, 0, 0, 40), 8.0f);

		if (show_back)
		{
			// Back of card
			dl->AddRectFilled(card_min, card_max, IM_COL32(91, 194, 231, 255), 8.0f);

			// Pattern on back
			float pattern_offset = half_width * 0.3f;
			dl->AddRect(
				ImVec2(card_min.x + pattern_offset, card_min.y + 20),
				ImVec2(card_max.x - pattern_offset, card_max.y - 20),
				IM_COL32(255, 255, 255, 100), 4.0f, 0, 2.0f);

			// Back text
			const char* back_text = "SECRET!";
			ImVec2 text_size = ImGui::CalcTextSize(back_text);
			if (text_size.x < half_width * 1.8f)
			{
				dl->AddText(ImVec2(card_center.x - text_size.x * 0.5f, card_center.y - text_size.y * 0.5f),
					IM_COL32_WHITE, back_text);
			}
		}
		else
		{
			// Front of card
			dl->AddRectFilled(card_min, card_max, IM_COL32(50, 54, 65, 255), 8.0f);

			// Front content
			const char* front_title = "Click to Flip";
			ImVec2 title_size = ImGui::CalcTextSize(front_title);
			if (title_size.x < half_width * 1.8f)
			{
				dl->AddText(ImVec2(card_center.x - title_size.x * 0.5f, card_center.y - 20),
					IM_COL32(220, 220, 230, 255), front_title);

				const char* front_sub = "Hover for info";
				ImVec2 sub_size = ImGui::CalcTextSize(front_sub);
				dl->AddText(ImVec2(card_center.x - sub_size.x * 0.5f, card_center.y + 10),
					IM_COL32(140, 140, 150, 255), front_sub);
			}
		}
	}

	ImGui::Dummy(card_size);
}

// ============================================================
// USECASE 22: Carousel / Image Slider
// ============================================================
static void ShowUsecase_Carousel()
{
	ImGui::TextWrapped(
		"Image carousel with smooth slide transitions. "
		"Includes navigation dots and auto-advance option.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static int current_slide = 0;
	static float auto_timer = 0.0f;
	static bool auto_advance = true;
	const int slide_count = 4;

	ImGui::Checkbox("Auto-advance", &auto_advance);

	if (auto_advance)
	{
		auto_timer += dt;
		if (auto_timer > 3.0f)
		{
			current_slide = (current_slide + 1) % slide_count;
			auto_timer = 0.0f;
		}
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 carousel_size(ImGui::GetContentRegionAvail().x, 140);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + carousel_size.x, pos.y + carousel_size.y),
		IM_COL32(20, 22, 30, 255), 8.0f);

	// Slide colors and labels
	ImU32 slide_colors[] = {
		IM_COL32(91, 194, 231, 255),
		IM_COL32(204, 120, 88, 255),
		IM_COL32(130, 200, 130, 255),
		IM_COL32(200, 180, 100, 255)
	};
	const char* slide_labels[] = { "Slide 1", "Slide 2", "Slide 3", "Slide 4" };

	// Animate slide position
	ImGuiID id = ImGui::GetID("carousel");
	float target_offset = (float)current_slide;
	float offset = iam_tween_float(id, ImHashStr("offset"), target_offset, 0.5f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	// Clip to carousel bounds
	float content_width = carousel_size.x - 80;
	float slide_width = content_width;
	ImVec2 content_pos(pos.x + 40, pos.y + 10);
	ImVec2 content_size(content_width, carousel_size.y - 50);

	dl->PushClipRect(content_pos, ImVec2(content_pos.x + content_size.x, content_pos.y + content_size.y), true);

	// Draw slides
	for (int i = 0; i < slide_count; i++)
	{
		float slide_x = content_pos.x + (i - offset) * slide_width;

		if (slide_x > content_pos.x - slide_width && slide_x < content_pos.x + content_width)
		{
			dl->AddRectFilled(
				ImVec2(slide_x + 4, content_pos.y),
				ImVec2(slide_x + slide_width - 4, content_pos.y + content_size.y),
				slide_colors[i], 6.0f);

			// Slide label
			ImVec2 label_size = ImGui::CalcTextSize(slide_labels[i]);
			dl->AddText(
				ImVec2(slide_x + (slide_width - label_size.x) * 0.5f, content_pos.y + (content_size.y - label_size.y) * 0.5f),
				IM_COL32_WHITE, slide_labels[i]);
		}
	}

	dl->PopClipRect();

	// Navigation arrows
	ImVec2 arrow_size(30, 30);

	// Left arrow
	ImGui::SetCursorScreenPos(ImVec2(pos.x + 5, pos.y + carousel_size.y * 0.5f - 25));
	if (ImGui::InvisibleButton("##carousel_left", arrow_size))
	{
		current_slide = (current_slide - 1 + slide_count) % slide_count;
		auto_timer = 0.0f;
	}
	ImVec2 left_center(pos.x + 20, pos.y + carousel_size.y * 0.5f - 10);
	dl->AddTriangleFilled(
		ImVec2(left_center.x + 8, left_center.y - 10),
		ImVec2(left_center.x + 8, left_center.y + 10),
		ImVec2(left_center.x - 6, left_center.y),
		ImGui::IsItemHovered() ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 180, 255));

	// Right arrow
	ImGui::SetCursorScreenPos(ImVec2(pos.x + carousel_size.x - 35, pos.y + carousel_size.y * 0.5f - 25));
	if (ImGui::InvisibleButton("##carousel_right", arrow_size))
	{
		current_slide = (current_slide + 1) % slide_count;
		auto_timer = 0.0f;
	}
	ImVec2 right_center(pos.x + carousel_size.x - 20, pos.y + carousel_size.y * 0.5f - 10);
	dl->AddTriangleFilled(
		ImVec2(right_center.x - 8, right_center.y - 10),
		ImVec2(right_center.x - 8, right_center.y + 10),
		ImVec2(right_center.x + 6, right_center.y),
		ImGui::IsItemHovered() ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 180, 255));

	// Navigation dots
	float dots_y = pos.y + carousel_size.y - 25;
	float dots_start_x = pos.x + (carousel_size.x - (slide_count * 20)) * 0.5f;

	for (int i = 0; i < slide_count; i++)
	{
		ImVec2 dot_center(dots_start_x + i * 20 + 6, dots_y);

		ImGui::SetCursorScreenPos(ImVec2(dot_center.x - 6, dot_center.y - 6));
		char dot_id[16];
		snprintf(dot_id, sizeof(dot_id), "##dot%d", i);
		if (ImGui::InvisibleButton(dot_id, ImVec2(12, 12)))
		{
			current_slide = i;
			auto_timer = 0.0f;
		}

		float dot_radius = (i == current_slide) ? 5.0f : 4.0f;
		ImU32 dot_color = (i == current_slide) ? IM_COL32(91, 194, 231, 255) : IM_COL32(100, 100, 110, 255);
		if (ImGui::IsItemHovered()) dot_color = IM_COL32(150, 150, 160, 255);

		dl->AddCircleFilled(dot_center, dot_radius, dot_color);
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + carousel_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE 23: Stepper / Timeline
// ============================================================
static void ShowUsecase_Stepper()
{
	ImGui::TextWrapped(
		"Multi-step progress indicator with animated transitions. "
		"Common in checkout flows and onboarding wizards.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static int current_step = 0;
	const int step_count = 4;
	const char* step_labels[] = { "Account", "Details", "Payment", "Confirm" };

	// Navigation buttons
	if (ImGui::Button("< Previous##Stepper") && current_step > 0)
		current_step--;
	ImGui::SameLine();
	if (ImGui::Button("Next >##Stepper") && current_step < step_count - 1)
		current_step++;
	ImGui::SameLine();
	if (ImGui::Button("Reset##Stepper"))
		current_step = 0;

	ImGuiID id = ImGui::GetID("stepper");

	// Animate progress
	float target_progress = (float)current_step / (step_count - 1);
	float progress = iam_tween_float(id, ImHashStr("progress"), target_progress, 0.4f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 stepper_size(ImGui::GetContentRegionAvail().x, 80);

	// Calculate positions
	float padding = 40.0f;
	float step_spacing = (stepper_size.x - padding * 2) / (step_count - 1);
	float line_y = pos.y + 30;
	float circle_radius = 16.0f;

	// Draw background line
	dl->AddLine(
		ImVec2(pos.x + padding, line_y),
		ImVec2(pos.x + stepper_size.x - padding, line_y),
		IM_COL32(60, 65, 80, 255), 4.0f);

	// Draw progress line
	float progress_width = progress * (stepper_size.x - padding * 2);
	if (progress_width > 0)
	{
		dl->AddLine(
			ImVec2(pos.x + padding, line_y),
			ImVec2(pos.x + padding + progress_width, line_y),
			IM_COL32(91, 194, 231, 255), 4.0f);
	}

	// Draw steps
	for (int i = 0; i < step_count; i++)
	{
		float step_x = pos.x + padding + i * step_spacing;
		ImVec2 step_center(step_x, line_y);

		// Calculate step state
		float step_progress = progress * (step_count - 1);
		bool is_complete = i < current_step;
		bool is_current = i == current_step;
		bool is_future = i > current_step;

		// Animate individual step
		ImGuiID step_id = id + i + 1;
		float target_fill = is_complete ? 1.0f : (is_current ? 0.5f : 0.0f);
		float fill = iam_tween_float(step_id, ImHashStr("fill"), target_fill, 0.3f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Step circle
		ImU32 bg_color = IM_COL32(40, 44, 55, 255);
		ImU32 fill_color = IM_COL32(91, 194, 231, 255);
		ImU32 border_color = is_current ? IM_COL32(91, 194, 231, 255) : IM_COL32(80, 85, 100, 255);

		dl->AddCircleFilled(step_center, circle_radius, bg_color);

		if (fill > 0.01f)
		{
			// Partial fill (from bottom)
			float fill_height = circle_radius * 2 * fill;
			dl->PushClipRect(
				ImVec2(step_center.x - circle_radius, step_center.y + circle_radius - fill_height),
				ImVec2(step_center.x + circle_radius, step_center.y + circle_radius),
				true);
			dl->AddCircleFilled(step_center, circle_radius - 2, fill_color);
			dl->PopClipRect();
		}

		dl->AddCircle(step_center, circle_radius, border_color, 0, 2.0f);

		// Step number or checkmark
		if (is_complete)
		{
			// Checkmark
			dl->AddLine(
				ImVec2(step_center.x - 5, step_center.y),
				ImVec2(step_center.x - 1, step_center.y + 4),
				IM_COL32_WHITE, 2.0f);
			dl->AddLine(
				ImVec2(step_center.x - 1, step_center.y + 4),
				ImVec2(step_center.x + 6, step_center.y - 4),
				IM_COL32_WHITE, 2.0f);
		}
		else
		{
			// Number
			char num[4];
			snprintf(num, sizeof(num), "%d", i + 1);
			ImVec2 num_size = ImGui::CalcTextSize(num);
			dl->AddText(
				ImVec2(step_center.x - num_size.x * 0.5f, step_center.y - num_size.y * 0.5f),
				is_future ? IM_COL32(120, 120, 130, 255) : IM_COL32_WHITE, num);
		}

		// Label
		ImVec2 label_size = ImGui::CalcTextSize(step_labels[i]);
		ImU32 label_color = is_future ? IM_COL32(100, 100, 110, 255) : IM_COL32(200, 200, 210, 255);
		dl->AddText(
			ImVec2(step_center.x - label_size.x * 0.5f, line_y + circle_radius + 8),
			label_color, step_labels[i]);
	}

	ImGui::Dummy(stepper_size);
}

// ============================================================
// USECASE 24: Search Bar Expansion
// ============================================================
static void ShowUsecase_SearchExpand()
{
	ImGui::TextWrapped(
		"Search icon that expands into a full search bar on click. "
		"Space-efficient pattern for headers and toolbars.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static bool expanded = false;
	static char search_text[128] = "";

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 80);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	ImGuiID id = ImGui::GetID("search_expand");

	// Animate expansion
	float target_width = expanded ? 280.0f : 48.0f;
	float width = iam_tween_float(id, ImHashStr("width"), target_width, 0.3f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	// Search bar position (right-aligned)
	float bar_height = 44.0f;
	ImVec2 bar_pos(pos.x + canvas_size.x - width - 20, pos.y + (canvas_size.y - bar_height) * 0.5f);

	// Draw search bar background
	dl->AddRectFilled(bar_pos, ImVec2(bar_pos.x + width, bar_pos.y + bar_height),
		IM_COL32(45, 48, 60, 255), bar_height * 0.5f);
	dl->AddRect(bar_pos, ImVec2(bar_pos.x + width, bar_pos.y + bar_height),
		expanded ? IM_COL32(91, 194, 231, 255) : IM_COL32(70, 75, 90, 255), bar_height * 0.5f, 0, 1.5f);

	// Search icon
	ImVec2 icon_center(bar_pos.x + 20, bar_pos.y + bar_height * 0.5f);
	float icon_size = 8.0f;

	// Magnifying glass
	dl->AddCircle(ImVec2(icon_center.x - 2, icon_center.y - 2), icon_size,
		IM_COL32(180, 180, 190, 255), 0, 2.0f);
	dl->AddLine(
		ImVec2(icon_center.x + 4, icon_center.y + 4),
		ImVec2(icon_center.x + 8, icon_center.y + 8),
		IM_COL32(180, 180, 190, 255), 2.0f);

	// Click handler for icon area
	ImGui::SetCursorScreenPos(bar_pos);
	if (ImGui::InvisibleButton("##search_toggle", ImVec2(40, bar_height)))
	{
		expanded = !expanded;
		if (!expanded) search_text[0] = '\0';
	}

	// Input field when expanded
	if (expanded && width > 100)
	{
		float input_alpha = ImClamp((width - 100) / 150.0f, 0.0f, 1.0f);

		// Center the input vertically in the bar
		float input_y = bar_pos.y + (bar_height - ImGui::GetTextLineHeight()) * 0.5f - 2;
		ImGui::SetCursorScreenPos(ImVec2(bar_pos.x + 48, input_y));
		ImGui::PushItemWidth(width - 90);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 220, 230, (int)(255 * input_alpha)));

		ImGui::InputTextWithHint("##search_input", "Search...", search_text, sizeof(search_text));

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();
		ImGui::PopItemWidth();

		// Close button
		ImVec2 close_pos(bar_pos.x + width - 25, bar_pos.y + bar_height * 0.5f);
		ImGui::SetCursorScreenPos(ImVec2(close_pos.x - 10, close_pos.y - 10));
		if (ImGui::InvisibleButton("##search_close", ImVec2(20, 20)))
		{
			expanded = false;
			search_text[0] = '\0';
		}

		// X icon
		float x_alpha = input_alpha * (ImGui::IsItemHovered() ? 1.0f : 0.7f);
		dl->AddLine(
			ImVec2(close_pos.x - 5, close_pos.y - 5),
			ImVec2(close_pos.x + 5, close_pos.y + 5),
			IM_COL32(180, 180, 190, (int)(255 * x_alpha)), 2.0f);
		dl->AddLine(
			ImVec2(close_pos.x + 5, close_pos.y - 5),
			ImVec2(close_pos.x - 5, close_pos.y + 5),
			IM_COL32(180, 180, 190, (int)(255 * x_alpha)), 2.0f);
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE 25: Toggle Switch
// ============================================================
static void ShowUsecase_ToggleSwitch()
{
	ImGui::TextWrapped(
		"iOS-style toggle switches with smooth thumb animation. "
		"Includes color transition and bounce effect.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static bool toggles[3] = { true, false, true };
	const char* labels[] = { "Notifications", "Dark Mode", "Auto-save" };

	ImVec2 pos = ImGui::GetCursorScreenPos();
	float switch_width = 50.0f;
	float switch_height = 28.0f;
	float row_height = 40.0f;

	for (int i = 0; i < 3; i++)
	{
		ImVec2 row_pos(pos.x, pos.y + i * row_height);
		ImGuiID id = ImGui::GetID(labels[i]);

		// Switch position (on the left)
		ImVec2 switch_pos(pos.x, row_pos.y + (row_height - switch_height) * 0.5f);

		// Click handler
		ImGui::SetCursorScreenPos(switch_pos);
		char btn_id[32];
		snprintf(btn_id, sizeof(btn_id), "##toggle_%d", i);
		if (ImGui::InvisibleButton(btn_id, ImVec2(switch_width, switch_height)))
			toggles[i] = !toggles[i];

		bool hovered = ImGui::IsItemHovered();

		// Animate thumb position
		float target_thumb = toggles[i] ? 1.0f : 0.0f;
		float thumb_pos = iam_tween_float(id, ImHashStr("thumb"), target_thumb, 0.25f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		// Animate background color
		ImVec4 off_color(0.3f, 0.32f, 0.38f, 1.0f);
		ImVec4 on_color(0.35f, 0.76f, 0.55f, 1.0f);
		ImVec4 bg_color = iam_tween_color(id, ImHashStr("bg"), toggles[i] ? on_color : off_color, 0.2f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, iam_col_oklab, dt);

		// Draw track
		float track_radius = switch_height * 0.5f;
		dl->AddRectFilled(switch_pos, ImVec2(switch_pos.x + switch_width, switch_pos.y + switch_height),
			ImGui::ColorConvertFloat4ToU32(bg_color), track_radius);

		// Draw thumb
		float thumb_radius = switch_height * 0.5f - 3.0f;
		float thumb_x = switch_pos.x + track_radius + thumb_pos * (switch_width - switch_height);
		float thumb_y = switch_pos.y + switch_height * 0.5f;

		// Thumb shadow
		dl->AddCircleFilled(ImVec2(thumb_x + 1, thumb_y + 2), thumb_radius, IM_COL32(0, 0, 0, 30));

		// Thumb
		ImU32 thumb_color = hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(245, 245, 245, 255);
		dl->AddCircleFilled(ImVec2(thumb_x, thumb_y), thumb_radius, thumb_color);

		// Label (on the right of the toggle)
		ImVec2 label_pos(pos.x + switch_width + 16, row_pos.y + (row_height - ImGui::GetTextLineHeight()) * 0.5f);
		dl->AddText(label_pos, IM_COL32(200, 200, 210, 255), labels[i]);
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 3 * row_height + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE 26: Circular Progress Ring
// ============================================================
static void ShowUsecase_CircularProgress()
{
	ImGui::TextWrapped(
		"Circular progress indicators with smooth fill animation. "
		"Perfect for download progress, skill levels, or completion rates.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static float progress_values[3] = { 0.75f, 0.45f, 0.90f };
	static bool animating = false;
	static float anim_time = 0.0f;

	if (ImGui::Button("Animate Progress##ProgressBars"))
	{
		animating = true;
		anim_time = 0.0f;
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset##ProgressBars"))
	{
		progress_values[0] = 0.0f;
		progress_values[1] = 0.0f;
		progress_values[2] = 0.0f;
		animating = true;
		anim_time = 0.0f;
	}

	if (animating)
	{
		anim_time += dt;
		float t = ImClamp(anim_time / 1.5f, 0.0f, 1.0f);
		float ease_t = iam_eval_preset(iam_ease_out_cubic, t);
		progress_values[0] = ease_t * 0.75f;
		progress_values[1] = ease_t * 0.45f;
		progress_values[2] = ease_t * 0.90f;
		if (t >= 1.0f) animating = false;
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 120);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	const char* labels[] = { "Skills", "Tasks", "Goals" };
	ImU32 colors[] = {
		IM_COL32(91, 194, 231, 255),
		IM_COL32(204, 120, 88, 255),
		IM_COL32(130, 200, 130, 255)
	};

	float ring_radius = 35.0f;
	float ring_thickness = 8.0f;
	float spacing = canvas_size.x / 3.0f;

	for (int i = 0; i < 3; i++)
	{
		ImVec2 center(pos.x + spacing * (i + 0.5f), pos.y + canvas_size.y * 0.45f);

		// Background ring
		dl->PathArcTo(center, ring_radius, 0.0f, IM_PI * 2.0f, 32);
		dl->PathStroke(IM_COL32(45, 48, 58, 255), 0, ring_thickness);

		// Progress arc (starting from top, going clockwise)
		float start_angle = -IM_PI * 0.5f;
		float end_angle = start_angle + progress_values[i] * IM_PI * 2.0f;
		if (progress_values[i] > 0.01f)
		{
			dl->PathArcTo(center, ring_radius, start_angle, end_angle, 32);
			dl->PathStroke(colors[i], 0, ring_thickness);
		}

		// Percentage text
		char pct_text[8];
		snprintf(pct_text, sizeof(pct_text), "%d%%", (int)(progress_values[i] * 100));
		ImVec2 text_size = ImGui::CalcTextSize(pct_text);
		dl->AddText(ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f),
			IM_COL32(220, 220, 230, 255), pct_text);

		// Label below
		ImVec2 label_size = ImGui::CalcTextSize(labels[i]);
		dl->AddText(ImVec2(center.x - label_size.x * 0.5f, pos.y + canvas_size.y - 20),
			IM_COL32(140, 140, 150, 255), labels[i]);
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE 27: Rating Stars
// ============================================================
static void ShowUsecase_RatingStars()
{
	ImGui::TextWrapped(
		"Interactive star rating with hover preview and click animation. "
		"Common pattern for reviews and feedback.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static int rating = 3;
	static int hover_rating = 0;
	static float click_time = 0.0f;
	static int clicked_star = -1;

	if (ImGui::Button("Reset Rating"))
	{
		rating = 0;
		click_time = 0.0f;
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 80);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	const int star_count = 5;
	float star_size = 24.0f;
	float star_spacing = 40.0f;
	float total_width = (star_count - 1) * star_spacing;
	float start_x = pos.x + (canvas_size.x - total_width) * 0.5f;
	float center_y = pos.y + canvas_size.y * 0.5f;

	// Update click animation
	if (click_time > 0.0f)
		click_time -= dt;

	hover_rating = 0;

	for (int i = 0; i < star_count; i++)
	{
		ImVec2 star_center(start_x + i * star_spacing, center_y);

		// Hit testing
		ImGui::SetCursorScreenPos(ImVec2(star_center.x - star_size, star_center.y - star_size));
		char btn_id[16];
		snprintf(btn_id, sizeof(btn_id), "##star_%d", i);
		if (ImGui::InvisibleButton(btn_id, ImVec2(star_size * 2, star_size * 2)))
		{
			rating = i + 1;
			click_time = 0.5f;  // Longer animation duration
			clicked_star = i;
		}
		if (ImGui::IsItemHovered())
			hover_rating = i + 1;

		// Determine if star should be filled
		int display_rating = hover_rating > 0 ? hover_rating : rating;
		bool filled = (i < display_rating);

		// Click animation scale - slower, more noticeable
		float scale = 1.0f;
		if (clicked_star == i && click_time > 0.0f)
		{
			float t = 1.0f - (click_time / 0.5f);  // Slower animation (0.5s instead of 0.3s)
			scale = 1.0f + iam_eval_preset(iam_ease_out_back, t) * 0.5f - t * 0.5f;  // Larger scale effect
		}

		// Draw star shape (5-pointed)
		ImU32 fill_color = filled ? IM_COL32(255, 200, 50, 255) : IM_COL32(60, 65, 80, 255);
		ImU32 outline_color = filled ? IM_COL32(255, 220, 100, 255) : IM_COL32(80, 85, 100, 255);

		// Star shape using concave polygon (5-pointed star)
		float outer_r = star_size * scale;
		float inner_r = outer_r * 0.4f;

		ImVec2 points[10];
		for (int j = 0; j < 10; j++)
		{
			float angle = -IM_PI * 0.5f + j * IM_PI * 0.2f;
			float r = (j % 2 == 0) ? outer_r : inner_r;
			points[j] = ImVec2(star_center.x + ImCos(angle) * r, star_center.y + ImSin(angle) * r);
		}
		// Use AddConcavePolyFilled for proper star rendering
		dl->AddConcavePolyFilled(points, 10, fill_color);
		// Draw outline
		for (int j = 0; j < 10; j++)
			dl->AddLine(points[j], points[(j + 1) % 10], outline_color, 1.5f);
	}

	// Display text
	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y - 20));
	char rating_text[32];
	snprintf(rating_text, sizeof(rating_text), "Rating: %d / 5", rating);
	ImVec2 text_size = ImGui::CalcTextSize(rating_text);
	dl->AddText(ImVec2(pos.x + (canvas_size.x - text_size.x) * 0.5f, pos.y + canvas_size.y - 18),
		IM_COL32(180, 180, 190, 255), rating_text);

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE 28: Countdown Timer
// ============================================================
static void ShowUsecase_CountdownTimer()
{
	ImGui::TextWrapped(
		"Animated countdown timer with smooth digit transitions. "
		"Used for sales, events, or time-limited offers.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static float countdown = 125.0f; // 2:05 in seconds
	static bool running = true;

	if (running)
		countdown -= dt;
	if (countdown < 0) countdown = 0;

	if (ImGui::Button(running ? "Pause" : "Resume"))
		running = !running;
	ImGui::SameLine();
	if (ImGui::Button("Reset (5:00)"))
	{
		countdown = 300.0f;
		running = true;
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 100);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	// Parse time
	int total_seconds = (int)countdown;
	int minutes = total_seconds / 60;
	int seconds = total_seconds % 60;

	// Draw time centered in canvas (simple overlapping style)
	float font_scale = 3.5f;

	// Format time string
	char time_str[16];
	snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);

	// Calculate size at scaled font
	ImVec2 text_size = ImGui::CalcTextSize(time_str);
	text_size.x *= font_scale;
	text_size.y *= font_scale;

	// Center position
	ImVec2 text_pos(
		pos.x + (canvas_size.x - text_size.x) * 0.5f,
		pos.y + (canvas_size.y - text_size.y) * 0.5f
	);

	// Animate scale when second changes
	float anim_scale = font_scale;
	if (running)
	{
		float frac = countdown - floorf(countdown);
		if (frac > 0.9f)
		{
			float t = (frac - 0.9f) / 0.1f;
			anim_scale = font_scale * (1.0f + t * 0.05f);  // Subtle pulse
		}
	}

	// Determine color
	ImU32 text_color = countdown <= 10 ? IM_COL32(231, 76, 60, 255) : IM_COL32(220, 220, 230, 255);

	// Draw the time (overlapping the background directly)
	dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * anim_scale, text_pos, text_color, time_str);

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE 29: Breadcrumb Navigation
// ============================================================
static void ShowUsecase_Breadcrumb()
{
	ImGui::TextWrapped(
		"Animated breadcrumb navigation with hover effects. "
		"Shows hierarchical path with clickable items.");

	float dt = GetUsecaseDeltaTime();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	static int current_level = 3;
	const char* items[] = { "Home", "Products", "Electronics", "Laptops", "Gaming" };
	const int item_count = 5;

	if (ImGui::Button("Go Back##Breadcrumb") && current_level > 0)
		current_level--;
	ImGui::SameLine();
	if (ImGui::Button("Go Forward##Breadcrumb") && current_level < item_count - 1)
		current_level++;
	ImGui::SameLine();
	if (ImGui::Button("Reset##Breadcrumb"))
		current_level = 0;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 50);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 27, 35, 255), 4.0f);

	float x = pos.x + 16.0f;
	float center_y = pos.y + canvas_size.y * 0.5f;

	for (int i = 0; i <= current_level && i < item_count; i++)
	{
		bool is_current = (i == current_level);
		ImGuiID id = ImGui::GetID(items[i]);

		// Calculate text size
		ImVec2 text_size = ImGui::CalcTextSize(items[i]);
		float padding = 8.0f;

		// Hover detection
		ImGui::SetCursorScreenPos(ImVec2(x - padding, center_y - text_size.y * 0.5f - padding));
		char btn_id[32];
		snprintf(btn_id, sizeof(btn_id), "##bread_%d", i);
		if (ImGui::InvisibleButton(btn_id, ImVec2(text_size.x + padding * 2, text_size.y + padding * 2)))
		{
			if (!is_current)
				current_level = i;
		}
		bool hovered = ImGui::IsItemHovered();

		// Animate hover - slower for more noticeable effect
		float target_hover = hovered ? 1.0f : 0.0f;
		float hover_anim = iam_tween_float(id, ImHashStr("hover"), target_hover, 0.35f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Colors
		ImU32 text_color;
		if (is_current)
			text_color = IM_COL32(91, 194, 231, 255);
		else
		{
			int alpha = (int)(150 + hover_anim * 105);
			text_color = IM_COL32(180, 180, 190, alpha);
		}

		// Hover background
		if (hover_anim > 0.01f && !is_current)
		{
			dl->AddRectFilled(
				ImVec2(x - padding, center_y - text_size.y * 0.5f - 4),
				ImVec2(x + text_size.x + padding, center_y + text_size.y * 0.5f + 4),
				IM_COL32(60, 65, 80, (int)(100 * hover_anim)), 4.0f);
		}

		// Text
		dl->AddText(ImVec2(x, center_y - text_size.y * 0.5f), text_color, items[i]);

		x += text_size.x + padding * 2;

		// Separator arrow
		if (i < current_level)
		{
			dl->AddText(ImVec2(x, center_y - text_size.y * 0.5f), IM_COL32(100, 100, 110, 255), ">");
			x += ImGui::CalcTextSize(">").x + padding * 2;
		}
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 30. Animated Checkbox
// Smooth checkmark drawing animation with scale effect
// ============================================================
static void ShowUsecase_AnimatedCheckbox()
{
	ImGui::TextWrapped("Animated checkboxes with smooth checkmark drawing and scale effect:");

	static bool checkboxes[3] = { false, true, false };
	static float check_anims[3] = { 0.0f, 1.0f, 0.0f };
	const char* labels[3] = { "Enable notifications", "Dark mode", "Auto-save" };

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	float line_height = 32.0f * scale;

	for (int i = 0; i < 3; i++)
	{
		ImGui::PushID(i);

		float box_size = 22.0f * scale;
		ImVec2 box_pos(pos.x, pos.y + i * line_height);
		ImVec2 box_min = box_pos;
		ImVec2 box_max(box_pos.x + box_size, box_pos.y + box_size);

		// Invisible button for interaction
		ImGui::SetCursorScreenPos(box_pos);
		if (ImGui::InvisibleButton("##check", ImVec2(box_size + ImGui::CalcTextSize(labels[i]).x + 10 * scale, box_size)))
		{
			checkboxes[i] = !checkboxes[i];
		}

		// Animate check state
		float target = checkboxes[i] ? 1.0f : 0.0f;
		ImGuiID id = ImGui::GetID("check_anim");
		check_anims[i] = iam_tween_float(id, ImHashStr("anim"), target, 0.25f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		float anim = check_anims[i];

		// Box background
		ImU32 box_bg = ImLerp(IM_COL32(50, 55, 65, 255), IM_COL32(76, 175, 80, 255), anim);
		float box_scale = 1.0f + iam_eval_preset(iam_ease_out_back, anim) * 0.1f - anim * 0.1f;

		ImVec2 center((box_min.x + box_max.x) * 0.5f, (box_min.y + box_max.y) * 0.5f);
		ImVec2 scaled_min(center.x - box_size * 0.5f * box_scale, center.y - box_size * 0.5f * box_scale);
		ImVec2 scaled_max(center.x + box_size * 0.5f * box_scale, center.y + box_size * 0.5f * box_scale);

		dl->AddRectFilled(scaled_min, scaled_max, box_bg, 4.0f * scale);
		dl->AddRect(scaled_min, scaled_max, IM_COL32(255, 255, 255, 50), 4.0f * scale);

		// Draw animated checkmark
		if (anim > 0.01f)
		{
			float check_progress = ImClamp(anim * 1.2f, 0.0f, 1.0f);

			// Checkmark points (relative to box)
			ImVec2 p1(center.x - box_size * 0.25f, center.y);
			ImVec2 p2(center.x - box_size * 0.05f, center.y + box_size * 0.2f);
			ImVec2 p3(center.x + box_size * 0.3f, center.y - box_size * 0.2f);

			float thickness = 2.5f * scale;

			// First segment (short down stroke)
			if (check_progress > 0.0f)
			{
				float seg1 = ImClamp(check_progress * 2.5f, 0.0f, 1.0f);
				ImVec2 end1 = ImLerp(p1, p2, seg1);
				dl->AddLine(p1, end1, IM_COL32(255, 255, 255, 255), thickness);
			}

			// Second segment (long up stroke)
			if (check_progress > 0.4f)
			{
				float seg2 = ImClamp((check_progress - 0.4f) * 2.5f, 0.0f, 1.0f);
				ImVec2 end2 = ImLerp(p2, p3, seg2);
				dl->AddLine(p2, end2, IM_COL32(255, 255, 255, 255), thickness);
			}
		}

		// Label
		dl->AddText(ImVec2(box_max.x + 8 * scale, box_pos.y + (box_size - ImGui::GetFontSize()) * 0.5f),
			IM_COL32(220, 220, 230, 255), labels[i]);

		ImGui::PopID();
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 3 * line_height + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 31. Slide-in Drawer
// Side panel that slides in/out smoothly
// ============================================================
static void ShowUsecase_SlideDrawer()
{
	ImGui::TextWrapped("Click the hamburger menu to toggle the side drawer:");

	static bool drawer_open = false;
	static float drawer_anim = 0.0f;

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 canvas_size(400, 380);  // Taller canvas for menu visibility
	ImVec2 pos = ImGui::GetCursorScreenPos();

	// Background (app content)
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(30, 35, 45, 255), 8.0f);

	// Header bar
	float header_height = 40.0f * scale;
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + header_height),
		IM_COL32(45, 50, 65, 255), 8.0f, ImDrawFlags_RoundCornersTop);

	// Hamburger menu button
	ImVec2 menu_pos(pos.x + 12 * scale, pos.y + header_height * 0.5f);
	ImGui::SetCursorScreenPos(ImVec2(pos.x + 4 * scale, pos.y + 4 * scale));
	if (ImGui::InvisibleButton("##menu", ImVec2(32 * scale, 32 * scale)))
	{
		drawer_open = !drawer_open;
	}

	// Draw hamburger/close icon
	float icon_anim = drawer_anim;
	float line_len = 14.0f * scale;
	float line_gap = 5.0f * scale;

	// Animate between hamburger and X
	for (int i = 0; i < 3; i++)
	{
		float y_offset = (i - 1) * line_gap * (1.0f - icon_anim);
		float rotation = icon_anim * (i == 1 ? 0.0f : (i == 0 ? 0.785f : -0.785f));
		float length = i == 1 ? line_len * (1.0f - icon_anim) : line_len;

		if (i == 1 && icon_anim > 0.5f) continue; // Hide middle line when transitioning to X

		float cos_r = ImCos(rotation);
		float sin_r = ImSin(rotation);

		ImVec2 start(menu_pos.x - length * 0.5f * cos_r, menu_pos.y + y_offset - length * 0.5f * sin_r);
		ImVec2 end(menu_pos.x + length * 0.5f * cos_r, menu_pos.y + y_offset + length * 0.5f * sin_r);

		dl->AddLine(start, end, IM_COL32(200, 200, 210, 255), 2.0f * scale);
	}

	// Title
	dl->AddText(ImVec2(pos.x + 44 * scale, pos.y + (header_height - ImGui::GetFontSize()) * 0.5f),
		IM_COL32(220, 220, 230, 255), "My Application");

	// Animate drawer
	float target = drawer_open ? 1.0f : 0.0f;
	ImGuiID id = ImGui::GetID("drawer");
	drawer_anim = iam_tween_float(id, ImHashStr("slide"), target, 0.35f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	// Drawer panel
	float drawer_width = 160.0f * scale;
	float drawer_x = pos.x - drawer_width + drawer_width * drawer_anim;

	if (drawer_anim > 0.01f)
	{
		// Dimmed backdrop
		dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
			IM_COL32(0, 0, 0, (int)(100 * drawer_anim)), 8.0f);

		// Drawer panel
		dl->AddRectFilled(ImVec2(drawer_x, pos.y), ImVec2(drawer_x + drawer_width, pos.y + canvas_size.y),
			IM_COL32(40, 45, 55, 255));

		// Drawer header
		dl->AddRectFilled(ImVec2(drawer_x, pos.y), ImVec2(drawer_x + drawer_width, pos.y + header_height),
			IM_COL32(55, 60, 75, 255));
		dl->AddText(ImVec2(drawer_x + 16 * scale, pos.y + (header_height - ImGui::GetFontSize()) * 0.5f),
			IM_COL32(200, 200, 220, 255), "Menu");

		// Menu items
		const char* menu_items[] = { "Home", "Profile", "Settings" };
		for (int i = 0; i < 3; i++)
		{
			float item_y = pos.y + header_height + 8 * scale + i * 36 * scale;
			dl->AddText(ImVec2(drawer_x + 16 * scale, item_y), IM_COL32(180, 180, 190, 255), menu_items[i]);
		}
	}

	// Content placeholder text
	dl->AddText(ImVec2(pos.x + 20 * scale, pos.y + header_height + 20 * scale),
		IM_COL32(120, 120, 130, 255), "Main content area...");

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(canvas_size.x, 1));
}

// ============================================================
// 32. Animated Radio Buttons
// Radio button group with smooth selection animation
// ============================================================
static void ShowUsecase_AnimatedRadio()
{
	ImGui::TextWrapped("Radio button group with smooth selection transitions:");

	static int selected = 0;
	static float selection_anims[4] = { 1.0f, 0.0f, 0.0f, 0.0f };

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	const char* options[] = { "Small", "Medium", "Large", "Extra Large" };

	ImVec2 pos = ImGui::GetCursorScreenPos();
	float line_height = 32.0f * scale;
	float radio_size = 20.0f * scale;

	for (int i = 0; i < 4; i++)
	{
		ImGui::PushID(i);

		ImVec2 radio_pos(pos.x, pos.y + i * line_height);
		ImVec2 center(radio_pos.x + radio_size * 0.5f, radio_pos.y + radio_size * 0.5f);

		// Invisible button for interaction
		ImGui::SetCursorScreenPos(radio_pos);
		if (ImGui::InvisibleButton("##radio", ImVec2(radio_size + ImGui::CalcTextSize(options[i]).x + 10 * scale, radio_size)))
		{
			selected = i;
		}

		// Animate selection
		float target = (selected == i) ? 1.0f : 0.0f;
		ImGuiID id = ImGui::GetID("radio_anim");
		selection_anims[i] = iam_tween_float(id, ImHashStr("sel"), target, 0.2f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		float anim = selection_anims[i];

		// Outer circle
		ImU32 outer_color = ImLerp(IM_COL32(80, 85, 100, 255), IM_COL32(76, 175, 80, 255), anim);
		dl->AddCircle(center, radio_size * 0.5f, outer_color, 24, 2.0f * scale);

		// Inner dot with scale animation
		if (anim > 0.01f)
		{
			float inner_radius = radio_size * 0.25f * iam_eval_preset(iam_ease_out_back, anim);
			dl->AddCircleFilled(center, inner_radius, IM_COL32(76, 175, 80, 255), 16);
		}

		// Label
		dl->AddText(ImVec2(radio_pos.x + radio_size + 8 * scale, radio_pos.y + (radio_size - ImGui::GetFontSize()) * 0.5f),
			IM_COL32(220, 220, 230, 255), options[i]);

		ImGui::PopID();
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 4 * line_height + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 33. Typing Text Effect
// Typewriter-style text reveal animation
// ============================================================
static void ShowUsecase_TypingText()
{
	ImGui::TextWrapped("Typewriter text animation with cursor:");

	static float char_timer = 0.0f;
	static int visible_chars = 0;
	static bool cursor_visible = true;
	static float cursor_timer = 0.0f;

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	const char* full_text = "Hello! Welcome to ImAnim demo...";
	int text_len = (int)strlen(full_text);

	// Typing animation
	char_timer += dt;
	float char_delay = 0.06f;
	if (char_timer >= char_delay && visible_chars < text_len)
	{
		visible_chars++;
		char_timer = 0.0f;
	}

	// Cursor blink
	cursor_timer += dt;
	if (cursor_timer >= 0.5f)
	{
		cursor_visible = !cursor_visible;
		cursor_timer = 0.0f;
	}

	// Reset button
	if (ImGui::Button("Restart"))
	{
		visible_chars = 0;
		char_timer = 0.0f;
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 60);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 8.0f);

	// Draw visible text
	char visible_text[256];
	int copy_len = ImMin(visible_chars, (int)sizeof(visible_text) - 1);
	strncpy(visible_text, full_text, copy_len);
	visible_text[copy_len] = '\0';

	ImVec2 text_pos(pos.x + 16 * scale, pos.y + (canvas_size.y - ImGui::GetFontSize()) * 0.5f);
	dl->AddText(text_pos, IM_COL32(200, 255, 200, 255), visible_text);

	// Cursor
	if (cursor_visible)
	{
		ImVec2 text_size = ImGui::CalcTextSize(visible_text);
		float cursor_x = text_pos.x + text_size.x + 2;
		float cursor_h = ImGui::GetFontSize();
		dl->AddRectFilled(
			ImVec2(cursor_x, text_pos.y),
			ImVec2(cursor_x + 2 * scale, text_pos.y + cursor_h),
			IM_COL32(200, 255, 200, 255));
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 34. Shake Animation
// Error feedback shake effect for form validation
// ============================================================
static void ShowUsecase_ShakeAnimation()
{
	ImGui::TextWrapped("Click 'Submit' with empty input to see shake animation:");

	static char input_text[64] = "";
	static float shake_time = 0.0f;
	static bool is_shaking = false;

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(300, 120);  // Taller canvas for better layout

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(30, 35, 45, 255), 8.0f);

	// Update shake
	if (is_shaking)
	{
		shake_time -= dt;
		if (shake_time <= 0.0f)
		{
			is_shaking = false;
			shake_time = 0.0f;
		}
	}

	// Calculate shake offset
	float shake_offset = 0.0f;
	if (is_shaking)
	{
		float shake_freq = 25.0f;
		float shake_decay = shake_time / 0.4f;
		shake_offset = ImSin(shake_time * shake_freq) * 8.0f * scale * shake_decay;
	}

	// Input field
	float input_y = pos.y + 24 * scale;
	float input_width = 200 * scale;
	float input_height = 32 * scale;

	ImVec2 input_min(pos.x + 16 * scale + shake_offset, input_y);
	ImVec2 input_max(input_min.x + input_width, input_y + input_height);

	ImU32 input_bg = is_shaking ? IM_COL32(80, 40, 40, 255) : IM_COL32(45, 50, 60, 255);
	ImU32 input_border = is_shaking ? IM_COL32(220, 80, 80, 255) : IM_COL32(70, 75, 90, 255);

	dl->AddRectFilled(input_min, input_max, input_bg, 4.0f);
	dl->AddRect(input_min, input_max, input_border, 4.0f);

	// Input text - position it vertically centered in the input box
	ImGui::SetCursorScreenPos(ImVec2(input_min.x + 8 * scale, input_y + (input_height - ImGui::GetFontSize()) * 0.5f));
	ImGui::PushItemWidth(input_width - 16 * scale);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
	ImGui::InputText("##shake_input", input_text, sizeof(input_text));
	ImGui::PopStyleColor(2);
	ImGui::PopItemWidth();

	// Placeholder
	if (strlen(input_text) == 0)
	{
		dl->AddText(ImVec2(input_min.x + 8 * scale, input_y + (input_height - ImGui::GetFontSize()) * 0.5f),
			IM_COL32(100, 100, 110, 255), "Enter your name...");
	}

	// Submit button
	ImVec2 btn_min(pos.x + 16 * scale + shake_offset, input_y + input_height + 12 * scale);
	ImVec2 btn_max(btn_min.x + 80 * scale, btn_min.y + 28 * scale);

	dl->AddRectFilled(btn_min, btn_max, IM_COL32(76, 175, 80, 255), 4.0f);
	ImVec2 btn_text_size = ImGui::CalcTextSize("Submit");
	dl->AddText(ImVec2(btn_min.x + (80 * scale - btn_text_size.x) * 0.5f,
		btn_min.y + (28 * scale - btn_text_size.y) * 0.5f),
		IM_COL32(255, 255, 255, 255), "Submit");

	ImGui::SetCursorScreenPos(btn_min);
	if (ImGui::InvisibleButton("##submit", ImVec2(80 * scale, 28 * scale)))
	{
		if (strlen(input_text) == 0)
		{
			is_shaking = true;
			shake_time = 0.4f;
		}
	}

	// Error message
	if (is_shaking)
	{
		dl->AddText(ImVec2(btn_max.x + 12 * scale, btn_min.y + (28 * scale - ImGui::GetFontSize()) * 0.5f),
			IM_COL32(220, 80, 80, 255), "Required field!");
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(canvas_size.x, 80 * scale));  // Extra space to prevent overlap
}

// ============================================================
// 35. Animated Tags/Chips
// Removable tags with smooth add/remove animations
// ============================================================
static void ShowUsecase_AnimatedTags()
{
	ImGui::TextWrapped("Add and remove skill tags with smooth animations:");

	// Game Graphics Engineer skill tags
	static const char* available_tags[] = { "DirectX", "Vulkan", "OpenGL", "HLSL", "GLSL", "Raytracing", "Shaders", "GPU" };
	static bool tag_active[8] = { true, true, true, true, false, false, false, false };
	static float tag_scales[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	static float x_hover_anims[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 280);  // Taller canvas to fit all tags

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(30, 35, 45, 255), 8.0f);

	// Tag colors (graphics/game engine themed)
	ImU32 tag_colors[] = {
		IM_COL32(0, 120, 215, 255),    // DirectX - Microsoft blue
		IM_COL32(172, 31, 45, 255),    // Vulkan - red
		IM_COL32(91, 178, 74, 255),    // OpenGL - green
		IM_COL32(255, 140, 0, 255),    // HLSL - orange
		IM_COL32(100, 200, 100, 255),  // GLSL - light green
		IM_COL32(118, 185, 0, 255),    // Raytracing - nvidia green
		IM_COL32(180, 100, 220, 255),  // Shaders - purple
		IM_COL32(230, 190, 0, 255)     // GPU - gold
	};

	float x = pos.x + 16 * scale;
	float y = pos.y + 16 * scale;
	float tag_height = 28 * scale;
	float padding = 10 * scale;
	float spacing = 8 * scale;

	for (int i = 0; i < 8; i++)
	{
		ImGui::PushID(i);

		// Animate scale (slower disappear animation)
		float target_scale = tag_active[i] ? 1.0f : 0.0f;
		ImGuiID id = ImGui::GetID("tag");
		tag_scales[i] = iam_tween_float(id, ImHashStr("scale"), target_scale, 0.6f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		if (tag_scales[i] > 0.02f)
		{
			float anim_scale = tag_scales[i];
			ImVec2 text_size = ImGui::CalcTextSize(available_tags[i]);
			float tag_width = (text_size.x + padding * 2 + 18 * scale) * anim_scale;

			// Wrap to next line
			if (x + tag_width > pos.x + canvas_size.x - 16 * scale)
			{
				x = pos.x + 16 * scale;
				y += tag_height + spacing;
			}

			ImVec2 tag_min(x, y + (tag_height - tag_height * anim_scale) * 0.5f);
			ImVec2 tag_max(x + tag_width, tag_min.y + tag_height * anim_scale);

			// Tag background
			dl->AddRectFilled(tag_min, tag_max, tag_colors[i], tag_height * anim_scale * 0.5f);

			// Tag text
			float text_alpha = ImClamp(anim_scale * 2.0f - 0.5f, 0.0f, 1.0f);
			dl->AddText(ImVec2(tag_min.x + padding * anim_scale, tag_min.y + (tag_max.y - tag_min.y - text_size.y * anim_scale) * 0.5f),
				IM_COL32(30, 30, 30, (int)(255 * text_alpha)), available_tags[i]);

			// Remove button (X)
			float x_size = 12 * scale * anim_scale;
			ImVec2 x_center(tag_max.x - padding * anim_scale - x_size * 0.3f, (tag_min.y + tag_max.y) * 0.5f);

			ImGui::SetCursorScreenPos(ImVec2(x_center.x - x_size * 0.7f, x_center.y - x_size * 0.7f));
			if (ImGui::InvisibleButton("##remove", ImVec2(x_size * 1.4f, x_size * 1.4f)))
			{
				tag_active[i] = false;
			}
			bool x_hovered = ImGui::IsItemHovered();

			// Animate X hover
			float x_hover_target = x_hovered ? 1.0f : 0.0f;
			x_hover_anims[i] = iam_tween_float(id, ImHashStr("xhover"), x_hover_target, 0.15f,
				iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

			// Draw X with hover animation
			float hover_scale = 1.0f + x_hover_anims[i] * 0.4f;  // Scale up on hover
			float x_half = x_size * 0.25f * hover_scale;
			float line_thickness = (2.0f + x_hover_anims[i] * 1.0f) * scale * anim_scale;  // Thicker on hover

			// X color: darker on hover, with red tint
			int r = (int)(30 + x_hover_anims[i] * 170);  // 30 -> 200 (red)
			int g = (int)(30 - x_hover_anims[i] * 30);   // 30 -> 0
			int b = (int)(30 - x_hover_anims[i] * 30);   // 30 -> 0
			ImU32 x_color = IM_COL32(r, g, b, (int)(255 * text_alpha));

			dl->AddLine(ImVec2(x_center.x - x_half, x_center.y - x_half),
				ImVec2(x_center.x + x_half, x_center.y + x_half),
				x_color, line_thickness);
			dl->AddLine(ImVec2(x_center.x + x_half, x_center.y - x_half),
				ImVec2(x_center.x - x_half, x_center.y + x_half),
				x_color, line_thickness);

			x += tag_width + spacing;
		}

		ImGui::PopID();
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));

	// Add tag buttons
	ImGui::Text("Add:");
	ImGui::SameLine();
	for (int i = 0; i < 8; i++)
	{
		if (!tag_active[i] && tag_scales[i] < 0.1f)
		{
			ImGui::PushID(i + 100);
			if (ImGui::SmallButton(available_tags[i]))
			{
				tag_active[i] = true;
			}
			ImGui::SameLine();
			ImGui::PopID();
		}
	}
	ImGui::NewLine();
}

// ============================================================
// 36. Pending Button
// Button with loading spinner when processing
// ============================================================
static void ShowUsecase_PendingButton()
{
	ImGui::TextWrapped("Click the button to trigger a loading state with spinner:");

	static bool is_pending[3] = { false, false, false };
	static float pending_time[3] = { 0.0f, 0.0f, 0.0f };
	static float spinner_angle[3] = { 0.0f, 0.0f, 0.0f };

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	const char* button_labels[] = { "Save", "Submit", "Upload" };

	ImVec2 pos = ImGui::GetCursorScreenPos();
	float button_width = 100.0f * scale;
	float button_height = 40.0f * scale;
	float spacing = 20.0f * scale;

	for (int i = 0; i < 3; i++)
	{
		ImGui::PushID(i);

		float x = pos.x + i * (button_width + spacing);
		ImVec2 btn_min(x, pos.y);
		ImVec2 btn_max(x + button_width, pos.y + button_height);
		ImVec2 btn_center((btn_min.x + btn_max.x) * 0.5f, (btn_min.y + btn_max.y) * 0.5f);

		// Update pending state
		if (is_pending[i])
		{
			pending_time[i] += dt;
			spinner_angle[i] += dt * 8.0f;  // Spinner rotation speed
			if (pending_time[i] > 2.0f)  // Auto-complete after 2 seconds
			{
				is_pending[i] = false;
				pending_time[i] = 0.0f;
			}
		}

		// Button background
		ImU32 btn_color = is_pending[i] ? IM_COL32(60, 65, 80, 255) : IM_COL32(76, 175, 80, 255);
		dl->AddRectFilled(btn_min, btn_max, btn_color, 6.0f * scale);

		// Click handler
		ImGui::SetCursorScreenPos(btn_min);
		if (ImGui::InvisibleButton("##btn", ImVec2(button_width, button_height)) && !is_pending[i])
		{
			is_pending[i] = true;
			pending_time[i] = 0.0f;
		}

		// Button text
		const char* label = is_pending[i] ? "..." : button_labels[i];
		ImVec2 text_size = ImGui::CalcTextSize(label);
		float text_x = btn_center.x - text_size.x * 0.5f;
		if (is_pending[i]) text_x -= 8 * scale;  // Shift left when spinner visible
		dl->AddText(ImVec2(text_x, btn_center.y - text_size.y * 0.5f),
			IM_COL32(255, 255, 255, 255), label);

		// Spinner on top-right when pending
		if (is_pending[i])
		{
			float spinner_radius = 8.0f * scale;
			ImVec2 spinner_center(btn_max.x - 16 * scale, btn_min.y + 16 * scale);

			// Draw spinner arc
			float start_angle = spinner_angle[i];
			float arc_length = IM_PI * 1.5f;
			int segments = 20;
			for (int s = 0; s < segments; s++)
			{
				float t1 = (float)s / segments;
				float t2 = (float)(s + 1) / segments;
				float a1 = start_angle + t1 * arc_length;
				float a2 = start_angle + t2 * arc_length;
				int alpha = (int)(255 * t2);  // Fade along arc
				dl->AddLine(
					ImVec2(spinner_center.x + ImCos(a1) * spinner_radius, spinner_center.y + ImSin(a1) * spinner_radius),
					ImVec2(spinner_center.x + ImCos(a2) * spinner_radius, spinner_center.y + ImSin(a2) * spinner_radius),
					IM_COL32(255, 255, 255, alpha), 2.0f * scale);
			}
		}

		ImGui::PopID();
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + button_height + 16));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 37. Animated Slider
// Custom slider with smooth thumb and track animation
// ============================================================
static void ShowUsecase_AnimatedSlider()
{
	ImGui::TextWrapped("Custom slider with animated thumb and glowing track:");

	static float slider_values[2] = { 0.3f, 0.7f };

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	const char* slider_labels[] = { "Volume", "Brightness" };

	ImVec2 pos = ImGui::GetCursorScreenPos();
	float slider_width = 250.0f * scale;
	float slider_height = 8.0f * scale;
	float thumb_radius = 12.0f * scale;
	float line_height = 50.0f * scale;

	for (int i = 0; i < 2; i++)
	{
		ImGui::PushID(i);

		float y = pos.y + i * line_height;

		// Label
		dl->AddText(ImVec2(pos.x, y), IM_COL32(200, 200, 210, 255), slider_labels[i]);

		// Track position
		float track_x = pos.x + 100 * scale;
		float track_y = y + ImGui::GetFontSize() * 0.5f - slider_height * 0.5f;

		// Track background
		dl->AddRectFilled(
			ImVec2(track_x, track_y),
			ImVec2(track_x + slider_width, track_y + slider_height),
			IM_COL32(50, 55, 65, 255), slider_height * 0.5f);

		// Filled portion with glow
		float fill_width = slider_values[i] * slider_width;
		ImU32 fill_color = (i == 0) ? IM_COL32(76, 175, 80, 255) : IM_COL32(255, 193, 7, 255);
		dl->AddRectFilled(
			ImVec2(track_x, track_y),
			ImVec2(track_x + fill_width, track_y + slider_height),
			fill_color, slider_height * 0.5f);

		// Thumb position
		float thumb_x = track_x + fill_width;
		float thumb_y = track_y + slider_height * 0.5f;

		// Invisible button for dragging
		ImGui::SetCursorScreenPos(ImVec2(track_x - thumb_radius, track_y - thumb_radius));
		ImGui::InvisibleButton("##slider", ImVec2(slider_width + thumb_radius * 2, slider_height + thumb_radius * 2));

		if (ImGui::IsItemActive())
		{
			float mouse_x = ImGui::GetIO().MousePos.x;
			slider_values[i] = ImClamp((mouse_x - track_x) / slider_width, 0.0f, 1.0f);
		}

		bool hovered = ImGui::IsItemHovered() || ImGui::IsItemActive();

		// Animate thumb scale
		ImGuiID id = ImGui::GetID("slider");
		float target_scale = hovered ? 1.3f : 1.0f;
		float thumb_scale = iam_tween_float(id, ImHashStr("scale"), target_scale, 0.15f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Draw thumb glow when hovered
		if (thumb_scale > 1.1f)
		{
			dl->AddCircleFilled(ImVec2(thumb_x, thumb_y), thumb_radius * thumb_scale * 1.5f,
				IM_COL32(255, 255, 255, 30));
		}

		// Thumb
		dl->AddCircleFilled(ImVec2(thumb_x, thumb_y), thumb_radius * thumb_scale, IM_COL32(255, 255, 255, 255));
		dl->AddCircle(ImVec2(thumb_x, thumb_y), thumb_radius * thumb_scale, fill_color, 0, 2.0f * scale);

		// Value text
		char value_text[16];
		snprintf(value_text, sizeof(value_text), "%.0f%%", slider_values[i] * 100);
		ImVec2 value_size = ImGui::CalcTextSize(value_text);
		dl->AddText(ImVec2(track_x + slider_width + 10 * scale, y), IM_COL32(180, 180, 190, 255), value_text);

		ImGui::PopID();
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 2 * line_height + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 38. Color Swatch Animation
// Color picker swatches with selection animation
// ============================================================
static void ShowUsecase_ColorSwatches()
{
	ImGui::TextWrapped("Click to select a color with smooth animation:");

	static int selected_color = 2;
	static float selection_anim = 1.0f;

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImU32 colors[] = {
		IM_COL32(231, 76, 60, 255),    // Red
		IM_COL32(230, 126, 34, 255),   // Orange
		IM_COL32(241, 196, 15, 255),   // Yellow
		IM_COL32(46, 204, 113, 255),   // Green
		IM_COL32(52, 152, 219, 255),   // Blue
		IM_COL32(155, 89, 182, 255),   // Purple
		IM_COL32(236, 240, 241, 255),  // White
		IM_COL32(44, 62, 80, 255)      // Dark
	};

	ImVec2 pos = ImGui::GetCursorScreenPos();
	float swatch_size = 36.0f * scale;
	float spacing = 10.0f * scale;

	for (int i = 0; i < 8; i++)
	{
		ImGui::PushID(i);

		float x = pos.x + i * (swatch_size + spacing);
		ImVec2 swatch_min(x, pos.y);
		ImVec2 swatch_max(x + swatch_size, pos.y + swatch_size);
		ImVec2 swatch_center((swatch_min.x + swatch_max.x) * 0.5f, (swatch_min.y + swatch_max.y) * 0.5f);

		// Click handler
		ImGui::SetCursorScreenPos(swatch_min);
		if (ImGui::InvisibleButton("##swatch", ImVec2(swatch_size, swatch_size)))
		{
			selected_color = i;
		}
		bool hovered = ImGui::IsItemHovered();

		// Animate selection
		float target = (i == selected_color) ? 1.0f : 0.0f;
		ImGuiID id = ImGui::GetID("swatch");
		float anim = iam_tween_float(id, ImHashStr("sel"), target, 0.2f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		// Hover animation
		float hover_target = hovered ? 1.0f : 0.0f;
		float hover_anim = iam_tween_float(id, ImHashStr("hover"), hover_target, 0.1f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Scale based on selection and hover
		float swatch_scale = 1.0f + anim * 0.15f + hover_anim * 0.08f;

		// Selection ring
		if (anim > 0.01f)
		{
			float ring_radius = swatch_size * 0.5f * swatch_scale + 4 * scale * anim;
			dl->AddCircle(swatch_center, ring_radius, IM_COL32(255, 255, 255, (int)(200 * anim)), 0, 2.5f * scale);
		}

		// Swatch
		float r = swatch_size * 0.5f * swatch_scale;
		dl->AddCircleFilled(swatch_center, r, colors[i]);

		// Checkmark for selected
		if (anim > 0.5f)
		{
			float check_alpha = (anim - 0.5f) * 2.0f;
			ImU32 check_color = (i == 6) ? IM_COL32(50, 50, 50, (int)(255 * check_alpha)) : IM_COL32(255, 255, 255, (int)(255 * check_alpha));
			float cs = 6 * scale;
			dl->AddLine(ImVec2(swatch_center.x - cs, swatch_center.y), ImVec2(swatch_center.x - cs * 0.3f, swatch_center.y + cs * 0.6f), check_color, 2.0f * scale);
			dl->AddLine(ImVec2(swatch_center.x - cs * 0.3f, swatch_center.y + cs * 0.6f), ImVec2(swatch_center.x + cs, swatch_center.y - cs * 0.4f), check_color, 2.0f * scale);
		}

		ImGui::PopID();
	}

	// Display selected color
	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + swatch_size + 16 * scale));
	ImGui::Text("Selected:");
	ImGui::SameLine();
	ImVec2 preview_pos = ImGui::GetCursorScreenPos();
	dl->AddRectFilled(preview_pos, ImVec2(preview_pos.x + 60 * scale, preview_pos.y + 20 * scale), colors[selected_color], 4.0f);
	ImGui::Dummy(ImVec2(60 * scale, 20 * scale));
}

// ============================================================
// 39. Timeline Scrubber
// Video/animation timeline with animated playhead
// ============================================================
static void ShowUsecase_TimelineScrubber()
{
	ImGui::TextWrapped("Animation timeline with draggable playhead and keyframes:");

	static float playhead_pos = 0.3f;
	static bool is_playing = false;
	static float play_time = 0.0f;

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	// Play/Pause button
	if (ImGui::Button(is_playing ? "Pause##Timeline" : "Play##Timeline"))
		is_playing = !is_playing;
	ImGui::SameLine();
	if (ImGui::Button("Reset##Timeline"))
	{
		playhead_pos = 0.0f;
		play_time = 0.0f;
	}

	// Auto-play
	if (is_playing)
	{
		play_time += dt * 0.3f;
		playhead_pos = ImFmod(play_time, 1.0f);
	}

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 140);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	// Timeline track
	float track_margin = 40 * scale;
	float track_width = canvas_size.x - track_margin * 2;
	float track_y = pos.y + 30 * scale;
	float track_height = 8 * scale;

	// Track background
	dl->AddRectFilled(
		ImVec2(pos.x + track_margin, track_y),
		ImVec2(pos.x + track_margin + track_width, track_y + track_height),
		IM_COL32(50, 55, 65, 255), track_height * 0.5f);

	// Filled portion
	float fill_width = playhead_pos * track_width;
	dl->AddRectFilled(
		ImVec2(pos.x + track_margin, track_y),
		ImVec2(pos.x + track_margin + fill_width, track_y + track_height),
		IM_COL32(91, 194, 231, 200), track_height * 0.5f);

	// Keyframe markers
	float keyframes[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
	for (int i = 0; i < 5; i++)
	{
		float kf_x = pos.x + track_margin + keyframes[i] * track_width;
		float kf_y = track_y + track_height * 0.5f;

		// Diamond shape for keyframe
		float kf_size = 6 * scale;
		ImVec2 diamond[4] = {
			ImVec2(kf_x, kf_y - kf_size),
			ImVec2(kf_x + kf_size, kf_y),
			ImVec2(kf_x, kf_y + kf_size),
			ImVec2(kf_x - kf_size, kf_y)
		};
		bool is_past = keyframes[i] <= playhead_pos;
		dl->AddConvexPolyFilled(diamond, 4, is_past ? IM_COL32(255, 200, 50, 255) : IM_COL32(80, 85, 100, 255));
	}

	// Playhead
	float playhead_x = pos.x + track_margin + playhead_pos * track_width;

	// Dragging
	ImGui::SetCursorScreenPos(ImVec2(pos.x + track_margin, track_y - 10 * scale));
	ImGui::InvisibleButton("##timeline", ImVec2(track_width, track_height + 20 * scale));
	if (ImGui::IsItemActive())
	{
		float mouse_x = ImGui::GetIO().MousePos.x;
		playhead_pos = ImClamp((mouse_x - pos.x - track_margin) / track_width, 0.0f, 1.0f);
		is_playing = false;
	}
	bool hovered = ImGui::IsItemHovered() || ImGui::IsItemActive();

	// Animate playhead scale
	ImGuiID id = ImGui::GetID("timeline");
	float target_scale = hovered ? 1.3f : 1.0f;
	float head_scale = iam_tween_float(id, ImHashStr("scale"), target_scale, 0.15f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	// Playhead line
	dl->AddLine(ImVec2(playhead_x, pos.y + 20 * scale), ImVec2(playhead_x, track_y + track_height + 10 * scale),
		IM_COL32(231, 76, 60, 255), 2 * scale);

	// Playhead handle (triangle)
	float head_size = 8 * scale * head_scale;
	ImVec2 head_tri[3] = {
		ImVec2(playhead_x - head_size, pos.y + 20 * scale),
		ImVec2(playhead_x + head_size, pos.y + 20 * scale),
		ImVec2(playhead_x, pos.y + 20 * scale + head_size * 1.2f)
	};
	dl->AddTriangleFilled(head_tri[0], head_tri[1], head_tri[2], IM_COL32(231, 76, 60, 255));

	// Time display
	int frame = (int)(playhead_pos * 100);
	char time_str[32];
	snprintf(time_str, sizeof(time_str), "Frame: %d", frame);
	ImVec2 time_size = ImGui::CalcTextSize(time_str);
	dl->AddText(ImVec2(pos.x + canvas_size.x - time_size.x - 10 * scale, pos.y + 8 * scale),
		IM_COL32(180, 180, 190, 255), time_str);

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 40. Keyframe Curve Editor
// Animation curve with editable keyframe points
// ============================================================
static void ShowUsecase_KeyframeCurve()
{
	ImGui::TextWrapped("Animation curve editor with draggable keyframe points:");

	static ImVec2 keyframe_values[4] = {
		ImVec2(0.0f, 0.2f),
		ImVec2(0.33f, 0.8f),
		ImVec2(0.66f, 0.3f),
		ImVec2(1.0f, 0.9f)
	};
	static int selected_kf = -1;
	static int dragging_kf = -1;

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 200);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	// Grid
	float margin = 30 * scale;
	float graph_width = canvas_size.x - margin * 2;
	float graph_height = canvas_size.y - margin * 2;
	ImVec2 graph_min(pos.x + margin, pos.y + margin);
	ImVec2 graph_max(pos.x + margin + graph_width, pos.y + margin + graph_height);

	// Grid lines
	for (int i = 0; i <= 4; i++)
	{
		float t = i / 4.0f;
		// Vertical
		float x = graph_min.x + t * graph_width;
		dl->AddLine(ImVec2(x, graph_min.y), ImVec2(x, graph_max.y), IM_COL32(50, 55, 65, 255));
		// Horizontal
		float y = graph_min.y + t * graph_height;
		dl->AddLine(ImVec2(graph_min.x, y), ImVec2(graph_max.x, y), IM_COL32(50, 55, 65, 255));
	}

	// Draw curve (Catmull-Rom spline approximation using line segments)
	ImVec2 prev_pt;
	for (int s = 0; s <= 50; s++)
	{
		float t = s / 50.0f;

		// Find which segment we're in
		int seg = 0;
		for (int i = 0; i < 3; i++)
		{
			if (t >= keyframe_values[i].x && t <= keyframe_values[i + 1].x)
			{
				seg = i;
				break;
			}
		}

		// Lerp within segment
		float seg_t = (t - keyframe_values[seg].x) / (keyframe_values[seg + 1].x - keyframe_values[seg].x);
		seg_t = ImClamp(seg_t, 0.0f, 1.0f);

		// Smooth step for nice curve
		float smooth_t = seg_t * seg_t * (3.0f - 2.0f * seg_t);
		float value = keyframe_values[seg].y + (keyframe_values[seg + 1].y - keyframe_values[seg].y) * smooth_t;

		ImVec2 pt(graph_min.x + t * graph_width, graph_max.y - value * graph_height);

		if (s > 0)
			dl->AddLine(prev_pt, pt, IM_COL32(91, 194, 231, 255), 2.0f * scale);
		prev_pt = pt;
	}

	// Draw and handle keyframe points
	for (int i = 0; i < 4; i++)
	{
		ImGui::PushID(i);

		float kf_x = graph_min.x + keyframe_values[i].x * graph_width;
		float kf_y = graph_max.y - keyframe_values[i].y * graph_height;
		float kf_radius = 8 * scale;

		// Hit test
		ImGui::SetCursorScreenPos(ImVec2(kf_x - kf_radius, kf_y - kf_radius));
		ImGui::InvisibleButton("##kf", ImVec2(kf_radius * 2, kf_radius * 2));

		if (ImGui::IsItemClicked())
			selected_kf = i;

		if (ImGui::IsItemActive() && i > 0 && i < 3)  // Don't drag first/last
		{
			dragging_kf = i;
			float new_x = (ImGui::GetIO().MousePos.x - graph_min.x) / graph_width;
			float new_y = 1.0f - (ImGui::GetIO().MousePos.y - graph_min.y) / graph_height;
			keyframe_values[i].x = ImClamp(new_x, keyframe_values[i - 1].x + 0.05f, keyframe_values[i + 1].x - 0.05f);
			keyframe_values[i].y = ImClamp(new_y, 0.0f, 1.0f);
		}
		else if (dragging_kf == i)
			dragging_kf = -1;

		bool hovered = ImGui::IsItemHovered() || (selected_kf == i);

		// Animate selection
		ImGuiID id = ImGui::GetID("kf");
		float target = hovered ? 1.0f : 0.0f;
		float anim = iam_tween_float(id, ImHashStr("sel"), target, 0.15f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		// Draw keyframe
		float draw_radius = kf_radius * (1.0f + anim * 0.3f);
		dl->AddCircleFilled(ImVec2(kf_x, kf_y), draw_radius, IM_COL32(255, 200, 50, 255));
		if (anim > 0.01f)
			dl->AddCircle(ImVec2(kf_x, kf_y), draw_radius + 3 * scale * anim, IM_COL32(255, 200, 50, (int)(150 * anim)), 0, 2.0f * scale);

		ImGui::PopID();
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 41. Layer Stack
// Layer panel with visibility toggles and reorder animation
// ============================================================
static void ShowUsecase_LayerStack()
{
	ImGui::TextWrapped("Layer panel with animated visibility toggles:");

	static bool layer_visible[4] = { true, true, false, true };
	static float visibility_anims[4] = { 1.0f, 1.0f, 0.0f, 1.0f };

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	const char* layer_names[] = { "Background", "Characters", "Effects", "UI Overlay" };
	ImU32 layer_colors[] = {
		IM_COL32(70, 130, 180, 255),   // Steel blue
		IM_COL32(144, 238, 144, 255),  // Light green
		IM_COL32(255, 165, 0, 255),    // Orange
		IM_COL32(221, 160, 221, 255)   // Plum
	};

	ImVec2 pos = ImGui::GetCursorScreenPos();
	float layer_height = 36 * scale;
	float panel_width = 250 * scale;

	for (int i = 0; i < 4; i++)
	{
		ImGui::PushID(i);

		float y = pos.y + i * (layer_height + 4 * scale);

		// Layer background
		dl->AddRectFilled(ImVec2(pos.x, y), ImVec2(pos.x + panel_width, y + layer_height),
			IM_COL32(40, 44, 55, 255), 4.0f);

		// Color indicator
		dl->AddRectFilled(ImVec2(pos.x + 4 * scale, y + 4 * scale),
			ImVec2(pos.x + 12 * scale, y + layer_height - 4 * scale),
			layer_colors[i], 2.0f);

		// Eye icon button
		float eye_x = pos.x + 24 * scale;
		float eye_y = y + layer_height * 0.5f;
		float eye_size = 10 * scale;

		ImGui::SetCursorScreenPos(ImVec2(eye_x - eye_size, eye_y - eye_size));
		if (ImGui::InvisibleButton("##eye", ImVec2(eye_size * 2, eye_size * 2)))
		{
			layer_visible[i] = !layer_visible[i];
		}

		// Animate visibility
		float target = layer_visible[i] ? 1.0f : 0.0f;
		ImGuiID id = ImGui::GetID("vis");
		visibility_anims[i] = iam_tween_float(id, ImHashStr("vis"), target, 0.4f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		float vis = visibility_anims[i];

		// Draw eye icon
		// Eye shape (ellipse approximation)
		float eye_w = eye_size * 0.9f;
		float eye_h = eye_size * 0.5f;
		ImU32 eye_color = IM_COL32(180, 180, 190, (int)(100 + 155 * vis));

		// Outer eye shape
		dl->AddEllipse(ImVec2(eye_x, eye_y), ImVec2(eye_w, eye_h), eye_color, 0.0f, 0, 1.5f * scale);

		// Pupil (animated)
		float pupil_radius = eye_h * 0.6f * vis;
		if (pupil_radius > 0.5f)
			dl->AddCircleFilled(ImVec2(eye_x, eye_y), pupil_radius, eye_color);

		// Strike-through when hidden (animated)
		if (vis < 0.99f)
		{
			float strike_progress = 1.0f - vis;
			float strike_len = eye_size * 1.4f * strike_progress;
			dl->AddLine(
				ImVec2(eye_x - strike_len, eye_y + strike_len * 0.5f),
				ImVec2(eye_x + strike_len, eye_y - strike_len * 0.5f),
				IM_COL32(231, 76, 60, (int)(255 * strike_progress)), 2.0f * scale);
		}

		// Layer name (faded when hidden)
		int name_alpha = (int)(100 + 155 * vis);
		dl->AddText(ImVec2(pos.x + 48 * scale, y + (layer_height - ImGui::GetFontSize()) * 0.5f),
			IM_COL32(220, 220, 230, name_alpha), layer_names[i]);

		ImGui::PopID();
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 4 * (layer_height + 4 * scale) + 8));
	ImGui::Dummy(ImVec2(panel_width, 1));
}

// ============================================================
// 42. Node Connector
// Animated connection line between nodes (node-based editor)
// ============================================================
static void ShowUsecase_NodeConnector()
{
	ImGui::TextWrapped("Node connection with animated flow indicator:");

	static float flow_offset = 0.0f;
	flow_offset += ImGui::GetIO().DeltaTime * 50.0f;

	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 120);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(30, 32, 40, 255), 4.0f);

	// Node positions
	ImVec2 node1_pos(pos.x + 60 * scale, pos.y + canvas_size.y * 0.5f);
	ImVec2 node2_pos(pos.x + canvas_size.x - 60 * scale, pos.y + canvas_size.y * 0.5f);
	float node_radius = 24 * scale;

	// Draw nodes
	auto DrawNode = [&](ImVec2 center, const char* label, ImU32 color) {
		dl->AddCircleFilled(center, node_radius, color);
		dl->AddCircle(center, node_radius, IM_COL32(255, 255, 255, 50), 0, 2 * scale);
		ImVec2 text_size = ImGui::CalcTextSize(label);
		dl->AddText(ImVec2(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f),
			IM_COL32(255, 255, 255, 255), label);
	};

	DrawNode(node1_pos, "IN", IM_COL32(76, 175, 80, 255));
	DrawNode(node2_pos, "OUT", IM_COL32(33, 150, 243, 255));

	// Connection points
	ImVec2 out_pt(node1_pos.x + node_radius, node1_pos.y);
	ImVec2 in_pt(node2_pos.x - node_radius, node2_pos.y);

	// Bezier curve control points
	float ctrl_dist = (in_pt.x - out_pt.x) * 0.5f;
	ImVec2 ctrl1(out_pt.x + ctrl_dist, out_pt.y);
	ImVec2 ctrl2(in_pt.x - ctrl_dist, in_pt.y);

	// Draw base connection line
	dl->AddBezierCubic(out_pt, ctrl1, ctrl2, in_pt, IM_COL32(100, 100, 120, 255), 3 * scale);

	// Animated flow dots
	int dot_count = 5;
	float dot_spacing = 1.0f / dot_count;

	for (int i = 0; i < dot_count; i++)
	{
		float t = ImFmod((float)i * dot_spacing + flow_offset * 0.01f, 1.0f);

		// Cubic bezier evaluation
		float u = 1.0f - t;
		float tt = t * t;
		float uu = u * u;
		float uuu = uu * u;
		float ttt = tt * t;

		ImVec2 pt;
		pt.x = uuu * out_pt.x + 3 * uu * t * ctrl1.x + 3 * u * tt * ctrl2.x + ttt * in_pt.x;
		pt.y = uuu * out_pt.y + 3 * uu * t * ctrl1.y + 3 * u * tt * ctrl2.y + ttt * in_pt.y;

		float dot_alpha = ImSin(t * IM_PI);  // Fade at ends
		dl->AddCircleFilled(pt, 4 * scale, IM_COL32(91, 194, 231, (int)(255 * dot_alpha)));
	}

	// Connection point indicators
	dl->AddCircleFilled(out_pt, 6 * scale, IM_COL32(255, 200, 50, 255));
	dl->AddCircleFilled(in_pt, 6 * scale, IM_COL32(255, 200, 50, 255));

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 44. Animated Bar Chart
// Bar chart with animated height transitions
// ============================================================
static void ShowUsecase_AnimatedBarChart()
{
	ImGui::TextWrapped("Bar chart with animated data transitions:");

	static float bar_values[6] = { 0.6f, 0.8f, 0.45f, 0.9f, 0.55f, 0.75f };
	static float target_values[6] = { 0.6f, 0.8f, 0.45f, 0.9f, 0.55f, 0.75f };

	if (ImGui::Button("Randomize Data##BarChart"))
	{
		for (int i = 0; i < 6; i++)
			target_values[i] = 0.2f + (float)(rand() % 80) / 100.0f;
	}

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 350);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	const char* labels[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun" };
	ImU32 bar_colors[] = {
		IM_COL32(91, 194, 231, 255),
		IM_COL32(76, 175, 80, 255),
		IM_COL32(255, 152, 0, 255),
		IM_COL32(233, 30, 99, 255),
		IM_COL32(156, 39, 176, 255),
		IM_COL32(0, 188, 212, 255)
	};

	float margin = 30 * scale;
	float chart_width = canvas_size.x - margin * 2;
	float chart_height = canvas_size.y - margin * 2 - 20 * scale;
	float bar_width = chart_width / 6 - 10 * scale;

	for (int i = 0; i < 6; i++)
	{
		// Animate bar height
		ImGuiID id = ImGui::GetID("barchart") + i;
		bar_values[i] = iam_tween_float(id, ImHashStr("height"), target_values[i], 0.5f,
			iam_ease_preset(iam_ease_out_elastic), iam_policy_crossfade, dt);

		float bar_x = pos.x + margin + i * (chart_width / 6) + 5 * scale;
		float bar_height = bar_values[i] * chart_height;
		float bar_y = pos.y + margin + chart_height - bar_height;

		// Bar
		dl->AddRectFilled(
			ImVec2(bar_x, bar_y),
			ImVec2(bar_x + bar_width, pos.y + margin + chart_height),
			bar_colors[i], 4.0f);

		// Value label
		char val_str[16];
		snprintf(val_str, sizeof(val_str), "%.0f%%", bar_values[i] * 100);
		ImVec2 val_size = ImGui::CalcTextSize(val_str);
		dl->AddText(ImVec2(bar_x + (bar_width - val_size.x) * 0.5f, bar_y - val_size.y - 4 * scale),
			IM_COL32(200, 200, 210, 255), val_str);

		// Month label
		ImVec2 label_size = ImGui::CalcTextSize(labels[i]);
		dl->AddText(ImVec2(bar_x + (bar_width - label_size.x) * 0.5f, pos.y + canvas_size.y - margin - 10 * scale),
			IM_COL32(150, 150, 160, 255), labels[i]);
	}

	// Axis line
	dl->AddLine(
		ImVec2(pos.x + margin - 5 * scale, pos.y + margin + chart_height),
		ImVec2(pos.x + canvas_size.x - margin + 5 * scale, pos.y + margin + chart_height),
		IM_COL32(60, 65, 80, 255), 2 * scale);

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 45. Animated Line Chart
// Real-time line chart with smooth data updates
// ============================================================
static void ShowUsecase_AnimatedLineChart()
{
	ImGui::TextWrapped("Line chart with animated real-time data:");

	static float data_points[20];
	static float target_points[20];
	static bool initialized = false;
	static float update_timer = 0.0f;

	if (!initialized)
	{
		for (int i = 0; i < 20; i++)
		{
			data_points[i] = 0.3f + (float)(i % 5) * 0.1f + ImSin(i * 0.5f) * 0.2f;
			target_points[i] = data_points[i];
		}
		initialized = true;
	}

	float dt = ImGui::GetIO().DeltaTime;

	// Auto-update data periodically
	update_timer += dt;
	if (update_timer > 1.5f)
	{
		update_timer = 0.0f;
		// Shift data left and add new point
		for (int i = 0; i < 19; i++)
			target_points[i] = target_points[i + 1];
		target_points[19] = 0.2f + (float)(rand() % 60) / 100.0f;
	}

	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 140);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	float margin = 20 * scale;
	float chart_width = canvas_size.x - margin * 2;
	float chart_height = canvas_size.y - margin * 2;

	// Grid lines
	for (int i = 0; i <= 4; i++)
	{
		float y = pos.y + margin + (i / 4.0f) * chart_height;
		dl->AddLine(ImVec2(pos.x + margin, y), ImVec2(pos.x + canvas_size.x - margin, y),
			IM_COL32(45, 50, 60, 255));
	}

	// Animate and draw line only (no dots, no fill)
	ImVec2 prev_pt;
	for (int i = 0; i < 20; i++)
	{
		// Animate each point
		ImGuiID id = ImGui::GetID("linechart") + i;
		data_points[i] = iam_tween_float(id, ImHashStr("val"), target_points[i], 0.4f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		float x = pos.x + margin + (i / 19.0f) * chart_width;
		float y = pos.y + margin + (1.0f - data_points[i]) * chart_height;
		ImVec2 pt(x, y);

		if (i > 0)
		{
			// Line segment only
			dl->AddLine(prev_pt, pt, IM_COL32(91, 194, 231, 255), 2 * scale);
		}

		prev_pt = pt;
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 46. Animated Pie Chart
// Donut chart with animated segments
// ============================================================
static void ShowUsecase_AnimatedPieChart()
{
	ImGui::TextWrapped("Donut chart with animated segment transitions:");

	static float segment_values[5] = { 0.3f, 0.25f, 0.2f, 0.15f, 0.1f };
	static float target_values[5] = { 0.3f, 0.25f, 0.2f, 0.15f, 0.1f };
	static int hovered_segment = -1;

	if (ImGui::Button("Shuffle##PieChart"))
	{
		float total = 0.0f;
		for (int i = 0; i < 5; i++)
		{
			target_values[i] = 0.1f + (float)(rand() % 40) / 100.0f;
			total += target_values[i];
		}
		// Normalize
		for (int i = 0; i < 5; i++)
			target_values[i] /= total;
	}

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 320);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	const char* labels[] = { "Product A", "Product B", "Product C", "Product D", "Other" };
	ImU32 colors[] = {
		IM_COL32(91, 194, 231, 255),
		IM_COL32(76, 175, 80, 255),
		IM_COL32(255, 152, 0, 255),
		IM_COL32(233, 30, 99, 255),
		IM_COL32(156, 39, 176, 255)
	};

	ImVec2 center(pos.x + 110 * scale, pos.y + canvas_size.y * 0.5f);
	float outer_radius = 70 * scale;
	float inner_radius = 40 * scale;

	// Check hover using angular detection
	ImVec2 mouse = ImGui::GetIO().MousePos;
	float mouse_dx = mouse.x - center.x;
	float mouse_dy = mouse.y - center.y;
	float mouse_dist = sqrtf(mouse_dx * mouse_dx + mouse_dy * mouse_dy);
	float mouse_angle = atan2f(mouse_dy, mouse_dx);
	// Convert to 0..2PI range starting from top (-PI/2)
	mouse_angle += IM_PI * 0.5f;  // Shift so top is 0
	if (mouse_angle < 0) mouse_angle += IM_PI * 2;
	if (mouse_angle >= IM_PI * 2) mouse_angle -= IM_PI * 2;

	hovered_segment = -1;

	// Animate and draw segments
	float start_angle = -IM_PI * 0.5f;  // Start from top
	float cumulative_angle = 0.0f;  // Track cumulative angle for hover check

	for (int i = 0; i < 5; i++)
	{
		// Animate value
		ImGuiID id = ImGui::GetID("pie") + i;
		segment_values[i] = iam_tween_float(id, ImHashStr("val"), target_values[i], 0.5f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		float sweep = segment_values[i] * IM_PI * 2;
		float end_angle = start_angle + sweep;

		// Angular hover detection (using cumulative angle from top)
		float seg_start = cumulative_angle;
		float seg_end = cumulative_angle + sweep;

		if (mouse_dist >= inner_radius && mouse_dist <= outer_radius * 1.2f)
		{
			// Check if mouse angle falls within this segment
			if (mouse_angle >= seg_start && mouse_angle < seg_end)
				hovered_segment = i;
			// Handle wrap-around for last segment
			else if (seg_end > IM_PI * 2 && mouse_angle < seg_end - IM_PI * 2)
				hovered_segment = i;
		}

		cumulative_angle += sweep;

		// Animate hover expansion
		float hover_target = (hovered_segment == i) ? 1.0f : 0.0f;
		float hover_anim = iam_tween_float(id, ImHashStr("hover"), hover_target, 0.15f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		float expand = hover_anim * 8 * scale;

		// Draw arc segment
		int seg_count = (int)(sweep / 0.1f) + 1;
		for (int s = 0; s < seg_count; s++)
		{
			float t1 = (float)s / seg_count;
			float t2 = (float)(s + 1) / seg_count;
			float a1 = start_angle + sweep * t1;
			float a2 = start_angle + sweep * t2;

			ImVec2 p1(center.x + ImCos(a1) * (inner_radius + expand), center.y + ImSin(a1) * (inner_radius + expand));
			ImVec2 p2(center.x + ImCos(a1) * (outer_radius + expand), center.y + ImSin(a1) * (outer_radius + expand));
			ImVec2 p3(center.x + ImCos(a2) * (outer_radius + expand), center.y + ImSin(a2) * (outer_radius + expand));
			ImVec2 p4(center.x + ImCos(a2) * (inner_radius + expand), center.y + ImSin(a2) * (inner_radius + expand));

			dl->AddQuadFilled(p1, p2, p3, p4, colors[i]);
		}

		start_angle = end_angle;
	}

	// Legend (positioned higher to avoid overlap)
	float legend_x = pos.x + 190 * scale;
	for (int i = 0; i < 5; i++)
	{
		float y = pos.y + 20 * scale + i * 24 * scale;

		// Color box
		dl->AddRectFilled(ImVec2(legend_x, y), ImVec2(legend_x + 16 * scale, y + 16 * scale), colors[i], 2.0f);

		// Label and percentage
		char label_str[64];
		snprintf(label_str, sizeof(label_str), "%s (%.0f%%)", labels[i], segment_values[i] * 100);

		int alpha = (hovered_segment == i || hovered_segment == -1) ? 255 : 120;
		dl->AddText(ImVec2(legend_x + 24 * scale, y), IM_COL32(200, 200, 210, alpha), label_str);
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 47. Heat Map
// Animated heat map grid visualization
// ============================================================
static void ShowUsecase_HeatMap()
{
	ImGui::TextWrapped("Heat map grid with animated color transitions:");

	static float cell_values[8][6];
	static float target_values[8][6];
	static bool initialized = false;

	if (!initialized)
	{
		for (int y = 0; y < 6; y++)
			for (int x = 0; x < 8; x++)
			{
				cell_values[x][y] = (float)(rand() % 100) / 100.0f;
				target_values[x][y] = cell_values[x][y];
			}
		initialized = true;
	}

	if (ImGui::Button("Regenerate##HeatMap"))
	{
		for (int y = 0; y < 6; y++)
			for (int x = 0; x < 8; x++)
				target_values[x][y] = (float)(rand() % 100) / 100.0f;
	}

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 340);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	float margin = 20 * scale;
	float label_margin = 55 * scale;  // More space for row labels
	float cell_width = (canvas_size.x - margin * 2 - label_margin) / 8;
	float cell_height = (canvas_size.y - margin * 2 - 35 * scale) / 6;

	const char* col_labels[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Avg" };
	const char* row_labels[] = { "00-04", "04-08", "08-12", "12-16", "16-20", "20-24" };

	// Row labels
	for (int y = 0; y < 6; y++)
	{
		float cy = pos.y + margin + y * cell_height + cell_height * 0.5f;
		ImVec2 label_size = ImGui::CalcTextSize(row_labels[y]);
		dl->AddText(ImVec2(pos.x + 4 * scale, cy - label_size.y * 0.5f),
			IM_COL32(120, 120, 130, 255), row_labels[y]);
	}

	// Column labels
	for (int x = 0; x < 8; x++)
	{
		float cx = pos.x + margin + label_margin + x * cell_width + cell_width * 0.5f;
		ImVec2 label_size = ImGui::CalcTextSize(col_labels[x]);
		dl->AddText(ImVec2(cx - label_size.x * 0.5f, pos.y + canvas_size.y - margin - 5 * scale),
			IM_COL32(120, 120, 130, 255), col_labels[x]);
	}

	// Draw cells
	for (int y = 0; y < 6; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			// Animate value
			ImGuiID id = ImGui::GetID("heatmap") + y * 8 + x;
			cell_values[x][y] = iam_tween_float(id, ImHashStr("val"), target_values[x][y], 0.6f,
				iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

			float val = cell_values[x][y];

			// Color gradient: blue -> green -> yellow -> red
			int r, g, b;
			if (val < 0.33f)
			{
				float t = val / 0.33f;
				r = (int)(50 + t * 50);
				g = (int)(100 + t * 155);
				b = (int)(200 - t * 100);
			}
			else if (val < 0.66f)
			{
				float t = (val - 0.33f) / 0.33f;
				r = (int)(100 + t * 155);
				g = (int)(255 - t * 55);
				b = (int)(100 - t * 50);
			}
			else
			{
				float t = (val - 0.66f) / 0.34f;
				r = 255;
				g = (int)(200 - t * 150);
				b = (int)(50 - t * 50);
			}

			float cx = pos.x + margin + label_margin + x * cell_width;
			float cy = pos.y + margin + y * cell_height;

			dl->AddRectFilled(
				ImVec2(cx + 2, cy + 2),
				ImVec2(cx + cell_width - 2, cy + cell_height - 2),
				IM_COL32(r, g, b, 255), 4.0f);
		}
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 48. Sparkline
// Mini inline chart for data trends
// ============================================================
static void ShowUsecase_Sparkline()
{
	ImGui::TextWrapped("Compact sparkline charts for inline data visualization:");

	static float sparkline_data[3][12];
	static float target_data[3][12];
	static bool initialized = false;

	if (!initialized)
	{
		for (int s = 0; s < 3; s++)
			for (int i = 0; i < 12; i++)
			{
				sparkline_data[s][i] = 0.3f + (float)(rand() % 50) / 100.0f;
				target_data[s][i] = sparkline_data[s][i];
			}
		initialized = true;
	}

	if (ImGui::Button("Update Trends##Sparkline"))
	{
		for (int s = 0; s < 3; s++)
			for (int i = 0; i < 12; i++)
				target_data[s][i] = 0.2f + (float)(rand() % 60) / 100.0f;
	}

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	float row_height = 50 * scale;

	const char* metric_names[] = { "Revenue", "Users", "Conversion" };
	const char* metric_values[] = { "$12.4K", "8,542", "3.2%" };
	const char* metric_changes[] = { "+12%", "+8%", "-2%" };
	ImU32 trend_colors[] = {
		IM_COL32(76, 175, 80, 255),   // Green (up)
		IM_COL32(76, 175, 80, 255),   // Green (up)
		IM_COL32(231, 76, 60, 255)    // Red (down)
	};

	for (int s = 0; s < 3; s++)
	{
		float y = pos.y + s * row_height;

		// Background
		dl->AddRectFilled(
			ImVec2(pos.x, y),
			ImVec2(pos.x + ImGui::GetContentRegionAvail().x, y + row_height - 4 * scale),
			IM_COL32(35, 40, 50, 255), 4.0f);

		// Metric name
		dl->AddText(ImVec2(pos.x + 12 * scale, y + 8 * scale),
			IM_COL32(150, 150, 160, 255), metric_names[s]);

		// Metric value
		dl->AddText(ImVec2(pos.x + 12 * scale, y + 24 * scale),
			IM_COL32(220, 220, 230, 255), metric_values[s]);

		// Change indicator
		dl->AddText(ImVec2(pos.x + 100 * scale, y + 24 * scale),
			trend_colors[s], metric_changes[s]);

		// Sparkline
		float spark_x = pos.x + 160 * scale;
		float spark_width = ImGui::GetContentRegionAvail().x - 180 * scale;
		float spark_height = 30 * scale;
		float spark_y = y + 8 * scale;

		ImVec2 prev_pt;
		for (int i = 0; i < 12; i++)
		{
			// Animate
			ImGuiID id = ImGui::GetID("spark") + s * 12 + i;
			sparkline_data[s][i] = iam_tween_float(id, ImHashStr("val"), target_data[s][i], 0.4f,
				iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

			float px = spark_x + (i / 11.0f) * spark_width;
			float py = spark_y + spark_height - sparkline_data[s][i] * spark_height;
			ImVec2 pt(px, py);

			if (i > 0)
				dl->AddLine(prev_pt, pt, trend_colors[s], 2 * scale);

			prev_pt = pt;
		}

		// End dot
		dl->AddCircleFilled(prev_pt, 4 * scale, trend_colors[s]);
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 3 * row_height + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 48. Radar Chart
// Animated radar/spider chart for multi-dimensional data
// ============================================================
static void ShowUsecase_RadarChart()
{
	ImGui::TextWrapped("Radar chart with animated data comparison:");

	static float data_a[6] = { 0.8f, 0.6f, 0.9f, 0.5f, 0.7f, 0.85f };
	static float data_b[6] = { 0.5f, 0.9f, 0.4f, 0.8f, 0.6f, 0.55f };
	static float target_a[6] = { 0.8f, 0.6f, 0.9f, 0.5f, 0.7f, 0.85f };
	static float target_b[6] = { 0.5f, 0.9f, 0.4f, 0.8f, 0.6f, 0.55f };

	if (ImGui::Button("Randomize##RadarChart"))
	{
		for (int i = 0; i < 6; i++)
		{
			target_a[i] = 0.3f + (float)(rand() % 70) / 100.0f;
			target_b[i] = 0.3f + (float)(rand() % 70) / 100.0f;
		}
	}

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 300);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	const char* axis_labels[] = { "Speed", "Power", "Defense", "Magic", "Stamina", "Luck" };
	ImVec2 center(pos.x + canvas_size.x * 0.35f, pos.y + canvas_size.y * 0.5f);
	float max_radius = 75 * scale;

	// Draw grid circles
	for (int ring = 1; ring <= 4; ring++)
	{
		float r = max_radius * ring / 4.0f;
		dl->AddCircle(center, r, IM_COL32(50, 55, 65, 255), 0, 1.0f);
	}

	// Draw axis lines and labels
	for (int i = 0; i < 6; i++)
	{
		float angle = -IM_PI * 0.5f + (i / 6.0f) * IM_PI * 2;
		ImVec2 axis_end(center.x + ImCos(angle) * max_radius, center.y + ImSin(angle) * max_radius);
		dl->AddLine(center, axis_end, IM_COL32(60, 65, 80, 255));

		// Label
		ImVec2 label_pos(center.x + ImCos(angle) * (max_radius + 15 * scale),
			center.y + ImSin(angle) * (max_radius + 15 * scale));
		ImVec2 label_size = ImGui::CalcTextSize(axis_labels[i]);
		dl->AddText(ImVec2(label_pos.x - label_size.x * 0.5f, label_pos.y - label_size.y * 0.5f),
			IM_COL32(150, 150, 160, 255), axis_labels[i]);
	}

	// Animate and draw data polygons
	ImVec2 points_a[6], points_b[6];
	for (int i = 0; i < 6; i++)
	{
		// Animate values
		ImGuiID id = ImGui::GetID("radar");
		data_a[i] = iam_tween_float(id + i, ImHashStr("a"), target_a[i], 0.5f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);
		data_b[i] = iam_tween_float(id + i + 6, ImHashStr("b"), target_b[i], 0.5f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		float angle = -IM_PI * 0.5f + (i / 6.0f) * IM_PI * 2;
		points_a[i] = ImVec2(center.x + ImCos(angle) * max_radius * data_a[i],
			center.y + ImSin(angle) * max_radius * data_a[i]);
		points_b[i] = ImVec2(center.x + ImCos(angle) * max_radius * data_b[i],
			center.y + ImSin(angle) * max_radius * data_b[i]);
	}

	// Draw filled polygons using triangles from center (handles concave shapes)
	for (int i = 0; i < 6; i++)
	{
		int next = (i + 1) % 6;
		dl->AddTriangleFilled(center, points_a[i], points_a[next], IM_COL32(91, 194, 231, 80));
		dl->AddTriangleFilled(center, points_b[i], points_b[next], IM_COL32(233, 30, 99, 80));
	}

	// Draw polygon outlines
	for (int i = 0; i < 6; i++)
	{
		int next = (i + 1) % 6;
		dl->AddLine(points_a[i], points_a[next], IM_COL32(91, 194, 231, 255), 2 * scale);
		dl->AddLine(points_b[i], points_b[next], IM_COL32(233, 30, 99, 255), 2 * scale);
	}

	// Draw data points
	for (int i = 0; i < 6; i++)
	{
		dl->AddCircleFilled(points_a[i], 4 * scale, IM_COL32(91, 194, 231, 255));
		dl->AddCircleFilled(points_b[i], 4 * scale, IM_COL32(233, 30, 99, 255));
	}

	// Legend
	float legend_x = pos.x + canvas_size.x - 100 * scale;
	dl->AddRectFilled(ImVec2(legend_x, pos.y + 20 * scale), ImVec2(legend_x + 16 * scale, pos.y + 32 * scale),
		IM_COL32(91, 194, 231, 255), 2.0f);
	dl->AddText(ImVec2(legend_x + 24 * scale, pos.y + 20 * scale), IM_COL32(200, 200, 210, 255), "Player A");

	dl->AddRectFilled(ImVec2(legend_x, pos.y + 44 * scale), ImVec2(legend_x + 16 * scale, pos.y + 56 * scale),
		IM_COL32(233, 30, 99, 255), 2.0f);
	dl->AddText(ImVec2(legend_x + 24 * scale, pos.y + 44 * scale), IM_COL32(200, 200, 210, 255), "Player B");

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 49. Gauge Meter
// Animated gauge/speedometer visualization
// ============================================================
static void ShowUsecase_GaugeMeter()
{
	ImGui::TextWrapped("Gauge meter with animated needle and gradient arc:");

	static float gauge_value = 0.65f;
	static float target_value = 0.65f;

	ImGui::SliderFloat("Value", &target_value, 0.0f, 1.0f);

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 200);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	// Center positioned at bottom of canvas for upward-facing arc
	ImVec2 center(pos.x + canvas_size.x * 0.5f, pos.y + canvas_size.y - 15 * scale);
	float outer_radius = 65 * scale;
	float inner_radius = 45 * scale;

	// Animate value
	ImGuiID id = ImGui::GetID("gauge");
	gauge_value = iam_tween_float(id, ImHashStr("val"), target_value, 0.6f,
		iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

	// Draw arc segments with gradient (semicircle pointing UP)
	float start_angle = IM_PI;     // 180 degrees (left)
	float end_angle = IM_PI * 2;   // 360 degrees (right) - arc goes UP
	int segments = 50;

	for (int s = 0; s < segments; s++)
	{
		float t = (float)s / segments;
		float a1 = start_angle + (end_angle - start_angle) * t;
		float a2 = start_angle + (end_angle - start_angle) * (t + 1.0f / segments);

		// Color gradient: green -> yellow -> red
		int r, g, b;
		if (t < 0.5f)
		{
			float lt = t * 2;
			r = (int)(76 + (255 - 76) * lt);
			g = (int)(175 + (193 - 175) * lt);
			b = (int)(80 - 80 * lt);
		}
		else
		{
			float lt = (t - 0.5f) * 2;
			r = (int)(255);
			g = (int)(193 - 193 * lt);
			b = (int)(0);
		}

		ImVec2 p1(center.x + ImCos(a1) * inner_radius, center.y + ImSin(a1) * inner_radius);
		ImVec2 p2(center.x + ImCos(a1) * outer_radius, center.y + ImSin(a1) * outer_radius);
		ImVec2 p3(center.x + ImCos(a2) * outer_radius, center.y + ImSin(a2) * outer_radius);
		ImVec2 p4(center.x + ImCos(a2) * inner_radius, center.y + ImSin(a2) * inner_radius);

		dl->AddQuadFilled(p1, p2, p3, p4, IM_COL32(r, g, b, 255));
	}

	// Draw tick marks
	for (int i = 0; i <= 10; i++)
	{
		float t = i / 10.0f;
		float angle = start_angle + (end_angle - start_angle) * t;
		float tick_inner = outer_radius + 3 * scale;
		float tick_outer = outer_radius + (i % 5 == 0 ? 10 : 6) * scale;

		dl->AddLine(
			ImVec2(center.x + ImCos(angle) * tick_inner, center.y + ImSin(angle) * tick_inner),
			ImVec2(center.x + ImCos(angle) * tick_outer, center.y + ImSin(angle) * tick_outer),
			IM_COL32(180, 180, 190, 255), (i % 5 == 0 ? 2 : 1) * scale);

		// Labels for major ticks
		if (i % 5 == 0)
		{
			char label[8];
			snprintf(label, sizeof(label), "%d", i * 10);
			ImVec2 label_size = ImGui::CalcTextSize(label);
			float label_r = outer_radius + 18 * scale;
			dl->AddText(
				ImVec2(center.x + ImCos(angle) * label_r - label_size.x * 0.5f,
					center.y + ImSin(angle) * label_r - label_size.y * 0.5f),
				IM_COL32(150, 150, 160, 255), label);
		}
	}

	// Draw needle
	float needle_angle = start_angle + (end_angle - start_angle) * gauge_value;
	float needle_length = inner_radius - 5 * scale;

	ImVec2 needle_tip(center.x + ImCos(needle_angle) * needle_length,
		center.y + ImSin(needle_angle) * needle_length);
	ImVec2 needle_base1(center.x + ImCos(needle_angle + IM_PI * 0.5f) * 4 * scale,
		center.y + ImSin(needle_angle + IM_PI * 0.5f) * 4 * scale);
	ImVec2 needle_base2(center.x + ImCos(needle_angle - IM_PI * 0.5f) * 4 * scale,
		center.y + ImSin(needle_angle - IM_PI * 0.5f) * 4 * scale);

	dl->AddTriangleFilled(needle_tip, needle_base1, needle_base2, IM_COL32(220, 220, 230, 255));
	dl->AddCircleFilled(center, 8 * scale, IM_COL32(50, 55, 70, 255));
	dl->AddCircleFilled(center, 5 * scale, IM_COL32(220, 220, 230, 255));

	// Value display (above the gauge)
	char value_str[16];
	snprintf(value_str, sizeof(value_str), "%.0f%%", gauge_value * 100);
	ImVec2 value_size = ImGui::CalcTextSize(value_str);
	dl->AddText(ImVec2(center.x - value_size.x * 0.5f, pos.y + 20 * scale),
		IM_COL32(220, 220, 230, 255), value_str);

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 50. Sankey Flow
// Animated flow diagram showing data relationships
// ============================================================
static void ShowUsecase_SankeyFlow()
{
	ImGui::TextWrapped("Animated flow diagram with particle effects:");

	static float flow_time = 0.0f;
	flow_time += ImGui::GetIO().DeltaTime;

	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 320);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	// Source nodes (left)
	const char* sources[] = { "Direct", "Search", "Social" };
	ImU32 source_colors[] = {
		IM_COL32(91, 194, 231, 255),
		IM_COL32(76, 175, 80, 255),
		IM_COL32(255, 152, 0, 255)
	};
	float source_weights[] = { 0.4f, 0.35f, 0.25f };

	// Target nodes (right)
	const char* targets[] = { "Sales", "Signup", "Bounce" };
	ImU32 target_colors[] = {
		IM_COL32(156, 39, 176, 255),
		IM_COL32(0, 188, 212, 255),
		IM_COL32(233, 30, 99, 255)
	};

	float left_x = pos.x + 100 * scale;
	float right_x = pos.x + canvas_size.x - 100 * scale;
	float flow_height = canvas_size.y - 80 * scale;
	float node_width = 35 * scale;

	// Draw source nodes
	float y_offset = pos.y + 30 * scale;
	for (int i = 0; i < 3; i++)
	{
		float node_height = flow_height * source_weights[i];

		// Node rectangle
		dl->AddRectFilled(
			ImVec2(left_x, y_offset),
			ImVec2(left_x + node_width, y_offset + node_height),
			source_colors[i], 4.0f);

		// Label
		ImVec2 label_size = ImGui::CalcTextSize(sources[i]);
		dl->AddText(ImVec2(left_x - label_size.x - 8 * scale, y_offset + (node_height - label_size.y) * 0.5f),
			IM_COL32(180, 180, 190, 255), sources[i]);

		y_offset += node_height + 10 * scale;
	}

	// Draw target nodes
	float target_weights[] = { 0.45f, 0.30f, 0.25f };
	y_offset = pos.y + 30 * scale;
	for (int i = 0; i < 3; i++)
	{
		float node_height = flow_height * target_weights[i];

		// Node rectangle
		dl->AddRectFilled(
			ImVec2(right_x, y_offset),
			ImVec2(right_x + node_width, y_offset + node_height),
			target_colors[i], 4.0f);

		// Label
		dl->AddText(ImVec2(right_x + node_width + 8 * scale, y_offset + (node_height - ImGui::GetFontSize()) * 0.5f),
			IM_COL32(180, 180, 190, 255), targets[i]);

		y_offset += node_height + 10 * scale;
	}

	// Draw flow connections with bezier curves
	struct FlowConnection {
		int from, to;
		float weight;
	};
	FlowConnection flows[] = {
		{0, 0, 0.20f}, {0, 1, 0.15f}, {0, 2, 0.05f},
		{1, 0, 0.18f}, {1, 1, 0.10f}, {1, 2, 0.07f},
		{2, 0, 0.07f}, {2, 1, 0.05f}, {2, 2, 0.13f}
	};

	// Calculate node positions
	float source_y[3], source_heights[3];
	float target_y[3], target_heights[3];

	float sy = pos.y + 30 * scale;
	for (int i = 0; i < 3; i++)
	{
		source_y[i] = sy;
		source_heights[i] = flow_height * source_weights[i];
		sy += source_heights[i] + 10 * scale;
	}

	float ty = pos.y + 30 * scale;
	for (int i = 0; i < 3; i++)
	{
		target_y[i] = ty;
		target_heights[i] = flow_height * target_weights[i];
		ty += target_heights[i] + 10 * scale;
	}

	// Track used height on each node
	float source_used[3] = {0, 0, 0};
	float target_used[3] = {0, 0, 0};

	for (int f = 0; f < 9; f++)
	{
		int from = flows[f].from;
		int to = flows[f].to;
		float weight = flows[f].weight;

		float band_height = flow_height * weight;

		float from_y = source_y[from] + source_used[from];
		float to_y = target_y[to] + target_used[to];

		source_used[from] += band_height;
		target_used[to] += band_height;

		// Draw gradient band using bezier
		ImVec2 p1(left_x + node_width, from_y);
		ImVec2 p2(left_x + node_width, from_y + band_height);
		ImVec2 p3(right_x, to_y);
		ImVec2 p4(right_x, to_y + band_height);

		float ctrl_offset = (right_x - left_x - node_width) * 0.4f;

		// Draw filled bezier band
		int steps = 20;
		for (int s = 0; s < steps; s++)
		{
			float t1 = (float)s / steps;
			float t2 = (float)(s + 1) / steps;

			// Cubic bezier for top edge
			auto bezier = [](ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d, float t) {
				float u = 1 - t;
				return ImVec2(
					u*u*u*a.x + 3*u*u*t*b.x + 3*u*t*t*c.x + t*t*t*d.x,
					u*u*u*a.y + 3*u*u*t*b.y + 3*u*t*t*c.y + t*t*t*d.y);
			};

			ImVec2 ctrl1_top(p1.x + ctrl_offset, p1.y);
			ImVec2 ctrl2_top(p3.x - ctrl_offset, p3.y);
			ImVec2 ctrl1_bot(p2.x + ctrl_offset, p2.y);
			ImVec2 ctrl2_bot(p4.x - ctrl_offset, p4.y);

			ImVec2 top1 = bezier(p1, ctrl1_top, ctrl2_top, p3, t1);
			ImVec2 top2 = bezier(p1, ctrl1_top, ctrl2_top, p3, t2);
			ImVec2 bot1 = bezier(p2, ctrl1_bot, ctrl2_bot, p4, t1);
			ImVec2 bot2 = bezier(p2, ctrl1_bot, ctrl2_bot, p4, t2);

			// Blend color from source to target
			float blend = (t1 + t2) * 0.5f;
			ImU32 col1 = source_colors[from];
			ImU32 col2 = target_colors[to];
			int r = (int)((1 - blend) * ((col1 >> 0) & 0xFF) + blend * ((col2 >> 0) & 0xFF));
			int g = (int)((1 - blend) * ((col1 >> 8) & 0xFF) + blend * ((col2 >> 8) & 0xFF));
			int b = (int)((1 - blend) * ((col1 >> 16) & 0xFF) + blend * ((col2 >> 16) & 0xFF));

			dl->AddQuadFilled(top1, top2, bot2, bot1, IM_COL32(r, g, b, 120));
		}

		// Animated flow particles
		int particle_count = (int)(weight * 20);
		for (int p = 0; p < particle_count; p++)
		{
			float pt = ImFmod(flow_time * 0.3f + (float)p / particle_count + (float)f * 0.1f, 1.0f);

			auto bezier = [](ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d, float t) {
				float u = 1 - t;
				return ImVec2(
					u*u*u*a.x + 3*u*u*t*b.x + 3*u*t*t*c.x + t*t*t*d.x,
					u*u*u*a.y + 3*u*u*t*b.y + 3*u*t*t*c.y + t*t*t*d.y);
			};

			ImVec2 mid_top = ImVec2(p1.x, (p1.y + p2.y) * 0.5f);
			ImVec2 mid_bot = ImVec2(p3.x, (p3.y + p4.y) * 0.5f);
			ImVec2 ctrl1(mid_top.x + (right_x - left_x) * 0.4f, mid_top.y);
			ImVec2 ctrl2(mid_bot.x - (right_x - left_x) * 0.4f, mid_bot.y);

			ImVec2 particle_pos = bezier(mid_top, ctrl1, ctrl2, mid_bot, pt);

			float alpha = ImSin(pt * IM_PI);
			dl->AddCircleFilled(particle_pos, 3 * scale, IM_COL32(255, 255, 255, (int)(180 * alpha)));
		}
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 51. Funnel Chart
// Animated funnel chart showing conversion stages
// ============================================================
static void ShowUsecase_FunnelChart()
{
	ImGui::TextWrapped("Funnel chart with animated stage transitions:");

	static float stage_values[5] = { 1.0f, 0.75f, 0.5f, 0.35f, 0.2f };
	static float target_values[5] = { 1.0f, 0.75f, 0.5f, 0.35f, 0.2f };

	if (ImGui::Button("Randomize##FunnelChart"))
	{
		float prev = 1.0f;
		for (int i = 0; i < 5; i++)
		{
			target_values[i] = prev * (0.5f + (float)(rand() % 50) / 100.0f);
			prev = target_values[i];
		}
	}

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 320);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	const char* stages[] = { "Visitors", "Leads", "Qualified", "Proposal", "Closed" };
	ImU32 colors[] = {
		IM_COL32(91, 194, 231, 255),
		IM_COL32(76, 175, 80, 255),
		IM_COL32(255, 193, 7, 255),
		IM_COL32(255, 152, 0, 255),
		IM_COL32(244, 67, 54, 255)
	};

	float center_x = pos.x + canvas_size.x * 0.5f;
	float max_width = canvas_size.x * 0.7f;
	float stage_height = (canvas_size.y - 60 * scale) / 5;

	for (int i = 0; i < 5; i++)
	{
		// Animate value
		ImGuiID id = ImGui::GetID("funnel") + i;
		stage_values[i] = iam_tween_float(id, ImHashStr("val"), target_values[i], 0.6f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		float y = pos.y + 30 * scale + i * stage_height;
		float this_width = max_width * stage_values[i];
		float next_width = (i < 4) ? max_width * stage_values[i + 1] : this_width * 0.3f;

		// Trapezoid shape
		ImVec2 p1(center_x - this_width * 0.5f, y);
		ImVec2 p2(center_x + this_width * 0.5f, y);
		ImVec2 p3(center_x + next_width * 0.5f, y + stage_height - 2 * scale);
		ImVec2 p4(center_x - next_width * 0.5f, y + stage_height - 2 * scale);

		dl->AddQuadFilled(p1, p2, p3, p4, colors[i]);

		// Label and percentage
		char label_str[64];
		snprintf(label_str, sizeof(label_str), "%s (%.0f%%)", stages[i], stage_values[i] * 100);
		ImVec2 text_size = ImGui::CalcTextSize(label_str);
		dl->AddText(ImVec2(center_x - text_size.x * 0.5f, y + (stage_height - text_size.y) * 0.5f - 2 * scale),
			IM_COL32(255, 255, 255, 255), label_str);
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 52. Animated Scatter Plot
// Scatter plot with animated point transitions
// ============================================================
static void ShowUsecase_ScatterPlot()
{
	ImGui::TextWrapped("Scatter plot with animated data clustering:");

	static float points_x[30], points_y[30];
	static float target_x[30], target_y[30];
	static bool initialized = false;

	if (!initialized)
	{
		for (int i = 0; i < 30; i++)
		{
			points_x[i] = (float)(rand() % 100) / 100.0f;
			points_y[i] = (float)(rand() % 100) / 100.0f;
			target_x[i] = points_x[i];
			target_y[i] = points_y[i];
		}
		initialized = true;
	}

	if (ImGui::Button("Cluster A##ScatterPlot"))
	{
		for (int i = 0; i < 30; i++)
		{
			target_x[i] = 0.2f + (float)(rand() % 30) / 100.0f;
			target_y[i] = 0.7f + (float)(rand() % 25) / 100.0f;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Cluster B##ScatterPlot"))
	{
		for (int i = 0; i < 30; i++)
		{
			target_x[i] = 0.7f + (float)(rand() % 25) / 100.0f;
			target_y[i] = 0.3f + (float)(rand() % 30) / 100.0f;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Scatter##ScatterPlot"))
	{
		for (int i = 0; i < 30; i++)
		{
			target_x[i] = 0.1f + (float)(rand() % 80) / 100.0f;
			target_y[i] = 0.1f + (float)(rand() % 80) / 100.0f;
		}
	}

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 300);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	float margin = 40 * scale;
	float chart_width = canvas_size.x - margin * 2;
	float chart_height = canvas_size.y - margin * 2;

	// Grid
	for (int i = 0; i <= 4; i++)
	{
		float t = i / 4.0f;
		// Horizontal
		dl->AddLine(
			ImVec2(pos.x + margin, pos.y + margin + chart_height * t),
			ImVec2(pos.x + margin + chart_width, pos.y + margin + chart_height * t),
			IM_COL32(50, 55, 65, 255));
		// Vertical
		dl->AddLine(
			ImVec2(pos.x + margin + chart_width * t, pos.y + margin),
			ImVec2(pos.x + margin + chart_width * t, pos.y + margin + chart_height),
			IM_COL32(50, 55, 65, 255));
	}

	// Animate and draw points
	for (int i = 0; i < 30; i++)
	{
		ImGuiID id = ImGui::GetID("scatter") + i;
		points_x[i] = iam_tween_float(id, ImHashStr("x"), target_x[i], 0.5f + (i % 10) * 0.03f,
			iam_ease_preset(iam_ease_out_elastic), iam_policy_crossfade, dt);
		points_y[i] = iam_tween_float(id, ImHashStr("y"), target_y[i], 0.5f + (i % 10) * 0.03f,
			iam_ease_preset(iam_ease_out_elastic), iam_policy_crossfade, dt);

		float px = pos.x + margin + points_x[i] * chart_width;
		float py = pos.y + margin + (1.0f - points_y[i]) * chart_height;

		// Color based on position
		int r = (int)(100 + points_x[i] * 155);
		int g = (int)(100 + points_y[i] * 155);
		int b = 200;

		dl->AddCircleFilled(ImVec2(px, py), 6 * scale, IM_COL32(r, g, b, 220));
		dl->AddCircle(ImVec2(px, py), 6 * scale, IM_COL32(255, 255, 255, 80));
	}

	// Axis labels
	dl->AddText(ImVec2(pos.x + margin + chart_width * 0.5f - 20, pos.y + canvas_size.y - margin + 10 * scale),
		IM_COL32(150, 150, 160, 255), "X-Axis");

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 53. Progress Dashboard
// Multi-metric dashboard with animated progress rings
// ============================================================
static void ShowUsecase_ProgressDashboard()
{
	ImGui::TextWrapped("Dashboard with animated progress indicators:");

	static float metrics[4] = { 0.72f, 0.45f, 0.88f, 0.33f };
	static float targets[4] = { 0.72f, 0.45f, 0.88f, 0.33f };

	if (ImGui::Button("Update Metrics##Dashboard"))
	{
		for (int i = 0; i < 4; i++)
			targets[i] = 0.1f + (float)(rand() % 90) / 100.0f;
	}

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 200);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	const char* labels[] = { "CPU", "Memory", "Network", "Disk" };
	ImU32 colors[] = {
		IM_COL32(91, 194, 231, 255),
		IM_COL32(76, 175, 80, 255),
		IM_COL32(255, 152, 0, 255),
		IM_COL32(233, 30, 99, 255)
	};

	float ring_spacing = canvas_size.x / 4;
	float ring_radius = 28 * scale;
	float ring_thickness = 6 * scale;

	for (int i = 0; i < 4; i++)
	{
		// Animate value
		ImGuiID id = ImGui::GetID("dashboard") + i;
		metrics[i] = iam_tween_float(id, ImHashStr("val"), targets[i], 0.6f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		ImVec2 center(pos.x + ring_spacing * (i + 0.5f), pos.y + canvas_size.y * 0.45f);

		// Background ring
		dl->AddCircle(center, ring_radius, IM_COL32(50, 55, 70, 255), 0, ring_thickness);

		// Progress arc
		float start_angle = -IM_PI * 0.5f;
		float sweep = metrics[i] * IM_PI * 2;
		int segments = (int)(sweep / 0.1f) + 1;

		for (int s = 0; s < segments; s++)
		{
			float t1 = (float)s / segments;
			float t2 = (float)(s + 1) / segments;
			float a1 = start_angle + sweep * t1;
			float a2 = start_angle + sweep * t2;

			ImVec2 p1(center.x + ImCos(a1) * (ring_radius - ring_thickness * 0.5f),
				center.y + ImSin(a1) * (ring_radius - ring_thickness * 0.5f));
			ImVec2 p2(center.x + ImCos(a1) * (ring_radius + ring_thickness * 0.5f),
				center.y + ImSin(a1) * (ring_radius + ring_thickness * 0.5f));
			ImVec2 p3(center.x + ImCos(a2) * (ring_radius + ring_thickness * 0.5f),
				center.y + ImSin(a2) * (ring_radius + ring_thickness * 0.5f));
			ImVec2 p4(center.x + ImCos(a2) * (ring_radius - ring_thickness * 0.5f),
				center.y + ImSin(a2) * (ring_radius - ring_thickness * 0.5f));

			dl->AddQuadFilled(p1, p2, p3, p4, colors[i]);
		}

		// Value text
		char val_str[16];
		snprintf(val_str, sizeof(val_str), "%.0f%%", metrics[i] * 100);
		ImVec2 val_size = ImGui::CalcTextSize(val_str);
		dl->AddText(ImVec2(center.x - val_size.x * 0.5f, center.y - val_size.y * 0.5f),
			IM_COL32(220, 220, 230, 255), val_str);

		// Label
		ImVec2 label_size = ImGui::CalcTextSize(labels[i]);
		dl->AddText(ImVec2(center.x - label_size.x * 0.5f, center.y + ring_radius + 12 * scale),
			IM_COL32(150, 150, 160, 255), labels[i]);
	}

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// 54. Area Chart
// Stacked area chart with animated fill
// ============================================================
static void ShowUsecase_AreaChart()
{
	ImGui::TextWrapped("Stacked area chart with smooth transitions:");

	static float series_a[10] = { 0.2f, 0.3f, 0.25f, 0.4f, 0.35f, 0.5f, 0.45f, 0.55f, 0.5f, 0.6f };
	static float series_b[10] = { 0.15f, 0.2f, 0.25f, 0.2f, 0.3f, 0.25f, 0.35f, 0.3f, 0.4f, 0.35f };
	static float target_a[10], target_b[10];
	static bool init = false;

	if (!init)
	{
		for (int i = 0; i < 10; i++)
		{
			target_a[i] = series_a[i];
			target_b[i] = series_b[i];
		}
		init = true;
	}

	if (ImGui::Button("Update Data##AreaChart"))
	{
		for (int i = 0; i < 10; i++)
		{
			target_a[i] = 0.15f + (float)(rand() % 50) / 100.0f;
			target_b[i] = 0.1f + (float)(rand() % 35) / 100.0f;
		}
	}

	float dt = ImGui::GetIO().DeltaTime;
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float scale = ImGui::GetFontSize() / 13.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 280);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 4.0f);

	float margin = 30 * scale;
	float chart_width = canvas_size.x - margin * 2;
	float chart_height = canvas_size.y - margin * 2;
	float point_spacing = chart_width / 9;

	// Animate values
	for (int i = 0; i < 10; i++)
	{
		ImGuiID id = ImGui::GetID("area") + i;
		series_a[i] = iam_tween_float(id, ImHashStr("a"), target_a[i], 0.5f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);
		series_b[i] = iam_tween_float(id, ImHashStr("b"), target_b[i], 0.5f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);
	}

	// Draw stacked areas using vertical strips (handles non-convex shapes)
	float base_y = pos.y + margin + chart_height;

	// Area for series B (top layer - cyan)
	for (int i = 0; i < 9; i++)
	{
		float x1 = pos.x + margin + i * point_spacing;
		float x2 = pos.x + margin + (i + 1) * point_spacing;
		float y1_top = pos.y + margin + chart_height - (series_a[i] + series_b[i]) * chart_height;
		float y2_top = pos.y + margin + chart_height - (series_a[i + 1] + series_b[i + 1]) * chart_height;
		float y1_bot = pos.y + margin + chart_height - series_a[i] * chart_height;
		float y2_bot = pos.y + margin + chart_height - series_a[i + 1] * chart_height;

		dl->AddQuadFilled(ImVec2(x1, y1_top), ImVec2(x2, y2_top), ImVec2(x2, y2_bot), ImVec2(x1, y1_bot),
			IM_COL32(91, 194, 231, 100));
	}

	// Area for series A (bottom layer - green)
	for (int i = 0; i < 9; i++)
	{
		float x1 = pos.x + margin + i * point_spacing;
		float x2 = pos.x + margin + (i + 1) * point_spacing;
		float y1_top = pos.y + margin + chart_height - series_a[i] * chart_height;
		float y2_top = pos.y + margin + chart_height - series_a[i + 1] * chart_height;

		dl->AddQuadFilled(ImVec2(x1, y1_top), ImVec2(x2, y2_top), ImVec2(x2, base_y), ImVec2(x1, base_y),
			IM_COL32(76, 175, 80, 150));
	}

	// Draw line on top
	for (int i = 0; i < 9; i++)
	{
		float x1 = pos.x + margin + i * point_spacing;
		float x2 = pos.x + margin + (i + 1) * point_spacing;
		float y1a = pos.y + margin + chart_height - series_a[i] * chart_height;
		float y2a = pos.y + margin + chart_height - series_a[i + 1] * chart_height;
		float y1b = pos.y + margin + chart_height - (series_a[i] + series_b[i]) * chart_height;
		float y2b = pos.y + margin + chart_height - (series_a[i + 1] + series_b[i + 1]) * chart_height;

		dl->AddLine(ImVec2(x1, y1a), ImVec2(x2, y2a), IM_COL32(76, 175, 80, 255), 2 * scale);
		dl->AddLine(ImVec2(x1, y1b), ImVec2(x2, y2b), IM_COL32(91, 194, 231, 255), 2 * scale);
	}

	// Grid lines
	for (int i = 1; i <= 4; i++)
	{
		float y = pos.y + margin + chart_height * i / 5;
		dl->AddLine(ImVec2(pos.x + margin, y), ImVec2(pos.x + margin + chart_width, y),
			IM_COL32(50, 55, 65, 255));
	}

	// Legend
	dl->AddRectFilled(ImVec2(pos.x + canvas_size.x - 120 * scale, pos.y + 20 * scale),
		ImVec2(pos.x + canvas_size.x - 104 * scale, pos.y + 32 * scale),
		IM_COL32(91, 194, 231, 200), 2.0f);
	dl->AddText(ImVec2(pos.x + canvas_size.x - 98 * scale, pos.y + 18 * scale),
		IM_COL32(200, 200, 210, 255), "Series B");

	dl->AddRectFilled(ImVec2(pos.x + canvas_size.x - 120 * scale, pos.y + 38 * scale),
		ImVec2(pos.x + canvas_size.x - 104 * scale, pos.y + 50 * scale),
		IM_COL32(76, 175, 80, 200), 2.0f);
	dl->AddText(ImVec2(pos.x + canvas_size.x - 98 * scale, pos.y + 36 * scale),
		IM_COL32(200, 200, 210, 255), "Series A");

	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + canvas_size.y + 8));
	ImGui::Dummy(ImVec2(1, 1));
}

// ============================================================
// USECASE: Floating Action Button (FAB)
// ============================================================
static void ShowUsecase_FloatingActionButton()
{
	ImGui::TextWrapped(
		"Floating Action Button that expands to reveal multiple actions. "
		"Material Design style with rotation and staggered menu items.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static bool fab_open = false;
	static float fab_time = 0.0f;

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 180 * scale);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Background
	dl->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(25, 30, 40, 255), 8 * scale);

	// FAB position (bottom right of canvas)
	float fab_radius = 28 * scale;
	ImVec2 fab_center(canvas_pos.x + canvas_size.x - 50 * scale,
					  canvas_pos.y + canvas_size.y - 50 * scale);

	// Animate FAB rotation
	float rotation = iam_tween_float(ImGui::GetID("fab_rotation"), 0,
		fab_open ? 0.785f : 0.0f, 0.25f,  // 45 degrees
		iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

	float expand = iam_tween_float(ImGui::GetID("fab_expand"), 0,
		fab_open ? 1.0f : 0.0f, 0.3f,
		iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

	// Mini FABs (actions)
	struct FabAction {
		const char* icon;
		ImU32 color;
		float offset_y;
	};
	FabAction actions[] = {
		{ "C", IM_COL32(76, 175, 80, 255), 70 },   // Camera/Create
		{ "S", IM_COL32(33, 150, 243, 255), 130 }, // Share
		{ "E", IM_COL32(255, 152, 0, 255), 190 }   // Edit
	};

	if (fab_open) fab_time += dt;
	else fab_time = 0.0f;

	for (int i = 0; i < 3; i++)
	{
		float delay = i * 0.05f;
		float item_expand = fab_time > delay ?
			iam_tween_float(ImGui::GetID("fab_item") + i, 0, 1.0f, 0.2f,
				iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt) :
			iam_tween_float(ImGui::GetID("fab_item") + i, 0, 0.0f, 0.15f,
				iam_ease_preset(iam_ease_in_quad), iam_policy_crossfade, dt);

		if (item_expand > 0.01f)
		{
			float mini_radius = 20 * scale * item_expand;
			ImVec2 mini_center(fab_center.x, fab_center.y - actions[i].offset_y * scale * expand);

			// Shadow
			dl->AddCircleFilled(ImVec2(mini_center.x + 2 * scale, mini_center.y + 2 * scale),
				mini_radius, IM_COL32(0, 0, 0, 60));
			// Button
			dl->AddCircleFilled(mini_center, mini_radius, actions[i].color);
			// Icon
			ImVec2 icon_size = ImGui::CalcTextSize(actions[i].icon);
			dl->AddText(ImVec2(mini_center.x - icon_size.x * 0.5f, mini_center.y - icon_size.y * 0.5f),
				IM_COL32(255, 255, 255, (int)(item_expand * 255)), actions[i].icon);
		}
	}

	// Main FAB shadow
	dl->AddCircleFilled(ImVec2(fab_center.x + 3 * scale, fab_center.y + 3 * scale),
		fab_radius, IM_COL32(0, 0, 0, 80));

	// Main FAB
	dl->AddCircleFilled(fab_center, fab_radius, IM_COL32(244, 67, 54, 255));

	// Plus icon that rotates to X
	float cos_r = ImCos(rotation);
	float sin_r = ImSin(rotation);
	float line_len = 10 * scale;

	// Horizontal line
	ImVec2 h1(fab_center.x - line_len * cos_r, fab_center.y - line_len * sin_r);
	ImVec2 h2(fab_center.x + line_len * cos_r, fab_center.y + line_len * sin_r);
	dl->AddLine(h1, h2, IM_COL32(255, 255, 255, 255), 3 * scale);

	// Vertical line
	ImVec2 v1(fab_center.x + line_len * sin_r, fab_center.y - line_len * cos_r);
	ImVec2 v2(fab_center.x - line_len * sin_r, fab_center.y + line_len * cos_r);
	dl->AddLine(v1, v2, IM_COL32(255, 255, 255, 255), 3 * scale);

	// Click detection for main FAB
	ImVec2 mouse = ImGui::GetMousePos();
	float dist_sq = (mouse.x - fab_center.x) * (mouse.x - fab_center.x) +
					(mouse.y - fab_center.y) * (mouse.y - fab_center.y);
	if (ImGui::IsMouseClicked(0) && dist_sq <= fab_radius * fab_radius)
	{
		fab_open = !fab_open;
	}

	// Close when clicking outside
	if (fab_open && ImGui::IsMouseClicked(0) && dist_sq > fab_radius * fab_radius)
	{
		bool clicked_mini = false;
		for (int i = 0; i < 3; i++)
		{
			ImVec2 mini_center(fab_center.x, fab_center.y - actions[i].offset_y * scale * expand);
			float mini_dist_sq = (mouse.x - mini_center.x) * (mouse.x - mini_center.x) +
								 (mouse.y - mini_center.y) * (mouse.y - mini_center.y);
			if (mini_dist_sq <= 20 * scale * 20 * scale) clicked_mini = true;
		}
		if (!clicked_mini) fab_open = false;
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE: Popover Menu
// ============================================================
static void ShowUsecase_PopoverMenu()
{
	ImGui::TextWrapped(
		"Animated popover menu that expands from a button click. "
		"Items fade in with staggered timing.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static bool show_popover = false;
	static float popover_time = 0.0f;

	ImVec2 btn_pos = ImGui::GetCursorScreenPos();
	if (ImGui::Button("Options##PopoverTrigger"))
		show_popover = !show_popover;

	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 popover_pos(btn_pos.x, btn_pos.y + 30 * scale);

	if (show_popover)
	{
		popover_time += dt;

		float expand = iam_tween_float(ImGui::GetID("popover_expand"), 0,
			1.0f, 0.2f, iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		ImVec2 popover_size(140 * scale, 120 * scale * expand);

		// Background
		dl->AddRectFilled(popover_pos,
			ImVec2(popover_pos.x + popover_size.x, popover_pos.y + popover_size.y),
			IM_COL32(55, 60, 70, 250), 8 * scale);
		dl->AddRect(popover_pos,
			ImVec2(popover_pos.x + popover_size.x, popover_pos.y + popover_size.y),
			IM_COL32(80, 85, 95, 255), 8 * scale);

		// Menu items with staggered fade
		const char* items[] = { "Edit", "Duplicate", "Share", "Delete" };
		ImU32 colors[] = {
			IM_COL32(255, 255, 255, 255),
			IM_COL32(255, 255, 255, 255),
			IM_COL32(100, 180, 255, 255),
			IM_COL32(255, 100, 100, 255)
		};

		for (int i = 0; i < 4; i++)
		{
			float delay = i * 0.05f;
			float item_alpha = popover_time > delay ?
				iam_tween_float(ImGui::GetID("popover_item") + i, 0,
					1.0f, 0.15f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt) : 0.0f;

			if (expand > 0.3f + i * 0.15f)
			{
				float y = popover_pos.y + 10 * scale + i * 26 * scale;
				ImU32 col = colors[i];
				col = (col & 0x00FFFFFF) | ((int)(item_alpha * 255) << 24);
				dl->AddText(ImVec2(popover_pos.x + 15 * scale, y), col, items[i]);
			}
		}

		// Close when clicking outside
		ImVec2 mouse = ImGui::GetMousePos();
		if (ImGui::IsMouseClicked(0) &&
			(mouse.x < popover_pos.x || mouse.x > popover_pos.x + popover_size.x ||
			 mouse.y < popover_pos.y || mouse.y > popover_pos.y + popover_size.y))
		{
			if (mouse.x < btn_pos.x || mouse.x > btn_pos.x + 60 * scale ||
				mouse.y < btn_pos.y || mouse.y > btn_pos.y + 25 * scale)
			{
				show_popover = false;
				popover_time = 0.0f;
			}
		}
	}
	else
	{
		iam_tween_float(ImGui::GetID("popover_expand"), 0, 0.0f, 0.15f,
			iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);
		popover_time = 0.0f;
	}

	ImGui::Dummy(ImVec2(0, show_popover ? 130 * scale : 0));
}

// ============================================================
// USECASE: Alert Banner
// ============================================================
static void ShowUsecase_AlertBanner()
{
	ImGui::TextWrapped(
		"Animated alert banners that slide in from top with different severity levels. "
		"Auto-dismiss with progress indicator.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static int alert_type = -1;  // -1 = none, 0 = info, 1 = success, 2 = warning, 3 = error
	static float alert_timer = 0.0f;
	const float alert_duration = 3.0f;

	if (ImGui::Button("Info##AlertBtn")) { alert_type = 0; alert_timer = 0.0f; }
	ImGui::SameLine();
	if (ImGui::Button("Success##AlertBtn")) { alert_type = 1; alert_timer = 0.0f; }
	ImGui::SameLine();
	if (ImGui::Button("Warning##AlertBtn")) { alert_type = 2; alert_timer = 0.0f; }
	ImGui::SameLine();
	if (ImGui::Button("Error##AlertBtn")) { alert_type = 3; alert_timer = 0.0f; }

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 80 * scale);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	dl->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(30, 32, 38, 255), 4 * scale);

	if (alert_type >= 0)
	{
		alert_timer += dt;

		float slide = iam_tween_float(ImGui::GetID("alert_slide"), 0,
			1.0f, 0.3f, iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		ImU32 bg_colors[] = {
			IM_COL32(33, 150, 243, 230),   // Info - blue
			IM_COL32(76, 175, 80, 230),    // Success - green
			IM_COL32(255, 152, 0, 230),    // Warning - orange
			IM_COL32(244, 67, 54, 230)     // Error - red
		};
		const char* messages[] = {
			"Information: This is an info message.",
			"Success: Operation completed successfully!",
			"Warning: Please review before continuing.",
			"Error: Something went wrong!"
		};
		const char* icons[] = { "i", "+", "!", "X" };

		ImVec2 banner_size(canvas_size.x - 20 * scale, 50 * scale);
		float y_offset = (1.0f - slide) * -60 * scale;
		ImVec2 banner_pos(canvas_pos.x + 10 * scale, canvas_pos.y + 15 * scale + y_offset);

		// Banner background
		dl->AddRectFilled(banner_pos,
			ImVec2(banner_pos.x + banner_size.x, banner_pos.y + banner_size.y),
			bg_colors[alert_type], 8 * scale);

		// Icon circle
		ImVec2 icon_center(banner_pos.x + 25 * scale, banner_pos.y + banner_size.y * 0.5f);
		dl->AddCircleFilled(icon_center, 12 * scale, IM_COL32(255, 255, 255, 60));
		dl->AddText(ImVec2(icon_center.x - 4 * scale, icon_center.y - 7 * scale),
			IM_COL32(255, 255, 255, 255), icons[alert_type]);

		// Message
		dl->AddText(ImVec2(banner_pos.x + 50 * scale, banner_pos.y + 17 * scale),
			IM_COL32(255, 255, 255, 255), messages[alert_type]);

		// Progress bar for auto-dismiss
		float progress = ImClamp(alert_timer / alert_duration, 0.0f, 1.0f);
		float bar_width = banner_size.x * (1.0f - progress);
		dl->AddRectFilled(
			ImVec2(banner_pos.x, banner_pos.y + banner_size.y - 4 * scale),
			ImVec2(banner_pos.x + bar_width, banner_pos.y + banner_size.y),
			IM_COL32(255, 255, 255, 100), 0, ImDrawFlags_RoundCornersBottom);

		// Auto dismiss
		if (alert_timer >= alert_duration)
		{
			alert_type = -1;
		}
	}
	else
	{
		iam_tween_float(ImGui::GetID("alert_slide"), 0, 0.0f, 0.2f,
			iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE: Expandable List Item
// ============================================================
static void ShowUsecase_ExpandableListItem()
{
	ImGui::TextWrapped(
		"List items that expand to reveal additional content with dynamic heights. "
		"Each item has different content lengths to demonstrate adaptive sizing.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static int expanded_item = -1;

	// Each item has different number of detail lines to show dynamic height
	struct ListItem {
		const char* title;
		const char* subtitle;
		const char* details[6];  // Up to 6 lines of detail
		int num_lines;
	};

	ListItem items[] = {
		{
			"Quick Task",
			"Due: Today",
			{ "Status: In Progress", "Priority: High" },
			2
		},
		{
			"Project Alpha",
			"Due: Dec 15",
			{ "Team: Alice, Bob, Charlie, Dana", "Progress: 75% complete", "Priority: Medium", "Last updated: 2 hours ago" },
			4
		},
		{
			"Major Release v2.0",
			"Due: Q1 2025",
			{ "Components: UI, Backend, API, Docs", "Teams involved: 3 departments", "Budget: Approved", "Risk level: Low", "Dependencies: 12 items", "Sign-off required: Yes" },
			6
		}
	};

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	float width = ImGui::GetContentRegionAvail().x;
	float y_cursor = pos.y;
	float line_height = ImGui::GetTextLineHeight();

	for (int i = 0; i < 3; i++)
	{
		bool is_expanded = (expanded_item == i);
		float expand_anim = iam_tween_float(ImGui::GetID("list_expand") + i, 0,
			is_expanded ? 1.0f : 0.0f, 0.3f,
			iam_ease_preset(iam_ease_out_cubic), iam_policy_crossfade, dt);

		float base_height = 45 * scale;
		// Dynamic expanded height based on content
		float content_height = items[i].num_lines * (line_height + 4 * scale) + 15 * scale;
		float expanded_height = base_height + content_height;
		float item_height = base_height + content_height * expand_anim;

		ImVec2 item_pos(pos.x, y_cursor);
		ImVec2 item_size(width, item_height);

		// Background with hover effect
		ImU32 bg_col = is_expanded ? IM_COL32(55, 60, 75, 255) : IM_COL32(40, 45, 55, 255);
		dl->AddRectFilled(item_pos, ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y),
			bg_col, 8 * scale);

		// Left accent bar - different colors for different content sizes
		ImU32 accent_colors[] = {
			IM_COL32(76, 175, 80, 255),   // Green for quick tasks
			IM_COL32(33, 150, 243, 255),  // Blue for medium projects
			IM_COL32(255, 152, 0, 255)    // Orange for major releases
		};
		dl->AddRectFilled(item_pos, ImVec2(item_pos.x + 4 * scale, item_pos.y + item_size.y),
			accent_colors[i], 8 * scale, ImDrawFlags_RoundCornersLeft);

		// Title
		dl->AddText(ImVec2(item_pos.x + 15 * scale, item_pos.y + 8 * scale),
			IM_COL32(255, 255, 255, 255), items[i].title);

		// Subtitle with line count hint
		char subtitle_with_count[64];
		snprintf(subtitle_with_count, sizeof(subtitle_with_count), "%s  (%d items)",
			items[i].subtitle, items[i].num_lines);
		dl->AddText(ImVec2(item_pos.x + 15 * scale, item_pos.y + 26 * scale),
			IM_COL32(140, 145, 155, 255), subtitle_with_count);

		// Expand indicator (chevron that rotates)
		float arrow_rot = expand_anim * 3.14159f * 0.5f;
		ImVec2 arrow_center(item_pos.x + width - 25 * scale, item_pos.y + 22 * scale);
		float arrow_size = 6 * scale;
		ImVec2 p1(arrow_center.x - arrow_size * ImCos(arrow_rot), arrow_center.y - arrow_size * ImSin(arrow_rot));
		ImVec2 p2(arrow_center.x + arrow_size * ImSin(arrow_rot), arrow_center.y - arrow_size * ImCos(arrow_rot));
		ImVec2 p3(arrow_center.x + arrow_size * ImCos(arrow_rot), arrow_center.y + arrow_size * ImSin(arrow_rot));
		dl->AddTriangleFilled(p1, p2, p3, IM_COL32(180, 185, 195, 255));

		// Expanded content - each line fades in with slight stagger
		if (expand_anim > 0.05f)
		{
			float content_alpha = ImClamp((expand_anim - 0.05f) / 0.5f, 0.0f, 1.0f);

			// Separator line
			float sep_width = (width - 30 * scale) * ImMin(expand_anim * 2.0f, 1.0f);
			dl->AddLine(
				ImVec2(item_pos.x + 15 * scale, item_pos.y + base_height - 2 * scale),
				ImVec2(item_pos.x + 15 * scale + sep_width, item_pos.y + base_height - 2 * scale),
				IM_COL32(80, 85, 95, (int)(content_alpha * 200)));

			// Draw each detail line
			for (int line = 0; line < items[i].num_lines; line++)
			{
				float line_delay = line * 0.08f;
				float line_alpha = ImClamp((expand_anim - 0.1f - line_delay) / 0.4f, 0.0f, 1.0f);

				if (line_alpha > 0.0f)
				{
					float y_offset = base_height + 8 * scale + line * (line_height + 4 * scale);

					// Bullet point
					ImU32 bullet_col = (accent_colors[i] & 0x00FFFFFF) | ((ImU32)(line_alpha * 255) << 24);
					dl->AddCircleFilled(
						ImVec2(item_pos.x + 20 * scale, item_pos.y + y_offset + line_height * 0.5f),
						3 * scale, bullet_col);

					// Text
					dl->AddText(
						ImVec2(item_pos.x + 30 * scale, item_pos.y + y_offset),
						IM_COL32(200, 205, 215, (int)(line_alpha * 255)),
						items[i].details[line]);
				}
			}
		}

		// Click detection on header area
		ImVec2 mouse = ImGui::GetMousePos();
		if (ImGui::IsMouseClicked(0) &&
			mouse.x >= item_pos.x && mouse.x <= item_pos.x + item_size.x &&
			mouse.y >= item_pos.y && mouse.y <= item_pos.y + base_height)
		{
			expanded_item = is_expanded ? -1 : i;
		}

		y_cursor += item_height + 6 * scale;
	}

	ImGui::Dummy(ImVec2(width, y_cursor - pos.y));
}

// ============================================================
// USECASE: Image Gallery Grid
// ============================================================
static void ShowUsecase_ImageGalleryGrid()
{
	ImGui::TextWrapped(
		"Image gallery with hover zoom effect and selection animation. "
		"Grid items scale up smoothly on hover.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static int selected_image = -1;
	static int hovered_image = -1;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	float cell_size = 70 * scale;
	float gap = 8 * scale;
	int cols = 4;
	int rows = 2;

	ImU32 colors[] = {
		IM_COL32(255, 107, 107, 255), IM_COL32(78, 205, 196, 255),
		IM_COL32(199, 244, 100, 255), IM_COL32(255, 230, 109, 255),
		IM_COL32(107, 185, 240, 255), IM_COL32(238, 130, 238, 255),
		IM_COL32(255, 165, 0, 255), IM_COL32(50, 205, 50, 255)
	};

	ImVec2 mouse = ImGui::GetMousePos();
	hovered_image = -1;

	for (int row = 0; row < rows; row++)
	{
		for (int col = 0; col < cols; col++)
		{
			int idx = row * cols + col;
			float x = pos.x + col * (cell_size + gap);
			float y = pos.y + row * (cell_size + gap);

			// Check hover
			bool is_hovered = mouse.x >= x && mouse.x <= x + cell_size &&
							  mouse.y >= y && mouse.y <= y + cell_size;
			if (is_hovered) hovered_image = idx;

			bool is_selected = (selected_image == idx);

			float hover_scale = iam_tween_float(ImGui::GetID("gallery_hover") + idx, 0,
				is_hovered ? 1.1f : 1.0f, 0.15f,
				iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

			float select_glow = iam_tween_float(ImGui::GetID("gallery_select") + idx, 0,
				is_selected ? 1.0f : 0.0f, 0.2f,
				iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

			float actual_size = cell_size * hover_scale;
			float offset = (actual_size - cell_size) * 0.5f;

			// Selection glow
			if (select_glow > 0.01f)
			{
				dl->AddRectFilled(
					ImVec2(x - offset - 3 * scale, y - offset - 3 * scale),
					ImVec2(x - offset + actual_size + 3 * scale, y - offset + actual_size + 3 * scale),
					IM_COL32(255, 255, 255, (int)(select_glow * 150)), 10 * scale);
			}

			// Image placeholder (colored rectangle)
			dl->AddRectFilled(
				ImVec2(x - offset, y - offset),
				ImVec2(x - offset + actual_size, y - offset + actual_size),
				colors[idx], 8 * scale);

			// Checkmark for selected
			if (select_glow > 0.5f)
			{
				ImVec2 check_center(x - offset + actual_size - 15 * scale, y - offset + 15 * scale);
				dl->AddCircleFilled(check_center, 10 * scale, IM_COL32(255, 255, 255, 255));
				dl->AddText(ImVec2(check_center.x - 5 * scale, check_center.y - 7 * scale),
					IM_COL32(0, 150, 0, 255), "+");
			}

			// Click to select
			if (is_hovered && ImGui::IsMouseClicked(0))
			{
				selected_image = is_selected ? -1 : idx;
			}
		}
	}

	ImGui::Dummy(ImVec2(cols * (cell_size + gap), rows * (cell_size + gap)));
}

// ============================================================
// USECASE: Animated Badge Counter
// ============================================================
static void ShowUsecase_AnimatedBadgeCounter()
{
	ImGui::TextWrapped(
		"Notification badges with animated count updates. "
		"Badges bounce and pulse when count changes.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static int badge_counts[4] = { 3, 12, 99, 0 };
	static float badge_bounce[4] = { 0, 0, 0, 0 };

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	const char* labels[] = { "Mail", "Chat", "Alerts", "Tasks" };
	ImU32 colors[] = {
		IM_COL32(244, 67, 54, 255),   // Red
		IM_COL32(76, 175, 80, 255),   // Green
		IM_COL32(255, 152, 0, 255),   // Orange
		IM_COL32(33, 150, 243, 255)   // Blue
	};

	float spacing = 90 * scale;

	for (int i = 0; i < 4; i++)
	{
		ImVec2 icon_pos(pos.x + 30 * scale + i * spacing, pos.y + 40 * scale);

		// Icon background (simulated app icon)
		dl->AddRectFilled(
			ImVec2(icon_pos.x - 25 * scale, icon_pos.y - 25 * scale),
			ImVec2(icon_pos.x + 25 * scale, icon_pos.y + 25 * scale),
			IM_COL32(60, 65, 80, 255), 12 * scale);

		// Icon letter
		ImVec2 letter_size = ImGui::CalcTextSize(labels[i]);
		dl->AddText(ImVec2(icon_pos.x - letter_size.x * 0.5f, icon_pos.y - letter_size.y * 0.5f),
			IM_COL32(200, 205, 215, 255), labels[i]);

		// Animate bounce decay
		if (badge_bounce[i] > 0.0f)
			badge_bounce[i] = ImMax(0.0f, badge_bounce[i] - dt * 3.0f);

		// Badge (only if count > 0)
		if (badge_counts[i] > 0)
		{
			float bounce = iam_tween_float(ImGui::GetID("badge_bounce") + i, 0,
				1.0f + badge_bounce[i] * 0.4f, 0.15f,
				iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

			ImVec2 badge_center(icon_pos.x + 20 * scale, icon_pos.y - 20 * scale);
			float badge_radius = 12 * scale * bounce;

			// Badge glow when bouncing
			if (badge_bounce[i] > 0.1f)
			{
				dl->AddCircleFilled(badge_center, badge_radius + 4 * scale,
					IM_COL32((colors[i] >> 0) & 0xFF, (colors[i] >> 8) & 0xFF, (colors[i] >> 16) & 0xFF,
						(int)(badge_bounce[i] * 100)));
			}

			// Badge circle
			dl->AddCircleFilled(badge_center, badge_radius, colors[i]);

			// Badge count
			char count_str[8];
			if (badge_counts[i] > 99)
				snprintf(count_str, sizeof(count_str), "99+");
			else
				snprintf(count_str, sizeof(count_str), "%d", badge_counts[i]);

			ImVec2 count_size = ImGui::CalcTextSize(count_str);
			dl->AddText(
				ImVec2(badge_center.x - count_size.x * 0.5f, badge_center.y - count_size.y * 0.5f),
				IM_COL32(255, 255, 255, 255), count_str);
		}

		// Label below
		ImVec2 label_size = ImGui::CalcTextSize(labels[i]);
		dl->AddText(ImVec2(icon_pos.x - label_size.x * 0.5f, icon_pos.y + 30 * scale),
			IM_COL32(150, 155, 165, 255), labels[i]);
	}

	// Control buttons
	ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 90 * scale));
	if (ImGui::Button("Add Notification##Badge"))
	{
		int idx = (int)(ImGui::GetTime() * 10) % 4;
		badge_counts[idx]++;
		badge_bounce[idx] = 1.0f;
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear All##Badge"))
	{
		for (int i = 0; i < 4; i++)
		{
			badge_counts[i] = 0;
			badge_bounce[i] = 0.0f;
		}
	}

	ImGui::Dummy(ImVec2(0, 20 * scale));
}

// ============================================================
// USECASE: Magnetic Cursor Effect
// ============================================================
static void ShowUsecase_MagneticCursor()
{
	ImGui::TextWrapped(
		"Buttons with magnetic cursor attraction effect. "
		"Elements subtly move towards the cursor when hovering nearby.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 140 * scale);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Background
	dl->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(25, 30, 40, 255), 8 * scale);

	ImVec2 mouse = ImGui::GetMousePos();

	struct MagneticButton {
		const char* label;
		ImVec2 base_pos;
		float radius;
		ImU32 color;
	};

	// Define buttons at base positions
	float center_y = canvas_pos.y + canvas_size.y * 0.5f;
	MagneticButton buttons[] = {
		{ "A", ImVec2(canvas_pos.x + 80 * scale, center_y), 30 * scale, IM_COL32(244, 67, 54, 255) },
		{ "B", ImVec2(canvas_pos.x + 180 * scale, center_y), 35 * scale, IM_COL32(76, 175, 80, 255) },
		{ "C", ImVec2(canvas_pos.x + 290 * scale, center_y), 28 * scale, IM_COL32(33, 150, 243, 255) },
		{ "D", ImVec2(canvas_pos.x + 380 * scale, center_y), 32 * scale, IM_COL32(255, 152, 0, 255) }
	};

	for (int i = 0; i < 4; i++)
	{
		ImVec2 base = buttons[i].base_pos;
		float attract_radius = buttons[i].radius * 2.5f;

		// Calculate distance to mouse
		float dx = mouse.x - base.x;
		float dy = mouse.y - base.y;
		float dist = sqrtf(dx * dx + dy * dy);

		// Magnetic attraction
		float attraction = 0.0f;
		if (dist < attract_radius && dist > 0.01f)
		{
			attraction = 1.0f - (dist / attract_radius);
			attraction = attraction * attraction; // Quadratic falloff
		}

		// Target offset towards cursor
		float max_offset = 15 * scale;
		float target_x = (dist > 0.01f) ? (dx / dist) * max_offset * attraction : 0.0f;
		float target_y = (dist > 0.01f) ? (dy / dist) * max_offset * attraction : 0.0f;

		// Smooth the offset
		float offset_x = iam_tween_float(ImGui::GetID("mag_x") + i, 0, target_x, 0.1f,
			iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);
		float offset_y = iam_tween_float(ImGui::GetID("mag_y") + i, 0, target_y, 0.1f,
			iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

		ImVec2 actual_pos(base.x + offset_x, base.y + offset_y);

		// Scale up when attracted
		float target_scale = 1.0f + attraction * 0.2f;
		float anim_scale = iam_tween_float(ImGui::GetID("mag_scale") + i, 0, target_scale, 0.15f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		float actual_radius = buttons[i].radius * anim_scale;

		// Glow effect when attracted
		if (attraction > 0.1f)
		{
			dl->AddCircleFilled(actual_pos, actual_radius + 8 * scale * attraction,
				IM_COL32((buttons[i].color >> 0) & 0xFF, (buttons[i].color >> 8) & 0xFF,
					(buttons[i].color >> 16) & 0xFF, (int)(attraction * 80)));
		}

		// Shadow
		dl->AddCircleFilled(ImVec2(actual_pos.x + 3 * scale, actual_pos.y + 3 * scale),
			actual_radius, IM_COL32(0, 0, 0, 60));

		// Button
		dl->AddCircleFilled(actual_pos, actual_radius, buttons[i].color);

		// Label
		ImVec2 label_size = ImGui::CalcTextSize(buttons[i].label);
		dl->AddText(ImVec2(actual_pos.x - label_size.x * 0.5f, actual_pos.y - label_size.y * 0.5f),
			IM_COL32(255, 255, 255, 255), buttons[i].label);

		// Click effect
		float click_dist = sqrtf((mouse.x - actual_pos.x) * (mouse.x - actual_pos.x) +
								  (mouse.y - actual_pos.y) * (mouse.y - actual_pos.y));
		if (ImGui::IsMouseClicked(0) && click_dist <= actual_radius)
		{
			// Could trigger action here
		}
	}

	// Instruction
	const char* hint = "Move cursor near buttons";
	ImVec2 hint_size = ImGui::CalcTextSize(hint);
	dl->AddText(ImVec2(canvas_pos.x + (canvas_size.x - hint_size.x) * 0.5f,
		canvas_pos.y + canvas_size.y - 25 * scale),
		IM_COL32(100, 105, 115, 255), hint);

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE: Particle Burst
// ============================================================
static void ShowUsecase_ParticleBurst()
{
	ImGui::TextWrapped(
		"Particle burst effect triggered on click. "
		"Great for celebrations, achievements, or feedback.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static float burst_time = -1.0f;
	static ImVec2 burst_pos;
	const int num_particles = 16;
	const float burst_duration = 1.0f;

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 120 * scale);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Background
	dl->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(30, 35, 45, 255), 6 * scale);

	// Instruction
	if (burst_time < 0)
	{
		const char* hint = "Click anywhere!";
		ImVec2 hint_size = ImGui::CalcTextSize(hint);
		dl->AddText(
			ImVec2(canvas_pos.x + (canvas_size.x - hint_size.x) * 0.5f,
				   canvas_pos.y + (canvas_size.y - hint_size.y) * 0.5f),
			IM_COL32(150, 155, 165, 255), hint);
	}

	// Check for click in canvas
	ImVec2 mouse = ImGui::GetMousePos();
	if (ImGui::IsMouseClicked(0) &&
		mouse.x >= canvas_pos.x && mouse.x <= canvas_pos.x + canvas_size.x &&
		mouse.y >= canvas_pos.y && mouse.y <= canvas_pos.y + canvas_size.y)
	{
		burst_time = 0.0f;
		burst_pos = mouse;
	}

	// Animate particles
	if (burst_time >= 0.0f)
	{
		burst_time += dt;

		float t = burst_time / burst_duration;
		if (t > 1.0f)
		{
			burst_time = -1.0f;
		}
		else
		{
			for (int i = 0; i < num_particles; i++)
			{
				float angle = (float)i / num_particles * 6.28318f;
				float speed = 80.0f + (i % 3) * 30.0f;
				float size = 4.0f + (i % 4) * 2.0f;

				// Eased position
				float eased_t = iam_eval_preset(iam_ease_out_expo, t);
				float dist = speed * eased_t * scale;

				ImVec2 p(
					burst_pos.x + ImCos(angle) * dist,
					burst_pos.y + ImSin(angle) * dist + t * t * 50 * scale  // gravity
				);

				// Fade out
				float alpha = 1.0f - t;
				float particle_size = size * scale * (1.0f - t * 0.5f);

				ImU32 colors[] = {
					IM_COL32(255, 100, 100, (int)(alpha * 255)),
					IM_COL32(100, 255, 100, (int)(alpha * 255)),
					IM_COL32(100, 100, 255, (int)(alpha * 255)),
					IM_COL32(255, 255, 100, (int)(alpha * 255))
				};

				dl->AddCircleFilled(p, particle_size, colors[i % 4]);
			}
		}
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE: Glowing Border
// ============================================================
static void ShowUsecase_GlowingBorder()
{
	ImGui::TextWrapped(
		"Animated glowing border effect for highlighting elements. "
		"Pulses with customizable color.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static float glow_time = 0.0f;
	glow_time += dt;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImDrawList* dl = ImGui::GetWindowDrawList();

	ImVec2 box_size(200 * scale, 80 * scale);

	// Animated glow intensity
	float pulse = (ImSin(glow_time * 3.0f) + 1.0f) * 0.5f;
	float glow_alpha = 0.3f + pulse * 0.4f;
	float glow_size = 8 * scale + pulse * 4 * scale;

	// Outer glow layers
	for (int i = 3; i >= 0; i--)
	{
		float layer_size = glow_size * (1.0f + i * 0.3f);
		float layer_alpha = glow_alpha * (1.0f - i * 0.25f);
		dl->AddRect(
			ImVec2(pos.x - layer_size, pos.y - layer_size),
			ImVec2(pos.x + box_size.x + layer_size, pos.y + box_size.y + layer_size),
			IM_COL32(100, 180, 255, (int)(layer_alpha * 255)),
			12 * scale + i * 2 * scale, 0, 2.0f + i);
	}

	// Main box
	dl->AddRectFilled(pos, ImVec2(pos.x + box_size.x, pos.y + box_size.y),
		IM_COL32(40, 45, 55, 255), 8 * scale);
	dl->AddRect(pos, ImVec2(pos.x + box_size.x, pos.y + box_size.y),
		IM_COL32(100, 180, 255, 255), 8 * scale, 0, 2.0f);

	// Content
	const char* text = "Featured Item";
	ImVec2 text_size = ImGui::CalcTextSize(text);
	dl->AddText(
		ImVec2(pos.x + (box_size.x - text_size.x) * 0.5f, pos.y + (box_size.y - text_size.y) * 0.5f),
		IM_COL32(255, 255, 255, 255), text);

	ImGui::Dummy(ImVec2(box_size.x, box_size.y + 20 * scale));
}

// ============================================================
// USECASE: Animated Graph Node
// ============================================================
static void ShowUsecase_AnimatedGraphNode()
{
	ImGui::TextWrapped(
		"Graph nodes with animated connections and hover states. "
		"Common in node-based editors and flowcharts.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static int hovered_node = -1;
	static float connection_anim = 0.0f;
	connection_anim += dt;

	struct Node {
		ImVec2 pos;
		const char* label;
		ImU32 color;
	};

	Node nodes[] = {
		{ ImVec2(50, 50), "Input", IM_COL32(100, 180, 255, 255) },
		{ ImVec2(180, 30), "Process", IM_COL32(255, 180, 100, 255) },
		{ ImVec2(180, 90), "Filter", IM_COL32(180, 100, 255, 255) },
		{ ImVec2(310, 60), "Output", IM_COL32(100, 255, 150, 255) }
	};

	int connections[][2] = { {0, 1}, {0, 2}, {1, 3}, {2, 3} };

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 canvas_size(380 * scale, 140 * scale);

	dl->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(25, 28, 35, 255), 6 * scale);

	ImVec2 mouse = ImGui::GetMousePos();
	hovered_node = -1;

	// Draw connections with animated flow
	for (int c = 0; c < 4; c++)
	{
		int from = connections[c][0];
		int to = connections[c][1];

		ImVec2 p1(canvas_pos.x + nodes[from].pos.x * scale + 35 * scale,
				  canvas_pos.y + nodes[from].pos.y * scale + 15 * scale);
		ImVec2 p2(canvas_pos.x + nodes[to].pos.x * scale,
				  canvas_pos.y + nodes[to].pos.y * scale + 15 * scale);

		// Base line
		dl->AddLine(p1, p2, IM_COL32(80, 85, 100, 255), 2 * scale);

		// Animated dot along line
		float t = ImFmod(connection_anim * 0.5f + c * 0.25f, 1.0f);
		ImVec2 dot(p1.x + (p2.x - p1.x) * t, p1.y + (p2.y - p1.y) * t);
		dl->AddCircleFilled(dot, 4 * scale, IM_COL32(255, 255, 255, 200));
	}

	// Draw nodes
	for (int i = 0; i < 4; i++)
	{
		ImVec2 node_pos(canvas_pos.x + nodes[i].pos.x * scale,
					    canvas_pos.y + nodes[i].pos.y * scale);
		ImVec2 node_size(70 * scale, 30 * scale);

		bool is_hovered = mouse.x >= node_pos.x && mouse.x <= node_pos.x + node_size.x &&
						  mouse.y >= node_pos.y && mouse.y <= node_pos.y + node_size.y;
		if (is_hovered) hovered_node = i;

		float hover_scale = iam_tween_float(ImGui::GetID("node_hover") + i, 0,
			is_hovered ? 1.05f : 1.0f, 0.15f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);

		float glow_alpha = iam_tween_float(ImGui::GetID("node_glow") + i, 0,
			is_hovered ? 0.5f : 0.0f, 0.2f,
			iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

		ImVec2 scaled_size(node_size.x * hover_scale, node_size.y * hover_scale);
		ImVec2 offset((scaled_size.x - node_size.x) * 0.5f, (scaled_size.y - node_size.y) * 0.5f);

		// Glow
		if (glow_alpha > 0.01f)
		{
			dl->AddRectFilled(
				ImVec2(node_pos.x - offset.x - 4 * scale, node_pos.y - offset.y - 4 * scale),
				ImVec2(node_pos.x - offset.x + scaled_size.x + 4 * scale,
					   node_pos.y - offset.y + scaled_size.y + 4 * scale),
				(nodes[i].color & 0x00FFFFFF) | ((int)(glow_alpha * 255) << 24),
				10 * scale);
		}

		// Node background
		dl->AddRectFilled(
			ImVec2(node_pos.x - offset.x, node_pos.y - offset.y),
			ImVec2(node_pos.x - offset.x + scaled_size.x, node_pos.y - offset.y + scaled_size.y),
			IM_COL32(50, 55, 65, 255), 6 * scale);
		dl->AddRect(
			ImVec2(node_pos.x - offset.x, node_pos.y - offset.y),
			ImVec2(node_pos.x - offset.x + scaled_size.x, node_pos.y - offset.y + scaled_size.y),
			nodes[i].color, 6 * scale, 0, 2.0f);

		// Label
		ImVec2 text_size = ImGui::CalcTextSize(nodes[i].label);
		dl->AddText(
			ImVec2(node_pos.x - offset.x + (scaled_size.x - text_size.x) * 0.5f,
				   node_pos.y - offset.y + (scaled_size.y - text_size.y) * 0.5f),
			IM_COL32(255, 255, 255, 255), nodes[i].label);
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE: Playback Controls
// ============================================================
static void ShowUsecase_PlaybackControls()
{
	ImGui::TextWrapped(
		"Media playback controls with animated state transitions. "
		"Play/pause morph and progress scrubbing.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static bool is_playing = false;
	static float playback_pos = 0.3f;
	static float play_anim = 0.0f;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 canvas_size(300 * scale, 60 * scale);

	// Background
	dl->AddRectFilled(pos, ImVec2(pos.x + canvas_size.x, pos.y + canvas_size.y),
		IM_COL32(35, 40, 50, 255), 8 * scale);

	// Play/Pause button with morph animation
	float morph = iam_tween_float(ImGui::GetID("play_morph"), 0,
		is_playing ? 1.0f : 0.0f, 0.2f,
		iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

	ImVec2 btn_center(pos.x + 30 * scale, pos.y + canvas_size.y * 0.5f);
	float btn_radius = 18 * scale;

	// Button background
	dl->AddCircleFilled(btn_center, btn_radius, IM_COL32(100, 180, 255, 255));

	// Morph between play triangle and pause bars
	if (morph < 0.5f)
	{
		// Play triangle (fading out as morph increases)
		float alpha = 1.0f - morph * 2.0f;
		ImVec2 p1(btn_center.x - 6 * scale, btn_center.y - 8 * scale);
		ImVec2 p2(btn_center.x - 6 * scale, btn_center.y + 8 * scale);
		ImVec2 p3(btn_center.x + 8 * scale, btn_center.y);
		dl->AddTriangleFilled(p1, p2, p3, IM_COL32(255, 255, 255, (int)(alpha * 255)));
	}
	if (morph > 0.5f)
	{
		// Pause bars (fading in as morph increases)
		float alpha = (morph - 0.5f) * 2.0f;
		dl->AddRectFilled(
			ImVec2(btn_center.x - 7 * scale, btn_center.y - 7 * scale),
			ImVec2(btn_center.x - 2 * scale, btn_center.y + 7 * scale),
			IM_COL32(255, 255, 255, (int)(alpha * 255)), 2 * scale);
		dl->AddRectFilled(
			ImVec2(btn_center.x + 2 * scale, btn_center.y - 7 * scale),
			ImVec2(btn_center.x + 7 * scale, btn_center.y + 7 * scale),
			IM_COL32(255, 255, 255, (int)(alpha * 255)), 2 * scale);
	}

	// Progress bar
	float bar_x = pos.x + 60 * scale;
	float bar_width = canvas_size.x - 80 * scale;
	float bar_y = pos.y + canvas_size.y * 0.5f;

	// Track
	dl->AddRectFilled(
		ImVec2(bar_x, bar_y - 3 * scale),
		ImVec2(bar_x + bar_width, bar_y + 3 * scale),
		IM_COL32(60, 65, 75, 255), 3 * scale);

	// Progress with smooth animation
	float anim_pos = iam_tween_float(ImGui::GetID("playback_pos"), 0,
		playback_pos, 0.1f,
		iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

	dl->AddRectFilled(
		ImVec2(bar_x, bar_y - 3 * scale),
		ImVec2(bar_x + bar_width * anim_pos, bar_y + 3 * scale),
		IM_COL32(100, 180, 255, 255), 3 * scale);

	// Scrubber handle
	float handle_x = bar_x + bar_width * anim_pos;
	dl->AddCircleFilled(ImVec2(handle_x, bar_y), 8 * scale, IM_COL32(255, 255, 255, 255));

	// Time display
	int current_sec = (int)(playback_pos * 180);
	int total_sec = 180;
	char time_str[32];
	snprintf(time_str, sizeof(time_str), "%d:%02d / %d:%02d",
		current_sec / 60, current_sec % 60, total_sec / 60, total_sec % 60);
	dl->AddText(ImVec2(bar_x, bar_y + 12 * scale), IM_COL32(150, 155, 165, 255), time_str);

	// Interaction
	ImVec2 mouse = ImGui::GetMousePos();
	if (ImGui::IsMouseClicked(0))
	{
		// Play/pause button click
		float dist = sqrtf((mouse.x - btn_center.x) * (mouse.x - btn_center.x) +
						   (mouse.y - btn_center.y) * (mouse.y - btn_center.y));
		if (dist <= btn_radius)
		{
			is_playing = !is_playing;
		}

		// Progress bar click
		if (mouse.x >= bar_x && mouse.x <= bar_x + bar_width &&
			mouse.y >= bar_y - 10 * scale && mouse.y <= bar_y + 10 * scale)
		{
			playback_pos = (mouse.x - bar_x) / bar_width;
		}
	}

	// Auto-advance when playing
	if (is_playing)
	{
		playback_pos += dt / 180.0f;
		if (playback_pos >= 1.0f)
		{
			playback_pos = 0.0f;
			is_playing = false;
		}
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// SURPRISE SECTION USECASES
// ============================================================

// ============================================================
// USECASE: Orbiting Planets
// ============================================================
static void ShowUsecase_OrbitingPlanets()
{
	ImGui::TextWrapped(
		"Mini solar system using iam_tween_float for smooth orbital motion and "
		"gravitational pull effects. Click anywhere to create gravity wells!");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static float orbit_angles[4] = { 0, 1.57f, 3.14f, 4.71f };
	static bool pull_active = false;
	static ImVec2 pull_target;

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 200 * scale);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Space background
	dl->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(10, 12, 20, 255), 8 * scale);

	ImVec2 center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

	// Twinkling stars using iam_oscillate for smooth brightness
	for (int i = 0; i < 20; i++)
	{
		float sx = canvas_pos.x + ((i * 127 + 47) % (int)canvas_size.x);
		float sy = canvas_pos.y + ((i * 89 + 23) % (int)canvas_size.y);

		// Each star twinkles at different rate using oscillate
		float twinkle = 0.65f + 0.35f * iam_oscillate(ImGui::GetID("star_twinkle") + i,
			1.0f, 0.5f + (i % 3) * 0.2f, iam_wave_sine, (float)i * 0.1f, dt);

		dl->AddCircleFilled(ImVec2(sx, sy), 2 * scale * twinkle,
			IM_COL32(255, 255, 255, (int)(twinkle * 255)));
	}

	// Sun pulse using iam_oscillate
	float sun_pulse = 1.0f + 0.15f * iam_oscillate(ImGui::GetID("sun_pulse"),
		1.0f, 0.5f, iam_wave_sine, 0.0f, dt);

	dl->AddCircleFilled(center, 25 * scale * sun_pulse, IM_COL32(255, 200, 50, 40));
	dl->AddCircleFilled(center, 18 * scale * sun_pulse, IM_COL32(255, 220, 100, 100));
	dl->AddCircleFilled(center, 12 * scale, IM_COL32(255, 240, 150, 255));

	// Planet properties
	float orbit_radii[] = { 45 * scale, 70 * scale, 100 * scale, 130 * scale };
	float orbit_speeds[] = { 2.0f, 1.2f, 0.7f, 0.4f };
	float planet_sizes[] = { 6 * scale, 10 * scale, 8 * scale, 14 * scale };
	ImU32 planet_colors[] = {
		IM_COL32(180, 180, 200, 255), IM_COL32(100, 150, 255, 255),
		IM_COL32(255, 100, 100, 255), IM_COL32(255, 200, 150, 255)
	};

	// Handle click for gravity pull
	ImVec2 mouse = ImGui::GetMousePos();
	if (ImGui::IsMouseClicked(0) &&
		mouse.x >= canvas_pos.x && mouse.x <= canvas_pos.x + canvas_size.x &&
		mouse.y >= canvas_pos.y && mouse.y <= canvas_pos.y + canvas_size.y)
	{
		pull_active = true;
		pull_target = mouse;
	}

	// Animate pull wave using tween
	float pull_wave = iam_tween_float(ImGui::GetID("pull_wave"), ImHashStr("wave"),
		pull_active ? 1.0f : 0.0f, 0.8f,
		iam_ease_preset(iam_ease_out_expo), iam_policy_crossfade, dt);

	if (pull_wave > 0.95f) pull_active = false;

	// Draw planets with tweened positions
	for (int i = 0; i < 4; i++)
	{
		// Update orbit angle
		orbit_angles[i] += dt * orbit_speeds[i];

		// Base orbital position
		float base_x = center.x + cosf(orbit_angles[i]) * orbit_radii[i];
		float base_y = center.y + sinf(orbit_angles[i]) * orbit_radii[i];

		// Apply gravity pull using tween for smooth attraction
		float pull_offset_x = 0, pull_offset_y = 0;
		if (pull_wave > 0.01f && pull_wave < 0.95f)
		{
			float dx = pull_target.x - base_x;
			float dy = pull_target.y - base_y;
			float attraction = (1.0f - pull_wave) * 30.0f / (i + 1);
			pull_offset_x = dx * 0.01f * attraction;
			pull_offset_y = dy * 0.01f * attraction;
		}

		// Smooth position using tween
		float px = iam_tween_float(ImGui::GetID("planet_x") + i, ImHashStr("px"),
			base_x + pull_offset_x, 0.1f,
			iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);
		float py = iam_tween_float(ImGui::GetID("planet_y") + i, ImHashStr("py"),
			base_y + pull_offset_y, 0.1f,
			iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

		// Orbit trail
		dl->AddCircle(center, orbit_radii[i], IM_COL32(100, 100, 150, 40), 64, 1.0f);

		// Planet shadow
		dl->AddCircleFilled(ImVec2(px + 2 * scale, py + 2 * scale),
			planet_sizes[i], IM_COL32(0, 0, 0, 60));

		// Planet
		dl->AddCircleFilled(ImVec2(px, py), planet_sizes[i], planet_colors[i]);
		dl->AddCircleFilled(ImVec2(px - planet_sizes[i] * 0.3f, py - planet_sizes[i] * 0.3f),
			planet_sizes[i] * 0.3f, IM_COL32(255, 255, 255, 80));

		// Moon for large planet - orbit using oscillate with sawtooth for continuous rotation
		if (i == 3)
		{
			// Use sawtooth wave for continuous 0 to 2*PI cycle
			static float moon_angle = 0.0f;
			moon_angle += dt * 3.14159f;  // Half rotation per second
			if (moon_angle > IAM_2PI) moon_angle -= IAM_2PI;
			float moon_dist = 22 * scale;
			float mx = px + cosf(moon_angle) * moon_dist;
			float my = py + sinf(moon_angle) * moon_dist;
			dl->AddCircleFilled(ImVec2(mx, my), 4 * scale, IM_COL32(200, 200, 220, 255));
		}
	}

	// Gravity wave effect
	if (pull_wave > 0.01f && pull_wave < 0.99f)
	{
		float wave_radius = pull_wave * 120 * scale;
		int alpha = (int)((1.0f - pull_wave) * 200);
		dl->AddCircle(pull_target, wave_radius, IM_COL32(100, 150, 255, alpha), 32, 2.0f);
		dl->AddCircle(pull_target, wave_radius * 0.6f, IM_COL32(150, 200, 255, alpha / 2), 32, 1.5f);
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE: Liquid Fill Gauge
// ============================================================
static void ShowUsecase_LiquidFillGauge()
{
	ImGui::TextWrapped(
		"Liquid gauge using iam_tween_float with elastic easing for bouncy fill, "
		"and ping-pong policy for continuous wave animation.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static float target_level = 0.65f;

	if (ImGui::Button("-10%##Liquid")) target_level = ImMax(0.0f, target_level - 0.1f);
	ImGui::SameLine();
	if (ImGui::Button("+10%##Liquid")) target_level = ImMin(1.0f, target_level + 0.1f);
	ImGui::SameLine();
	ImGui::Text("Target: %.0f%%", target_level * 100);

	// Main level uses elastic easing for bouncy liquid effect
	float level = iam_tween_float(ImGui::GetID("liquid_level"), ImHashStr("lvl"),
		target_level, 0.8f, iam_ease_preset(iam_ease_out_elastic), iam_policy_crossfade, dt);

	// Wave animations using oscillate for continuous wave motion
	float wave_phase1 = 0.5f + 0.5f * iam_oscillate(ImGui::GetID("wave1"),
		1.0f, 0.8f, iam_wave_sine, 0.0f, dt);
	float wave_phase2 = 0.5f + 0.5f * iam_oscillate(ImGui::GetID("wave2"),
		1.0f, 1.2f, iam_wave_sine, 0.5f, dt);

	// Color transition using tween
	float color_t = iam_tween_float(ImGui::GetID("liquid_color"), ImHashStr("col"),
		level, 0.3f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 160 * scale);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Background
	dl->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(25, 30, 40, 255), 8 * scale);

	// Gauge container
	float gauge_radius = 60 * scale;
	ImVec2 gauge_center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

	// Outer ring with glow based on level
	float ring_glow = iam_tween_float(ImGui::GetID("ring_glow"), ImHashStr("glow"),
		level > 0.8f ? 1.0f : 0.0f, 0.3f,
		iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);
	if (ring_glow > 0.01f)
	{
		dl->AddCircle(gauge_center, gauge_radius + 8 * scale,
			IM_COL32(100, 200, 255, (int)(ring_glow * 100)), 64, 6 * scale);
	}
	dl->AddCircle(gauge_center, gauge_radius + 5 * scale, IM_COL32(60, 70, 90, 255), 64, 4 * scale);

	// Clip liquid to circle
	float liquid_y = gauge_center.y + gauge_radius - level * gauge_radius * 2;

	// Smooth color interpolation
	int r = (int)(255 * (1.0f - color_t) + 100 * color_t);
	int g = (int)(100 * (1.0f - color_t) + 200 * color_t);
	int b = (int)(100 * (1.0f - color_t) + 255 * color_t);
	ImU32 liquid_color = IM_COL32(r, g, b, 200);

	// Draw liquid with ImAnim-driven wave effect
	for (int y_step = 0; y_step <= 30; y_step++)
	{
		float y = gauge_center.y - gauge_radius + (y_step / 30.0f) * gauge_radius * 2;
		if (y < liquid_y) continue;

		float dy = y - gauge_center.y;
		float half_width = sqrtf(ImMax(0.0f, gauge_radius * gauge_radius - dy * dy));

		// Wave offset driven by tweened phases
		float wave1 = (wave_phase1 * 2.0f - 1.0f) * 4 * scale * sinf(y * 0.08f);
		float wave2 = (wave_phase2 * 2.0f - 1.0f) * 3 * scale * sinf(y * 0.12f + 1.5f);
		float wave_offset = wave1 + wave2;

		// Adjust wave intensity based on distance from surface
		float surface_dist = y - liquid_y;
		float wave_factor = ImClamp(1.0f - surface_dist / (20 * scale), 0.0f, 1.0f);
		wave_offset *= wave_factor;

		if (half_width > 0)
		{
			dl->AddRectFilled(
				ImVec2(gauge_center.x - half_width + wave_offset, y),
				ImVec2(gauge_center.x + half_width + wave_offset, y + gauge_radius * 2 / 30.0f + 1),
				liquid_color);
		}
	}

	// Percentage text with scale animation
	float text_scale = iam_tween_float(ImGui::GetID("pct_scale"), ImHashStr("scl"),
		1.0f, 0.2f, iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);
	char pct[16];
	snprintf(pct, sizeof(pct), "%.0f%%", level * 100);
	ImVec2 text_size = ImGui::CalcTextSize(pct);
	dl->AddText(ImVec2(gauge_center.x - text_size.x * 0.5f, gauge_center.y - text_size.y * 0.5f),
		IM_COL32(255, 255, 255, 255), pct);

	// Glass highlight
	dl->AddCircle(gauge_center, gauge_radius, IM_COL32(255, 255, 255, 40), 64, 2 * scale);

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE: DNA Helix
// ============================================================
static void ShowUsecase_DNAHelix()
{
	ImGui::TextWrapped(
		"DNA helix using iam_tween_float with repeat policy for continuous rotation "
		"and ping-pong for pulsing nucleotides. Click to toggle speed!");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static bool fast_mode = false;

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 180 * scale);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Handle click to toggle speed
	ImVec2 mouse = ImGui::GetMousePos();
	if (ImGui::IsMouseClicked(0) &&
		mouse.x >= canvas_pos.x && mouse.x <= canvas_pos.x + canvas_size.x &&
		mouse.y >= canvas_pos.y && mouse.y <= canvas_pos.y + canvas_size.y)
	{
		fast_mode = !fast_mode;
	}

	// Background
	dl->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(15, 20, 35, 255), 8 * scale);

	ImVec2 center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

	// Helix dimensions with oscillating width for breathing effect
	float helix_breathe = 1.0f + 0.05f * iam_oscillate(ImGui::GetID("helix_breathe"),
		1.0f, 0.5f, iam_wave_sine, 0.0f, dt);
	float helix_width = 50 * scale * helix_breathe;
	float helix_height = 140 * scale;

	// Rotation using accumulated time for continuous spinning
	static float dna_rotation = 0.0f;
	float rotation_speed = fast_mode ? 2.0f : 1.0f;
	dna_rotation += dt * rotation_speed;
	if (dna_rotation > IAM_2PI) dna_rotation -= IAM_2PI;
	float rotation = dna_rotation;

	// Speed transition effect
	float speed_indicator = iam_tween_float(ImGui::GetID("speed_ind"), ImHashStr("spd"),
		fast_mode ? 1.0f : 0.0f, 0.3f,
		iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

	// Base pair colors
	ImU32 colors1[] = { IM_COL32(255, 100, 100, 255), IM_COL32(100, 255, 100, 255) };  // A-T
	ImU32 colors2[] = { IM_COL32(100, 100, 255, 255), IM_COL32(255, 255, 100, 255) };  // G-C

	struct HelixPoint {
		ImVec2 pos;
		float z;
		ImU32 color;
		int index;
	};

	HelixPoint points[80];
	int point_count = 0;

	for (int i = 0; i < 20; i++)
	{
		float t = (float)i / 20.0f;
		float y = center.y - helix_height * 0.5f + t * helix_height;
		float angle = rotation + t * IAM_2PI * 2.0f;

		float x1 = center.x + cosf(angle) * helix_width;
		float z1 = sinf(angle);

		float x2 = center.x + cosf(angle + IAM_PI) * helix_width;
		float z2 = sinf(angle + IAM_PI);

		ImU32* pair_colors = (i % 2 == 0) ? colors1 : colors2;

		points[point_count++] = { ImVec2(x1, y), z1, pair_colors[0], i * 2 };
		points[point_count++] = { ImVec2(x2, y), z2, pair_colors[1], i * 2 + 1 };
	}

	// Depth sort
	for (int i = 0; i < point_count - 1; i++)
	{
		for (int j = 0; j < point_count - i - 1; j++)
		{
			if (points[j].z > points[j + 1].z)
			{
				HelixPoint temp = points[j];
				points[j] = points[j + 1];
				points[j + 1] = temp;
			}
		}
	}

	// Draw base pair connections
	for (int i = 0; i < 20; i++)
	{
		float t = (float)i / 20.0f;
		float y = center.y - helix_height * 0.5f + t * helix_height;
		float angle = rotation + t * IAM_2PI * 2.0f;

		float x1 = center.x + cosf(angle) * helix_width;
		float x2 = center.x + cosf(angle + IAM_PI) * helix_width;

		float z_mid = (sinf(angle) + sinf(angle + IAM_PI)) * 0.5f;
		int alpha = (int)(150 + z_mid * 50);
		dl->AddLine(ImVec2(x1, y), ImVec2(x2, y), IM_COL32(100, 100, 150, alpha), 2 * scale);
	}

	// Draw backbone strands
	for (int strand = 0; strand < 2; strand++)
	{
		float phase = strand * IAM_PI;
		ImVec2 prev_pos;

		for (int i = 0; i <= 40; i++)
		{
			float t = (float)i / 40.0f;
			float y = center.y - helix_height * 0.5f + t * helix_height;
			float angle = rotation + t * IAM_2PI * 2.0f + phase;

			float x = center.x + cosf(angle) * helix_width;
			float z = sinf(angle);

			ImVec2 pos(x, y);
			int alpha = (int)(150 + z * 100);
			ImU32 strand_color = strand == 0 ?
				IM_COL32(255, 150, 100, alpha) : IM_COL32(100, 200, 255, alpha);

			if (i > 0)
				dl->AddLine(prev_pos, pos, strand_color, (2 + z) * scale);
			prev_pos = pos;
		}
	}

	// Draw nucleotides with individual pulse animations
	for (int i = 0; i < point_count; i++)
	{
		// Each nucleotide pulses at slightly different rate using oscillate
		float pulse_osc = iam_oscillate(ImGui::GetID("nucleotide") + points[i].index,
			0.15f, 2.0f + (points[i].index % 5) * 0.2f, iam_wave_sine, (float)(points[i].index) * 0.1f, dt);
		float pulse = 1.0f + pulse_osc;

		float size = (4 + points[i].z * 2) * scale * pulse;
		int alpha = (int)(200 + points[i].z * 55);
		ImU32 col = (points[i].color & 0x00FFFFFF) | ((ImU32)alpha << 24);
		dl->AddCircleFilled(points[i].pos, size, col);
	}

	// Speed indicator text
	const char* speed_text = fast_mode ? "FAST - Click to slow" : "Click to speed up";
	ImVec2 text_size = ImGui::CalcTextSize(speed_text);
	int text_alpha = (int)(150 + speed_indicator * 50);
	dl->AddText(ImVec2(canvas_pos.x + (canvas_size.x - text_size.x) * 0.5f,
		canvas_pos.y + canvas_size.y - 20 * scale),
		IM_COL32(200, 200, 200, text_alpha), speed_text);

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE: Heartbeat Monitor
// ============================================================
static void ShowUsecase_HeartbeatMonitor()
{
	ImGui::TextWrapped(
		"ECG monitor using iam_tween_float with repeat policy for heartbeat cycle "
		"and crossfade for smooth BPM transitions. Heart icon pulses with each beat.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	static int target_bpm = 72;
	static bool flatline = false;

	ImGui::SliderInt("BPM##Heartbeat", &target_bpm, 40, 180);
	ImGui::SameLine();
	if (ImGui::Button(flatline ? "Revive" : "Flatline"))
		flatline = !flatline;

	// Smooth BPM transition using tween
	float current_bpm = iam_tween_float(ImGui::GetID("bpm_tween"), ImHashStr("bpm"),
		flatline ? 0.0f : (float)target_bpm, 0.5f,
		iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 120 * scale);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Monitor background
	dl->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(10, 20, 15, 255), 8 * scale);

	// Grid lines with oscillating opacity
	float grid_alpha = 0.5f + 0.5f * iam_oscillate(ImGui::GetID("grid_pulse"),
		1.0f, 0.5f, iam_wave_sine, 0.0f, dt);

	for (int i = 0; i <= 10; i++)
	{
		float x = canvas_pos.x + (i / 10.0f) * canvas_size.x;
		dl->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y),
			IM_COL32(0, 60, 30, (int)(80 + grid_alpha * 40)), 1);
	}
	for (int i = 0; i <= 4; i++)
	{
		float y = canvas_pos.y + (i / 4.0f) * canvas_size.y;
		dl->AddLine(ImVec2(canvas_pos.x, y), ImVec2(canvas_pos.x + canvas_size.x, y),
			IM_COL32(0, 60, 30, (int)(80 + grid_alpha * 40)), 1);
	}

	// Beat phase using accumulated time for continuous heartbeat cycle
	static float beat_time = 0.0f;
	float beat_period = current_bpm > 1.0f ? 60.0f / current_bpm : 100.0f;
	beat_time += dt;
	float beat_phase = fmodf(beat_time / beat_period, 1.0f);

	float center_y = canvas_pos.y + canvas_size.y * 0.5f;

	// ECG waveform generator
	auto ecg_sample = [](float t) -> float {
		t = fmodf(t, 1.0f);
		if (t < 0.1f) return 0.15f * sinf(t / 0.1f * IAM_PI);
		if (t < 0.15f) return 0.0f;
		if (t < 0.18f) return -0.1f * sinf((t - 0.15f) / 0.03f * IAM_PI);
		if (t < 0.22f) return 1.0f * sinf((t - 0.18f) / 0.04f * IAM_PI);
		if (t < 0.26f) return -0.25f * sinf((t - 0.22f) / 0.04f * IAM_PI);
		if (t < 0.35f) return 0.0f;
		if (t < 0.5f) return 0.25f * sinf((t - 0.35f) / 0.15f * IAM_PI);
		return 0.0f;
	};

	// Waveform amplitude based on BPM (flatline when 0)
	float amplitude_scale = iam_tween_float(ImGui::GetID("amplitude"), ImHashStr("amp"),
		flatline ? 0.0f : 1.0f, 0.5f,
		iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

	// Draw waveform using history buffer simulated with phase offsets
	ImVec2 prev_point;
	int num_samples = 60;

	for (int i = 0; i <= num_samples; i++)
	{
		float x = canvas_pos.x + (float)i / num_samples * canvas_size.x;

		// Each sample represents a point in past time
		float sample_phase = beat_phase - (float)(num_samples - i) / num_samples * 2.0f;
		sample_phase = fmodf(sample_phase + 10.0f, 1.0f);

		float amplitude = ecg_sample(sample_phase) * 40 * scale * amplitude_scale;
		float y = center_y - amplitude;

		ImVec2 point(x, y);

		if (i > 0)
		{
			// Line glow intensity using oscillate
			float glow = 0.5f + 0.5f * iam_oscillate(ImGui::GetID("line_glow"),
				1.0f, 1.2f, iam_wave_sine, 0.0f, dt);

			dl->AddLine(prev_point, point, IM_COL32(0, 255, 100, (int)(30 + glow * 30)), 6 * scale);
			dl->AddLine(prev_point, point, IM_COL32(0, 255, 100, (int)(80 + glow * 40)), 3 * scale);
			dl->AddLine(prev_point, point, IM_COL32(100, 255, 150, 255), 2 * scale);
		}

		prev_point = point;
	}

	// BPM display with smooth update
	char bpm_text[16];
	snprintf(bpm_text, sizeof(bpm_text), "%.0f BPM", current_bpm);
	dl->AddText(ImVec2(canvas_pos.x + 10 * scale, canvas_pos.y + 10 * scale),
		IM_COL32(0, 255, 100, 255), bpm_text);

	// Heart icon with oscillating pulse tied to beat
	float heart_pulse_osc = iam_oscillate(ImGui::GetID("heart_pulse"),
		0.15f, current_bpm / 60.0f, iam_wave_sine, 0.0f, dt);
	float heart_pulse = 1.0f + fabsf(heart_pulse_osc);

	// Heart visibility fades with flatline
	float heart_alpha = iam_tween_float(ImGui::GetID("heart_alpha"), ImHashStr("halp"),
		flatline ? 0.3f : 1.0f, 0.5f,
		iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

	ImVec2 heart_pos(canvas_pos.x + canvas_size.x - 30 * scale, canvas_pos.y + 25 * scale);
	float hs = 8 * scale * (flatline ? 1.0f : heart_pulse);
	ImU32 heart_col = IM_COL32(255, 50, 50, (int)(heart_alpha * 255));

	dl->AddCircleFilled(ImVec2(heart_pos.x - hs * 0.5f, heart_pos.y), hs * 0.6f, heart_col);
	dl->AddCircleFilled(ImVec2(heart_pos.x + hs * 0.5f, heart_pos.y), hs * 0.6f, heart_col);
	dl->AddTriangleFilled(
		ImVec2(heart_pos.x - hs, heart_pos.y + hs * 0.2f),
		ImVec2(heart_pos.x + hs, heart_pos.y + hs * 0.2f),
		ImVec2(heart_pos.x, heart_pos.y + hs * 1.2f),
		heart_col);

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE: Bouncing Balls Physics
// ============================================================
static void ShowUsecase_BouncingBalls()
{
	ImGui::TextWrapped(
		"Physics simulation with iam_tween_float for spawn animations, "
		"squash/stretch effects, and smooth position interpolation. Click to spawn!");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	struct Ball {
		ImVec2 pos;
		ImVec2 vel;
		float radius;
		ImU32 color;
		bool active;
		float spawn_time;
		float squash;  // For squash/stretch on bounce
	};

	static Ball balls[8];
	static bool initialized = false;
	static int spawn_index = 0;

	if (!initialized)
	{
		for (int i = 0; i < 8; i++)
			balls[i].active = false;
		balls[0] = { ImVec2(100, 50), ImVec2(80, 0), 15, IM_COL32(255, 100, 100, 255), true, 1.0f, 0.0f };
		balls[1] = { ImVec2(200, 30), ImVec2(-60, 20), 12, IM_COL32(100, 255, 100, 255), true, 1.0f, 0.0f };
		balls[2] = { ImVec2(300, 60), ImVec2(40, -30), 18, IM_COL32(100, 100, 255, 255), true, 1.0f, 0.0f };
		spawn_index = 3;
		initialized = true;
	}

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 150 * scale);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Background
	dl->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(20, 25, 35, 255), 8 * scale);

	// Floor with subtle glow using oscillate
	float floor_glow = 0.5f + 0.5f * iam_oscillate(ImGui::GetID("floor_glow"),
		1.0f, 0.7f, iam_wave_sine, 0.0f, dt);
	dl->AddRectFilled(
		ImVec2(canvas_pos.x, canvas_pos.y + canvas_size.y - 5 * scale),
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(60 + (int)(floor_glow * 20), 70, 90, 255), 0, ImDrawFlags_RoundCornersBottom);

	// Spawn ball on click
	ImVec2 mouse = ImGui::GetMousePos();
	if (ImGui::IsMouseClicked(0) &&
		mouse.x >= canvas_pos.x && mouse.x <= canvas_pos.x + canvas_size.x &&
		mouse.y >= canvas_pos.y && mouse.y <= canvas_pos.y + canvas_size.y)
	{
		ImU32 colors[] = {
			IM_COL32(255, 100, 100, 255), IM_COL32(100, 255, 100, 255),
			IM_COL32(100, 100, 255, 255), IM_COL32(255, 255, 100, 255),
			IM_COL32(255, 100, 255, 255), IM_COL32(100, 255, 255, 255)
		};
		int idx = spawn_index % 8;
		float r = 12 + (spawn_index % 4) * 3;
		balls[idx] = {
			ImVec2(mouse.x - canvas_pos.x, mouse.y - canvas_pos.y),
			ImVec2((float)((spawn_index * 47) % 200) - 100, 0),
			r * scale,
			colors[spawn_index % 6],
			true,
			0.0f,  // Just spawned
			0.0f
		};
		spawn_index++;
	}

	float gravity = 400 * scale;
	float bounce_factor = 0.75f;
	float friction = 0.99f;
	float floor_y = canvas_size.y - 5 * scale;

	// Update and draw balls
	for (int i = 0; i < 8; i++)
	{
		if (!balls[i].active) continue;

		Ball& b = balls[i];

		// Spawn animation using tween
		float spawn_scale = iam_tween_float(ImGui::GetID("ball_spawn") + i, ImHashStr("spn"),
			1.0f, 0.3f,
			iam_ease_preset(iam_ease_out_back), iam_policy_crossfade, dt);
		b.spawn_time += dt;

		// Squash effect decay
		if (b.squash > 0.01f)
		{
			b.squash = iam_tween_float(ImGui::GetID("ball_squash") + i, ImHashStr("sqsh"),
				0.0f, 0.15f,
				iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);
		}

		// Physics update
		b.vel.y += gravity * dt;
		b.vel.x *= friction;
		b.pos.x += b.vel.x * dt;
		b.pos.y += b.vel.y * dt;

		// Wall bounces
		if (b.pos.x - b.radius < 0)
		{
			b.pos.x = b.radius;
			b.vel.x = -b.vel.x * bounce_factor;
		}
		if (b.pos.x + b.radius > canvas_size.x)
		{
			b.pos.x = canvas_size.x - b.radius;
			b.vel.x = -b.vel.x * bounce_factor;
		}

		// Floor bounce with squash effect
		if (b.pos.y + b.radius > floor_y)
		{
			b.pos.y = floor_y - b.radius;
			float impact = fabsf(b.vel.y);
			b.vel.y = -b.vel.y * bounce_factor;

			// Trigger squash based on impact velocity
			if (impact > 50)
				b.squash = ImMin(0.4f, impact / 500.0f);

			if (fabsf(b.vel.y) < 10 && fabsf(b.vel.x) < 10)
			{
				b.vel.y = 0;
				b.vel.x = 0;
			}
		}

		// Smooth rendered position using tween
		float render_x = iam_tween_float(ImGui::GetID("ball_rx") + i, ImHashStr("rx"),
			b.pos.x, 0.05f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);
		float render_y = iam_tween_float(ImGui::GetID("ball_ry") + i, ImHashStr("ry"),
			b.pos.y, 0.05f, iam_ease_preset(iam_ease_out_quad), iam_policy_crossfade, dt);

		// Calculate squash/stretch
		float squash_x = 1.0f + b.squash * 0.5f;
		float squash_y = 1.0f - b.squash * 0.3f;

		// Shadow (size based on height)
		float height_factor = 1.0f - (render_y / floor_y);
		float shadow_scale = 0.6f + height_factor * 0.4f;
		dl->AddEllipseFilled(
			ImVec2(canvas_pos.x + render_x, canvas_pos.y + floor_y - 2 * scale),
			ImVec2(b.radius * shadow_scale * squash_x, b.radius * 0.25f * squash_x),
			IM_COL32(0, 0, 0, (int)(40 + height_factor * 40)));

		// Ball with squash/stretch
		ImVec2 screen_pos(canvas_pos.x + render_x, canvas_pos.y + render_y);
		float actual_radius = b.radius * spawn_scale;

		// Draw as ellipse for squash effect
		dl->AddEllipseFilled(screen_pos,
			ImVec2(actual_radius * squash_x, actual_radius * squash_y), b.color);

		// Highlight
		dl->AddCircleFilled(
			ImVec2(screen_pos.x - actual_radius * 0.25f, screen_pos.y - actual_radius * 0.25f),
			actual_radius * 0.25f, IM_COL32(255, 255, 255, 120));
	}

	// Hint with pulsing alpha using oscillate
	float hint_alpha = 0.5f + 0.5f * iam_oscillate(ImGui::GetID("hint_alpha"),
		1.0f, 1.0f, iam_wave_sine, 0.0f, dt);
	const char* hint = "Click to spawn balls!";
	ImVec2 hint_size = ImGui::CalcTextSize(hint);
	dl->AddText(ImVec2(canvas_pos.x + (canvas_size.x - hint_size.x) * 0.5f, canvas_pos.y + 5 * scale),
		IM_COL32(100, 110, 130, (int)(150 + hint_alpha * 50)), hint);

	ImGui::Dummy(canvas_size);
}

// ============================================================
// USECASE: Neon Sign
// ============================================================
static void ShowUsecase_NeonSign()
{
	ImGui::TextWrapped(
		"Flickering neon sign effect with glow animation. "
		"Uses iam_oscillate for intensity pulse, power surge, and bloom effects.");

	float dt = GetUsecaseDeltaTime();
	float scale = ImGui::GetIO().FontGlobalScale;

	// ImAnim: Intensity pulse using oscillate
	float pulse = 0.5f + 0.5f * iam_oscillate(ImGui::GetID("neon_pulse"),
		1.0f, 10.0f, iam_wave_sine, 0.0f, dt);

	// ImAnim: Power surge effect using oscillate (slow)
	float surge_t = 0.5f + 0.5f * iam_oscillate(ImGui::GetID("neon_surge"),
		1.0f, 0.5f, iam_wave_sine, 0.0f, dt);
	float surge = surge_t * 0.3f;

	// ImAnim: Flicker cycle using sawtooth oscillate for continuous 0-10 cycling
	static float flicker_time = 0.0f;
	flicker_time += dt;
	float flicker_cycle = fmodf(flicker_time * 6.67f, 10.0f);  // 1.5s period for 0-10

	// Derive flicker state from cycle
	int flicker_state = (int)flicker_cycle;
	bool is_on = flicker_state < 8;  // Occasionally flicker off

	// ImAnim: Glow bloom radius using oscillate
	float bloom_factor = 1.0f + 0.2f * iam_oscillate(ImGui::GetID("neon_bloom"),
		1.0f, 3.3f, iam_wave_sine, 0.0f, dt);

	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, 120 * scale);
	ImDrawList* dl = ImGui::GetWindowDrawList();

	// Dark background (like night)
	dl->AddRectFilled(canvas_pos,
		ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
		IM_COL32(15, 15, 25, 255), 8 * scale);

	ImVec2 center(canvas_pos.x + canvas_size.x * 0.5f, canvas_pos.y + canvas_size.y * 0.5f);

	// Neon text "OPEN"
	const char* text = "OPEN";
	float font_scale = 3.0f;
	ImVec2 text_size = ImGui::CalcTextSize(text);
	text_size.x *= font_scale;
	text_size.y *= font_scale;

	ImVec2 text_pos(center.x - text_size.x * 0.5f, center.y - text_size.y * 0.5f);

	// Calculate final intensity using ImAnim tweens
	float base_intensity = is_on ? (0.8f + 0.2f * pulse) : 0.2f;
	float intensity = base_intensity + surge;

	ImU32 neon_color = IM_COL32(255, 50, 100, (int)(intensity * 255));
	ImU32 glow_color = IM_COL32(255, 50, 100, (int)(intensity * 60));

	// Multiple glow layers
	for (int layer = 4; layer >= 0; layer--)
	{
		float glow_offset = layer * 4 * scale;
		int glow_alpha = (int)(intensity * 40 / (layer + 1));

		// Draw glowing background for each character
		for (int c = 0; c < 4; c++)
		{
			char ch[2] = { text[c], 0 };
			ImVec2 char_size = ImGui::CalcTextSize(ch);
			float char_x = text_pos.x + c * char_size.x * font_scale;

			if (layer == 0)
			{
				// Main text
				for (float ox = -1; ox <= 1; ox += 0.5f)
				{
					for (float oy = -1; oy <= 1; oy += 0.5f)
					{
						ImGui::SetWindowFontScale(font_scale);
						dl->AddText(ImVec2(char_x + ox * scale, text_pos.y + oy * scale),
							glow_color, ch);
						ImGui::SetWindowFontScale(1.0f);
					}
				}
				ImGui::SetWindowFontScale(font_scale);
				dl->AddText(ImVec2(char_x, text_pos.y), neon_color, ch);
				ImGui::SetWindowFontScale(1.0f);
			}
		}
	}

	// Add bloom circles around letters for extra glow (using bloom_factor tween)
	for (int c = 0; c < 4; c++)
	{
		char ch[2] = { text[c], 0 };
		ImVec2 char_size = ImGui::CalcTextSize(ch);
		float char_x = text_pos.x + c * char_size.x * font_scale + char_size.x * font_scale * 0.5f;
		float char_y = text_pos.y + char_size.y * font_scale * 0.5f;

		float bloom_radius = 30 * scale * intensity * bloom_factor;
		dl->AddCircleFilled(ImVec2(char_x, char_y), bloom_radius,
			IM_COL32(255, 50, 100, (int)(intensity * 20)));
	}

	// Reflection on floor
	float reflect_y = canvas_pos.y + canvas_size.y - 15 * scale;
	for (int c = 0; c < 4; c++)
	{
		char ch[2] = { text[c], 0 };
		ImVec2 char_size = ImGui::CalcTextSize(ch);
		float char_x = text_pos.x + c * char_size.x * font_scale;

		ImGui::SetWindowFontScale(font_scale * 0.3f);
		dl->AddText(ImVec2(char_x + char_size.x * font_scale * 0.35f, reflect_y),
			IM_COL32(255, 50, 100, (int)(intensity * 40)), ch);
		ImGui::SetWindowFontScale(1.0f);
	}

	ImGui::Dummy(canvas_size);
}

// ============================================================
// MAIN USECASE WINDOW
// ============================================================
void ImAnimUsecaseWindow()
{
	ImGui::SetNextWindowSize(ImVec2(700, 800), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("ImAnim Usecases - UI Mockups"))
	{
		ImGui::End();
		return;
	}

	ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.9f, 1.0f), "Real-World UI Animation Patterns");
	ImGui::TextWrapped(
		"This window showcases practical animation patterns commonly used in modern UI design. "
		"Each example demonstrates how ImAnim's tween API can create polished, responsive interfaces.");
	ImGui::Separator();
	ImGui::Spacing();

	// Buttons for expand/collapse all
	static int open_all = 0;
	if (ImGui::Button("Expand All")) open_all = 1;
	ImGui::SameLine();
	if (ImGui::Button("Collapse All")) open_all = -1;
	ImGui::Spacing();

	// Reset after one frame
	int current_open_all = open_all;
	open_all = 0;

	// Helper macro for usecase items
	#define USECASE_ITEM(name, func) \
		if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always); \
		if (ImGui::TreeNode(name)) { func(); ImGui::TreePop(); }

	// ========================================
	// BUTTONS & INDICATORS
	// ========================================
	if ( current_open_all != 0 ) ImGui::SetNextItemOpen( current_open_all > 0, ImGuiCond_Always );
	if (ImGui::CollapsingHeader("Buttons & Indicators"))
	{
		ImGui::Indent();
		USECASE_ITEM("Animated Buttons", ShowUsecase_AnimatedButton)
		USECASE_ITEM("Progress Bar", ShowUsecase_ProgressBar)
		USECASE_ITEM("Pulse Badge", ShowUsecase_PulseBadge)
		USECASE_ITEM("Pending Button", ShowUsecase_PendingButton)
		USECASE_ITEM("Animated Slider", ShowUsecase_AnimatedSlider)
		ImGui::Unindent();
	}

	// ========================================
	// NAVIGATION & MENUS
	// ========================================
	if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always);
	if (ImGui::CollapsingHeader("Navigation & Menus"))
	{
		ImGui::Indent();
		USECASE_ITEM("Sidebar Navigation", ShowUsecase_SidebarNavigation)
		USECASE_ITEM("Tab Transitions", ShowUsecase_TabTransitions)
		USECASE_ITEM("Accordion Sections", ShowUsecase_Accordion)
		USECASE_ITEM("FAB Menu", ShowUsecase_FABMenu)
		USECASE_ITEM("Hamburger Menu Morph", ShowUsecase_HamburgerMorph)
		USECASE_ITEM("Breadcrumb Navigation", ShowUsecase_Breadcrumb)
		USECASE_ITEM("Slide-in Drawer", ShowUsecase_SlideDrawer)
		USECASE_ITEM("Search Bar Expansion", ShowUsecase_SearchExpand)
		ImGui::Unindent();
	}

	// ========================================
	// DIALOGS & MODALS
	// ========================================
	if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always);
	if (ImGui::CollapsingHeader("Dialogs & Modals"))
	{
		ImGui::Indent();
		USECASE_ITEM("Modal Dialog", ShowUsecase_ModalDialog)
		USECASE_ITEM("Toast Notifications", ShowUsecase_ToastNotifications)
		USECASE_ITEM("Tooltip Animation", ShowUsecase_TooltipAnimation)
		USECASE_ITEM("Floating Action Button", ShowUsecase_FloatingActionButton)
		USECASE_ITEM("Popover Menu", ShowUsecase_PopoverMenu)
		USECASE_ITEM("Alert Banner", ShowUsecase_AlertBanner)
		ImGui::Unindent();
	}

	// ========================================
	// LOADING & PROGRESS
	// ========================================
	if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always);
	if (ImGui::CollapsingHeader("Loading & Progress"))
	{
		ImGui::Indent();
		USECASE_ITEM("Loading Spinners", ShowUsecase_LoadingSpinners)
		USECASE_ITEM("Skeleton Loading", ShowUsecase_SkeletonLoading)
		USECASE_ITEM("Circular Progress Ring", ShowUsecase_CircularProgress)
		USECASE_ITEM("Countdown Timer", ShowUsecase_CountdownTimer)
		USECASE_ITEM("Stepper / Timeline", ShowUsecase_Stepper)
		ImGui::Unindent();
	}

	// ========================================
	// INPUT CONTROLS
	// ========================================
	if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always);
	if (ImGui::CollapsingHeader("Input Controls"))
	{
		ImGui::Indent();
		USECASE_ITEM("Toggle Switch", ShowUsecase_ToggleSwitch)
		USECASE_ITEM("Animated Checkbox", ShowUsecase_AnimatedCheckbox)
		USECASE_ITEM("Animated Radio Buttons", ShowUsecase_AnimatedRadio)
		USECASE_ITEM("Rating Stars", ShowUsecase_RatingStars)
		USECASE_ITEM("Color Swatches", ShowUsecase_ColorSwatches)
		ImGui::Unindent();
	}

	// ========================================
	// CARDS & CONTENT
	// ========================================
	if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always);
	if (ImGui::CollapsingHeader("Cards & Content"))
	{
		ImGui::Indent();
		USECASE_ITEM("Card Hover Effects", ShowUsecase_CardHover)
		USECASE_ITEM("Flip Card", ShowUsecase_FlipCard)
		USECASE_ITEM("Swipe Cards", ShowUsecase_SwipeCards)
		USECASE_ITEM("Carousel / Image Slider", ShowUsecase_Carousel)
		USECASE_ITEM("Expandable List Item", ShowUsecase_ExpandableListItem)
		USECASE_ITEM("Image Gallery Grid", ShowUsecase_ImageGalleryGrid)
		ImGui::Unindent();
	}

	// ========================================
	// TEXT & DISPLAY
	// ========================================
	if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always);
	if (ImGui::CollapsingHeader("Text & Display"))
	{
		ImGui::Indent();
		USECASE_ITEM("Animated Counter", ShowUsecase_AnimatedCounter)
		USECASE_ITEM("Typewriter Text", ShowUsecase_TypewriterText)
		USECASE_ITEM("Typing Text", ShowUsecase_TypingText)
		USECASE_ITEM("Animated Tags", ShowUsecase_AnimatedTags)
		USECASE_ITEM("Animated Badge Counter", ShowUsecase_AnimatedBadgeCounter)
		USECASE_ITEM("Magnetic Cursor", ShowUsecase_MagneticCursor)
		ImGui::Unindent();
	}

	// ========================================
	// VISUAL EFFECTS
	// ========================================
	if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always);
	if (ImGui::CollapsingHeader("Visual Effects"))
	{
		ImGui::Indent();
		USECASE_ITEM("Gradient Animation", ShowUsecase_GradientAnimation)
		USECASE_ITEM("Ripple Effect", ShowUsecase_RippleEffect)
		USECASE_ITEM("Shake Animation (Form Validation)", ShowUsecase_ShakeAnimation)
		USECASE_ITEM("Avatar Stack", ShowUsecase_AvatarStack)
		USECASE_ITEM("Particle Burst", ShowUsecase_ParticleBurst)
		USECASE_ITEM("Glowing Border", ShowUsecase_GlowingBorder)
		ImGui::Unindent();
	}

	// ========================================
	// VFX / GAME / TIMELINE TOOLS
	// ========================================
	if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always);
	if (ImGui::CollapsingHeader("VFX / Game / Timeline Tools"))
	{
		ImGui::Indent();
		USECASE_ITEM("Timeline Scrubber", ShowUsecase_TimelineScrubber)
		USECASE_ITEM("Keyframe Curve Editor", ShowUsecase_KeyframeCurve)
		USECASE_ITEM("Layer Stack", ShowUsecase_LayerStack)
		USECASE_ITEM("Node Connector", ShowUsecase_NodeConnector)
		USECASE_ITEM("Animated Graph Node", ShowUsecase_AnimatedGraphNode)
		USECASE_ITEM("Playback Controls", ShowUsecase_PlaybackControls)
		ImGui::Unindent();
	}

	// ========================================
	// DATA VISUALIZATION
	// ========================================
	if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always);
	if (ImGui::CollapsingHeader("Data Visualization"))
	{
		ImGui::Indent();
		USECASE_ITEM("Animated Bar Chart", ShowUsecase_AnimatedBarChart)
		USECASE_ITEM("Animated Line Chart", ShowUsecase_AnimatedLineChart)
		USECASE_ITEM("Animated Pie Chart", ShowUsecase_AnimatedPieChart)
		USECASE_ITEM("Area Chart", ShowUsecase_AreaChart)
		USECASE_ITEM("Heat Map", ShowUsecase_HeatMap)
		USECASE_ITEM("Sparkline", ShowUsecase_Sparkline)
		USECASE_ITEM("Radar Chart", ShowUsecase_RadarChart)
		USECASE_ITEM("Gauge Meter", ShowUsecase_GaugeMeter)
		USECASE_ITEM("Sankey Flow", ShowUsecase_SankeyFlow)
		USECASE_ITEM("Funnel Chart", ShowUsecase_FunnelChart)
		USECASE_ITEM("Scatter Plot", ShowUsecase_ScatterPlot)
		USECASE_ITEM("Progress Dashboard", ShowUsecase_ProgressDashboard)
		ImGui::Unindent();
	}

	// ========================================
	// SURPRISE!
	// ========================================
	if (current_open_all != 0) ImGui::SetNextItemOpen(current_open_all > 0, ImGuiCond_Always);
	if (ImGui::CollapsingHeader("Surprise!"))
	{
		ImGui::Indent();
		USECASE_ITEM("Orbiting Planets", ShowUsecase_OrbitingPlanets)
		USECASE_ITEM("Liquid Fill Gauge", ShowUsecase_LiquidFillGauge)
		USECASE_ITEM("DNA Helix", ShowUsecase_DNAHelix)
		USECASE_ITEM("Heartbeat Monitor", ShowUsecase_HeartbeatMonitor)
		USECASE_ITEM("Bouncing Balls Physics", ShowUsecase_BouncingBalls)
		USECASE_ITEM("Neon Sign", ShowUsecase_NeonSign)
		ImGui::Unindent();
	}

	#undef USECASE_ITEM

	ImGui::End();
}