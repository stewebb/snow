
#ifndef GLOBAL_H
#define GLOBAL_H

#define MY_PI       3.1415926

/**
 * OpenGL Rendering Window
*/

#define GL_WINDOW_NAME      "SnowGL (OpenGL Rendering)"
#define WINDOW_WIDTH        1600
#define WINDOW_HEIGHT       900

/**
 * Eye position
*/

#define EYE_POS_X           0
#define EYE_POS_Y           -30
#define EYE_POS_Z           12.5

#define HORIZONTAL_FIXED    true
#define VERTICAL_FIXED      true

#define HORIZONTAL_ANGLE    (MY_PI * 1.00)
#define VERTICAL_ANGLE      (-MY_PI * 1.50)

/**
 * Lighting Configuration
 */

/**
 * Models
*/

#define MODEL_LOCATION "models/StatueOfLiberty.obj"
//#define MODEL_LOCATION "models/grass.obj"
//#define MODEL_LOCATION "models/football.obj"

#define TEXTURE_LOCATION "textures/pure_color.bmp"
//#define TEXTURE_LOCATION "textures/rainbow.bmp"
//#define TEXTURE_LOCATION "textures/checkerboard.bmp"


/**
 * OpenCV Capture Window and output
*/

#define USE_OPENCV              // Comment this line if OpenCV is not installed
#define CV_WINDOW_NAME          "SnowGL (OpenCV Capture)"

#define OUTPUT_VIDEO_FILENAME   "outputs/StatueOfLiberty.mp4"
#define OUTPUT_VIDEO_FPS        25
#define FRAME_MICRO_STEP        1.0
#define AUTO_STOP_RECORDING     true

/**
 * Snow effect
*/

#define SNOW_COLOR_R            0.9375
#define SNOW_COLOR_G            0.9375
#define SNOW_COLOR_B            1.0000
#define DISTORTION_SCALAR       0.1000

#endif // GLOBAL_H