#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 Normal_modelspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace[6];
in vec4 ShadowCoord;

// Output data
layout(location = 0) out vec3 color;

uniform sampler2D myTextureSampler;
uniform mat4 MV;
uniform vec3 LightPosition_worldspace;
uniform sampler2DShadow shadowMap;
uniform int numLights;
uniform vec3 snow_color;
uniform vec3 distortion_scalar;

uniform float snow_amount;
uniform float light_intensity;
uniform vec3 sun_color;

vec2 poissonDisk[16] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
);

/**
 * Generates a pseudo-random number based on a 3D seed vector and an integer.
 *
 * This function calculates a random number by first extending the 3D seed vector into a 4D vector
 * using the provided integer as the fourth component. It then calculates a dot product of this 
 * extended vector with a fixed vector. The sine of the dot product is taken and multiplied by a 
 * large number to scale the result, and finally the fractional part of this result is returned 
 * to ensure the output is between 0.0 and 1.0.
 *
 * @param seed A vec3 used as the base seed for the random number generation.
 * @param i An int that modifies the seed to generate different random numbers.
 *
 * @return float A pseudo-random number between 0.0 and 1.0.
 */
 
 float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

/**
 * Maps a dot product result to a color based on predefined angle ranges.
 *
 * This function uses the dot product of two normalized vectors (cosine of the angle between them) 
 * to map to specific colors representing various ranges of angles. The mapping is as follows:
 * - [90, 180] degrees maps to Red
 * - [75, 90) degrees maps to Orange
 * - [60, 75) degrees maps to Yellow
 * - [45, 60) degrees maps to Green
 * - [30, 45) degrees maps to Blue
 * - [15, 30) degrees maps to Indigo
 * - [0, 15) degrees maps to Violet
 *
 * Each range is associated with a specific vec3 color value. The function assumes the input `dotn` 
 * is the cosine of the angle, with -1 corresponding to 180 degrees and 1 corresponding to 0 degrees.
 *
 * @param dotn The cosine of the angle between two vectors; should be in the range [-1, 1].
 *
 * @return vec3 The color associated with the given angle range.
 */

vec3 angleColorMapping(float dotn) {

    if (dotn <= 0.0000) 	 {	return vec3(1.00, 0.50, 0.50);	}	// [90, 180] -> Red
	else if (dotn <= 0.2588) {	return vec3(1.00, 0.65, 0.50);	} 	// [75, 90) -> Orange
	else if (dotn <= 0.5000) {	return vec3(1.00, 1.00, 0.60);	} 	// [60, 75) -> Yellow
	else if (dotn <= 0.7071) {	return vec3(0.60, 1.00, 0.60);	} 	// [45, 60) -> Green
	else if (dotn <= 0.8660) {	return vec3(0.50, 0.70, 1.00);	} 	// [30, 45) -> Blue
	else if (dotn <= 0.9659) {	return vec3(0.40, 0.40, 0.70);	} 	// [15, 30) -> Indigo
	else 					 {	return vec3(0.8, 0.6, 1.0);		}	// [0, 15) -> Violet
}

/**
 * Maps a visibility value to a specific color to visually represent varying levels of visibility.
 *
 * This function uses a single floating-point visibility value, normalized between 0 and 1, to determine
 * the color output based on predefined thresholds. The mapping is as follows:
 * - Visibility of 0.0000 or less maps to a red color (vec3(1.00, 0.50, 0.50)).
 * - Visibility up to 0.1666 maps to orange (vec3(1.00, 0.65, 0.50)).
 * - Visibility up to 0.3333 maps to yellow (vec3(1.00, 1.00, 0.60)).
 * - Visibility up to 0.5000 maps to green (vec3(0.60, 1.00, 0.60)).
 * - Visibility up to 0.6666 maps to blue (vec3(0.50, 0.70, 1.00)).
 * - Visibility up to 0.8333 maps to indigo (vec3(0.40, 0.40, 0.70)).
 * - Visibility greater than 0.8333 maps to violet (vec3(0.80, 0.60, 1.00)).
 *
 * @param visibility The normalized visibility value, expected to be in the range [0, 1].
 *
 * @return vec3 The color associated with the given visibility value.
 */

vec3 visibilityColorMapping(float visibility) {

    if (visibility <= 0.0000) 	   {	return vec3(1.00, 0.50, 0.50);	}	// Red
	else if (visibility <= 0.1666) {	return vec3(1.00, 0.65, 0.50);	} 	// Orange
	else if (visibility <= 0.3333) {	return vec3(1.00, 1.00, 0.60);	} 	// Yellow
	else if (visibility <= 0.5000) {	return vec3(0.60, 1.00, 0.60);	} 	// Green
	else if (visibility <= 0.6666) {	return vec3(0.50, 0.70, 1.00);	} 	// Blue
	else if (visibility <= 0.8333) {	return vec3(0.40, 0.40, 0.70);	} 	// Indigo
	else 					 	   {	return vec3(0.8, 0.6, 1.0);		}	// Violet
}

