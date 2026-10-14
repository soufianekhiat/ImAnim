// Performance benchmarks: per-frame cost of common workloads. Prints a table; fails only on gross
// regressions (generous limits, CI machines vary). Build optimized (-O2).
#include "im_anim.h"
#include "im_anim_lottie.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <string>
#include <chrono>

static int g_fail = 0;

struct bench_result { const char* name; int count; double ms_per_frame; double limit_ms; };

template<typename F>
static double measure(int frames, F work) {
	ImGuiIO& io = ImGui::GetIO();
	double total = 0.0;
	for (int f = 0; f < frames; f++) {
		ImGui::NewFrame();
		iam_update_begin_frame();
		auto t0 = std::chrono::high_resolution_clock::now();
		iam_clip_update(io.DeltaTime);
		work(f);
		auto t1 = std::chrono::high_resolution_clock::now();
		ImGui::Render();
		if (f >= frames / 4) total += std::chrono::duration<double, std::milli>(t1 - t0).count();  // skip warm-up
	}
	return total / (frames - frames / 4);
}

static void report(bench_result const& r) {
	bool ok = r.ms_per_frame <= r.limit_ms;
	if (!ok) g_fail++;
	printf("%-34s %7d  %8.3f ms/frame  %8.1f ns/item  (limit %.1f ms) %s\n", r.name, r.count, r.ms_per_frame,
		r.ms_per_frame * 1e6 / (r.count > 0 ? r.count : 1), r.limit_ms, ok ? "" : "  <-- FAIL");
}

