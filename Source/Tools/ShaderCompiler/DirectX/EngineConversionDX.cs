using System.Diagnostics;

namespace ShaderCompiler
{
	class EngineConversionDX
	{
		public static string StructElementToEngineType(StructElement inStructElement)
		{
			int num_col = inStructElement.NumCol;
			int num_rows = inStructElement.NumRows;
			string scalar_type = inStructElement.BaseTypeName;
			string variable_name = inStructElement.VariableName;
			string array_string = inStructElement.ArrayString;

			Debug.Assert(num_col > 0 && num_col <= 4);
			Debug.Assert(num_rows >= 0 && num_rows <= 4);

			string engine_type;

			// Base scalar type
			switch (scalar_type)
			{
			case "int":
				engine_type = "int32";
				break;
			case "uint":
			case "dword":
				engine_type = "uint32";
				break;
			case "half":
				engine_type = "int16";
				break;
			default:
				engine_type = scalar_type;
				break;
			}

			// Vector types like float2, int3
			if (num_col > 1 && num_rows == 1)
			{
				switch (scalar_type)
				{
				case "float":
					engine_type = "Vector" + num_col + "f";
					break;
				default:
					// Default is just an array
					array_string += "[" + num_col + "]";
					break;
				}
			}
			// Matrix types like matrix, float4x4
			else if (num_col > 1 && num_rows > 1)
			{
				switch (scalar_type)
				{
				default:
					// Default is just an array
					array_string += "[" + num_col + "][" + num_rows + "]";
					break;
				}
			}

			return engine_type + " " + variable_name + array_string + ";";
		}
	}
}