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

			if (args.Length != 1)
			{
				Console.WriteLine("Wrong number of arguments.");
				Console.WriteLine("Usage: <Path to shader folder>");
				return -1;
			}

			string pathToShaderFolder = args[0];

			if (!Directory.Exists(pathToShaderFolder))
			{
				Console.WriteLine("Given folder \"{0}\" doesn't exist", pathToShaderFolder);
				return -2;
			}

			Console.WriteLine("Searching folder \"{0}\"", pathToShaderFolder);

			try
			{
				ShaderFileGatherer.StartProcessing(pathToShaderFolder);

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
