#include "MeshLoader.h"
#include <iostream>
int main() {
	auto&& pMeshes = Mesh_Loader::gML.Load("../Meshes/bunny_norm.obj");
	std::cout << pMeshes.size() << "\n";
	std::cin.get();
	return 0;
}
