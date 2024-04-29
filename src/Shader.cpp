Eigen::Vector3f phong_fragment_shader(const fragment_shader_payload &payload) {

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);
    Eigen::Vector3f amb_light_intensity {10, 10, 10};
    float p = 150;

    // TODO Change color range
    auto lights = payload.view_lights;
    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f eye_pos = {0.0f, 0.0f, 5.0f};

    // n, v need to be normalized!
    Eigen::Vector3f n = normal.normalized();
    Eigen::Vector3f v = (eye_pos - point).normalized();

    Eigen::Vector3f Ia = ka.cwiseProduct(amb_light_intensity); 
    //        Vector3f Ia = 0;
    Eigen::Vector3f Id = 0;
    Eigen::Vector3f Is = 0;

    for (auto &light : lights) {

        //float len2 = dotProduct()

            

        // Id = kd * I * (N dot L) if N dot L > 0; 0 otherwise
        Eigen::Vector3f I = light.intensity;
        //Vector3f I = light->intensity;

        //Vector3f L = (light->position - hitPoint).normalized();
        Eigen::Vector3f L = (light.position - point).normalized();

        //float L2 = dotProduct(L, L);
        //float tnear = INFINITY;

 

        float NL = N.dot(L);
        //float NL = dotProduct(N, L);

        if(NL > 0){
            Id += Kd * I * NL;
        }

        // Is = ks * I * (V dot R) ** a if V dot R > 0 and N dot L > 0; 0 otherwise

        // TODO Possible (L, N)?
        Vector3f R = reflect(-L, N);
        Vector3f V = dir.normalized();

        float VR = V.dot(R);
        if(VR > 0 && NL > 0){
            Is += Kd * I * pow(VR, a);
        }   

        result_color += Ia;
        result_color += Id;
        result_color += Is;

    }

    return result_color;

}


// TODO Change to Phong shading
Eigen::Vector3f blinn_phong_fragment_shader(const fragment_shader_payload &payload) {

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);
    Eigen::Vector3f amb_light_intensity {10, 10, 10};
    float p = 150;

    auto lights = payload.view_lights;
    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f eye_pos = {0.0f, 0.0f, 5.0f};

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
    return result_color * 255.f;
}

Eigen::Vector3f texture_fragment_shader(const fragment_shader_payload &payload) {

    // Get the texture value at the texture coordinates of the current fragment.
    Eigen::Vector3f return_color = {0, 0, 0};
    if (payload.texture) {

        // Get the u-v values
        float u = payload.tex_coords[0];
        float v = payload.tex_coords[1];

        // u-v should between [0, 1], restrain values to avoid errors.
        if(u < 0.0f)   u = 0.0f;
        if(u > 1.0f)   u = 1.0f;
        if(v < 0.0f)   v = 0.0f;
        if(v > 1.0f)   v = 1.0f;

        // Find the color value on the texture map
        return_color = payload.texture->getColor(u, v);
    }

    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    // The following code are the normal blinn-phong reflection model.
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    // The color range from the texture map are [0, 255], needs to convert to [0, 1]
    Eigen::Vector3f kd = texture_color / 255.f;
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    float p = 150;

    std::vector<light> lights = payload.view_lights;
    Eigen::Vector3f color = texture_color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f eye_pos = {0.0f, 0.0f, 5.0f};
    Eigen::Vector3f n = normal.normalized();
    Eigen::Vector3f v = (eye_pos - point).normalized();

    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f La = ka.cwiseProduct(amb_light_intensity);

    for (auto &light : lights) {
        
        Eigen::Vector3f l = light.position - point;
        Eigen::Vector3f I = light.intensity;
        float r2 = std::pow(l.norm(), 2.0f);
        Eigen::Vector3f Ir2 = I / r2;

        l = l.normalized();
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
    return result_color * 255.f;
}

//Eigen::Vector3f reflect(const Eigen::Vector3f &vec, const Eigen::Vector3f &axis) {
//    auto costheta = vec.dot(axis);
//    return (2 * costheta * axis - vec).normalized();
//}


Eigen::Vector3f reflect(const Eigen::Vector3f &I, const Eigen::Vector3f &N) const{
    //return I - 2 * dotProduct(I, N) * N;
    return I - 2 * I.dot(N) * N;
}
