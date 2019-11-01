using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace ShaderCompiler
{
	class ShaderCompiler
	{
		// Captures any line that starts with // ShaderCompiler and puts the content of the header in Group1
		private static readonly string HeaderRegex = @"//\s*ShaderCompiler\.(.*)";
		// Reads a single tag from a header and puts the result in Group1
		private static readonly string HeaderTagRegex = @"\s*:\s*(.*?)\s*(,|$)";
		// Captures anything between Begin{0} (Group1) and End{0} (Group3) in Group2
		private static readonly string FindBeginEndRegex = @"(\/\/\s*Begin{0})([\s\S]*)(\/\/\s*End{0})";
		// Captures the name of const unsigned char g_ array. Puts the result in Group1
		private static readonly string ByteArrayNameFromGeneratedHeaderFileRegex = @"const\s*unsigned\s*char\s*g_(.*)\[\]";

		// TODO: This is actually in the Windows SDK folder. Use this instead?
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
			public List<string> Defines;

			public string DebugPrint()
			{
				string definesString = ", Defines:" + String.Join(", ", Defines.ToArray()) + ".";
				return "Name: " + Name + ", EntryPoint: " + EntryPoint + ", Type: " + Type + (Defines.Count==0 ? "." : definesString);
			}

			// Returns the value of a tag from a given header string
			private static string ReadTagFromHeader(string tag, string str, int lineNum, string defaultValue = null)
			{
				// (tag), {return}
				// ShaderCompiler. (Name): {ShaderFileToTest_01}, (EntryPoint): {main}, (Type): {PS}, (Defines): DEFINE1;DEFINE2=1;

				string tagRegex = tag + HeaderTagRegex;

				Regex tagReg = new Regex(tagRegex, RegexOptions.IgnoreCase);
				Match tagMatch = tagReg.Match(str);
				if (tagMatch.Success)
				{
					return tagMatch.Groups[1].ToString();
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
				header.Name			= ReadTagFromHeader("Name", str, lineNum);
				header.EntryPoint	= ReadTagFromHeader("EntryPoint", str, lineNum, "main");
				header.Type			= ShaderTypeFromString(ReadTagFromHeader("Type", str, lineNum, "ps"));
				header.Defines		= new List<string>();

				string defines = ReadTagFromHeader("Defines", str, lineNum, "");
				foreach (string define in defines.Split(new[] { ";" }, StringSplitOptions.RemoveEmptyEntries))
				{
					header.Defines.Add(define);
				}

				return header;
			}
		}

		private static List<HeaderInfo> ReadHeader(string fileContent)
		{
			// Example of a shader header:
			// "// ShaderCompiler. Name: Test_01, EntryPoint: main, Type: PS, Defines: DEFINE1; DEFINE2 = 0"

			List<HeaderInfo> headerInfos = new List<HeaderInfo>();

			// For each line
			using (StringReader reader = new StringReader(fileContent))
			{
				string line;
				int lineNum = 0;
				while ((line = reader.ReadLine()) != null)
				{
					Regex headerReg = new Regex(HeaderRegex, RegexOptions.IgnoreCase);
					Match headerMatch = headerReg.Match(line);
					if (headerMatch.Success)
					{
						string str = headerMatch.Groups[1].ToString();
						headerInfos.Add(HeaderInfo.FromString(str, lineNum));
					}

					lineNum++;
				}
			}

			return headerInfos;
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
		public static void Compile(ShaderFile shaderFile)
		{
			CurrentShaderFile = shaderFile;
			List<HeaderInfo> headers = ReadHeader(shaderFile.Content);

			foreach (HeaderInfo header in headers)
			{
				Console.WriteLine("HEADER:\t\t" + header.DebugPrint());

				string DXC = GetDirectXCompilerDirectory();

				// Shader code as header file or binary file
				bool headerFile			= true;
				string exportOption		= headerFile ? "/Fh" : "/Fo";
				string extension		= headerFile ? Config.GeneratedHeaderExtension : ".bin";
				string variableName		= headerFile ? "/Vng_" + header.Name : "";
				string optimization		= "/Od";
				string debugInfo		= "/Zi";
				string defines			= GenerateDefines(header);
				string shaderName		= shaderFile.GetFileName() + "_" + header.Name + "_" + ShaderTypeToString(header.Type);
				string shaderOutputFile = Config.GeneratedFolderPath + shaderName + extension;

				// 0: input file, 1: Entry point, 2: Profile, 3: Optimization, 4: Debug info, 5: Export option 6: Output file, 7: Variable name for the header file, 8: Defines
				string args = @"{0} /E{1} /T{2} {3} {4} {5}{6} {7} {8} -nologo";
				args = String.Format(args,
					shaderFile.FullPath,
					header.EntryPoint,
					ShaderTypeToString(header.Type) + "_" + Config.ShaderModel,
					optimization,
					Config.EnableDebugInformation ? debugInfo : "",
					exportOption,
					shaderOutputFile,
					variableName,
					defines
					);

				System.Diagnostics.Process process = System.Diagnostics.Process.Start(DXC);
				process.StartInfo.RedirectStandardOutput	= true;
				process.StartInfo.RedirectStandardError		= true;
				process.StartInfo.UseShellExecute			= false;
				process.StartInfo.CreateNoWindow			= true;
				process.StartInfo.Arguments					= args;

				process.Start();

				// To avoid deadlocks, use an asynchronous read operation on at least one of the streams.
				process.StandardOutput.ReadToEnd();

				process.WaitForExit();

				if (process.ExitCode != 0)
				{
					string errorMessage = process.StandardError.ReadToEnd() + "Failed with commandline:\n" + DXC + " " + args + "\n\n";
					throw new Exception(errorMessage);
					//throw new Exception(process.StandardOutput.ReadToEnd());
				}

				//Console.WriteLine(process.StandardError.ReadToEnd());
			}
		}

		// Will replace anything bewtween Begin{Pattern} and End{Pattern} with Replacement string
		static void ReplaceHeaderFileContent(string Pattern, string Replacement)
		{
			// TODO: hardcoded for now
			string shaderHeaderFile = Config.ShaderSourcePath + @"\include\shaders.h";

			if (!File.Exists(shaderHeaderFile))
			{
				throw new Exception("File (" + shaderHeaderFile + ") does not exist.");
			}

			string headerFileContent = File.ReadAllText(shaderHeaderFile);

			string regexPattern = string.Format(FindBeginEndRegex, Pattern);

			Regex regex = new Regex(regexPattern, RegexOptions.IgnoreCase | RegexOptions.Multiline);
			Match regMatch = regex.Match(headerFileContent);
			if (!regMatch.Success)
			{
				throw new Exception("BeginShaderByteCode or EndShaderByteCode were not found in file (" + shaderHeaderFile + ")");
			}

			string newFileContent = regex.Replace(headerFileContent, "$1" + Environment.NewLine + Replacement + "$3");

			// Before overwritting the file, check if the content is actually different.
			// This will prevent recompiling if the content was not actually changed.
			if (headerFileContent != newFileContent)
			{
				File.WriteAllText(shaderHeaderFile, newFileContent);
			}
		}

		public static void GenerateShaderHeaderFile()
		{
			// Search all generated header files (.generated.h) to grab the name of the array inside it and the filename as well
			StringBuilder shaderByteCodeBuilder = new StringBuilder();
			StringBuilder includeBuilder = new StringBuilder();
			foreach (string fileStr in Directory.GetFiles(Config.GeneratedFolderPath))
			{
				// Don't process non shader compiled files
				if (!fileStr.EndsWith(Config.GeneratedHeaderExtension))
				{
					continue;
				}

				string GeneratedFileContent = File.ReadAllText(fileStr);

				// Look for the byte array containing the shader code
				Regex byteArrayNameReg = new Regex(ByteArrayNameFromGeneratedHeaderFileRegex, RegexOptions.IgnoreCase);
				Match byteArrayNameMatch = byteArrayNameReg.Match(GeneratedFileContent);
				if (!byteArrayNameMatch.Success)
				{
					throw new Exception("Given header file (" + fileStr + ") does not contain a Byte array. Something is seriously wrong.");
				}

				string ByteArrayName = byteArrayNameMatch.Groups[1].ToString();

				shaderByteCodeBuilder.AppendLine("\tINIT_SHADER_BYTECODE(" + ByteArrayName + ");");
				includeBuilder.AppendLine("\t#include \"shaders\\generated\\" + Path.GetFileName(fileStr) + "\"");
			}

			ReplaceHeaderFileContent("Include", includeBuilder.ToString());
			ReplaceHeaderFileContent("ShaderByteCode", shaderByteCodeBuilder.ToString());
		}

		static ShaderFile CurrentShaderFile;
	}
}
