#pragma once

#include <string>
#include <map>

enum class OBJKeyword
{
	// Per Vertex Data
	VertexPosition = 0,
	VertexNormal,
	VertexUV,

	// Material
	MaterialLibrary,
	MaterialName,

	// Element type
	Point,
	Line,
	Polygon,

	// Grouping
	ObjectName,
	GroupName,
	SmoothingGroup,

	Comment,

	Invalid,

	NumKeywords
};

class MeshLoader
{
protected:
	static std::map<std::string, OBJKeyword> s_OBJKeywords;

public:
	static void Init();
	static void LoadFromFile(const std::string& inFile);

private:
	static void ProcessLine(const std::string& inLine);
	static OBJKeyword GetKeywordFromString(const std::string& inStr);
};