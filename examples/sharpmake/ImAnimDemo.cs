using System.IO;
using Sharpmake;

namespace ImAnim
{
    // ImAnim Demo Application project - includes all 4 example main.cpp files
    // with proper exclusions per configuration
    [Generate]
    public class ImAnimDemoProject : CommonProject
    {
        public ImAnimDemoProject()
        {
            Name = "ImAnimDemo";

            // Set source root to examples folder
            SourceRootPath = Path.Combine("[project.SharpmakeCsPath]", "..");

            // Only include specific files, not automatic scanning
            SourceFilesExtensions.Clear();

            // Add im_anim from repository root
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "..", "im_anim.h"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "..", "im_anim.cpp"));

            // Add im_anim_demo.cpp and im_anim_doc.cpp from repository root (shared by all configs)
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "..", "im_anim_demo.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "..", "im_anim_doc.cpp"));

            // Add all 4 main.cpp files (will be excluded per config)
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "glfw_opengl3", "main.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "sdl2_opengl3", "main.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "win32_directx11", "main.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "implatform", "main.cpp"));

            // Add ImPlatform header (for ImPlatform config)
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "ImPlatform", "ImPlatform", "ImPlatform.h"));

            // Add imgui core source files (always included)
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_demo.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_draw.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_tables.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_widgets.cpp"));

            // Add imgui backend files (conditionally excluded per configuration)
            // Platform backends
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_win32.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_glfw.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_sdl2.cpp"));

            // Graphics backends
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_dx11.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_opengl3.cpp"));
        }

        public override void ConfigureAll(Configuration conf, ImAnimTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.Output = Configuration.OutputType.Exe;

            // Add root include path for im_anim.h
            conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", ".."));

            // Add imgui include paths
            conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui"));
            conf.IncludePaths.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends"));

            // Subsystem: Windows (no console)
            conf.Options.Add(Options.Vc.Linker.SubSystem.Windows);

            // Entry point for Windows app
            conf.AdditionalLinkerOptions.Add("/ENTRY:mainCRTStartup");

            // Configure based on example type
            ConfigureExampleType(conf, target);
        }

        private void ConfigureExampleType(Configuration conf, ImAnimTarget target)
        {
            string basePath = Path.Combine("[project.SharpmakeCsPath]", "..");
            string backendsPath = Path.Combine(basePath, "extern", "imgui", "backends");

            // Exclude all main.cpp files first, then include the right one
            string glfwMain = Path.Combine(basePath, "glfw_opengl3", "main.cpp");
            string sdl2Main = Path.Combine(basePath, "sdl2_opengl3", "main.cpp");
            string win32Main = Path.Combine(basePath, "win32_directx11", "main.cpp");
            string implatformMain = Path.Combine(basePath, "implatform", "main.cpp");

            // Exclude ImPlatform header by default
            string implatformHeader = Path.Combine(basePath, "extern", "ImPlatform", "ImPlatform", "ImPlatform.h");

            switch (target.ExampleType)
            {
                case ExampleType.GLFW_OpenGL3:
                    // Include GLFW main, exclude others
                    conf.SourceFilesBuildExclude.Add(sdl2Main);
                    conf.SourceFilesBuildExclude.Add(win32Main);
                    conf.SourceFilesBuildExclude.Add(implatformMain);
                    conf.SourceFilesBuildExclude.Add(implatformHeader);

                    // Exclude unused backends
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_win32.cpp"));
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_sdl2.cpp"));
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_dx11.cpp"));

                    // GLFW + OpenGL3 libs
                    conf.IncludePaths.Add(Path.Combine(basePath, "extern", "imgui", "examples", "libs", "glfw", "include"));
                    conf.LibraryPaths.Add(Path.Combine(basePath, "extern", "imgui", "examples", "libs", "glfw", "lib-vc2010-64"));
                    conf.LibraryFiles.Add("glfw3.lib");
                    conf.LibraryFiles.Add("opengl32.lib");
                    break;

                case ExampleType.SDL2_OpenGL3:
                    // Include SDL2 main, exclude others
                    conf.SourceFilesBuildExclude.Add(glfwMain);
                    conf.SourceFilesBuildExclude.Add(win32Main);
                    conf.SourceFilesBuildExclude.Add(implatformMain);
                    conf.SourceFilesBuildExclude.Add(implatformHeader);

                    // Exclude unused backends
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_win32.cpp"));
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_glfw.cpp"));
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_dx11.cpp"));

                    // SDL2 + OpenGL3 libs
                    conf.IncludePaths.Add(Path.Combine(basePath, "extern", "SDL2", "include"));
                    conf.LibraryPaths.Add(Path.Combine(basePath, "extern", "SDL2", "lib"));
                    conf.LibraryFiles.Add("SDL2.lib");
                    conf.LibraryFiles.Add("SDL2main.lib");
                    conf.LibraryFiles.Add("opengl32.lib");
                    break;

                case ExampleType.Win32_DirectX11:
                    // Include Win32 main, exclude others
                    conf.SourceFilesBuildExclude.Add(glfwMain);
                    conf.SourceFilesBuildExclude.Add(sdl2Main);
                    conf.SourceFilesBuildExclude.Add(implatformMain);
                    conf.SourceFilesBuildExclude.Add(implatformHeader);

                    // Exclude unused backends
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_glfw.cpp"));
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_sdl2.cpp"));
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_opengl3.cpp"));

                    // DirectX11 libs
                    conf.LibraryFiles.Add("d3d11.lib");
                    conf.LibraryFiles.Add("dxgi.lib");
                    conf.LibraryFiles.Add("d3dcompiler.lib");
                    break;

                case ExampleType.ImPlatform:
                    // Include ImPlatform main, exclude others
                    conf.SourceFilesBuildExclude.Add(glfwMain);
                    conf.SourceFilesBuildExclude.Add(sdl2Main);
                    conf.SourceFilesBuildExclude.Add(win32Main);

                    // Exclude unused backends (ImPlatform uses Win32 + DX11 by default)
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_glfw.cpp"));
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_sdl2.cpp"));
                    conf.SourceFilesBuildExclude.Add(Path.Combine(backendsPath, "imgui_impl_opengl3.cpp"));

                    // ImPlatform include path
                    conf.IncludePaths.Add(Path.Combine(basePath, "extern", "ImPlatform", "ImPlatform"));

                    // ImPlatform uses Win32 + DX11
                    conf.Defines.Add("IM_CONFIG_PLATFORM=IM_PLATFORM_WIN32");
                    conf.Defines.Add("IM_CONFIG_GFX=IM_GFX_DIRECTX11");

                    // DirectX11 + dwmapi libs
                    conf.LibraryFiles.Add("d3d11.lib");
                    conf.LibraryFiles.Add("dxgi.lib");
                    conf.LibraryFiles.Add("d3dcompiler.lib");
                    conf.LibraryFiles.Add("dwmapi.lib");
                    break;
            }
        }
    }
}
