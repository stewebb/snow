#include <Eigen/Eigen>

//Eigen::Matrix4f get_rotation(float rotation_angle, const Eigen::Vector3f &axis);
//Eigen::Matrix4f get_translation(const Eigen::Vector3f &translation);
//Eigen::Matrix4f look_at(Eigen::Vector3f eye_pos, Eigen::Vector3f target, Eigen::Vector3f up);
Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos);
Eigen::Matrix4f get_model_matrix(float rotation_angle, const Eigen::Vector3f &axis, const Eigen::Vector3f &translation);
Eigen::Matrix4f get_projection_matrix(float eye_fovy, float aspect_ratio, float zNear, float zFar);