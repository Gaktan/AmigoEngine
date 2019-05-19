using System;
using System.Collections.Generic;
using System.Linq;

namespace ShaderCompiler
{
	class ShaderFileParser
	{
		protected List<ShaderFile>	ShaderFiles;
		protected List<Struct>      Structs;


		public ShaderFileParser(List<ShaderFile> shaderFiles)
		{
			ShaderFiles = shaderFiles;
			Structs = new List<Struct>();
		}

		public void Process(ShaderFile shaderFile)
		{
			// Get all structs
			Structs.AddRange(Struct.GetAllStructsFromShaderFile(shaderFile));
		}

		public void ProcessAllFiles()
		{
			foreach (ShaderFile shaderFile in ShaderFiles)
			{
				Process(shaderFile);
			}

			// Find if it has any duplicates
			bool duplicates = Structs.GroupBy(n => n).Any(c => c.Count() > 1);

			if (duplicates)
			{
				throw new Exception("Multiple structs have the same Name. Probably shouldn't allow it.");
			}

			DebugPrint();
		}

		void DebugPrint()
		{
			foreach(Struct s in Structs)
			{
				Console.WriteLine("\n");
				s.DebugPrint();
			}
		}
	}
}