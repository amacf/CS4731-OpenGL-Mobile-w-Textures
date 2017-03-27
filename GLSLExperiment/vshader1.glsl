#version 150

uniform mat4 ctm;
uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform vec4 LightPosition;
uniform float Shininess;
uniform bool UseFlatShading;
uniform float iorefr;

in vec4 vPosition;
in vec4 vNormal;

out vec3 fN;
flat out vec3 flatfN;
out vec3 fE;
out vec3 fL;
out vec3 R;
out vec3 T;

in  vec4 vTexCoord;
out vec2 texCoord;

void main() 
{
	texCoord = vTexCoord.st;

	vec4 pos = ctm * vPosition;
	vec4 norm = ctm * vNormal;
	fN = norm.xyz;
	flatfN = norm.xyz;
	fE = -pos.xyz;
	fL = LightPosition.xyz;

	if(LightPosition.w != 0.0){
		fL = LightPosition.xyz - pos.xyz;
	}
  
	gl_Position = pos;

	vec3 eyePos = pos.xyz;
	vec3 N = normalize(norm.xyz);
	R = reflect(eyePos,N);

	T = refract(eyePos.xyz,N,iorefr);
} 
