#version 150

in vec2 texCoord;

in vec3 fN;
in vec3 R;
in vec3 T;
flat in vec3 flatfN;
in vec3 fE;
in vec3 fL;
uniform sampler2D texture;
uniform samplerCube texMap;

out vec4 fColor;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;
uniform bool UseFlatShading;
uniform bool UseTexture;
uniform bool UseReflection;
uniform bool UseRefraction;

void main() 
{ 
	vec3 N = normalize(fN);
	vec3 E = normalize(fE);
	vec3 L = normalize(fL);

	if(UseFlatShading){
		N = normalize(flatfN);
	}

	vec3 H = normalize(L + E);
	vec4 ambient = AmbientProduct;
	float Kd = max(dot(L, N), 0.0);
	vec4 diffuse = Kd*DiffuseProduct;
	float Ks = pow(max(dot(N, H), 0.0), Shininess);
	vec4 specular = Ks*SpecularProduct;
	// discard the specular highlight if the light's behind the vertex
	if( dot(L, N) < 0.0 )
		specular = vec4(0.0, 0.0, 0.0, 1.0);

	fColor = ambient + diffuse + specular;
	fColor.a = 1.0;

	if(UseTexture){
		fColor = fColor * texture2D( texture, texCoord );
		fColor.a = 1.0;
	}
	if(UseReflection){
		fColor = textureCube( texMap, R );
	}

	if(UseRefraction){
		vec4 refractColor = textureCube(texMap, T); 
		refractColor = mix(refractColor, vec4(1,1,1,1), 0.3);
		fColor = refractColor;
	}
} 
