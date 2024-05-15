
#ifndef GLOBAL_H
#define GLOBAL_H

typedef unsigned char u08;

#define MY_PI 3.1415926
#define TWO_PI (2.0* MY_PI)

/**
 * Window width and height. They should equal.
*/

#define WINDOW_WIDTH    512
#define WINDOW_HEIGHT   512

/**
 * Lighting Configuration for Object Illumination
 *
 * This setup includes five lights (A-E) arranged to optimally illuminate an object from above.
 * Light E is positioned directly above the object, while lights A, B, C, and D are positioned at
 * the vertices of a square, centered around light E.
 *
 * Diagram:
 *   A ----------- B
 *   |             |
 *   |             |
 *   |      E      |
 *   |             |
 *   |             |
 *   C ----------- D
 *
 * Vertical Distances (v):
 *   - v_A = v_B = v_C = v_D > 0; v_E > 0
 *   - Typically, v_E > v_A
 *
 * Horizontal Distances (h):
 *   - h_A = h_B = h_C = h_D > 0; h_E = 0
 *
 * Light Intensities (I):
 *   - I_E > I_A = I_B = I_C = I_D
 *   - All lights can be turned off by setting their intensities to 0.
 */

// Intensity and distance definitions for central light (E)
#define CENTER_INTENSITY        25   // Intensity of light E
#define CENTER_VERTICAL_DIST    10   // Vertical distance of light E

// Intensity and distance definitions for side lights (A, B, C, D)
#define SIDE_INTENSITY          100  // Intensity of lights A, B, C, D
#define SIDE_HORIZONTAL_DIST    10   // Horizontal distance for lights A, B, C, D
#define SIDE_VERTICAL_DIST      10   // Vertical distance for lights A, B, C, D

#define EYE_POS {10, 10, 10}

/**
 * A football model
*/

#define MODEL_OBJECT_ID 1
#define MODEL_OBJ_LOCATION "../../models/football.obj"
#define MODEL_OBJ_OFFSET {0, 1.5, 0}
#define MODEL_HAS_TEXTURE false
#define MODEL_TEXTURE_MAP ""

/**
 * A grass ground model
*/

#define GROUND_OBJECT_ID 0
#define GROUND_OBJ_LOCATION "../../models/grass.obj"
#define GROUND_OBJ_OFFSET {0, -1, 0}
#define GROUND_HAS_TEXTURE true
#define GROUND_TEXTURE_MAP "../../models/grass.jpg"

// Snow color, in RGB format (white with a higher blue part)
#define SNOW_COLOR {240, 240, 255}

#define DISTORTION_SCALAR 0.1

#endif // GLOBAL_H