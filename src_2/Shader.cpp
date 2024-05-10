#include "Shader.hpp"
#include "global.hpp"

#include <random>

class RandomNumberGenerator {
private:
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<double> distribution;

public:
    RandomNumberGenerator() : gen(rd()), distribution(-1.0, 1.0) {}

    double generateRandomNumber() {
        return distribution(gen);
    }
};
    

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

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f Kd = payload.color;
    Eigen::Vector3f Ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

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

RandomNumberGenerator rng;


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

    // Normalize vectors
    Eigen::Vector3f N = normal.normalized();

    float alpha = 0.1;
    Eigen::Vector3f n{rng.generateRandomNumber(), rng.generateRandomNumber(), rng.generateRandomNumber()};
    N = (N + alpha*n).normalized();


    Eigen::Vector3f V = (eye_pos - point).normalized();

    // Ambient light contribution
    Eigen::Vector3f Ia = ka.cwiseProduct(amb_light_intensity);  

    // TODO: Implement noise vector `n` and compute `dE`
    // Eigen::Vector3f n = compute_noise_vector();  
    // FIXME: Use Perlin Noise
    // Eigen::Vector3f dE = compute_exposure_derivative_vector();
    // FIXME:

    //std::cout << rng.generateRandomNumber() << std::endl;
    

    // TODO: Modify normal `N` using noise `n` and exposure derivative `dE`
    // float alpha = 0.4; // or 0.8 for specular component
    // N = N + alpha * n - dE;
    // N.normalize(); // Ensure N remains a unit vector


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