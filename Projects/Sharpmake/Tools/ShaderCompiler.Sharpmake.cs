using System.IO; // for Path.Combine
using Sharpmake; // contains the entire Sharpmake object library.

[Generate]
class ShaderCompiler : CSharpProject
{
	public ShaderCompiler()
	{
		Name = "ShaderCompiler";

		RootPath = @"[project.SharpmakeCsPath]\..\..\..\";
		SourceRootPath = @"[project.RootPath]\Source\Tools\[project.Name]";

		// Add Embedded Resources
		AdditionalEmbeddedResource.Add(@"[project.SourceRootPath]\Resources\DefaultConfig.ini");

		AddTargets(new Target(
			Platform.win64,
			DevEnv.vs2017,
			Optimization.Debug | Optimization.Release,
			OutputType.Lib,
			Blob.NoBlob,
			BuildSystem.MSBuild,
			DotNetFramework.v4_5));

		//PreserveLinkFolderPaths = true;
	}
	
	[Configure()]
	public void ConfigureAll(Configuration conf, Target target)
	{
		conf.ProjectPath = @"[project.RootPath]\Projects\Tools\";
		conf.ProjectFileName = @"[project.Name]";
		conf.IntermediatePath = @"[project.RootPath]\Output\Temp\[target.DevEnv]\[target.Platform]\[target.Optimization]\[project.Name]";
		conf.TargetPath = @"[project.RootPath]\Tools\[project.Name]";

		conf.Options.Add(Sharpmake.Options.CSharp.TreatWarningsAsErrors.Enabled);
		
		// Make this project an Executable without console
		conf.Output = Project.Configuration.OutputType.DotNetWindowsApp;
		
		// TODO: Create a filter "Tools" in the solution
		//conf.SolutionFolder = conf.ProjectPath;
		
		// Add a C# reference, kind of wordy if you ask me
		conf.DotNetReferences.Add(new DotNetReference("System", DotNetReference.ReferenceType.DotNet));
	}
}