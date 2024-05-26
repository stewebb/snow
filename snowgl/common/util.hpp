#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sstream>
#include <iomanip>
#include <opencv2/opencv.hpp>

// Function declarations

/**
 * @brief Captures the current OpenGL framebuffer and converts it into an OpenCV Mat object.
 * @param width The width of the framebuffer to capture.
 * @param height The height of the framebuffer to capture.
 * @return cv::Mat A Mat object containing the captured image in BGR format.
 */
cv::Mat frameBufferToCVMat(const int width, const int height);

/**
 * @brief Converts a floating point number to a string with fixed precision.
 * @param value The floating point number to convert to string.
 * @return std::string A string representation of the floating point number, formatted to two decimal places.
 */
std::string floatToString(float value);

/**
 * @brief Converts an integer to its string representation.
 * @param value The integer value to convert to string.
 * @return std::string A string representation of the integer.
 */
std::string intToString(int value);

#endif // UTIL_HPP
