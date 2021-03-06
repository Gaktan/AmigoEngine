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

		public string GetDebugString()
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

		public string GetGeneratedFileName(ShaderFile inShaderFile)
		{
			// TODO: Only header files for now
			bool header_file			= true;
			string output_extension		= header_file ? Config.GeneratedHeaderExtension : ".bin";
			string shader_name			= inShaderFile.GetFileName() + "_" + Name + "_" + EnumUtils.ToDescription(Type);
			string shader_output_file	= Path.Combine(Config.GeneratedFolderPath, shader_name) + output_extension;

			return shader_output_file;
		}
	}

	class ShaderCompiler
	{
		// Captures anything between Begin{0} (Group1) and End{0} (Group3) in Group2
		private static readonly string FindBeginEndRegex = @"(\/\/\s*Begin{0})([\s\S]*)(\/\/\s*End{0})";
		// Captures the name of "const unsigned char" (or "const BYTE" in the case of FXC) g_ array. Puts the result in Group4
		private static readonly string ByteArrayNameFromGeneratedHeaderFileRegex = @"const\s*((unsigned\s*char)|(BYTE))\s*g_(.*)\[\]";

		// Processes all headers from a shader file and compiles every permutation
		public static void Compile(ShaderFile inShaderFile)
		{
			Console.WriteLine("Compiling shaders for file \"{0}\"", inShaderFile.FullPath);

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

			foreach (HeaderInfo header in inShaderFile.Headers)
				shaderCompiler.Compile(header, inShaderFile);
		}

		// Will replace anything in the input file bewtween Begin{inPattern} and End{inPattern} with inReplacement string
		static void ReplaceFileContent(string inFile, string inPattern, string inReplacement)
		{
			if (!File.Exists(inFile))
				throw new Exception("File (" + inFile + ") does not exist.");

			string file_content = File.ReadAllText(inFile);

			string regexPattern = string.Format(FindBeginEndRegex, inPattern);
			Regex regex = new Regex(regexPattern, RegexOptions.IgnoreCase | RegexOptions.Multiline);
			Match regMatch = regex.Match(file_content);
			if (!regMatch.Success)
				throw new Exception("BeginShaderByteCode or EndShaderByteCode were not found in file (" + inFile + ")");

			string new_file_content = regex.Replace(file_content, "$1" + Environment.NewLine + inReplacement + "$3");

			// Before overwritting the file, check if the content is actually different.
			// This will prevent recompiling if the content was not actually changed.
			if (file_content != new_file_content)
				File.WriteAllText(inFile, new_file_content);
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
					continue;

				string GeneratedFileContent = File.ReadAllText(fileStr);

				// Look for the byte array containing the shader code
				Match byteArrayNameMatch = Regex.Match(GeneratedFileContent, ByteArrayNameFromGeneratedHeaderFileRegex, RegexOptions.IgnoreCase);
				if (!byteArrayNameMatch.Success)
					throw new Exception("Given header file (" + fileStr + ") does not contain a Byte array. Something is seriously wrong.");

				string ByteArrayName = byteArrayNameMatch.Groups[4].ToString();

				shaderByteCodeBuilder.AppendLine("\tINIT_SHADER_BYTECODE(" + ByteArrayName + ");");
				includeBuilder.AppendLine("\t#include \"Shaders\\generated\\" + Path.GetFileName(fileStr) + "\"");
			}

			ReplaceFileContent(Config.ShaderHFile, "Include", includeBuilder.ToString());
			ReplaceFileContent(Config.ShaderHFile, "ShaderByteCode", shaderByteCodeBuilder.ToString());
		}

		public static void GenerateConstantBufferHFile(List<Struct> inStructs)
		{
			StringBuilder string_builder = new StringBuilder();

			// Go through all structs and only append constant buffers
			foreach (Struct sr in inStructs)
			{
				if (!sr.IsVertexShaderOutput && !sr.IsVertexLayout)
					string_builder.AppendLine(sr.GetConstantBufferString());
			}

			string constant_buffer_string = string_builder.ToString().TrimEnd() + Environment.NewLine;

			ReplaceFileContent(Config.ConstantBufferHFile, "ConstantBuffer", constant_buffer_string);
		}

		public static void GenerateVertexLayoutHFile(List<Struct> inStructs)
		{
			StringBuilder vertex_format_sb = new StringBuilder();
			StringBuilder vertex_layout_sb = new StringBuilder();

			// Go through all structs and only append vertex layouts
			foreach (Struct sr in inStructs)
			{
				if (sr.IsVertexLayout)
				{
					// Print them as constant buffers to generate the cpp struct for the format
					vertex_format_sb.AppendLine(sr.GetConstantBufferString());

					// Generate vertex layout
					vertex_layout_sb.AppendLine(sr.GetVertexLayoutString());
				}
			}

			string vertex_format_string = vertex_format_sb.ToString().TrimEnd() + Environment.NewLine;
			string vertex_layout_string = vertex_layout_sb.ToString().TrimEnd() + Environment.NewLine;

			ReplaceFileContent(Config.VertexLayoutHFile, "VertexFormat", vertex_format_string);
			ReplaceFileContent(Config.VertexLayoutHFile, "VertexInputLayout", vertex_layout_string);
		}
	}
}
