using System.IO; // for Path.Combine
using Sharpmake; // contains the entire Sharpmake object library.

[Generate]
class AmigoEngine : Project
{
    public AmigoEngine()
    {
        Name = "Engine";

		RootPath = @"[project.SharpmakeCsPath]\..\..\";
		SourceRootPath = @"[project.RootPath]\source\engine";

        AddTargets(new Target(
            Platform.win64,
            DevEnv.vs2017,
            Optimization.Debug | Optimization.Release,
			OutputType.Lib,
			Blob.NoBlob,
			BuildSystem.MSBuild));
			
		ResourceFilesExtension.Add(".hlsl");
    }
	
	[Configure()]
	public void ConfigureAll(Configuration conf, Target target)
	{
		conf.ProjectPath = @"[project.SharpmakeCsPath]\..\[project.Name]\";
		conf.ProjectFileName = @"[project.Name].[target.DevEnv].[target.Platform]";
		conf.IntermediatePath = @"[conf.ProjectPath]\temp\[target.DevEnv]\[target.Platform]\[target]";
		
		conf.IncludePaths.Add(@"[project.SourceRootPath]\");
		
		conf.PrecompHeader = "engine_precomp.h";
		conf.PrecompSource = "engine_precomp.cpp";
	}
	
	[Configure(Platform.win64)]
	public void ConfigurePC(Configuration conf, Target target)
	{
		conf.Options.Add(Options.Vc.Linker.SubSystem.Application);
		
		// DX12
		conf.IncludePaths.Add(@"[project.RootPath]\external\d3d12\include");
		//conf.LibraryPaths.Add(@"[project.BasePath]\lib");
		conf.LibraryFiles.Add(@"D3DCompiler.lib");
		conf.LibraryFiles.Add(@"D3D12.lib");
		conf.LibraryFiles.Add(@"DXGI.lib");
	}
	
	[Generate]
    public class AmigoSolution : Solution
    {
        public AmigoSolution()
        {
            Name = "Amigo";
			
			AddTargets(new Target(
				Platform.win64,
				DevEnv.vs2017,
				Optimization.Debug | Optimization.Release));
        }

        [Configure]
        public void ConfigureAll(Configuration conf, Target target)
        {
			// Sets proper Windows Kits version
			KitsRootPaths.SetUseKitsRootForDevEnv(
				target.DevEnv,
				KitsRootEnum.KitsRoot10,
				Options.Vc.General.WindowsTargetPlatformVersion.v10_0_17763_0);

            conf.SolutionFileName = "[solution.Name]";
            conf.SolutionPath = @"[solution.SharpmakeCsPath]\..\..\";

            conf.AddProject<AmigoEngine>(target);
        }
    }
	
	public static class Main
    {
        [Sharpmake.Main]
        public static void SharpmakeMain(Arguments arguments)
        {
            arguments.Generate<AmigoSolution>();
        }
    }
}