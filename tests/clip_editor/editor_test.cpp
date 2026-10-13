// Headless test of iam_show_clip_editor: drives it with simulated input and checks the clip data.
// Writes the "Copy C++" output to exported_clip*.inl in the current directory (compiled by compile_export.cpp).
#include "im_anim.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <string.h>
#include <string>

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { g_fail++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } else { printf("ok   "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static std::string g_clipboard;
static void set_clip(ImGuiContext*, const char* t) { g_clipboard = t ? t : ""; }
static const char* get_clip(ImGuiContext*) { return g_clipboard.c_str(); }

static ImGuiID CLIP, INST, CH_A, CH_B, CH_C;
static char g_title[64];
static ImGuiID g_probe_id = 0;     // id to compute inside the editor window
static const char* g_probe_label = nullptr;
static bool g_modified = false;

static void frame() {
	ImGuiIO& io = ImGui::GetIO();
	ImGui::NewFrame();
	iam_update_begin_frame();
	iam_clip_update(io.DeltaTime);
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(1200, 600), ImGuiCond_Always);
	g_modified = iam_show_clip_editor(CLIP, nullptr, INST);
	if (g_probe_label) {
		ImGui::Begin(g_title);
		ImGui::PushID((int)CLIP);
		g_probe_id = ImGui::GetID(g_probe_label);
		ImGui::PopID();
		ImGui::End();
	}
	ImGui::Render();
}
static void frames(int n) { for (int i = 0; i < n; i++) frame(); }
static void mouse_to(ImVec2 p) { ImGui::GetIO().AddMousePosEvent(p.x, p.y); frame(); }
static void mouse_btn(int b, bool down) { ImGui::GetIO().AddMouseButtonEvent(b, down); frame(); }
static void click(ImVec2 p, int b = 0) { mouse_to(p); mouse_btn(b, true); mouse_btn(b, false); }
static void key(ImGuiKey k, bool ctrl = false, bool shift = false) {
	ImGuiIO& io = ImGui::GetIO();
	if (ctrl) io.AddKeyEvent(ImGuiMod_Ctrl, true);
	if (shift) io.AddKeyEvent(ImGuiMod_Shift, true);
	io.AddKeyEvent(k, true); frame();
	io.AddKeyEvent(k, false);
	if (ctrl) io.AddKeyEvent(ImGuiMod_Ctrl, false);
	if (shift) io.AddKeyEvent(ImGuiMod_Shift, false);
	frame();
}

// Layout mirror of the editor (see editor_body)
struct layout { ImVec2 lanes_pos; float label_w, tl_x, tl_w, row_h, t0, len; };
static layout get_layout(float duration) {
	ImGuiContext& g = *GImGui;
	layout L = {};
	for (int i = 0; i < g.Windows.Size; i++)
		if (strstr(g.Windows[i]->Name, "##lanes")) { L.lanes_pos = g.Windows[i]->Pos; L.tl_w = g.Windows[i]->Size.x; }
	L.row_h = ImGui::GetFrameHeight();
	L.label_w = ImGui::CalcTextSize("0x00000000  vec4_rel").x + ImGui::GetFontSize() * 6.0f;
	L.tl_w = L.tl_w - L.label_w - ImGui::GetStyle().ScrollbarSize;
	L.tl_x = L.lanes_pos.x + L.label_w;
	float d = duration > 0 ? duration : 1.0f;
	L.t0 = -d * 0.02f; L.len = d * 1.06f;
	return L;
}
static ImVec2 key_pos(layout const& L, int row, float t) { return ImVec2(L.tl_x + (t - L.t0) / L.len * L.tl_w, L.lanes_pos.y + L.row_h * (row + 0.5f)); }

static std::string export_cpp() {
	// Find and click the "Copy C++" button by scanning the toolbar for its hover id
	g_probe_label = "Copy C++"; frame(); ImGuiID target = g_probe_id; g_probe_label = nullptr;
	ImGuiContext& g = *GImGui;
	for (float y = 30; y < 110; y += 6)
		for (float x = 15; x < 1200; x += 6) {
			mouse_to(ImVec2(x, y));
			if (g.HoveredId == target) {
				g_clipboard.clear();
				mouse_btn(0, true); mouse_btn(0, false);
				return g_clipboard;
			}
		}
	printf("Copy C++ button not found\n");
	return "";
}
static int count_occ(std::string const& s, const char* sub) { int n = 0; for (size_t p = s.find(sub); p != std::string::npos; p = s.find(sub, p + 1)) n++; return n; }

static int row_of(std::string const& code, const char* name) {
	// Track order = order of first appearance of a channel in the key lines ("\t.key_xxx(iam_id(\"name\")")
	int row = 0; std::string seen;
	size_t p = 0;
	while ((p = code.find("\t.key", p)) != std::string::npos) {
		size_t q = code.find("(iam_id(\"", p);
		size_t eol = code.find('\n', p);
		p = (eol == std::string::npos) ? code.size() : eol;
		if (q == std::string::npos || q > p) continue;
		size_t e = code.find('"', q + 9);
		std::string n = code.substr(q + 9, e - q - 9);
		if (seen.find("|" + n + "|") != std::string::npos) continue;
		if (n == name) return row;
		seen += "|" + n + "|"; row++;
	}
	return -1;
}

int main() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(1280, 720);
	io.DeltaTime = 1.0f / 60.0f;
	io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	ImGui::GetPlatformIO().Platform_SetClipboardTextFn = set_clip;
	ImGui::GetPlatformIO().Platform_GetClipboardTextFn = get_clip;

	CLIP = iam_id("test_clip"); INST = ImHashStr("inst");
	CH_A = iam_id("alpha"); CH_B = iam_id("pos"); CH_C = iam_id("tint");
	snprintf(g_title, sizeof(g_title), "###iam_clip_editor_%08X", CLIP);
	iam_clip::begin(CLIP)
		.key_float(CH_A, 0.0f, 0.0f, iam_ease_out_cubic)
		.key_float(CH_A, 1.0f, 1.0f)
		.key_vec2(CH_B, 0.0f, iam_v2(0, 0))
		.key_vec2(CH_B, 0.5f, iam_v2(10, 20), iam_ease_in_out_sine)
		.key_vec2(CH_B, 2.0f, iam_v2(100, 50))
		.key_color(CH_C, 0.0f, iam_v4(1, 0, 0, 1), iam_col_oklch)
		.key_color(CH_C, 2.0f, iam_v4(0, 0, 1, 1), iam_col_oklch)
		.end();
	iam_play(CLIP, INST).pause();
	frames(3);
	CHECK(iam_clip_duration(CLIP) == 2.0f, "initial duration 2.0 (got %.3f)", iam_clip_duration(CLIP));

	std::string code = export_cpp();
	CHECK(!code.empty(), "Copy C++ produced text (%d bytes)", (int)code.size());
	CHECK(count_occ(code, ".key_float(iam_id(\"alpha\")") == 2, "export has 2 alpha keys");
	CHECK(count_occ(code, ".key_vec2(iam_id(\"pos\")") == 3, "export has 3 pos keys");
	CHECK(count_occ(code, "iam_col_oklch") == 2, "export keeps color space");
	int const R_A = row_of(code, "alpha"), R_B = row_of(code, "pos"), R_C = row_of(code, "tint");

	// Drag alpha key at t=1.0 to the right by ~0.5s
	layout L = get_layout(2.0f);
	ImVec2 k = key_pos(L, R_A, 1.0f);
	mouse_to(k); mouse_btn(0, true);
	float px_per_s = L.tl_w / L.len;
	for (int i = 1; i <= 10; i++) mouse_to(ImVec2(k.x + px_per_s * 0.5f * i / 10.0f, k.y));
	mouse_btn(0, false);
	frames(2);
	code = export_cpp();
	CHECK(code.find(".key_float(iam_id(\"alpha\"), 1.5f, 1.0f") != std::string::npos, "dragged alpha key to 1.5s (snapped)");
	CHECK(iam_clip_duration(CLIP) == 2.0f, "duration still 2.0");

	// Drag past the end: duration grows
	L = get_layout(2.0f);
	k = key_pos(L, R_A, 1.5f);
	mouse_to(k); mouse_btn(0, true);
	for (int i = 1; i <= 10; i++) mouse_to(ImVec2(k.x + px_per_s * 1.0f * i / 10.0f, k.y));
	mouse_btn(0, false);
	CHECK(ImFabs(iam_clip_duration(CLIP) - 2.5f) < 1e-4f, "duration grows to 2.5 when dragging past end (got %.3f)", iam_clip_duration(CLIP));

	// Undo twice -> back to 1.0s, duration 2.0
	key(ImGuiKey_Z, true); key(ImGuiKey_Z, true);
	code = export_cpp();
	CHECK(code.find(".key_float(iam_id(\"alpha\"), 1.0f, 1.0f") != std::string::npos, "undo x2 restores alpha key at 1.0s");
	CHECK(iam_clip_duration(CLIP) == 2.0f, "undo restores duration 2.0");
	// Redo once -> 1.5s
	key(ImGuiKey_Y, true);
	code = export_cpp();
	CHECK(code.find(".key_float(iam_id(\"alpha\"), 1.5f, 1.0f") != std::string::npos, "redo moves it back to 1.5s");

	// Double-click on pos lane at t=1.25 -> new key, value on the existing curve
	L = get_layout(2.0f);
	ImVec2 dc = key_pos(L, R_B, 1.25f);
	mouse_to(dc); mouse_btn(0, true); mouse_btn(0, false); mouse_btn(0, true); mouse_btn(0, false);
	frames(2);
	code = export_cpp();
	CHECK(count_occ(code, ".key_vec2(iam_id(\"pos\")") == 4, "double-click added a pos key (now %d)", count_occ(code, ".key_vec2(iam_id(\"pos\")"));
	CHECK(code.find(".key_vec2(iam_id(\"pos\"), 1.25f") != std::string::npos, "new key at 1.25s");
	iam_instance inst(INST);

	// Delete selected (the new key) with Delete
	mouse_to(ImVec2(dc.x, dc.y));  // keep hovering editor so it stays focused
	key(ImGuiKey_Delete);
	code = export_cpp();
	CHECK(count_occ(code, ".key_vec2(iam_id(\"pos\")") == 3, "Delete removed the key");

	// Box select: drag from the middle of the tint lane (empty) to the left edge, then Shift+box to the right edge
	L = get_layout(2.0f);
	{
		ImVec2 mid = key_pos(L, R_C, 1.0f), left = key_pos(L, R_C, -0.035f), right = key_pos(L, R_C, 2.07f);
		mouse_to(ImVec2(mid.x, mid.y - L.row_h * 0.4f)); mouse_btn(0, true);
		for (int i = 1; i <= 8; i++) mouse_to(ImVec2(mid.x + (left.x - mid.x) * i / 8.0f, mid.y + L.row_h * 0.4f));
		mouse_btn(0, false);
		frames(30);  // avoid a double-click
		ImGui::GetIO().AddKeyEvent(ImGuiMod_Shift, true);
		mouse_to(ImVec2(mid.x + 20, mid.y - L.row_h * 0.4f)); mouse_btn(0, true);
		for (int i = 1; i <= 8; i++) mouse_to(ImVec2(mid.x + (right.x - mid.x) * i / 8.0f, mid.y + L.row_h * 0.4f));
		mouse_btn(0, false);
		ImGui::GetIO().AddKeyEvent(ImGuiMod_Shift, false);
		frame();
	}
	key(ImGuiKey_Delete);
	code = export_cpp();
	CHECK(count_occ(code, ".key_color(iam_id(\"tint\")") == 0, "box select + Delete removed both tint keys");
	key(ImGuiKey_Z, true);
	code = export_cpp();
	CHECK(count_occ(code, ".key_color(iam_id(\"tint\")") == 2, "undo restored tint keys");

	// Ctrl+A selects all; Delete removes all; undo
	key(ImGuiKey_A, true);
	key(ImGuiKey_Delete);
	code = export_cpp();
	CHECK(count_occ(code, ".key_") == 0, "Ctrl+A + Delete removed every key");
	CHECK(iam_clip_duration(CLIP) == 0.0f, "duration 0 with no keys");
	key(ImGuiKey_Z, true);
	CHECK(iam_clip_duration(CLIP) == 2.0f, "undo restored all keys (duration %.2f)", iam_clip_duration(CLIP));

	// Scrub: click on empty lane space at t=0.5 moves the paused preview
	L = get_layout(2.0f);
	click(key_pos(L, R_A, 0.75f));
	frames(1);
	CHECK(ImFabs(inst.time() - 0.75f) < 0.02f, "click on empty space moves playhead to 0.75 (got %.3f)", inst.time());
	float alpha = -1; inst.get_float(CH_A, &alpha);
	CHECK(alpha > 0.0f && alpha < 1.0f, "preview value updates on scrub (alpha=%.3f)", alpha);

	// Write the exported code into a compilable file
	FILE* f = fopen("exported_clip.inl", "wb"); if (f) { fputs(code.c_str(), f); fclose(f); }

	// Every key kind -> export -> must compile (compile_export.cpp)
	{
		static float const bz[4] = { 0.1f, 0.7f, 0.3f, 1.2f };
		iam_spring_params sp = { 1.0f, 180.0f, 12.0f, 0.5f };
		iam_vec2 const bump[] = { iam_v2(0, 0), iam_v2(0.2f, 0.0f), iam_v2(0.3f, 1.3f), iam_v2(0.5f, 1.2f), iam_v2(0.7f, 1.1f), iam_v2(0.8f, 1.0f), iam_v2(1, 1) };
		iam_ease_curve_set(iam_id("bump"), bump, 7);
		static float const steps4[4] = { 4.0f, 0.0f, 0.0f, 0.0f };
		iam_clip::begin(CLIP)
			.key_float(iam_id("curved"), 0.0f, 0.0f, iam_ease_curve, iam_ease_curve_ref(iam_id("bump")))
			.key_float(iam_id("curved"), 1.0f, 1.0f)
			.key_float(iam_id("stepped"), 0.0f, 0.0f, iam_ease_steps, steps4)
			.key_float(iam_id("stepped"), 1.0f, 1.0f)
			.key_float(CH_A, 0.0f, 0.0f, iam_ease_cubic_bezier, bz)
			.key_float_spring(CH_A, 0.5f, 1.0f, sp)
			.key_float(CH_A, 1.0f, 0.25f)
			.key_int(iam_id("count"), 0.0f, 0, iam_ease_steps)
			.key_int(iam_id("count"), 1.0f, 10)
			.key_vec4(iam_id("rect"), 0.0f, iam_v4(0, 0, 10, 10))
			.key_vec4(iam_id("rect"), 1.0f, iam_v4(5, 5, 20, 20), iam_ease_out_back)
			.key_float_rel(iam_id("frel"), 0.0f, 0.5f, 10.0f, iam_anchor_window, 1)
			.key_float_rel(iam_id("frel"), 1.0f, 0.25f, -4.0f, iam_anchor_window, 1)
			.key_vec2_rel(iam_id("v2rel"), 0.0f, iam_v2(0.5f, 0.5f), iam_v2(1, 2), iam_anchor_viewport)
			.key_vec4_rel(iam_id("v4rel"), 0.0f, iam_v4(0.1f, 0.2f, 0.3f, 0.4f), iam_v4(1, 2, 3, 4), iam_anchor_last_item)
			.key_color_rel(iam_id("crel"), 0.0f, iam_v4(0.1f, 0.2f, 0.3f, 1), iam_v4(0, 0, 0, 0), iam_col_hsv, iam_anchor_window_content)
			.key_color(CH_C, 0.0f, iam_v4(1, 1, 1, 1), iam_col_srgb_linear, iam_ease_in_expo)
			.key_float_var(iam_id("varied"), 0.0f, 0.0f, iam_varf_inc(5.0f))
			.key_float(iam_id("varied"), 1.0f, 1.0f)
			.key_vec2_var(iam_id("varied2"), 0.0f, iam_v2(0, 0), iam_varv2_rand(3.0f, 4.0f))
			.key_vec2(iam_id("varied2"), 1.0f, iam_v2(1, 1))
			.set_delay(0.125f)
			.set_loop(true, iam_dir_alternate, 3)
			.set_loop_delay(0.5f)
			.end();
		frames(2);
		std::string all = export_cpp();
		CHECK(all.find("[4] = { 0.1f, 0.7f, 0.3f, 1.2f }") != std::string::npos, "export bezier array");
		CHECK(all.find(".key_float_spring(iam_id(\"alpha\"), 0.5f, 1.0f, iam_spring_params{ 1.0f, 180.0f, 12.0f, 0.5f })") != std::string::npos, "export spring");
		CHECK(all.find(".set_loop(true, iam_dir_alternate, 3)") != std::string::npos && all.find(".set_delay(0.125f)") != std::string::npos && all.find(".set_loop_delay(0.5f)") != std::string::npos, "export loop/delay");
		CHECK(all.find("iam_anchor_window, 1, iam_ease_linear") != std::string::npos, "export float_rel anchor+axis");
		CHECK(all.find("iam_col_hsv, iam_anchor_window_content") != std::string::npos, "export color_rel space+anchor");
		CHECK(all.find("iam_ease_curve_set(iam_id(\"bump\"), curve_0, 7);") != std::string::npos, "export defines the curve");
		CHECK(all.find("iam_ease_curve, iam_ease_curve_ref(iam_id(\"bump\"))") != std::string::npos, "export references the curve");
		CHECK(all.find("iam_ease_steps, bezier_") != std::string::npos, "export keeps steps params");
		CHECK(all.find(".key_float_var(iam_id(\"varied\"), 0.0f, 0.0f, iam_variation_float{ iam_var_increment, 5.0f") != std::string::npos, "export float variation as key_float_var");
		CHECK(all.find(".key_vec2_var(iam_id(\"varied2\")") != std::string::npos && all.find("iam_variation_vec2{ iam_var_random, iam_v2(3.0f, 4.0f)") != std::string::npos, "export vec2 variation");
		FILE* f2 = fopen("exported_clip2.inl", "wb"); if (f2) { fputs(all.c_str(), f2); fclose(f2); }
	}

	// ---------------- Copy / paste / duplicate, track reorder, ruler labels, curve undo, variation save ----------------
	{
		ImGuiID const A = iam_id("ka"), B = iam_id("kb"), CV = iam_id("cv_curve");
		iam_vec2 const c1[] = { iam_v2(0, 0), iam_v2(0.3f, 0.0f), iam_v2(0.7f, 1.0f), iam_v2(1, 1) };
		iam_ease_curve_set(CV, c1, 4);
		iam_clip::begin(CLIP)
			.key_float(A, 0.0f, 0.0f, iam_ease_curve, iam_ease_curve_ref(CV)).key_float(A, 1.0f, 1.0f)
			.key_float(B, 0.0f, 5.0f).key_float(B, 2.0f, 6.0f)
			.label(iam_id("cue"), 0.5f)
			.end();
		inst = iam_instance(INST);
		iam_play(CLIP, INST).pause();
		frames(40);
		std::string code = export_cpp();
		int ra = row_of(code, "ka"), rb = row_of(code, "kb");
		CHECK(ra == 0 && rb == 1, "rows: ka %d, kb %d", ra, rb);
		L = get_layout(2.0f);
		// Select ka@1.0, copy, move the playhead to 1.5, paste
		click(key_pos(L, ra, 1.0f));
		key(ImGuiKey_C, true);
		frames(30);
		click(key_pos(L, rb, 1.5f));                   // empty lane space: moves the playhead
		key(ImGuiKey_V, true);
		code = export_cpp();
		CHECK(count_occ(code, ".key_float(iam_id(\"ka\")") == 3 && code.find(".key_float(iam_id(\"ka\"), 1.5f, 1.0f") != std::string::npos, "Ctrl+C / Ctrl+V pastes at the playhead");
		key(ImGuiKey_D, true);                         // duplicate the pasted key (selected) right after it
		code = export_cpp();
		CHECK(count_occ(code, ".key_float(iam_id(\"ka\")") == 4, "Ctrl+D duplicates the selection");
		// Undo restores curve points captured with the keys
		iam_vec2 const c2[] = { iam_v2(0, 0), iam_v2(0.1f, 0.9f), iam_v2(0.2f, 1.0f), iam_v2(1, 1) };
		iam_ease_curve_set(CV, c2, 4);
		key(ImGuiKey_Z, true);
		iam_vec2 back[4]; iam_ease_curve_get(CV, back, 4);
		CHECK(IAM_VEC2_TO_IM(back[1]).x == 0.3f, "undo restores the curve points (%.2f)", IAM_VEC2_TO_IM(back[1]).x);
		// Track reorder: drag the ka label down to the kb row
		frames(30);
		L = get_layout(iam_clip_duration(CLIP));
		ImVec2 lab(L.lanes_pos.x + 10.0f, L.lanes_pos.y + L.row_h * 0.5f);
		mouse_to(lab); mouse_btn(0, true);
		for (int i = 1; i <= 6; i++) mouse_to(ImVec2(lab.x, lab.y + L.row_h * 1.2f * i / 6.0f));
		mouse_btn(0, false);
		code = export_cpp();
		CHECK(row_of(code, "kb") == 0 && row_of(code, "ka") == 1, "dragging a track label reorders tracks");
		// Ruler: drag the "cue" label from 0.5s to ~1.0s
		frames(30);
		float d = iam_clip_duration(CLIP);
		L = get_layout(d);
		float ruler_y = L.lanes_pos.y - L.row_h * 0.5f;
		ImVec2 lx(L.tl_x + (0.5f - L.t0) / L.len * L.tl_w, ruler_y);
		mouse_to(lx); mouse_btn(0, true);
		for (int i = 1; i <= 6; i++) mouse_to(ImVec2(lx.x + (0.5f / L.len * L.tl_w) * i / 6.0f, ruler_y));
		mouse_btn(0, false);
		CHECK(fabsf(iam_clip_label_time(CLIP, iam_id("cue")) - 1.0f) < 0.051f, "dragging a ruler label retimes it (%.3f)", iam_clip_label_time(CLIP, iam_id("cue")));
		key(ImGuiKey_Z, true);
		CHECK(iam_clip_label_time(CLIP, iam_id("cue")) == 0.5f, "undo restores the label time");

		// Variations survive save/load (v7)
		ImGuiID const VC = iam_id("var_clip");
		iam_clip::begin(VC).key_float_var(1, 0.0f, 10.0f, iam_varf_inc(5.0f)).key_float(1, 1.0f, 10.0f).set_loop(true, iam_dir_normal, -1).end();
		CHECK(iam_clip_save(VC, "var_clip.iamclip") == iam_ok, "save clip with variation");
		iam_clip::begin(VC).key_float(1, 0.0f, 0.0f).end();
		ImGuiID loaded = 0;
		CHECK(iam_clip_load("var_clip.iamclip", &loaded) == iam_ok && loaded == VC, "load");
		remove("var_clip.iamclip");
		iam_instance vi = iam_play(VC, ImHashStr("vi"));
		for (int f = 0; f < 70; f++) frame();            // second loop: start value 10 + 5
		float vv = 0; vi.get_float(1, &vv);
		CHECK(vv > 10.5f, "variation still applies after load (%.2f)", vv);
	}

	ImGui::DestroyContext();
	printf("%s (%d failures)\n", g_fail ? "FAILED" : "ALL PASSED", g_fail);
	return g_fail ? 1 : 0;
}
