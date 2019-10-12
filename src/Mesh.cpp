#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include "Mesh.h"
#include "Model.h"
#include "InfoLog.h"

using std::vector;
using std::string;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT2;

MeshFactory gMF;

Mesh::Mesh(vector<XMFLOAT3>& pos,
	vector<XMFLOAT3>& nor,
	vector<XMFLOAT3>& tan,
	vector<XMFLOAT2>& td,
	vector<UINT>& ind, string name) : m_position(pos), m_normal(nor), 
	m_tangent(tan), m_texcoord(td), 
	m_index(ind), m_name(name), m_hasSetup(false) {}


bool Mesh::Setup(ID3D11Device* dev) {
	if (m_hasSetup) return true;
	static const char* funcTag = "Mesh::Setup: ";
	// create buffers
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA subData;
	size_t size = -1;
	ZeroMemory(&desc, sizeof(desc));
	ZeroMemory(&subData, sizeof(subData));

	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;

	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;

	auto fp = [&](size_t size, void* data,
		Microsoft::WRL::ComPtr<ID3D11Buffer>& buf,
		D3D11_BIND_FLAG flag = D3D11_BIND_VERTEX_BUFFER)->bool {
		desc.ByteWidth = static_cast<UINT>(size);
		desc.BindFlags = flag;
		subData.pSysMem = data;
		if (FAILED(dev->CreateBuffer(&desc, &subData, buf.ReleaseAndGetAddressOf()))) return false;
		return true;
	};

	// create position buffer
	if (!fp(m_position.size() * sizeof(XMFLOAT3),
		&m_position[0], m_bufPosition)) {
		MLOG(LL, funcTag, LW, "create position buffer failed!");
		return false;
	}
	
	// create normal buffer
	if (!fp(m_normal.size() * sizeof(XMFLOAT3),
		&m_normal[0], m_bufNormal)) {
		MLOG(LL, funcTag, LW, "create normal buffer failed!");
		return false;
	}

	// create tangent buffer
	if (!fp(m_tangent.size() * sizeof(XMFLOAT3),
		&m_tangent[0], m_bufTangent)) {
		MLOG(LL, funcTag, LW, "create tangent buffer failed!");
		return false;
	}

	// create texcoord buffer
	if (!fp(m_texcoord.size() * sizeof(XMFLOAT2),
		&m_texcoord[0], m_bufTexcoord)) {
		MLOG(LL, funcTag, LW, "create texcoord buffer failed!");
		return false;
	}

	// create index buffer
	if (!fp(m_index.size() * sizeof(UINT),
		&m_index[0], m_bufIndex, D3D11_BIND_INDEX_BUFFER)) {
		MLOG(LL, funcTag, LW, "create index buffer failed!");
		return false;
	}
	m_hasSetup = true;
	return true;
}

