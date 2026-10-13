// Text stagger split units + glyph callback, stagger values/jitter, state-driven clip blending.
#include "im_anim.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { g_fail++; printf("FAIL %d: ", __LINE__); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)
static bool near(float a, float b, float eps) { return fabsf(a - b) <= eps; }

struct glyph_log { int count; int units[64]; float t[64]; unsigned cp[64]; };
static void log_glyph(iam_text_glyph* g, void* user) {
	glyph_log* l = (glyph_log*)user;
	if (l->count < 64) { l->units[l->count] = g->unit; l->t[l->count] = g->t; l->cp[l->count] = g->codepoint; l->count++; }
	if (g->codepoint == 'X') g->alpha = 0.0f;  // callback can hide glyphs
}

static void begin_frame() { ImGui::NewFrame(); iam_update_begin_frame(); iam_clip_update(ImGui::GetIO().DeltaTime); ImGui::Begin("w"); }
static void end_frame() { ImGui::End(); ImGui::Render(); }
static void frames(int n) { for (int i = 0; i < n; i++) { begin_frame(); end_frame(); } }

int main() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(800, 600); io.DeltaTime = 1.0f / 60.0f; io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	// ---------------- Text split ----------------
	iam_text_stagger_opts o;
	o.char_delay = 0.1f; o.char_duration = 0.2f;
	CHECK(near(iam_text_stagger_duration("ab cd", o), 0.6f, 1e-5f), "chars: 5 units -> 0.6s");
	o.split = iam_text_split_words;
	CHECK(near(iam_text_stagger_duration("ab  cd ef", o), 0.4f, 1e-5f), "words: 3 units -> 0.4s");
	o.split = iam_text_split_lines;
	CHECK(near(iam_text_stagger_duration("line one\nline two", o), 0.3f, 1e-5f), "lines: 2 units -> 0.3s");

	glyph_log log = {};
	o.split = iam_text_split_words;
	o.glyph_callback = log_glyph; o.glyph_user = &log;
	begin_frame();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	int v0 = dl->VtxBuffer.Size;
	iam_text_stagger(1, "ab Xd", 0.5f, o);  // duration 0.3s, at 0.15s: word 0 eased > 0, word 1 not started... (delay 0.1)
	int v1 = dl->VtxBuffer.Size;
	end_frame();
	CHECK(log.count == 5, "callback called for every glyph (%d)", log.count);
	CHECK(log.units[0] == 0 && log.units[1] == 0 && log.units[3] == 1 && log.units[4] == 1, "word units 0,0,_,1,1");
	CHECK(log.t[0] == log.t[1] && log.t[3] == log.t[4] && log.t[0] > log.t[3], "characters of a word share timing (%.3f, %.3f)", log.t[0], log.t[3]);
	CHECK(v1 > v0, "draws glyphs");
	// Newlines: second line drawn below the first
	iam_text_stagger_opts lo; lo.effect = iam_text_fx_none; lo.split = iam_text_split_lines;
	begin_frame();
	dl = ImGui::GetWindowDrawList();
	v0 = dl->VtxBuffer.Size;
	iam_text_stagger(2, "A\nB", 1.0f, lo);
	float y_a = dl->VtxBuffer[v0].pos.y, y_b = dl->VtxBuffer[v0 + 4].pos.y;
	end_frame();
	CHECK(y_b > y_a + ImGui::GetFontSize() * 0.5f, "'\\n' starts a new line (%.1f -> %.1f)", y_a, y_b);
	CHECK(iam_text_stagger_width("ab\nabcd", lo) > iam_text_stagger_width("ab", lo) && near(iam_text_stagger_width("ab\nabcd", lo), iam_text_stagger_width("abcd", lo), 0.01f), "width = widest line");

	// ---------------- Stagger values ----------------
	iam_stagger_grid_opts g; g.cols = 5; g.rows = 1;
	CHECK(iam_stagger_value_index(0, 10, 50, g) == 10.0f && iam_stagger_value_index(4, 10, 50, g) == 50.0f && near(iam_stagger_value_index(2, 10, 50, g), 30.0f, 1e-4f), "linear spread 10..50");
	g.from = iam_stagger_center;
	CHECK(iam_stagger_value_index(2, 0, 1, g) == 0.0f && iam_stagger_value_index(0, 0, 1, g) == 1.0f, "from center");
	g.from = iam_stagger_first; g.reversed = true;
	CHECK(iam_stagger_value_index(0, 0, 1, g) == 1.0f && iam_stagger_value_index(4, 0, 1, g) == 0.0f, "reversed");
	g.reversed = false;
	float j0 = iam_stagger_value_index(1, 0, 0, g, 5.0f), j0b = iam_stagger_value_index(1, 0, 0, g, 5.0f);
	bool bounded = true, varied = false;
	for (int i = 0; i < 50; i++) { iam_stagger_grid_opts gg = g; gg.cols = 50; float v = iam_stagger_value_index(i, 0, 0, gg, 5.0f); bounded &= fabsf(v) <= 5.0f; varied |= fabsf(v - j0) > 0.5f; }
	CHECK(j0 == j0b && bounded && varied, "value jitter deterministic, within +/-5, varied");
	g.delay = 0.1f; g.jitter = 0.02f; g.seed = 3;
	float d = iam_stagger_grid_delay_index(2, g);
	CHECK(d != 0.2f && fabsf(d - 0.2f) <= 0.02f + 1e-6f, "delay jitter within +/-0.02 (%.4f)", d);

	// ---------------- State blending ----------------
	ImGuiID const IDLE = ImHashStr("idle"), HOVER = ImHashStr("hover"), M = ImHashStr("machine");
	iam_clip::begin(IDLE).key_float(1, 0.0f, 0.0f).key_float(1, 1.0f, 0.0f).key_color(2, 0.0f, iam_v4(1, 0, 0, 1)).key_color(2, 1.0f, iam_v4(1, 0, 0, 1)).set_loop(true).end();
	iam_clip::begin(HOVER).key_float(1, 0.0f, 100.0f).key_float(1, 1.0f, 100.0f).key_color(2, 0.0f, iam_v4(0, 0, 1, 1)).key_color(2, 1.0f, iam_v4(0, 0, 1, 1)).set_loop(true).end();
	float v = -1;
	begin_frame(); iam_state_play(M, IDLE, 0.5f); end_frame();
	iam_state_get_float(M, 1, &v);
	CHECK(v == 0.0f && iam_state_current(M) == IDLE && iam_state_blend(M) == 1.0f, "first state applies immediately");
	for (int f = 0; f < 15; f++) { begin_frame(); iam_state_play(M, HOVER, 0.5f); end_frame(); }
	iam_state_get_float(M, 1, &v);
	float b = iam_state_blend(M);
	CHECK(v > 30.0f && v < 70.0f && b > 0.3f && b < 0.7f, "mid crossfade blends (v=%.1f, blend=%.2f)", v, b);
	iam_vec4 col; iam_state_get_color(M, 2, &col);
	ImVec4 C = IAM_VEC4_TO_IM(col);
	CHECK(C.x > 0.1f && C.z > 0.1f, "colors blend too (%.2f %.2f %.2f)", C.x, C.y, C.z);
	for (int f = 0; f < 30; f++) { begin_frame(); iam_state_play(M, HOVER, 0.5f); end_frame(); }
	iam_state_get_float(M, 1, &v);
	CHECK(v == 100.0f && iam_state_blend(M) == 1.0f && !iam_state_instance(M, IDLE).valid(), "crossfade done: new state only, old instance freed");
	// Back to idle and return to hover mid-fade: hover keeps playing (no restart)
	iam_instance hover_inst = iam_state_instance(M, HOVER);
	for (int f = 0; f < 10; f++) { begin_frame(); iam_state_play(M, IDLE, 0.5f); end_frame(); }
	float hover_t = hover_inst.time();
	begin_frame(); iam_state_play(M, HOVER, 0.5f); end_frame();
	CHECK(hover_inst.valid() && hover_inst.time() >= hover_t, "returning mid-fade keeps the clip playing (t %.2f -> %.2f)", hover_t, hover_inst.time());
	// Instant switch
	begin_frame(); iam_state_play(M, IDLE, 0.0f); end_frame();
	iam_state_get_float(M, 1, &v);
	CHECK(v == 0.0f && iam_state_blend(M) == 1.0f, "crossfade 0 switches instantly");
	// Reduced motion: instant even with a crossfade
	iam_set_reduced_motion(true);
	begin_frame(); iam_state_play(M, HOVER, 0.5f); end_frame();
	iam_state_get_float(M, 1, &v);
	iam_set_reduced_motion(false);
	CHECK(v == 100.0f, "reduced motion switches instantly (%.1f)", v);
	// GC
	frames(5);
	iam_gc(2);
	CHECK(iam_state_current(M) == 0 && !iam_state_get_float(M, 1, &v), "unused machine collected by iam_gc");

	ImGui::DestroyContext();
	printf("%s (%d failures)\n", g_fail ? "FAILED" : "ALL PASSED", g_fail);
	return g_fail ? 1 : 0;
}
