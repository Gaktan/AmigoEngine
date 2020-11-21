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
		public void Compile(HeaderInfo inHeader, ShaderFile ioShaderFile)
		{
			Console.WriteLine("HEADER:\t\t" + inHeader.GetDebugString());

			// Shader code as header file or binary file
			// TODO: Only header files for now
			bool header_file			= true;
			string export_option		= header_file ? "Fh" : "Fo";

			string cmd_input_file		= ioShaderFile.FullPath;
			string cmd_output_file		= CreateCommand(export_option, inHeader.GetGeneratedFileName(ioShaderFile));
			string cmd_variable_name	= header_file ? CreateCommand("Vn", "g_" + inHeader.Name) : "";
			string cmd_entry_point		= CreateCommand("E", inHeader.EntryPoint);
			string cmd_profile			= CreateCommand("T", EnumUtils.ToDescription(inHeader.Type).ToLower() + "_" + EnumUtils.ToDescription(Config.ShaderModel));
			string cmd_optimization		= CreateCommand("Od");
			string cmd_debug_info		= Config.EnableDebugInformation ? CreateCommand("Zi") : "";
			string cmd_defines			= GenerateDefines(inHeader, Config.ShaderModel);
			string cmd_nologo			= CreateCommand("nologo");

			// Don't output to file in Test. This should just output the shader code in the console
			if (Arguments.Operation == Arguments.OperationType.Test)
			{
				cmd_variable_name = "";
				cmd_output_file = "";
			}

			string args = CombineCommands(cmd_input_file, cmd_output_file, cmd_variable_name, cmd_entry_point, cmd_profile,
				cmd_optimization, cmd_debug_info, cmd_defines, cmd_nologo);

			string compiler_exe = GetCompilerPath();
			Process process = Process.Start(compiler_exe);
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

			ioShaderFile.DidCompile = (process.ExitCode == 0);

			if (!ioShaderFile.DidCompile)
			{
				string errorMessage = process.StandardError.ReadToEnd() + "Failed with commandline:\n" + compiler_exe + " " + args + "\n\n";
				throw new Exception(errorMessage);
				//throw new Exception(process.StandardOutput.ReadToEnd());
			}

			//Console.WriteLine(process.StandardError.ReadToEnd());
		}
	}
}