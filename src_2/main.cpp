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

#define EYE_POS {0, 0, -5}

// Eye (camera) position. 
// It need to be used in multiple places, so it's a global variable.
Eigen::Vector3f eye_pos = EYE_POS;

Eigen::Matrix4f get_rotation(float rotation_angle, const Eigen::Vector3f &axis) {
    // Calculate a rotation matrix from rotation axis and angle.
    // Note: rotation_angle is in degree.
    Eigen::Matrix4f rotation_matrix = Eigen::Matrix4f::Identity();

    float rotation_angle_rad = rotation_angle * MY_PI / 180.0;
    float cos_theta = cos(rotation_angle_rad);
    float sin_theta = sin(rotation_angle_rad);

    Eigen::Vector3f axis_ = axis.normalized();
    Eigen::Matrix3f identity = Eigen::Matrix3f::Identity();
    Eigen::Matrix3f ux;
    ux << 0, -axis_.z(), axis_.y(), axis_.z(), 0, -axis_.x(), -axis_.y(), axis_.x(), 0;

    Eigen::Matrix3f rotation_matrix_3x3 =
        cos_theta * identity + (1 - cos_theta) * (axis_ * axis_.transpose()) + sin_theta * ux;
    rotation_matrix.block<3, 3>(0, 0) = rotation_matrix_3x3;

    return rotation_matrix;
}

// Calculate a transformation matrix of given translation vector.
Eigen::Matrix4f get_translation(const Eigen::Vector3f &translation) {
    Eigen::Matrix4f trans = Eigen::Matrix4f::Identity();
    trans(0, 3) = translation.x();
    trans(1, 3) = translation.y();
    trans(2, 3) = translation.z();
    return trans;
}

Eigen::Matrix4f look_at(Eigen::Vector3f eye_pos, Eigen::Vector3f target, Eigen::Vector3f up = Eigen::Vector3f(0, 1, 0)) {
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Vector3f z = (eye_pos - target).normalized();
    Eigen::Vector3f x = up.cross(z).normalized();
    Eigen::Vector3f y = z.cross(x).normalized();

    Eigen::Matrix4f rotate;
    rotate << x.x(), x.y(), x.z(), 0, y.x(), y.y(), y.z(), 0, z.x(), z.y(), z.z(), 0, 0, 0, 0, 1;

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1, -eye_pos[2], 0, 0, 0, 1;

    view = rotate * translate * view;
    return view;
}

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos) {
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
    view = look_at(eye_pos, Eigen::Vector3f(0, 0, 0), Eigen::Vector3f(0, 1, 0));
    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle, const Eigen::Vector3f &axis, const Eigen::Vector3f &translation) {
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f rotation = get_rotation(rotation_angle, axis);
    Eigen::Matrix4f trans = get_translation(translation);
    model = trans * rotation * model;
    return model;
}

// Create the projection matrix for the given parameters. Then return it.
Eigen::Matrix4f get_projection_matrix(float eye_fovy, float aspect_ratio, float zNear, float zFar) {

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    float eye_fovy_rad = eye_fovy * MY_PI / 180.0;
    float top = zNear * tan(eye_fovy_rad / 2.0);
    float bottom = -top;
    float right = top * aspect_ratio;
    float left = -right;

    projection << zNear / right, 0, 0, 0, 0, zNear / top, 0, 0, 0, 0, (zNear + zFar) / (zNear - zFar),
        2 * zNear * zFar / (zNear - zFar), 0, 0, -1, 0;
    return projection;
}

Eigen::Vector3f vertex_shader(const vertex_shader_payload &payload) {
    return payload.position;
}

Vector3f reflect(const Vector3f &I, const Vector3f &N){
    return I - 2 * I.dot(N) * N;
}

//static Eigen::Vector3f reflect(const Eigen::Vector3f &vec, const Eigen::Vector3f &axis) {
//    auto costheta = vec.dot(axis);
//    return (2 * costheta * axis - vec).normalized();
//}

