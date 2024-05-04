#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/ini_parser.hpp>

#include "Eigen/Dense"
#include "OBJ_Loader.h"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Triangle.hpp"
#include "global.hpp"
#include "rasterizer.hpp"

#include "MVP.hpp"


// Eye (camera) position. 
// It need to be used in multiple places, so it's a global variable.
Eigen::Vector3f eye_pos = EYE_POS;


Eigen::Vector3f vertex_shader(const vertex_shader_payload &payload) {
    return payload.position;
}

//Vector3f reflect(const Vector3f &I, const Vector3f &N){
//    return I - 2 * I.dot(N) * N;
//}


Eigen::Vector3f blinn_phong_fragment_shader(const fragment_shader_payload &payload) {

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);
    Eigen::Vector3f amb_light_intensity {10, 10, 10};
    float p = 10;

    auto lights = payload.view_lights;
    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f eye_pos = EYE_POS;

    // n, v need to be normalized!
    Eigen::Vector3f n = normal.normalized();
    Eigen::Vector3f v = (eye_pos - point).normalized();
    Eigen::Vector3f La = ka.cwiseProduct(amb_light_intensity);  // cwiseProduct--dot product

    for (auto &light : lights) {

        // Ld = kd * (I/r^2) * max (0, n dot l)
        Eigen::Vector3f l = light.position - point;
        float r2 = std::pow(l.norm(), 2.0f);

        Eigen::Vector3f I = light.intensity;
        Eigen::Vector3f Ir2 = I / r2;

        // l also needs to be normalized!
        l = l.normalized();
        float nl = n.dot(l);

        // kd, Ir2 element-wise multiplication
        Eigen::Vector3f Ld = kd.array() * Ir2.array() * std::max(0.0f, nl);
        result_color += Ld;

        //if(Ld.norm() > 0){
        //    std::cout << kd.transpose() << std::endl;
        //}
        
        
        // Ls = ks * (I/r^2) * (max(0, n dot h))^p, h = (v+l) / norm(v+l)
        Eigen::Vector3f h = (v + l).normalized();

        float nh = n.dot(h);
        float nhp = std::pow(std::max(0.0f, nh), p);
        Eigen::Vector3f Ls = ks.array() * Ir2.array() * nhp;
        result_color += Ls;        

        //if(Ls.norm() > 0){
        //    std::cout << Ls.transpose() << std::endl;
        //}
        
    }

    result_color += La;
    return result_color * 255.f;
}

int main(int argc, const char **argv) {

    // Load configurations
    /*
    boost::property_tree::ptree pt;
    try {
        boost::property_tree::ini_parser::read_ini("config.ini", pt);
        std::string host = pt.get<std::string>("Database.host");
        int port = pt.get<int>("Database.port");

        std::cout << "Host: " << host << std::endl;
        std::cout << "Port: " << port << std::endl;
    } catch(const boost::property_tree::ini_parser::ini_parser_error& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    */

    std::vector<Triangle *> TriangleList;

    float angle = 140.0;    // TODO INI 

    std::string filename = "output.png";    // TODO INI 
    rst::Shading shading = rst::Shading::Phong;
    objl::Loader Loader;
    //std::string obj_path = "../models/spot/";

    // Load .obj File TODO INI 
    //bool loadout = Loader.LoadFile("../../common_models/happy.obj");
    bool loadout = Loader.LoadFile("../../common_models/stanford-bunny.obj");
    //bool loadout = Loader.LoadFile("../models/spot/spot_triangulated_good.obj");

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

    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = phong_fragment_shader;

    Eigen::Vector3f eye_pos = EYE_POS;

    //std::vector<light> lights = {l1, l2, l3, l4, l5, l6, l7, l8};
    //std::vector<light> lights = {l1, l3, l5, l7};
    

    auto l1 = light{{-5, 5, 5}, {50, 50, 50}};
    auto l2 = light{{5, 0, 0}, {50, 50, 50}};
    std::vector<light> lights = {l1, l2};


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
        cv::imwrite(filename, image);
        key = cv::waitKey(10);
        //angle += 5;
        //std::cout << "frame count: " << frame_count++ << std::endl;

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


/*
int main(int argc, const char **argv) {

    std::vector<Triangle *> TriangleList;

    float angle = 140.0;
    std::string filename = "output.png";
    rst::Shading shading = rst::Shading::Phong;
    objl::Loader Loader;
    std::string texture_path = "../models/scene.png";

    // Load .obj File
    bool loadout = Loader.LoadFile("../../common_models/spot.obj");
    for (auto mesh : Loader.LoadedMeshes) {
        for (int i = 0; i < mesh.Vertices.size(); i += 3) {
            Triangle *t = new Triangle();
            for (int j = 0; j < 3; j++) {
                t->setVertex(j, Vector4f(mesh.Vertices[i + j].Position.X, mesh.Vertices[i + j].Position.Y,
                                         mesh.Vertices[i + j].Position.Z, 1.0));
                t->setNormal(j, Vector3f(mesh.Vertices[i + j].Normal.X, mesh.Vertices[i + j].Normal.Y,
                                         mesh.Vertices[i + j].Normal.Z));
                t->setTexCoord(
                    j, Vector2f(mesh.Vertices[i + j].TextureCoordinate.X, mesh.Vertices[i + j].TextureCoordinate.Y));
            }
            TriangleList.push_back(t);
        }
    }

    // Only the texture fragment shader is used in task 3.
    rst::rasterizer r(700, 700);
    r.set_fragment_shader(blinn_phong_fragment_shader);
    r.set_texture(Texture(texture_path));
    r.set_vertex_shader(vertex_shader);

    // Only one light in this scene
    Eigen::Vector3f light_position = {5, 5, 5};
    Eigen::Vector3f light_intensity = {50, 50, 50};
    std::vector<light> lights = {light{light_position, light_intensity}};

    int key = 0;
    int frame_count = 0;
    bool shadow = false;

    while (key != 27) {

        // Update the shadow-related buffers when view changed!
        if(shadow){

            r.clear(rst::Buffers::Color | rst::Buffers::Depth);
            r.set_model(get_model_matrix(angle, {0, 1, 0}, {0, 0, 0}));

            // Eye position is the light source
            Eigen::Vector3f shadow_eye_pos = light_position;
            Eigen::Matrix4f shadow_view = get_view_matrix(shadow_eye_pos);
            Eigen::Matrix4f shadow_proj = get_projection_matrix(45.0, 1, 0.1, 50);

            r.set_view(shadow_view);
            r.set_projection(shadow_proj);
            r.set_lights(lights);
            r.draw(TriangleList, true, shading);

            // Set and store shadow depth buffer, shadow view and shadow projection.
            r.set_shadow_buffer(r.depth_buffer());
            r.set_shadow_view(shadow_view);
            r.shadow_projection = shadow_proj;
        }
    
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        r.set_model(get_model_matrix(angle, {0, 1, 0}, {0, 0, 0}));
        r.set_view(get_view_matrix(eye_pos));
        
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));
        r.set_lights(lights);

        r.draw(TriangleList, true, shading, shadow);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imshow("image", image);
        cv::imwrite(filename, image);
        key = cv::waitKey(10);

        if (key == 'a') { eye_pos.x() -= 1; } 
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
*/