using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace ShaderCompiler
{
	class Config
	{
		// TODO: Fix this annoying warning
#pragma warning disable CS0649
		public static List<string> ShaderExtensions;
#pragma warning restore CS0649
		public static string ShaderSourcePath;
		public static string DatabasePath;
		public static string GeneratedFolderPath;
		public static string GeneratedHeaderExtension;

		public static string ShaderModel;

		public static void DebugPrint()
		{
			Console.Write("ShaderExtensions: ");
			foreach (string extension in ShaderExtensions)
			{
				Console.Write(extension + ", ");
			}
			Console.WriteLine();

			Console.WriteLine("ShaderSourcePath: " + ShaderSourcePath);
			Console.WriteLine("DatabasePath: " + DatabasePath);
			Console.WriteLine("GeneratedFolderPath: " + GeneratedFolderPath);
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

		private static string ResolvePath(string path, bool checkIfExists)
		{
			string resolvedPath = Path.Combine(Arguments.RootFolder, path);
			if (!File.Exists(resolvedPath) && !Directory.Exists(resolvedPath) && checkIfExists)
			{
				throw new Exception("Path \"" + resolvedPath + "\" is not a file or a directory.");
			}

			return resolvedPath;
		}

		private static void FillConfig()
		{
			Config.ShaderExtensions = new List<string>();
			foreach (string extension in Data["ShaderCompiler"]["ShaderExtensions"].Split(','))
			{
				Config.ShaderExtensions.Add(extension.Trim());
			}

			Config.ShaderSourcePath = ResolvePath(Data["ShaderCompiler"]["ShaderSourcePath"], true);
			Config.DatabasePath = ResolvePath(Data["ShaderCompiler"]["DatabasePath"], false);
			Config.GeneratedFolderPath = ResolvePath(Data["ShaderCompiler"]["GeneratedFolderPath"], false);

			Config.GeneratedHeaderExtension = Data["ShaderCompiler"]["GeneratedHeaderExtension"];
			Config.ShaderModel = Data["ShaderCompiler"]["ShaderModel"];

			Config.DebugPrint();
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