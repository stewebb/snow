#version 330 core

// Interpolated values from the vertex shaders
in vec2 UV;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 Normal_modelspace;


in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;
in vec4 ShadowCoord;

// Output data
layout(location = 0) out vec3 color;

// Values that stay constant for the whole mesh.
uniform sampler2D myTextureSampler;
uniform mat4 MV;
uniform vec3 LightPosition_worldspace;
uniform sampler2DShadow shadowMap;

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
	float LightPower = 1.0f;

	// Material properties
	vec3 MaterialDiffuseColor = texture(myTextureSampler, UV).rgb;
	vec3 MaterialAmbientColor = vec3(0.1, 0.1, 0.1) * MaterialDiffuseColor;
	vec3 MaterialSpecularColor = vec3(0.5, 0.5, 0.5);
	float MaterialSpecularExponent = 150.0f;

	// Normal of the computed fragment, in camera space
	vec3 n = normalize( Normal_cameraspace );

	// Direction of the light (from the fragment to the light)
	vec3 l = normalize( LightDirection_cameraspace );

	// Cosine of the angle between the normal and the light direction, 
	// clamped above 0
	//  - light is at the vertical of the triangle -> 1
	//  - light is perpendicular to the triangle -> 0
	//  - light is behind the triangle -> 0
	float cosTheta = clamp(dot(n, l), 0, 1);

	// Eye vector (towards the camera)
	vec3 E = normalize(EyeDirection_cameraspace);
	
	// Direction in which the triangle reflects the light
	vec3 R = reflect(-l, n);

	// Cosine of the angle between the Eye vector and the Reflect vector,
	// clamped to 0
	//  - Looking into the reflection -> 1
	//  - Looking elsewhere -> < 1
	float cosAlpha = clamp(dot(E, R ),0, 1);

	vec3 Ambient = MaterialAmbientColor;
	vec3 Diffuse = MaterialDiffuseColor * LightColor * LightPower * cosTheta;
	vec3 Specular = MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha, MaterialSpecularExponent);

	return Ambient + Diffuse + Specular;
}

// Calculate the snow color.
vec3 snowColor(){

	vec3 LightColor = vec3(1.0, 1.0, 1.0);
	float LightPower = 1.0f;

	// Fixed snow color (white), RGB: (240, 250, 255)
	vec3 SnowDiffuseColor = vec3(0.9375, 0.9375, 1.0000);
	vec3 SnowAmbientColor = vec3(0.1, 0.1, 0.1) * SnowDiffuseColor;
	vec3 SnowSpecularColor = vec3(0.2, 0.2, 0.2);
	float SnowSpecularExponent = 25.0f;

	float distortion_scalar = 0.1;
	
	// Distorted normal
	vec3 n = normalize(Normal_cameraspace + distortion_scalar * random(Normal_modelspace, 1));
	vec3 l = normalize(LightDirection_cameraspace);
	float cosTheta = clamp(dot(n, l), 0, 1);
	
	vec3 E = normalize(EyeDirection_cameraspace);
	vec3 R = reflect(-l, n);
	float cosAlpha = clamp(dot(E, R),0, 1);

	vec3 Ambient = SnowAmbientColor;
	vec3 Diffuse = SnowDiffuseColor * LightColor * LightPower * cosTheta;
	vec3 Specular = SnowSpecularColor * LightColor * LightPower * pow(cosAlpha, SnowSpecularExponent);

	return Ambient + Diffuse + Specular;

	/*
	color = 
		// Ambient : simulates indirect lighting
		MaterialAmbientColor +
		// Diffuse : "color" of the object
		visibility * MaterialDiffuseColor * LightColor * LightPower * cosTheta+
		// Specular : reflective highlight, like a mirror
		visibility * MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha, SpecularExponent);
	*/
}

void main(){

	/*
	// Light emission properties
	vec3 LightColor = vec3(1.0, 1.0, 1.0);
	float LightPower = 1.0f;
	
	// Material properties
	vec3 MaterialDiffuseColor = texture(myTextureSampler, UV).rgb;
	vec3 MaterialAmbientColor = vec3(0.1, 0.1, 0.1) * MaterialDiffuseColor;
	vec3 MaterialSpecularColor = vec3(0.5, 0.5, 0.5);
	float SpecularExponent = 150.0f;

	// Distance to the light
	//float distance = length( LightPosition_worldspace - Position_worldspace );

	// Normal of the computed fragment, in camera space
	vec3 n = normalize( Normal_cameraspace );
	// Direction of the light (from the fragment to the light)
	vec3 l = normalize( LightDirection_cameraspace );
	// Cosine of the angle between the normal and the light direction, 
	// clamped above 0
	//  - light is at the vertical of the triangle -> 1
	//  - light is perpendicular to the triangle -> 0
	//  - light is behind the triangle -> 0
	float cosTheta = clamp( dot( n,l ), 0,1 );
	
	// Eye vector (towards the camera)
	vec3 E = normalize(EyeDirection_cameraspace);
	// Direction in which the triangle reflects the light
	vec3 R = reflect(-l,n);
	// Cosine of the angle between the Eye vector and the Reflect vector,
	// clamped to 0
	//  - Looking into the reflection -> 1
	//  - Looking elsewhere -> < 1
	float cosAlpha = clamp( dot( E,R ), 0,1 );
	
	float visibility=1.0;

	// Fixed bias, or...
	float bias = 0.005;

	// ...variable bias
	// float bias = 0.005*tan(acos(cosTheta));
	// bias = clamp(bias, 0,0.01);

	// Sample the shadow map 4 times
	for (int i=0;i<4;i++){
		// use either :
		//  - Always the same samples.
		//    Gives a fixed pattern in the shadow, but no noise
		int index = i;
		//  - A random sample, based on the pixel's screen location. 
		//    No banding, but the shadow moves with the camera, which looks weird.
		// int index = int(16.0*random(gl_FragCoord.xyy, i))%16;
		//  - A random sample, based on the pixel's position in world space.
		//    The position is rounded to the millimeter to avoid too much aliasing
		// int index = int(16.0*random(floor(Position_worldspace.xyz*1000.0), i))%16;
		
		// being fully in the shadow will eat up 4*0.2 = 0.8
		// 0.2 potentially remain, which is quite dark.
		
		//float in_shadow = texture(shadowMap, vec3(ShadowCoord.xy + poissonDisk[index]/700.0,  (ShadowCoord.z-bias)/ShadowCoord.w));
		
		//if(in_shadow == 1.0){
		//	MaterialDiffuseColor = vec3(0.96,0.96,1);
		//}
		//visibility -= 0.2*(1.0-in_shadow);

	}
	vec3 nn = normalize(Normal_modelspace);
	vec3 uu = vec3(0, 0, 1);
	float dotn = dot(nn, uu);
	MaterialDiffuseColor = angleColorMapping(dotn);
	
	// For spot lights, use either one of these lines instead.
	// if ( texture( shadowMap, (ShadowCoord.xy/ShadowCoord.w) ).z  <  (ShadowCoord.z-bias)/ShadowCoord.w )
	// if ( textureProj( shadowMap, ShadowCoord.xyw ).z  <  (ShadowCoord.z-bias)/ShadowCoord.w )
	
	color = 
		// Ambient : simulates indirect lighting
		MaterialAmbientColor +
		// Diffuse : "color" of the object
		visibility * MaterialDiffuseColor * LightColor * LightPower * cosTheta+
		// Specular : reflective highlight, like a mirror
		visibility * MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha, SpecularExponent);
	*/

	//color = snowColor();
	color = objectColor();
}