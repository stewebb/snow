
#ifndef GLOBAL_H
#define GLOBAL_H

// OpenGL Rendering Window
#define GL_WINDOW_NAME          "SnowGL (OpenGL Rendering)"
#define WINDOW_WIDTH            1024
#define WINDOW_HEIGHT           1024
#define WINDOW_BORDER           false

// OpenCV Capture Window
//#define USE_OPENCV              // Comment this line if OpenCV is not installed
#define CV_WINDOW_NAME          "SnowGL (OpenCV Capture)"

// OpenCV Output
#define OUTPUT_IMAGE_FILENAME   "outputs/ALLT1700L60N.png"
#define OUTPUT_VIDEO_FILENAME   "outputs/StatueOfLiberty.mp4"
#define OUTPUT_VIDEO_FPS        25
#define AUTO_STOP_RECORDING     false

// Operating Mode 
#define IS_WINDOWS_OS           // Comment this line on non-Windows Operating Systems
#define DAYTIME_SIMULATION      true      
#define FRAME_MICRO_STEP        0.0
#define INITIAL_TIME_OF_DAY     0 * 60

// Manual defined item (If DAYTIME_SIMULATION is set to false)
#define MANUAL_SNOW_AMOUNT      0.0
#define MANUAL_LIGHT_INTENSITY  1.0

// Eye position
#define EYE_POS_X               0
#define EYE_POS_Y               -30
#define EYE_POS_Z               12.5

// Camera view
#define HORIZONTAL_FIXED        true
#define VERTICAL_FIXED          true
#define HORIZONTAL_ANGLE        (MY_PI * 1.00)
#define VERTICAL_ANGLE          (-MY_PI * 1.50)

// Lighting Configuration
// Models and textures
#define MODEL_LOCATION          "models/StatueOfLiberty.obj"
#define TEXTURE_LOCATION        "textures/rainbow.bmp"

// Snow effect
#define SNOW_COLOR_R            0.9375
#define SNOW_COLOR_G            0.9375
#define SNOW_COLOR_B            1.0000
#define DISTORTION_SCALAR       0.1000

// Mathematical constants
#define MY_PI                   3.1415926

#endif // GLOBAL_H