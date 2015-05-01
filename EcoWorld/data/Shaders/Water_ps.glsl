// Modified version

uniform vec3 CameraPosition;
uniform float WaveHeight;
uniform vec4 WaterColor;
uniform float ColorBlendFactor;
uniform sampler2D WaterBump, RefractionMap, ReflectionMap;

varying float WaveHeightPos;

void main()
{
 	// bump color
	vec2 bumpTexCoords = gl_TexCoord[0].xy;
	bumpTexCoords.x += WaveHeightPos;
	bumpTexCoords.y += WaveHeightPos;
	vec2 bumpColor = (texture2D(WaterBump, bumpTexCoords)).rg;
	vec2 perturbation = (bumpColor - 0.5f) * WaveHeight;
	perturbation.x += WaveHeightPos;
	perturbation.y += WaveHeightPos;

	// refraction
	float doubleRefractionInv = 0.5 / gl_TexCoord[1].z;
	vec2 ProjectedRefractionTexCoords;
    ProjectedRefractionTexCoords.x = gl_TexCoord[1].x * doubleRefractionInv + 0.5;
    ProjectedRefractionTexCoords.y = gl_TexCoord[1].y * doubleRefractionInv + 0.5;
    vec4 refractiveColor = texture2D(RefractionMap, ProjectedRefractionTexCoords + perturbation);

	// reflection
	float doubleReflectionInv = 0.5 / gl_TexCoord[2].z;
    vec2 ProjectedReflectionTexCoords;
    ProjectedReflectionTexCoords.x = gl_TexCoord[2].x * doubleReflectionInv + 0.5;
    ProjectedReflectionTexCoords.y = -gl_TexCoord[2].y * doubleReflectionInv + 0.5;
    vec4 reflectiveColor = texture2D(ReflectionMap, ProjectedReflectionTexCoords + perturbation);

 	// fresnel
    vec3 eyeVector = normalize(CameraPosition - gl_TexCoord[3].xyz);
    vec3 normalVector = vec3(0.0, 1.0, 0.0);
    float fresnelTerm = max(dot(eyeVector, normalVector), 0.0);
    vec4 combinedColor = mix(refractiveColor, reflectiveColor * (1.0 - fresnelTerm), 0.25);

    vec4 finalColor = ((ColorBlendFactor * WaterColor) + ((1.0 - ColorBlendFactor) * combinedColor));

	// Ajouté : Calcul du brouillard (nécessaire en GLSL car OpenGL ne l'effectue pas automatiquement, contrairement à DirectX) :
	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fogFactor = clamp((gl_Fog.end - z) * gl_Fog.scale, 0.0, 1.0);
	gl_FragColor = mix(gl_Fog.color, finalColor, fogFactor);
}
