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

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
GLFWwindow* window;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <common/global.hpp>
#include <common/csv_reader.hpp>
#include <common/util.hpp>

#ifdef USE_OPENCV
#include <opencv2/opencv.hpp>
#endif

double f_daytime_index = 0.0f;
int daytime_size = 0;

/**
 * @brief Handles scroll events in a GLFW window to adjust a daytime index.
 *
 * This function updates a global variable `f_daytime_index` based on the vertical scroll input (`yoffset`).
 * The function increments or decrements the `f_daytime_index` by `yoffset`, ensuring that the index
 * wraps around if it exceeds the bounds defined by `daytime_size`. This is useful for cycling through
 * a series of values (like time of day settings) in response to scroll input.
 *
 * @param window A pointer to the GLFWwindow that received the event.
 * @param xoffset The scroll offset along the x-axis. This parameter is not used in this function.
 * @param yoffset The scroll offset along the y-axis, used to adjust the `f_daytime_index`.
 */

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    f_daytime_index += yoffset;

    if (f_daytime_index >= daytime_size) {
        f_daytime_index = 0.0;
    } else if (f_daytime_index < 0.0) {
        f_daytime_index = daytime_size - 1.0;
    }
}

int main(void){

	// Read generated data file from day_time_simulator.py
	csv_reader reader("data/data.csv");
    if(!reader.read_csv()){
		fprintf(stderr, "Failed to read data file.\n" );
		getchar();
		return -1;
	}

    auto daytime_data = reader.getData();
	daytime_size = daytime_data.size();

	if(daytime_size <= 0){
		fprintf(stderr, "No valid data found.\n" );
		getchar();
		return -1;
	}

	// Setup VideoWriter
	#ifdef USE_OPENCV
    cv::VideoWriter video(OUTPUT_VIDEO_FILENAME, cv::VideoWriter::fourcc('X','2','6','4'), OUTPUT_VIDEO_FPS, cv::Size(WINDOW_WIDTH, WINDOW_HEIGHT));
    if (!video.isOpened()) {
        std::cerr << "Error: Could not open the video file for output\n";
		getchar();
        return -1;
    }
	#endif
    
	if(!glfwInit()){
		fprintf( stderr, "Failed to initialize GLFW.\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, GL_WINDOW_NAME, NULL, NULL);
	if(window == NULL ){
		fprintf( stderr, "Failed to open GLFW window.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
    
    int windowWidth = WINDOW_WIDTH;
    int windowHeight = WINDOW_HEIGHT;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW.\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwPollEvents();
    glfwSetCursorPos(window, WINDOW_WIDTH/2, WINDOW_HEIGHT/2);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); 
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	GLuint depthProgramID = LoadShaders( "shaders/DepthRTT.vert", "shaders/DepthRTT.frag" );
	GLuint depthMatrixID = glGetUniformLocation(depthProgramID, "depthMVP");

	// Load model and texture
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	bool res = loadOBJ(MODEL_LOCATION, vertices, uvs, normals);
	GLuint Texture = loadBMP_custom(TEXTURE_LOCATION);

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

	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

	// Render to Texture
	GLuint FramebufferName = 0;
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

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

	glDrawBuffer(GL_NONE);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

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

	GLuint quad_programID = LoadShaders( "shaders/Passthrough.vert", "shaders/SimpleTexture.frag" );
	GLuint texID = glGetUniformLocation(quad_programID, "texture");
	GLuint programID = LoadShaders( "shaders/ShadowMapping.vert", "shaders/ShadowMapping.frag" );
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
	GLuint DepthBiasID = glGetUniformLocation(programID, "DepthBiasMVP");
	GLuint ShadowMapID = glGetUniformLocation(programID, "shadowMap");

 	// The mouse scroll callback
    glfwSetScrollCallback(window, scroll_callback);
	
	// Data for FPS calculation
	double lastTime = glfwGetTime();
 	int nbFrames = 0;
	double fps = 0;
	int frame_count = 0;

	do {

		// FPS Calculation
		double currentTime = glfwGetTime();
     	nbFrames++;
     	if (currentTime - lastTime >= 1.0 ){
			fps = 1000.0 / double(nbFrames);
        	nbFrames = 0;
        	lastTime += 1.0;
     	}

		// Increase time
		f_daytime_index += FRAME_MICRO_STEP;
		if(f_daytime_index > daytime_size - 1.0){
			f_daytime_index = 0;
		}

		int daytime_index = (int)f_daytime_index;
		auto current_time = daytime_data[daytime_index];

		// Set some parameters based on time
		glClearColor(current_time.sky_color_r, current_time.sky_color_g, current_time.sky_color_b, 0.0f);
		glUniform3f(glGetUniformLocation(programID, "sun_color"), current_time.sun_color_r, current_time.sun_color_g, current_time.sun_color_b);
		glUniform1f(glGetUniformLocation(programID, "snow_amount"), current_time.snow_amount);
		glUniform1f(glGetUniformLocation(programID, "light_intensity"), current_time.light_intensity);

		glUniform3f(glGetUniformLocation(programID, "snow_color"), SNOW_COLOR_R, SNOW_COLOR_G, SNOW_COLOR_B);
		glUniform1f(glGetUniformLocation(programID, "distortion_scalar"), DISTORTION_SCALAR);

		// Render to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glViewport(0,0,WINDOW_WIDTH,WINDOW_WIDTH);
	
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(depthProgramID);

		// A virtual "light" to get the occlusion map
		// Typically the light source is right above the object if no wind.
		glm::vec3 snow_occlusion_light_direction = glm::vec3(0.0f, 0.0, 1.0);

		// Compute the MVP matrix from the light's point of view
		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-30, 30, -30, 30, -30, 30);
		glm::mat4 depthViewMatrix = glm::lookAt(snow_occlusion_light_direction, glm::vec3(0,0,0), glm::vec3(0,1,0));
		glm::mat4 depthModelMatrix = glm::mat4(1.0);
		glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
		glUniformMatrix4fv(depthMatrixID, 1, GL_FALSE, &depthMVP[0][0]);

		// 1st attribute buffer: vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, (void*)0);
		glDisableVertexAttribArray(0);

		// Render to the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, windowWidth, windowHeight);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		glm::vec3 eye_pos = computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		
		glm::mat4 biasMatrix(
			0.5, 0.0, 0.0, 0.0, 
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0
		);

		glm::mat4 depthBiasMVP = biasMatrix*depthMVP;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniformMatrix4fv(DepthBiasID, 1, GL_FALSE, &depthBiasMVP[0][0]);

		// Set up light(s)
		glm::vec3 lightInvDirs[6];
		lightInvDirs[0] = glm::vec3(current_time.light_direction_x, current_time.light_direction_y,  current_time.light_direction_z);
			
		GLuint lightInvDirID = glGetUniformLocation(programID, "LightInvDirection_worldspace");
		glUniform3fv(lightInvDirID, 6, &lightInvDirs[0][0]);
		GLuint numLightsID = glGetUniformLocation(programID, "numLights");
		glUniform1i(numLightsID, 6);

		// Texture binding
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glUniform1i(TextureID, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glUniform1i(ShadowMapID, 1);

		// 1st attribute buffer: vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,	0, (void*)0);

		// 2nd attribute buffer: UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// 3rd attribute buffer: normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(2, 3,	GL_FLOAT, GL_FALSE, 0, (void*)0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, (void*)0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glViewport(0, 0, 512, 512);
		glUseProgram(quad_programID);

		// Bind texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glUniform1i(texID, 0);

		// 1st attribute buffer: vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glDisableVertexAttribArray(0);
		
		// Convert the OpenGL Framebuffer to OpenCV Mat

		#ifdef USE_OPENCV
        cv::Mat capturedImage = frameBufferToCVMat(WINDOW_WIDTH, WINDOW_HEIGHT);

		// Set some statistical texts.
		std::string fpsText 		   = "FPS: " 			 + std::to_string(int(fps));
        std::string eyePosText 		   = "Eye Position: (" 	 + intToString(eye_pos.x) + ", " + intToString(eye_pos.y) + ", " + intToString(eye_pos.z) + ")";

		std::string timeText 		   = "Time: " 			 + current_time.time;
		std::string temperatureText    = "Temperature: " 	 + floatToString(current_time.temperature)			+ "C";
		std::string snowAmountText 	   = "Snow Amount: " 	 + intToString(current_time.snow_amount * 100)		+ "%";
		std::string lightIntensityText = "Light Intensity: " + intToString(current_time.light_intensity * 100)	+ "%";
		std::string elevationAngleText = "Elevation Angle: " + floatToString(current_time.elevation_angle)		+ "deg";

		// Display those statistical texts.
		int left_pos = 10;
		int down_pos = 20;
        cv::putText(capturedImage, fpsText, 			cv::Point(left_pos, down_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);	down_pos += 20;
        cv::putText(capturedImage, eyePosText, 			cv::Point(left_pos, down_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);	down_pos += 40;
		
		cv::putText(capturedImage, timeText, 			cv::Point(left_pos, down_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);	down_pos += 20;
		cv::putText(capturedImage, snowAmountText,  	cv::Point(left_pos, down_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);	down_pos += 20;
		cv::putText(capturedImage, temperatureText, 	cv::Point(left_pos, down_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);	down_pos += 20;
		cv::putText(capturedImage, lightIntensityText,	cv::Point(left_pos, down_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);	down_pos += 20;
		cv::putText(capturedImage, elevationAngleText,  cv::Point(left_pos, down_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 2);	down_pos += 20;

		video.write(capturedImage);
        cv::imshow(CV_WINDOW_NAME, capturedImage);
		if (cv::waitKey(1) >= 0) break;

		frame_count++;
		if(AUTO_STOP_RECORDING && frame_count >= daytime_size){
			break;
		}
		#endif

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} 
	
	// Check if the ESC key was pressed or the window was closed
	while(glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

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

	#ifdef USE_OPENCV
	video.release();
    cv::destroyAllWindows();
	#endif

	return 0;
}