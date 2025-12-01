using Sharpmake;

[module: Sharpmake.Include("common.cs")]
[module: Sharpmake.Include("ImAnimLib.cs")]
[module: Sharpmake.Include("ImAnimDemo.cs")]

namespace ImAnim
{
    [Generate]
    public class ImAnimSolution : Solution
    {
        public ImAnimSolution() : base(typeof(ImAnimTarget))
        {
            Name = "ImAnim";
            IsFileNameToLower = false;

            // Add all backend combinations
            AddTargets(CommonProject.CreateTargets());
        }

        [Configure]
        public void ConfigureAll(Configuration conf, ImAnimTarget target)
        {
            conf.SolutionFileName = "[solution.Name]_[target.DevEnv]_[target.Platform]";
            conf.SolutionPath = @"[solution.SharpmakeCsPath]\..";

            // Name the solution configuration to include backend (e.g., "Debug_Win32_DX11")
            string backendName = target.GetBackendName();
            conf.Name = $"[target.Optimization]_{backendName}";

            // Add projects
            conf.AddProject<ImAnimLibProject>(target);
            conf.AddProject<ImAnimDemoProject>(target);
        }

        [Main]
        public static void SharpmakeMain(Arguments args)
        {
            args.Generate<ImAnimSolution>();
        }
    }
}
