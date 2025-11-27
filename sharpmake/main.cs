using Sharpmake;

[module: Sharpmake.Include("common.cs")]
[module: Sharpmake.Include("ImAnimLib.cs")]
[module: Sharpmake.Include("ImAnimDemo.cs")]

namespace ImAnim
{
    [Generate]
    public class ImAnimSolution : Solution
    {
        public ImAnimSolution()
        {
            Name = "ImAnim";
            IsFileNameToLower = false;

            AddTargets(new Target(
                Platform.win64,
                DevEnv.vs2022,
                Optimization.Debug | Optimization.Release
            ));
        }

        [Configure]
        public void ConfigureAll(Configuration conf, Target target)
        {
            conf.SolutionFileName = "[solution.Name]_[target.DevEnv]_[target.Platform]";
            conf.SolutionPath = @"[solution.SharpmakeCsPath]\..";

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
