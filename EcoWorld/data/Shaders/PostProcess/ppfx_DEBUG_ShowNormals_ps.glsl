uniform sampler2D texture0;

void main()
{
	gl_FragColor = vec4(texture2D(texture0, gl_TexCoord[0].xy).rgb, 1.0);
}
