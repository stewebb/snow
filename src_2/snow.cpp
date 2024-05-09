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
    return std::max(UN, 0.0f);

    // angle = acos(U.dot(N) / (||U|| * ||N||))
    //double angle_rad = acos(U.dot(N));
    //double angle_deg = angle_rad * (180.0 / MY_PI);

    //return (angle_deg >= 0 && angle_deg <= 90) ? cos(angle_rad) : 0.0f;

}