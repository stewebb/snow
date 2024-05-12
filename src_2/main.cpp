#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
//#include <random>

#include "Eigen/Dense"
#include "OBJ_Loader.h"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include "rasterizer.hpp"

#include "MVP.hpp"

// Load a model with XYZ offsets.
void loadModel(const std::string& filePath, Eigen::Vector3f offset, std::vector<Triangle*>& TriangleList) {
    
    objl::Loader Loader;
    bool loadOut = Loader.LoadFile(filePath);

    // If you see "Assertion `loadOut == true' failed. Aborted (core dumped)" error.
    // This means that the obj model cannot be loaded (e.g., file does not exist).
    assert(loadOut == true);

    // Load meshes and apply offsets
    for (auto& mesh : Loader.LoadedMeshes) {
        for (size_t i = 0; i < mesh.Vertices.size(); i += 3) {
            Triangle* t = new Triangle();
            for (int j = 0; j < 3; j++) {
                
                // Adjust X, Y, and Z coordinates by adding the respective offsets
                Eigen::Vector4f adjustedPosition(
                    mesh.Vertices[i + j].Position.X + offset.x(),
                    mesh.Vertices[i + j].Position.Y + offset.y(),
                    mesh.Vertices[i + j].Position.Z + offset.z(),
                    1.0
                );

                t->setVertex(j, adjustedPosition);
                t->setNormal(j, Eigen::Vector3f(mesh.Vertices[i + j].Normal.X, mesh.Vertices[i + j].Normal.Y, mesh.Vertices[i + j].Normal.Z));
                t->setTexCoord(j, Eigen::Vector2f(mesh.Vertices[i + j].TextureCoordinate.X, mesh.Vertices[i + j].TextureCoordinate.Y));
            }
            TriangleList.push_back(t);
        }
    }
}

int main(int argc, const char **argv) {

    // Load OBJ models
    // TODO: FIXME: LOAD MULTIPLE MODELS ON THE SAME SCREEN.
    std::vector<Triangle *> TriangleList;
    loadModel(MODEL_OBJ_LOCATION, MODEL_OBJ_OFFSET, TriangleList);
    loadModel(GROUND_OBJ_LOCATION, GROUND_OBJ_OFFSET, TriangleList);

    float angle = 0.0; 

    // Window size is fixed to 700*700 (px)
    rst::rasterizer r(700, 700);

    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = snow_phong_fragment_shader;

    Eigen::Vector3f eye_pos = EYE_POS;

    // Set up lights
    Eigen::Vector3f center_intensity {CENTER_INTENSITY, CENTER_INTENSITY, CENTER_INTENSITY};
    Eigen::Vector3f side_intensity   {SIDE_INTENSITY, SIDE_INTENSITY, SIDE_INTENSITY};
    
    auto L_E = light{{                    0, -CENTER_VERTICAL_DIST,                     0}, center_intensity};
    auto L_A = light{{ SIDE_HORIZONTAL_DIST,   -SIDE_VERTICAL_DIST,  SIDE_HORIZONTAL_DIST}, side_intensity};
    auto L_B = light{{ SIDE_HORIZONTAL_DIST,   -SIDE_VERTICAL_DIST, -SIDE_HORIZONTAL_DIST}, side_intensity};
    auto L_C = light{{-SIDE_HORIZONTAL_DIST,   -SIDE_VERTICAL_DIST,  SIDE_HORIZONTAL_DIST}, side_intensity};
    auto L_D = light{{-SIDE_HORIZONTAL_DIST,   -SIDE_VERTICAL_DIST, -SIDE_HORIZONTAL_DIST}, side_intensity};
    std::vector<light> lights = {L_E, L_A, L_B, L_C, L_D};


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

        r.draw(TriangleList, true, rst::Shading::Phong);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imshow("image", image);
        //cv::imwrite(filename, image);
        key = cv::waitKey(16);  // 60 FPS
        angle += 5;

        // Key operations
        if      (key == 'a') { eye_pos.x() -= 1; } 
        else if (key == 'd') { eye_pos.x() += 1; } 
        else if (key == 'w') { eye_pos.y() += 1; } 
        else if (key == 's') { eye_pos.y() -= 1; } 
        else if (key == 'q') { eye_pos.z() -= 1; } 
        else if (key == 'e') { eye_pos.z() += 1; } 
        else if (key == 'j') { angle += 10; } 
        else if (key == 'k') { angle -= 10; }

        std::cout << "frame count: " << frame_count++ << std::endl;
        std::cout << "eye_pos: " << eye_pos.transpose() << std::endl;

    }
    return 0;
}