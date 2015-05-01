// nvidia shader library
// http://developer.download.nvidia.com/shaderlibrary/webpages/shader_library.html

uniform float Sharpness;
uniform float Shake;
uniform float RandomValue;

void main()
{
	gl_Position =  gl_Vertex * 2.0 - 1.0;
	gl_TexCoord[0]  = gl_Vertex;
	
    vec2 i = Shake*vec2(RandomValue, RandomValue);
    vec2 j = abs(i);
    vec2 pj = vec2(pow(float(j), Sharpness));
    vec2 si = sign(i);
    vec2 shook = si * pj;
	gl_TexCoord[0].xy -= shook;
}
