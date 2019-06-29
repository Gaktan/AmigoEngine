using System;
using System.IO;

namespace ShaderCompiler
{
	class Arguments
	{
		public enum OperationType
		{
			Build,
			Clean,
			Rebuild,
			Unknown
		};

		public static OperationType Operation;
		public static string SourceFolder;

		public static bool IsBuild()
		{
			return Operation == OperationType.Build || Operation == OperationType.Rebuild;
		}
		public static bool IsClean()
		{
			return Operation == OperationType.Clean || Operation == OperationType.Rebuild;
		}

		private static bool ParseOperation(string operationString)
		{
			string op = operationString.ToLower();

			bool success = false;
			OperationType operationType = OperationType.Unknown;

			switch(op)
			{
			case "-build":
				success = true;
				operationType = OperationType.Build;
				break;
			case "-clean":
				success = true;
				operationType = OperationType.Clean;
				break;
			case "-rebuild":
				success = true;
				operationType = OperationType.Rebuild;
				break;
			}

			Operation = operationType;
			return success;
		}

		public static bool ParseArguments(string[] args)
		{
			if (args.Length != 2)
			{
				Console.WriteLine("Wrong number of arguments. Excpeted 2, got {0}", args.Length);
				PrintUsage();
				return false;
			}

			string operationString = args[0];
			if (!ParseOperation(operationString))
			{
				Console.WriteLine(@"Unkown argument '{0}' in commandline '{1}'", operationString, string.Join(" ", args));
				PrintUsage();
				return false;
			}

			SourceFolder = args[1];

			if (!Directory.Exists(SourceFolder))
			{
				Console.WriteLine(@"Given folder '{0}' doesn't exist", SourceFolder);
				return false;
			}

			return true;
		}

		private static void PrintSingleUsage(string command, string usage)
		{
			// On tab is 4 spaces/characters. One command should be 16 char max
			int numTabs = (16 - command.Length) / 4;
			numTabs = Math.Max(numTabs, 1);
			string tabs = new string('\t', numTabs);

			Console.WriteLine("\t{0}{1}{2}", command, tabs, usage);
		}

		private static void PrintUsage()
		{
			Console.WriteLine("Usage:");
			Console.WriteLine("ShaderCompiler.exe <Operation> <Source>\n");
			PrintSingleUsage("Operation", "Operation to be performed by the ShaderCompiler. Can be \"-Build\", \"-Clean\" or \"-Rebuild\".");
			PrintSingleUsage("Source", "Source folder of the shaders to be compiled. All files inside this folder and its subfolders will be processed.");

			Console.WriteLine();
		}
	}
}