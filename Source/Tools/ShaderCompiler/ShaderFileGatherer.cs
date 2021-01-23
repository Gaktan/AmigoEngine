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
		[NonSerialized]
		public string		FullPath;

		// List all direct and indirect dependencies (includes) FROM this shader
		[NonSerialized]
		public List<ShaderFile> Dependencies = null;

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
		public void ParseHeaders()
		{
			// Example of a shader header:
			// "// ShaderCompiler. Name: Test_01, EntryPoint: main, Type: PS, Defines: DEFINE1; DEFINE2 = 0"

			// Captures any line that starts with "// ShaderCompiler." and puts the content of the header in Group1
			const string header_regex = @"//\s*ShaderCompiler\.(.*)";

			Headers = new List<HeaderInfo>();

			MatchCollection matches = Regex.Matches(Content, header_regex, RegexOptions.IgnoreCase);
			foreach (Match match in matches)
			{
				try
				{
					string str = match.Groups[1].ToString();
					Headers.Add(HeaderInfo.FromString(str));
				}
				catch (MissingHeaderTagException e)
				{
					// Find line number where the problem occurs
					int line_number = 1;
					for (int i = 0; i < match.Index; i++)
					{
						if (Content[i] == '\n')
							line_number++;
					}
					throw new Exception("Missing tag \"" + e.TagMissing + "\" in header. " + FullPath + "(" + line_number + ")");
				}
			}
		}

		public List<string> ParseIncludes()
		{
			List<string> includes = new List<string>();
			// Captures any line that starts with "#include" and puts the include name without quotes in Group1
			const string include_regex = "#include\\s*\"(.*)\"";

			MatchCollection matches = Regex.Matches(Content, include_regex, RegexOptions.IgnoreCase);
			foreach (Match match in matches)
			{
				string str = match.Groups[1].ToString();
				includes.Add(str);
			}

			return includes;
		}
	}

	static class ShaderFileGatherer
	{
		private static Dictionary<UInt32, ShaderFile> PreviousDB = null;
		private static Dictionary<UInt32, ShaderFile> CurrentDB = null;

		private static UInt32 GetFilenameHash(string inFullPathToFile)
		{
			return new Crc32().Get(inFullPathToFile);
		}

		private static void ProcessFile(string inFullPathToFile)
		{
			// Make sure file ends with one of the shader extension
			if (Config.ShaderExtensions.Any(x => inFullPathToFile.EndsWith(x)))
			{
				string file_content = File.ReadAllText(inFullPathToFile);
				DateTime modified_time = File.GetLastWriteTime(inFullPathToFile);

				// Create hash from file name, to check with previous version of the file
				UInt32 hash = GetFilenameHash(inFullPathToFile);

				ShaderFile shaderFile = new ShaderFile
				{
					Content			= file_content,
					ShouldCompile	= true,
					FullPath		= inFullPathToFile,
					FilenameHash	= GetFilenameHash(inFullPathToFile),
					ModifiedTime	= modified_time,
					DidCompile		= true,
					Dependencies	= new List<ShaderFile>()
				};

				// Find all headers in files
				shaderFile.ParseHeaders();

				// Find generated file names in advance
				shaderFile.GeneratedFiles = new List<string>();
				foreach (HeaderInfo header in shaderFile.Headers)
				{
					shaderFile.GeneratedFiles.Add(header.GetGeneratedFileName(shaderFile));
				}

				if (CurrentDB.ContainsKey(hash))
					throw new Exception("Same file shouldn't be processed twice");

				CurrentDB[hash] = shaderFile;
			}
		}

		private static void ProcessFolder(string inFullPathToFolder)
		{
			// Check that this folder is not ignored
			if (Config.IgnoredFolders.Any(x => inFullPathToFolder.Equals(x)))
				return;

			// Look through all folders and subfloders for shader files
			if (Config.RecursiveSearch)
			{
				List<string> dirs = new List<string>(Directory.EnumerateDirectories(inFullPathToFolder));
				foreach (string dir in dirs)
					ProcessFolder(dir);
			}

			List<string> files = new List<string>(Directory.EnumerateFiles(inFullPathToFolder));
			foreach (string file in files)
				ProcessFile(file);
		}

		private static void ReadDataBase()
		{
			if (File.Exists(Config.DatabasePath))
			{
				IFormatter formatter = new BinaryFormatter();
				Stream stream = new FileStream(Config.DatabasePath, FileMode.Open, FileAccess.Read);
				try
				{
					PreviousDB = (Dictionary<UInt32, ShaderFile>) formatter.Deserialize(stream);
				}
				catch (SerializationException)
				{
					// Deserialization didnd't work. Recompile everything
					PreviousDB = new Dictionary<UInt32, ShaderFile>();
				}

				stream.Close();
			}
			else
			{
				PreviousDB = new Dictionary<UInt32, ShaderFile>();
			}
		}

		static void WriteDataBase()
		{
			IFormatter formatter = new BinaryFormatter();
			Stream stream = new FileStream(Config.DatabasePath, FileMode.OpenOrCreate, FileAccess.Write);
			formatter.Serialize(stream, CurrentDB);
			stream.Close();
		}

		// Create new function for Dictionary to return null if key was not found
		private static TValue GetValue<TKey, TValue>(this IDictionary<TKey, TValue> inDictionary, TKey inKey, TValue inDefaultValue = default(TValue))
		{
			return inDictionary.TryGetValue(inKey, out TValue value) ? value : inDefaultValue;
		}

		private static void ProcessDeletedShaderFile(ShaderFile inShaderFile)
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

		// Recursive function to find all dependencies FROM a given shader file
		private static void FindDependenciesInFile(ShaderFile inShaderFile, List<ShaderFile> outList)
		{
			string shader_directory = Path.GetDirectoryName(inShaderFile.FullPath);

			List<string> includes = inShaderFile.ParseIncludes();
			foreach (string include in includes)
			{
				string include_path = Path.GetFullPath(Path.Combine(shader_directory, include));

				// Check the file actually exists
				if (File.Exists(include_path))
				{
					// Get shader file from full path
					UInt32 file_hash = GetFilenameHash(include_path);
					ShaderFile include_shader_file = GetValue(CurrentDB, file_hash);

					// Check that a ShaderFile of this path exists
					if (include_shader_file != null)
					{
						// Check that we didn't parse this include already. Otherwise it means we have cyclic dependencies
						if (!outList.Contains(include_shader_file))
						{
							outList.Add(include_shader_file);

							// Recursion. Find all dependencies of all included files
							FindDependenciesInFile(include_shader_file, outList);
						}
						else
							throw new Exception(String.Format("Include path \"{0}\" in file \"{1}\" is a cyclic dependency.", include, inShaderFile.FullPath));
					}
					else
					{
						throw new Exception(String.Format("Include path \"{0}\" in file \"{1}\" exists, but wasn't parsed by the ShaderFileGatherer." +
							"Extension missing in configuration?", include, inShaderFile.FullPath));
					}
				}
				else
					throw new Exception(String.Format("Include path \"{0}\" in file \"{1}\" doesn't exist", include, inShaderFile.FullPath));
			}
		}

		private static void CreateDependencies()
		{
			// List all DIRECT and INDIRECT dependencies FROM each ShaderFiles
			foreach (KeyValuePair<UInt32, ShaderFile> pair in CurrentDB)
			{
				ShaderFile current_file = pair.Value;
				FindDependenciesInFile(current_file, current_file.Dependencies);
			}
		}

		// Go through all previous elements in the DB to detect ShaderFiles that were deleted, added, or modified
		// Set their ShouldCompile member variable to true if we need to recompile them
		// Returns true if the previous DB doesn't match the current DB
		private static bool UpdateDadaBase()
		{
			bool db_changed = false;

			// Process previous files in DB. Could still exist, or be deleted
			foreach (KeyValuePair<UInt32, ShaderFile> pair in PreviousDB)
			{
				ShaderFile previous_file	= pair.Value;
				ShaderFile current_file		= CurrentDB.GetValue(pair.Key);

				// File was not deleted
				if (current_file != null)
				{
					// Retry shaders that didn't compile last time
					bool file_changed = previous_file.DidCompile == false;
					// Recompile shaders that were modified
					file_changed |= (current_file.ModifiedTime != previous_file.ModifiedTime);
					// Recompile moved/renamed shaders 
					file_changed |= (current_file.FilenameHash != previous_file.FilenameHash);

					// Recompile if generated files don't exist
					if (previous_file.GeneratedFiles.Any(filename => !File.Exists(filename)))
					{
						file_changed = true;
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
			foreach (KeyValuePair<UInt32, ShaderFile> pair in CurrentDB)
			{
				ShaderFile current_file		= pair.Value;
				ShaderFile previous_file	= PreviousDB.GetValue(pair.Key);

				// Current file was added
				if (previous_file == null)
				{
					current_file.ShouldCompile	= true;
					db_changed = true;
				}
			}

			// Process dependencies
			foreach (KeyValuePair<UInt32, ShaderFile> pair in CurrentDB)
			{
				ShaderFile current_file = pair.Value;

				// Go through all direct and indirect dependencies
				foreach (ShaderFile dependency in current_file.Dependencies)
				{
					// Recompile if at least one of them was changed
					if (dependency.ShouldCompile)
					{
						current_file.ShouldCompile = true;
						break;
					}
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

			CreateDependencies();

			// Tag all shaders that need to recompile and check if the DB was changed compared to the previous build
			bool db_changed = UpdateDadaBase();
			if (db_changed)
			{
				ShaderFileParser fileParser = new ShaderFileParser(CurrentDB.Values.ToList());
				fileParser.ProcessFiles();

				// Generate \Shaders\include\Shaders.h
				ShaderCompiler.GenerateShaderHFile();

				// Generate \Shaders\include\ConstantBuffers.h
				ShaderCompiler.GenerateConstantBufferHFile(fileParser.Structs);

				// Generate \Shaders\include\VertexLayouts.h
				ShaderCompiler.GenerateVertexLayoutHFile(fileParser.Structs);

				// Update DB by overwritting it
				WriteDataBase();
			}
		}

		public static void BuildForTest()
		{
			// Go through all the files and create Shaderfiles
			ProcessFolder(Config.ShaderSourcePath);

			ShaderFileParser fileParser = new ShaderFileParser(CurrentDB.Values.ToList());

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
			CurrentDB = new Dictionary<UInt32, ShaderFile>();

			if (Arguments.IsClean())
				Clean();

			if (Arguments.IsBuild())
				Build();
			else if (Arguments.Operation == Arguments.OperationType.Test)
				BuildForTest();
		}
	}
}