#pragma once

#include <map>
#include <string>
#include <vector>

#include "Gfx/Mesh.h"
#include "Gfx/RenderPass.h"

#include "Shaders/Include/VertexLayouts.h"
using namespace VertexFormats;

class ShaderObject;

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

struct Range
{
	int m_Start	= 0;
	int m_End	= -1;
};

struct MeshInfo
{
	Range				m_VertexBuffeRange;
	Range				m_IndexBufferRange;

	std::string			m_ObjectName;
	std::string			m_MaterialName;

	MeshInfo();
	MeshInfo(const MeshInfo& inPreviousMeshInfo);

	void EndRange(int inVertexBufferEnd, int inIndexBufferEnd);
};

struct MaterialInfo
{
	bool m_IsTransparent = false;
};

class MeshLoader
{
protected:
	// TODO: Hardcoded VertexFormat
	std::vector<Vec3>				m_AllPositions;
	std::vector<Vec3>				m_AllNormals;
	std::vector<Vec2>				m_AllUVCoords;
	std::vector<VertexPosUVNormal>	m_VertexData;
	std::vector<uint16>				m_IndexData;
	std::map<std::string, uint16>	m_IndexMap;

	uint16							m_IncrementalIndexValue	= 0;

	std::vector<MeshInfo*>			m_MeshInfos;
	size_t							m_CurrentMeshInfo		= 0;

	std::map<std::string, MaterialInfo>		m_MaterialInfos;

public:
	void	LoadFromFile(const std::string& inFile);
	void	Finalize(ID3D12GraphicsCommandList2* inCommandList,
					 const std::map<std::string, ShaderObject*>& inShaderObjects, RenderBuckets& outBuckets);

private:
	void	ProcessLine(const std::string& inLine);
	void	ProcessTriangle(const std::vector<std::string>& inFaceElements);
	void	ReverseWinding();
	void	ProcessMaterialLibraryFile(const std::string& inFile);

	// Static members
protected:
	static std::map<std::string, OBJKeyword> s_OBJKeywords;

	static OBJKeyword GetKeywordFromString(const std::string& inStr);
public:
	static void Init();
};