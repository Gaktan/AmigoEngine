using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace ShaderCompiler
{
	class ShaderCompiler
	{
		private static string GetDirectXCompilerDirectory()
		{
			return @"..\..\external\DirectXShaderCompiler\bin\dxc.exe";
		}

		enum ShaderType
		{
			VertexShader,
			PixelShader
		};

		private static ShaderType ShaderTypeFromString(string str)
		{
			switch (str.ToLower())
			{
			case "vs":
			case "vertex":
				return ShaderType.VertexShader;
			case "ps":
			case "pixel":
			case "fragment":
			default:
				return ShaderType.PixelShader;
			}
		}

		private static string ShaderTypeToString(ShaderType shaderType)
		{
			switch(shaderType)
			{
			case ShaderType.VertexShader:
				return "vs";
			case ShaderType.PixelShader:
			default:
				return "ps";
			}
		}

		private static readonly string ShaderModel = "6_0";

		struct HeaderInfo
		{
			public string Name;
			public string EntryPoint;
			public ShaderType Type;

			public string DebugPrint()
			{
				return "Name: " + Name + ", EntryPoint: " + EntryPoint + ", Type: " + Type + ".";
			}

			private static string GetInfoFromHeader(string tag, string str, int lineNum, string defaultValue = null)
			{
				// (Tag), {return}
				// ShaderCompiler. (Name): {ShaderFileToTest_01}, EntryPoint: main, Type: PS

				string TagRegex = tag + @"\s*:\s*([a-z_][a-z0-9_]*)\s*,*";

				var TagReg = new Regex(TagRegex, RegexOptions.IgnoreCase);
				Match TagMatch = TagReg.Match(str);
				if (TagMatch.Success)
				{
					return TagMatch.Groups[1].ToString();
				}
				else if (defaultValue != null)
				{
					return defaultValue;
				}
				else
				{
					throw new Exception("Missing tag \"" + tag + "\" in ShaderCompilerHeader.\nIn " + CurrentShaderFile.FullPath + ":line " + lineNum);
				}
			}

			public static HeaderInfo FromString(string str, int lineNum)
			{
				HeaderInfo header;
				header.Name = GetInfoFromHeader("Name", str, lineNum);
				header.EntryPoint = GetInfoFromHeader("EntryPoint", str, lineNum, "main");
				header.Type = ShaderTypeFromString(GetInfoFromHeader("Type", str, lineNum, "ps"));

				return header;
			}
		}

		private static List<HeaderInfo> ReadHeader(string fileContent)
		{
			// Example of a shader header:
			// "// ShaderCompiler. Name: ShaderFileToTest_01, EntryPoint: main, Type: PS"

			List<HeaderInfo> headerInfos = new List<HeaderInfo>();

			string HeaderRegex = @"// *ShaderCompiler\.(.*)";

			// For each line
			using (StringReader reader = new StringReader(fileContent))
			{
				string line;
				int lineNum = 0;
				while ((line = reader.ReadLine()) != null)
				{
					var HeaderReg = new Regex(HeaderRegex, RegexOptions.IgnoreCase);
					Match HeaderMatch = HeaderReg.Match(line);
					if (HeaderMatch.Success)
					{
						string str = HeaderMatch.Groups[1].ToString();
						headerInfos.Add(HeaderInfo.FromString(str, lineNum));
					}

					lineNum++;
				}
			}

			return headerInfos;
		}

		public static void Compile(ShaderFile shaderFile)
		{
			CurrentShaderFile = shaderFile;
			List<HeaderInfo> headers = ReadHeader(shaderFile.Content);

			foreach (HeaderInfo header in headers)
			{
				Console.WriteLine("HEADER:\t\t" + header.DebugPrint());

				string DXC = GetDirectXCompilerDirectory();

				// Shader code as header file or binary file
				bool HeaderFile = true;
				string ExportOption = HeaderFile ? "Fh" : "Fo";
				string Extension = HeaderFile ? ".h" : ".bin";

				string ShaderName = shaderFile.GetFileName() + "_" + header.Name + "_" + header.Type;
				string ShaderOutputFile = ShaderFileGatherer.ShaderSourcePath + @"\generated\" + ShaderName + Extension;
				string VariableName = "g_" + header.Name;
				string Optimization = "Od";

				// 0: input file, 1: Entry point, 2: Profile, 3: optimization, 4: Export option 5: Output file, 6: Variable name for the header file, 
				string args = @"{0} -Zi /E{1} /T{2} -{3} /{4}{5} {6} -nologo";
				args = String.Format(args,
					shaderFile.FullPath,
					header.EntryPoint,
					ShaderTypeToString(header.Type) + "_" + ShaderModel,
					Optimization,
					ExportOption,
					ShaderOutputFile,
					HeaderFile ? "/Vn" + VariableName : ""
					);

				System.Diagnostics.Process process = System.Diagnostics.Process.Start(DXC);
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
					throw new Exception(process.StandardError.ReadToEnd());
					//throw new Exception(process.StandardOutput.ReadToEnd());
				}
				
				//Console.WriteLine(process.StandardError.ReadToEnd());
			}
		}

		static ShaderFile CurrentShaderFile;
	}
}