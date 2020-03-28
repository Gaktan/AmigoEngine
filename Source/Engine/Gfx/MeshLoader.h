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

class MeshLoader
{
protected:
	std::vector<Vec4>				m_AllPositions;
	std::vector<Vec4>				m_AllNormals;
	std::vector<Vec4>				m_AllUVCoords;
	std::vector<VertexPosUVNormal>	m_VertexData;
	std::vector<uint16>				m_IndexData;
	std::map<std::string, uint16>	m_IndexMap;

	uint16							m_CurrentIndex = 0;

public:
	void	LoadFromFile(const std::string& inFile);
	Mesh*	CreateMeshObject(DX12Device& inDevice, ID3D12GraphicsCommandList2* inCommandList);

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