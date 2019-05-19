using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace ShaderCompiler
{
	class StructElement
	{
		public string Interpolation;
		public string Type;
		public string Name;
		public string Semantic;
		public string Array;

		private static readonly string VariableNameRegex = @"([a-z_][a-z0-9_]*)";
		private static readonly string InterpolationRegex = "(linear|centroid|nointerpolation|noperspective|sample)";
		private static readonly string TypeNameRegex =
			@"\s*" +										//	Match spaces, if any
			VariableNameRegex +								// GROUP 1: Full type
			@"\s+" +										//	There should be at least one space character between type and name
			VariableNameRegex +								// GROUP 2: Variable name
			@"\s*" +										//	There may or may not be any space before square brackets []
			@"(\[([0-9]+|" + VariableNameRegex + @")\])?"	// GROUP 3: [Array], Group 4: (Number + Text), Group 5 (Text only). Only use GROUP 4
		;
		private static readonly string SemanticRegex = ".*:[ \t]*([a-z_][a-z0-9_]*);?";

		public StructElement(string shaderCode)
		{
			// Find interpolator, if any
			var InterpolReg = new Regex(InterpolationRegex, RegexOptions.None);
			Interpolation = InterpolReg.Match(shaderCode).ToString();
			if (Interpolation.Length > 0)
			{
				shaderCode = shaderCode.Replace(Interpolation, "");
			}
			else
			{
				Interpolation = "";
			}

			// Find Type, Name, Array (if any)
			var TypeReg = new Regex(TypeNameRegex, RegexOptions.IgnoreCase);
			Match TypeMatch = TypeReg.Match(shaderCode);
			if (TypeMatch.Success)
			{
				Type = TypeMatch.Groups[1].ToString();
				Name = TypeMatch.Groups[2].ToString();
				Array = TypeMatch.Groups[4].ToString();

				shaderCode = shaderCode.Replace(Type, "");
			}
			else
			{
				throw new Exception("Missing or incorrect type argument in structure.");
			}

			// Find Semantic, if any
			var SemanticReg = new Regex(SemanticRegex, RegexOptions.IgnoreCase);
			Match SemanticMatch = SemanticReg.Match(shaderCode);
			if (SemanticMatch.Success)
			{
				// Get the result from Group1 directly
				Semantic = SemanticMatch.Groups[1].ToString();
			}
			else
			{
				Semantic = "";
			}
		}
	}

	class Struct
	{
		public string Name;
		public List<StructElement> Elements;

		public Struct(string name)
		{
			Name = name;
			Elements = new List<StructElement>();
		}

		public void AddStructElement(string shaderCode)
		{
			if (shaderCode.Length > 0)
			{
				Elements.Add(new StructElement(shaderCode));
			}
		}

		public override bool Equals(object obj)
		{
			if (obj is Struct s)
			{
				return Name.Equals(s.Name);
			}

			return false;
		}

		public override int GetHashCode()
		{
			return base.GetHashCode();
		}

		public void DebugPrint()
		{
			Console.Write(PrintAsConstantBuffer());
			Console.WriteLine(PrintAsVertexLayout());
		}

		public string PrintAsConstantBuffer()
		{
			string ret = "struct " + Name + "\n{\n";

			foreach (StructElement se in Elements)
			{
				string line = "\t" + se.Type + " " + se.Name + (se.Array != "" ? "[" + se.Array + "]" : "") + ";";
				// On tab is 4 spaces/characters. One line should be 32 char max
				int numTabs = (32 - line.Length) / 4;
				string tabs = new string('\t', numTabs);

				string comment  =@"\\ " + (se.Semantic != "" ? "(:" + se.Semantic + ")" : "") +" TODO: Change this " + se.Type + " to the equivalent CPP type.\n";

				ret += line + tabs + comment;
			}

			ret += "};\n";
			return ret;
		}

		public string PrintAsVertexLayout()
		{
			// Prints something like this:
			/*
			D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};
			*/

			string ret = "D3D12_INPUT_ELEMENT_DESC g_" + Name + "[] =\n{\n";

			foreach (StructElement se in Elements)
			{
				// TODO: Lots of TODOs in here gee.
				string  SemanticName         = se.Semantic;
				uint    SemanticIndex        = 0;                                                // TODO: Change this depending on number of float4
				string  Format               = "DXGI_FORMAT_R32G32B32A32_FLOAT";                 // TODO: Change format depending on num of components and packing
				uint    InputSlot            = 0;                                                // TODO: Change depending on the input slot (in case of de interleaved buffers)
				string  AlignedByteOffset    = "D3D12_APPEND_ALIGNED_ELEMENT";                   // TODO: Handle packing (if any).
				string  InputSlotClass       = "D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA";     // TODO: Handle per instance data?
				uint    InstanceDataStepRate = 0;            // Needs to be 0 with Per_Vertex data. TODO: Same as above

				string line = string.Format("\"{0}\", {1}, {2}, {3}, {4}, {5}, {6}",
											SemanticName,
											SemanticIndex,
											Format,
											InputSlot,
											AlignedByteOffset,
											InputSlotClass, InstanceDataStepRate);

				ret += "\t{ " + line + " },\n";
			}

			ret += "};\n";
			return ret;
		}

		public static List<Struct> GetAllStructsFromShaderFile(ShaderFile shaderFile)
		{
			var identifierReg = new Regex(@"^\s*([a-z_][a-z0-9_]*)\s*", RegexOptions.IgnoreCase);
			var structReg = new Regex("{.+?}", RegexOptions.IgnoreCase | RegexOptions.Singleline);

			List<Struct> structs = new List<Struct>();

			// For each structure in the code
			foreach (string str in shaderFile.Content.Split(new[] { "struct" }, StringSplitOptions.RemoveEmptyEntries))
			{
				// Need to remove those annoying \r\n
				string text = Regex.Replace(str, "(\r\n|\n\r|\n|\r)", "\n");

				// Get its name, should be specified right after the "struct Keyword"
				var nameMatch = identifierReg.Match(text);
				string name = nameMatch.Success ? nameMatch.Groups[1].ToString() : "GeneratedStructName";

				Struct s = new Struct(name);

				// Get everything between brackets {}
				text = structReg.Match(text).ToString();

				// Remove brackets, newlines, tabs
				text = new Regex(@"\t|\n|{|}", RegexOptions.IgnoreCase).Replace(text, " ");

				// For each element separated by a semi-colon ;
				foreach (string e in text.Split(new[] { ";" }, StringSplitOptions.RemoveEmptyEntries))
				{
					s.AddStructElement(e.Trim());
				}

				structs.Add(s);
			}

			return structs;
		}
	}
}