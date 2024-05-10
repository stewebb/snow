#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "Eigen/Dense"
#include "OBJ_Loader.h"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include "rasterizer.hpp"

#include "MVP.hpp"

int main(int argc, const char **argv) {
    std::vector<Triangle *> TriangleList;

    float angle = 140.0; 

    //std::string filename = "output.png";
    rst::Shading shading = rst::Shading::Phong;
    objl::Loader Loader;
    //std::string obj_path = "../models/spot/";

    // Load .obj File
    bool loadout = Loader.LoadFile(OBJ_FILE_LOCATION);

    // Load meshes
    for (auto mesh : Loader.LoadedMeshes) {
        for (int i = 0; i < mesh.Vertices.size(); i += 3) {
            Triangle *t = new Triangle();
            for (int j = 0; j < 3; j++) {
                t->setVertex(j, Vector4f(mesh.Vertices[i + j].Position.X, mesh.Vertices[i + j].Position.Y, mesh.Vertices[i + j].Position.Z, 1.0));
                t->setNormal(j, Vector3f(mesh.Vertices[i + j].Normal.X, mesh.Vertices[i + j].Normal.Y, mesh.Vertices[i + j].Normal.Z));
                t->setTexCoord(j, Vector2f(mesh.Vertices[i + j].TextureCoordinate.X, mesh.Vertices[i + j].TextureCoordinate.Y));
            }
            TriangleList.push_back(t);
        }
    }

    rst::rasterizer r(700, 700);

    //auto texture_path = "hmap.jpg";
    //r.set_texture(Texture(obj_path + texture_path));
    // TODO eye pos issue

    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = snow_phong_fragment_shader;

    Eigen::Vector3f eye_pos = EYE_POS;

    //std::vector<light> lights = {l1, l2, l3, l4, l5, l6, l7, l8};
    //std::vector<light> lights = {l1, l3, l5, l7};
    
    float width = 12.5;
    float height = 12.5;

    auto l0 = light{{0, -height, 0}, {100, 100, 100}};
    auto l1 = light{{width, -height, width}, {100, 100, 100}};
    auto l2 = light{{width, -height, -width}, {100, 100, 100}};
    auto l3 = light{{-width, -height, width}, {100, 100, 100}};
    auto l4 = light{{-width, -height, -width}, {100, 100, 100}};

    //auto l1 = light{{dist, 0, 0}, {100, 100, 100}};
    //auto l2 = light{{0, dist, 0}, {100, 100, 100}};
    //auto l3 = light{{0, 0, dist}, {100, 100, 100}};

    //auto l4 = light{{-dist, 0, 0}, {100, 100, 100}};
    //auto l5 = light{{0, -dist, 0}, {100, 100, 100}};
    //auto l6 = light{{0, 0, -dist}, {100, 100, 100}};

    std::vector<light> lights = {l0, l1, l2, l3, l4};

    //std::vector<light> lights = {l1, l2, l3};
    //std::vector<light> lights = {l4, l5, l6};
    //std::vector<light> lights = {l1, l2, l3, l4, l5, l6};


    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(active_shader);

    int key = 0;
    int frame_count = 0;

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle, {0, 1, 0}, {0, 0, 0}));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));
        r.set_lights(lights);

        r.draw(TriangleList, true, shading);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imshow("image", image);
        //cv::imwrite(filename, image);
        key = cv::waitKey(16);  // 60 FPS
        angle += 5;

        // Key operations
        if      (key == 'a') { eye_pos.x() -= 0.1; } 
        else if (key == 'd') { eye_pos.x() += 0.1; } 
        else if (key == 'w') { eye_pos.y() += 0.1; } 
        else if (key == 's') { eye_pos.y() -= 0.1; } 
        else if (key == 'q') { eye_pos.z() -= 0.1; } 
        else if (key == 'e') { eye_pos.z() += 0.1; } 
        else if (key == 'j') { angle += 10; } 
        else if (key == 'k') { angle -= 10; }

        std::cout << "frame count: " << frame_count++ << std::endl;
        std::cout << "eye_pos: " << eye_pos.transpose() << std::endl;

    }
    return 0;
}