#include "Shader.hpp"
#include "global.hpp"
#include "RandomNumber.hpp"


Eigen::Vector3f vertex_shader(const vertex_shader_payload &payload) {
    return payload.position;
}

static Eigen::Vector3f reflect(const Eigen::Vector3f &vec, const Eigen::Vector3f &axis) {
    auto costheta = vec.dot(axis);
    return (2 * costheta * axis - vec).normalized();
}

/**
 * Normal Phong Shader
*/

Eigen::Vector3f phong_fragment_shader(const fragment_shader_payload &payload) {

    // Get the texture value at the texture coordinates of the current fragment.
    Eigen::Vector3f return_color = {0, 0, 0};
    if (payload.texture) {

        // Get the u-v values
        float u = payload.tex_coords[0];
        float v = payload.tex_coords[1];

        //std::cout << u << " " << v << std::endl;

        // u-v should between [0, 1], restrain values to avoid errors.
        if(u < 0.0f)   u = 0.0f;
        if(u > 1.0f)   u = 1.0f;
        if(v < 0.0f)   v = 0.0f;
        if(v > 1.0f)   v = 1.0f;

        // Find the color value on the texture map
        return_color = payload.texture->getColor(u, v);

        //std::cout << return_color.transpose() << std::endl;

    }

    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    //Eigen::Vector3f Kd = payload.color;
    Eigen::Vector3f Ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    //Eigen::Vector3f Kd = payload.texture ? return_color / 255.0 : payload.color;
    Eigen::Vector3f Kd = return_color / 255.0;

    //std::cout << Kd.transpose() * 255 << std::endl;

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

    
    for (auto &light : lights) {


        Eigen::Vector3f l = light.position - point;
        Eigen::Vector3f L = l.normalized();
        float r2 = std::pow(l.norm(), 2.0f);

        Eigen::Vector3f I = light.intensity;
        Eigen::Vector3f Ir2 = I / r2;

        float NL = N.dot(L);
        Eigen::Vector3f Ld = Kd.array() * Ir2.array() * std::max(0.0f, NL);
        result_color += Ld;

        Eigen::Vector3f R = reflect(L, N);
        float VR = V.dot(R);
        Eigen::Vector3f Ls = {0, 0, 0};

        if(VR > 0 && NL > 0){
            Ls = Ks.array() * Ir2.array() * pow(VR, a);
        }
        result_color += Ls;
    }

    result_color += Ia;
    return result_color * 255.0f;
}

/**
 * The modified Phong Shader for snow.
*/

RandomNumber rn;


Eigen::Vector3f snow_phong_fragment_shader(const fragment_shader_payload &payload) {
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    //Eigen::Vector3f Kd = payload.color;
    Eigen::Vector3f Ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    // Diffuse reflectance Kd is snow color (range: [0, 1])
    Eigen::Vector3f Kd SNOW_COLOR;
    Kd /= 255.0;

    Eigen::Vector3f amb_light_intensity {10, 10, 10};
    float a = 150;

    auto lights = payload.view_lights;
    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f eye_pos = EYE_POS;

    Eigen::Vector3f N = normal.normalized();
    Eigen::Vector3f V = (eye_pos - point).normalized();

    // Ambient light contribution
    Eigen::Vector3f Ia = ka.cwiseProduct(amb_light_intensity);  

    // Calculate the distorted surface normal 
    // N_α = N + αn − dE (Assuming N and n are normalized)
    float alpha = DISTORTION_SCALAR;
    Eigen::Vector3f n = rn.generate3DVector();
    Eigen::Vector3f dE = {0.0f, 0.0f, 0.0f};    // TODO: FIXME: Exposure Function E
    N = (N + alpha*n - dE).normalized();


    for (auto &light : lights) {
        Eigen::Vector3f l = light.position - point;
        Eigen::Vector3f L = l.normalized();
        float r2 = std::pow(l.norm(), 2.0f);

        Eigen::Vector3f I = light.intensity;
        Eigen::Vector3f Ir2 = I / r2;

        float NL = N.dot(L);
        Eigen::Vector3f Ld = Kd.array() * Ir2.array() * std::max(0.0f, NL);
        result_color += Ld;

        Eigen::Vector3f R = reflect(L, N);
        float VR = V.dot(R);
        Eigen::Vector3f Ls = {0, 0, 0};

        if (VR > 0 && NL > 0) {
            Ls = Ks.array() * Ir2.array() * pow(VR, a);
        }
        result_color += Ls;
    }

    // Add ambient component
    result_color += Ia;

    // TODO: Adjust the color to increase the blue component for snow effect
    // result_color.z() *= 1.1; // Assuming 'z' is the blue component in Eigen::Vector3f

    return result_color * 255.0f;  // Scale color to 0-255 range
}

/*
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
        
        // Ls = ks * (I/r^2) * (max(0, n dot h))^p, h = (v+l) / norm(v+l)
        Eigen::Vector3f h = (v + l).normalized();

        float nh = n.dot(h);
        float nhp = std::pow(std::max(0.0f, nh), p);
        Eigen::Vector3f Ls = ks.array() * Ir2.array() * nhp;
        result_color += Ls;        
    }

    result_color += La;
    return result_color * 255.0f;
}
*/