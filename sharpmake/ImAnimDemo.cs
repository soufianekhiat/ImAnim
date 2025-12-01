using System.IO;
using Sharpmake;

namespace ImAnim
{
    // ImAnim Demo Application project
    [Generate]
    public class ImAnimDemoProject : CommonProject
    {
        public ImAnimDemoProject()
        {
            Name = "ImAnimDemo";
            SourceRootPath = Path.Combine("[project.SharpmakeCsPath]", "..", "demo");

            // Add ImPlatform source files
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "ImPlatform", "ImPlatform", "ImPlatform.h"));

            // Add imgui core source files (always included)
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_demo.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_draw.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_tables.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_widgets.cpp"));

            // Add all imgui backend files (will be conditionally excluded per configuration)
            // Platform backends
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_win32.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_glfw.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_sdl2.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_sdl3.cpp"));

            // Graphics backends
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_dx9.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_dx10.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_dx11.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_dx12.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_opengl3.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_vulkan.cpp"));
        }

        public override void ConfigureAll(Configuration conf, ImAnimTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.Output = Configuration.OutputType.Exe;

            // Add src include path for im_anim.h
            conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "src"));

            // Add imgui include paths
            conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui"));
            conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends"));

            // Add ImPlatform include path
            conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "ImPlatform", "ImPlatform"));

            // Configure platform/graphics defines and libraries
            ConfigurePlatformBackend(conf, target);
            ConfigureGraphicsBackend(conf, target);

            // Link with ImAnimLib
            conf.AddPrivateDependency<ImAnimLibProject>(target);

            // Subsystem: Windows (no console)
            conf.Options.Add(Options.Vc.Linker.SubSystem.Windows);

            // Entry point for Windows app
            conf.AdditionalLinkerOptions.Add("/ENTRY:mainCRTStartup");

            // Always need dwmapi for ImPlatform
            conf.LibraryFiles.Add("dwmapi.lib");
        }

        private void ConfigurePlatformBackend(Configuration conf, ImAnimTarget target)
        {
            string basePath = Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends");

            // Set platform define
            string platformDefine = target.ImPlatform switch
            {
                ImPlatform.Win32 => "IM_CONFIG_PLATFORM=IM_PLATFORM_WIN32",
                ImPlatform.GLFW => "IM_CONFIG_PLATFORM=IM_PLATFORM_GLFW",
                ImPlatform.SDL2 => "IM_CONFIG_PLATFORM=IM_PLATFORM_SDL2",
                ImPlatform.SDL3 => "IM_CONFIG_PLATFORM=IM_PLATFORM_SDL3",
                _ => "IM_CONFIG_PLATFORM=IM_PLATFORM_WIN32"
            };
            conf.Defines.Add(platformDefine);

            // Exclude non-matching platform backend files
            if (target.ImPlatform != ImPlatform.Win32)
                conf.SourceFilesBuildExclude.Add(Path.Combine(basePath, "imgui_impl_win32.cpp"));

            if (target.ImPlatform != ImPlatform.GLFW)
                conf.SourceFilesBuildExclude.Add(Path.Combine(basePath, "imgui_impl_glfw.cpp"));

            if (target.ImPlatform != ImPlatform.SDL2)
                conf.SourceFilesBuildExclude.Add(Path.Combine(basePath, "imgui_impl_sdl2.cpp"));

            if (target.ImPlatform != ImPlatform.SDL3)
                conf.SourceFilesBuildExclude.Add(Path.Combine(basePath, "imgui_impl_sdl3.cpp"));

            // Add platform-specific include paths and libraries
            switch (target.ImPlatform)
            {
                case ImPlatform.GLFW:
                    // GLFW include and library paths (adjust as needed for your setup)
                    conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "glfw", "include"));
                    conf.LibraryPaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "glfw", "lib"));
                    conf.LibraryFiles.Add("glfw3.lib");
                    break;

                case ImPlatform.SDL2:
                    // SDL2 include and library paths (adjust as needed for your setup)
                    conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "SDL2", "include"));
                    conf.LibraryPaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "SDL2", "lib"));
                    conf.LibraryFiles.Add("SDL2.lib");
                    conf.LibraryFiles.Add("SDL2main.lib");
                    break;

                case ImPlatform.SDL3:
                    // SDL3 include and library paths (adjust as needed for your setup)
                    conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "SDL3", "include"));
                    conf.LibraryPaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "SDL3", "lib"));
                    conf.LibraryFiles.Add("SDL3.lib");
                    break;
            }
        }

        private void ConfigureGraphicsBackend(Configuration conf, ImAnimTarget target)
        {
            string basePath = Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends");

            // Set graphics define
            string gfxDefine = target.ImGfx switch
            {
                ImGfx.OpenGL3 => "IM_CONFIG_GFX=IM_GFX_OPENGL3",
                ImGfx.DirectX9 => "IM_CONFIG_GFX=IM_GFX_DIRECTX9",
                ImGfx.DirectX10 => "IM_CONFIG_GFX=IM_GFX_DIRECTX10",
                ImGfx.DirectX11 => "IM_CONFIG_GFX=IM_GFX_DIRECTX11",
                ImGfx.DirectX12 => "IM_CONFIG_GFX=IM_GFX_DIRECTX12",
                ImGfx.Vulkan => "IM_CONFIG_GFX=IM_GFX_VULKAN",
                _ => "IM_CONFIG_GFX=IM_GFX_DIRECTX11"
            };
            conf.Defines.Add(gfxDefine);

            // Exclude non-matching graphics backend files
            if (target.ImGfx != ImGfx.DirectX9)
                conf.SourceFilesBuildExclude.Add(Path.Combine(basePath, "imgui_impl_dx9.cpp"));

            if (target.ImGfx != ImGfx.DirectX10)
                conf.SourceFilesBuildExclude.Add(Path.Combine(basePath, "imgui_impl_dx10.cpp"));

            if (target.ImGfx != ImGfx.DirectX11)
                conf.SourceFilesBuildExclude.Add(Path.Combine(basePath, "imgui_impl_dx11.cpp"));

            if (target.ImGfx != ImGfx.DirectX12)
                conf.SourceFilesBuildExclude.Add(Path.Combine(basePath, "imgui_impl_dx12.cpp"));

            if (target.ImGfx != ImGfx.OpenGL3)
                conf.SourceFilesBuildExclude.Add(Path.Combine(basePath, "imgui_impl_opengl3.cpp"));

            if (target.ImGfx != ImGfx.Vulkan)
                conf.SourceFilesBuildExclude.Add(Path.Combine(basePath, "imgui_impl_vulkan.cpp"));

            // Add graphics-specific libraries
            switch (target.ImGfx)
            {
                case ImGfx.DirectX9:
                    conf.LibraryFiles.Add("d3d9.lib");
                    break;

                case ImGfx.DirectX10:
                    conf.LibraryFiles.Add("d3d10.lib");
                    conf.LibraryFiles.Add("dxgi.lib");
                    conf.LibraryFiles.Add("d3dcompiler.lib");
                    break;

                case ImGfx.DirectX11:
                    conf.LibraryFiles.Add("d3d11.lib");
                    conf.LibraryFiles.Add("dxgi.lib");
                    conf.LibraryFiles.Add("d3dcompiler.lib");
                    break;

                case ImGfx.DirectX12:
                    conf.LibraryFiles.Add("d3d12.lib");
                    conf.LibraryFiles.Add("dxgi.lib");
                    conf.LibraryFiles.Add("d3dcompiler.lib");
                    break;

                case ImGfx.OpenGL3:
                    conf.LibraryFiles.Add("opengl32.lib");
                    break;

                case ImGfx.Vulkan:
                    // Vulkan SDK paths (uses environment variable)
                    conf.IncludePaths.Add("$(VULKAN_SDK)/Include");
                    conf.LibraryPaths.Add("$(VULKAN_SDK)/Lib");
                    conf.LibraryFiles.Add("vulkan-1.lib");
                    break;
            }
        }
    }
}
