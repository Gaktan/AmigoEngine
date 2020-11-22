#include "Engine.h"
#include "MeshLoader.h"

#include "Utils/FileReader.h"
#include "Utils/String.h"

#include "Gfx/DrawableObject.h"
#include "Gfx/ShaderObject.h"

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

// Crude test to check if material should be transparent
// http://paulbourke.net/dataformats/mtl/
bool IsIlluminationModelTransparent(int inIlluminationModel)
{
	return (inIlluminationModel == 4 || inIlluminationModel == 6 ||
			inIlluminationModel == 7 || inIlluminationModel == 9);
}

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
		it->Position.x = (1.f - it->Position.x);
	}

	// Flip UV's Y axis
	// Blender uses OpenGL coords. Origin is at the bottom left corner.
	// Whereas DirectX's origin is at the top left corner
	for (auto it = m_VertexData.begin(); it != m_VertexData.end(); ++it)
	{
		it->UV.y = (1.f - it->UV.y);
	}
}

void MeshLoader::ProcessMaterialLibraryFile(const std::string& inFile)
{
	FileReader file_reader;
	// TODO: Hack: Find a nice way to read from the same directory as obj file?
	bool success = file_reader.ReadFile("Data\\" + inFile);
	Assert(success);

	std::istringstream stream(file_reader.GetContentAsString());
	std::string line;

	MaterialInfo* current_material = nullptr;

	// Process a single line
	while (String::GetLine(stream, line))
	{
		// TODO: Those spaces right there is a hack.
		const std::string newmtl("newmtl ");
		const std::string illum("illum ");

		// TODO: It's tricky to list all MTL keywords. Let's just detect the Illumination Model (illum) for transparency for now

		// Keyword to create a new material
		if (line.substr(0, newmtl.size()) == newmtl)
		{
			std::string material_name = line.substr(newmtl.size());
			Assert(m_MaterialInfos.find(material_name) == m_MaterialInfos.end());

			m_MaterialInfos[material_name] = MaterialInfo();

			current_material = &(*m_MaterialInfos.find(material_name)).second;
		}
		// Check Illumination
		else if (line.substr(0, illum.size()) == illum)
		{
			Assert(current_material != nullptr);

			std::string value_str = line.substr(illum.size());
			int illumination_model = String::ToInt(value_str);
			current_material->m_IsTransparent = IsIlluminationModelTransparent(illumination_model);
		}
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
	while (String::GetLine(stream, line))
	{
		ProcessLine(line);
	}

	// Close range on the last MeshInfo
	MeshInfo& last_mesh_info = *m_MeshInfos[m_CurrentMeshInfo];
	last_mesh_info.EndRange((int) m_VertexData.size(), (int) m_IndexData.size());

	// Blender exports meshes in Right Hand Coordinates. DX12 uses Left Hand. We need to flip the winding order
	ReverseWinding();
}

// Final step of loading OBJ files
// Create materials, create meshes, create Drawable objects
void MeshLoader::Finalize(ID3D12GraphicsCommandList2* inCommandList,
						  const std::map<std::string, ShaderObject*>& inShaderObjects, RenderBuckets& outBuckets)
{
	Assert(m_VertexData.size() > 0);

	for (size_t i = 0; i <= m_CurrentMeshInfo; i++)
	{
		MeshInfo* mesh_info = m_MeshInfos[i];

		const Range vertex_range	= mesh_info->m_VertexBuffeRange;
		const Range index_range		= mesh_info->m_IndexBufferRange;

		uint32 vertex_size	= static_cast<uint32>(vertex_range.m_End	- vertex_range.m_Start) * sizeof(VertexPosUVNormal);
		uint32 index_size	= static_cast<uint32>(index_range.m_End		- index_range.m_Start) * sizeof(uint16);

		Mesh* mesh = new Mesh();
		mesh->Init(inCommandList,
				   D3D_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
				   m_VertexData.data()	+ vertex_range.m_Start,	vertex_size, sizeof(VertexPosUVNormal),
				   m_IndexData.data()	+ index_range.m_Start,	index_size);

		// Set Mesh debug name
		const std::string mesh_name = mesh_info->m_ObjectName + "_" + mesh_info->m_MaterialName;
		mesh->SetResourceName(mesh_name);

		bool is_transparent = m_MaterialInfos[mesh_info->m_MaterialName].m_IsTransparent;
		const ShaderObject* shader_object = is_transparent ? inShaderObjects.at("Transparent") : inShaderObjects.at("OpaqueGeometry");
		DrawableObject* drawable = new DrawableObject(mesh, shader_object);
		outBuckets[(uint32) shader_object->GetRenderPass()].emplace_back(drawable);

		delete mesh_info;
	}
}

void MeshLoader::ProcessLine(const std::string& inLine)
{
	// Don't process empty lines
	if (inLine.empty())
	{
		return;
	}

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

		m_AllPositions.push_back(Vec4(x, y, z, 0));
		break;
	}
	case OBJKeyword::VertexNormal:
	{
		Assert(num_elems == 3);

		float x = String::ToFloat(all_elem[0]);
		float y = String::ToFloat(all_elem[1]);
		float z = String::ToFloat(all_elem[2]);

		m_AllNormals.push_back(Vec4(x, y, z, 0));
		break;
	}
	case OBJKeyword::VertexUV:
	{
		// Only support 2D UV coordinates
		Assert(num_elems == 2);

		float x = String::ToFloat(all_elem[0]);
		float y = String::ToFloat(all_elem[1]);

		m_AllUVCoords.push_back(Vec4(x, y, 0, 0));
		break;
	}
	case OBJKeyword::MaterialLibrary:
		// Should only be one element. The filename
		Assert(num_elems == 1);

		ProcessMaterialLibraryFile(all_elem[0]);
		break;
	case OBJKeyword::MaterialName:
	{
		// Special case for different materials within the same mesh.
		// Recreate a new mesh but still based on the current 

		Assert(m_MeshInfos.size() != 0);

		MeshInfo& current_mesh_info = *m_MeshInfos[m_CurrentMeshInfo];

		// If we haven't processed any vertices yet. Don't create a new MeshInfo. We are processing the first material of a new object
		if (m_IncrementalIndexValue != 0)
		{
			// Deal in the same was as if it was a new object.
			// TODO: Use the same vertex buffer with different index buffer?

			current_mesh_info.EndRange((int) m_VertexData.size(), (int) m_IndexData.size());

			MeshInfo* new_mesh_info = new MeshInfo(current_mesh_info);
			new_mesh_info->m_ObjectName = current_mesh_info.m_ObjectName;

			m_MeshInfos.push_back(new_mesh_info);

			m_IndexMap.clear();
			m_IncrementalIndexValue = 0;

			m_CurrentMeshInfo++;
		}

		// Actually set the material name for the current or new MeshInfo
		Assert(m_MeshInfos[m_CurrentMeshInfo]->m_MaterialName.length() == 0);
		Assert(m_MaterialInfos.find(all_elem[0]) != m_MaterialInfos.end());

		m_MeshInfos[m_CurrentMeshInfo]->m_MaterialName = all_elem[0];

		break;
	}
	case OBJKeyword::Point:
	case OBJKeyword::Line:
	{
		Assert(false);
		break;
	}
	case OBJKeyword::Polygon:
	{
		// Only support triangles
		Assert(num_elems == 3);
		ProcessTriangle(all_elem);
		break;
	}
	case OBJKeyword::ObjectName:
	{
		// Object name is different so we need to start creating a new mesh

		if (m_MeshInfos.size() == 0)
		{
			// Create first empty mesh info
			m_MeshInfos.push_back(new MeshInfo);
		}
		else
		{
			// Create new MeshInfo based on infos from the previous one
			MeshInfo& current_mesh_info = *m_MeshInfos[m_CurrentMeshInfo];
			current_mesh_info.EndRange((int) m_VertexData.size(), (int) m_IndexData.size());

			m_MeshInfos.push_back(new MeshInfo(current_mesh_info));

			m_IndexMap.clear();
			m_IncrementalIndexValue = 0;

			m_CurrentMeshInfo++;
		}

		// Actually set the object name for the new MeshInfo
		m_MeshInfos[m_CurrentMeshInfo]->m_ObjectName = all_elem[0];

		break;
	}
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
			m_IndexMap[vertex_string] = m_IncrementalIndexValue;
			m_IndexData.push_back(m_IncrementalIndexValue);
			m_IncrementalIndexValue++;

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

OBJKeyword MeshLoader::GetKeywordFromString(const std::string& inStr)
{
	auto search = s_OBJKeywords.find(inStr);
	if (search != s_OBJKeywords.end())
	{
		return (*search).second;
	}
	return OBJKeyword::Invalid;
}

MeshInfo::MeshInfo()
{
	m_VertexBuffeRange = { 0, -1 };
	m_IndexBufferRange = { 0, -1 };
}

MeshInfo::MeshInfo(const MeshInfo& inPreviousMeshInfo)
{
	m_VertexBuffeRange = { inPreviousMeshInfo.m_VertexBuffeRange.m_End, -1 };
	m_IndexBufferRange = { inPreviousMeshInfo.m_IndexBufferRange.m_End, -1 };
}

void MeshInfo::EndRange(int inVertexBufferEnd, int inIndexBufferEnd)
{
	m_VertexBuffeRange.m_End = inVertexBufferEnd;
	m_IndexBufferRange.m_End = inIndexBufferEnd;
}