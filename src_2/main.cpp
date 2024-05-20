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
#include "CSV_Reader.hpp"

#include "MVP.hpp"

// Load a model with XYZ offsets.
void loadModel(int objectId, const std::string& filePath, Eigen::Vector3f offset, std::vector<Triangle*>& TriangleList) {
    
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

                t->setObjectId(objectId);
                t->setVertex(j, adjustedPosition);
                t->setNormal(j, Eigen::Vector3f(mesh.Vertices[i + j].Normal.X, mesh.Vertices[i + j].Normal.Y, mesh.Vertices[i + j].Normal.Z));
                t->setTexCoord(j, Eigen::Vector2f(mesh.Vertices[i + j].TextureCoordinate.X, mesh.Vertices[i + j].TextureCoordinate.Y));
                
                // Kd in mtl is between 0 and 1, scale it to [0, 255].
                for(int k=0; k<3; k++){
                    t->setColor(k, mesh.MeshMaterial.Kd.X * 255, mesh.MeshMaterial.Kd.Y * 255, mesh.MeshMaterial.Kd.Z * 255);
                }
            }
            TriangleList.push_back(t);
        }
    }
}

int main(int argc, const char **argv) {

    CSV_Reader reader("../../daylight_simulate.csv");
    reader.readCSV();
    auto data = reader.getData();
    /*
    if (reader.readCSV()) {
        for (const auto& entry : reader.getData()) {
            std::cout << "Time: " << entry.time
                      << ", Minute: " << entry.minute
                      << ", Light Intensity R: " << entry.lightIntensityR
                      << ", Light Intensity G: " << entry.lightIntensityG
                      << ", Light Intensity B: " << entry.lightIntensityB
                      << ", Light Angle: " << entry.lightAngle
                      << ", Background Color R: " << entry.backgroundColorR
                      << ", Background Color G: " << entry.backgroundColorG
                      << ", Background Color B: " << entry.backgroundColorB
                      << std::endl;
        }
    }
    */

    // Load OBJ models and set textures
    std::vector<Triangle *> TriangleList;
    std::vector<Texture *> TextureList;

    loadModel(MODEL_OBJECT_ID, MODEL_OBJ_LOCATION, MODEL_OBJ_OFFSET, TriangleList);
    loadModel(GROUND_OBJECT_ID, GROUND_OBJ_LOCATION, GROUND_OBJ_OFFSET, TriangleList);

    TextureList.push_back(new Texture(MODEL_TEXTURE_MAP, MODEL_OBJECT_ID, MODEL_HAS_TEXTURE));
    TextureList.push_back(new Texture(GROUND_TEXTURE_MAP, GROUND_OBJECT_ID, GROUND_HAS_TEXTURE));

    // Set up lights
    Eigen::Vector3f center_intensity {CENTER_INTENSITY, CENTER_INTENSITY, CENTER_INTENSITY};
    Eigen::Vector3f side_intensity   {SIDE_INTENSITY, SIDE_INTENSITY, SIDE_INTENSITY};
    
    auto L_E = light{{                    0, CENTER_VERTICAL_DIST,                     0}, center_intensity};
    auto L_A = light{{ SIDE_HORIZONTAL_DIST,   SIDE_VERTICAL_DIST,  SIDE_HORIZONTAL_DIST}, side_intensity};
    auto L_B = light{{ SIDE_HORIZONTAL_DIST,   SIDE_VERTICAL_DIST, -SIDE_HORIZONTAL_DIST}, side_intensity};
    auto L_C = light{{-SIDE_HORIZONTAL_DIST,   SIDE_VERTICAL_DIST,  SIDE_HORIZONTAL_DIST}, side_intensity};
    auto L_D = light{{-SIDE_HORIZONTAL_DIST,   SIDE_VERTICAL_DIST, -SIDE_HORIZONTAL_DIST}, side_intensity};
    std::vector<light> lights = {L_E, L_A, L_B, L_C, L_D};

    // Set up scene
    rst::rasterizer r(WINDOW_WIDTH, WINDOW_HEIGHT);
    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(phong_fragment_shader);
    r.setTextures(TextureList);

    // Initialize some variables
    float angle = 0.0; 
    int key = 0;
    int frame_count = 0;
    auto start = std::chrono::high_resolution_clock::now();
    Eigen::Vector3f eye_pos = EYE_POS;

    r.set_occlusion_view(get_view_matrix(Eigen::Vector3f(0, 0, 1)));

    while (key != 27) {

        int minute_count = frame_count % 1440;
        auto current_minute = data[minute_count];

        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle, {0, 1, 0}, {0, 0, 0}));

        // calculate occlusion map
        r.set_occlusion_view(get_view_matrix({0, 0, 100}));


        r.draw_occlusion_map(TriangleList, false);
        //std::cout << r.occlusion_buffer() << std::endl;

        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));
        r.set_lights(lights);

        r.draw(TriangleList, true, false, true);


        //auto of = r.occlusion_buffer();
        //for(float o : of){
        //    //if(o != 0){
        //        std::cout << o << " ";
        //    //}
        //}
        //std::cout << std::endl;

        cv::Mat image(WINDOW_HEIGHT, WINDOW_WIDTH, CV_32FC3, r.frame_buffer().data());
        //cv::Mat image(WINDOW_HEIGHT, WINDOW_WIDTH, CV_32FC3, r.depth_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        // Calculate FPS (Frames per second)
        frame_count++;
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        float fps = 1.0 / elapsed.count();
        start = end;

        // Display statistical information.
        std::string fpsText = "FPS: " + std::to_string(int(fps));
        std::string eyePosText = "Eye Position: (" + std::to_string(int(eye_pos.x())) + ", " + std::to_string(int(eye_pos.y())) + ", " + std::to_string(int(eye_pos.z())) + ")";
        std::string timeText = "Clock: " + current_minute.time;

        cv::putText(image, fpsText, cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);
        cv::putText(image, eyePosText, cv::Point(10, 40), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);
        //cv::putText(image, timeText, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);

        cv::imshow("Snow", image);
        key = cv::waitKey(16);
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
        else if (key == ' ') {cv::imwrite("snow.png", image); }
    }

    cv::destroyWindow("Snow");
    return 0;
}