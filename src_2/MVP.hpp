#ifndef MVP_H
#define MVP_H

#include <Eigen/Eigen>

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos);
Eigen::Matrix4f get_model_matrix(float rotation_angle, const Eigen::Vector3f &axis, const Eigen::Vector3f &translation);
Eigen::Matrix4f get_projection_matrix(float eye_fovy, float aspect_ratio, float zNear, float zFar);

#endif  // MVP_H