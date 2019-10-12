#pragma once

#include <vector>
#include <array>
#include <memory>
#include <string>
#include <algorithm>

struct aiNode;
struct aiScene;
struct aiMesh;

namespace Mesh_Loader {
	constexpr size_t NUMBER_OF_POSITION_SET = 2;
	const
	/** 网格数据的原始容器  */
	class MeshMetaData {
	public:
		const float* PositionData(size_t index = 0) const;
		const float* NormalData() const;
		const float* TexcoordData(size_t index = 0) const;
		const float* TangentData() const;
		const float* IndexData() const;
		size_t NumberOfVertex() const;
		size_t NumberOfIndex() const;
	private:
		std::array< std::vector<float> > m_positions;
		std::array< std::vector<float> > m_texcoords;
	};

	class MeshFactory {
	public:
		std::vector<std::shared_ptr<Mesh>>& Load(const char* path);
		/** 创建朝向z轴负方向的平面，中心与坐标原点重合 */
		std::shared_ptr<Mesh> CreatePlane(float width, float height);
		/** 创建立方体
		 * Note: 纹理坐标和切线坐标无效 */
		std::shared_ptr<Mesh> CreateCube(DirectX::XMFLOAT3 min, DirectX::XMFLOAT3 max);

	private:

		void processNode(aiNode*, const aiScene*);
		Mesh* processMesh(aiMesh*, const aiScene*);

	private:
		std::vector<std::shared_ptr<Mesh>>					m_tempMesh;
	};

	extern MeshFactory gMF;

}