/**
 * Computes the color of an object based on its material properties and lighting, without any snow effect.
 *
 * This function calculates the final color of an object by considering its diffuse, ambient, and specular
 * contributions under multiple light sources. The lighting calculation includes:
 * - Ambient lighting, which is a small fraction of the object's diffuse color to simulate low-level 
 *   omnidirectional light.
 * - Diffuse lighting, which depends on the angle between the light direction and the object's normal.
 * - Specular lighting, which depends on the view direction and the direction of perfect reflection.
 *
 * Each light's contribution is calculated using the Phong reflection model, and the final color is the 
 * sum of the ambient, diffuse, and specular contributions from all lights affecting the object.
 *
 * Material and light properties such as color, intensity, and specular exponent are used to determine 
 * the appearance of the object under lighting.
 *
 * @return vec3 The computed RGB color of the object under the given lighting conditions.
 */
 
 vec3 objectColor(){

	// Light emission properties
	vec3 LightColor = sun_color;
	float LightPower = light_intensity;

	// Material properties
	vec3 MaterialDiffuseColor = texture(myTextureSampler, UV).rgb;
	vec3 MaterialAmbientColor = vec3(0.05, 0.05, 0.05) * MaterialDiffuseColor;
	vec3 MaterialSpecularColor = vec3(0.5, 0.5, 0.5);
	float MaterialSpecularExponent = 150.0f;

	// Normal of the computed fragment, in camera space
	vec3 n = normalize(Normal_cameraspace);

	// Eye vector (towards the camera)
	vec3 E = normalize(EyeDirection_cameraspace);

	// Calculate color contribution from each light
	vec3 color = vec3(0.0);
	for (int i = 0; i < numLights; i++) {

		// Direction of the light (from the fragment to the light)
		vec3 l = normalize(LightDirection_cameraspace[i]);

		// Cosine of the angle between the normal and the light direction
		float cosTheta = clamp(dot(n, l), 0.0, 1.0);

		// Direction in which the triangle reflects the light
		vec3 R = reflect(-l, n);

		// Cosine of the angle between the Eye vector and the Reflect vector
		float cosAlpha = clamp(dot(E, R), 0.0, 1.0);

		// Calculate Ambient, Diffuse, and Specular components
		vec3 Ambient = MaterialAmbientColor;
		vec3 Diffuse = MaterialDiffuseColor * LightColor * LightPower * cosTheta;
		vec3 Specular = MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha, MaterialSpecularExponent);

		// Accumulate contributions from each light
		color += Ambient + Diffuse + Specular;
	}

	return color;
}

/**
 * Computes the color of snow based on its material properties and lighting conditions.
 *
 * This function calculates the snow color by considering its ambient, diffuse, and specular
 * contributions under multiple light sources. The snow color is set to a near-white color, and 
 * the lighting effects include:
 * - Ambient lighting, which is a fraction of the snow's diffuse color to simulate low-level omnidirectional light.
 * - Diffuse lighting, which depends on the angle between the light direction and a distorted normal of the snow surface.
 * - Specular lighting, which is affected by the view direction and the direction of perfect reflection.
 *
 * The distortion in the normal is added to simulate the irregular surface of snow, and each light's
 * contribution is calculated using the Phong reflection model. The final color is the sum of the
 * ambient, diffuse, and specular contributions from all lights affecting the snow.
 *
 * Material and light properties such as color, intensity, and specular exponent are used to determine 
 * the appearance of the snow under lighting conditions.
 *
 * @return vec3 The computed RGB color of the snow under the given lighting conditions.
 */
 
 vec3 snowColor(){

	vec3 LightColor = sun_color;
	float LightPower = light_intensity;

	// Fixed snow color (white), RGB: (240, 250, 255)
	// It seems that the snow is black in Windows OS if the value is imported via uniform.
	// In this situdation, set it manually in GLSL.
	//vec3 SnowDiffuseColor = snow_color;
	vec3 SnowDiffuseColor = vec3(0.9375, 0.9375, 1.0000);

	vec3 SnowAmbientColor = vec3(0.10, 0.10, 0.10) * SnowDiffuseColor;
	vec3 SnowSpecularColor = vec3(0.2, 0.2, 0.2);
	float SnowSpecularExponent = 25.0f;

	// Calculate color contribution from each light
	vec3 color = vec3(0.0);
	for (int i = 0; i < numLights; i++) {

		// Distorted normal
		vec3 n = normalize(Normal_cameraspace + distortion_scalar * random(Normal_modelspace, 1));
		vec3 l = normalize(LightDirection_cameraspace[i]);
		float cosTheta = clamp(dot(n, l), 0.0, 1.0);

		vec3 E = normalize(EyeDirection_cameraspace);
		vec3 R = reflect(-l, n);
		float cosAlpha = clamp(dot(E, R), 0.0, 1.0);

		// Calculate Ambient, Diffuse, and Specular components
		vec3 Ambient = SnowAmbientColor;
		vec3 Diffuse = SnowDiffuseColor * LightColor * LightPower * cosTheta;
		vec3 Specular = SnowSpecularColor * LightColor * LightPower * pow(cosAlpha, SnowSpecularExponent);
		color += Ambient + Diffuse + Specular;
	}

	return color;
}

