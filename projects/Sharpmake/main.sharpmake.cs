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

		// Pixel shader
		ResourceFilesExtensions.Add(".ps");
		ExtensionBuildTools.Add(".ps", "FxCompile");

		// Vertex shader
		ResourceFilesExtensions.Add(".vs");
		ExtensionBuildTools.Add(".vs", "FxCompile");
    }
	
	[Configure()]
	public void ConfigureAll(Configuration conf, Target target)
	{
		conf.ProjectPath = @"[project.RootPath]\projects\[project.Name]\";
		conf.ProjectFileName = @"[project.Name].[target.DevEnv].[target.Platform]";
		conf.IntermediatePath = @"[project.RootPath]\output\temp\[target.DevEnv]\[target.Platform]";
		conf.TargetPath = @"[project.RootPath]\output\[target.Platform]";
		
		conf.IncludePaths.Add(@"[project.SourceRootPath]\");
		
		conf.PrecompHeader = @"[project.Name]_precomp.h";
		conf.PrecompSource = @"[project.Name]_precomp.cpp";
	}
	
	[Configure(Platform.win64)]
	public void ConfigurePC(Configuration conf, Target target)
	{
		// Make this project an Executable
		conf.Options.Add(Options.Vc.Linker.SubSystem.Application);
		
		// Enable C++17
		conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.CPP17);
		
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