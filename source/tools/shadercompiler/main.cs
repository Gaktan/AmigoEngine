using System;
using System.IO;
using System.Collections.Generic;

namespace ShaderCompiler
{
	class MainProgram
	{
		private static int Main(string[] args)
		{
			Console.WriteLine("\n\n\n\t\t=== Starting compiling Shaders ===");

			bool success = Arguments.ParseArguments(args);
			if (!success)
			{
				return -1;
			}

			Console.WriteLine("Searching folder \"{0}\"", Arguments.SourceFolder);

			try
			{
				ShaderFileGatherer.StartProcessing(Arguments.SourceFolder);

				Console.WriteLine("\t\t=== Finished compiling Shaders with SUCCESS ===\n\n\n");
			}
			catch (Exception e)
			{
				Console.WriteLine("\n\n\n" + e.ToString() + "\n\n\n");
				Console.WriteLine("\t\t=== Finished compiling Shaders with ERRORS ===\n\n\n");

				return -3;
			}

			// Always return -1 when debugging to force recompiling
			//return -1;
			return 0;
		}
	}
}
