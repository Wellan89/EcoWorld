// Modified version

uniform mat4 View;
uniform mat4 WorldReflectionViewProj;
uniform float WaveLengthInv;
uniform vec2 WindTime;

varying float WaveHeightPos;

void main()
{
	WaveHeightPos = gl_Vertex.y * 0.02f;

    gl_Position = ftransform();
    gl_TexCoord[0].xy = gl_MultiTexCoord0.xy * WaveLengthInv + WindTime;
    gl_TexCoord[1].xyz = (gl_ModelViewProjectionMatrix * gl_Vertex).xyw;
    gl_TexCoord[2].xyz = (WorldReflectionViewProj * gl_Vertex).xyw;
    gl_TexCoord[3].xyz = (View * gl_Vertex).xyz;
}
