using System;
using System.Diagnostics;
using System.IO;
using System.Threading;

namespace ShaderCompiler
{
	class Arguments
	{
		public enum OperationType
		{
			Build,
			Clean,
			Rebuild,
			Test,
			Unknown
		};

		public static OperationType Operation;
		public static string ConfigFile;
		public static string RootFolder;

		public static bool IsBuild()
		{
			return Operation == OperationType.Build || Operation == OperationType.Rebuild;
		}
		public static bool IsClean()
		{
			return Operation == OperationType.Clean || Operation == OperationType.Rebuild || Operation == OperationType.Test;
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
			case "-test":
				success = true;
				operationType = OperationType.Test;
				break;
			}

			Operation = operationType;
			return success;
		}

		public static bool ParseArguments(string[] args)
		{
			if (args.Length == 0)
			{
				Console.WriteLine("Expected arguments.");
				PrintUsage();
				return false;
			}

			Console.WriteLine("Using arguments: {0}", string.Join(" ", args));

			for (int i = 0; i < args.Length; i++)
			{
				string command = args[i];
				switch (command.ToLower())
				{
				case "-c":
				{
					string pathToExe = System.Reflection.Assembly.GetEntryAssembly().Location;
					ConfigFile = Path.GetDirectoryName(pathToExe) + @"\" + args[++i];
					if (!File.Exists(ConfigFile))
					{
						Console.WriteLine(@"Given config file '{0}' doesn't exist", ConfigFile);
						return false;
					}
					break;
				}
				case "-r":
				{
					RootFolder = args[++i];
					if (!Directory.Exists(RootFolder))
					{
						Console.WriteLine(@"Given root folder '{0}' doesn't exist", RootFolder);
						return false;
					}
					break;
				}
				case "-wait_for_debugger":
				{
					Console.WriteLine("Waiting for debugger to attach");
					while (!Debugger.IsAttached)
					{
						Thread.Sleep(100);
					}
					Console.WriteLine("Debugger attached");
					Debugger.Break();
					break;
				}
				default:
				{
					if (!ParseOperation(command))
					{
						Console.WriteLine(@"Unkown argument '{0}' in commandline '{1}'", command, string.Join(" ", args));
						PrintUsage();
						return false;
					}
					break;
				}
				}
			}

			return true;
		}

		private static void PrintUsage(string command, string usage)
		{
			// One tab is 4 spaces/characters. One command should be 24 char max
			int numTabs = (24 - command.Length) / 4;
			numTabs = Math.Max(numTabs, 1);
			string tabs = new string('\t', numTabs);

			Console.WriteLine("\t{0}{1}{2}", command, tabs, usage);
		}

		private static void PrintUsage()
		{
			Console.WriteLine("Usage:");
			Console.WriteLine("ShaderCompiler.exe <Operation> -c <ConfigFile> -r <RootFolder>\n");
			PrintUsage("Operation", "Operation to be performed by the ShaderCompiler. Can be \"-Build\" (default), \"-Clean\", \"-Rebuild\", or \"-Test\".");
			PrintUsage("ConfigFile", "INI file used for configuration. Optional. Relative to the executable location.");
			PrintUsage("RootFolder", "ShaderCompiler will run from this folder. Default is workspace directory");

			Console.WriteLine();
		}
	}
}