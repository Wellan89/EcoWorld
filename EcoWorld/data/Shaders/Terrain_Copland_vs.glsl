uniform float TerrainHeight;
uniform vec3 lightDirection;
uniform vec3 diffuseColor;

void main()
{
	gl_Position = ftransform();

	float diffuseFactor = max(dot(gl_Normal, lightDirection), 0.0);
	gl_FrontColor = vec4(diffuseColor * diffuseFactor, gl_Vertex.y / TerrainHeight);

	gl_TexCoord[0].xy = gl_MultiTexCoord1.xy;
}
