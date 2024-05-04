#include "Shader.hpp"
#include "global.hpp"

static Eigen::Vector3f reflect(const Eigen::Vector3f &vec, const Eigen::Vector3f &axis) {
    auto costheta = vec.dot(axis);
    return (2 * costheta * axis - vec).normalized();
}

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
    return result_color * 255.f;
}
