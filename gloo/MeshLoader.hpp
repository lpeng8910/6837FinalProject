#ifndef GLOO_MESH_LOADER_H_
#define GLOO_MESH_LOADER_H_

#include "parsers/ObjParser.hpp"
#include "MeshData.hpp"

namespace GLOO {
class MeshLoader {
 public:
  static MeshData Import(const std::string& filename);
  static bool Export(const std::string& filename, std::shared_ptr<VertexObject> vertex_obj);
};
}  // namespace GLOO

#endif
