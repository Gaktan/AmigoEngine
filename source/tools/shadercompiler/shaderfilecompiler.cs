using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace ShaderCompiler
{
	class ShaderCompiler
	{
		// TODO: This is actually in the Windows SDK folder. Use this instead?
		private static string GetDirectXCompilerDirectory()
		{
			return @"..\..\external\DirectXShaderCompiler\bin\dxc.exe";
		}

		static readonly string ShaderModel = "6_0";
		static readonly string GeneratedHeaderExtension = ".generated.h";

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
			switch (shaderType)
			{
			case ShaderType.VertexShader:
				return "vs";
			case ShaderType.PixelShader:
			default:
				return "ps";
			}
		}

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
			// "// ShaderCompiler. Name: Test_01, EntryPoint: main, Type: PS"

			List<HeaderInfo> headerInfos = new List<HeaderInfo>();

			string HeaderRegex = @"//\s*ShaderCompiler\.(.*)";

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
				string Extension = HeaderFile ? GeneratedHeaderExtension : ".bin";
				string VariableName = HeaderFile ? "/Vng_" + header.Name : "";
				string Optimization = "Od";

				string ShaderName = shaderFile.GetFileName() + "_" + header.Name + "_" + ShaderTypeToString(header.Type);
				string ShaderOutputFile = ShaderFileGatherer.ShaderSourcePath + @"\generated\" + ShaderName + Extension;


				// 0: input file, 1: Entry point, 2: Profile, 3: optimization, 4: Export option 5: Output file, 6: Variable name for the header file, 
				string args = @"{0} -Zi /E{1} /T{2} -{3} /{4}{5} {6} -nologo";
				args = String.Format(args,
					shaderFile.FullPath,
					header.EntryPoint,
					ShaderTypeToString(header.Type) + "_" + ShaderModel,
					Optimization,
					ExportOption,
					ShaderOutputFile,
					VariableName
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

		// Will replace anything bewtween Begin{Pattern} and End{Pattern} with Replacement string
		static void ReplaceHeaderFileContent(string Pattern, string Replacement)
		{
			if (!File.Exists(ShaderHeaderFile))
			{
				throw new Exception("File (" + ShaderHeaderFile + ") does not exist.");
			}

			string HeaderFileContent = File.ReadAllText(ShaderHeaderFile);

			string RegexPattern = string.Format(FindBeginEndRegex, Pattern);

			var Reg = new Regex(RegexPattern, RegexOptions.IgnoreCase | RegexOptions.Multiline);
			Match RegMatch = Reg.Match(HeaderFileContent);
			if (!RegMatch.Success)
			{
				throw new Exception("BeginShaderByteCode or EndShaderByteCode were not found in file (" + ShaderHeaderFile + ")");
			}

			string newFileContent = Reg.Replace(HeaderFileContent, "$1" + Environment.NewLine + Replacement + "$3");
			File.WriteAllText(ShaderHeaderFile, newFileContent);
		}

		// Captures anything between Begin{0} (group 1) and End{0} (group 3) in group 2
		static readonly string FindBeginEndRegex = @"(\/\/\s*Begin{0})([\s\S]*)(\/\/\s*End{0})";
		static readonly string ByteArrayNameFromGeneratedHeaderFileRegex = @"const\s*unsigned\s*char\s*g_(.*)\[\]";

		static readonly string ShaderHeaderFile = ShaderFileGatherer.ShaderSourcePath + @"\include\shaders.h";

		public static void GenerateShaderHeaderFile()
		{
			// Search all generated header files (.generated.h) to grab the name of the array inside it and the filename as well
			StringBuilder ShaderByteCodeBuilder = new StringBuilder();
			StringBuilder IncludeBuilder = new StringBuilder();
			foreach (string FileStr in Directory.GetFiles(ShaderFileGatherer.GeneratedFolder))
			{
				if (!FileStr.EndsWith(GeneratedHeaderExtension))
				{
					continue;
				}

				string GeneratedFileContent = File.ReadAllText(FileStr);

				var ByteArrayNameReg = new Regex(ByteArrayNameFromGeneratedHeaderFileRegex, RegexOptions.IgnoreCase);
				Match ByteArrayNameMatch = ByteArrayNameReg.Match(GeneratedFileContent);
				if (!ByteArrayNameMatch.Success)
				{
					throw new Exception("Given header file (" + FileStr + ") does not contain a Byte array. Something is seriously wrong.");
				}

				string ByteArrayName = ByteArrayNameMatch.Groups[1].ToString();

				ShaderByteCodeBuilder.AppendLine("\tINIT_SHADER_BYTECODE(" + ByteArrayName + ");");
				IncludeBuilder.AppendLine("\t#include \"shaders\\generated\\" + Path.GetFileName(FileStr) + "\"");
			}

			ReplaceHeaderFileContent("Include", IncludeBuilder.ToString());
			ReplaceHeaderFileContent("ShaderByteCode", ShaderByteCodeBuilder.ToString());

			/*
				// BeginInclude
				// EndInclude

				#define SHADER_BYTECODE(name) D3D12_SHADER_BYTECODE name # ByteCode = { g_ # name, sizeof(g_ # name) }

				// BeginShaderByteCode
				// EndShaderByteCode
			*/
		}

		static ShaderFile CurrentShaderFile;
	}
}
 