int main() {
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(1280, 720); io.DeltaTime = 1.0f / 60.0f; io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	iam_ease_desc ez = iam_ease_preset(iam_ease_out_cubic);
	int const frames = 120;

	{   // Float tweens, retargeted every 30 frames
		int const n = 10000;
		double ms = measure(frames, [&](int f) {
			float target = (f / 30) % 2 ? 100.0f : 0.0f;
			for (int i = 0; i < n; i++) iam_tween_float((ImGuiID)i + 1, 1, target, 0.5f, ez, iam_policy_crossfade, io.DeltaTime);
		});
		report({ "tween_float (retarget/30f)", n, ms, 20.0 });
	}
	{   // Vec2 tweens
		int const n = 5000;
		double ms = measure(frames, [&](int f) {
			float t = (f / 30) % 2 ? 100.0f : 0.0f;
			for (int i = 0; i < n; i++) iam_tween_vec2((ImGuiID)i + 1, 2, iam_v2(t, -t), 0.5f, ez, iam_policy_crossfade, io.DeltaTime);
		});
		report({ "tween_vec2", n, ms, 20.0 });
	}
	{   // Color tweens in OKLAB
		int const n = 2000;
		double ms = measure(frames, [&](int f) {
			iam_vec4 c = (f / 30) % 2 ? iam_v4(1, 0, 0, 1) : iam_v4(0, 0, 1, 1);
			for (int i = 0; i < n; i++) iam_tween_color((ImGuiID)i + 1, 3, c, 0.5f, ez, iam_policy_crossfade, iam_col_oklab, io.DeltaTime);
		});
		report({ "tween_color (oklab)", n, ms, 20.0 });
	}
	{   // Springs following a moving target
		int const n = 5000;
		iam_spring_params sp = iam_spring_default();
		double ms = measure(frames, [&](int f) {
			for (int i = 0; i < n; i++) iam_spring_float((ImGuiID)i + 1, 4, (float)(f % 60), sp, io.DeltaTime);
		});
		report({ "spring_float (moving target)", n, ms, 20.0 });
	}
	{   // Clip instances: 4 tracks, looping
		int const n = 1000;
		ImGuiID clip = ImHashStr("bench_clip");
		iam_clip::begin(clip)
			.key_float(1, 0.0f, 0.0f, iam_ease_out_cubic).key_float(1, 1.0f, 1.0f)
			.key_vec2(2, 0.0f, iam_v2(0, 0)).key_vec2(2, 0.5f, iam_v2(10, 5), iam_ease_in_out_sine).key_vec2(2, 1.0f, iam_v2(20, 0))
			.key_color(3, 0.0f, iam_v4(1, 0, 0, 1)).key_color(3, 1.0f, iam_v4(0, 0, 1, 1))
			.key_float(4, 0.0f, 0.0f, iam_ease_out_bounce).key_float(4, 1.0f, 1.0f)
			.set_loop(true).end();
		for (int i = 0; i < n; i++) iam_play(clip, (ImGuiID)i + 100);
		double ms = measure(frames, [&](int) {});
		report({ "clip instances (4 tracks) update", n, ms, 25.0 });
		double ms_read = measure(frames, [&](int) {
			for (int i = 0; i < n; i++) { float v; iam_instance((ImGuiID)i + 100).get_float(1, &v); }
		}) - ms;
		report({ "  + get_float per instance", n, ms_read > 0 ? ms_read : 0.0, 10.0 });
		for (int i = 0; i < n; i++) iam_instance((ImGuiID)i + 100).destroy();
	}
	{   // State machines switching every 20 frames
		int const n = 2000;
		ImGuiID a = ImHashStr("bench_state_a"), b = ImHashStr("bench_state_b");
		iam_clip::begin(a).key_float(1, 0.0f, 0.0f).key_float(1, 1.0f, 1.0f).set_loop(true).end();
		iam_clip::begin(b).key_float(1, 0.0f, 1.0f).key_float(1, 1.0f, 0.0f).set_loop(true).end();
		double ms = measure(frames, [&](int f) {
			for (int i = 0; i < n; i++) {
				iam_state_play((ImGuiID)i + 50000, ((f + i) / 20) % 2 ? a : b, 0.25f);
				float v; iam_state_get_float((ImGuiID)i + 50000, 1, &v);
			}
		});
		report({ "state_play + get (crossfading)", n, ms, 30.0 });
	}
	{   // Presence toggling
		int const n = 2000;
		double ms = measure(frames, [&](int f) {
			for (int i = 0; i < n; i++) { float t; iam_presence((ImGuiID)i + 90000, ((f + i) / 15) % 2 == 0, &t, io.DeltaTime); }
		});
		report({ "presence (toggling)", n, ms, 10.0 });
	}
	{   // Clip editor frame on a 20-track clip (UI cost)
		ImGuiID clip = ImHashStr("bench_editor_clip");
		iam_clip c = iam_clip::begin(clip);
		for (int t = 0; t < 20; t++) for (int k = 0; k < 8; k++) c.key_float((ImGuiID)t + 1, k * 0.25f, (float)k, iam_ease_in_out_cubic);
		c.end();
		double ms = measure(50, [&](int) {
			ImGui::SetNextWindowSize(ImVec2(1200, 600));
			iam_show_clip_editor(clip);
		});
		report({ "clip editor frame (20 tracks x 8)", 1, ms, 10.0 });
	}

	{   // Lottie: 24 rotating groups (star + gradient ellipse with animated size + stroke), 8 draws of 256 px per frame
		std::string sh;
		char buf[1024];
		for (int g = 0; g < 24; g++) {
			float const x = (float)(g % 6) * 32.0f - 80.0f, y = (float)(g / 6) * 40.0f - 60.0f;
			snprintf(buf, sizeof(buf),
				"%s{\"ty\":\"gr\",\"it\":["
				"{\"ty\":\"sr\",\"sy\":1,\"pt\":{\"a\":0,\"k\":5},\"p\":{\"a\":0,\"k\":[%g,%g]},\"r\":{\"a\":0,\"k\":0},\"ir\":{\"a\":0,\"k\":6},\"or\":{\"a\":0,\"k\":14},\"is\":{\"a\":0,\"k\":0},\"os\":{\"a\":0,\"k\":0}},"
				"{\"ty\":\"el\",\"p\":{\"a\":0,\"k\":[%g,%g]},\"s\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[8,8],\"o\":{\"x\":[0.4],\"y\":[0]},\"i\":{\"x\":[0.6],\"y\":[1]}},{\"t\":60,\"s\":[30,22]}]}},"
				"{\"ty\":\"st\",\"c\":{\"a\":0,\"k\":[0.1,0.1,0.2,1]},\"o\":{\"a\":0,\"k\":100},\"w\":{\"a\":0,\"k\":2},\"lc\":2,\"lj\":2},"
				"{\"ty\":\"gf\",\"t\":1,\"s\":{\"a\":0,\"k\":[%g,%g]},\"e\":{\"a\":0,\"k\":[%g,%g]},\"g\":{\"p\":2,\"k\":{\"a\":0,\"k\":[0,1,0.5,0,1,0.2,0.4,1]}},\"o\":{\"a\":0,\"k\":100},\"r\":1},"
				"{\"ty\":\"tr\",\"p\":{\"a\":0,\"k\":[0,0]},\"a\":{\"a\":0,\"k\":[0,0]},\"s\":{\"a\":0,\"k\":[100,100]},\"r\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[0],\"o\":{\"x\":[0.4],\"y\":[0]},\"i\":{\"x\":[0.6],\"y\":[1]}},{\"t\":120,\"s\":[%d]}]},\"o\":{\"a\":0,\"k\":100}}]}",
				g ? "," : "", x, y, x, y, x - 15, y, x + 15, y, g % 2 ? 360 : -360);
			sh += buf;
		}
		std::string const json = "{\"v\":\"5.7.0\",\"fr\":60,\"ip\":0,\"op\":120,\"w\":256,\"h\":256,\"layers\":[{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":120,\"st\":0,"
			"\"ks\":{\"o\":{\"a\":0,\"k\":100},\"r\":{\"a\":0,\"k\":0},\"p\":{\"a\":0,\"k\":[128,128]},\"a\":{\"a\":0,\"k\":[0,0]},\"s\":{\"a\":0,\"k\":[100,100]}},"
			"\"shapes\":[" + sh + "]}]}";
		iam_lottie* anim = iam_lottie_load_from_memory(json.data(), json.size());
		if (!anim || iam_lottie_warnings(anim)[0]) { printf("lottie bench animation failed to load: %s\n", anim ? iam_lottie_warnings(anim) : "null"); g_fail++; }
		else {
			io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // replay cache: draw lists past 64k vertices
			auto draw = [&](int f, bool live) {
				ImDrawList* dl = ImGui::GetForegroundDrawList();
				for (int k = 0; k < 8; k++)
					iam_lottie_draw(dl, anim, live ? (f + k * 15) / 60.0f : k * 0.25f, iam_v2((k % 4) * 260.0f, (k / 4) * 260.0f), iam_v2(256, 256));
			};
			report({ "lottie draw (24 groups, live)", 8, measure(frames, [&](int f) { draw(f, true); }), 60.0 });
			report({ "lottie draw (repeated frames)", 8, measure(frames, [&](int f) { draw(f, false); }), 10.0 });
			iam_lottie_free(anim);
		}
	}

	ImGui::DestroyContext();
	printf("%s (%d over limit)\n", g_fail ? "FAILED" : "ALL WITHIN LIMITS", g_fail);
	return g_fail ? 1 : 0;
}
