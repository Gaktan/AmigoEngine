#include "Engine.h"
#include "MeshLoader.h"

#include "Utils/FileReader.h"

#include <sstream>
#include <vector>

std::map<std::string, OBJKeyword> MeshLoader::s_OBJKeywords;

const char* obj_keywords_strings[] =
{
	"v",			//VertexPosition
	"vn",			// VertexNormal
	"vt",			// VertexUV

	"mtllib",		// MaterialLibrary
	"usemtl",		// MaterialName

	"p",			// Point
	"l",			// Line
	"f",			// Polygon

	"o",			// ObjectName,
	"g",			// GroupName,
	"s",			// SmoothingGroup

	"#",			// Comment
	"Invalid"		// Invalid
 };

static_assert(_countof(obj_keywords_strings) == static_cast<int>(OBJKeyword::NumKeywords));

// Split string inStr with delimiter inDelimiter
std::vector<std::string> split(const std::string& inStr, const std::string& inDelimiter)
{
	size_t pos_start	= 0;
	size_t pos_end		= 0;
	size_t delim_len	= inDelimiter.length();

	std::string token;
	std::vector<std::string> result;

	while ((pos_end = inStr.find(inDelimiter, pos_start)) != std::string::npos)
	{
		token = inStr.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
		result.push_back(token);
	}

	result.push_back(inStr.substr(pos_start));
	return result;
}

void MeshLoader::Init()
{
	for (size_t i = 0; i < static_cast<size_t>(OBJKeyword::NumKeywords); i++)
	{
		OBJKeyword keyword = static_cast<OBJKeyword>(i);
		const char* keyword_string = obj_keywords_strings[i];
		s_OBJKeywords[keyword_string] = keyword;
	}
}

void MeshLoader::LoadFromFile(const std::string& inFile)
{
	Assert(s_OBJKeywords.size() > 0);

	FileReader file_reader;
	bool success = file_reader.ReadFile(inFile);
	Assert(success);

	std::istringstream stream(file_reader.GetContentAsString());
	std::string line;
	while (std::getline(stream, line))
	{
		ProcessLine(line);
	}
}

void MeshLoader::ProcessLine(const std::string& inLine)
{
	// Get all elements from line
	std::vector<std::string> all_elem = split(inLine, " ");
	size_t num_elems = all_elem.size();
	Assert(num_elems > 0);

	std::string keyword_string = all_elem[0];
	OBJKeyword keyword = GetKeywordFromString(keyword_string);

	// Remove first element (keyword)
	all_elem.erase(all_elem.begin());
	num_elems = num_elems - 1;

	switch (keyword)
	{
	case OBJKeyword::VertexPosition:
	case OBJKeyword::VertexNormal:
		Assert(num_elems == 3);
		break;
	case OBJKeyword::VertexUV:
		// Only support 2D UV coordinates
		Assert(num_elems == 2);
		break;

	case OBJKeyword::MaterialLibrary:
	case OBJKeyword::MaterialName:
		// Not supported for now, but harmless, so don't do anything
		break;

	case OBJKeyword::Point:
	case OBJKeyword::Line:
		Assert(false);
	case OBJKeyword::Polygon:
		// Only support triangles
		Assert(num_elems == 3);
		break;

	case OBJKeyword::ObjectName:
	case OBJKeyword::GroupName:
	case OBJKeyword::SmoothingGroup:
		// Don't do anything for now
		break;

		// Ignore comments
	case OBJKeyword::Comment:
		break;

	case OBJKeyword::Invalid:
	default:
		Assert(false);
		break;
	}
}

OBJKeyword MeshLoader::GetKeywordFromString(const std::string & inStr)
{
	auto search = s_OBJKeywords.find(inStr);

	if (search != s_OBJKeywords.end())
	{
		return (*search).second;
	}

	return OBJKeyword::Invalid;
}
