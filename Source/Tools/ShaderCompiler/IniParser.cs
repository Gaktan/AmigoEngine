using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace ShaderCompiler
{
	class Config
	{
		public static List<string>	ShaderExtensions;
		public static string		ShaderSourcePath;
		public static string		ShaderHFile;
		public static string		DatabasePath;
		public static string		GeneratedFolderPath;
		public static string		GeneratedHeaderExtension;
		public static bool			EnableDebugInformation;

		public static Compiler		Compiler;
		public static ShaderModel	ShaderModel;

		public static List<string>  GlobalDefines;

		private static bool Throw(string exception, bool throws)
		{
			if (throws)
			{
				throw new Exception(exception);
			}

			return false;
		}

		// Make sure the current config is valid
		public static bool Verify(bool throws)
		{
			//Config.DebugPrint();

			if (Config.Compiler == Compiler.FXC)
			{
				if (Config.ShaderModel > ShaderModel._5_1)
				{
					return Throw("FXC doesn't support shader model beyond 5.1", throws);
				}
			}

			return true;
		}

		public static void DebugPrint()
		{
			Console.WriteLine();

			Console.WriteLine("ShaderExtensions:			" + String.Join(", ", ShaderExtensions.ToArray()));
			Console.WriteLine("ShaderSourcePath:			" + ShaderSourcePath);
			Console.WriteLine("ShaderHFile:					" + ShaderHFile);
			Console.WriteLine("DatabasePath:				" + DatabasePath);
			Console.WriteLine("GeneratedFolderPath:			" + GeneratedFolderPath);
			Console.WriteLine("GeneratedHeaderExtension:	" + GeneratedHeaderExtension);
			Console.WriteLine("EnableDebugInformation:		" + EnableDebugInformation);
			Console.WriteLine("Compiler:					" + Compiler);
			Console.WriteLine("ShaderModel:					" + ShaderModel);
			Console.WriteLine("GlobalDefines:				" + String.Join(", ", GlobalDefines.ToArray()));

			Console.WriteLine();
		}
	}

	class IniParser
	{
		// Captures a Section between 2 square brackets and puts the name in Group1
		private static readonly string SectionRegex = @"\[(.*)\]";
		// Captures a key and value. Group1: Key, Group2: Value
		private static readonly string KeyRegex = @"\s*(.+)\s*=\s*(.*)\s*";
		// Captures a commented line. A comment can start with either ; or #
		private static readonly string CommentRegex = @"\s*[;#](.*)";
		// Captures an empty line
		private static readonly string EmptyLineRegex = @"^\s*$";

		private static readonly string PathToDefaultIni = @"Resources\defaultconfig.ini";

		// 2D Hashmap containing: <SectionName, <key, value>>
		public static Dictionary<string, Dictionary<string, string>> Data;
		private static string CurrentSectionKey;

		public static void ParseLine(string line)
		{
			// Is this line a comment?
			Regex commentReg = new Regex(CommentRegex);
			Match commentMatch = commentReg.Match(line);
			if (commentMatch.Success)
			{
				// Ignore it
				return;
			}

			// Is this line empty?
			Regex emptyLineReg = new Regex(EmptyLineRegex);
			Match emptyLineMatch = emptyLineReg.Match(line);
			if (emptyLineMatch.Success)
			{
				// Ignore it
				return;
			}

			// Is this line a section?
			Regex sectionReg = new Regex(SectionRegex);
			Match sectionMatch = sectionReg.Match(line);
			if (sectionMatch.Success)
			{
				// Add the section to the data
				string sectionName = sectionMatch.Groups[1].ToString();
				if (!Data.ContainsKey(sectionName))
				{
					Data.Add(sectionName, new Dictionary<string, string>());
				}
				CurrentSectionKey = sectionName;

				return;
			}

			// Is this line a key?
			Regex keyReg = new Regex(KeyRegex);
			Match keyMatch = keyReg.Match(line);
			if (keyMatch.Success)
			{
				// Add the key and its value to the current section
				string key = keyMatch.Groups[1].ToString();
				string value = keyMatch.Groups[2].ToString();

				// INI values can be escaped, unescape them
				value = Regex.Unescape(value);

				if (!Data.ContainsKey(CurrentSectionKey))
				{
					throw new Exception("Missing section in INI configuration file before line \"" + line + "\"");
				}

				Data[CurrentSectionKey][key] = value;

				return;
			}

			// Any line that is not a empty, a comment, a section or a key is invalid
			throw new Exception("Invalid line \"" + line + "\" in INI configuration file.");
		}

		public static void DebugPrint()
		{
			foreach (var section in Data)
			{
				Console.WriteLine("Section = " + section.Key);
				foreach (var key in section.Value)
				{
					Console.Write("\t");
					Console.WriteLine(key.Key + " = " + key.Value);
				}
			}
		}

		public static void ParseIniFile(string filePath)
		{
			// Reset current key
			CurrentSectionKey = "";
			string fileContent = File.ReadAllText(filePath);

			// For each line
			using (StringReader reader = new StringReader(fileContent))
			{
				string line = null;
				while ((line = reader.ReadLine()) != null)
				{
					ParseLine(line);
				}
			}

			//DebugPrint();
		}

		private static string ResolvePath(string path, bool mustExist)
		{
			string resolvedPath = Path.Combine(Arguments.RootFolder, path);
			if (mustExist && !File.Exists(resolvedPath) && !Directory.Exists(resolvedPath))
			{
				throw new Exception("Path \"" + resolvedPath + "\" is not a file or a directory.");
			}

			return resolvedPath;
		}

		private static void FillConfig()
		{
			string[] separators = { "," };

			Config.ShaderExtensions = new List<string>();
			foreach (string extension in Data["ShaderCompiler"]["ShaderExtensions"].Split(separators, StringSplitOptions.RemoveEmptyEntries))
			{
				Config.ShaderExtensions.Add(extension.Trim());
			}

			Config.ShaderSourcePath			= ResolvePath(Data["ShaderCompiler"]["ShaderSourcePath"], true);
			Config.ShaderHFile				= ResolvePath(Data["ShaderCompiler"]["ShaderHFile"], true);
			Config.DatabasePath				= ResolvePath(Data["ShaderCompiler"]["DatabasePath"], false);
			Config.GeneratedFolderPath		= ResolvePath(Data["ShaderCompiler"]["GeneratedFolderPath"], false);
			Config.GeneratedHeaderExtension = Data["ShaderCompiler"]["GeneratedHeaderExtension"];

			// EnableDebugInformation
			bool succes						= Boolean.TryParse(Data["ShaderCompiler"]["EnableDebugInformation"], out Config.EnableDebugInformation);
			if (!succes)
			{
				Config.EnableDebugInformation = false;
			}

			// Weird setup, this is to make code smaller
			string currentKey = "";
			try
			{
				currentKey			= "Compiler";
				Config.Compiler		= EnumUtils.FromDescription<Compiler>(Data["ShaderCompiler"][currentKey]);

				currentKey			= "ShaderModel";
				Config.ShaderModel	= EnumUtils.FromDescription<ShaderModel>(Data["ShaderCompiler"][currentKey]);
			}
			catch (EnumException e)
			{
				throw new Exception(e.Message + " when parsing config file. Key=" + currentKey);
			}

			Config.GlobalDefines = new List<string>();
			foreach (string define in Data["ShaderCompiler"]["GlobalDefines"].Split(separators, StringSplitOptions.RemoveEmptyEntries))
			{
				Config.GlobalDefines.Add(define.Trim());
			}

			Config.Verify(throws : true);
		}

		public static void Parse()
		{
			Data = new Dictionary<string, Dictionary<string, string>>();

			string pathToExe = System.Reflection.Assembly.GetEntryAssembly().Location;
			string pathToDefaultIniFile = Path.GetDirectoryName(pathToExe) + @"\" + PathToDefaultIni;

			if (!File.Exists(pathToDefaultIniFile))
			{
				throw new Exception("Could not locate default INI file. Looking for \"" + pathToDefaultIniFile + "\"");
			}

			// Parse default file first to fill the data with default values
			ParseIniFile(pathToDefaultIniFile);

			// Parse user ini file
			if (Arguments.ConfigFile != null)
			{
				// We are already checking when parsing arguments, but just to be safe
				if (!File.Exists(Arguments.ConfigFile))
				{
					throw new Exception("Could not locate user INI file. Looking for \"" + Arguments.ConfigFile + "\"");
				}

				ParseIniFile(Arguments.ConfigFile);
			}

			// Fill Config class with settings from the INI file(s) we just parsed
			FillConfig();
		}
	}
}