void Mesh::Bind(ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot) {
	static UINT stride = -1;
	static UINT offset = 0;
	// Model
	// position
	stride = sizeof(XMFLOAT3);
	devCtx->IASetVertexBuffers(0, 1, m_bufPosition.GetAddressOf(), &stride, &offset);
	// normal
	devCtx->IASetVertexBuffers(1, 1, m_bufNormal.GetAddressOf(), &stride, &offset);
	// tangent
	devCtx->IASetVertexBuffers(2, 1, m_bufTangent.GetAddressOf(), &stride, &offset);
	// texcoord
	stride = sizeof(XMFLOAT2);
	devCtx->IASetVertexBuffers(3, 1, m_bufTexcoord.GetAddressOf(), &stride, &offset);
	// index
	stride = sizeof(UINT);
	devCtx->IASetIndexBuffer(m_bufIndex.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Draw call
	devCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devCtx->DrawIndexed(static_cast<UINT>(m_index.size()), 0, 0);
}

void Mesh::Unbind(ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot) {
	static ID3D11Buffer* nullBuffer[] = { nullptr, nullptr, nullptr, nullptr };
	static UINT nullUint[] = { 0, 0, 0, 0 };
	devCtx->IASetVertexBuffers(0, 4, nullBuffer, nullUint, nullUint);
	devCtx->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	devCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	return;
}

// 获取Mesh中的位置信息
// 输出: 获取成功返回true,否则返回false
// 输入: 
// data: 获取成功时返回指向顶点位置信息的数组的指针，否则返回空指针
// size: 获取成功返回顶点位置信息数组的长度，获取失败时返回-1

bool Mesh::GetPosition(const void*& data, size_t& size)
{
	// 有位置信息才能够获取
	if (m_position.size()) {
		data = &m_position[0];
		size = m_position.size() * sizeof(XMFLOAT3);
		return true;
	}
	else {
		data = NULL;
		size = -1;
		MLOG(LL, "Mesh::GetPosition: There're no position data");
		return false;
	}
	return false;
}

// 获取Mesh中的法线信息
// 输出: 获取成功返回true, 否则返回false
// 输入: 
// data: 获取成功返回指向法线信息数组的指针，否则返回空指针
// size: 获取成功时返回法线信息数组的长度，否则返回-1

bool Mesh::GetNormal(const void*& data, size_t& size) {
	// 有法线信息才能够获取
	if (m_normal.size()) {
		data = &m_normal[0];
		size = m_normal.size() * sizeof(XMFLOAT3);
		return true;
	}
	else {
		data = NULL;
		size = -1;
		MLOG(LL, "Mesh::GetNormal: There're no normal data");
		return false;
	}
	return false;
}

// 获取Mesh中的切线信息
// 输出: 获取成功返回true, 获取失败返回false
// 输入:
// data: 获取成功时返回指向切线信息数组的指针，否则返回空指针
// size: 获取成功返回切线信息数组的长度，否则返回-1

bool Mesh::GetTangent(const void*& data, size_t& size) {
	if (m_tangent.size()) {
		data = &m_tangent[0];
		size = m_tangent.size() * sizeof(XMFLOAT3);
		return true;
	}
	else {
		data = NULL;
		size = -1;
		MLOG(LL, "Mesh::GetTangent: There're no tangent data");
		return false;
	}
	return false;
}

// 获取Mesh中的顶点UV信息

bool Mesh::GetTexcoord(const void*& data, size_t& size) {
	if (m_texcoord.size()) {
		data = &m_texcoord[0];
		size = m_texcoord.size() * sizeof(XMFLOAT2);
		return true;
	}
	else {
		data = NULL;
		size = -1;
		MLOG(LL, "Mesh::GetTexcoord: There're no texture coordinate data");
		return false;
	}
	return false;
}

bool Mesh::GetIndex(const void*& data, size_t& size) {
	if (m_index.size()) {
		data = &m_index[0];
		size = m_index.size() * sizeof(UINT);
		return true;
	}
	else {
		data = NULL;
		size = -1;
		MLOG(LL, "Mesh::GetIndex: There're no index data");
		return false;
	}
	return false;
}

Box Mesh::GetLocalBoundingBox() const
{
	Box res;
	res.vMax = res.vMin = m_position[0];
	for (const auto& pos : m_position) {
		res.vMax.x = pos.x > res.vMax.x ? pos.x : res.vMax.x;
		res.vMax.y = pos.y > res.vMax.y ? pos.y : res.vMax.y;
		res.vMax.z = pos.z > res.vMax.z ? pos.z : res.vMax.z;

		res.vMin.x = pos.x < res.vMin.x ? pos.x : res.vMin.x;
		res.vMin.y = pos.y < res.vMin.y ? pos.y : res.vMin.y;
		res.vMin.z = pos.z < res.vMin.z ? pos.z : res.vMin.z;
	}
	return res;
}

string& Mesh::GetName() { return m_name; }

////////////////////////////////////////////////////////////////////////////////////////
// Mesh Factory!
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////
// Public Fuction
////////////////////////////

std::vector<std::shared_ptr<Mesh>>& MeshFactory::Load(const char* path) {
	static const char* funcTag = "ModelFactory::Load: ";
	m_tempMesh.clear();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		/*使用Direct时候必须将模型文件中的坐标转换成左手坐标系*/
		aiProcess_ConvertToLeftHanded);
	if (!scene) {
		// 文件读取失败
		MLOG(LE, funcTag, LL, "create importer failed!");
		return m_tempMesh;
	}

	processNode(scene->mRootNode, scene);
	if (!m_tempMesh.size()) {
		// 没有任何的网格被载入
		MLOG(LW, funcTag, LL, "there is no mesh has been loaded!");
		return m_tempMesh;
	}

	return m_tempMesh;
}

