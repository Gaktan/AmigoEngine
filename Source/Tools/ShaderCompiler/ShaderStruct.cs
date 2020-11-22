using System;
using System.Collections.Generic;
using System.Diagnostics;
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
			@"((\[.*\])*)?"									// Group 3: All arrays (ArrayString), GROUP 4: A single array. Only use GROUP 3
		;
		private static readonly string SemanticRegex = @".*:\s*([a-z_][a-z0-9_]*);?";

		// Decompose TypeName into Group 1: (BaseTypeName), Group 2: (NumCol), Group 4: (NumRows)
		// e.g. "float3x2" -> (float)(3)(2); "float" -> (float)()(); "float4" -> (float)(4)()
		private static readonly string TypeNameDecompose = "(" + string.Join("|", AllScalarTypes) + @")([1-4]|)(x([1-4])|)$";

		public StructElement(string shaderCode)
		{
			int current_pos = 0;
			// Find interpolator, if any
			Regex interpolation_reg = new Regex(InterpolationRegex, RegexOptions.None);
			Match interpolation_match = interpolation_reg.Match(shaderCode, current_pos);
			if (interpolation_match.Success)
			{
				Interpolation = interpolation_match.ToString();
				current_pos += interpolation_match.Length;
			}

			// Find Type, Name, ArrayCount (if any)
			Regex type_reg = new Regex(TypeNameRegex, RegexOptions.IgnoreCase);
			Match type_match = type_reg.Match(shaderCode, current_pos);
			if (type_match.Success)
			{
				BaseTypeName	= type_match.Groups[1].ToString();
				VariableName	= type_match.Groups[2].ToString();
				ArrayString		= type_match.Groups[3].ToString();

				current_pos += type_match.Length;
			}
			else
			{
				throw new Exception("Missing or incorrect type argument in structure.");
			}

			// Find NumCol and NumRows (if any)
			Match decompose_match = Regex.Match(BaseTypeName, TypeNameDecompose, RegexOptions.None);
			if (decompose_match.Success)
			{
				// Decompose TypeName into Group 1: (BaseTypeName), Group 2: (NumCol), Group 4: (NumRows)
				BaseTypeName = decompose_match.Groups[1].ToString();
				if (!int.TryParse(decompose_match.Groups[2].ToString(), out NumCol))
					NumCol = 1;

				if (!int.TryParse(decompose_match.Groups[4].ToString(), out NumRows))
					NumRows = 1;
			}

			// Find Semantic, if any
			Regex semantic_reg = new Regex(SemanticRegex, RegexOptions.IgnoreCase);
			Match semantic_match = semantic_reg.Match(shaderCode, current_pos);
			if (semantic_match.Success)
			{
				// Get the result from Group1 directly
				Semantic = semantic_match.Groups[1].ToString();
			}
		}

		public string GetDXGIFormat()
		{
			// Matrices not supported
			Debug.Assert(NumRows == 1);

			string dxgi_format = "DXGI_FORMAT_UNKNOWN";

			string dxgi_type;

			switch (BaseTypeName)
			{
			case "float":
				dxgi_type = "FLOAT";
				break;
			case "int":
				dxgi_type = "SINT";
				break;
			case "uint":
			case "dword":
				dxgi_type = "UINT";
				break;
			default:
				dxgi_type = "TYPELESS";
				break;
			}

			string dxgi_channels = "R{0}";

			if (NumCol > 1)
				dxgi_channels += "G{0}";
			if (NumCol > 2)
				dxgi_channels += "B{0}";
			if (NumCol > 3)
				dxgi_channels += "A{0}";

			// Stick with 32bits types only for now
			int type_size = 32;
			dxgi_channels = String.Format(dxgi_channels, type_size);

			dxgi_format = String.Format(@"DXGI_FORMAT_{0}_{1}", dxgi_channels, dxgi_type);

			return dxgi_format;
		}
	}

	class Struct
	{
		public string Name;
		public List<StructElement> Elements;
		public bool IsConstantBuffer	= false;
		public bool IsVertexLayout		= false;

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
				Elements.Add(new StructElement(shaderCode));
		}

		public void DetectStructType()
		{
			foreach (StructElement se in Elements)
			{
				// If we use SV_Position, it means this is a vertex shader output, not an input. So we don't consider it as a vertex layout or constant buffer
				// TODO: Regex test maybe?
				if (se.Semantic != null && se.Semantic == "SV_Position")
				{
					IsConstantBuffer	= false;
					IsVertexLayout		= false;
					return;
				}
			}

			foreach (StructElement se in Elements)
			{
				// At least one semantic or interpolation means we are definitely sure this is a vertex layout
				if (se.Semantic != null || se.Interpolation != null)
				{
					IsConstantBuffer	= false;
					IsVertexLayout		= true;
					return;
				}
			}

			// Else, this is probably a constant buffer
			IsConstantBuffer	= true;
			IsVertexLayout		= false;
		}

		public override bool Equals(object obj)
		{
			if (obj is Struct s)
				return Name.Equals(s.Name);

			return false;
		}

		public override int GetHashCode()
		{
			return base.GetHashCode();
		}

		public void DebugPrint()
		{
			if (IsConstantBuffer)
				Console.Write(GetConstantBufferString());
			else
				Console.Write(GetVertexLayoutString());
		}

		public string GetConstantBufferString()
		{
			StringBuilder ret = new StringBuilder("struct " + Name + Environment.NewLine + "{");

			foreach (StructElement se in Elements)
			{
				string engine_type = EngineConversionDX.StructElementToEngineType(se);

				ret.AppendLine();
				ret.Append("\t" + engine_type);
			}

			ret.AppendLine(Environment.NewLine + "};");
			return ret.ToString();
		}

		public string GetVertexLayoutString()
		{
			// Prints something like this:
			/*
			static D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};
			*/

			string ret = "static D3D12_INPUT_ELEMENT_DESC " + Name + "[] =" + Environment.NewLine + "{" + Environment.NewLine;

			foreach (StructElement se in Elements)
			{
				// TODO: Lots of TODOs in here gee.
				string  semantic_name			= se.Semantic;
				uint    semantic_index			= 0;												// TODO: Change this depending on number of float4
				string  format					= se.GetDXGIFormat();
				uint    input_slot				= 0;												// TODO: Change depending on the input slot (in case of de interleaved buffers)
				string  aligned_byte_offset		= "D3D12_APPEND_ALIGNED_ELEMENT";					// TODO: Handle packing (if any).
				string  input_slot_class		= "D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA";		// TODO: Handle per instance data?
				uint    instance_data_step_rate	= 0;			// Needs to be 0 with Per_Vertex data. TODO: Same as above

				string line = string.Format("\"{0}\", {1}, {2}, {3}, {4}, {5}, {6}",
											semantic_name,
											semantic_index,
											format,
											input_slot,
											aligned_byte_offset,
											input_slot_class, instance_data_step_rate);

				ret += "\t{ " + line + " }," + Environment.NewLine;
			}

			ret += "};" + Environment.NewLine;
			return ret;
		}

		public static List<Struct> GetAllStructsFromShaderFile(ShaderFile shaderFile)
		{
			MatchCollection matches = Regex.Matches(shaderFile.Content, StructRegex, RegexOptions.Multiline);

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
					s.AddStructElement(e.Trim());

				// Flag if struct is a constant buffer or not
				s.DetectStructType();

				structs.Add(s);
			}

			return structs;
		}
	}
}