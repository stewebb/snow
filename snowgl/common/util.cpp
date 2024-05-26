#include "util.hpp"
#include <GL/gl.h>

cv::Mat frameBufferToCVMat(const int width, const int height) {
    std::vector<unsigned char> buffer(width * height * 3);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, buffer.data());

    cv::Mat tempMat(height, width, CV_8UC3, buffer.data());
    cv::Mat resultMat;
    cv::flip(tempMat, resultMat, 0);

    return resultMat;
}

std::string floatToString(float value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

std::string intToString(int value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}