/**
 * Calculates the inclination value of a snow surface based on its normal vector and random noise.
 *
 * This function determines the inclination (or slope) of a snow surface relative to the vertical axis. 
 * It uses the dot product between the normalized normal vector of the surface and the up vector (0, 0, 1) 
 * to calculate the cosine of the angle to the vertical. A random noise factor between 0 and 0.4 is added 
 * to the cosine value to simulate natural irregularities in the snow surface.
 *
 * The final inclination value, adjusted by noise, is clamped between 0 and 1. If the dot product is 
 * negative (implying the surface is facing downward or sideways relative to the up vector), the 
 * inclination is set to zero.
 *
 * @param n The normal vector of the snow surface in model space.
 *
 * @return float The inclination value, ranging from 0 (horizontal or downward facing) to 1 (upright).
 */
 
 float inclication(vec3 n){

	// The inclication noise, between 0 and 0.4
	float noise = random(Normal_modelspace, 1) * 0.4;

	// Normalize vectors to simplify the angle calculation.
	n = normalize(n);
	vec3 u = vec3(0, 0, 1);

	// cos(theta) = dot(n, u) / (norm(n) * norm(u))
	float dotn = dot(n, u);

	// If 0 < angle < 90, f_inc = cos(theta) + n, otherwise f_inc = 0
	// Make sure f_inc betweens 0 and 1
	return (dotn > 0) ? min(dotn + noise, 1.0f) : 0.0f;
}

/**
 * Main function for fragment shader to calculate the final color of a fragment with potential snow accumulation.
 *
 * This function performs multiple tasks:
 * - It samples the shadow map multiple times to determine the visibility of the fragment under
 *   consideration, taking into account potential shadowing from multiple light sources.
 * - It calculates the snow accumulation prediction using three factors:
 *   - f_e: The exposure component based on visibility from shadow calculations.
 *   - f_inc: The inclination function that estimates how much snow can accumulate based on the 
 *     surface's orientation.
 *   - f_u: A user-defined scalar factor representing the amount of snow, which must be within [0, 1].
 * - It calculates the colors of the object with and without snow, and blends these colors based
 *   on the computed snow accumulation prediction.
 *
 * The blending uses a linear interpolation between the object color and the snow color, weighted
 * by the snow accumulation prediction factor, to create a realistic depiction of snow-covered surfaces.
 *
 * Outputs:
 * - color: The final color of the fragment, taking into account the potential for snow coverage and shadowing.
 */

void main(){

	float bias = 0.005;
	float visibility = 1.0;

	// Sample the shadow map 4 times
	for (int i=0; i<4; i++){
		float in_shadow = texture(
			shadowMap, 
			vec3(ShadowCoord.xy + poissonDisk[i] / 700.0, (ShadowCoord.z - bias) / ShadowCoord.w)
		);
		
		visibility -= 0.25 * (1.0 - in_shadow);
	}

	// f_e: The exposure component. f_inc: The inclication function. 
	// f_u: a user-defined function to customize/manipulate the snow effect.
	// It can be any function, but the range of it must in [0, 1]
	float f_e = visibility;
	float f_inc = inclication(Normal_modelspace);
	float f_u = snow_amount;

	// Snow accumulation prediction function f_p = f_e * f_inc * f_u
	float f_p = f_e * f_inc * f_u;

	// c_s: The snow color. c_o: The object color without snow
	vec3 c_s = snowColor();
	vec3 c_o = objectColor();

	// The Full snow equation is the blend of those two colors. 
	// i,e,, C = c_s * f_p + c_o * (1 - f_p)
	color = c_s * f_p + c_o * (1.00 - f_p);

	// To get the visibility/color mapping, uncomment the following line of code.
	// color = visibilityColorMapping(visibility);

	// To get the angle/color mapping, uncomment the following line of code.
	// color = angleColorMapping(dot(normalize(Normal_modelspace), vec3(0, 0, 1)));
}