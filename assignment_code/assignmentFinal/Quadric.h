//
// Created by lisa on 12/7/21.
//

#ifndef INC_6_837_QUADRIC_H
#define INC_6_837_QUADRIC_H

#include <eigen3/Eigen/Dense>
#include <glm/glm.hpp>

#include <queue>
#include <tuple>

#include <string>
#include <vector>
#include <map>

#include <assert.h>
#include <iostream>

namespace GLOO {

class Quadric {
public:
	Quadric() {
		A_ = glm::mat3(0.0);
		b_ = glm::vec4(0.0);
		c_ = 0;
	}

	Quadric(const glm::vec4 &plane, double weight = 1) {

		Eigen::Vector3d n_eig;
		n_eig << plane[0], plane[1], plane[2];
		A_eig = weight * n_eig * n_eig.transpose();
		b_eig = weight * plane[3] * n_eig;

		glm::vec3 n = glm::vec3(plane);
		A_ = float(weight) * glm::outerProduct(n, n);
		b_ = float(weight) * plane[3] * n;
		c_ = float(weight) * plane[3] * plane[3];
	}

	Quadric &operator+=(const Quadric &other) {
		A_eig += other.A_eig;
		b_eig += other.b_eig;

		A_ += other.A_;
		b_ += other.b_;
		c_ += other.c_;
		return *this;
	}

	Quadric operator+(const Quadric &other) const {
		Quadric res;
		res.A_eig = A_eig + other.A_eig;
		res.b_eig = b_eig + other.b_eig;

		res.A_ = A_ + other.A_;
		res.b_ = b_ + other.b_;
		res.c_ = c_ + other.c_;
		return res;
	}

	double Eval(const glm::vec3 &v) const {
		Eigen::Vector3d v_eig;
		v_eig << v[0], v[1], v[2];
		Eigen::Vector3d Av_eig = A_eig * v_eig;
		double q_eig = v_eig.dot(Av_eig) + 2 * b_eig.dot(v_eig) + c_;

		glm::vec3 Av = A_ * v;
		double q = glm::dot(v, Av) + 2 * glm::dot(b_, v) + c_;

		assert (fabs(q-q_eig) < 1e-4);

		return q;
	}

	bool IsInvertible() const { return std::fabs(glm::determinant(A_)) > 1e-4; }

	glm::vec3 Minimum() const {
		Eigen::Vector3d result_eig = -A_eig.ldlt().solve(b_eig);

		glm::vec3 result = glm::inverse(-A_) * b_;
		assert (fabs(result_eig(0) - result[0]) < 1e-4 );
		assert (fabs(result_eig(1) - result[1]) < 1e-4 );
		assert (fabs(result_eig(2) - result[2]) < 1e-4 );

		return result;
	}

	static inline bool EigenEqGlmMat3(Eigen::Matrix3d mat_eigen, glm::mat3 mat_glm) {
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				if ((mat_eigen(i,j) - mat_glm[j][i]) > 1e-3) {
					return false;
				}
			}
		}
	}

public:
	/// A_ = n . n^T, where n is the plane normal (in equation ax + by + cz + d = 0, normal is (a,b,c))
//	Eigen::Matrix3d A_;
	glm::mat3x3 A_;
	/// b_ = d . n, where n is the plane normal and d the non-normal component
	/// of the plane parameters
//	Eigen::Vector3d b_;
	glm::vec3 b_;
	/// c_ = d . d, where d the non-normal component pf the plane parameters
	double c_;

	//EIGEN
	Eigen::Matrix3d A_eig;
	Eigen::Vector3d b_eig;

};
} // namespace GLOO
#endif //INC_6_837_QUADRIC_H
