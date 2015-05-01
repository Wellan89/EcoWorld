uniform mat4 mWorldViewProj;

void main()
{
	vec4 tPos = mWorldViewProj * gl_Vertex;
	gl_Position = tPos;

	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;
	gl_TexCoord[0].z = tPos.z;
}
