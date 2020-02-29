using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace ShaderCompiler
{
	enum ShaderType
	{
		[Description("VS")]
		VertexShader,
		[Description("PS")]
		PixelShader
	};

	enum ShaderModel
	{
		[Description("5_0")]
		_5_0,
		[Description("5_1")]
		_5_1,
		[Description("6_0")]
		_6_0,
		[Description("6_1")]
		_6_1,
		[Description("6_2")]
		_6_2,
		[Description("6_3")]
		_6_3,
	};

	enum Compiler
	{
		FXC,
		DXC
	};

	static class ShaderModelMethods
	{
		public static float ToInt(ShaderModel shaderModel)
		{
			switch (shaderModel)
			{
			case ShaderModel._5_0:
				return 50;
			case ShaderModel._5_1:
				return 51;
			case ShaderModel._6_0:
				return 60;
			case ShaderModel._6_1:
				return 61;
			case ShaderModel._6_2:
				return 62;
			case ShaderModel._6_3:
				return 63;
			default:
				throw new Exception("Invalid ShaderModel \"" + shaderModel + "\"");
			}
		}
	};

	class MissingHeaderTagException : Exception
	{
		public string TagMissing;

		public MissingHeaderTagException(string tag)
		{
			TagMissing = tag;
		}
	}

	struct HeaderInfo
	{
		public string Name;
		public string EntryPoint;
		public ShaderType Type;
		public List<string> Defines;

		// Reads a single tag from a header and puts the result in Group1
		private static readonly string HeaderTagRegex = @"\s*:\s*(.*?)\s*(,|$)";

		public string DebugPrint()
		{
			string definesString = ", Defines:" + String.Join(", ", Defines.ToArray()) + ".";
			return "Name: " + Name + ", EntryPoint: " + EntryPoint + ", Type: " + Type + (Defines.Count == 0 ? "." : definesString);
		}

		// Returns the value of a tag from a given header string
		private static string ReadTagFromHeader(string tag, string str, string defaultValue = null)
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
				throw new MissingHeaderTagException(tag);
			}
		}

		public static HeaderInfo FromString(string str)
		{
			HeaderInfo header;
			header.Name			= ReadTagFromHeader("Name", str);
			header.EntryPoint	= ReadTagFromHeader("EntryPoint", str, "main");
			header.Type			= EnumUtils.FromDescription<ShaderType>(ReadTagFromHeader("Type", str, "ps"));
			header.Defines		= new List<string>();

			string defines = ReadTagFromHeader("Defines", str, "");
			foreach (string define in defines.Split(new[] { ";" }, StringSplitOptions.RemoveEmptyEntries))
			{
				header.Defines.Add(define);
			}

			return header;
		}
	}

	class ShaderCompiler
	{
		// Captures any line that starts with // ShaderCompiler and puts the content of the header in Group1
		private static readonly string HeaderRegex = @"//\s*ShaderCompiler\.(.*)";
		// Captures anything between Begin{0} (Group1) and End{0} (Group3) in Group2
		private static readonly string FindBeginEndRegex = @"(\/\/\s*Begin{0})([\s\S]*)(\/\/\s*End{0})";
		// Captures the name of "const unsigned char" (or "const BYTE" in the case of FXC) g_ array. Puts the result in Group4
		private static readonly string ByteArrayNameFromGeneratedHeaderFileRegex = @"const\s*((unsigned\s*char)|(BYTE))\s*g_(.*)\[\]";

		private static List<HeaderInfo> ReadHeader(ShaderFile shaderFile)
		{
			// Example of a shader header:
			// "// ShaderCompiler. Name: Test_01, EntryPoint: main, Type: PS, Defines: DEFINE1; DEFINE2 = 0"

			List<HeaderInfo> headerInfos = new List<HeaderInfo>();

			// For each line
			using (StringReader reader = new StringReader(shaderFile.Content))
			{
				string line;
				int lineNum = 1;
				while ((line = reader.ReadLine()) != null)
				{
					Match headerMatch = Regex.Match(line, HeaderRegex, RegexOptions.IgnoreCase);
					if (headerMatch.Success)
					{
						try
						{
							string str = headerMatch.Groups[1].ToString();
							headerInfos.Add(HeaderInfo.FromString(str));
						}
						catch (MissingHeaderTagException e)
						{
							throw new Exception("Missing tag \"" + e.TagMissing + "\" in header. " + shaderFile.FullPath + "(" + lineNum + ")");
						}
					}

					lineNum++;
				}
			}

			return headerInfos;
		}

		// Processes all headers from a shader file and compiles every permutation
		public static void Compile(ShaderFile shaderFile)
		{
			ShaderCompilerDX shaderCompiler;

			switch (Config.Compiler)
			{
			case Compiler.FXC:
				shaderCompiler = new ShaderCompilerFXC();
				break;
			case Compiler.DXC:
				shaderCompiler = new ShaderCompilerDXC();
				break;
			default:
				throw new Exception("Uknown compiler \"" + Config.Compiler + "\"");
			}

			List<HeaderInfo> headers = ReadHeader(shaderFile);
			foreach (HeaderInfo header in headers)
			{
				shaderCompiler.Compile(header, shaderFile);
			}
		}

		// Will replace anything in the input file bewtween Begin{inPattern} and End{inPattern} with inReplacement string
		static void ReplaceFileContent(string inFile, string inPattern, string inReplacement)
		{
			if (!File.Exists(inFile))
			{
				throw new Exception("File (" + inFile + ") does not exist.");
			}

			string file_content = File.ReadAllText(inFile);

			string regexPattern = string.Format(FindBeginEndRegex, inPattern);
			Regex regex = new Regex(regexPattern, RegexOptions.IgnoreCase | RegexOptions.Multiline);
			Match regMatch = regex.Match(file_content);
			if (!regMatch.Success)
			{
				throw new Exception("BeginShaderByteCode or EndShaderByteCode were not found in file (" + inFile + ")");
			}

			string hew_file_content = regex.Replace(file_content, "$1" + Environment.NewLine + inReplacement + "$3");

			// Before overwritting the file, check if the content is actually different.
			// This will prevent recompiling if the content was not actually changed.
			if (file_content != hew_file_content)
			{
				File.WriteAllText(inFile, hew_file_content);
			}
		}

		public static void GenerateShaderHFile()
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
				Match byteArrayNameMatch = Regex.Match(GeneratedFileContent, ByteArrayNameFromGeneratedHeaderFileRegex, RegexOptions.IgnoreCase);
				if (!byteArrayNameMatch.Success)
				{
					throw new Exception("Given header file (" + fileStr + ") does not contain a Byte array. Something is seriously wrong.");
				}

				string ByteArrayName = byteArrayNameMatch.Groups[4].ToString();

				shaderByteCodeBuilder.AppendLine("\tINIT_SHADER_BYTECODE(" + ByteArrayName + ");");
				includeBuilder.AppendLine("\t#include \"Shaders\\generated\\" + Path.GetFileName(fileStr) + "\"");
			}

			ReplaceFileContent(Config.ShaderHFile, "Include", includeBuilder.ToString());
			ReplaceFileContent(Config.ShaderHFile, "ShaderByteCode", shaderByteCodeBuilder.ToString());
		}

		public static void GenerateConstantBufferHFile(List<Struct> inStructs)
		{
			StringBuilder constantBuffersBuilder = new StringBuilder();
			foreach (Struct sr in inStructs)
			{
				string constant_buffer = sr.PrintAsConstantBuffer();
				if (constant_buffer != null)
					constantBuffersBuilder.AppendLine(constant_buffer);
			}

			string constant_buffer_string = constantBuffersBuilder.ToString().TrimEnd() + Environment.NewLine;

			ReplaceFileContent(Config.ConstantBufferHFile, "ConstantBuffer", constant_buffer_string);
		}
	}
}
