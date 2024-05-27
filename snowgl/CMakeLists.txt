cmake_minimum_required (VERSION 3.0)
project (SnowGL)

# option(USE_OPENCV "Enable some optional feature" ON)

find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED)

# Compile external dependencies 
add_subdirectory (external)

# On Visual 2005 and above, this module can set the debug working directory
cmake_policy(SET CMP0026 OLD)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
include(CreateLaunchers)
include(MSVCMultipleProcessCompile) # /MP

if(INCLUDE_DISTRIB)
	add_subdirectory(distrib)
endif(INCLUDE_DISTRIB)

include_directories(${OpenCV_INCLUDE_DIRS})


include_directories(
	external/AntTweakBar-1.16/include/
	external/glfw-3.1.2/include/
	external/glm-0.9.7.1/
	external/glew-1.13.0/include/
	external/assimp-3.0.1270/include/
	external/bullet-2.81-rev2613/src/
	external/bullet-2.81-rev2613/src/
	.
)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	glfw
	GLEW_1130
)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

# The project code
add_executable(SnowGL

	main.cpp
	common/global.hpp
	common/csv_reader.hpp
	common/csv_reader.cpp

	common/shader.cpp
	common/shader.hpp
	common/controls.cpp
	common/controls.hpp
	common/texture.cpp
	common/texture.hpp
	common/objloader.cpp
	common/objloader.hpp
	common/vboindexer.cpp
	common/vboindexer.hpp
	common/util.cpp
	common/util.hpp

	shaders/ShadowMapping.vert
	shaders/ShadowMapping.frag
	shaders/DepthRTT.vert
	shaders/DepthRTT.frag
	shaders/Passthrough.vert
	shaders/SimpleTexture.frag
	
)

target_link_libraries(SnowGL
	${ALL_LIBS} ${OpenCV_LIBRARIES}
)

# Xcode and Visual working directories
set_target_properties(SnowGL PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/")
create_target_launcher(SnowGL WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")

SOURCE_GROUP(common REGULAR_EXPRESSION ".*/common/.*" )
SOURCE_GROUP(shaders REGULAR_EXPRESSION ".*/.*shader$" )

set(output_folder build)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${output_folder}/models/)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/models/)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${output_folder}/textures/)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/textures/)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${output_folder}/shaders/)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/shaders/)
endif()

add_custom_command(
    TARGET SnowGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/data/" "${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/data"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders/" "${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/shaders"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/models/" "${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/models/"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/textures/" "${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/textures/"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/outputs/" "${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/outputs/"
)