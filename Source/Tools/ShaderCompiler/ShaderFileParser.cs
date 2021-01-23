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

			// Find if it has any duplicate structs that aren't vertex shader outputs
			var duplicates = Structs.Where(s => !s.IsVertexShaderOutput).GroupBy(s => s.Name).Where(g => g.Count() > 1).Select(g => g.Key).ToList();

			if (duplicates.Count > 0)
			{
				Console.WriteLine("ERROR: Duplicate Structs: " + String.Join(", ", duplicates));
				throw new Exception("Multiple structs have the same Name. Probably shouldn't allow it.");
			}

			//DebugPrint();
		}

		void DebugPrint()
		{
			foreach (Struct s in Structs)
				s.DebugPrint();
		}
	}
}