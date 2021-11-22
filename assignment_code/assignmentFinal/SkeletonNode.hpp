#ifndef SKELETON_NODE_H_
#define SKELETON_NODE_H_

#include "gloo/SceneNode.hpp"
#include "gloo/VertexObject.hpp"
#include "gloo/shaders/ShaderProgram.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "gloo/components/RenderingComponent.hpp"
#include "gloo/components/ShadingComponent.hpp"
#include "gloo/components/MaterialComponent.hpp"


#include <string>
#include <vector>
#include <map>

namespace GLOO {
class SkeletonNode : public SceneNode {
 public:
//  enum class DrawMode { Skeleton, SSD };
//  struct EulerAngle {
//    float rx, ry, rz;
//  };

	std::shared_ptr<PhongShader> shader_;
//	std::shared_ptr<VertexObject> sphere_mesh_;
//	std::shared_ptr<VertexObject> cylinder_mesh_;
//	std::map<int, SceneNode*> idx_to_joint_node_ {{-1, this}};
//	std::map<int, SceneNode*> idx_to_bone_component;
	std::shared_ptr<Material> material_;
	std::shared_ptr<VertexObject> mesh_;
//	std::vector<std::vector<float>> weights_matrix_;
//	std::vector<glm::mat4> Bs_inv;
//	std::vector<glm::mat4> Ts;
//	std::vector<glm::vec3> vertex_bind_poses;
	SceneNode* mesh_node_ptr;



	SkeletonNode(const std::string& filename);
  void Update(double delta_time) override;
	void ComputeNormals();



	private:
  void LoadAllFiles(const std::string& prefix);
  void LoadMeshFile(const std::string& filename);

};
}  // namespace GLOO

#endif
