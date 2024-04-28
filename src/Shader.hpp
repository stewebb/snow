
#ifndef RASTERIZER_SHADER_H
#define RASTERIZER_SHADER_H
#include <Eigen/Eigen>
#include <vector>
#include "Texture.hpp"

struct light {
    Eigen::Vector3f position;
    Eigen::Vector3f intensity;
};

struct fragment_shader_payload
{
    fragment_shader_payload()
    {
        texture = nullptr;
    }

    fragment_shader_payload(const Eigen::Vector3f& col, const Eigen::Vector3f& nor,const Eigen::Vector2f& tc, const std::vector<light> vl, Texture* tex) :
         color(col), normal(nor), tex_coords(tc), view_lights(vl), texture(tex) {}


    Eigen::Vector3f view_pos;
    Eigen::Vector3f color;
    Eigen::Vector3f normal;
    Eigen::Vector2f tex_coords;
    std::vector<light> view_lights;
    Texture* texture;
};

struct vertex_shader_payload
{
    Eigen::Vector3f position;
};


#endif //RASTERIZER_SHADER_H
