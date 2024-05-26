
#ifndef GLOBAL_H
#define GLOBAL_H

#define MY_PI       3.1415926

/**
 * Window
*/

#define WINDOW_NAME "SnowGL"
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 900


/**
 * Eye position
*/

#define EYE_POS_X 0
#define EYE_POS_Y -40
#define EYE_POS_Z 15

#define HORIZONTAL_FIXED true
#define VERTICAL_FIXED true

#define HORIZONTAL_ANGLE (MY_PI * 1.00)
#define VERTICAL_ANGLE (-MY_PI * 1.50)

/**
 * Lighting Configuration for Object Illumination
 *
 */


/**
 * Models
*/

#define MODEL_LOCATION "models/StatueOfLiberty.obj"
//#define MODEL_LOCATION "models/grass.obj"
//#define MODEL_LOCATION "models/football.obj"

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