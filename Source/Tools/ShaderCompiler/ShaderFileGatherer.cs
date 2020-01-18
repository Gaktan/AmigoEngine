using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;

namespace ShaderCompiler
{
	[Serializable]
	class ShaderFile
	{
		public string FullPath;
		[NonSerialized]
		public string Content;
		// TODO: Deal with dependencies (includes). Will require a deep read of the file.
		//public List<ShaderFile> Dependencies = null;
		public UInt32 Hash;
		public bool DidCompile;

		public string GetFileName()
		{
			return Path.GetFileNameWithoutExtension(FullPath);
		}
	}

	class ShaderFileGatherer
	{
		private static Dictionary<UInt32, ShaderFile> PreviousResult = null;
		private static Dictionary<UInt32, ShaderFile> CurrentResult = null;

		static void ProcessFile(string fullPathToFile)
		{
			if (Config.ShaderExtensions.Any(x => fullPathToFile.EndsWith(x)))
			{
				//Console.WriteLine("Processing file {0}", file);

				string allText = File.ReadAllText(fullPathToFile);
				//Console.WriteLine(allText);

				Crc32 crc32 = new Crc32();
				UInt32 hash = crc32.Get(allText);

				ShaderFile shaderFile = new ShaderFile
				{
					DidCompile = false,
					Hash = hash,
					FullPath = fullPathToFile,
					Content = allText
				};

				if (CurrentResult.ContainsKey(hash))
				{
					throw new Exception("Same file shouldn't be processed twice");
				}

				CurrentResult[hash] = shaderFile;
			}
		}

		static void ProcessFolder(string folder)
		{
			List<string> dirs = new List<string>(Directory.EnumerateDirectories(folder));
			foreach (string dir in dirs)
			{
				ProcessFolder(dir);
			}

			List<string> files = new List<string>(Directory.EnumerateFiles(folder));
			foreach (string file in files)
			{
				ProcessFile(file);
			}
		}

		static void ReadDataBase()
		{
			if (File.Exists(Config.DatabasePath))
			{
				IFormatter formatter = new BinaryFormatter();
				Stream stream = new FileStream(Config.DatabasePath, FileMode.Open, FileAccess.Read);
				try
				{
					PreviousResult = (Dictionary<UInt32, ShaderFile>) formatter.Deserialize(stream);
				}
				catch (SerializationException)
				{
					// Deserialization didnd't work. Recompile everything
					PreviousResult = new Dictionary<uint, ShaderFile>();
				}

				stream.Close();
			}
		}

		static void WriteDataBase()
		{
			IFormatter formatter = new BinaryFormatter();
			Stream stream = new FileStream(Config.DatabasePath, FileMode.OpenOrCreate, FileAccess.Write);
			formatter.Serialize(stream, CurrentResult);
			stream.Close();
		}

		// Can't think of a better name, sorry
		static void Add(ShaderFile shaderFile, List<ShaderFile> list)
		{
			//foreach (ShaderFile f in shaderFile.Dependencies)
			//{
			//	Add(f, list);
			//}

			list.Add(shaderFile);
		}

		static List<ShaderFile> GetShaderFilesThatNeedToCompile()
		{
			List<ShaderFile> ret = new List<ShaderFile>();

			foreach (KeyValuePair<UInt32, ShaderFile> pair in PreviousResult)
			{
				ShaderFile f = pair.Value;

				// Current has this key. Which means the file was not deleted
				if (CurrentResult.ContainsKey(pair.Key))
				{
					// Add Shaders that didn't compile last time
					if (!f.DidCompile)
					{
						Add(CurrentResult[pair.Key], ret);
					}
				}
			}

			foreach (KeyValuePair<UInt32, ShaderFile> pair in CurrentResult)
			{
				ShaderFile f = pair.Value;

				// Previous didn't have this key. Which means it's either a new file or edited file
				if (!PreviousResult.ContainsKey(pair.Key))
				{
					Add(f, ret);
				}
			}

			// Remove duplicates before returning
			return ret.Distinct().ToList();
		}

		private static void CreateGeneratedFolder()
		{
			// DXC doesn't create directories automatically, so we need to do it manually.
			if (!Directory.Exists(Config.GeneratedFolderPath))
			{
				Directory.CreateDirectory(Config.GeneratedFolderPath);
			}
		}

		public static void Build()
		{
			CreateGeneratedFolder();

			// Read back from DB
			ReadDataBase();

			// Go through all the files and create Shaderfiles
			ProcessFolder(Config.ShaderSourcePath);

			ShaderFileParser fileParser = new ShaderFileParser(CurrentResult.Values.ToList());
			fileParser.ProcessAllFiles();

			// Generate \Shaders\include\Shaders.h
			ShaderCompiler.GenerateShaderHFile();

			// TODO: Compile only the files that need recompiling
			//List<ShaderFile> shaderFiles = GetShaderFilesThatNeedToCompile();
			//foreach(ShaderFile f in shaderFiles)
			//{
			//	Console.WriteLine("Press " + f.Name + " to pay Respect\n\n");
			//	new ShaderFileParser(f).Process();
			//	Console.WriteLine("\n\n");
			//}

			// TODO: Delete generated files from shader files that were deleted

			// Update DB by overwritting it
			WriteDataBase();
		}

		public static void BuildForTest()
		{
			// Go through all the files and create Shaderfiles
			ProcessFolder(Config.ShaderSourcePath);

			ShaderFileParser fileParser = new ShaderFileParser(CurrentResult.Values.ToList());

			// Iterate all Compiler/ShaderModel combinations
			EnumUtils.ForEach<Compiler>((compiler) =>
			{
				EnumUtils.ForEach<ShaderModel>((shaderModel) =>
				{
					Config.Compiler		= compiler;
					Config.ShaderModel	= shaderModel;

					if (Config.Verify(throws : false))
					{
						fileParser.ProcessAllFiles();
					}
				});
			});
		}

		public static void Clean()
		{
			// The DB is in the generated folder but delete it just in case
			if (File.Exists(Config.DatabasePath))
			{
				File.Delete(Config.DatabasePath);
			}

			if (Directory.Exists(Config.GeneratedFolderPath))
			{
				Directory.Delete(Config.GeneratedFolderPath, true);
			}
		}

		public static void StartProcessing(string rootFolder)
		{
			CurrentResult = new Dictionary<UInt32, ShaderFile>();

			if (Arguments.IsClean())
			{
				Clean();
			}

			if (Arguments.IsBuild())
			{
				Build();
			}
			else if (Arguments.Operation == Arguments.OperationType.Test)
			{
				BuildForTest();
			}
		}
	}
}