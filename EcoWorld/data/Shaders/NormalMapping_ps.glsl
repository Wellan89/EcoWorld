uniform vec3 diffuseColor;
uniform vec3 ambientColor;
uniform float fogEnabled;

uniform sampler2D baseMap;
uniform sampler2D bumpMap;

void main()
{
	// Fonction d'éclairage avec le normal mapping :
	vec3 LightDir = gl_TexCoord[1].xyz;
	vec3 normal = normalize((texture2D(bumpMap, gl_TexCoord[0].xy).xyz * 2.0) - 1.0);
	float diffuseFactor = max(dot(normal, normalize(LightDir)), 0.0);
	vec3 totalDiffuseColor = diffuseColor * diffuseFactor;

	vec4 baseColor = texture2D(baseMap, gl_TexCoord[0].xy);
	vec4 finalColor = vec4((ambientColor + totalDiffuseColor) * baseColor.rgb, baseColor.a);

	// Calcul du brouillard (nécessaire en GLSL car OpenGL ne l'effectue pas automatiquement, contrairement à DirectX) :
	//if (fogEnabled > 0.0)
	if (fogEnabled != 0.0)
	{
		float z = gl_FragCoord.z / gl_FragCoord.w;
		float fogFactor = clamp((gl_Fog.end - z) * gl_Fog.scale, 0.0, 1.0);
		gl_FragColor = mix(gl_Fog.color, finalColor, fogFactor);
	}
	else
	{
		gl_FragColor = finalColor;
	}
}
