#include "SkeletonNode.hpp"

#include <iostream>
#include <fstream>
#include <map>

#include "gloo/utils.hpp"
#include "gloo/InputManager.hpp"
#include "gloo/MeshLoader.hpp"
#include "gloo/shaders/PhongShader.hpp"
#include "gloo/shaders/SimpleShader.hpp"
#include "gloo/VertexObject.hpp"
#include "Quadric.h"

#include "gloo/debug/PrimitiveFactory.hpp"

#include "glm/gtx/string_cast.hpp"



namespace GLOO {
SkeletonNode::SkeletonNode(const std::string& filename)
    : SceneNode(){
	shader_ = std::make_shared<PhongShader>();
	material_ = std::make_shared<Material>(Material::GetDefault());
	Material material_line = Material::GetDefault();
	material_line.SetAmbientColor(glm::vec3(0.f, 1.f, 0.f));
	material_line.SetDiffuseColor(glm::vec3(0.f, 1.f, 0.f));

	material_lines_ = std::make_shared<Material>(material_line);
	shader_lines_ = std::make_shared<SimpleShader>();


	LoadAllFiles(filename);
	ComputeNormals();
	InitializeTriangles();
	DrawMeshLines();

//	auto new_mesh_ptr = mesh_node_ptr->GetComponentPtr<RenderingComponent>()->GetVertexObjectPtr()->SimplifyQuadricDecimation(900, 1000, 1);
//	mesh_node_ptr->GetComponentPtr<RenderingComponent>()->SetVertexObject(new_mesh_ptr);
//	ComputeNormals();

}



void SkeletonNode::Update(double delta_time) {
  // Prevent multiple toggle.
  static bool prev_released = true;
  if (InputManager::GetInstance().IsKeyPressed('S')) {
    if (prev_released) {
			auto new_mesh_ptr = mesh_node_ptr->GetComponentPtr<RenderingComponent>()->GetVertexObjectPtr()->SimplifyQuadricDecimation(num_triangles_, 10, 1);
			num_triangles_ = int(num_triangles_*.75);
			mesh_node_ptr->GetComponentPtr<RenderingComponent>()->SetVertexObject(new_mesh_ptr);
			ComputeNormals();
    }
    prev_released = false;
  } else if (InputManager::GetInstance().IsKeyReleased('S')) {
    prev_released = true;
  }
}

void SkeletonNode::DrawMeshLines() {
	VertexObject* mesh_component_ptr = mesh_node_ptr->GetComponentPtr<RenderingComponent>()->GetVertexObjectPtr();

	PositionArray positions = mesh_component_ptr->GetPositions();
	for (size_t pos_idx; pos_idx < positions.size(); pos_idx++) {
		auto mesh_line_node = make_unique<SceneNode>();
		auto line_seg = PrimitiveFactory::CreateLineSegment(positions[0], positions[1]);
		mesh_line_node->CreateComponent<RenderingComponent>(std::move(line_seg));
		mesh_line_node->CreateComponent<ShadingComponent>(shader_lines_);
		mesh_line_node->CreateComponent<MaterialComponent>(material_lines_);
		AddChild(std::move(mesh_line_node));

		auto mesh_line_node1 = make_unique<SceneNode>();
		auto line_seg1 = PrimitiveFactory::CreateLineSegment(positions[1], positions[2]);
		mesh_line_node1->CreateComponent<RenderingComponent>(std::move(line_seg1));
		mesh_line_node1->CreateComponent<ShadingComponent>(shader_lines_);
		mesh_line_node1->CreateComponent<MaterialComponent>(material_lines_);
		AddChild(std::move(mesh_line_node1));

		auto mesh_line_node2 = make_unique<SceneNode>();
		auto line_seg2 = PrimitiveFactory::CreateLineSegment(positions[0], positions[2]);
		mesh_line_node2->CreateComponent<RenderingComponent>(std::move(line_seg2));
		mesh_line_node2->CreateComponent<ShadingComponent>(shader_lines_);
		mesh_line_node2->CreateComponent<MaterialComponent>(material_lines_);
		AddChild(std::move(mesh_line_node2));
	}

}

void SkeletonNode::ComputeNormals() {
	VertexObject* mesh_component_ptr = mesh_node_ptr->GetComponentPtr<RenderingComponent>()->GetVertexObjectPtr();
	std::cout << "TOTAL NUMBER OF TRIANGLES: " << std::to_string(mesh_component_ptr->triangles_.size()) << std::endl;

	PositionArray positions = mesh_component_ptr->GetPositions();
	IndexArray indices = mesh_component_ptr->GetIndices();
	std::map<int, std::vector<int>> vertex_to_incident_face_idx; // map vertex index to list of incident face indices

	auto face_normals = make_unique<NormalArray>();
	std::vector<float> face_areas;
	auto vertex_normals = make_unique<NormalArray>();
	//compute face normals
	for (size_t i = 0; i < indices.size(); i+=3) {
		// cross product of triangle edges
		glm::vec3 edge1 = positions[indices[i+1]] - positions[indices[i]];
		glm::vec3 edge2 = positions[indices[i+2]] - positions[indices[i]];
		face_normals->push_back(glm::normalize(glm::cross(edge1, edge2)));
		face_areas.push_back(glm::length(glm::cross(edge1,edge2))/2.); // cross product over 2
		// we're iterating through indices that define a face, so we should add the face to
		// all vertices
		vertex_to_incident_face_idx[indices[i]].push_back(i/3.);
		vertex_to_incident_face_idx[indices[i+1]].push_back(i/3.);
		vertex_to_incident_face_idx[indices[i+2]].push_back(i/3.);
	}
	for (size_t i = 0; i < positions.size(); i++) {
		glm::vec3 weighted_avg = glm::vec3(0,0,0);
		std::vector<int> incident_face_idx = vertex_to_incident_face_idx[i];

		for (size_t j = 0; j < incident_face_idx.size(); j++) {
			weighted_avg = weighted_avg + face_areas[incident_face_idx[j]] * face_normals->operator[](incident_face_idx[j]);
		}
		weighted_avg = glm::normalize(weighted_avg);
		vertex_normals->push_back(weighted_avg);
	}
	mesh_component_ptr->UpdateNormals(std::move(vertex_normals));
}

void SkeletonNode::InitializeTriangles() {
	VertexObject* vtx_obj_ptr = mesh_node_ptr->GetComponentPtr<RenderingComponent>()->GetVertexObjectPtr();
	IndexArray mesh_indices = vtx_obj_ptr->GetIndices();
	for (size_t i = 0; i < mesh_indices.size(); i++) {
		if (i % 3 == 0) {
			vtx_obj_ptr->triangles_.push_back(glm::vec3(mesh_indices[i], mesh_indices[i+1], mesh_indices[i+2]));
		}
	}
	num_triangles_ = vtx_obj_ptr->triangles_.size();
	std::cout << "TOTAL NUMBER OF INITIAL TRIANGLES: " << std::to_string(vtx_obj_ptr->triangles_.size()) << std::endl;
}

void SkeletonNode::LoadMeshFile(const std::string& filename) {
  std::shared_ptr<VertexObject> vtx_obj =
      MeshLoader::Import(filename).vertex_obj;
  mesh_ = vtx_obj;
	auto mesh_node = make_unique<SceneNode>();
	mesh_node->CreateComponent<RenderingComponent>(mesh_);
	mesh_node->CreateComponent<ShadingComponent>(shader_);
	mesh_node->CreateComponent<MaterialComponent>(material_);
	mesh_node_ptr = mesh_node.get();
	AddChild(std::move(mesh_node));
	MeshLoader::Export(filename, vtx_obj);

	//o3d.io.read_triangle_mesh(filename)
}


void SkeletonNode::LoadAllFiles(const std::string& prefix) {
  std::string prefix_full = GetAssetDir() + prefix;
  LoadMeshFile(prefix + ".obj");
}
}  // namespace GLOO
