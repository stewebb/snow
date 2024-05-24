
#ifndef GLOBAL_H
#define GLOBAL_H

//typedef unsigned char u08;

//#define MY_PI 3.1415926
//#define TWO_PI (2.0* MY_PI)

/**
 * Window width and height. They should equal.
*/

#define WINDOW_NAME "SnowGL"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

/**
 * Lighting Configuration for Object Illumination
 *
 */


/**
 * Models
*/

#define MODEL_LOCATION "models/StatueOfLiberty.obj"
//#define MODEL_LOCATION "models/grass.obj"

//#define TEXTURE_LOCATION "textures/rainbow.bmp"
#define TEXTURE_LOCATION "textures/checkerboard.bmp"

// Snow color, in RGB format (white with a higher blue part)
//#define SNOW_COLOR_R    240
//#define SNOW_COLOR_G    240
//#define SNOW_COLOR_B    255


#define BACKGROUND_COLOR {0, 0, 0}

// #define BACKGROUND_R 255

#define DISTORTION_SCALAR 0.1

#endif // GLOBAL_H