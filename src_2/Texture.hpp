#ifndef TEXTURE_H
#define TEXTURE_H

#include "global.hpp"
#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>

class Texture {
private:
    int objectId;
    bool hasTexture;
    cv::Mat image_data;

public:
    Texture(const std::string& name, int objectId, bool hasTexture);
    int width, height;
    Eigen::Vector3f getColor(float u, float v);

    void setObjectId(int objectId);
    int getObjectId() const;

    void setHasTexture(bool hasTexture);
    bool getHasTexture() const;
};

#endif // TEXTURE_H
