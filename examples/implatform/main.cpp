// main.cpp — ImAnim Demo using ImPlatform

#ifdef _WIN32
extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

// Platform and Graphics API configuration
// Using Win32 + DirectX11 by default
#ifndef IM_CONFIG_PLATFORM
#define IM_CONFIG_PLATFORM IM_PLATFORM_WIN32
#endif

#ifndef IM_CONFIG_GFX
#define IM_CONFIG_GFX IM_GFX_DIRECTX11
#endif

#define IM_CURRENT_PLATFORM IM_CONFIG_PLATFORM
#define IM_CURRENT_GFX IM_CONFIG_GFX

#define IMPLATFORM_IMPLEMENTATION
#include <ImPlatform.h>

#include <stdio.h>

extern void ImAnimDemoWindow();
extern void ImAnimDocWindow();

int main()
{
	bool bGood;

	bGood = ImPlatform_CreateWindow("ImAnim Demo", ImVec2(100, 100), 1280, 720);
	if (!bGood)
	{
		fprintf(stderr, "ImPlatform: Cannot create window.\n");
		return 1;
	}

	bGood = ImPlatform_InitGfxAPI();
	if (!bGood)
	{
		fprintf(stderr, "ImPlatform: Cannot initialize the Graphics API.\n");
		return 1;
	}

	bGood = ImPlatform_ShowWindow();
	if (!bGood)
	{
		fprintf(stderr, "ImPlatform: Cannot show the window.\n");
		return 1;
	}

	IMGUI_CHECKVERSION();
	bGood = ImGui::CreateContext() != nullptr;
	if (!bGood)
	{
		fprintf(stderr, "ImGui: Cannot create context.\n");
		return 1;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
#ifdef IMGUI_HAS_DOCK
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#ifdef IMGUI_HAS_VIEWPORT
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

	ImGui::StyleColorsDark();

	// Setup DPI scaling (Win32 only) - must be after window creation
	ImGuiStyle& style = ImGui::GetStyle();
#if defined(IM_CURRENT_PLATFORM) && (IM_CURRENT_PLATFORM == IM_PLATFORM_WIN32)
	float dpi_scale = ImPlatform_App_GetDpiScale_Win32();
	style.ScaleAllSizes(dpi_scale);
	style.FontScaleDpi = dpi_scale;
#endif

#ifdef IMGUI_HAS_VIEWPORT
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
#endif

	bGood = ImPlatform_InitPlatform();
	if (!bGood)
	{
		fprintf(stderr, "ImPlatform: Cannot initialize platform.\n");
		return 1;
	}

	bGood = ImPlatform_InitGfx();
	if (!bGood)
	{
		fprintf(stderr, "ImPlatform: Cannot initialize graphics.\n");
		return 1;
	}

	ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

	while (ImPlatform_PlatformContinue())
	{
		ImPlatform_PlatformEvents();

		if (!ImPlatform_GfxCheck())
			continue;

		ImPlatform_GfxAPINewFrame();
		ImPlatform_PlatformNewFrame();
		ImGui::NewFrame();

		// Show ImAnim demo window
		ImAnimDemoWindow();

		// Show Doc
		ImAnimDocWindow();

		// Show ImGui demo window for reference
		static bool show_imgui_demo = true;
		if (show_imgui_demo)
			ImGui::ShowDemoWindow(&show_imgui_demo);

		ImGui::Render();
		ImPlatform_GfxAPIClear(clear_color);
		ImPlatform_GfxAPIRender(clear_color);

#ifdef IMGUI_HAS_VIEWPORT
		ImPlatform_GfxViewportPre();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		ImPlatform_GfxViewportPost();
#endif

		ImPlatform_GfxAPISwapBuffer();
	}

	ImPlatform_ShutdownGfxAPI();
	ImPlatform_ShutdownWindow();
	ImPlatform_ShutdownPostGfxAPI();

	ImGui::DestroyContext();

	ImPlatform_DestroyWindow();

	return 0;
}
