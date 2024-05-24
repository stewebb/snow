cmake_minimum_required (VERSION 3.0)
project (SnowGL)

find_package(OpenGL REQUIRED)

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

include_directories(
	external/AntTweakBar-1.16/include/
	external/glfw-3.1.2/include/
	external/glm-0.9.7.1/
	external/glew-1.13.0/include/
	external/assimp-3.0.1270/include/
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

	#main.cpp
	src/main.cpp

	common/global.hpp
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

	src/ShadowMapping.vert
	src/ShadowMapping.frag
	src/DepthRTT.vert
	src/DepthRTT.frag
	src/Passthrough.vert
	src/SimpleTexture.frag
	
)

target_link_libraries(SnowGL
	${ALL_LIBS}
)

# Xcode and Visual working directories
set_target_properties(SnowGL PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/")
create_target_launcher(SnowGL WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/")

SOURCE_GROUP(common REGULAR_EXPRESSION ".*/common/.*" )
SOURCE_GROUP(shaders REGULAR_EXPRESSION ".*/.*shader$" )

set(output_folder build)

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/build/models/)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/models/)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/build/textures/)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/textures/)
endif()

add_custom_command(
    TARGET SnowGL POST_BUILD
    #COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/SnowGL${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_SOURCE_DIR}/output/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/src/*.frag" "${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/src/*.vert" "${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/src/models/*.obj" "${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/models/"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/src/textures/*.bmp" "${CMAKE_CURRENT_SOURCE_DIR}/${output_folder}/textures/"
)

#if (NOT ${CMAKE_GENERATOR} MATCHES "Xcode" )

#add_custom_command(
#    TARGET SnowGL POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/SnowGL${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_SOURCE_DIR}/output/"
#    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/src/*.frag" "${CMAKE_CURRENT_SOURCE_DIR}/output/"#
#	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/src/*.vert" "${CMAKE_CURRENT_SOURCE_DIR}/output/"
#	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/src/*.obj" "${CMAKE_CURRENT_SOURCE_DIR}/output/"
#	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/src/*.bmp" "${CMAKE_CURRENT_SOURCE_DIR}/output/"
#)

##elseif (${CMAKE_GENERATOR} MATCHES "Xcode" )

#endif (NOT ${CMAKE_GENERATOR} MATCHES "Xcode" )
