cmake_minimum_required(VERSION 3.10)
project(Rasterizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-O3")

find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)

include_directories(${EIGEN3_INCLUDE_DIR} ${OpenCV_INCLUDE_DIRS})

# include_directories(/usr/local/include ./include)
#add_executable(task1 task1.cpp rasterizer.hpp rasterizer.cpp global.hpp mesh.cpp Triangle.hpp Triangle.cpp)
#target_link_libraries(task1 ${OpenCV_LIBRARIES})

add_executable(task2 task2.cpp rasterizer.hpp rasterizer.cpp global.hpp Triangle.hpp Triangle.cpp Texture.hpp Texture.cpp Shader.hpp OBJ_Loader.h)
target_link_libraries(task2 ${OpenCV_LIBRARIES})

add_executable(snow main.cpp rasterizer.hpp rasterizer.cpp global.hpp Triangle.hpp Triangle.cpp Texture.hpp Texture.cpp Shader.hpp OBJ_Loader.h)
target_link_libraries(snow ${OpenCV_LIBRARIES})