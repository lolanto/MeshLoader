#pragma once

#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <assert.h>

struct aiNode;
struct aiScene;
struct aiMesh;

namespace Mesh_Loader {
	constexpr size_t NUMBER_OF_TEXCOORD_SET = 2; /**< 支持最大的纹理信息组数量 */
	struct float3 {
		float x, y, z;
		float& operator[](size_t index) {
			switch (index) {
			case 0: return x;
			case 1: return y;
			case 2: return z;
			default: assert(false); /**< 下标越界 */
			}
		}
		float operator[](size_t index) const {
			switch (index) {
			case 0: return x;
			case 1: return y;
			case 2: return z;
			default: assert(false); /**< 下标越界 */
			}
		}
	};
	struct float2 {
		union {
			struct {
				float x, y;
			};
			struct {
				float u, v;
			};
		};
		float& operator[](size_t index) {
			switch (index) {
			case 0: return x;
			case 1: return y;
			default: assert(false); /**< 下标越界 */
			}
		}
		float operator[](size_t index) const {
			switch (index) {
			case 0: return x;
			case 1: return y;
			default: assert(false); /**< 下标越界 */
			}
		}
	};
	/** 网格数据的原始容器  */
	class MeshMetaData {
	public:
		friend class MeshLoader;
		static size_t SizeOfPosition, SizeOfNormal, SizeOfTexcoord, SizeOfTangent, SizeOfIndex;
	public:
		/** 获得第index组的位置数据缓冲，单个元素的格式为float3，数量应与顶点数量一致 */
		const float3* PositionData() const;
		/** 获得法线缓冲，单个元素格式为float3，数量应与顶点数量一致 */
		const float3* NormalData() const;
		/** 获得第index组纹理坐标信息缓冲，单个元素格式为float3，数量应与顶点数量一致 */
		const float2* TexcoordData(size_t index = 0) const;
		/** 正切数据缓冲，单个元素格式为float3, 数量与顶点数量一致 */
		const float3* TangentData() const;
		/** 索引数据缓冲，单个元素格式为uint32_t，数量与面元数量相关 */
		const uint32_t* IndexData() const;
		size_t NumberOfVertices() const;
		size_t NumberOfIndices() const;
		/** 返回网格的局部包围盒，返回值为float3 min, float3 max */
		std::pair<float3, float3> LocalBoundingBox() const;
		/** 范围网格的包围球，返回值为float3 center, float radius */
		std::pair<float3, float> LocalBoundingSphere() const;
		MeshMetaData(const MeshMetaData&) = default;
		MeshMetaData(MeshMetaData&&);

		MeshMetaData& operator=(MeshMetaData&&);
		MeshMetaData& operator=(const MeshMetaData&);

	private:
		MeshMetaData() = default;
	private:
		std::vector<float3> m_positions; /**< 顶点数据数组 */
		std::array< std::vector<float2>, NUMBER_OF_TEXCOORD_SET > m_texcoords; /**< 纹理数据数组 */
		std::vector<float3> m_normals; /**< 法线数据 */
		std::vector<float3> m_tangents; /**< 切线数据 */
		std::vector<uint32_t> m_indices; /**< 索引数据 */
		std::vector<float3> m_bitangents; /**< 负切线数据 */
		float3 m_localBoundingBoxMin, m_localBoundingBoxMax; /**< 局部空间包围盒的两端 */
	};

	class MeshLoader {
	public:
		void UseRightHandCoordinate(bool option = false) {
			m_useRightHandCoordinate = option;
		}
	public:
		std::vector<const MeshMetaData*> Load(const char* path);
		/** 创建朝向z轴负方向的平面，中心与坐标原点重合 */
		const MeshMetaData* CreatePlane(float width, float height);
		/** 创建立方体
		 * Note: 纹理坐标和切线坐标无效 */
		const MeshMetaData* CreateCube(float3 min, float3 max);

	private:
		std::vector<MeshMetaData> processNode(aiNode*, const aiScene*);
		std::optional<MeshMetaData> processMesh(aiMesh*, const aiScene*);
	private:
		std::mutex m_loaderMutex;
		std::unordered_map<std::string, 
			std::vector<MeshMetaData> > m_loadedMeshes;
		std::string m_curLoadingMeshName;
		bool m_useRightHandCoordinate = false;
	};

	extern MeshLoader gML;

}