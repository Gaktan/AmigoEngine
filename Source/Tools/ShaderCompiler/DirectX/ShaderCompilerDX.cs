using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace ShaderCompiler
{
	// Common class for DX11 and DX12
	abstract class ShaderCompilerDX
	{
		protected abstract string GetCompilerPath();
		protected abstract string CreateCommand(string command, string arg = null);
		protected abstract string GetCompilerDefine();

		private string GenerateDefines(HeaderInfo header, ShaderModel shaderModel)
		{
			List<string> allDefines = new List<string>();
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

			allDefines.Add("SHADER_MODEL=" + ShaderModelMethods.ToInt(shaderModel));
			allDefines.Add("LANGUAGE_HLSL");
			allDefines.Add(GetCompilerDefine());

			string defines = "";
			foreach (string define in allDefines)
			{
				defines += CreateCommand("D", define.Trim());
			}

			return defines;
		}

		private string CombineCommands(params string[] commands)
		{
			string result = "";
			foreach (string command in commands)
			{
				result += command;
			}

			return result;
		}

		// Processes all headers from a shader file and compiles every permutation
		public void Compile(HeaderInfo header, ShaderFile shaderFile)
		{
			Console.WriteLine("HEADER:\t\t" + header.DebugPrint());

			// Shader code as header file or binary file
			bool headerFile			= true;
			string exportOption		= headerFile ? "Fh" : "Fo";
			string outputExtension	= headerFile ? Config.GeneratedHeaderExtension : ".bin";
			string variableName		= headerFile ? CreateCommand("Vn", "g_" + header.Name) : "";

			string shaderName		= shaderFile.GetFileName() + "_" + header.Name + "_" + EnumUtils.ToDescription(header.Type);
			string shaderOutputFile	= Config.GeneratedFolderPath + shaderName + outputExtension;

			string inputFile		= shaderFile.FullPath;
			string outputFile		= CreateCommand(exportOption, shaderOutputFile);
			string entryPoint		= CreateCommand("E", header.EntryPoint);
			string profile			= CreateCommand("T", EnumUtils.ToDescription(header.Type).ToLower() + "_" + EnumUtils.ToDescription(Config.ShaderModel));
			string optimization		= CreateCommand("Od");
			string debugInfo		= Config.EnableDebugInformation ? CreateCommand("Zi") : "";
			string defines			= GenerateDefines(header, Config.ShaderModel);
			string nologo			= CreateCommand("nologo");

			// Don't output to file in Test. This should just output the shader code in the console
			if (Arguments.Operation == Arguments.OperationType.Test)
			{
				variableName = "";
				outputFile = "";
			}

			string args = CombineCommands(inputFile, outputFile, entryPoint, profile, optimization, debugInfo, variableName, defines, nologo);

			string compilerExe = GetCompilerPath();
			Process process = Process.Start(compilerExe);
			process.StartInfo.RedirectStandardOutput	= true;
			process.StartInfo.RedirectStandardError		= true;
			process.StartInfo.UseShellExecute			= false;
			process.StartInfo.CreateNoWindow			= true;
			process.StartInfo.WindowStyle				= ProcessWindowStyle.Hidden;
			process.StartInfo.Arguments					= args;

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