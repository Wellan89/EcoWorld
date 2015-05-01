uniform sampler2D texture0;

uniform float BufferWidth;
uniform float BufferHeight;
uniform float Contrast;

void main()
{
 	vec2 texSize = vec2(1.0/BufferWidth, 1.0/BufferHeight);
    vec4 finalCol;
    finalCol.a = 1.0;
    finalCol.rgb = vec3(0.5);
    finalCol -= texture2D(texture0, gl_TexCoord[0].xy+texSize)*Contrast;
    finalCol += texture2D(texture0, gl_TexCoord[0].xy-texSize)*Contrast;
    finalCol.rgb = vec3((finalCol.r+finalCol.g+finalCol.b)/3.0);
	gl_FragColor = finalCol;
}
