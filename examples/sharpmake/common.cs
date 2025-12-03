using System;
using System.IO;
using Sharpmake;

namespace ImAnim
{
    // Example configuration types
    [Fragment, Flags]
    public enum ExampleType
    {
        GLFW_OpenGL3    = 1 << 0,
        SDL2_OpenGL3    = 1 << 1,
        Win32_DirectX11 = 1 << 2,
        ImPlatform      = 1 << 3,
    }

    // Extended target with example type support
    public class ImAnimTarget : Target
    {
        public ExampleType ExampleType;

        public ImAnimTarget() { }

        public ImAnimTarget(
            Platform platform,
            DevEnv devEnv,
            Optimization optimization,
            ExampleType exampleType)
            : base(platform, devEnv, optimization)
        {
            ExampleType = exampleType;
        }

        // Helper to get the configuration name
        public string GetConfigName()
        {
            return ExampleType switch
            {
                ExampleType.GLFW_OpenGL3 => "GLFW_OpenGL3",
                ExampleType.SDL2_OpenGL3 => "SDL2_OpenGL3",
                ExampleType.Win32_DirectX11 => "Win32_DX11",
                ExampleType.ImPlatform => "ImPlatform",
                _ => ExampleType.ToString()
            };
        }
    }

    // Common project base for all ImAnim projects
    public abstract class CommonProject : Project
    {
        public CommonProject() : base(typeof(ImAnimTarget))
        {
            IsFileNameToLower = false;
            IsTargetFileNameToLower = false;

            // Add all example configurations
            AddTargets(CreateTargets());
        }

        public static ImAnimTarget[] CreateTargets()
        {
            return new ImAnimTarget[]
            {
                new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ExampleType.GLFW_OpenGL3),
                new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ExampleType.SDL2_OpenGL3),
                new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ExampleType.Win32_DirectX11),
                new ImAnimTarget(Platform.win64, DevEnv.vs2022, Optimization.Debug | Optimization.Release, ExampleType.ImPlatform),
            };
        }

        [Configure()]
        public virtual void ConfigureAll(Configuration conf, ImAnimTarget target)
        {
            string configName = target.GetConfigName();

            // Project paths
            conf.ProjectFileName = "[project.Name]_[target.DevEnv]_[target.Platform]";
            conf.ProjectPath = Path.Combine("[project.SharpmakeCsPath]", "projects", "[project.Name]");

            // Build artifacts separated by config
            conf.IntermediatePath = Path.Combine("[project.SharpmakeCsPath]", "tmp", "[project.Name]", configName, "[target.Optimization]");
            conf.TargetPath = Path.Combine("[project.SharpmakeCsPath]", "bin", configName);
            conf.TargetLibraryPath = Path.Combine("[project.SharpmakeCsPath]", "tmp", "lib", configName + "_[target.Optimization]");

            // Configuration name includes example type
            conf.Name = $"[target.Optimization]_{configName}";

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
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebugDLL);
            }
            else
            {
                conf.Defines.Add("NDEBUG");
                conf.Options.Add(Options.Vc.Compiler.Optimization.MaximizeSpeed);
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);
                conf.Options.Add(Options.Vc.Compiler.Inline.AnySuitable);
            }

            // Set working directory for Visual Studio debugging
            conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings();
            conf.VcxprojUserFile.LocalDebuggerWorkingDirectory = conf.TargetPath;
        }
    }
}
