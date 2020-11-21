using System;
using System.Collections.Generic;
using System.Linq;

namespace ShaderCompiler
{
	class ShaderFileParser
	{
		protected List<ShaderFile>	ShaderFiles;
		public List<Struct>			Structs;

		public ShaderFileParser(List<ShaderFile> shaderFiles)
		{
			ShaderFiles = shaderFiles;
			Structs = new List<Struct>();
		}

		public void ProcessSingleFile(ShaderFile inShaderFile)
		{
			// Get all structs
			Structs.AddRange(Struct.GetAllStructsFromShaderFile(inShaderFile));

			// Compile
			if (inShaderFile.ShouldCompile)
				ShaderCompiler.Compile(inShaderFile);
		}

		public void ProcessFiles()
		{
			foreach (ShaderFile shaderFile in ShaderFiles)
				ProcessSingleFile(shaderFile);

			// Find if it has any duplicates
			bool duplicates = Structs.GroupBy(n => n).Any(c => c.Count() > 1);

			if (duplicates)
				throw new Exception("Multiple structs have the same Name. Probably shouldn't allow it.");

			//DebugPrint();
		}

		void DebugPrint()
		{
			foreach (Struct s in Structs)
				s.DebugPrint();
		}
	}
}