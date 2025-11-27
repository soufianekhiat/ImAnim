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

            // Add imgui source files
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_demo.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_draw.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_tables.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "imgui_widgets.cpp"));

            // Add imgui backends
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_win32.cpp"));
            SourceFiles.Add(Path.Combine("[project.SharpmakeCsPath]", "..", "extern", "imgui", "backends", "imgui_impl_dx11.cpp"));
        }

        public override void ConfigureAll(Configuration conf, Target target)
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

            // Platform/Graphics configuration: Win32 + DirectX11
            conf.Defines.Add("IM_CONFIG_PLATFORM=IM_PLATFORM_WIN32");
            conf.Defines.Add("IM_CONFIG_GFX=IM_GFX_DIRECTX11");

            // Link with DirectX and Windows libraries
            conf.LibraryFiles.Add("d3d11.lib");
            conf.LibraryFiles.Add("dxgi.lib");
            conf.LibraryFiles.Add("d3dcompiler.lib");
            conf.LibraryFiles.Add("dwmapi.lib");

            // Link with ImAnimLib
            conf.AddPrivateDependency<ImAnimLibProject>(target);

            // Subsystem: Windows (no console)
            conf.Options.Add(Options.Vc.Linker.SubSystem.Windows);

            // Entry point for Windows app
            conf.AdditionalLinkerOptions.Add("/ENTRY:mainCRTStartup");
        }
    }
}
