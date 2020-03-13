#include "Engine.h"
#include "MeshLoader.h"

#include "Utils/FileReader.h"
#include "Utils/String.h"

#include <sstream>

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

// Flip winding of geometric primitives for LH vs. RH coords
void MeshLoader::ReverseWinding()
{
	// Make sure the number of indices make up only triangles
	Assert((m_IndexData.size() % 3) == 0);

	for (auto it = m_IndexData.begin(); it != m_IndexData.end(); it += 3)
	{
		std::swap(*it, *(it + 2));
	}

	// Flip X axis
	for (auto it = m_VertexData.begin(); it != m_VertexData.end(); ++it)
	{
		it->Position.X() = (1.f - it->Position.X());
	}
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
	ReverseWinding();
}

Mesh * MeshLoader::CreateMeshObject(ID3D12Device* inDevice, ID3D12GraphicsCommandList2* inCommandList)
{
	Assert(m_VertexData.size() > 0);

	Mesh* mesh = new Mesh();

	mesh->Init(inDevice, inCommandList,
			   D3D_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
			   m_VertexData.data(), static_cast<uint32>(m_VertexData.size()) * sizeof(VertexPosUVNormal), sizeof(VertexPosUVNormal),
			   m_IndexData.data(), static_cast<uint32>(m_IndexData.size()) * sizeof(uint16));

	return mesh;
}

void MeshLoader::ProcessLine(const std::string& inLine)
{
	// Get all elements from line
	std::vector<std::string> all_elem = String::Split(inLine, " ");
	size_t num_elems = all_elem.size();
	Assert(num_elems > 0);

	const std::string& keyword_string = all_elem[0];
	OBJKeyword keyword = GetKeywordFromString(keyword_string);

	// Remove first element (keyword)
	all_elem.erase(all_elem.begin());
	num_elems = num_elems - 1;

	switch (keyword)
	{
	case OBJKeyword::VertexPosition:
	{
		Assert(num_elems == 3);

		float x = String::ToFloat(all_elem[0]);
		float y = String::ToFloat(all_elem[1]);
		float z = String::ToFloat(all_elem[2]);
		float w = 1.0;

		m_AllPositions.push_back(Vec4(x, y, z, w));
		break;
	}
	case OBJKeyword::VertexNormal:
	{
		Assert(num_elems == 3);

		float x = String::ToFloat(all_elem[0]);
		float y = String::ToFloat(all_elem[1]);
		float z = String::ToFloat(all_elem[2]);

		m_AllNormals.push_back(Vec4(x, y, z));
		break;
	}
	case OBJKeyword::VertexUV:
	{
		// Only support 2D UV coordinates
		Assert(num_elems == 2);

		float x = String::ToFloat(all_elem[0]);
		float y = String::ToFloat(all_elem[1]);

		m_AllUVCoords.push_back(Vec4(x, y, 0.0f));
		break;
	}
	case OBJKeyword::MaterialLibrary:
	case OBJKeyword::MaterialName:
		// Not supported for now, but harmless, so don't do anything
		break;

	case OBJKeyword::Point:
	case OBJKeyword::Line:
		Assert(false);
	case OBJKeyword::Polygon:
	{
		// Only support triangles
		Assert(num_elems == 3);
		ProcessTriangle(all_elem);
		break;
	}
	case OBJKeyword::ObjectName:
	case OBJKeyword::GroupName:
		// Don't do anything for now
		break;
	case OBJKeyword::SmoothingGroup:
	{
		Assert(num_elems == 1);
		// Don't do anything for now
		break;
	}
	case OBJKeyword::Comment:
		// Ignore comments
		break;

	case OBJKeyword::Invalid:
	default:
		Assert(false);
		break;
	}
}

void MeshLoader::ProcessTriangle(const std::vector<std::string>& inFaceElements)
{
	// Make sure we are dealing with a triangle
	Assert(inFaceElements.size() == 3);

	for (size_t i = 0; i < 3; i++)
	{
		const std::string& vertex_string = inFaceElements[i];

		auto index_search = m_IndexMap.find(vertex_string);
		if (index_search == m_IndexMap.end())
		{
			static uint16 s_Index = 0;
			m_IndexMap[vertex_string] = s_Index;
			m_IndexData.push_back(s_Index);
			s_Index++;

			std::vector<std::string> all_elem = String::Split(vertex_string, "/");
			size_t num_elems = all_elem.size();

			// Only support Position/UV/Normal for now
			Assert(num_elems == 3);

			// OBJ index starts at 1
			size_t position_index	= String::ToInt(all_elem[0]) - 1;
			size_t coords_index		= String::ToInt(all_elem[1]) - 1;
			size_t normal_index		= String::ToInt(all_elem[2]) - 1;

			// TODO: Only one Vertex layout for now
			VertexPosUVNormal vertex;
			vertex.Position		= m_AllPositions[position_index];
			vertex.UV			= m_AllUVCoords[coords_index];
			vertex.Normal		= m_AllNormals[normal_index];

			m_VertexData.push_back(vertex);
		}
		else
		{
			m_IndexData.push_back((*index_search).second);
		}
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
