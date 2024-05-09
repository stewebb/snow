#include "snow.hpp"
#include "global.hpp"

/**
 * Calculate the inclination value f_inc.
 * @param U The resultant direction.
 * @param N Surface Normal.
 * @param n Noise value, typically between 0 and 0.4.
 * @return The inclination value.
*/

float inclination(Eigen::Vector3f U, Eigen::Vector3f N, float n){

    // Normalize vectors, ||U|| = ||N|| = 1
    U = U.normalized();
    N = N.normalized();

    // U.dot(n) is cos(Θ)
    float UN = U.dot(N);
    return UN >= 0 ? UN + n : 0.0f;
    //return std::max(UN+n, 0.0f);
}

/**
 * Calculate the exposure value f_c
 * TODO FIXME Use correct value to replace the random value!!!
*/

float exposure(){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    // ---- FIXME ----
    return dis(gen);
    // ---- FIXME ----
}

//std::random_device rd;  // Will be used to obtain a seed for the random number engine
//    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    
//    // Create a distribution in range [0, 1]
//    std::uniform_real_distribution<> dis(0.0, 1.0);
    
//    // Generate and print 10 random numbers
//    for (int n = 0; n < 10; ++n) {
//        std::cout << dis(gen) << ' ';
//    }
