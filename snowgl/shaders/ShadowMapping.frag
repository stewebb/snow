#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 Normal_modelspace;


in vec3 EyeDirection_cameraspace;
//in vec3 LightDirection_cameraspace;
in vec3 LightDirection_cameraspace[6];

in vec4 ShadowCoord;

// Output data
layout(location = 0) out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D myTextureSampler;
uniform mat4 MV;
uniform vec3 LightPosition_worldspace;
uniform sampler2DShadow shadowMap;
uniform int numLights;

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

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

// Maps the cosine of an angle (measured in degrees) to a specific color 
// to visually represent the angle's inclination.
vec3 angleColorMapping(float dotn) {

	// [90, 180] -> Red
    if (dotn <= 0.0000) {
        return vec3(1.00, 0.50, 0.50);
    } 
	
	// [75, 90) -> Orange
	else if (dotn <= 0.2588) {
        return vec3(1.00, 0.65, 0.50);
    } 
	
	// [60, 75) -> Yellow
	else if (dotn <= 0.5) {
        return vec3(1.0, 1.0, 0.6);
    } 
	
	// [45, 60) -> Green
	else if (dotn <= 0.7071) {
        return vec3(0.60, 1.00, 0.60);
    } 
	
	// [30, 45) -> Blue
	else if (dotn <= 0.8660) {
        return vec3(0.50, 0.70, 1.00);
    } 
	
	// [15, 30) -> Indigo
	else if (dotn <= 0.9659) {
        return vec3(0.40, 0.40, 0.70);
    } 

	// [0, 15) -> Violet
	else {
        return vec3(0.8, 0.6, 1.0);
    }
}

// Calculate the object color without snow effect.
vec3 objectColor(){

	// Light emission properties
	vec3 LightColor = vec3(1.0, 1.0, 1.0);
	float LightPower = 0.167f;

	// Material properties
	vec3 MaterialDiffuseColor = texture(myTextureSampler, UV).rgb;
	vec3 MaterialAmbientColor = vec3(0.1, 0.1, 0.1) * MaterialDiffuseColor;
	vec3 MaterialSpecularColor = vec3(0.5, 0.5, 0.5);
	float MaterialSpecularExponent = 150.0f;

	// Normal of the computed fragment, in camera space
	vec3 n = normalize(Normal_cameraspace);

	// Eye vector (towards the camera)
	vec3 E = normalize(EyeDirection_cameraspace);

	vec3 color = vec3(0.0); // Initialize final color

	// Calculate color contribution from each light
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


// Calculate the snow color.
vec3 snowColor(){

	vec3 LightColor = vec3(1.0, 1.0, 1.0);
	float LightPower = 0.167f;

	// Fixed snow color (white), RGB: (240, 250, 255)
	vec3 SnowDiffuseColor = vec3(0.9375, 0.9375, 1.0000);
	vec3 SnowAmbientColor = vec3(0.1, 0.1, 0.1) * SnowDiffuseColor;
	vec3 SnowSpecularColor = vec3(0.2, 0.2, 0.2);
	float SnowSpecularExponent = 25.0f;

	float distortion_scalar = 0.1;
	vec3 color = vec3(0.0);

	// Calculate color contribution from each light
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


// Calculate the inclication value of a snow surface.
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

void main(){

	// Fixed bias, or...
	float bias = 0.005;

	// ...variable bias
	// float bias = 0.005*tan(acos(cosTheta));
	// bias = clamp(bias, 0,0.01);


	float visibility=1.0;

	// Sample the shadow map 4 times
	for (int i=0; i<4; i++){
		float in_shadow = texture(
			shadowMap, 
			vec3(ShadowCoord.xy + poissonDisk[i] / 700.0, (ShadowCoord.z - bias) / ShadowCoord.w)
		);
		
		visibility -= 0.25 * (1.0 - in_shadow);
	}

	// f_e is the exposure component.
	float f_e = visibility;

	// f_inc is the inclication function.
	float f_inc = inclication(Normal_modelspace);

	
	// f_u is a user-defined function to customize/manipulate the snow effect.
	// It can be any function with any domain, but the range of it must in [0, 1]
	float f_u = 0.00;

	// Snow accumulation prediction function f_p = f_e * f_inc * f_u
	float f_p = f_e * f_inc * f_u;

	color = snowColor() * f_p + objectColor() * (1.00-f_p);

	//color = objectColor();

}