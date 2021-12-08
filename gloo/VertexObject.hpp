#ifndef GLOO_VERTEX_OBJECT_H_
#define GLOO_VERTEX_OBJECT_H_

#include "gloo/gl_wrapper/VertexArray.hpp"
#include <glm/gtx/hash.hpp>
#include <unordered_map>

namespace GLOO {
// Instances of this class store various vertex data and are responsible
// for sending data from CPU to GPU via the Update* methods.
class VertexObject {
 public:
  VertexObject() : vertex_array_(make_unique<VertexArray>()) {
  }

  // Vertex buffers are created in a lazy manner in the following Update*.
  void UpdatePositions(std::unique_ptr<PositionArray> positions);
  void UpdateNormals(std::unique_ptr<NormalArray> normals);
  void UpdateColors(std::unique_ptr<ColorArray> colors);
  void UpdateTexCoord(std::unique_ptr<TexCoordArray> tex_coords);
  void UpdateIndices(std::unique_ptr<IndexArray> indices);

  bool HasPositions() const {
    return positions_ != nullptr;
  }

  bool HasNormals() const {
    return normals_ != nullptr;
  }

  bool HasColors() const {
    return colors_ != nullptr;
  }

  bool HasTexCoors() const {
    return tex_coords_ != nullptr;
  }

  bool HasIndices() const {
    return indices_ != nullptr;
  }

  const PositionArray& GetPositions() const {
    if (positions_ == nullptr)
      throw std::runtime_error("No position in VertexObject!");
    return *positions_;
  }

  const NormalArray& GetNormals() const {
    if (normals_ == nullptr)
      throw std::runtime_error("No normal in VertexObject!");
    return *normals_;
  }

  const ColorArray& GetColors() const {
    if (colors_ == nullptr)
      throw std::runtime_error("No color in VertexObject!");
    return *colors_;
  }

  const TexCoordArray& GetTexCoords() const {
    if (tex_coords_ == nullptr)
      throw std::runtime_error("No texture coordinate in VertexObject!");
    return *tex_coords_;
  }

  const IndexArray& GetIndices() const {
    if (indices_ == nullptr)
      throw std::runtime_error("No indices in VertexObject!");
    return *indices_;
  }

  VertexArray& GetVertexArray() {
    return *vertex_array_.get();
  }
  const VertexArray& GetVertexArray() const {
    return *vertex_array_.get();
  }

	/// Function to simplify mesh using Quadric Error Metric Decimation by
	/// Garland and Heckbert.
	/// \param target_number_of_triangles defines the number of triangles that
	/// the simplified mesh should have. It is not guaranteed that this number
	/// will be reached.
	/// \param maximum_error defines the maximum error where a vertex is allowed
	/// to be merged
	/// \param boundary_weight a weight applied to edge vertices used to
	/// preserve boundaries
	std::shared_ptr<VertexObject> SimplifyQuadricDecimation(
			int target_number_of_triangles,
			double maximum_error,
			double boundary_weight) const;

	/// Function that computes the plane equation from the three points.
	/// If the three points are co-linear, then this function returns the
	/// invalid plane (0, 0, 0, 0).
	static glm::vec4 ComputeTrianglePlane(const glm::vec3 &p0,
																							const glm::vec3 &p1,
																							const glm::vec3 &p2);

	/// Function that computes the plane equation of a mesh triangle identified
	/// by the triangle index.
	glm::vec4 GetTrianglePlane(size_t triangle_idx) const;

	/// Function that computes the area of a mesh triangle identified by the
	/// triangle index
	double GetTriangleArea(size_t triangle_idx) const;

	/// Function that returns a map from edges (vertex0, vertex1) to the
	/// triangle indices the given edge belongs to.
	std::unordered_map<glm::vec2,
	std::vector<int>> GetEdgeToTrianglesMap() const;

	/// Helper function to get an edge with ordered vertex indices.
	static inline glm::vec2 GetOrderedEdge(int vidx0, int vidx1) {
		return glm::vec2(std::min(vidx0, vidx1), std::max(vidx0, vidx1));
	}
	/// index of the three vertices that make up the triangle
	std::vector<glm::vec3> triangles_; //this should also be private but this is kind of a hack

 private:
  std::unique_ptr<VertexArray> vertex_array_;

  // Owner of vertex data.
  std::unique_ptr<PositionArray> positions_;
  std::unique_ptr<NormalArray> normals_;
  std::unique_ptr<ColorArray> colors_;
  std::unique_ptr<TexCoordArray> tex_coords_;
  std::unique_ptr<IndexArray> indices_;
};

}  // namespace GLOO

#endif
