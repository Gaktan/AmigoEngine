using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text.RegularExpressions;

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

		// List all generated files so we can check if we should recompile them if they are missing. Or delete them if the shader file was deleted.
		public List<string>		GeneratedFiles;

		// All headers defined in the Shader
		[NonSerialized]
		public List<HeaderInfo> Headers;

		public UInt32		FilenameHash;
		public DateTime		ModifiedTime;
		public bool			DidCompile;

		public string GetFileName()
		{
			return Path.GetFileNameWithoutExtension(FullPath);
		}

		// Parses the whole file and finds "// ShaderCompiler." headers
		public void CreateHeaders()
		{
			// Example of a shader header:
			// "// ShaderCompiler. Name: Test_01, EntryPoint: main, Type: PS, Defines: DEFINE1; DEFINE2 = 0"

			// Captures any line that starts with "// ShaderCompiler." and puts the content of the header in Group1
			string HeaderRegex = @"//\s*ShaderCompiler\.(.*)";

			Headers = new List<HeaderInfo>();

			// For each line
			using (StringReader reader = new StringReader(Content))
			{
				string line;
				int lineNum = 1;
				while ((line = reader.ReadLine()) != null)
				{
					Match headerMatch = Regex.Match(line, HeaderRegex, RegexOptions.IgnoreCase);
					if (headerMatch.Success)
					{
						try
						{
							string str = headerMatch.Groups[1].ToString();
							Headers.Add(HeaderInfo.FromString(str));
						}
						catch (MissingHeaderTagException e)
						{
							throw new Exception("Missing tag \"" + e.TagMissing + "\" in header. " + FullPath + "(" + lineNum + ")");
						}
					}

					lineNum++;
				}
			}
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
					Content			= file_content,
					ShouldCompile	= true,
					FullPath		= inFullPathToFile,
					FilenameHash	= hash,
					ModifiedTime	= modified_time,
					DidCompile		= true
				};

				// Find all headers in files
				shaderFile.CreateHeaders();

				// Find generated file names in advance
				shaderFile.GeneratedFiles = new List<string>();
				foreach (HeaderInfo header in shaderFile.Headers)
				{
					shaderFile.GeneratedFiles.Add(header.GetGeneratedFileName(shaderFile));
				}

				if (CurrentResult.ContainsKey(hash))
					throw new Exception("Same file shouldn't be processed twice");

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

		static void ProcessDeletedShaderFile(ShaderFile inShaderFile)
		{
			string full_path = Config.GeneratedFolderPath;
			// Delete obsolete generated H files
			foreach (string filename in inShaderFile.GeneratedFiles)
			{
				// Don't process non shader compiled files
				if (!filename.EndsWith(Config.GeneratedHeaderExtension))
					throw new Exception("New generated files? Make sure they are handled properly");

				File.Delete(Path.Combine(full_path, filename));
			}
		}

		// Go through all previous elements in the DB to detect ShaderFiles that were deleted, added, or modified
		// Returns true if the previous DB doesn't match the current DB
		static bool UpdateDadaBase()
		{
			bool db_changed = false;

			// Process previous files in DB. Could still exist, or be deleted
			foreach (KeyValuePair<UInt32, ShaderFile> pair in PreviousResult)
			{
				ShaderFile previous_file	= pair.Value;
				ShaderFile current_file		= CurrentResult.GetValue(pair.Key);

				// File was not deleted
				if (current_file != null)
				{
					// Retry shaders that didn't compile last time
					bool file_changed = previous_file.DidCompile == false;
					// Retry Shaders that were modified
					file_changed |= (current_file.ModifiedTime > previous_file.ModifiedTime);

					// Check if generated files are still here. If not, we need to recompile this shader
					foreach (string filename in previous_file.GeneratedFiles)
					{
						if (!File.Exists(filename))
						{
							file_changed = true;
							break;
						}
					}

					current_file.ShouldCompile = file_changed;
					db_changed |= file_changed;
				}
				// File was deleted
				else
				{
					ProcessDeletedShaderFile(previous_file);
					db_changed = true;
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
					db_changed = true;
				}
			}

			return db_changed;
		}

		private static void CreateGeneratedFolder()
		{
			// DXC doesn't create directories automatically, so we need to do it manually.
			if (!Directory.Exists(Config.GeneratedFolderPath))
				Directory.CreateDirectory(Config.GeneratedFolderPath);
		}

		public static void Build()
		{
			CreateGeneratedFolder();

			// Read back from DB
			ReadDataBase();

			// Go through all the files and create Shaderfiles
			ProcessFolder(Config.ShaderSourcePath);

			// Tag all shaders that need to recompile and check if the DB was changed compared to the previous build
			bool db_changed = UpdateDadaBase();
			if (db_changed)
			{
				ShaderFileParser fileParser = new ShaderFileParser(CurrentResult.Values.ToList());
				fileParser.ProcessFiles();

				// Generate \Shaders\include\Shaders.h
				ShaderCompiler.GenerateShaderHFile();

				// Generate \Shaders\include\ConstantBuffers.h
				ShaderCompiler.GenerateConstantBufferHFile(fileParser.Structs);

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
				File.Delete(Config.DatabasePath);

			
			if (Directory.Exists(Config.GeneratedFolderPath))
				Directory.Delete(Config.GeneratedFolderPath, recursive: true);
		}

		public static void StartProcessing(string rootFolder)
		{
			CurrentResult = new Dictionary<UInt32, ShaderFile>();

			if (Arguments.IsClean())
				Clean();

			if (Arguments.IsBuild())
				Build();
			else if (Arguments.Operation == Arguments.OperationType.Test)
				BuildForTest();
		}
	}
}