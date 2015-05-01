uniform sampler2D texture0; //SceneBuffer

uniform float BufferWidth;
uniform float BufferHeight;

void main ()
{
	vec2 texSize = vec2(1.0/BufferWidth, 1.0/BufferHeight);
	vec4 average = vec4(0.0);
    for (int v=0; v<4; v++)
	{
		for (int u=0; u<4; u++)
		{
			average += texture2D(texture0, gl_TexCoord[0].xy+texSize*vec2(u, v));
		}
	}
    average *= 0.0625;
    gl_FragColor = average;	
}
