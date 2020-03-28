#pragma once

#include <map>
#include <string>
#include <vector>

#include "Math/Vec4.h"

#include "Gfx/Mesh.h"

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

class MeshLoader
{
protected:
	std::vector<Vec4>				m_AllPositions;
	std::vector<Vec4>				m_AllNormals;
	std::vector<Vec4>				m_AllUVCoords;
	std::vector<VertexPosUVNormal>	m_VertexData;
	std::vector<uint16>				m_IndexData;
	std::map<std::string, uint16>	m_IndexMap;

	uint16							m_IncrementalIndexValue	= 0;

	std::vector<Range>				m_IndexBuffersRange;
	std::vector<Range>				m_VertexBufferRange;
	size_t							m_CurrentBufferRange	= 0;

public:
	void	LoadFromFile(const std::string& inFile);
	void	CreateMeshObjects(DX12Device& inDevice, ID3D12GraphicsCommandList2* inCommandList, std::vector<Mesh*>& outMeshes);

private:
	void	ProcessLine(const std::string& inLine);
	void	ProcessTriangle(const std::vector<std::string>& inFaceElements);
	void	ReverseWinding();

	// Static members
protected:
	static std::map<std::string, OBJKeyword> s_OBJKeywords;

	static OBJKeyword GetKeywordFromString(const std::string& inStr);
public:
	static void Init();
};