std::shared_ptr<Mesh> MeshFactory::CreatePlane(float width, float height) {
	// create position/normal/tangent/texcoord/index buffer data
	// position
	std::vector<DirectX::XMFLOAT3> position = {
		{ width / -2.0f, height / 2.0f, 0.0f },
		{ width / 2.0f, height / 2.0f, 0.0f },
		{ width / 2.0f, height / -2.0f, 0.0f },
		{ width / -2.0f, height / -2.0f, 0.0f }
	};
	// normal
	std::vector<DirectX::XMFLOAT3> normal = {
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f }
	};
	// tangent
	std::vector<DirectX::XMFLOAT3> tangent = {
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f }
	};
	// texcoord
	std::vector<DirectX::XMFLOAT2> texcoord = {
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
		{ 0.0f, 1.0f }
	};
	// index
	std::vector<UINT> index = {
		0, 1, 2,
		0, 2, 3
	};
	return std::make_shared<Mesh>(position, normal, tangent, texcoord, index);
}

std::shared_ptr<Mesh> MeshFactory::CreateCube(DirectX::XMFLOAT3 min, DirectX::XMFLOAT3 max) {
	/**
			8----7
		  / |     / |
		5-4---6 3
		 |/      | /
		 1----2
	*/
	constexpr float rd3 = 1.732f; /**< 根号3 */
	std::vector<DirectX::XMFLOAT3> positions = {
		min,
		{max.x, min.y, min.z},
		{max.x, min.y, max.z},
		{min.x, min.y, max.z},
		{min.x, max.y, min.z},
		{max.x, max.y, min.z},
		max,
		{min.x, max.y, max.z}
	};
	std::vector<DirectX::XMFLOAT3> normals = {
		{ -rd3, -rd3, -rd3 },
		{ rd3, -rd3, -rd3 },
		{ rd3, -rd3, rd3 },
		{ -rd3, -rd3, rd3 },
		{ -rd3, rd3, -rd3 },
		{ rd3, rd3, -rd3 },
		{ rd3, rd3, rd3 },
		{ -rd3, rd3, rd3 }
	};
	/** 切线坐标和纹理坐标无效 */
	std::vector<DirectX::XMFLOAT3> tangents(8);
	std::vector<DirectX::XMFLOAT2> texcoords(8);
	std::vector<uint32_t> indices = {
		0, 4, 5, 0, 5, 1,
		1, 5, 6, 1, 6, 2,
		2, 6, 7, 2, 7, 3,
		3, 7, 4, 3, 4, 0,
		4, 7 ,6, 4, 6, 5,
		1, 2, 3, 1, 3, 0
	};
	return std::make_shared<Mesh>(positions, normals, tangents, texcoords, indices);
}

//////////////////////////
// Private Function
//////////////////////////

void MeshFactory::processNode(aiNode* node, const aiScene* scene) {
	for (UINT i = 0; i < node->mNumMeshes; ++i) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		Mesh* t = processMesh(mesh, scene);
		if (!t) continue;
		m_tempMesh.push_back(std::shared_ptr<Mesh>(t));
	}
	for (UINT i = 0; i < node->mNumChildren; ++i) {
		processNode(node->mChildren[i], scene);
	}
}

