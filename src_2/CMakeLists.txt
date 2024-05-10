cmake_minimum_required(VERSION 3.10)
project(Snow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-O3")

# include_directories(/usr/include/)

find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(libnoise REQUIRED)
#find_package(Boost  REQUIRED)

include_directories(${EIGEN3_INCLUDE_DIR} ${OpenCV_INCLUDE_DIRS})

# include_directories(/usr/local/include ./include)

add_executable(snow main.cpp rasterizer.hpp rasterizer.cpp global.hpp Triangle.hpp Triangle.cpp Texture.hpp Texture.cpp Shader.cpp Shader.hpp OBJ_Loader.h MVP.cpp MVP.hpp snow.cpp snow.hpp)
target_link_libraries(snow ${OpenCV_LIBRARIES})