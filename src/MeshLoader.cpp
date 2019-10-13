#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include <iostream>
#include <algorithm>
#include "MeshLoader.h"

using std::vector;
using std::string;

namespace Mesh_Loader {

	float3 FromVec3DToFloat3(const aiVector3D& input) {
		return { input.x, input.y, input.z };
	}
	float2 FromVec3DToFloat2(const aiVector3D& input) {
		return { input.x, input.y };
	}

	MeshLoader gML;

	size_t MeshMetaData::SizeOfPosition = sizeof(float3), 
		MeshMetaData::SizeOfNormal = sizeof(float3), 
		MeshMetaData::SizeOfTexcoord = sizeof(float2),
		MeshMetaData::SizeOfTangent = sizeof(float3),
		MeshMetaData::SizeOfIndex = sizeof(uint32_t);

	MeshMetaData::MeshMetaData(MeshMetaData&& rhs) {
		m_positions.swap(rhs.m_positions);
		m_normals.swap(rhs.m_normals);
		m_texcoords.swap(rhs.m_texcoords);
		m_tangents.swap(rhs.m_tangents);
		m_indices.swap(rhs.m_indices);
		m_localBoundingBoxMax = rhs.m_localBoundingBoxMax;
		m_localBoundingBoxMin = rhs.m_localBoundingBoxMin;
	}

	MeshMetaData & MeshMetaData::operator=(MeshMetaData && rhs)
	{
		m_positions.swap(rhs.m_positions);
		m_normals.swap(rhs.m_normals);
		m_texcoords.swap(rhs.m_texcoords);
		m_tangents.swap(rhs.m_tangents);
		m_indices.swap(rhs.m_indices);
		m_localBoundingBoxMax = rhs.m_localBoundingBoxMax;
		m_localBoundingBoxMin = rhs.m_localBoundingBoxMin;
		return *this;
	}

	MeshMetaData & MeshMetaData::operator=(const MeshMetaData & rhs)
	{
		m_positions = rhs.m_positions;
		m_normals = rhs.m_normals;
		m_texcoords = rhs.m_texcoords;
		m_tangents = rhs.m_tangents;
		m_indices = rhs.m_indices;
		m_localBoundingBoxMax = rhs.m_localBoundingBoxMax;
		m_localBoundingBoxMin = rhs.m_localBoundingBoxMin;
		return *this;
	}

	

	const float3 * MeshMetaData::PositionData() const {
		if (m_positions.empty()) { return nullptr; }
		else return m_positions.data();
	}

	const float3* MeshMetaData::NormalData() const {
		if (m_normals.empty()) return nullptr;
		return m_normals.data();
	}

	const float2* MeshMetaData::TexcoordData(size_t index) const {
		if (m_texcoords.size() > index) { return m_texcoords[index].data(); }
		else return nullptr;
	}

	const float3* MeshMetaData::TangentData() const {
		if (m_tangents.empty()) { return nullptr; }
		return m_tangents.data();
	}

	const uint32_t* MeshMetaData::IndexData() const {
		if (m_indices.empty()) { return nullptr; }
		return m_indices.data();
	}

	size_t MeshMetaData::NumberOfVertices() const {
		return m_positions.size();
	}

	size_t MeshMetaData::NumberOfIndices() const {
		return m_indices.size();
	}

	std::pair<float3, float3> MeshMetaData::LocalBoundingBox() const {
		return { m_localBoundingBoxMin, m_localBoundingBoxMax };
	}

	std::pair<float3, float> MeshMetaData::LocalBoundingSphere() const {
		float3 dimensions;
		dimensions[0] = m_localBoundingBoxMax[0] - m_localBoundingBoxMin[0];
		dimensions[1] = m_localBoundingBoxMax[1] - m_localBoundingBoxMin[1];
		dimensions[2] = m_localBoundingBoxMax[2] - m_localBoundingBoxMin[2];
		float diagonal = dimensions[0] * dimensions[0] + dimensions[1] * dimensions[1] + dimensions[2] * dimensions[2];
		diagonal = std::sqrtf(diagonal) / 2;
		return {dimensions, diagonal};
	}

	std::vector<const MeshMetaData*> MeshLoader::Load(const char* path) {
		std::vector<const MeshMetaData*> output;
		std::lock_guard<decltype(m_loaderMutex)> lg(m_loaderMutex);
		auto record = m_loadedMeshes.find(path); {
			if (record == m_loadedMeshes.end()) {
				/** 当前没有加载过，需要重新加载 */
				m_curLoadingMeshName = path;
				Assimp::Importer importer;
				const aiScene* scene = importer.ReadFile(path,
					aiProcess_CalcTangentSpace |
					aiProcess_Triangulate |
					/*使用Direct时候必须将模型文件中的坐标转换成左手坐标系*/
					(m_useRightHandCoordinate ? 0 : aiProcess_ConvertToLeftHanded));
				if (!scene) {
					// 文件读取失败
					std::cerr << __FUNCTION__ << " load mesh file failed! \n";
					assert(false);
					return {};
				}

				auto&& loadedMeshes = processNode(scene->mRootNode, scene);
				if (loadedMeshes.empty()) {
					std::cerr << __FUNCTION__ << " no mesh data was loaded!\n";
					return {};
				}
				else {
					auto[record, insertRes] = m_loadedMeshes.insert(std::make_pair(path, std::move(loadedMeshes)));
					assert(insertRes == true);
					for (auto& submesh : record->second) {
						output.push_back(&submesh);
					}
				}
			}
			else {
				for (auto& submesh : record->second) {
					output.push_back(&submesh);
				}
			}
		}

		return output;
	}

