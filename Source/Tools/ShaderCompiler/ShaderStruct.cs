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
			Regex interpolReg = new Regex(InterpolationRegex, RegexOptions.None);
			Interpolation = interpolReg.Match(shaderCode).ToString();
			if (Interpolation.Length > 0)
			{
				shaderCode = shaderCode.Replace(Interpolation, "");
			}
			else
			{
				Interpolation = "";
			}

			// Find Type, Name, Array (if any)
			Regex typeReg = new Regex(TypeNameRegex, RegexOptions.IgnoreCase);
			Match typeMatch = typeReg.Match(shaderCode);
			if (typeMatch.Success)
			{
				Type = typeMatch.Groups[1].ToString();
				Name = typeMatch.Groups[2].ToString();
				Array = typeMatch.Groups[4].ToString();

				shaderCode = shaderCode.Replace(Type, "");
			}
			else
			{
				throw new Exception("Missing or incorrect type argument in structure.");
			}

			// Find Semantic, if any
			Regex semanticReg = new Regex(SemanticRegex, RegexOptions.IgnoreCase);
			Match semanticMatch = semanticReg.Match(shaderCode);
			if (semanticMatch.Success)
			{
				// Get the result from Group1 directly
				Semantic = semanticMatch.Groups[1].ToString();
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

		// Captures a single struct and its content between {}. Group1: Name, Group2: Content
		private static readonly string StructRegex = @"struct\s+(.+)\s*\{([^}]*)\}";

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
				// One tab is 4 spaces/characters. One line should be 32 char max
				int numTabs = (32 - line.Length) / 4;
				numTabs = Math.Max(numTabs, 1);
				string tabs = new string('\t', numTabs);

				string comment = @"\\ " + (se.Semantic != "" ? "(:" + se.Semantic + ")" : "") + " TODO: Change this " + se.Type + " to the equivalent CPP type.\n";

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
				string  semanticName         = se.Semantic;
				uint    semanticIndex        = 0;                                                // TODO: Change this depending on number of float4
				string  format               = "DXGI_FORMAT_R32G32B32A32_FLOAT";                 // TODO: Change format depending on num of components and packing
				uint    inputSlot            = 0;                                                // TODO: Change depending on the input slot (in case of de interleaved buffers)
				string  alignedByteOffset    = "D3D12_APPEND_ALIGNED_ELEMENT";                   // TODO: Handle packing (if any).
				string  inputSlotClass       = "D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA";     // TODO: Handle per instance data?
				uint    instanceDataStepRate = 0;            // Needs to be 0 with Per_Vertex data. TODO: Same as above

				string line = string.Format("\"{0}\", {1}, {2}, {3}, {4}, {5}, {6}",
											semanticName,
											semanticIndex,
											format,
											inputSlot,
											alignedByteOffset,
											inputSlotClass, instanceDataStepRate);

				ret += "\t{ " + line + " },\n";
			}

			ret += "};\n";
			return ret;
		}

		public static List<Struct> GetAllStructsFromShaderFile(ShaderFile shaderFile)
		{
			Regex structReg = new Regex(StructRegex, RegexOptions.Multiline);
			MatchCollection matches = structReg.Matches(shaderFile.Content);

			List<Struct> structs = new List<Struct>();

			// For each structure in the code
			foreach (Match match in matches)
			{
				string name		= match.Groups[1].ToString().Trim();
				string content	= match.Groups[2].ToString();

				if (name.Length == 0)
				{
					// Structs should have a name. If they don't, it's probably a mistake. Make sure we catch this.
					throw new Exception("Nameless structs not allowed. Culprit:\n" + content);
				}

				Struct s = new Struct(name);

				// For each element separated by a semi-colon ;
				foreach (string e in content.Split(new[] { ";" }, StringSplitOptions.RemoveEmptyEntries))
				{
					s.AddStructElement(e.Trim());
				}

				structs.Add(s);
			}

			return structs;
		}
	}
}