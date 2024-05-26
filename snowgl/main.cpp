// -----------------------------------------------------------------------
// 					  				README
// 1. Most of this paper can be implemented in ShadowMapping.frag file
// 2. I am using the shadow map as the occlusion map currently.
//	  But I set the light source of the shadow map to right above the object
// 3. I have tried to implement two separate shadow maps, one for normal shadow,
// 	  another for snow occlusion, but it doesn't work.
//    Two shadow maps affect each other, which shouldn't happen.
//	  Try to redo my process, but remember to backup the current code (c++ and glsl) first
//	  copy the file and add .bak extension.
// 4. This shadow map supports soft shadowing.
// 5. I have modified the normal distortion function, so there is no dE anymore.
// 6. I have modified Snow accumulation prediction function and added a new parameter
//    to customize/manipulate the snow effect.
// 7. The dynamic timing system can be used here, as I have found a way to control 
//	  the snow amount based on time, see day_time_temperature.py in the s folder
// -----------------------------------------------------------------------

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <iostream>
#include <sstream>
#include <iomanip>  // for setprecision

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

//#include <ft2build.h>
//#include FT_FREETYPE_H
//#include <GL/freeglut.h>
//#include "OpenGLText.h"
#include <opencv2/opencv.hpp>

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

//#define GLT_IMPLEMENTATION

#include <common/global.hpp>
#include <common/csv_reader.hpp>
//#include <common/gltext.h>
//#include "common/arial_10.h"
//OpenGLText oglText;

// Capture the current OpenGL framebuffer and convert it to an OpenCV Mat
cv::Mat captureFramebufferToCVMat(const int width, const int height) {

    std::vector<unsigned char> buffer(width * height * 3);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, buffer.data());

    cv::Mat tempMat(height, width, CV_8UC3, buffer.data());
    cv::Mat resultMat;
    cv::flip(tempMat, resultMat, 0); // Flip the image along the x-axis

    return resultMat;
}

std::string doubleToString(double value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}


int daytime_index = 360;
int daytime_size = 0;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Handle scroll event
    //printf("Scrolled: xoffset = %f, yoffset = %f\n", xoffset, yoffset);

	daytime_index += (int)yoffset;
	//std::cout << daytime_index << std::endl;
	
	if (daytime_index > daytime_size - 1) {
        daytime_index = 0;
    }

	if(daytime_index < 0){
		daytime_index = daytime_size - 1;
	}

	//std::cout << daytime_index << std::endl;
}

