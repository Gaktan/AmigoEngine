using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

namespace ShaderCompiler
{
	class StructElement
	{
		public string Interpolation;
		public string BaseTypeName;
		public string VariableName;
		public string Semantic;
		public int NumCol = 1;
		public int NumRows = 1;
		public string ArrayString;

		private static readonly string[] AllScalarTypes =
		{
			"bool",
			"int",
			"uint",
			"dword",
			"half",
			"float",
			"double",
		};

		private static readonly string[] AllInterpolations =
		{
			"linear",
			"centroid",
			"nointerpolation",
			"noperspective",
			"sample",
		};

		private static readonly string VariableNameRegex = @"([a-z_][a-z0-9_]*)";
		private static readonly string InterpolationRegex = "(" + string.Join("|", AllInterpolations) + ")";
		private static readonly string TypeNameRegex =
			@"\s*" +										//	Match spaces, if any
			VariableNameRegex +								// GROUP 1: Full type
			@"\s+" +										//	There should be at least one space character between type and name
			VariableNameRegex +								// GROUP 2: Variable name
			@"\s*" +										//	There may or may not be any space before square brackets []
			@"((\[[\d]*\])*)?"								// Group 3: All arrays (ArrayString), GROUP 4: A single array. Only use GROUP 3
		;
		private static readonly string SemanticRegex = ".*:[ \t]*([a-z_][a-z0-9_]*);?";

		// Decompose TypeName into Group 1: (BaseTypeName), Group 2: (NumCol), Group 4: (NumRows)
		private static readonly string TypeNameDecompose = "(" + string.Join("|", AllScalarTypes) + @")([1-4]|)(x([1-4])|)$";

		public StructElement(string shaderCode)
		{
			int current_pos = 0;
			// Find interpolator, if any
			Regex interpolReg = new Regex(InterpolationRegex, RegexOptions.None);
			Interpolation = interpolReg.Match(shaderCode, current_pos).ToString();
			if (Interpolation.Length > 0)
			{
				current_pos += Interpolation.Length;
			}

			// Find Type, Name, ArrayCount (if any)
			Regex typeReg = new Regex(TypeNameRegex, RegexOptions.IgnoreCase);
			Match typeMatch = typeReg.Match(shaderCode, current_pos);
			if (typeMatch.Success)
			{
				BaseTypeName = typeMatch.Groups[1].ToString();
				VariableName = typeMatch.Groups[2].ToString();
				ArrayString = typeMatch.Groups[3].ToString();

				current_pos += typeMatch.Length;
			}
			else
			{
				throw new Exception("Missing or incorrect type argument in structure.");
			}

			// Find NumCol and NumRows (if any)
			Regex decomposeReg = new Regex(TypeNameDecompose, RegexOptions.None);
			Match decomposeMatch = decomposeReg.Match(BaseTypeName);
			if (decomposeMatch.Success)
			{
				// Decompose TypeName into Group 1: (BaseTypeName), Group 2: (NumCol), Group 4: (NumRows)
				BaseTypeName = decomposeMatch.Groups[1].ToString();
				if(!int.TryParse(decomposeMatch.Groups[2].ToString(), out NumCol))
				{
					NumCol = 1;
				}
				if (!int.TryParse(decomposeMatch.Groups[4].ToString(), out NumRows))
				{
					NumRows = 1;
				}
			}

			// Find Semantic, if any
			Regex semanticReg = new Regex(SemanticRegex, RegexOptions.IgnoreCase);
			Match semanticMatch = semanticReg.Match(shaderCode, current_pos);
			if (semanticMatch.Success)
			{
				// Get the result from Group1 directly
				Semantic = semanticMatch.Groups[1].ToString();
			}
		}
	}

	class Struct
	{
		public string Name;
		public List<StructElement> Elements;
		public bool IsConstantBuffer;

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

		public void DetectStructType()
		{
			foreach (StructElement se in Elements)
			{
				// At least one semantic or interpolation means we are definitely sure this is not a constant buffer
				if (se.Semantic != null || se.Interpolation != null)
				{
					IsConstantBuffer = false;
					return;
				}
			}

			IsConstantBuffer = true;
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
			if (IsConstantBuffer)
			{
				Console.Write(PrintAsConstantBuffer());
			}
			else
			{
				Console.Write(PrintAsVertexLayout());
			}
		}

		public string PrintAsConstantBuffer()
		{
			StringBuilder ret = new StringBuilder("struct " + Name + Environment.NewLine + "{");

			foreach (StructElement se in Elements)
			{
				// If we have a semantic then, this is not an actual Constant Buffer
				if (se.Semantic != null)
				{
					return null;
				}
				string engine_type = EngineConversionDX.StructElementToEngineType(se);

				ret.AppendLine();
				ret.Append("\t" + engine_type);
			}

			ret.AppendLine(Environment.NewLine + "};");
			return ret.ToString();
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

				// Flag if struct is a constant buffer or not
				s.DetectStructType();

				structs.Add(s);
			}

			return structs;
		}
	}
}