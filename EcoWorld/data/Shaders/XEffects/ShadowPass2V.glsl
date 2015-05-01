uniform mat4 mWorldViewProj;
uniform mat4 mWorldViewProj2;
uniform vec3 LightDir;

void main()
{
	gl_Position = (mWorldViewProj * gl_Vertex);

	vec4 ShadowMapSamplingPos = (mWorldViewProj2 * gl_Vertex) * 0.5;
	gl_TexCoord[0] = ShadowMapSamplingPos;

	gl_TexCoord[1].x = max(dot(gl_Normal.xyz, LightDir), 0.0);
}