Eigen::Vector3f phong_fragment_shader(const fragment_shader_payload &payload) {

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f Kd = payload.color;
    Eigen::Vector3f Ks = Eigen::Vector3f(0.2, 0.2, 0.2);

    //Eigen::Vector3f Ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);
    //std::cout << Kd.transpose() << std::endl;

    Eigen::Vector3f amb_light_intensity {10, 10, 10};
    float a = 150;

    auto lights = payload.view_lights;
    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f eye_pos = EYE_POS;

    // n, v need to be normalized!
    Eigen::Vector3f N = normal.normalized();
    Eigen::Vector3f V = (eye_pos - point).normalized();

    Eigen::Vector3f Ia = ka.cwiseProduct(amb_light_intensity);  // cwiseProduct--dot product
    //Eigen::Vector3f Id = {0, 0, 0};
    //Eigen::Vector3f Is = {0, 0, 0};

    
    for (auto &light : lights) {


        Eigen::Vector3f l = light.position - point;
        Eigen::Vector3f L = l.normalized();
        float r2 = std::pow(l.norm(), 2.0f);

        //r2 = 1.0;

        Eigen::Vector3f I = light.intensity;
        Eigen::Vector3f Ir2 = I / r2;

        float NL = N.dot(L);
        Eigen::Vector3f Ld = Kd.array() * Ir2.array() * std::max(0.0f, NL);
        

        if(Ld.norm() > 0){
            //std::cout << Kd.transpose() << std::endl;
        }

        result_color += Ld;
        
        //Eigen::Vector3f Ir2 = I / r2;

        // l also needs to be normalized!
        //l = l.normalized();
            

            // Id = kd * I * (N dot L) if N dot L > 0; 0 otherwise
            //Vector3f I = light.intensity;
            //Vector3f L = (light.position - point).normalized();

        

            //float L2 = dotProduct(L, L);
            //float tnear = INFINITY;

            //Ray shadowRay(hitPoint + EPSILON * normalize(L), normalize(L));
            //bool isInShadow = false;
            // Check for intersections with any object up to the distance of the light source
            //if (intersect(shadowRay).happened) {
                //isInShadow = true; // The ray intersects an object before hitting the light, so hitPoint is in shadow
                //std::cout << 123 << std::endl;
            //    continue;
            //}


        

            //if(NL > 0){
            //    Id[0] += Kd[0] * I[0] * NL;
            //    Id[1] += Kd[1] * I[1] * NL;
            //    Id[2] += Kd[2] * I[2] * NL;
            //}

        

            //std::cout << Id.transpose() << std::endl;

            // Is = ks * I * (V dot R) ** a if V dot R > 0 and N dot L > 0; 0 otherwise

            //Eigen::Vector3f R = reflect(L, N);
            //Vector3f V = dir.normalized();

            //float VR = V.dot(R);
            //if(VR > 0 && NL > 0){
                //std::cout << 233 << std::endl;
                //Is[0] += Ks[0] * Ir2[0] * pow(VR, a);
                //Is[1] += Ks[1] * Ir2[1] * pow(VR, a);
                //Is[2] += Ks[2] * Ir2[2] * pow(VR, a);
                //std::cout << Is << std::endl;
                //std::cout << Ks << " " << I << " " << VR << " " << a << std::endl;
            //}
        

        
    }

    
    result_color += Ia;
    //result_color += Id;
    //result_color += Is;

    //std::cout << Ia.transpose() << " " << Id.transpose() << " " << Is.transpose() << std::endl;


    //result_color += La;
    return result_color * 255.f;
}

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
        //result_color += Ld;

        if(Ld.norm() > 0){
            std::cout << kd.transpose() << std::endl;
        }
        
        
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

    //result_color += La;
    return result_color * 255.f;
}

Eigen::Vector3f texture_fragment_shader(const fragment_shader_payload &payload) {
    Eigen::Vector3f return_color = {0, 0, 0};
    if (payload.texture) {

        // Get the current u and v
        float u = payload.tex_coords[0];
        float v = payload.tex_coords[1];

        // Boundary check
        if(u < 0.0f)   u = 0.0f;
        if(u > 1.0f)   u = 1.0f;
        if(v < 0.0f)   v = 0.0f;
        if(v > 1.0f)   v = 1.0f;

        // Perform u-v mapping
        return_color = payload.texture->getColor(u, v);
    }

    // Set texture color
    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = texture_color / 255.0f;    // [0-255] --> [0-1]
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    float p = 150;

    std::vector<light> lights = payload.view_lights;
    Eigen::Vector3f color = texture_color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f La = ka.cwiseProduct(amb_light_intensity);  // cwiseProduct--dot product

    Eigen::Vector3f n = normal.normalized();
    Eigen::Vector3f v = (eye_pos - point).normalized();
    
    // Iterate all lights
    for (auto &light : lights) {

        Eigen::Vector3f l = light.position - point;

        // Square of the distance r
        float r2 = std::pow(l.norm(), 2.0f);

        Eigen::Vector3f I = light.intensity;
        Eigen::Vector3f Ir2 = I / r2;
        float nl = n.dot(l);
        Eigen::Vector3f Ld = kd.array() * Ir2.array() * std::max(0.0f, nl);
        result_color += Ld;

        Eigen::Vector3f h = (v + l).normalized();
        float nh = n.dot(h);
        float nhp = std::pow(std::max(0.0f, nh), p);

        Eigen::Vector3f Ls = ks.array() * Ir2.array() * nhp;
        result_color += Ls;        
    }
    result_color += La;

    // [0-1] --> [0-255]
    return result_color * 255.0f;
}


