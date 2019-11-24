using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace ShaderCompiler
{
	enum ShaderType
	{
		VertexShader,
		PixelShader
	};

	static class ShaderTypeMethods
	{
		public static ShaderType FromString(string str)
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

		public static string ToString(ShaderType shaderType)
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
			header.Type			= ShaderTypeMethods.FromString(ReadTagFromHeader("Type", str, "ps"));
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
		// Captures the name of const unsigned char g_ array. Puts the result in Group1
		private static readonly string ByteArrayNameFromGeneratedHeaderFileRegex = @"const\s*unsigned\s*char\s*g_(.*)\[\]";

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
					Regex headerReg = new Regex(HeaderRegex, RegexOptions.IgnoreCase);
					Match headerMatch = headerReg.Match(line);
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
			List<HeaderInfo> headers = ReadHeader(shaderFile);

			foreach (HeaderInfo header in headers)
			{
				ShaderCompilerDX.Compile(header, shaderFile);
			}
		}

		// Will replace anything bewtween Begin{Pattern} and End{Pattern} with Replacement string
		static void ReplaceHFileContent(string Pattern, string Replacement)
		{
			string shaderHFile = Config.ShaderHFile;

			if (!File.Exists(shaderHFile))
			{
				throw new Exception("File (" + shaderHFile + ") does not exist.");
			}

			string headerFileContent = File.ReadAllText(shaderHFile);

			string regexPattern = string.Format(FindBeginEndRegex, Pattern);

			Regex regex = new Regex(regexPattern, RegexOptions.IgnoreCase | RegexOptions.Multiline);
			Match regMatch = regex.Match(headerFileContent);
			if (!regMatch.Success)
			{
				throw new Exception("BeginShaderByteCode or EndShaderByteCode were not found in file (" + shaderHFile + ")");
			}

			string newFileContent = regex.Replace(headerFileContent, "$1" + Environment.NewLine + Replacement + "$3");

			// Before overwritting the file, check if the content is actually different.
			// This will prevent recompiling if the content was not actually changed.
			if (headerFileContent != newFileContent)
			{
				File.WriteAllText(shaderHFile, newFileContent);
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

			ReplaceHFileContent("Include", includeBuilder.ToString());
			ReplaceHFileContent("ShaderByteCode", shaderByteCodeBuilder.ToString());
		}
	}
}
