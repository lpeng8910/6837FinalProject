#include "MeshLoader.hpp"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

#include "gloo/utils.hpp"

namespace GLOO {
MeshData MeshLoader::Import(const std::string& filename) {
  std::string file_path = GetAssetDir() + filename;
  bool success;
  auto parsed_data = ObjParser::Parse(file_path, success);
  if (!success) {
    std::cerr << "Load mesh file " << filename << " failed!" << std::endl;
    return {};
  }
  // Remove empty groups.
  parsed_data.groups.erase(
      std::remove_if(parsed_data.groups.begin(), parsed_data.groups.end(),
                     [](MeshGroup& g) { return g.num_indices == 0; }),
      parsed_data.groups.end());

  MeshData mesh_data;
  mesh_data.vertex_obj = make_unique<VertexObject>();
  if (parsed_data.positions) {
    mesh_data.vertex_obj->UpdatePositions(std::move(parsed_data.positions));
  }
  if (parsed_data.normals) {
    mesh_data.vertex_obj->UpdateNormals(std::move(parsed_data.normals));
  }
  if (parsed_data.tex_coords) {
    mesh_data.vertex_obj->UpdateTexCoord(std::move(parsed_data.tex_coords));
  }
  if (parsed_data.indices) {
    mesh_data.vertex_obj->UpdateIndices(std::move(parsed_data.indices));
  }

  mesh_data.groups = std::move(parsed_data.groups);

  return mesh_data;
}

bool MeshLoader::Export(const std::string& filename, std::shared_ptr<VertexObject> vertex_obj) {
  std::string output_filename = std::string(filename);
	output_filename.replace(output_filename.end()-4, output_filename.end(), "_processed.obj");
	std::string file_path = GetAssetDir() + output_filename;
	std::cout << file_path << std::endl;
	bool success;
	PositionArray positions = vertex_obj->GetPositions();
	IndexArray indices = vertex_obj->GetIndices();
	std::fstream file;
	file.open(file_path, std::ios::out);
	if (!file) {
		std::cout << "Error in creating file!";
		return 0;
	}
	std::cout << "File created successfully.";
	//TODO: add how many vertices and faces
	file << "# " << positions.size() << " " << indices.size()/3 << '\n' << '\n';
	for (int v = 0; v < positions.size(); v++) {
		file << "v " << positions[v][0] << " " << positions[v][1] << " " << positions[v][2] << '\n';
	}
	for (int i = 0; i < indices.size()-1; i++) {
		if (i%3 == 0) {
			// obj files are 1-indexed
			file << "f " << indices[i]+1 << " " << indices[i+1]+1 << " " << indices[i+2]+1 << '\n';
		}
	}
	file.close();
	return 0;


}
}  // namespace GLOO
