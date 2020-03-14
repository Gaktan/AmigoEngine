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
		[NonSerialized]
		public string		Content;
		[NonSerialized]
		public bool			ShouldCompile;

		public string		FullPath;
		// TODO: Deal with dependencies (includes). Will require a deep read of the file.
		//public List<ShaderFile> Dependencies = null;
		public UInt32		FilenameHash;
		public DateTime		ModifiedTime;
		public bool			DidCompile;

		public string GetFileName()
		{
			return Path.GetFileNameWithoutExtension(FullPath);
		}
	}

	static class ShaderFileGatherer
	{
		private static Dictionary<UInt32, ShaderFile> PreviousResult = null;
		private static Dictionary<UInt32, ShaderFile> CurrentResult = null;

		static void ProcessFile(string inFullPathToFile)
		{
			// Make sure file ends with one of the shader extension
			if (Config.ShaderExtensions.Any(x => inFullPathToFile.EndsWith(x)))
			{
				string file_content = File.ReadAllText(inFullPathToFile);
				DateTime modified_time = File.GetLastWriteTime(inFullPathToFile);

				// Create hash from file name, to check with previous version of the file
				UInt32 hash = new Crc32().Get(inFullPathToFile);

				ShaderFile shaderFile = new ShaderFile
				{
					Content         = file_content,
					ShouldCompile   = true,
					FullPath        = inFullPathToFile,
					FilenameHash    = hash,
					ModifiedTime    = modified_time,
					DidCompile      = true
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
					PreviousResult = new Dictionary<UInt32, ShaderFile>();
				}

				stream.Close();
			}
			else
			{
				PreviousResult = new Dictionary<UInt32, ShaderFile>();
			}
		}

		static void WriteDataBase()
		{
			IFormatter formatter = new BinaryFormatter();
			Stream stream = new FileStream(Config.DatabasePath, FileMode.OpenOrCreate, FileAccess.Write);
			formatter.Serialize(stream, CurrentResult);
			stream.Close();
		}

		// TODO: Should also add all the dependencies!
		static void AddShaderFileToList(ShaderFile shaderFile, List<ShaderFile> list)
		{
			//foreach (ShaderFile f in shaderFile.Dependencies)
			//{
			//	Add(f, list);
			//}

			list.Add(shaderFile);
		}

		// Create new function for Dictionary to return null if key was not found
		public static TValue GetValue<TKey, TValue>(this IDictionary<TKey, TValue> inDictionary, TKey inKey, TValue inDefaultValue = default(TValue))
		{
			return inDictionary.TryGetValue(inKey, out TValue value) ? value : inDefaultValue;
		}

		static bool MarkShaderFilesThatNeedToCompile()
		{
			bool any_file_need_compile = false;
			// Process existing files
			foreach (KeyValuePair<UInt32, ShaderFile> pair in PreviousResult)
			{
				ShaderFile previous_file	= pair.Value;
				ShaderFile current_file		= CurrentResult.GetValue(pair.Key);

				// File was not deleted
				if (current_file != null)
				{
					// Retry shaders that didn't compile last time
					bool should_compile = previous_file.DidCompile == false;
					// Retry Shaders that were modified
					should_compile |= (current_file.ModifiedTime > previous_file.ModifiedTime);

					current_file.ShouldCompile = should_compile;
					any_file_need_compile |= should_compile;
				}
			}

			// Process added files
			foreach (KeyValuePair<UInt32, ShaderFile> pair in CurrentResult)
			{
				ShaderFile current_file		= pair.Value;
				ShaderFile previous_file	= PreviousResult.GetValue(pair.Key);

				// Current file was added
				if (previous_file == null)
				{
					current_file.ShouldCompile	= true;
					any_file_need_compile = true;
				}
			}

			return any_file_need_compile;
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

			// Tag all shaders that need to recompile
			bool any_file_need_compile = MarkShaderFilesThatNeedToCompile();

			// Don't process anything if all files are up to date
			if (any_file_need_compile)
			{
				ShaderFileParser fileParser = new ShaderFileParser(CurrentResult.Values.ToList());
				fileParser.ProcessFiles();

				// Generate \Shaders\include\Shaders.h
				ShaderCompiler.GenerateShaderHFile();

				// Generate \Shaders\include\ConstantBuffers.h
				ShaderCompiler.GenerateConstantBufferHFile(fileParser.Structs);

				// TODO: Delete generated files from shader files that were deleted

				// Update DB by overwritting it
				WriteDataBase();
			}
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
						fileParser.ProcessFiles();
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