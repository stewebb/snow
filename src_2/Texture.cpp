#include "Texture.hpp"

Texture::Texture(const std::string& name) {
    image_data = cv::imread(name);
    if (image_data.empty()) {
        std::cerr << "Error loading texture from file: " << name << std::endl;
        hasTexture = false;
    } else {
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;
        hasTexture = true;
    }
}

Eigen::Vector3f Texture::getColor(float u, float v) {
    if (!hasTexture) {
        return Eigen::Vector3f(0, 0, 0); // Return black if no texture is loaded
    }

    auto u_img = static_cast<int>(u * width);
    auto v_img = static_cast<int>((1 - v) * height);

    // Check for bounds to avoid accessing out of range pixels
    u_img = std::min(std::max(u_img, 0), width - 1);
    v_img = std::min(std::max(v_img, 0), height - 1);

    auto color = image_data.at<cv::Vec3b>(v_img, u_img);
    return Eigen::Vector3f(color[0], color[1], color[2]);
}

void Texture::setObjectId(int objectId) {
    this->objectId = objectId;
}

int Texture::getObjectId() const {
    return objectId;
}

void Texture::setHasTexture(bool hasTexture) {
    this->hasTexture = hasTexture;
}

bool Texture::getHasTexture() const {
    return hasTexture;
}