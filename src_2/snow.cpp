#include "snow.hpp"
#include "global.hpp"
#include <iostream>

/**
 * Calculate the inclination value f_inc.
 * @param U The resultant direction.
 * @param N Surface Normal.
 * @param n Noise value, typically between 0 and 0.4.
 * @return The inclination value.
*/

std::random_device rd;
std::uniform_real_distribution<> inclination_random_dis(0.0, 0.4);
std::mt19937 inclination_random_gen(rd());

float inclination(Eigen::Vector3f N){
    float mag = N.norm();
    if (mag < 0.001) return 0.0f;

    Eigen::Vector3f U = Eigen::Vector3f(0, 1, 0);
    //Eigen::Vector3f U = Eigen::Vector3f(0, 0, 1);

    float cosTheta = U.dot(N) / N.norm();
    if (cosTheta < 0) return 0.0f;

    float n = inclination_random_dis(inclination_random_gen);
    return std::min(cosTheta + n, 1.0f);
}

/**
 * Calculate the exposure value f_c
 * TODO: FIXME: Use correct value to replace the random value!!!
*/

float exposure(){
}

/**
 * Calculate the prediction value f_p
*/
float prediction(Eigen::Vector3f N) {
    return exposure() * inclination(N);
}

//std::random_device rd;  // Will be used to obtain a seed for the random number engine
//    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    
//    // Create a distribution in range [0, 1]
//    std::uniform_real_distribution<> dis(0.0, 1.0);
    
//    // Generate and print 10 random numbers
//    for (int n = 0; n < 10; ++n) {
//        std::cout << dis(gen) << ' ';
//    }

/**
 * #include <iostream>
#include <vector>
#include <cmath>

// Placeholder for an actual Perlin noise function
float perlin(float x, float y, float z) {
    // Implementation or library function call
    // For example, you might use a library like libnoise or FastNoise
    return std::sin(x) * std::cos(y) * std::sin(z);  // Simplified example
}

Eigen::Vector3f compute_noise_vector(const Eigen::Vector3f& position) {
    // Scale factors adjust the "frequency" of the noise
    float scale = 0.1;
    Eigen::Vector3f noise_vector;
    noise_vector.x() = perlin(position.x() * scale, position.y() * scale, position.z() * scale);
    noise_vector.y() = perlin(position.x() * scale + 5.2, position.y() * scale + 5.2, position.z() * scale + 5.2);
    noise_vector.z() = perlin(position.x() * scale + 10.4, position.y() * scale + 10.4, position.z() * scale + 10.4);

    return noise_vector.normalized();  // Normalize to keep the magnitude manageable
}

int main() {
    Eigen::Vector3f position(1.0, 2.0, 3.0);  // Example position
    Eigen::Vector3f noise_vec = compute_noise_vector(position);
    std::cout << "Noise Vector: " << noise_vec.transpose() << std::endl;

    return 0;
}
*/

/**
 * #include <Eigen/Core>
#include <cmath>

// Placeholder for a simple exposure function
float exposure_function(const Eigen::Vector3f& position, const Eigen::Vector3f& light_pos) {
    // Simplistic exposure function: inverse square law attenuated by angle
    Eigen::Vector3f to_light = light_pos - position;
    float distance_squared = to_light.squaredNorm();
    float cos_theta = to_light.normalized().dot(Eigen::Vector3f(0, 0, 1)); // assuming light from above
    return std::max(0.0f, cos_theta) / (1.0f + distance_squared);
}

Eigen::Vector3f compute_exposure_derivative_vector(const Eigen::Vector3f& position, const Eigen::Vector3f& light_pos) {
    float h = 0.001f;  // Small offset for finite differences
    Eigen::Vector3f dx(h, 0, 0), dy(0, h, 0);

    // Central differences for better accuracy
    float dEx = (exposure_function(position + dx, light_pos) - exposure_function(position - dx, light_pos)) / (2 * h);
    float dEy = (exposure_function(position + dy, light_pos) - exposure_function(position - dy, light_pos)) / (2 * h);

    return Eigen::Vector3f(dEx, dEy, 0.0f);  // Assuming no z-component needed for 2D screen space derivatives
}

int main() {
    Eigen::Vector3f position(1.0, 2.0, 3.0);  // Example position
    Eigen::Vector3f light_pos(5.0, 5.0, 10.0); // Example light position
    Eigen::Vector3f dE = compute_exposure_derivative_vector(position, light_pos);
    std::cout << "Exposure Derivative Vector: " << dE.transpose() << std::endl;

    return 0;
}

*/
