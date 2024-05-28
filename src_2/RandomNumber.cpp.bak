#include "RandomNumber.hpp"

RandomNumber::RandomNumber() : gen(rd()), distribution(-1.0, 1.0) {}

float RandomNumber::generateNumber() {
    return distribution(gen);
}

Eigen::Vector2f RandomNumber::generate2DVector(){
    return Eigen::Vector2f(distribution(gen), distribution(gen)).normalized();
}

Eigen::Vector3f RandomNumber::generate3DVector(){
    return Eigen::Vector3f(distribution(gen), distribution(gen), distribution(gen)).normalized();
}