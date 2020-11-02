using System;
using System.Collections;

namespace ShaderCompiler
{
	// Class specific to FXC
	class ShaderCompilerFXC : ShaderCompilerDX
	{
		protected override string GetCompilerPath()
		{
			// Hardcoding path to Windows Kits. No clue if we can detect it or not. Version must match sharpmake project.
			return @"C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe";
		}

		protected override string GetCompilerDefine()
		{
			return "COMPILER_FXC";
		}

		protected override string CreateCommand(string command, string arg = null)
		{
			string str = @"/" + command;
			if (arg != null)
			{
				str += "\"" + arg + "\"";
			}

			return " " + str;
		}
	}
}