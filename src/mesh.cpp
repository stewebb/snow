#include "mesh.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

bool Mesh::load_obj(const std::string &filepath) {
  if (filepath.substr(filepath.size() - 4, 4) != ".obj") {
    std::cerr << "Only obj file is supported." << std::endl;
    return false;
  }

  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << filepath << std::endl;
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;

    if ("v" == prefix) {
      Vector3f vertex;
      iss >> vertex[0] >> vertex[1] >> vertex[2];
      vertices.push_back(vertex);
    } else if ("f" == prefix) {
      Vector3i face;
      iss >> face[0] >> face[1] >> face[2];
      face = face.array() - 1;
      faces.push_back(face);
    }
  }
  return true;
}

bool Mesh::save_obj(const std::string &filepath) const {
  if (filepath.substr(filepath.size() - 4, 4) != ".obj") {
    std::cerr << "Only obj file is supported." << std::endl;
    return false;
  }

  std::ofstream out_file(filepath);
  if (!out_file.is_open()) {
    std::cerr << "Failed to open file: " << filepath << std::endl;
    return false;
  }

  // write vertices
  for (const auto &vertex : this->vertices) {
    out_file << "v " << vertex[0] << " " << vertex[1] << " " << vertex[2] << "\n";
  }

  // write faces
  for (const auto &face : this->faces) {
    out_file << "f " << face[0] + 1 << " " << face[1] + 1 << " " << face[2] + 1 << "\n";
  }

  out_file.close();
  return true;
}

Mesh Mesh::transform(const Eigen::Matrix4f &matrix) const {
  Mesh transformed_mesh;
  transformed_mesh.vertices.reserve(this->vertices.size());
  for (const auto &vertex : this->vertices) {
    auto new_vertex = matrix * vertex.homogeneous();
    transformed_mesh.vertices.push_back(new_vertex.hnormalized().head<3>());
  }
  transformed_mesh.faces = this->faces;
  return transformed_mesh;
}

Mesh Mesh::operator+(const Mesh &other) const {
  Mesh combined_mesh;
  // vertices
  combined_mesh.vertices.reserve(this->vertices.size() + other.vertices.size());

  combined_mesh.vertices.insert(combined_mesh.vertices.end(), this->vertices.begin(),
                                this->vertices.end());

  combined_mesh.vertices.insert(combined_mesh.vertices.end(), other.vertices.begin(),
                                other.vertices.end());

  // faces
  combined_mesh.faces.reserve(this->faces.size() + other.faces.size());
  combined_mesh.faces.insert(combined_mesh.faces.end(), this->faces.begin(), this->faces.end());
  for (const auto &face : other.faces) {
    auto new_face = face.array() + this->vertices.size();
    combined_mesh.faces.push_back(new_face);
  }

  return combined_mesh;
}