	const MeshMetaData* MeshLoader::CreatePlane(float width, float height)
	{
		/** 构造这个网格的“签名” */
		std::string meshName = __FUNCTION__;
		meshName += "Width: ";
		meshName += std::to_string(width);
		meshName += "Height: ";
		meshName += std::to_string(height);
		const MeshMetaData* output = nullptr;
		std::lock_guard<decltype(m_loaderMutex)> lg(m_loaderMutex);
		auto record = m_loadedMeshes.find(meshName);
		if (record == m_loadedMeshes.end()) {
			/** 需要创建新的网格 */
			MeshMetaData newMesh;
			// create position/normal/tangent/texcoord/index buffer data
			// position
			newMesh.m_positions = {
				{ width / -2.0f, height / 2.0f, 0.0f },
				{ width / 2.0f, height / 2.0f, 0.0f },
				{ width / 2.0f, height / -2.0f, 0.0f },
				{ width / -2.0f, height / -2.0f, 0.0f }
			};
			// normal
			newMesh.m_normals = {
				{ 0.0f, 0.0f, -1.0f },
				{ 0.0f, 0.0f, -1.0f },
				{ 0.0f, 0.0f, -1.0f },
				{ 0.0f, 0.0f, -1.0f }
			};
			// tangent
			newMesh.m_tangents = {
				{ 1.0f, 0.0f, 0.0f },
				{ 1.0f, 0.0f, 0.0f },
				{ 1.0f, 0.0f, 0.0f },
				{ 1.0f, 0.0f, 0.0f }
			};
			// texcoord
			newMesh.m_texcoords[0] = {
				{ 0.0f, 0.0f },
				{ 1.0f, 0.0f },
				{ 1.0f, 1.0f },
				{ 0.0f, 1.0f }
			};
			// index
			newMesh.m_indices = {
				0, 1, 2,
				0, 2, 3
			};
			newMesh.m_localBoundingBoxMin = { width / -2.0f, height / -2.0f, 0.0f };
			newMesh.m_localBoundingBoxMax = { width / 2.0f, height / 2.0f, 0.0f };
			{
				std::vector<MeshMetaData> plane;
				plane.push_back(std::move(newMesh));
				auto [record, insertRes] = m_loadedMeshes.insert(std::make_pair(meshName, std::move(plane)));
				assert(insertRes == true);
				output = &record->second[0];
			}
		}
		else {
			output = &record->second[0];
		}
		return output;
	}

	const MeshMetaData * MeshLoader::CreateCube(float3 min, float3 max)
	{
		/** 构造这个网格的“签名” */
		std::string meshName = __FUNCTION__;
		meshName += "Min: ";
		for (auto v : {0, 1, 2}) { meshName += std::to_string(min[v]); }
		meshName += "Max: ";
		for (auto v : { 0, 1, 2 }) { meshName += std::to_string(max[v]); }
		const MeshMetaData* output = nullptr;
		auto record = m_loadedMeshes.find(meshName);
		if (record == m_loadedMeshes.end()) {
			MeshMetaData newMesh;
			/** 需要重新创建 */
			/**
				8----7
			  / |     / |
			5-4---6 3
			 |/      | /
			 1----2
			*/
			constexpr float rd3 = 1.732f; /**< 根号3 */
			newMesh.m_positions = {
				min,
				{max.x, min.y, min.z},
				{max.x, min.y, max.z},
				{min.x, min.y, max.z},
				{min.x, max.y, min.z},
				{max.x, max.y, min.z},
				max,
				{min.x, max.y, max.z}
			};
			newMesh.m_normals = {
				{ -rd3, -rd3, -rd3 },
				{ rd3, -rd3, -rd3 },
				{ rd3, -rd3, rd3 },
				{ -rd3, -rd3, rd3 },
				{ -rd3, rd3, -rd3 },
				{ rd3, rd3, -rd3 },
				{ rd3, rd3, rd3 },
				{ -rd3, rd3, rd3 }
			};
			/** TODO: 切线坐标和纹理坐标缺省 */
			newMesh.m_indices = {
				0, 4, 5, 0, 5, 1,
				1, 5, 6, 1, 6, 2,
				2, 6, 7, 2, 7, 3,
				3, 7, 4, 3, 4, 0,
				4, 7 ,6, 4, 6, 5,
				1, 2, 3, 1, 3, 0
			};
			newMesh.m_localBoundingBoxMin = min;
			newMesh.m_localBoundingBoxMax = max;
			{
				std::vector<MeshMetaData> cube;
				cube.push_back(std::move(newMesh));
				auto[record, insertRes] = m_loadedMeshes.insert(std::make_pair(meshName, std::move(cube)));
				assert(insertRes == true);
				output = &record->second[0];
			}
		}
		else {
			output = &record->second[0];
		}
		return output;
	}