int main(void){

	//int g_width = 800, g_height = 600;

    //if(!oglText.init("arial", g_width, g_height))
    //  //if(!oglText.init(PROJECT_SOURCE_DIR "/Candy Script_48", g_width, g_height))
    //exit(1);

	csv_reader reader("data/data.csv");
    reader.read_csv();
    auto daytime_data = reader.getData();
	daytime_size = daytime_data.size();

	//std::cout << daytime_size << std::endl;
    
	/*
    if (reader.read_csv()) {
        for (const auto& entry : reader.getData()) {
            std::cout << "Time: " << entry.time
                      << ", Minute: " << entry.minute
                      << ", Temperature: " << entry.temperature
                      << ", Snow Amount: " << entry.snow_amount
                      //<< ", Light Intensity B: " << entry.lightIntensityB
                      //<< ", Light Angle: " << entry.lightAngle
                      //<< ", Background Color R: " << entry.backgroundColorR
                      //<< ", Background Color G: " << entry.backgroundColorG
                      //<< ", Background Color B: " << entry.backgroundColorB
                      << std::endl;
        }
    }
	*/
    
	// Initialize GLFW
	if(!glfwInit()){

		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make macOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);


	//FT_Library ft;
	//if (FT_Init_FreeType(&ft)) {
    //	fprintf(stderr, "Could not init freetype library\n");
   	//	return 1;
	//}

	//FT_Face face;
	//if (FT_New_Face(ft, FONT_LOCATION, 0, &face)) {
   	//	fprintf(stderr, "Could not open font\n");
   	//	return 1;
	//}

	//FT_Set_Pixel_Sizes(face, 0, 48);

    
    // We would expect width and height to be WINDOW_WIDTH and WINDOW_HEIGHT
    int windowWidth = WINDOW_WIDTH;
    int windowHeight = WINDOW_HEIGHT;
    // But on MacOS X with a retina screen it'll be WINDOW_WIDTH*2 and WINDOW_HEIGHT*2, so we get the actual framebuffer size:
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}


	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, WINDOW_WIDTH/2, WINDOW_HEIGHT/2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);

	// Accept fragment if it is closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint depthProgramID = LoadShaders( "shaders/DepthRTT.vert", "shaders/DepthRTT.frag" );

	// Get a handle for our "MVP" uniform
	GLuint depthMatrixID = glGetUniformLocation(depthProgramID, "depthMVP");

	// Load the texture
	GLuint Texture = loadBMP_custom(TEXTURE_LOCATION);
	
	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	bool res = loadOBJ(MODEL_LOCATION, vertices, uvs, normals);

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	// Load it into a VBO

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);


	// ---------------------------------------------
	// Render to Texture - specific code begins here
	// ---------------------------------------------

	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	GLuint FramebufferName = 0;
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

	// Depth texture. Slower than a depth buffer, but you can sample it later in your shader
	GLuint depthTexture;
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT16, WINDOW_WIDTH, WINDOW_WIDTH, 0,GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		 
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);

	// No color output in the bound framebuffer, only depth.
	glDrawBuffer(GL_NONE);

	// Always check that our framebuffer is ok
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;


	// The quad's FBO. Used only for visualizing the shadowmap.
	static const GLfloat g_quad_vertex_buffer_data[] = { 
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
	};

	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	// Create and compile our GLSL program from the shaders
	GLuint quad_programID = LoadShaders( "shaders/Passthrough.vert", "shaders/SimpleTexture.frag" );
	GLuint texID = glGetUniformLocation(quad_programID, "texture");

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "shaders/ShadowMapping.vert", "shaders/ShadowMapping.frag" );

	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
	GLuint DepthBiasID = glGetUniformLocation(programID, "DepthBiasMVP");
	GLuint ShadowMapID = glGetUniformLocation(programID, "shadowMap");


 	// Set the scroll callback
    glfwSetScrollCallback(window, scroll_callback);


	float angle = 0.0f;
    //int daytime_index = 0;

	double lastTime = glfwGetTime();
 	int nbFrames = 0;

	double fps = 0;

	do{

		double currentTime = glfwGetTime();
     	nbFrames++;
     	if (currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1 sec ago
         // printf and reset timer
         // printf("%f ms/frame\n", 1000.0/double(nbFrames));
			fps = 1000.0/double(nbFrames);
        	nbFrames = 0;
        	lastTime += 1.0;
     	}

		//std::cout << daytime_index << std::endl;
		auto current_time = daytime_data[daytime_index];

		//daytime_index++;
		//if (daytime_index == daytime_size) {
        //    daytime_index = 0;
        //}

		glUniform1f(glGetUniformLocation(programID, "snow_amount"), current_time.snow_amount);
		glUniform1f(glGetUniformLocation(programID, "light_intensity"), current_time.light_intensity);

		// Render to our framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glViewport(0,0,WINDOW_WIDTH,WINDOW_WIDTH); // Render on the whole framebuffer, complete from the lower left corner to the upper right

		// We don't use bias in the shader, but instead we draw back faces, 
		// which are already separated from the front faces by a small distance 
		// (if your geometry is made this way)
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(depthProgramID);

		glm::vec3 lightInvDir1 = glm::vec3(0.0f, 0.0, 1.0);

		glm::vec3 lightInvDir = glm::vec3(0.0f, 0.0, 1.0);

		// Compute the MVP matrix from the light's point of view
		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-30,30,-30,30,-30,30);
		glm::mat4 depthViewMatrix = glm::lookAt(lightInvDir1, glm::vec3(0,0,0), glm::vec3(0,1,0));
		// or, for spot light :
		//glm::vec3 lightPos(5, 20, 20);
		//glm::mat4 depthProjectionMatrix = glm::perspective<float>(45.0f, 1.0f, 2.0f, 50.0f);
		//glm::mat4 depthViewMatrix = glm::lookAt(lightPos, lightPos-lightInvDir, glm::vec3(0,1,0));

		glm::mat4 depthModelMatrix = glm::mat4(1.0);
		glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(depthMatrixID, 1, GL_FALSE, &depthMVP[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,  // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);

		// Render to the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,windowWidth,windowHeight); // Render on the whole framebuffer, complete from the lower left corner to the upper right

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		glm::vec3 eye_pos = computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		//ViewMatrix = glm::lookAt(glm::vec3(14,6,4), glm::vec3(0,1,0), glm::vec3(0,1,0));
		//glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 ModelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		
		glm::mat4 biasMatrix(
			0.5, 0.0, 0.0, 0.0, 
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0
		);

		glm::mat4 depthBiasMVP = biasMatrix*depthMVP;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniformMatrix4fv(DepthBiasID, 1, GL_FALSE, &depthBiasMVP[0][0]);

		// Put 6 lights, on +x/-x/+y/-y/+z/-z direction respectively.
		glm::vec3 lightInvDirs[6];
		//lightInvDirs[0] = glm::vec3( 1.0f,  0.0f,  0.0f);  // +x
		//lightInvDirs[1] = glm::vec3(-1.0f,  0.0f,  0.0f);  // -x
		//lightInvDirs[2] = glm::vec3( 0.0f,  1.0f,  0.0f);  // +y
		lightInvDirs[3] = glm::vec3( 0.0f, -1.0f,  0.0f);  // -y
		
		//lightInvDirs[4] = glm::vec3( 0.0f,  0.0f,  1.0f);  // +z
		//lightInvDirs[5] = glm::vec3( 0.0f,  0.0f, -1.0f);  // -z

			
		// Get a handle for our "LightPosition" uniform
		GLuint lightInvDirID = glGetUniformLocation(programID, "LightInvDirection_worldspace");

		// Pass the array of light directions to the shader
		glUniform3fv(lightInvDirID, 6, &lightInvDirs[0][0]);

		// Pass the number of lights
		GLuint numLightsID = glGetUniformLocation(programID, "numLights");
		glUniform1i(numLightsID, 6);




		glUniform3f(lightInvDirID, lightInvDir.x, lightInvDir.y, lightInvDir.z);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glUniform1i(ShadowMapID, 1);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Optionally render the shadowmap (for debug only)

	


		// Render only on a corner of the window (or we we won't see the real rendering...)
		glViewport(0,0,512,512);

		// Use our shader
		glUseProgram(quad_programID);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		// Set our "renderedTexture" sampler to use Texture Unit 0
		glUniform1i(texID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Draw the triangle !
		// You have to disable GL_COMPARE_R_TO_TEXTURE above in order to see anything !
		//glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
		glDisableVertexAttribArray(0);

        cv::Mat capturedImage = captureFramebufferToCVMat(WINDOW_WIDTH, WINDOW_HEIGHT);

		std::string fpsText = "FPS: " + std::to_string(int(fps));
        std::string eyePosText = "Eye Position: (" + std::to_string(int(eye_pos.x)) + ", " + std::to_string(int(eye_pos.y)) + ", " + std::to_string(int(eye_pos.z)) + ")";
        

		std::string timeText = "Clock: " + current_time.time;
		std::string temperatureText = "Temperature: " + doubleToString(current_time.temperature);
		std::string lightIntensityText = "Light Intensity: " + doubleToString(current_time.light_intensity);

        cv::putText(capturedImage, fpsText, cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);
        cv::putText(capturedImage, eyePosText, cv::Point(10, 40), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);
		
		cv::putText(capturedImage, timeText, cv::Point(10, 80), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);
		cv::putText(capturedImage, temperatureText, cv::Point(10, 100), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);
		cv::putText(capturedImage, lightIntensityText, cv::Point(10, 120), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);

        cv::imshow("OpenGL Capture", capturedImage);
		if (cv::waitKey(1) >= 0) break;

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

		angle += 0.5f;
    	if (angle > 360.0f)
       		angle -= 360.0f;
	} 
	
	// Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	//FT_Done_Face(face);
	//FT_Done_FreeType(ft);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);
	glDeleteProgram(programID);
	glDeleteProgram(depthProgramID);
	glDeleteProgram(quad_programID);
	glDeleteTextures(1, &Texture);

	glDeleteFramebuffers(1, &FramebufferName);
	glDeleteTextures(1, &depthTexture);
	glDeleteBuffers(1, &quad_vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

