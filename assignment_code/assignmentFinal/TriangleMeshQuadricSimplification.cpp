//
// Created by lisa on 12/7/21.
//
#include <iostream>
#include <fstream>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <glm/gtx/hash.hpp>

#include "gloo/VertexObject.hpp"
#include "Quadric.h"
#include "assert.h"


namespace GLOO {
glm::vec4 VertexObject::GetTrianglePlane(size_t triangle_idx) const {
	const glm::vec3 &triangle = triangles_[triangle_idx];
	const glm::vec3 &vertex0 = GetPositions()[triangle[0]];
	const glm::vec3 &vertex1 = GetPositions()[triangle[1]];
	const glm::vec3 &vertex2 = GetPositions()[triangle[2]];
	return ComputeTrianglePlane(vertex0, vertex1, vertex2);
}

double VertexObject::GetTriangleArea(size_t triangle_idx) const {
	const glm::vec3 &triangle = triangles_[triangle_idx];
	const glm::vec3 &p0 = GetPositions()[triangle[0]];
	const glm::vec3 &p1 = GetPositions()[triangle[1]];
	const glm::vec3 &p2 = GetPositions()[triangle[2]];

	const glm::vec3 x = p0 - p1;
	const glm::vec3 y = p0 - p2;
	double area = 0.5 * glm::length(glm::cross(x, y));

	return area;
}

std::unordered_map<glm::vec2,
		std::vector<int>>
VertexObject::GetEdgeToTrianglesMap() const {
	std::unordered_map<glm::vec2, std::vector<int>> trias_per_edge;
	auto AddEdge = [&](int vidx0, int vidx1, int tidx) {
		trias_per_edge[GetOrderedEdge(vidx0, vidx1)].push_back(tidx);
	};

	for (size_t tidx = 0; tidx < triangles_.size(); ++tidx) {
		const auto &triangle = triangles_[tidx];
		AddEdge(triangle[0], triangle[1], int(tidx));
		AddEdge(triangle[1], triangle[2], int(tidx));
		AddEdge(triangle[2], triangle[0], int(tidx));
	}

	return trias_per_edge;
}

glm::vec4 VertexObject::ComputeTrianglePlane(const glm::vec3 &p0,
																									 const glm::vec3 &p1,
																									 const glm::vec3 &p2) {
	const glm::vec3 e0 = p1 - p0;
	const glm::vec3 e1 = p2 - p0;
	glm::vec3 abc = glm::cross(e0,e1);
	double norm = glm::length(abc);
	// if the three points are co-linear, return invalid plane
	if (norm == 0) {
		return glm::vec4(0, 0, 0, 0);
	}
	abc /= glm::length(abc);
	double d = glm::dot(-abc, p0);
	return glm::vec4(abc[0], abc[1], abc[2], d);
}

std::shared_ptr<VertexObject> VertexObject::SimplifyQuadricDecimation(
		int target_number_of_triangles,
		double maximum_error = std::numeric_limits<double>::infinity(),
		double boundary_weight = 1.0) const {

	typedef std::tuple<double, int, int> CostEdge;

	// Create new VertexObject to hold new mesh for modification
	auto mesh = std::make_shared<VertexObject>();
	mesh->UpdatePositions(std::move(make_unique<PositionArray>(GetPositions())));
	mesh->UpdateNormals(std::move(make_unique<NormalArray>(GetNormals())));
	mesh->UpdateIndices(std::move(make_unique<IndexArray>(GetIndices())));
	mesh->triangles_ = triangles_;

	const PositionArray& vertices_ = GetPositions();

	std::vector<bool> vertices_deleted(vertices_.size(), false);
	std::vector<bool> triangles_deleted(triangles_.size(), false);

	// Map vertices to triangles and compute triangle planes and areas
	std::vector<std::unordered_set<int>> vert_to_triangles(vertices_.size());
	std::vector<glm::vec4> triangle_planes(triangles_.size());
	std::vector<double> triangle_areas(triangles_.size());
	for (size_t tidx = 0; tidx < triangles_.size(); ++tidx) {
		vert_to_triangles[triangles_[tidx][0]].emplace(static_cast<int>(tidx));
		vert_to_triangles[triangles_[tidx][1]].emplace(static_cast<int>(tidx));
		vert_to_triangles[triangles_[tidx][2]].emplace(static_cast<int>(tidx));

		triangle_planes[tidx] = GetTrianglePlane(tidx);
		triangle_areas[tidx] = GetTriangleArea(tidx);

	}

	// Compute the error metric per vertex
	std::vector<Quadric> Qs(vertices_.size());
	for (size_t vidx = 0; vidx < vertices_.size(); ++vidx) {
		for (int tidx : vert_to_triangles[vidx]) {
			Qs[vidx] += Quadric(triangle_planes[tidx], triangle_areas[tidx]);
		}
	}

	// For boundary edges add perpendicular plane quadric
	auto edge_triangle_count = GetEdgeToTrianglesMap();
	auto AddPerpPlaneQuadric = [&](int vidx0, int vidx1, int vidx2,
																 double area) {
		int min = std::min(vidx0, vidx1);
		int max = std::max(vidx0, vidx1);
		glm::vec2 edge(min, max);
		if (edge_triangle_count[edge].size() != 1) {
			return;
		}
		const auto& vert0 = mesh->GetPositions()[vidx0];
		const auto& vert1 = mesh->GetPositions()[vidx1];
		const auto& vert2 = mesh->GetPositions()[vidx2];
		glm::vec3 vert2p = glm::cross((vert2 - vert0),(vert2 - vert1));
		/// test equality
		Eigen::Vector3d vert0_eig;
		vert0_eig << vert0[0], vert0[1], vert0[2];
		Eigen::Vector3d vert1_eig;
		vert1_eig << vert1[0], vert1[1], vert1[2];

		Eigen::Vector3d vert2_eig;
		vert2_eig << vert2[0], vert2[1], vert2[2];

		Eigen::Vector3d vert2p_eig = (vert2_eig - vert0_eig).cross(vert2_eig - vert1_eig);

		assert (fabs(vert2p_eig(0)-vert2p[0]) < 1e-4);
		assert (fabs(vert2p_eig(1)-vert2p[1]) < 1e-4);
		assert (fabs(vert2p_eig(2)-vert2p[2]) < 1e-4);

		///
		glm::vec4 plane = ComputeTrianglePlane(vert0, vert1, vert2p);
		Quadric quad(plane, area * boundary_weight);
		Qs[vidx0] += quad;
		Qs[vidx1] += quad;
	};
	for (size_t tidx = 0; tidx < triangles_.size(); ++tidx) {
		const auto& tria = triangles_[tidx];
		double area = triangle_areas[tidx];
		AddPerpPlaneQuadric(tria[0], tria[1], tria[2], area);
		AddPerpPlaneQuadric(tria[1], tria[2], tria[0], area);
		AddPerpPlaneQuadric(tria[2], tria[0], tria[1], area);
	}

	// Get valid edges and compute cost
	// Note: We could also select all vertex pairs as edges with dist < eps
	std::unordered_map<glm::vec2, glm::vec3> vbars;
	std::unordered_map<glm::vec2, double> costs;
	auto CostEdgeComp = [](const CostEdge& a, const CostEdge& b) {
		return std::get<0>(a) > std::get<0>(b);
	};
	std::priority_queue<CostEdge, std::vector<CostEdge>, decltype(CostEdgeComp)>
			queue(CostEdgeComp);

	auto AddEdge = [&](int vidx0, int vidx1, bool update) {
		int min = std::min(vidx0, vidx1);
		int max = std::max(vidx0, vidx1);
		glm::vec2 edge(min, max);
		if (update || vbars.count(edge) == 0) {
			const Quadric& Q0 = Qs[min];
			const Quadric& Q1 = Qs[max];
			Quadric Qbar = Q0 + Q1;
			double cost;
			glm::vec3 vbar;
			if (Qbar.IsInvertible()) {
				vbar = Qbar.Minimum();
				cost = Qbar.Eval(vbar);
			} else {
				const glm::vec3& v0 = mesh->GetPositions()[vidx0];
				const glm::vec3& v1 = mesh->GetPositions()[vidx1];
				glm::vec3 vmid = (v0 + v1) / 2.f;
				double cost0 = Qbar.Eval(v0);
				double cost1 = Qbar.Eval(v1);
				double costmid = Qbar.Eval(vmid);
				cost = std::min(cost0, std::min(cost1, costmid));
				if (cost == costmid) {
					vbar = vmid;
				} else if (cost == cost0) {
					vbar = v0;
				} else {
					vbar = v1;
				}
			}
			vbars[edge] = vbar;
			costs[edge] = cost;
			queue.push(CostEdge(cost, min, max));
		}
	};

	// add all edges to priority queue
	for (const auto& triangle : triangles_) {
		AddEdge(triangle[0], triangle[1], false);
		AddEdge(triangle[1], triangle[2], false);
		AddEdge(triangle[2], triangle[0], false);
	}

	// perform incremental edge collapse
	bool has_vert_normal = HasNormals();
	int n_triangles = int(triangles_.size());
	while (n_triangles > target_number_of_triangles && !queue.empty()) {
		// retrieve edge from queue
		double cost;
		int vidx0, vidx1;
		std::tie(cost, vidx0, vidx1) = queue.top();
		queue.pop();

		if (cost > maximum_error) {
			break;
		}

		// test if the edge has been updated (reinserted into queue)
		glm::vec2 edge(vidx0, vidx1);
		bool valid = !vertices_deleted[vidx0] && !vertices_deleted[vidx1] &&
								 cost == costs[edge];
		if (!valid) {
			continue;
		}

		// avoid flip of triangle normal
		bool flipped = false;
		for (int tidx : vert_to_triangles[vidx1]) {
			if (triangles_deleted[tidx]) {
				continue;
			}

			const glm::vec3& tria = mesh->triangles_[tidx];
			bool has_vidx0 =
					vidx0 == tria[0] || vidx0 == tria[1] || vidx0 == tria[2];
			bool has_vidx1 =
					vidx1 == tria[0] || vidx1 == tria[1] || vidx1 == tria[2];
			if (has_vidx0 && has_vidx1) {
				continue;
			}

			glm::vec3 vert0 = mesh->GetPositions()[tria[0]];
			glm::vec3 vert1 = mesh->GetPositions()[tria[1]];
			glm::vec3 vert2 = mesh->GetPositions()[tria[2]];
			glm::vec3 norm_before = glm::cross((vert1 - vert0),(vert2 - vert0));
			norm_before /= glm::length(norm_before);

			/// test equality
			Eigen::Vector3d vert0_eig;
			vert0_eig << vert0[0], vert0[1], vert0[2];
			Eigen::Vector3d vert1_eig;
			vert1_eig << vert1[0], vert1[1], vert1[2];
			Eigen::Vector3d vert2_eig;
			vert2_eig << vert2[0], vert2[1], vert2[2];
			Eigen::Vector3d norm_before_eig = (vert1_eig - vert0_eig).cross(vert2_eig - vert0_eig);
			norm_before_eig /= norm_before_eig.norm();

			assert (fabs(norm_before_eig(0)-norm_before[0]) < 1e-4);
			assert (fabs(norm_before_eig(1)-norm_before[1]) < 1e-4);
			assert (fabs(norm_before_eig(2)-norm_before[2]) < 1e-4);

			///


			if (vidx1 == tria[0]) {
				vert0 = vbars[edge];
			} else if (vidx1 == tria[1]) {
				vert1 = vbars[edge];
			} else if (vidx1 == tria[2]) {
				vert2 = vbars[edge];
			}

			glm::vec3 norm_after = glm::cross((vert1 - vert0),(vert2 - vert0));
			norm_after /= glm::length(norm_after);
			if (glm::dot(norm_before,norm_after) < 0) {
				flipped = true;
				break;
			}
		}
		if (flipped) {
			continue;
		}

		// Connect triangles from vidx1 to vidx0, or mark deleted
		for (int tidx : vert_to_triangles[vidx1]) {
			if (triangles_deleted[tidx]) {
				continue;
			}

			glm::vec3& tria = mesh->triangles_[tidx];
			bool has_vidx0 =
					vidx0 == tria[0] || vidx0 == tria[1] || vidx0 == tria[2];
			bool has_vidx1 =
					vidx1 == tria[0] || vidx1 == tria[1] || vidx1 == tria[2];

			if (has_vidx0 && has_vidx1) {
				triangles_deleted[tidx] = true;
				n_triangles--;
				continue;
			}

			if (vidx1 == tria[0]) {
				tria[0] = vidx0;
			} else if (vidx1 == tria[1]) {
				tria[1] = vidx0;
			} else if (vidx1 == tria[2]) {
				tria[2] = vidx0;
			}
			vert_to_triangles[vidx0].insert(tidx);
		}

		// update vertex vidx0 to vbar
		PositionArray new_positions = mesh->GetPositions();
		new_positions[vidx0] = vbars[edge];
		auto new_positions_ptr = make_unique<PositionArray>(new_positions);
		mesh->UpdatePositions(std::move(new_positions_ptr));
//		mesh->vertices_[vidx0] = vbars[edge];

		Qs[vidx0] += Qs[vidx1];
		NormalArray new_normals = mesh->GetNormals();
		if (has_vert_normal) {
//			mesh->vertex_normals_[vidx0] = 0.5 * (mesh->vertex_normals_[vidx0] +
//																						mesh->vertex_normals_[vidx1]);
			new_normals[vidx0] = 0.5f * (mesh->GetNormals()[vidx0] +
																						mesh->GetNormals()[vidx1]);
		}
		auto new_normals_ptr = make_unique<NormalArray>(new_normals);
		mesh->UpdateNormals(std::move(new_normals_ptr));
//		if (has_vert_color) {
//			mesh->vertex_colors_[vidx0] = 0.5 * (mesh->vertex_colors_[vidx0] +
//																					 mesh->vertex_colors_[vidx1]);
//		}
		vertices_deleted[vidx1] = true;

		// Update edge costs for all triangles connecting to vidx0
		for (const auto& tidx : vert_to_triangles[vidx0]) {
			if (triangles_deleted[tidx]) {
				continue;
			}
			const glm::vec3& tria = mesh->triangles_[tidx];
			if (tria[0] == vidx0 || tria[1] == vidx0) {
				AddEdge(tria[0], tria[1], true);
			}
			if (tria[1] == vidx0 || tria[2] == vidx0) {
				AddEdge(tria[1], tria[2], true);
			}
			if (tria[2] == vidx0 || tria[0] == vidx0) {
				AddEdge(tria[2], tria[0], true);
			}
		}
	}

	PositionArray new_positions = mesh->GetPositions(); //added
	NormalArray new_normals = mesh->GetNormals();
	IndexArray new_indices;

	// Apply changes to the triangle mesh
	int next_free = 0;
	std::unordered_map<int, int> vert_remapping;
	for (size_t idx = 0; idx < mesh->GetPositions().size(); ++idx) {
		if (!vertices_deleted[idx]) {
			vert_remapping[int(idx)] = next_free;
			new_positions[next_free] = mesh->GetPositions()[idx];
			if (has_vert_normal) {
				new_normals[next_free] = mesh->GetNormals()[idx];
			}
//			if (has_vert_color) {
//				mesh->vertex_colors_[next_free] = mesh->vertex_colors_[idx];
//			}
			next_free++;
		}
	}
	new_positions.resize(next_free);
	if (has_vert_normal) {
		new_normals.resize(next_free);
	}
//	if (has_vert_color) {
//		mesh->vertex_colors_.resize(next_free);
//	}
	//added
	auto new_positions_ptr = make_unique<PositionArray>(new_positions);
	mesh->UpdatePositions(std::move(new_positions_ptr));
	auto new_normals_ptr = make_unique<NormalArray>(new_normals);
	mesh->UpdateNormals(std::move(new_normals_ptr));

	next_free = 0;
	for (size_t idx = 0; idx < mesh->triangles_.size(); ++idx) {
		if (!triangles_deleted[idx]) {
			glm::vec3 tria = mesh->triangles_[idx];
			mesh->triangles_[next_free][0] = vert_remapping[tria[0]];
			mesh->triangles_[next_free][1] = vert_remapping[tria[1]];
			mesh->triangles_[next_free][2] = vert_remapping[tria[2]];
			next_free++;
		}
	}
	mesh->triangles_.resize(next_free);

	// update indices
	for (size_t idx = 0; idx < mesh->triangles_.size(); ++idx) {
		new_indices.push_back(mesh->triangles_[idx][0]);
		new_indices.push_back(mesh->triangles_[idx][1]);
		new_indices.push_back(mesh->triangles_[idx][2]);
		assert (new_indices.size() == triangles_.size() * 3);
	}

	auto new_indices_ptr = make_unique<IndexArray>(new_indices);
	mesh->UpdateIndices(std::move(new_indices_ptr));

//	if (HasTriangleNormals()) {
//		mesh->ComputeTriangleNormals();
//	}
	//remember to compute normals again after calling this function

	return mesh;
}

}