	std::vector<MeshMetaData> MeshLoader::processNode(aiNode* node, const aiScene* scene) {
		std::vector<MeshMetaData> output;
		for (size_t i = 0; i < node->mNumMeshes; ++i) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			auto newMesh = processMesh(mesh, scene);
			if (newMesh.has_value() == false) {
				std::cerr << __FUNCTION__ << " process mesh " << m_curLoadingMeshName << ' ' << i << " failed!\n";
				assert(false);
				continue;
			}
			output.push_back(std::move(newMesh.value()));
		}
		for (size_t i = 0; i < node->mNumChildren; ++i) {
			auto&& meshes = processNode(node->mChildren[i], scene);
			output.insert(output.end(), meshes.begin(), meshes.end());
		}
		return output;
	}

	std::optional<MeshMetaData> MeshLoader::processMesh(aiMesh* mesh, const aiScene* scene) {
		MeshMetaData submesh;
		{
			/** 处理位置信息，这是唯一必须要有的信息 */
			auto& positions = submesh.m_positions;
			if (mesh->HasPositions() == false) {
				std::cerr << __FUNCTION__ << " a submesh of " << m_curLoadingMeshName << " has no position data!\n";
				assert(false);
				return std::nullopt;
			}
			positions.resize(mesh->mNumVertices * 3);
			for (size_t i = 0; i < mesh->mNumVertices; ++i) {
				positions[i] = FromVec3DToFloat3(mesh->mVertices[i]);
			}
			/** 处理包围盒 */
			submesh.m_localBoundingBoxMin = FromVec3DToFloat3(mesh->mAABB.mMin);
			submesh.m_localBoundingBoxMax = FromVec3DToFloat3(mesh->mAABB.mMax);
		}
		{
			/** 处理法线信息，可以缺省 */
			if (mesh->HasNormals() == false) {
				std::cerr << "Warnning: a submesh of " << m_curLoadingMeshName << " has no normal data!\n";
			}
			else {
				auto& normals = submesh.m_normals;
				normals.resize(mesh->mNumVertices);
				for (size_t i = 0; i < mesh->mNumVertices; ++i) {
					normals[i] = FromVec3DToFloat3(mesh->mNormals[i]);
				}
			}
		}
		{
			/** 处理切线信息，可以省略 */
			if (mesh->HasTangentsAndBitangents() == false) {
				std::cerr << "Warnning: a submesh of " << m_curLoadingMeshName << " has no tangent data!\n";
			}
			else {
				auto& tangents = submesh.m_tangents;
				tangents.resize(mesh->mNumVertices);
				for (size_t i = 0; i < mesh->mNumVertices; ++i) {
					tangents[i] = FromVec3DToFloat3(mesh->mTangents[i]);
				}
				auto& bitangents = submesh.m_bitangents;
				bitangents.resize(mesh->mNumVertices);
				for (size_t i = 0; i < mesh->mNumVertices; ++i) {
					bitangents[i] = FromVec3DToFloat3(mesh->mBitangents[i]);
				}
			}
		}
		{
			/** 处理纹理坐标数据，可以省略 */
			size_t librarySupportedTexcoordSets = AI_MAX_NUMBER_OF_TEXTURECOORDS;
			size_t setIndex = 0;
			for (setIndex = 0; setIndex < std::min(NUMBER_OF_TEXCOORD_SET, librarySupportedTexcoordSets); ++setIndex) {
				if (mesh->HasTextureCoords((unsigned int)setIndex) == false) {
					break;
				}
				else {
					auto& texcoords = submesh.m_texcoords[setIndex];
					for (size_t i = 0; i < mesh->mNumVertices; ++i) {
						/** TODO: 仅支持2维纹理坐标 */
						texcoords[i] = FromVec3DToFloat2(mesh->mTextureCoords[setIndex][i]);
					}
				}
			}
			std::cout << "Submesh of " << m_curLoadingMeshName << " has " << setIndex << " set(s) of texcoords\n";
		}
		{
			/** 处理索引数据，可以省略 */
			if (mesh->HasFaces() == false) {
				std::cout << "Submesh of " << m_curLoadingMeshName << " doesn't use indices\n";
			}
			else {
				auto& indices = submesh.m_indices;
				for (size_t i = 0; i < mesh->mNumFaces; ++i) {
					for (size_t j = 0; j < mesh->mFaces[i].mNumIndices; ++j) {
						indices.push_back(mesh->mFaces[i].mIndices[j]);
					}
				}
			}
		}
		return submesh;
	}


}