int main(int argc, const char **argv) {
    std::vector<Triangle *> TriangleList;

    float angle = 0.0;
    bool command_line = false;

    std::string filename = "output.png";
    rst::Shading shading = rst::Shading::Phong;
    objl::Loader Loader;
    std::string obj_path = "../models/spot/";

    // Load .obj File
    //bool loadout = Loader.LoadFile("../../common_models/spot.obj");
    //bool loadout = Loader.LoadFile("../../common_models/stanford-bunny.obj");
    bool loadout = Loader.LoadFile("../models/spot/spot_triangulated_good.obj");
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

    rst::rasterizer r(700, 700);

    auto texture_path = "hmap.jpg";
    r.set_texture(Texture(obj_path + texture_path));

    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = phong_fragment_shader;

    /*
    if (argc < 3) {
        std::cout << "Usage: [Shader (texture, normal, blinn-phong, bump)] [Shading "
                     "Frequency (Flat, Gouraud, Phong)]  [savename.png]"
                  << std::endl;
        return 1;
    } else {
        if (argc == 4) {
            command_line = true;
            filename = std::string(argv[3]);
        }
        if (std::string(argv[1]) == "texture") {
            std::cout << "Rasterizing using the texture shader\n";
            active_shader = texture_fragment_shader;
            texture_path = "spot_texture.png";
            r.set_texture(Texture(obj_path + texture_path));
        } else if (std::string(argv[1]) == "normal") {
            std::cout << "Rasterizing using the normal shader\n";
            active_shader = normal_fragment_shader;
        } else if (std::string(argv[1]) == "blinn-phong") {
            std::cout << "Rasterizing using the phong shader\n";
            active_shader = blinn_phong_fragment_shader;
        } else if (std::string(argv[1]) == "bump") {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = bump_fragment_shader;
        }

        if (std::string(argv[2]) == "Flat") {
            std::cout << "Rasterizing using Flat shading\n";
            shading = rst::Shading::Flat;
        } else if (std::string(argv[2]) == "Gouraud") {
            std::cout << "Rasterizing using Goround shading\n";
            shading = rst::Shading::Gouraud;
        } else if (std::string(argv[2]) == "Phong") {
            std::cout << "Rasterizing using Phong shading\n";
            shading = rst::Shading::Phong;
        }
    }
    */

    /*
    //rst::Shading shading = rst::Shading::Phong;
    auto l1 = light{{ 5,  5,  5}, {50, 50, 50}};
    auto l2 = light{{ 5,  5, -5}, {50, 50, 50}};
    auto l3 = light{{ 5, -5,  5}, {50, 50, 50}};
    auto l4 = light{{ 5, -5, -5}, {50, 50, 50}};
    auto l5 = light{{-5,  5,  5}, {50, 50, 50}};
    auto l6 = light{{-5,  5, -5}, {50, 50, 50}};
    auto l7 = light{{-5, -5,  5}, {50, 50, 50}};
    auto l8 = light{{-5, -5, -5}, {50, 50, 50}};

    Eigen::Vector3f eye_pos = EYE_POS;

    //std::vector<light> lights = {l1, l2, l3, l4, l5, l6, l7, l8};
    std::vector<light> lights = {l1, l3, l5, l7};
    */

    auto l1 = light{{-5, 5, 5}, {10, 10, 10}};
    auto l2 = light{{-20, 20, 0}, {20, 20, 20}};
    std::vector<light> lights = {l1, l2};


    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(active_shader);

    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        r.set_model(get_model_matrix(angle, {0, 1, 0}, {0, 0, 0}));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));
        r.set_lights(lights);

        r.draw(TriangleList, true, shading);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
        cv::imwrite(filename, image);

        return 0;
    }

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
        angle += 5;
        std::cout << "frame count: " << frame_count++ << std::endl;
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