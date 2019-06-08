using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using SharpDX.D3DCompiler;

namespace ShaderCompiler
{
	class ShaderCompiler
	{
		struct HeaderInfo
		{
			public string Name;
			public string EntryPoint;
			public string Type;

			public string DebugPrint()
			{
				return "Name: " + Name + ", EntryPoint: " + EntryPoint + ", Type: " + Type + ".";
			}

			private static string ShaderTypeFromString(string str)
			{
				switch(str.ToLower())
				{
				case "vs":
				case "vertex":
					return "vs_5_0";
				case "ps":
				case "pixel":
				case "fragment":
				default:
					return "ps_5_0";
				}
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
					throw new Exception("Missing tag \"" + tag + "\" in ShaderCompilerHeader.\nIn " + CurrentShaderFile.Name + ":line " + lineNum);
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

				ShaderBytecode bytecode = ShaderBytecode.Compile(shaderFile.Content, header.EntryPoint, header.Type);
			}

			//ShaderBytecode bytecode = ShaderBytecode.Compile(shaderFile.Content, "main", "vs_5_0");
		}

		static ShaderFile CurrentShaderFile;
	}
}