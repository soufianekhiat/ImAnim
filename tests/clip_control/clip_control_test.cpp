// Clip playhead control: reverse(), tween_to(), labels, iam_dir_reverse start, save/load of labels.
#include "im_anim.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <math.h>

static int g_fail = 0;
#define CHECK(cond, ...) do { if (!(cond)) { g_fail++; printf("FAIL %d: ", __LINE__); } else printf("ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)

static void frames(int n) {
	for (int i = 0; i < n; i++) { ImGui::NewFrame(); iam_update_begin_frame(); iam_clip_update(ImGui::GetIO().DeltaTime); ImGui::Render(); }
}
static float value(iam_instance inst) { float v = -1; inst.get_float(1, &v); return v; }
static bool near(float a, float b, float eps) { return fabsf(a - b) <= eps; }
static int g_marker_hits = 0;
static void on_marker(ImGuiID, ImGuiID, float, void*) { g_marker_hits++; }

int main() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(800, 600); io.DeltaTime = 1.0f / 60.0f; io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	ImGuiID const LIN = ImHashStr("lin"), REV = ImHashStr("rev"), LOOP = ImHashStr("loop"), LAB = ImHashStr("lab");
	iam_clip::begin(LIN).key_float(1, 0.0f, 0.0f).key_float(1, 1.0f, 1.0f).marker(0.4f, 7, on_marker).label(ImHashStr("mid"), 0.5f).end();
	iam_clip::begin(REV).key_float(1, 0.0f, 0.0f).key_float(1, 1.0f, 1.0f).set_loop(false, iam_dir_reverse, 0).end();
	iam_clip::begin(LOOP).key_float(1, 0.0f, 0.0f).key_float(1, 1.0f, 1.0f).set_loop(true, iam_dir_normal, -1).end();
	iam_clip::begin(LAB)
		.seq_begin()
			.key_float(1, 0.0f, 0.0f).key_float(1, 1.0f, 1.0f)
		.seq_end()
		.seq_begin()
			.label(ImHashStr("second"), 0.25f)
			.key_float(1, 0.5f, 2.0f)
		.seq_end()
		.end();

	// --- reverse() mid-play
	iam_instance a = iam_play(LIN, ImHashStr("a"));
	frames(30);
	CHECK(near(a.time(), 0.5f, 0.02f), "plays forward to 0.5 (%.3f)", a.time());
	a.reverse();
	CHECK(a.is_reversed(), "is_reversed after reverse()");
	frames(15);
	CHECK(near(a.time(), 0.25f, 0.02f) && near(value(a), 0.25f, 0.02f), "plays backward (%.3f)", a.time());
	frames(30);
	CHECK(!a.is_playing() && a.time() == 0.0f && value(a) == 0.0f, "finishes at the start when reversed");

	// --- reverse() on a finished instance plays it back
	iam_instance b = iam_play(LIN, ImHashStr("b"));
	frames(70);
	CHECK(!b.is_playing() && b.time() == 1.0f, "finished forward");
	b.reverse();
	frames(30);
	CHECK(b.is_playing() && near(b.time(), 0.5f, 0.02f), "finished instance plays back after reverse (%.3f)", b.time());

	// --- iam_dir_reverse starts at the end (non-looping reverse clips used to finish immediately)
	iam_instance r = iam_play(REV, ImHashStr("r"));
	CHECK(value(r) == 1.0f && r.time() == 1.0f, "reverse clip starts at the end (t=%.3f v=%.3f)", r.time(), value(r));
	frames(30);
	CHECK(r.is_playing() && near(value(r), 0.5f, 0.02f), "reverse clip plays back (%.3f)", value(r));
	frames(40);
	CHECK(!r.is_playing() && value(r) == 0.0f, "reverse clip finishes at 0");

	// --- normal looping unchanged
	iam_instance l = iam_play(LOOP, ImHashStr("l"));
	frames(90);
	CHECK(l.is_playing() && near(l.time(), 0.5f, 0.02f), "normal loop wraps (%.3f)", l.time());

	// --- tween_to: eases the playhead and holds; fires markers it crosses once
	iam_instance c = iam_play(LIN, ImHashStr("c"));
	c.pause();
	g_marker_hits = 0;
	c.tween_to(0.8f, 0.5f, iam_ease_out_cubic);
	frames(10);
	float mid_t = c.time();
	frames(25);
	CHECK(mid_t > 0.3f && mid_t < 0.8f, "tween_to moving (%.3f)", mid_t);
	CHECK(c.time() == 0.8f && near(value(c), 0.8f, 1e-4f) && c.is_paused(), "tween_to lands and holds paused (%.3f)", c.time());
	CHECK(g_marker_hits == 1, "marker crossed once (%d)", g_marker_hits);
	c.tween_to(0.2f, 0.0f);
	frames(1);
	CHECK(c.time() == 0.2f, "zero-duration tween_to jumps (%.3f)", c.time());
	c.stop();
	c.tween_to(0.6f, 0.2f);
	frames(20);
	CHECK(c.time() == 0.6f, "tween_to works from a stopped instance (%.3f)", c.time());

	// --- labels
	CHECK(iam_clip_label_time(LIN, ImHashStr("mid")) == 0.5f, "label time");
	CHECK(near(iam_clip_label_time(LAB, ImHashStr("second")), 1.25f, 1e-4f), "label inside a seq block is offset (%.3f)", iam_clip_label_time(LAB, ImHashStr("second")));
	CHECK(iam_clip_label_time(LIN, ImHashStr("nope")) < 0.0f, "missing label: -1");
	iam_instance d = iam_play(LIN, ImHashStr("d"));
	d.pause();
	CHECK(d.seek_label(ImHashStr("mid")) && d.time() == 0.5f && near(value(d), 0.5f, 1e-4f), "seek_label jumps and re-evaluates");
	CHECK(!d.seek_label(ImHashStr("nope")) && !d.tween_to_label(ImHashStr("nope"), 1.0f), "unknown label returns false");
	CHECK(d.tween_to_label(ImHashStr("mid"), 0.1f), "tween_to_label");

	// --- labels survive save/load (v5)
	CHECK(iam_clip_save(LAB, "clip_control_test.iamclip") == iam_ok, "save");
	iam_clip::begin(LAB).key_float(1, 0.0f, 0.0f).end();  // wipe
	ImGuiID loaded = 0;
	CHECK(iam_clip_load("clip_control_test.iamclip", &loaded) == iam_ok && loaded == LAB, "load");
	CHECK(near(iam_clip_label_time(LAB, ImHashStr("second")), 1.25f, 1e-4f), "label restored by load");
	remove("clip_control_test.iamclip");

	ImGui::DestroyContext();
	printf("%s (%d failures)\n", g_fail ? "FAILED" : "ALL PASSED", g_fail);
	return g_fail ? 1 : 0;
}
