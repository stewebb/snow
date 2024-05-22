#ifndef MODEL_H
#define MODEL_H

#include <Eigen/Eigen>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace Eigen;
class Mesh {
 public:
  std::vector<Vector3f> vertices;
  std::vector<Vector3i> faces;

  Mesh(){};
  Mesh transform(const Eigen::Matrix4f& matrix) const;
  Mesh operator+(const Mesh& other) const;
  bool load_obj(const std::string& filepath);
  bool save_obj(const std::string& filepath) const;
};

#endif  // MODEL_H