Mesh* MeshFactory::processMesh(aiMesh* mesh, const aiScene* scene) {

	static const char* funcTag = "ModelFactory::processMesh: ";
	if (!mesh->HasNormals()) {
		MLOG(LL, funcTag, LW, "this mesh has no normal data! create failed");
		return NULL;
	}
	if (!mesh->HasTangentsAndBitangents()) {
		MLOG(LL, funcTag, LW, "this mesh has no tangent data! create failed");
		return NULL;
	}
	if (!mesh->HasTextureCoords(0)) {
		MLOG(LL, funcTag, LW, "this mesh has no texture coordinate data! create failed!");
		return NULL;
	}

	vector<XMFLOAT3> position;
	vector<XMFLOAT3> normal;
	vector<XMFLOAT3> tangent;
	vector<XMFLOAT2> texcoord;
	vector<UINT>			 index;

	for (UINT i = 0; i < mesh->mNumVertices; ++i) {
		position.push_back(XMFLOAT3(
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z
		));

		normal.push_back(XMFLOAT3(
			mesh->mNormals[i].x,
			mesh->mNormals[i].y,
			mesh->mNormals[i].z
		));

		tangent.push_back(XMFLOAT3(
			mesh->mTangents[i].x,
			mesh->mTangents[i].y,
			mesh->mTangents[i].z
		));

		texcoord.push_back(XMFLOAT2(
			mesh->mTextureCoords[0][i].x,
			mesh->mTextureCoords[0][i].y
		));
	}

	for (UINT i = 0; i < mesh->mNumFaces; ++i) {
		for (UINT j = 0; j < mesh->mFaces[i].mNumIndices; ++j) {
			index.push_back(mesh->mFaces[i].mIndices[j]);
		}
	}

	return new Mesh(position, normal, tangent, texcoord, index);
}

bool PointSet::Setup(ID3D11Device* dev)
{
	if (m_hasSetup) return true;

	// create buffers
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA subData;
	size_t size = -1;
	ZeroMemory(&desc, sizeof(desc));
	ZeroMemory(&subData, sizeof(subData));

	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;

	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;

	auto fp = [&](size_t size, void* data,
		Microsoft::WRL::ComPtr<ID3D11Buffer>& buf,
		D3D11_BIND_FLAG flag = D3D11_BIND_VERTEX_BUFFER)->bool {
			desc.ByteWidth = static_cast<UINT>(size);
			desc.BindFlags = flag;
			subData.pSysMem = data;
			if (FAILED(dev->CreateBuffer(&desc, &subData, buf.ReleaseAndGetAddressOf()))) return false;
			return true;
	};

	// create position buffer
	if (!fp(m_positions.size() * sizeof(XMFLOAT3),
		&m_positions[0], m_bufPosition)) {
		MLOG(LL, __FUNCTION__ , LW, "create position buffer failed!");
		return false;
	}

	m_hasSetup = true;
	return true;
}

void PointSet::Bind(ID3D11DeviceContext* devCtx, ShaderBindTarget, SIZE_T)
{
	static UINT stride = 0;
	static UINT offset = 0;
	// position
	stride = sizeof(XMFLOAT3);
	devCtx->IASetVertexBuffers(0, 1, m_bufPosition.GetAddressOf(), &stride, &offset);

	// Draw call
	devCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	devCtx->Draw(static_cast<UINT>(m_positions.size()), 0);
}

void PointSet::Unbind(ID3D11DeviceContext* devCtx, ShaderBindTarget, SIZE_T) {
	static ID3D11Buffer* nullBuffer[] = { nullptr };
	static UINT nullUint[] = { 0 };
	devCtx->IASetVertexBuffers(0, 1, nullBuffer, nullUint, nullUint);
	devCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	return;
}
