/*

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H

#include "global.hpp"
#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>
class Texture{
private:

    int objectId;
    bool hasTexture;
    cv::Mat image_data;

public:

    Texture(const std::string& name){
        image_data = cv::imread(name);

        //std::cout << image_data << std::endl;

        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;
    }

    void setObjectId(int objectId){
        this->objectId = objectId;
    }

    int width, height;

    Eigen::Vector3f getColor(float u, float v){
        auto u_img = u * width;
        auto v_img = (1 - v) * height;
        auto color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);
    }

};
#endif //RASTERIZER_TEXTURE_H


*/

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H

#include "global.hpp"
#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>

class Texture {
private:
    int objectId;
    bool hasTexture;
    cv::Mat image_data;

public:
    Texture(const std::string& name);
    int width, height;
    Eigen::Vector3f getColor(float u, float v);

    void setObjectId(int objectId);
    int getObjectId() const;
    
    void setHasTexture(bool hasTexture);
    bool getHasTexture() const;
};

#endif // RASTERIZER_TEXTURE_H
