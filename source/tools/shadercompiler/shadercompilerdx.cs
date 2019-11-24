using System;
using System.Collections;

namespace ShaderCompiler
{
	// Common class for DX11 and DX12
	class ShaderCompilerDX
	{
		private static string GetCompilerPath()
		{
			// Hardcoding path to Windows Kits. No clue if we can detect it or not. Version must match sharpmake project.
			return @"C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\x64\dxc.exe";
		}

		private static string GenerateDefines(HeaderInfo header)
		{
			ArrayList allDefines = new ArrayList();
			allDefines.AddRange(Config.GlobalDefines);

			switch (header.Type)
			{
			case ShaderType.VertexShader:
				allDefines.Add("VERTEX_SHADER");
				break;
			case ShaderType.PixelShader:
				allDefines.Add("PIXEL_SHADER");
				break;
			}

			allDefines.AddRange(header.Defines);

			string defines = "";
			foreach (string define in allDefines)
			{
				defines += "-D\"" + define + "\" ";
			}

			return defines;
		}

		// Processes all headers from a shader file and compiles every permutation
		public static void Compile(HeaderInfo header, ShaderFile shaderFile)
		{
			Console.WriteLine("HEADER:\t\t" + header.DebugPrint());

			string compilerExe = GetCompilerPath();

			// Shader code as header file or binary file
			bool headerFile         = true;
			string exportOption     = headerFile ? "/Fh" : "/Fo";
			string extension        = headerFile ? Config.GeneratedHeaderExtension : ".bin";
			string variableName     = headerFile ? "/Vng_" + header.Name : "";
			string optimization     = "/Od";
			string debugInfo        = "/Zi";
			string defines          = GenerateDefines(header);
			string shaderName       = shaderFile.GetFileName() + "_" + header.Name + "_" + ShaderTypeMethods.ToString(header.Type);
			string shaderOutputFile = Config.GeneratedFolderPath + shaderName + extension;

			// 0: input file, 1: Entry point, 2: Profile, 3: Optimization, 4: Debug info, 5: Export option 6: Output file, 7: Variable name for the header file, 8: Defines
			string args = @"{0} /E{1} /T{2} {3} {4} {5}{6} {7} {8} -nologo";
			args = String.Format(args,
				shaderFile.FullPath,
				header.EntryPoint,
				ShaderTypeMethods.ToString(header.Type) + "_" + Config.ShaderModel,
				optimization,
				Config.EnableDebugInformation ? debugInfo : "",
				exportOption,
				shaderOutputFile,
				variableName,
				defines
				);

			System.Diagnostics.Process process = System.Diagnostics.Process.Start(compilerExe);
			process.StartInfo.RedirectStandardOutput = true;
			process.StartInfo.RedirectStandardError = true;
			process.StartInfo.UseShellExecute = false;
			process.StartInfo.CreateNoWindow = true;
			process.StartInfo.Arguments = args;

			process.Start();

			// To avoid deadlocks, use an asynchronous read operation on at least one of the streams.
			process.StandardOutput.ReadToEnd();

			process.WaitForExit();

			if (process.ExitCode != 0)
			{
				string errorMessage = process.StandardError.ReadToEnd() + "Failed with commandline:\n" + compilerExe + " " + args + "\n\n";
				throw new Exception(errorMessage);
				//throw new Exception(process.StandardOutput.ReadToEnd());
			}

			//Console.WriteLine(process.StandardError.ReadToEnd());
		}
	}
}