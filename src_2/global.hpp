
#ifndef RASTERIZER_GLOBAL_H
#define RASTERIZER_GLOBAL_H

typedef unsigned char u08;

#define MY_PI 3.1415926
#define TWO_PI (2.0* MY_PI)

/**
 * There are 5 lights (A-E) and all of them are placed above the object.
 * One of them (E) is right above the object.
 * The rest 4 lights (A, B, C, D) are placed to the vertices of a square, whose center is E.
 * 
 * A ----------- B
 * |             |
 * |             |
 * |      E - h -|
 * |             |
 * |             |
 * C ----------- D
 * 
 * v is the vertical distance of the light. 
 * v_A = v_B = v_C = v_D > 0; v_E > 0.
 * 
 * h is the horizontal distance of the light. 
 * h_A = h_B = h_C = h_D > 0; h_E = 0;
 * 
 * Tipically,
 * v_E > v_A; I_E > I_A = I_B = I_C = I_D (I is the light intensity)
 * All lights can be turned off by setting to 0.
*/

#define CENTER_INTENSITY        50      // I_E
#define CENTER_VERTICAL_DIST    15      // h_E

#define SIDE_INTENSITY          100     // I_A, I_B, I_C, I_D
#define SIDE_HORIZONTAL_DIST    10      // h_A, h_B, h_C, h_D
#define SIDE_VERTICAL_DIST      10      // v_A, v_B, v_C, v_D


// Stanford Bunny
//#define OBJ_FILE_LOCATION "../../common_models/stanford-bunny.obj"
//#define EYE_POS {0.25, 0.15, -0.40}

// Football
//#define OBJ_FILE_LOCATION "../../common_models/football.obj"
//#define EYE_POS {2, 2, 2}

// Teapot
//#define OBJ_FILE_LOCATION "../../common_models/teapot.obj"
//#define EYE_POS {5, 5, 5}

// Happy
//#define OBJ_FILE_LOCATION "../../common_models/happy.obj"
//#define EYE_POS {0, 0.4, -0.2}

// Spot
#define OBJ_FILE_LOCATION "../models/spot/spot_triangulated_good.obj"
#define EYE_POS {10, 10, 10}


// Snow color, in RGB format (white with a higher blue part)
#define SNOW_COLOR {240, 240, 255}

#define DISTORTION_SCALAR 0.0

#endif //RASTERIZER_GLOBAL_H
