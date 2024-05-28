#ifndef RANDOM_NUMBER_H
#define RANDOM_NUMBER_H

#include <random>
#include <Eigen/Eigen>

class RandomNumber {
private:
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<float> distribution;

public:
    RandomNumber();
    float generateNumber();
    Eigen::Vector2f generate2DVector();
    Eigen::Vector3f generate3DVector();
};

#endif // RANDOM_NUMBER_H
