using System.IO; // for Path.Combine
using Sharpmake; // contains the entire Sharpmake object library.

[module: Sharpmake.Include("Tools/ShaderCompiler.Sharpmake.cs")]

[Generate]
class AmigoEngine : Project
{
	public AmigoEngine()
	{
		Name = "Engine";

		RootPath = @"[project.SharpmakeCsPath]\..\..";
		SourceRootPath = @"[project.RootPath]\Source\Engine";

		// Shader files
		SourceFilesExtensions.Add(".hlsl");
		SourceFilesExcludeRegex.Add(@".*\.generated\.h");

		AddTargets(new Target(
			Platform.win64,
			DevEnv.vs2017,
			Optimization.Debug | Optimization.Release,
			OutputType.Lib,
			Blob.NoBlob,
			BuildSystem.MSBuild,
			DotNetFramework.v4_5));

		// if set to true, dependencies that the project uses will be copied to the output directory
		DependenciesCopyLocal = DependenciesCopyLocalTypes.None;
	}

	[Configure()]
	public void ConfigureAll(Configuration conf, Target target)
	{
		conf.ProjectPath		= @"[project.RootPath]\Projects\[project.Name]\";
		conf.ProjectFileName	= @"[project.Name].[target.DevEnv].[target.Platform]";
		conf.IntermediatePath	= @"[project.RootPath]\Output\Temp\[target.DevEnv]\[target.Platform]";
		conf.TargetPath			= @"[project.RootPath]\Output\[target.Platform]";

		conf.IncludePaths.Add(@"[project.SourceRootPath]\");

		conf.PrecompHeader = @"[project.Name].h";
		conf.PrecompSource = @"[project.Name].cpp";

		// Set default Working directory
		conf.VcxprojUserFile = new Sharpmake.Project.Configuration.VcxprojUserFileSettings();
		conf.VcxprojUserFile.LocalDebuggerWorkingDirectory = "$(SolutionDir)";
	}

	[Configure(Platform.win64)]
	public void ConfigurePC(Configuration conf, Target target)
	{
		// Make this project an Executable
		conf.Options.Add(Options.Vc.Linker.SubSystem.Application);

		// Enable C++17
		conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.CPP17);
		
		// Enable RTTI
		conf.Options.Add(Options.Vc.Compiler.RTTI.Enable);

		if (target.Optimization == Optimization.Debug)
			conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebugDLL);
		else
			conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);

		// Exception handling in std lib
		conf.Defines.Add("_HAS_EXCEPTIONS=0");

		// DX12
		conf.IncludePaths.Add(@"[project.RootPath]\External\D3D12\Include");
		conf.LibraryFiles.Add(@"D3D12.lib");
		conf.LibraryFiles.Add(@"DXGI.lib");
		conf.LibraryFiles.Add(@"dxguid.lib");
		
		// DirectXTex
		conf.IncludePaths.Add(@"[project.RootPath]\External\DirectXTex");
		string target_configuration = (target.Optimization == Optimization.Debug) ? "Debug" : "Release";
		conf.LibraryPaths.Add(@"[project.RootPath]\External\DirectXTex\DirectXTex\Bin\Desktop_2017\x64\" + target_configuration);
		conf.LibraryFiles.Add(@"DirectXTex.lib");

		// Add dependency to ShaderCompiler
		conf.AddPrivateDependency<ShaderCompiler>(target, DependencySetting.OnlyBuildOrder);

		// Compiler Shaders during Pre-Build event
		conf.EventPreBuild.Add(@"""[project.RootPath]\Tools\ShaderCompiler\ShaderCompiler.exe"" -Build -c Config.ini -r ""[project.SourceRootPath]\shaders""");
	}
}

[Generate]
public class AmigoSolution : Solution
{
	public AmigoSolution()
	{
		Name = "Amigo";

		Target target = new Target(
			Platform.win64,
			DevEnv.vs2017,
			Optimization.Debug | Optimization.Release);
		target.Framework = DotNetFramework.v4_5;

		AddTargets(target);
	}

	[Configure()]
	public void ConfigureAll(Configuration conf, Target target)
	{
		// Sets proper Windows Kits version
		KitsRootPaths.SetUseKitsRootForDevEnv(
			target.DevEnv,
			KitsRootEnum.KitsRoot10,
			Options.Vc.General.WindowsTargetPlatformVersion.v10_0_19041_0);

		conf.SolutionFileName = "[solution.Name]";
		conf.SolutionPath = @"[solution.SharpmakeCsPath]\..\..\";

		conf.AddProject<AmigoEngine>(target);
		conf.AddProject<ShaderCompiler>(target);
	}
	
	[Sharpmake.Main]
	public static void SharpmakeMain(Arguments arguments)
	{
		arguments.Generate<AmigoSolution>();
	}
}
