using System;
using System.IO;
using Sharpmake;

namespace ImAnim
{
    // Platform backends - this is a fragment that extends Target
    [Fragment, Flags]
    public enum ImPlatform
    {
        Win32   = 1 << 0,
        GLFW    = 1 << 1,
        SDL2    = 1 << 2,
        SDL3    = 1 << 3,
    }

    // Graphics backends - this is a fragment that extends Target
    [Fragment, Flags]
    public enum ImGfx
    {
        OpenGL3     = 1 << 0,
        DirectX9    = 1 << 1,
        DirectX10   = 1 << 2,
        DirectX11   = 1 << 3,
        DirectX12   = 1 << 4,
        Vulkan      = 1 << 5,
    }

    // Extended target with platform/graphics backend support
    public class ImAnimTarget : Target
    {
        public ImPlatform ImPlatform;
        public ImGfx ImGfx;

        public ImAnimTarget() { }

        public ImAnimTarget(
            Platform platform,
            DevEnv devEnv,
            Optimization optimization,
            ImPlatform imPlatform,
            ImGfx imGfx)
            : base(platform, devEnv, optimization)
        {
            ImPlatform = imPlatform;
            ImGfx = imGfx;
        }

        // Helper to get the backend configuration name (e.g., "Win32_DX11")
        public string GetBackendName()
        {
            string platformName = ImPlatform.ToString();
            string gfxName = ImGfx switch
            {
                ImGfx.OpenGL3 => "OpenGL3",
                ImGfx.DirectX9 => "DX9",
                ImGfx.DirectX10 => "DX10",
                ImGfx.DirectX11 => "DX11",
                ImGfx.DirectX12 => "DX12",
                ImGfx.Vulkan => "Vulkan",
                _ => ImGfx.ToString()
            };
            return $"{platformName}_{gfxName}";
        }
    }

    // Common project base for all ImAnim projects
    public abstract class CommonProject : Project
    {
        public string WorkingDir = Path.Combine("[project.SharpmakeCsPath]", "..", "working_dir");

        public CommonProject() : base(typeof(ImAnimTarget))
        {
            IsFileNameToLower = false;
            IsTargetFileNameToLower = false;
            SourceRootPath = @"[project.SharpmakeCsPath]\..\src";

            // Add all backend combinations
            AddTargets(CreateTargets());
        }

        public static ImAnimTarget[] CreateTargets()
        {
            var targets = new System.Collections.Generic.List<ImAnimTarget>();

            // Win32 backends
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.Win32, ImGfx.DirectX9));
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.Win32, ImGfx.DirectX10));
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.Win32, ImGfx.DirectX11));
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.Win32, ImGfx.DirectX12));
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.Win32, ImGfx.OpenGL3));

            // GLFW backends
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.GLFW, ImGfx.OpenGL3));
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.GLFW, ImGfx.Vulkan));

            // SDL2 backends
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.SDL2, ImGfx.DirectX11));
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.SDL2, ImGfx.OpenGL3));
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.SDL2, ImGfx.Vulkan));

            // SDL3 backends
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.SDL3, ImGfx.DirectX11));
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.SDL3, ImGfx.OpenGL3));
            targets.Add(new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ImPlatform.SDL3, ImGfx.Vulkan));

            return targets.ToArray();
        }

        [Configure()]
        public virtual void ConfigureAll(Configuration conf, ImAnimTarget target)
        {
            // Include backend name in project filename for clarity
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = Path.Combine("[project.SharpmakeCsPath]", "..", "projects", "[project.Name]");

            // Use backend name in intermediate path to separate build artifacts
            string backendName = target.GetBackendName();
            conf.IntermediatePath = Path.Combine("[project.SharpmakeCsPath]", "..", "tmp", "[project.Name]", backendName, "[target.Optimization]");
            conf.TargetPath = WorkingDir;
            conf.TargetLibraryPath = Path.Combine("[project.SharpmakeCsPath]", "..", "tmp", "lib", backendName + "_[target.Optimization]");

            // Configuration name includes backend (e.g., "Debug_Win32_DX11")
            conf.Name = $"[target.Optimization]_{backendName}";

            // C++17 standard
            conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.CPP17);

            // Warning level 4
            conf.AdditionalCompilerOptions.Add("/W4");

            // Basic defines
            conf.Defines.Add("NOMINMAX");
            conf.Defines.Add("WIN32");
            conf.Defines.Add("_CRT_SECURE_NO_WARNINGS");

            // Optimization settings
            if (target.Optimization == Optimization.Debug)
            {
                conf.Defines.Add("_DEBUG");
                conf.Options.Add(Options.Vc.Compiler.Optimization.Disable);
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebug);
            }
            else
            {
                conf.Defines.Add("NDEBUG");
                conf.Options.Add(Options.Vc.Compiler.Optimization.MaximizeSpeed);
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreaded);
                conf.Options.Add(Options.Vc.Compiler.Inline.AnySuitable);
            }

            // Set working directory for Visual Studio debugging
            conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings();
            conf.VcxprojUserFile.LocalDebuggerWorkingDirectory = "$(SolutionDir)working_dir";
        }
    }
}
